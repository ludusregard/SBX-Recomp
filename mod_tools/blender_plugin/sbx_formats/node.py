"""NODE files (00 02 00 00): a model's nodes (parts or bones).

Read only for now; the file is also kept as raw bytes so it writes back
unchanged. Format: docs/game_data.md, "Node files". Big-endian.
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
