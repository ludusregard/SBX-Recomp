"""Blender materials for display: one per distinct material.

Two draws share a material when their records match in everything but
where their indices sit and which slot of their own part's shader list
the shader is in (the shader pair itself, by its library hash, must
match). Fields not worked out count as real differences. Materials and
textures already in the .blend from an earlier import are reused (their
sbx_key and sbx_xpr tags).

Each material shows the record's first bound texture (from the XPR2 the
part's texture indices point into), the diffuse color, and a roughness
from the specular power. It's only a preview:
every shape keeps one material slot per draw, and export takes each
draw's record from the sidecar through the slot.

Names: the bound textures' names ('sky35_ex', 'sky35_ex+tex49'), then
the shared shader's role when there is one (paint, bonnet, cover, trim;
docs/game_data.md, "Part files") and 'blend' for blended draws.
"""

import hashlib
import os

import bpy
import numpy as np

from . import properties
from .sbx_formats import xpr_textures
from .log import get_logger
from .sbx_formats import layout

log = get_logger(__name__)

# Fields that only place a draw's indices, and the part-local shader
# index (compared by the pair's hash instead): two records that differ
# only in these are the same material.
_IGNORED_FIELDS = ('index_count', 'primitive_count', 'index_offset', 'index_size',
                   'index_file_offset', 'shader')



def material_key(record, shader_pair, xpr_name):
    """A short hash naming a material: same hash, same material."""
    b = bytearray(record.raw)
    for name in _IGNORED_FIELDS:
        _, off, fmt = record.field(name)
        b[off:off + 4] = b'\0\0\0\0'
    h = hashlib.sha1(bytes(b))
    h.update(('|%s|%s' % (shader_pair or '', xpr_name or '')).encode())
    return h.hexdigest()[:16]


def is_glow(record):
    """A light glow plane: +0x074 set, blended, diffuse alpha 0
    (docs/game_data.md, material record +0x074). Its texture has a black
    background; shown additive, black adding nothing (inferred)."""
    return (int.from_bytes(record.get('unknown_074')[0:4], 'big') == 1 and record.get('blend')
            and record.get('diffuse')[3] == 0.0)


def material_name(texture_names, record):
    name = '+'.join(texture_names) if texture_names else 'untextured'
    shared = record.get('shared_shader')
    if shared:
        name += ' ' + layout.SHARED_SHADER_ROLES.get(shared, 'shared%d' % shared)
    if is_glow(record):
        name += ' glow'
    elif record.get('blend'):
        name += ' blend'
    return name


class TextureCache:
    """XPR2 files read once; textures made into Blender images once."""

    def __init__(self):
        self.xprs = {}     # path -> (data, texture list) or None
        self.images = {}   # (path, index) -> bpy image or None

    def xpr(self, path):
        if not path:
            return None
        if path not in self.xprs:
            self.xprs[path] = None
            if os.path.exists(path):
                with open(path, 'rb') as f:
                    data = f.read()
                try:
                    self.xprs[path] = (data, xpr_textures.read_XPR(data))
                    log.info('read %s: %d textures', os.path.basename(path), len(self.xprs[path][1]))
                except Exception as e:
                    log.warning('%s: not readable as XPR2: %s', os.path.basename(path), e)
            else:
                log.warning('XPR2 not found: %s', path)
        return self.xprs[path]

    def name(self, path, index):
        x = self.xpr(path)
        if x and 0 <= index < len(x[1]):
            return x[1][index]['name']
        return 'tex%d' % index

    def names(self, path):
        x = self.xpr(path)
        return [t['name'] for t in x[1]] if x else None

    def image(self, path, index):
        key = (path, index)
        if key in self.images:
            return self.images[key]
        self.images[key] = None
        base = os.path.basename(path) if path else ''
        for img in bpy.data.images:   # made by an earlier import
            if img.get('sbx_xpr') == base and img.get('sbx_texture_index') == index:
                self.images[key] = img
                return img
        x = self.xpr(path)
        if not x or not 0 <= index < len(x[1]) or x[1][index]['type'] != 'TX2D':
            return None
        tex = x[1][index]
        try:
            rgba = xpr_textures.decode(x[0], tex)
        except Exception as e:
            log.warning('%s: texture %s not decoded: %s', os.path.basename(path), tex['name'], e)
            return None
        h, w = rgba.shape[:2]
        img = bpy.data.images.new(tex['name'], w, h, alpha=True)
        img.pixels.foreach_set((rgba[::-1].astype(np.float32) / 255.0).ravel())
        img['sbx_xpr'] = os.path.basename(path)
        img['sbx_texture_index'] = index
        try:
            img.pack()
        except Exception as e:
            log.warning('image %s not packed: %s', img.name, e)
        self.images[key] = img
        return img


class MaterialMaker:
    def __init__(self, textures):
        self.textures = textures
        self.made = {}   # (xpr path, key) -> material

    def get(self, record, xpr_path, record_ref, shader_pair):
        xpr_name = os.path.basename(xpr_path) if xpr_path else ''
        key = material_key(record, shader_pair, xpr_name)
        if key in self.made:
            return self.made[key]
        mat = next((m for m in bpy.data.materials if m.get('sbx_key') == key), None)
        if mat is not None:
            log.debug('material %s: reused (%s)', mat.name, record_ref)
            self.made[key] = mat
            return mat
        tex = record.textures
        names = [self.textures.name(xpr_path, t) for _, t in tex]
        mat = bpy.data.materials.new(material_name(names, record))
        mat['sbx_key'] = key
        properties.tag_material(mat, record_ref, shader_pair, record.get('shared_shader'),
                                tex, names, xpr_name)
        self._nodes(mat, record, xpr_path)
        log.debug('material %s: made from %s', mat.name, record_ref)
        self.made[key] = mat
        return mat

    def _nodes(self, mat, record, xpr_path):
        mat.use_nodes = True
        nt = mat.node_tree
        bsdf = next((n for n in nt.nodes if n.type == 'BSDF_PRINCIPLED'), None)
        if bsdf is None:
            return
        diffuse = [min(max(c, 0.0), 1.0) for c in record.get('diffuse')]
        bsdf.inputs['Base Color'].default_value = diffuse
        bsdf.inputs['Metallic'].default_value = 0.0
        power = record.get('power')
        bsdf.inputs['Roughness'].default_value = float(np.sqrt(2.0 / (power + 2.0))) if power > 0 else 1.0
        tex = record.textures
        img = self.textures.image(xpr_path, tex[0][1]) if tex else None
        if is_glow(record):
            self._glow_nodes(mat, img, diffuse)
            return
        if img is not None:
            node = nt.nodes.new('ShaderNodeTexImage')
            node.image = img
            node.location = (-400, 300)
            nt.links.new(node.outputs['Color'], bsdf.inputs['Base Color'])
        blended = record.get('blend') or diffuse[3] < 1.0
        if blended:
            if img is not None:
                if diffuse[3] < 1.0:
                    mul = nt.nodes.new('ShaderNodeMath')
                    mul.operation = 'MULTIPLY'
                    mul.inputs[1].default_value = diffuse[3]
                    mul.location = (-150, 0)
                    nt.links.new(node.outputs['Alpha'], mul.inputs[0])
                    nt.links.new(mul.outputs[0], bsdf.inputs['Alpha'])
                else:
                    nt.links.new(node.outputs['Alpha'], bsdf.inputs['Alpha'])
            else:
                bsdf.inputs['Alpha'].default_value = diffuse[3]
            if hasattr(mat, 'surface_render_method'):
                mat.surface_render_method = 'BLENDED'

    def _glow_nodes(self, mat, img, diffuse):
        """Additive: transparent plus the texture as emission."""
        nt = mat.node_tree
        for n in list(nt.nodes):
            if n.type != 'OUTPUT_MATERIAL':
                nt.nodes.remove(n)
        out = next(n for n in nt.nodes if n.type == 'OUTPUT_MATERIAL')
        emit = nt.nodes.new('ShaderNodeEmission')
        emit.location = (-200, 100)
        emit.inputs['Color'].default_value = diffuse[:3] + [1.0]
        if img is not None:
            node = nt.nodes.new('ShaderNodeTexImage')
            node.image = img
            node.location = (-500, 100)
            nt.links.new(node.outputs['Color'], emit.inputs['Color'])
        clear = nt.nodes.new('ShaderNodeBsdfTransparent')
        clear.location = (-200, -100)
        add = nt.nodes.new('ShaderNodeAddShader')
        add.location = (0, 0)
        nt.links.new(clear.outputs[0], add.inputs[0])
        nt.links.new(emit.outputs[0], add.inputs[1])
        nt.links.new(add.outputs[0], out.inputs['Surface'])
        if hasattr(mat, 'surface_render_method'):
            mat.surface_render_method = 'BLENDED'
