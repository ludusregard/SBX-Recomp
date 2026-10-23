#!/usr/bin/env python3
"""Applies the cut cars' wheel baselines (docs/cut_car_wheels.md).

    python cut_car_wheels.py BUILD_OUT OUT [--specs 00000072.bin]

Writes to OUT, named for the patch loader (assets/PATCH_DATA/BUILD/TXR/BUILD/):
- 00000072.bin: the car specs with the L86, T86 and S30's new tire sizes
  (all 21 front and 21 rear tires; grip scaled so each tire keeps its grade).
  From BUILD_OUT's 00000072.bin, or --specs to start from another one.
- The set-a and set-b node files of the ECLI, L86, S30 and T86 with their
  tire nodes moved.

The physics track (block 1 +540, +544) isn't changed. Formats:
docs/game_data.md ("72: car specs", "Tires", "Node files").
"""

import argparse
import math
import os
import struct
import sys

import numpy as np

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), 'blender_plugin'))

from sbx_formats import node_by_name, read_NODE_nodes  # noqa: E402
from sbx_formats.binary import s32, u32  # noqa: E402

SPEC_SIZE = 8432
TIRES = 4244   # 42 tires of 92 bytes: 21 front, then 21 rear.
TIRE_SIZE = 92

# New tire sizes: car id -> (width mm, aspect %, rim in, rim width in), front and rear.
TIRE_BASELINES = {
    18: (185, 60, 14, 5.5),  # L86
    20: (185, 60, 14, 5.5),  # T86
    9: (195, 70, 14, 6.0),   # S30
}

# New tire node positions (node frame, decimeters), left side; the right
# side is the same with x negated. Car model block -> (front, rear).
BLOCK_STRIDE_A, BLOCK_STRIDE_B = 226, 197
NODE_BASELINES = {
    19: ((7.85, 1.834, -12.875), (7.85, 2.084, 12.875)),  # ECLI
    20: ((6.825, 1.574, -12.0), (6.975, 1.574, 12.0)),    # L86
    21: ((7.18, 1.668, -11.75), (7.08, 1.668, 11.75)),    # S30
    22: ((6.825, 1.574, -12.0), (6.975, 1.574, 12.0)),    # T86
}


def stock_grip(radius, width):
    """The stock tire's grip on road cars (docs/game_data.md, "Tires")."""
    return 0.525 * (1 + 3.67 * math.sqrt(radius * width))


def patch_specs(specs):
    d = bytearray(specs)
    for car, (width_mm, aspect, rim, rim_width) in TIRE_BASELINES.items():
        name = d[car * SPEC_SIZE:car * SPEC_SIZE + 8].decode('ascii').strip()
        width = width_mm / 1000
        radius = rim * 0.0254 / 2 + width * aspect / 100
        for side in (0, 1):
            first = car * SPEC_SIZE + TIRES + side * 21 * TIRE_SIZE
            old_radius, old_width = struct.unpack_from('<2f', d, first + 44)
            factor = stock_grip(radius, width) / stock_grip(old_radius, old_width)
            for i in range(21):
                o = first + i * TIRE_SIZE
                struct.pack_into('<2f', d, o + 44, radius, width)
                struct.pack_into('<3f', d, o + 80, rim, aspect / 100, rim_width)
                for g in (64, 68):
                    grip, = struct.unpack_from('<f', d, o + g)
                    struct.pack_into('<f', d, o + g, grip * factor)
            print('%s %s: %d/%dR%d %.1fJ, radius %.4f, grip x %.4f'
                  % (name, 'front' if side == 0 else 'rear', width_mm, aspect, rim, rim_width,
                     radius, factor))
    return bytes(d)


def move_node(raw, index, xyz):
    """A NODE file's bytes with node index moved to xyz. Its parents must all be
    at the origin with no rotation (true of tire_pos and the root in every car),
    so its world matrix is its own translation."""
    d = bytearray(raw)
    count = u32(d, 4)
    nodes_off, t_off, world_off, inv_off = u32(d, 0x10), u32(d, 0x14), u32(d, 0x20), u32(d, 0x28)
    if not 0 <= index < count:
        raise ValueError('node %d of %d' % (index, count))
    parent = s32(d, nodes_off + index * 160 + 0x80)
    while parent >= 0:
        world = np.frombuffer(bytes(d), '>f4', 16, world_off + 64 * parent).reshape(4, 4)
        if not np.allclose(world, np.eye(4), atol=1e-6):
            raise ValueError('node %d: parent %d isn\'t at the origin' % (index, parent))
        parent = s32(d, nodes_off + parent * 160 + 0x80)
    struct.pack_into('>3f', d, t_off + 16 * index, *xyz)
    world = np.frombuffer(bytes(d), '>f4', 16, world_off + 64 * index).reshape(4, 4).astype(np.float64)
    world[3, :3] = xyz
    struct.pack_into('>3f', d, world_off + 64 * index + 48, *xyz)
    inverse = np.linalg.inv(world.astype(np.float32).astype(np.float64))
    struct.pack_into('>16f', d, inv_off + 64 * index, *inverse.ravel())
    return bytes(d)


def patch_nodes(raw, front, rear):
    nodes = read_NODE_nodes(raw)
    for name, (x, y, z) in (('tire_fl', front), ('tire_fr', front),
                            ('tire_rl', rear), ('tire_rr', rear)):
        index = node_by_name(nodes, name)
        if index is None:
            raise ValueError('no %s node' % name)
        xyz = (x if name.endswith('l') else -x, y, z)
        raw = move_node(raw, index, xyz)
    return raw


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('build_out', help='the dumped BUILD archive (GUT Archive Tools)')
    ap.add_argument('out', help='output folder')
    ap.add_argument('--specs', help='car specs to start from (default: BUILD_OUT/00000072.bin)')
    args = ap.parse_args()
    os.makedirs(args.out, exist_ok=True)

    specs_path = args.specs or os.path.join(args.build_out, '00000072.bin')
    with open(specs_path, 'rb') as f:
        specs = f.read()
    with open(os.path.join(args.out, '00000072.bin'), 'wb') as f:
        f.write(patch_specs(specs))

    for block, (front, rear) in NODE_BASELINES.items():
        for node_id in (892 + BLOCK_STRIDE_A * block, 11062 + BLOCK_STRIDE_B * block):
            name = '%08d.node' % node_id
            with open(os.path.join(args.build_out, name), 'rb') as f:
                raw = f.read()
            root = read_NODE_nodes(raw)[0].name
            with open(os.path.join(args.out, name), 'wb') as f:
                f.write(patch_nodes(raw, front, rear))
            print('%s (%s): tire nodes front %s, rear %s' % (name, root, front, rear))


if __name__ == '__main__':
    main()
