"""NODE files (00 02 00 00): a model's nodes (parts or bones).

Kept as raw bytes so a file writes back unchanged; nodes can be moved
(set_node_translation) and added (insert_node). Format:
docs/game_data.md, "Node files". Big-endian.

Layout (all 399 node files checked, the cars' and the ones embedded in
parts): a 0x40-byte header, the node records (160 bytes each), then per
node the translations, rotations and scales (16 bytes each), the world
matrices, a second matrix and the inverse world matrices (64 bytes
each), then a 16-byte tail. The header's +0x10 to +0x30 are those
sections' offsets and the file's end; +0x34 holds 12 bytes not worked
out, carried as they are.
"""

import struct

import numpy as np

from .binary import cstr, s32, u32

NODE_MAGIC = b'\x00\x02\x00\x00'
NODE_SIZE = 160
NODE_END_FIELD = 0x30   # header word: end of the file (its size)


class Node:
    """One node of a NODE file.

    translation, rotation (quaternion x, y, z, w) and scale are local to
    the parent. world is the bind pose (row vectors: p_world = p @ world),
    inverse_bind its inverse, both as stored in the file.
    """

    def __init__(self, name, parent, depth):
        self.name = name
        self.parent = parent
        self.depth = depth
        self.translation = np.zeros(3)
        self.rotation = np.array([0.0, 0.0, 0.0, 1.0])
        self.scale = np.ones(3)
        self.world = np.eye(4)
        self.inverse_bind = np.eye(4)

    def __repr__(self):
        return 'Node(%r, parent=%d, t=%s)' % (self.name, self.parent,
                                             np.round(self.translation, 3))


class NODEFile:
    """A NODE file: its nodes, and its raw bytes for writing back."""

    def __init__(self, raw, nodes):
        self.raw = bytes(raw)
        self.nodes = nodes


def is_NODE(d, base=0):
    return d[base:base + 4] == NODE_MAGIC


def NODE_size(d, base=0):
    """Size of the NODE file at base, from its header."""
    return u32(d, base + NODE_END_FIELD)


def read_NODE_nodes(d, base=0):
    """NODE file at offset base -> list of Node."""
    if not is_NODE(d, base):
        raise ValueError('not a node file (magic %s)' % d[base:base + 4].hex())
    count = u32(d, base + 4)
    nodes_off, t_off, r_off, s_off, world_off, _, inv_off = (
        base + u32(d, base + 0x10 + 4 * k) for k in range(7))

    def mats(off):
        return np.frombuffer(d, '>f4', count * 16, off).reshape(count, 4, 4).astype(np.float64)

    world = mats(world_off)
    inverse = mats(inv_off)
    out = []
    for i in range(count):
        o = nodes_off + i * NODE_SIZE
        n = Node(cstr(d[o:o + 0x5C]), s32(d, o + 0x80), s32(d, o + 0x98))
        n.translation = np.array(struct.unpack_from('>3f', d, t_off + 16 * i))
        n.rotation = np.array(struct.unpack_from('>4f', d, r_off + 16 * i))
        n.scale = np.array(struct.unpack_from('>3f', d, s_off + 16 * i))
        n.world = world[i]
        n.inverse_bind = inverse[i]
        out.append(n)
    return out


def read_NODE(d, base=0):
    """NODE file at offset base -> NODEFile."""
    nodes = read_NODE_nodes(d, base)
    return NODEFile(d[base:base + NODE_size(d, base)], nodes)


def write_NODE(node_file):
    """NODEFile -> bytes. Nodes aren't editable yet, so this is the raw file."""
    return node_file.raw


def set_node_translation(raw, index, xyz):
    """A NODE file's bytes with node index moved to xyz (node frame).

    For a top-level node (every marker in the game's parts): its
    translation, the translation row of its world matrix, and its
    inverse world matrix (recomputed) change; its rotation stays."""
    d = bytearray(raw)
    count = u32(d, 4)
    if not 0 <= index < count:
        raise ValueError('node %d of %d' % (index, count))
    t_off, world_off, inv_off = u32(d, 0x14), u32(d, 0x20), u32(d, 0x28)
    o = node_offset = u32(d, 0x10) + index * NODE_SIZE
    if s32(d, node_offset + 0x80) != -1:
        raise ValueError('node %d isn\'t top-level' % index)
    struct.pack_into('>3f', d, t_off + 16 * index, *map(float, xyz))
    world = np.frombuffer(bytes(d), '>f4', 16, world_off + 64 * index).reshape(4, 4).astype(np.float64)
    world[3, :3] = xyz
    struct.pack_into('>3f', d, world_off + 64 * index + 48, *map(float, xyz))
    inverse = np.linalg.inv(world.astype(np.float32).astype(np.float64))
    struct.pack_into('>16f', d, inv_off + 64 * index, *map(float, inverse.ravel()))
    del o
    return bytes(d)


def node_by_name(nodes, name):
    return next((i for i, n in enumerate(nodes) if n.name == name), None)


# Per-node sections after the records, in file order: (header word, bytes per node).
NODE_ARRAYS = ((0x14, 16), (0x18, 16), (0x1C, 16), (0x20, 64), (0x24, 64), (0x28, 64))
NODE_HEADER_SIZE = 0x40
NODE_TAIL_SIZE = 16
NAME_SIZE = 0x5C


def split_NODE(raw):
    """NODE bytes -> (header, [records], [[entries] per array], tail).

    Raises ValueError when the file doesn't follow the layout above."""
    d = bytes(raw)
    count = u32(d, 4)
    o = NODE_HEADER_SIZE
    if u32(d, 0x10) != o:
        raise ValueError('node records at 0x%X, expected 0x40' % u32(d, 0x10))
    records = [d[o + NODE_SIZE * i:o + NODE_SIZE * (i + 1)] for i in range(count)]
    o += NODE_SIZE * count
    arrays = []
    for word, size in NODE_ARRAYS:
        if u32(d, word) != o:
            raise ValueError('section at header +0x%X isn\'t where the layout puts it' % word)
        arrays.append([d[o + size * i:o + size * (i + 1)] for i in range(count)])
        o += size * count
    if u32(d, 0x2C) != o or u32(d, NODE_END_FIELD) != o + NODE_TAIL_SIZE or len(d) < o + NODE_TAIL_SIZE:
        raise ValueError('tail or end of the node file isn\'t where the layout puts it')
    # The tail, and whatever follows the stated end (16 zero bytes in the
    # cars' node files).
    return d[:NODE_HEADER_SIZE], records, arrays, d[o:]


def join_NODE(header, records, arrays, tail):
    """The pieces of split_NODE -> NODE bytes, offsets and counts recomputed.

    join_NODE(*split_NODE(raw)) == raw for every node file checked."""
    count = len(records)
    h = bytearray(header)
    struct.pack_into('>I', h, 4, count)
    if u32(header, 0x0C) == u32(header, 4):   # +0x0C is the count in every car and part node file
        struct.pack_into('>I', h, 0x0C, count)
    body = bytearray(b''.join(records))
    o = NODE_HEADER_SIZE + len(body)
    struct.pack_into('>I', h, 0x10, NODE_HEADER_SIZE)
    for (word, size), entries in zip(NODE_ARRAYS, arrays):
        if len(entries) != count:
            raise ValueError('%d entries at header +0x%X for %d nodes' % (len(entries), word, count))
        struct.pack_into('>I', h, word, o)
        body += b''.join(entries)
        o += size * count
    struct.pack_into('>I', h, 0x2C, o)
    struct.pack_into('>I', h, NODE_END_FIELD, o + NODE_TAIL_SIZE)
    return bytes(h) + bytes(body) + bytes(tail)


def insert_node(raw, at, copy_of, name=None):
    """A NODE file's bytes with a copy of node copy_of inserted at index at.

    The copy keeps every byte of the original node (record and arrays)
    but its name, when one is given. Only for files whose nodes are all
    top-level (every node file embedded in a part): a parent index would
    need renumbering, so a file with parents is refused."""
    header, records, arrays, tail = split_NODE(raw)
    if not 0 <= copy_of < len(records) or not 0 <= at <= len(records):
        raise ValueError('node %d copied to %d, of %d' % (copy_of, at, len(records)))
    if u32(header, 8) != 0 or u32(header, 0x0C) != len(records):
        raise ValueError('node file header not worked out for adding nodes')
    if any(s32(r, 0x80) != -1 for r in records):
        raise ValueError('only node files with top-level nodes can take new nodes')
    rec = bytearray(records[copy_of])
    if name is not None:
        b = name.encode('ascii')
        if len(b) >= 32:
            raise ValueError('node name %r is longer than 31 characters' % name)
        rec[0:NAME_SIZE] = b + b'\0' * (NAME_SIZE - len(b))
    records.insert(at, bytes(rec))
    for entries in arrays:
        entries.insert(at, entries[copy_of])
    return join_NODE(header, records, arrays, tail)


def set_node_rotation_identity(raw, index):
    """A NODE file's bytes with node index's rotation reset (quaternion 0, 0,
    0, 1; world matrix rotation identity, inverse recomputed). Top-level
    nodes only, as set_node_translation."""
    d = bytearray(raw)
    if s32(d, u32(d, 0x10) + index * NODE_SIZE + 0x80) != -1:
        raise ValueError('node %d isn\'t top-level' % index)
    r_off, world_off, inv_off = u32(d, 0x18), u32(d, 0x20), u32(d, 0x28)
    struct.pack_into('>4f', d, r_off + 16 * index, 0.0, 0.0, 0.0, 1.0)
    world = np.frombuffer(bytes(d), '>f4', 16, world_off + 64 * index).reshape(4, 4).astype(np.float64)
    m = np.eye(4)
    m[3, :3] = world[3, :3]
    struct.pack_into('>16f', d, world_off + 64 * index, *map(float, m.ravel()))
    inverse = np.linalg.inv(m.astype(np.float32).astype(np.float64))
    struct.pack_into('>16f', d, inv_off + 64 * index, *map(float, inverse.ravel()))
    return bytes(d)
