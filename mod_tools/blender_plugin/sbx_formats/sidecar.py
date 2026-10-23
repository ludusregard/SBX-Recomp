"""The sidecar: everything in a PART that Blender can't hold, as JSON.

One sidecar per imported part, in two halves:

- the readable half (00002882.sbx.json): the part as a tree to read and
  check: each shape with its streams and draws (material name, textures
  by name, shader by id, colors, flags, and under unknown_fields any
  field not worked out whose value isn't the usual one, see USUAL), then
  its markers, nodes and shader pairs.
- the raw half: every record and section, the original geometry and its
  hashes. This is what makes an unedited part write back identical.
  It's JSON in memory; stored, it's zlib-compressed (raw_blob): a binary
  file next to the readable one on disk (00002882.sbx.raw), base64 text
  inside a .blend.

The export compares Blender's geometry with the original: a shape that
wasn't edited is written from the original bytes (Blender can't hold the
strip order, the half-float normals or the stream types not worked out
exactly). Everything is keyed by stable things (shape index, draw index,
original names), never by Blender names.

    sc = to_sidecar(part, name='00002882')
    readable, raw = split(sc)
    ...
    part = from_sidecar(join(readable, raw), geometry)

geometry is one entry per shape: {'streams': [Stream or None] x 3,
'indices': [index buffer bytes per draw]}. geometry_of(part) makes it
from a model; stored_geometry(sc) gives the original from the sidecar,
and from_sidecar uses that when geometry is None.
"""

import base64
import hashlib
import json
import zlib

import numpy as np

from . import layout, node, shaders, streams as streams_mod
from .part_model import Draw, MaterialBlock, PARTFile, PARTHeader, PartNode, Shape, SkinArray
from .streams import Stream
from .strips import strip_to_triangles

FORMAT = 'sbx-part-sidecar'
VERSION = 2


def b64(b):
    return base64.b64encode(bytes(b)).decode('ascii')


def unb64(s):
    return base64.b64decode(s)


def b64z(b):
    return base64.b64encode(zlib.compress(bytes(b), 6)).decode('ascii')


def unb64z(s):
    return zlib.decompress(base64.b64decode(s))


def sha256(b):
    return hashlib.sha256(bytes(b)).hexdigest()


def stream_hash(st):
    return sha256(st.raw.tobytes()) if st is not None else None


def _plain(v):
    """A field value for the readable half."""
    if isinstance(v, bytes):
        if not any(v):
            return 'zero'
        body = v.rstrip(b'\0')
        body = body + b'\0' * (-len(body) % 4)   # whole words
        rest = len(v) - len(body)
        return body.hex() + (', then %d zero bytes' % rest if rest else '')
    if isinstance(v, tuple):
        return [round(x, 6) if isinstance(x, float) else x for x in v]
    if isinstance(v, float):
        return round(v, 6)
    return v


# The value nearly every draw has in each field not worked out (over all
# 27,402 draws: 67% for unknown_000, 81-100% for the rest). The readable
# half lists a field only when a draw's value differs.
USUAL = {
    'unknown_000': 0,
    'unknown_038': 'zero',
    'unknown_064': 0,
    'unknown_06c': 3,
    'unknown_074': '000000000000000000000000ffffffff, then 4 zero bytes',
    'unknown_08c': 'zero',
    'unknown_0d8': 'ffffffff000000000000000000000000000000000000000000000000000000003f800000, then 92 zero bytes',
    'unknown_164': '000000062000000200000001000000000000000000000000ffff0000',
    'unknown_18c': 0,
    'unknown_194': 'ffffffff, then 104 zero bytes',
}


def _readable_draw(dr, k, pairs, texture_names):
    tex = []
    for xpr, t in dr.textures:
        e = {'index': t, 'xpr': xpr}
        if texture_names is not None and xpr == 0:
            e['name'] = texture_names[t] if 0 <= t < len(texture_names) else 'tex%d' % t
        tex.append(e)
    sh = dr.get('shader')
    shared = dr.get('shared_shader')
    out = {
        'draw': k,
        'material': None,   # the Blender material, filled in by the import
        'textures': tex,
        'triangles': len(strip_to_triangles(dr.indices)),
        'shader': pairs[sh].ids if sh < len(pairs) else 'pair %d missing' % sh,
        'shader_slot': sh,
        'shared_shader': '%d (%s)' % (shared, layout.SHARED_SHADER_ROLES.get(shared, 'role not known'))
                         if shared else 0,
    }
    for name in ('blend', 'diffuse', 'ambient', 'specular', 'power', 'params', 'flags'):
        out[name] = _plain(dr.get(name))
    odd = {}
    for name, _, _ in layout.MATERIAL_FIELDS:
        if name.startswith('unknown_'):
            v = _plain(dr.get(name))
            if v != USUAL.get(name):
                odd[name] = v
    if odd:
        out['unknown_fields'] = odd
    return out


def to_sidecar(part, name=None, source=None, texture_names=None):
    """PARTFile -> sidecar dict (both halves).

    name: the part's file name without extension ('00002882'); source:
    the part file's bytes (for its hash); texture_names: the texture
    names in the XPR2 the texture indices point into, if known.
    """
    _, pairs, _ = shaders.split_section(part.shaders_raw)
    emb = part.embedded_nodes.nodes if part.embedded_nodes else []
    nodes = []
    for n in part.nodes:
        e = {'name': n.name, 'kind': 'shape' if n.is_shape_node else 'marker',
             'embedded_node': n.get('node_index')}
        if n.is_shape_node:
            e['shape'] = n.get('shape_index')
        if 0 <= n.get('node_index') < len(emb):
            e['position_node_frame'] = [round(float(x), 6) for x in emb[n.get('node_index')].translation]
        nodes.append(e)

    sc = {
        'format': FORMAT,
        'version': VERSION,
        'part': {'name': name,
                 'id': int(name) if name and name.isdigit() else None,
                 'attach': next((n.name for n in part.nodes if n.is_shape_node), None),
                 'sha256': sha256(source) if source is not None else None,
                 'size': len(source) if source is not None else None},
        'shapes': [],
        'markers': [{'name': n['name'], 'position_node_frame': n.get('position_node_frame')}
                    for n in nodes if n['kind'] == 'marker'],
        'nodes': nodes,
        'shader_pairs': [{'slot': k, 'shader': p.ids, 'vs_hash': p.vs_hash, 'ps_hash': p.ps_hash}
                         for k, p in enumerate(pairs)],
        'raw': {
            'header': b64(part.header.raw),
            'nodes': [b64(n.raw) for n in part.nodes],
            'embedded': {'raw': b64(part.embedded_raw), 'tail': b64(part.embedded_tail)},
            'small_sections': [b64(raw) for _, raw in part.small_sections],
            'small_slots': [[list(a[0]), a[1]] if a else None for a in part.small_slots],
            'shaders': b64(part.shaders_raw),
            'shapes': [],
        },
    }
    for i, s in enumerate(part.shapes):
        mb = s.material_block
        count = next((st.count for st in s.streams if st is not None), 0)
        sc['shapes'].append({
            'shape': i,
            'name': s.name,
            'object': None,     # the Blender object, filled in by the import
            'vertices': count,
            'streams': ['slot %d: type 0x%02X, stride %d: %s' % (k, st.type, st.stride,
                                                                  streams_mod.describe(st.type, st.stride))
                        for k, st in enumerate(s.streams) if st is not None],
            'skinned': bool(s.skin_positions.stream or s.skin_normals.stream),
            'draws': [_readable_draw(dr, k, pairs, texture_names) for k, dr in enumerate(mb.draws)],
        })
        sc['raw']['shapes'].append({
            'name': s.name,
            'record': b64(s.raw),
            'streams': [None if st is None else
                        {'type': st.type, 'stride': st.stride, 'count': st.count, 'size': st.size,
                         'sha256': stream_hash(st)} for st in s.streams],
            'material_block': {'head': b64(mb.head), 'tail': b64(mb.tail)},
            'skin': {'raw': b64(s.skin_raw), 'source_offset': s.skin_source_offset,
                     'positions': b64(s.skin_positions.raw), 'normals': b64(s.skin_normals.raw)},
            'draws': [{'record': b64(dr.raw), 'indices_sha256': sha256(dr.indices_raw)} for dr in mb.draws],
            'geometry': {'streams': [None if st is None else b64z(st.raw.tobytes()) for st in s.streams],
                         'indices': [b64z(dr.indices_raw) for dr in mb.draws]},
        })
    return sc


class EditError(Exception):
    pass


EDITABLE = ('blend', 'diffuse', 'ambient', 'specular', 'power', 'params', 'flags',
            'shared_shader', 'textures', 'shader')


def _shared_value(v):
    """'1000 (paint)' or 1000 or 0 -> int."""
    if isinstance(v, str):
        return int(v.split()[0])
    return int(v)


def apply_readable_edits(sc):
    """Apply edits made in the readable half to the raw records.

    For each draw, every field in EDITABLE whose readable value differs
    from the raw record's is written into the record; all other bytes stay
    as they are. Texture indices can change, not how many textures are
    bound (+0x06C and +0x0D8 follow the count; not worked out). The
    shader is given by id ('vs_02 + ps_03') and must be one of the part's
    own pairs (the shader library comes later). Returns the changes as
    text; raises EditError for an edit that can't be applied.
    """
    raw = sc['raw']
    _, pairs, _ = shaders.split_section(unb64(raw['shaders']))
    ids = [p.ids for p in pairs]
    changes = []
    for shape, rshape in zip(sc['shapes'], raw['shapes']):
        for d in shape['draws']:
            k = d['draw']
            rec = Draw(unb64(rshape['draws'][k]['record']))
            was = _readable_draw(rec, k, pairs, None)
            where = '%s draw %d' % (shape['name'], k)
            for name in EDITABLE:
                if name not in d:
                    continue
                new, old = d[name], was[name]
                if name == 'textures':
                    new_idx = [(t['xpr'], t['index']) for t in new]
                    old_idx = [(t['xpr'], t['index']) for t in old]
                    if new_idx == old_idx:
                        continue
                    if len(new_idx) != len(old_idx):
                        raise EditError('%s: the number of textures can\'t change yet' % where)
                    count = rec.get('texture_count')
                    xprs, texs = list(rec.get('texture_xprs')), list(rec.get('texture_indices'))
                    slots = [j for j in range(min(count, 8)) if texs[j] >= 0]
                    for j, (x, t) in zip(slots, new_idx):
                        xprs[j], texs[j] = int(x), int(t)
                    rec.set('texture_xprs', xprs)
                    rec.set('texture_indices', texs)
                elif name == 'shader':
                    if new == old:
                        continue
                    if new not in ids:
                        raise EditError('%s: shader %s isn\'t one of this part\'s (%s)'
                                        % (where, new, ', '.join(ids)))
                    rec.set('shader', ids.index(new))
                elif name == 'shared_shader':
                    if _shared_value(new) == _shared_value(old):
                        continue
                    rec.set('shared_shader', _shared_value(new))
                elif isinstance(old, list):
                    if [round(float(x), 6) for x in new] == old:
                        continue
                    if len(new) != len(old):
                        raise EditError('%s: %s needs %d values' % (where, name, len(old)))
                    rec.set(name, [float(x) for x in new])
                elif isinstance(old, float):
                    if round(float(new), 6) == old:
                        continue
                    rec.set(name, float(new))
                else:
                    if int(new) == old:
                        continue
                    rec.set(name, int(new))
                changes.append('%s: %s %s -> %s' % (where, name, old, new))
            rshape['draws'][k]['record'] = b64(rec.raw)
    return changes


def split(sc):
    """Sidecar -> (readable half, raw half)."""
    readable = {k: v for k, v in sc.items() if k != 'raw'}
    raw = {'format': FORMAT + '-raw', 'version': VERSION, 'part': sc['part']['name'], **sc['raw']}
    return readable, raw


def join(readable, raw):
    """The two halves -> sidecar."""
    if raw.get('format') != FORMAT + '-raw' or raw.get('version') != VERSION:
        raise ValueError('not a version %d raw sidecar' % VERSION)
    if raw.get('part') != readable.get('part', {}).get('name'):
        raise ValueError('raw sidecar of %s, readable of %s' % (raw.get('part'), readable.get('part', {}).get('name')))
    sc = dict(readable)
    sc['raw'] = {k: v for k, v in raw.items() if k not in ('format', 'version', 'part')}
    return sc


def raw_blob(raw):
    """Raw half -> compressed bytes."""
    return zlib.compress(raw_text(raw).encode('utf-8'), 9)


def raw_from_blob(blob):
    return json.loads(zlib.decompress(blob).decode('utf-8'))


def raw_blob_text(raw):
    """Raw half -> one line of base64 (for a Blender text block)."""
    return base64.b64encode(raw_blob(raw)).decode('ascii')


def raw_from_blob_text(text):
    return raw_from_blob(base64.b64decode(text.strip()))


def readable_text(readable):
    """Indented JSON with lists of plain values kept on one line."""
    return _dump(readable, 0)


def _dump(v, depth):
    pad = '  ' * (depth + 1)
    end = '  ' * depth
    if isinstance(v, dict):
        if not v:
            return '{}'
        items = ['%s%s: %s' % (pad, json.dumps(k), _dump(x, depth + 1)) for k, x in v.items()]
        return '{\n' + ',\n'.join(items) + '\n' + end + '}'
    if isinstance(v, list):
        if all(not isinstance(x, (dict, list)) for x in v):
            return json.dumps(v)
        return '[\n' + ',\n'.join(pad + _dump(x, depth + 1) for x in v) + '\n' + end + ']'
    return json.dumps(v)


def raw_text(raw):
    return json.dumps(raw, separators=(',', ':'))


def geometry_of(part):
    """A model's geometry, in the form from_sidecar takes."""
    return [{'streams': list(s.streams), 'indices': [dr.indices_raw for dr in s.draws]}
            for s in part.shapes]


def stored_geometry(sc):
    """The original geometry kept in the sidecar, in geometry_of's form."""
    out = []
    for shape in sc['raw']['shapes']:
        g = shape['geometry']
        streams = []
        for e, raw in zip(shape['streams'], g['streams']):
            if e is None:
                streams.append(None)
                continue
            data = np.frombuffer(unb64z(raw), np.uint8).reshape(e['count'], e['stride']).copy()
            streams.append(Stream(e['type'], e['stride'], data, e['size']))
        out.append({'streams': streams, 'indices': [unb64z(r) for r in g['indices']]})
    return out


def edited_shapes(sc, geometry):
    """Indices of the shapes whose geometry differs from the sidecar's hashes."""
    out = []
    for i, (shape, g) in enumerate(zip(sc['raw']['shapes'], geometry)):
        same = all((e is None and st is None) or
                   (e is not None and st is not None and e['sha256'] == stream_hash(st))
                   for e, st in zip(shape['streams'], g['streams']))
        same = same and len(g['indices']) == len(shape['draws']) and all(
            d['indices_sha256'] == sha256(ib) for d, ib in zip(shape['draws'], g['indices']))
        if not same:
            out.append(i)
    return out


def _skin_array(raw):
    raw = bytes(raw)
    count, stype, stride, size = np.frombuffer(raw[:16], '>u4').tolist()
    st = None
    if count:
        st = Stream(stype, stride, np.frombuffer(raw[16:16 + count * stride], np.uint8)
                    .reshape(count, stride).copy(), size)
    return SkinArray(raw[:16], st)


def from_sidecar(sc, geometry=None):
    """Sidecar dict + geometry -> PARTFile, ready for write_PART.

    Only the raw half is used. geometry None: the original geometry
    stored in the sidecar.
    """
    if sc.get('format') != FORMAT:
        raise ValueError('not an SBX part sidecar')
    if sc.get('version') != VERSION:
        raise ValueError('sidecar version %s, expected %d (import the part again)' % (sc.get('version'), VERSION))
    if geometry is None:
        geometry = stored_geometry(sc)
    raw = sc['raw']
    if len(geometry) != len(raw['shapes']):
        raise ValueError('geometry for %d shapes, the sidecar has %d' % (len(geometry), len(raw['shapes'])))
    part = PARTFile()
    part.header = PARTHeader(unb64(raw['header']))
    part.nodes = [PartNode(unb64(n)) for n in raw['nodes']]
    part.embedded_raw = unb64(raw['embedded']['raw'])
    part.embedded_tail = unb64(raw['embedded']['tail'])
    if part.embedded_raw and node.is_NODE(part.embedded_raw):
        part.embedded_nodes = node.read_NODE(part.embedded_raw)
    part.small_sections = [(None, unb64(r)) for r in raw['small_sections']]
    part.small_slots = [(tuple(a[0]), a[1]) if a else None for a in raw['small_slots']]
    part.shaders_raw = unb64(raw['shaders'])
    for shape, g in zip(raw['shapes'], geometry):
        s = Shape(unb64(shape['record']))
        s.streams = list(g['streams'])
        draws = shape['draws']
        if len(g['indices']) != len(draws):
            raise ValueError('%s: %d index buffers for %d draws' % (shape['name'], len(g['indices']), len(draws)))
        mb = shape['material_block']
        s.material_block = MaterialBlock(unb64(mb['head']),
                                         [Draw(unb64(d['record']), ib) for d, ib in zip(draws, g['indices'])],
                                         unb64(mb['tail']))
        sk = shape['skin']
        s.skin_raw = unb64(sk['raw'])
        s.skin_source_offset = sk['source_offset']
        s.skin_positions = _skin_array(unb64(sk['positions']))
        s.skin_normals = _skin_array(unb64(sk['normals']))
        part.shapes.append(s)
    return part


def write_sidecar(path, sc):
    """Both halves next to each other: PATH (readable, '<part>.sbx.json')
    and the raw half compressed in '<part>.sbx.raw'."""
    readable, raw = split(sc)
    with open(path, 'w', encoding='utf-8', newline='\n') as f:
        f.write(readable_text(readable))
    with open(raw_path(path), 'wb') as f:
        f.write(raw_blob(raw))


def read_sidecar(path):
    with open(path, encoding='utf-8') as f:
        readable = json.load(f)
    with open(raw_path(path), 'rb') as f:
        raw = raw_from_blob(f.read())
    return join(readable, raw)


def raw_path(path):
    """'00002882.sbx.json' -> '00002882.sbx.raw'."""
    return path[:-5] + '.raw' if path.endswith('.json') else path + '.raw'
