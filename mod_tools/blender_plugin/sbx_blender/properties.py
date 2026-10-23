"""The sbx_* tags on Blender objects and materials, in one place.

They point back into the sidecar, kept in the .blend as two text blocks
per part: '<part>.sbx.json' (readable) and '.<part>.sbx.raw' (the raw
half export needs, compressed; the leading dot hides it from Blender's
lists). Add-on 0.1.2 and 0.1.3 named the raw block '<part>.sbx.raw.json'
and stored it as plain JSON; sidecar_raw_text finds either. They're keyed by stable things: part name,
shape index, original shape name, draw index. Nothing else reads or
writes these names directly.

Objects:
- part root (empty): sbx_part, sbx_sidecar, sbx_sidecar_raw, sbx_group,
  sbx_attach
- shape (mesh): sbx_part, sbx_shape_index, sbx_shape_name, sbx_normals
  (a fingerprint of the corner normals as imported, so the export can
  tell when only the normals were edited)
- node (empty): sbx_node_file, sbx_node
- marker (empty): sbx_part, sbx_marker
- car root (empty): sbx_node_file, sbx_set, sbx_car (the car's name)

Materials: sbx_key (the material's identity, see materials.py),
sbx_record ('part:shape:draw' of the record it was made from), sbx_shader (the part shader pair's library hashes, 'vs:ps'),
sbx_shared_shader, sbx_textures (XPR2 list index and texture index of
each bound texture, 'xpr:index,...'), sbx_texture_names, sbx_xpr.
"""

SIDECAR_SUFFIX = '.sbx.json'
SIDECAR_RAW_SUFFIX = '.sbx.raw'
OLD_RAW_SUFFIX = '.sbx.raw.json'   # add-on 0.1.2 and 0.1.3


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


def tag_part_root(obj, part_name, group='', attach=''):
    obj['sbx_part'] = part_name
    obj['sbx_sidecar'] = sidecar_text_name(part_name)
    obj['sbx_sidecar_raw'] = sidecar_raw_text_name(part_name)
    obj['sbx_group'] = group
    obj['sbx_attach'] = attach


def tag_shape(obj, part_name, shape_index, shape_name):
    obj['sbx_part'] = part_name
    obj['sbx_shape_index'] = shape_index
    obj['sbx_shape_name'] = shape_name


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


def record_key(part_name, shape_index, draw_index):
    return '%s:%d:%d' % (part_name, shape_index, draw_index)


def shape_ref(obj):
    """(part, shape index, original name) of a shape object, or None."""
    if 'sbx_part' in obj and 'sbx_shape_index' in obj:
        return obj['sbx_part'], int(obj['sbx_shape_index']), obj.get('sbx_shape_name', '')
    return None
