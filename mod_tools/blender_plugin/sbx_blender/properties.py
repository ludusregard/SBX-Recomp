"""The sbx_* tags on Blender objects and materials, in one place.

They point back into the sidecar, kept in the .blend as two text blocks
per part: '<part>.sbx.json' (readable) and '.<part>.sbx.raw' (the raw
half export needs, compressed; the leading dot hides it from Blender's
lists). Add-on 0.1.2 and 0.1.3 named the raw block '<part>.sbx.raw.json'
and stored it as plain JSON; sidecar_raw finds either. They're keyed by
stable things: part name, shape index, original shape name, draw index.
Nothing else reads or writes these names directly.

Objects:
- part root (empty): sbx_part, sbx_sidecar, sbx_sidecar_raw, sbx_group,
  sbx_attach, sbx_xpr (the XPR2 its texture indices point into),
  sbx_xpr_names (that XPR2's texture names, one per line)
- shape (mesh): sbx_part, sbx_shape_index, sbx_shape_name, sbx_normals
  (a fingerprint of the corner normals as imported, so the export can
  tell when only the normals were edited)
- new shape (mesh, added with Add to Part): sbx_part, sbx_new_shape (its
  name in the part)
- node (empty): sbx_node_file, sbx_node
- marker (empty): sbx_part, sbx_marker
- new marker (empty, added with Add to Part): sbx_part, sbx_new_marker
- car root (empty): sbx_node_file, sbx_set, sbx_car (the car's name)

Materials: sbx_key (the material's identity, see materials.py),
sbx_record ('part:shape:draw' of the record it was made from),
sbx_shader (the shader pair's library hashes, 'vs:ps'),
sbx_shared_shader, sbx_textures (XPR2 list index and texture index of
each bound texture, 'xpr:index,...'), sbx_texture_names, sbx_xpr.

A new material (made with Make SBX Material) has sbx_new, sbx_shader,
sbx_template (the record it starts from: 'part:shape:draw' of an
imported part, or 'library:<pair>:<example>'), sbx_textures, and the
values its draws start from: sbx_blend, sbx_diffuse, sbx_ambient,
sbx_specular, sbx_power, sbx_params, sbx_flags, sbx_shared_shader
(each draw's values are then edited in the part's readable sidecar).
"""

import re

SIDECAR_SUFFIX = '.sbx.json'
SIDECAR_RAW_SUFFIX = '.sbx.raw'
OLD_RAW_SUFFIX = '.sbx.raw.json'   # add-on 0.1.2 and 0.1.3

# The record fields a new material sets, with their custom property names.
NEW_MATERIAL_FIELDS = ('blend', 'diffuse', 'ambient', 'specular', 'power', 'params', 'flags', 'shared_shader')

_BLENDER_SUFFIX = re.compile(r'\.\d{3}$')


def game_name(name):
    """A Blender name without its '.001'-style suffix."""
    return _BLENDER_SUFFIX.sub('', name)


def sidecar_text_name(part_name):
    return part_name + SIDECAR_SUFFIX


def sidecar_raw_text_name(part_name):
    return '.' + part_name + SIDECAR_RAW_SUFFIX


def sidecar_raw(texts, part_name):
    """The raw half of a part's sidecar from the .blend's text blocks, or None."""
    import json
    from .sbx_formats import sidecar
    t = texts.get(sidecar_raw_text_name(part_name))
    if t is not None:
        return sidecar.raw_from_blob_text(t.as_string())
    t = texts.get(part_name + OLD_RAW_SUFFIX)
    return json.loads(t.as_string()) if t is not None else None


def tag_part_root(obj, part_name, group='', attach='', xpr_name='', xpr_names=()):
    obj['sbx_part'] = part_name
    obj['sbx_sidecar'] = sidecar_text_name(part_name)
    obj['sbx_sidecar_raw'] = sidecar_raw_text_name(part_name)
    obj['sbx_group'] = group
    obj['sbx_attach'] = attach
    obj['sbx_xpr'] = xpr_name or ''
    obj['sbx_xpr_names'] = '\n'.join(xpr_names or ())


def is_part_root(obj):
    return obj is not None and obj.type == 'EMPTY' and 'sbx_part' in obj and 'sbx_sidecar' in obj


def xpr_names(root):
    """The texture names of a part root's XPR2 (empty if not known)."""
    s = root.get('sbx_xpr_names', '') if root is not None else ''
    return s.split('\n') if s else []


def tag_shape(obj, part_name, shape_index, shape_name):
    obj['sbx_part'] = part_name
    obj['sbx_shape_index'] = shape_index
    obj['sbx_shape_name'] = shape_name


def tag_new_shape(obj, part_name, shape_name):
    for k in ('sbx_shape_index', 'sbx_shape_name', 'sbx_normals', 'sbx_marker', 'sbx_new_marker'):
        obj.pop(k, None)
    obj['sbx_part'] = part_name
    obj['sbx_new_shape'] = shape_name


def tag_new_marker(obj, part_name, name):
    for k in ('sbx_marker', 'sbx_new_shape'):
        obj.pop(k, None)
    obj['sbx_part'] = part_name
    obj['sbx_new_marker'] = name


def untag_new(obj):
    for k in ('sbx_part', 'sbx_new_shape', 'sbx_new_marker'):
        obj.pop(k, None)


def normals_fingerprint(me):
    """A short hash of a mesh's corner normals, rounded to 1e-3."""
    import hashlib
    import numpy as np
    n = np.zeros(len(me.loops) * 3, np.float32)
    me.corner_normals.foreach_get('vector', n)
    q = np.round(n.astype(np.float64) * 1000.0).astype(np.int32)
    return hashlib.sha1(q.tobytes()).hexdigest()[:16]


def tag_node(obj, node_file, index):
    obj['sbx_node_file'] = node_file
    obj['sbx_node'] = index


def tag_marker(obj, part_name, name):
    obj['sbx_part'] = part_name
    obj['sbx_marker'] = name


def tag_car_root(obj, node_file, car_set):
    obj['sbx_node_file'] = node_file
    obj['sbx_set'] = car_set


def tag_material(mat, record, shader_pair, shared_shader, textures, texture_names, xpr_name):
    mat['sbx_record'] = record
    mat['sbx_shader'] = shader_pair or ''
    mat['sbx_shared_shader'] = shared_shader
    mat['sbx_textures'] = ','.join('%d:%d' % t for t in textures)
    mat['sbx_texture_names'] = ','.join(texture_names)
    mat['sbx_xpr'] = xpr_name or ''


def tag_new_material(mat, template, shader_pair, record, textures, texture_names, xpr_name):
    """A new material: template ref, pair key, its record's values to start
    from (part_model.Draw), the chosen textures [(xpr, index)]."""
    for k in ('sbx_key', 'sbx_record'):
        mat.pop(k, None)
    mat['sbx_new'] = 1
    mat['sbx_template'] = template
    mat['sbx_shader'] = shader_pair
    mat['sbx_textures'] = ','.join('%d:%d' % t for t in textures)
    mat['sbx_texture_names'] = ','.join(texture_names)
    mat['sbx_xpr'] = xpr_name or ''
    for name in NEW_MATERIAL_FIELDS:
        v = record.get(name)
        mat['sbx_' + name] = list(v) if isinstance(v, tuple) else v


def is_sbx_material(mat):
    return mat is not None and ('sbx_record' in mat or 'sbx_new' in mat)


def parse_textures(s):
    """'0:12,0:49' -> [(0, 12), (0, 49)]."""
    out = []
    for t in (s or '').split(','):
        if t:
            x, i = t.split(':')
            out.append((int(x), int(i)))
    return out


def record_key(part_name, shape_index, draw_index):
    return '%s:%d:%d' % (part_name, shape_index, draw_index)


def shape_ref(obj):
    """(part, shape index, original name) of a shape object, or None."""
    if 'sbx_part' in obj and 'sbx_shape_index' in obj:
        return obj['sbx_part'], int(obj['sbx_shape_index']), obj.get('sbx_shape_name', '')
    return None


def new_shape_ref(obj):
    """(part, name) of a new shape object, or None."""
    if obj.type == 'MESH' and 'sbx_part' in obj and 'sbx_new_shape' in obj:
        return obj['sbx_part'], str(obj['sbx_new_shape'])
    return None


def new_marker_ref(obj):
    """(part, name) of a new marker empty, or None."""
    if obj.type == 'EMPTY' and 'sbx_part' in obj and 'sbx_new_marker' in obj:
        return obj['sbx_part'], str(obj['sbx_new_marker'])
    return None
