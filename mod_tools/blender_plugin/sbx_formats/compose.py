"""Adding to a PART: new shapes, new markers, new shader pairs.

    add_shape(part, shape)          a new shape under the part's attach node
    add_marker(part, name, xyz)     a new marker (node frame)
    add_shader_pair(part, pair)     a pair appended to the shader section;
                                    returns its slot (material record +0x068)

The rules every game part follows (check_rules, all 395 parts checked;
docs/game_data.md, "Part files"):
- shapes are sorted by name (byte order of the 32-byte names);
- the part's node list is sorted by name; each shape has its own node
  (kind 1), named after what the part attaches to (fb, ss, door), and
  each marker its node (kind 0);
- the embedded node file has one node per entry of the node list, in the
  same order and with the same names, all top-level;
- a node's node_index (+0x24) is the first embedded node with its name:
  all of a part's shape nodes point at the first one, a marker at its
  own;
- the shader section's +0x04 is its size less 0x20 x (pairs + 1).

New pieces copy every byte they can from a piece of the same kind (a
shape node copies the part's first shape node, a marker node the first
marker, or the shape node with kind 0), so the fields not worked out
hold values the game already reads.
"""

import struct

from . import shaders
from .node import insert_node, read_NODE, set_node_rotation_identity, set_node_translation
from .part_model import PartNode

MAX_NAME = 31   # 32-byte names, zero-terminated


class ComposeError(Exception):
    pass


def _name_bytes(name):
    try:
        b = name.encode('ascii')
    except UnicodeEncodeError:
        raise ComposeError('%r: names must be plain ASCII' % name)
    if not b or len(b) > MAX_NAME:
        raise ComposeError('%r: names are 1 to %d characters' % (name, MAX_NAME))
    if b'\0' in b:
        raise ComposeError('%r: no zero bytes in names' % name)
    return b + b'\0' * (32 - len(b))


def _key(raw_name):
    """Sort key of a 32-byte name: its bytes up to the first zero."""
    return bytes(raw_name).split(b'\0')[0]


def attach_name(part):
    """The name of the part's shape nodes (what it attaches to), or None."""
    return next((n.name for n in part.nodes if n.is_shape_node), None)


def check_rules(part):
    """The rules above -> list of what doesn't hold (empty: all do)."""
    out = []
    names = [_key(s.get('name')) for s in part.shapes]
    if names != sorted(names):
        out.append('shapes not sorted by name')
    if len(set(names)) != len(names):
        out.append('shape names repeat')
    nodes = [_key(n.get('name')) for n in part.nodes]
    if nodes != sorted(nodes):
        out.append('nodes not sorted by name')
    shape_nodes = [n for n in part.nodes if n.is_shape_node]
    if sorted(n.get('shape_index') for n in shape_nodes) != list(range(len(part.shapes))):
        out.append('not one shape node per shape')
    if len({n.name for n in shape_nodes}) > 1:
        out.append('shape nodes with different names')
    for i, n in enumerate(part.nodes):
        if n.get('node_index') != nodes.index(nodes[i]):
            out.append('node %d (%s): node_index %d, first node with its name is %d'
                       % (i, n.name, n.get('node_index'), nodes.index(nodes[i])))
        if not n.is_shape_node and n.get('shape_index') != -1:
            out.append('marker %s has a shape index' % n.name)
    if part.embedded_nodes is not None:
        emb = part.embedded_nodes.nodes
        if [e.name for e in emb] != [n.name for n in part.nodes]:
            out.append('embedded node names differ from the node list')
        if any(e.parent != -1 for e in emb):
            out.append('embedded nodes with parents')
    raw = part.shaders_raw
    if len(raw) >= 8:
        count, size = struct.unpack_from('>2I', raw, 0)
        if size != len(raw) - 0x20 * (count + 1):
            out.append('shader section size word is %d, expected %d' % (size, len(raw) - 0x20 * (count + 1)))
    return out


def _renumber_node_indices(part):
    keys = [_key(n.get('name')) for n in part.nodes]
    for i, n in enumerate(part.nodes):
        n.set('node_index', keys.index(keys[i]))


def _insert_node(part, node, embedded_copy_of):
    """Insert a PartNode at its sorted place (after nodes with the same name)
    and a copy of embedded node embedded_copy_of at the same place.
    Returns the new node's index."""
    if part.embedded_nodes is None:
        raise ComposeError('the part has no embedded node file; nodes can\'t be added')
    key = _key(node.get('name'))
    at = len(part.nodes)
    for i, n in enumerate(part.nodes):
        if _key(n.get('name')) > key:
            at = i
            break
    part.nodes.insert(at, node)
    try:
        part.embedded_raw = insert_node(part.embedded_raw, at, embedded_copy_of, node.name)
    except ValueError as e:
        part.nodes.pop(at)
        raise ComposeError('embedded node file: %s' % e)
    part.embedded_nodes = read_NODE(part.embedded_raw)
    _renumber_node_indices(part)
    return at


def add_shape(part, shape):
    """Insert a new shape (part_model.Shape, complete: streams, draws, skin)
    at its sorted place, with its shape node and embedded node. Returns
    the new shape's index. The shape's name must be new to the part."""
    raw_name = shape.get('name')
    key = _key(raw_name)
    if not key:
        raise ComposeError('the shape has no name')
    if any(_key(s.get('name')) == key for s in part.shapes):
        raise ComposeError('%s: the part already has a shape with that name' % shape.name)
    template = next((n for n in part.nodes if n.is_shape_node), None)
    if template is None:
        raise ComposeError('the part has no shapes, so what new ones attach to isn\'t known')
    k = len(part.shapes)
    for i, s in enumerate(part.shapes):
        if _key(s.get('name')) > key:
            k = i
            break
    part.shapes.insert(k, shape)
    for n in part.nodes:
        if n.is_shape_node and n.get('shape_index') >= k:
            n.set('shape_index', n.get('shape_index') + 1)
    part.small_slots = [_shift_anchor(a, k) for a in part.small_slots]
    node = PartNode(template.raw)
    node.set('shape_index', k)
    first = next(i for i, n in enumerate(part.nodes) if _key(n.get('name')) == _key(template.get('name')))
    _insert_node(part, node, first)
    return k


def _shift_anchor(a, k):
    """An anchor into a stream or index buffer of a shape at or after k moves up one."""
    if a is None:
        return a
    key, dist = a
    if key[0] in ('stream', 'indices') and key[1] >= k:
        return ((key[0], key[1] + 1) + tuple(key[2:]), dist)
    return a


def add_marker(part, name, position):
    """Insert a new marker node at its sorted place, at position (node
    frame: decimeters, x to the car's left, z to the rear), rotation
    identity. Returns its node index."""
    raw_name = _name_bytes(name)
    if any(_key(n.get('name')) == _key(raw_name) for n in part.nodes):
        raise ComposeError('%s: the part already has a node with that name' % name)
    template = next((n for n in part.nodes if not n.is_shape_node), None) or \
        next((n for n in part.nodes if n.is_shape_node), None)
    if template is None:
        raise ComposeError('the part has no nodes to copy a marker from')
    copy_of = part.nodes.index(template)
    node = PartNode(template.raw)
    node.set('name', raw_name)
    node.set('kind', 0)
    node.set('shape_index', -1)
    at = _insert_node(part, node, copy_of)
    part.embedded_raw = set_node_rotation_identity(part.embedded_raw, at)
    part.embedded_raw = set_node_translation(part.embedded_raw, at, position)
    part.embedded_nodes = read_NODE(part.embedded_raw)
    return at


def set_shape_name(shape, name):
    shape.set('name', _name_bytes(name))


def add_shader_pair(part, pair):
    """Append a shaders.ShaderPair to the part's shader section, unless the
    part has the same pair (by the shaders' hashes) already. Returns its
    slot, the value for material record +0x068."""
    header, pairs, rest = shaders.split_section(part.shaders_raw)
    for k, p in enumerate(pairs):
        if p.vs_hash == pair.vs_hash and p.ps_hash == pair.ps_hash:
            return k
    if len(header) < shaders.SECTION_HEADER_SIZE:
        raise ComposeError('the part has no shader section')
    pairs.append(pair)
    raw = bytearray(shaders.join_section(header, pairs, rest))
    struct.pack_into('>2I', raw, 0, len(pairs), len(raw) - 0x20 * (len(pairs) + 1))
    part.shaders_raw = bytes(raw)
    return len(pairs) - 1
