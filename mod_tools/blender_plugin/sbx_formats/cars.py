"""Car blocks: the part slots, the node frame and the textures of a car.

docs/game_data.md, "Car models", "Part slots", "Car space and the sets
compared".
"""

import os
import re

import numpy as np

# Node frame (decimeters, x left, y up, z rear) to car space (meters,
# x forward, y up, z left), for row vectors: car = node @ NODE_TO_CAR.
NODE_TO_CAR = np.array([[0.0, 0.0, 0.1],
                        [0.0, 0.1, 0.0],
                        [-0.1, 0.0, 0.0]])

# Car blocks: slot groups as offsets from the node file's id, per set
# (docs/game_data.md, "Part slots"). Groups marked optional are left out
# of a stock car.
CAR_SLOT_GROUPS = {
    'a': [('fb', 1, 13), ('cool', 14, 104, 'optional'), ('rb', 105, 117),
          ('ss', 118, 130), ('rs', 131, 143), ('bn', 144, 151),
          ('door_L', 152, 154), ('door_R', 155, 157), ('mirror_L', 158, 169),
          ('mirror_R', 170, 181), ('hl', 182, 185), ('tl', 186, 189),
          ('cabin', 190, 190), ('fender', 191, 196), ('engine', 197, 211),
          ('muffler', 212, 216), ('meters', 217, 217, 'optional'),
          ('roll', 218, 218, 'optional'), ('interior', 219, 219),
          ('gauges', 220, 220), ('seats', 221, 221), ('steering', 222, 222),
          ('neon', 223, 223, 'optional')],
    'b': [('fb', 1, 13), ('cool', 14, 104, 'optional'), ('rb', 105, 117),
          ('ss', 118, 130), ('rs', 131, 143), ('bn', 144, 151),
          ('door', 152, 154), ('mirror', 155, 166), ('hl', 167, 170),
          ('tl', 171, 174), ('cabin', 175, 175), ('fender', 176, 181),
          ('muffler', 182, 186), ('shadow', 187, 187, 'optional'),
          ('meters', 188, 188, 'optional'), ('roll', 189, 189, 'optional'),
          ('interior', 190, 190), ('gauges', 191, 191), ('seats', 192, 192),
          ('steering', 193, 193), ('neon', 194, 194, 'optional')],
    'c': [('bn', 1, 13), ('door', 14, 16), ('mirror', 17, 28), ('hl', 29, 32),
          ('cabin', 33, 33), ('fender', 34, 46),
          ('meters', 47, 47, 'optional'), ('roll', 48, 48, 'optional'),
          ('interior', 49, 49), ('gauges', 50, 50), ('seats', 51, 51),
          ('steering', 52, 52)],
}

# Shapes that are alternatives of the first one in the same part (steering
# wheels, gauge faces): a stock car keeps only the 00 version.
STOCK_SKIP_SHAPE = re.compile(r'^(steer|speed|tacho)0[1-9]')

TIRE_NODES = ('tire_fl', 'tire_fr', 'tire_rl', 'tire_rr')



# Sets: (first body XPR2 id, stride, number of cars). A block is: body
# XPR2, interior textures, node file, then the part slots.
CAR_SETS = {'a': (890, 226, 45), 'b': (11060, 197, 45), 'c': (19925, 55, 45)}


# The 45 car blocks, in block order (the same in sets a, b and c):
# (code, name). Codes are the car spec ids (BUILD 72; docs/game_data.md,
# "Car ids"), confirmed by each node file's root node, named
# <code>_<set>_tr (rx7_a_tr ...). The names are the cars the codes stand
# for; the rivals' are their in-game names.
CAR_BLOCKS = [
    ('RX7', 'Mazda RX-7 (FD)'), ('RX8', 'Mazda RX-8'),
    ('EVO9', 'Mitsubishi Lancer Evolution IX'), ('CIMA', 'Nissan Cima'),
    ('N180', 'Nissan 180SX'), ('SIL15', 'Nissan Silvia (S15)'),
    ('SKY32', 'Nissan Skyline (R32)'), ('SKY34', 'Nissan Skyline (R34)'),
    ('SKY35', 'Nissan Skyline (V35)'), ('Z32', 'Nissan Fairlady Z (Z32)'),
    ('Z33', 'Nissan Fairlady Z (Z33)'), ('IMP_GC8', 'Subaru Impreza (GC8)'),
    ('IMP_GDB', 'Subaru Impreza (GDB)'), ('ARISTO', 'Toyota Aristo'),
    ('CEL', 'Toyota Celsior'), ('CROWN', 'Toyota Crown'),
    ('SUPRA70', 'Toyota Supra (A70)'), ('SUPRA80', 'Toyota Supra (A80)'),
    ('CON_X', 'Skull Bullet (the player\'s)'), ('ECLI', 'Mitsubishi Eclipse'),
    ('L86', 'Toyota Corolla Levin (AE86)'), ('S30', 'Nissan Fairlady Z (S30)'),
    ('T86', 'Toyota Sprinter Trueno (AE86)'), ('BOSS00', 'Devil Z (rival)'),
    ('BOSS01', 'Snake Eyes (rival)'), ('BOSS02', 'Emperor (rival)'),
    ('BOSS03', 'Crimson Devil (rival)'), ('BOSS04', 'White Charisma (rival)'),
    ('BOSS05', 'Melancholy Angel (rival)'), ('BOSS06', 'The Rook (rival)'),
    ('BOSS07', 'The Bishop (rival)'), ('BOSS08', 'The Knight (rival)'),
    ('BOSS09', 'Blood Hound (rival)'), ('BOSS10', 'Midnight Cinderella (rival)'),
    ('BOSS11', 'Silvery Nobleman (rival)'), ('BOSS20', 'Skull Bullet (rival)'),
    ('WON01', 'Death Driver (rival)'), ('WON02', 'Matterhorn (rival)'),
    ('WON03', 'Rolling Master (rival)'), ('WON04', 'Gold License (rival)'),
    ('WON06', 'Stalking Hammer (rival)'), ('LEAD01', 'Rolling Guy No. 1 (rival)'),
    ('LEAD02', 'Rolling Gal No. 1 (rival)'), ('LEAD04', 'White Fairy (rival)'),
    ('LEAD06', 'Naniwa\'s Warship (rival)'),
]

# Wheel models, in id order: wheel i is the rim (and wheel face) at
# 587 + 3i, its brake and hub at the next id, its textures (XPR2) at the
# id before. Named by the parts' first node. The first 20 are the stock
# wheels of car blocks 0-19, then 16 traffic wheels, then the 64
# aftermarket wheels, whose ids are table 73's (BUILD 73).
WHEELS = ('RX7 RX8 EVO9 CIMA N180 SIL15 SKY32 SKY34 SKY35 Z32 Z33 IMP_GC8 IMP_GDB ARISTO CEL CROWN '
          'SUPRA70 SUPRA80 CON_X ECLI LORRY TRACK01 TRACK02 BUS AD CEFIRO MARCH QUON FORESTER LEGACY '
          'ESTIMA MK100 MK110 TOWN TRAILER VITZ AD00 AD01 AD02 AD03 AD04 AD05 AD06 AD07 AD08 EN00 EN01 '
          'EN02 EN03 EN04 EN05 TA00 TA01 TA02 TA03 TA04 TA05 TA06 TA07 TA08 TA09 VR00 VR01 VR02 VR03 '
          'VR04 VR05 VR06 VR07 GL00 GL01 GL02 GL03 GL04 GL05 VE00 VE01 TE00 GA00 GA01 TDR00 TDR01 '
          'GG00 GG01 GG02 AX00 ZIG00 ZIG01 ZIG02 ZIG03 ZIG04 JP00 WAL00 TOM00 TOM01 TOM02 TOM03 '
          'GRP00 GRP01 GRP02').split()
FIRST_RIM = 587

# The two tire models (the same size, 0.61 m across, 0.255 m wide), with
# their morph targets.
TIRES = {581: 'tire_00', 583: 'tire_01'}


def wheel_parts(name):
    """A wheel's name -> (rim id, brake id), or None."""
    name = name.upper()
    if name not in WHEELS:
        return None
    rim = FIRST_RIM + 3 * WHEELS.index(name)
    return rim, rim + 1


def car_name(node_id):
    """A car block's node file id -> (code, name, set) or None."""
    block = car_block_of(node_id)
    if block is None or block[1] != node_id:
        return None
    first, stride, _ = CAR_SETS[block[0]]
    k = (node_id - 2 - first) // stride
    code, name = CAR_BLOCKS[k]
    return code, name, block[0]


def car_block_of(file_id):
    """Any id inside a car block -> (set, node file id), or None."""
    for name, (first, stride, count) in CAR_SETS.items():
        k, off = divmod(file_id - first, stride)
        if 0 <= k < count:
            return name, first + stride * k + 2
    return None


def car_part_ids(build_out, node_id, car_set, stock=False):
    """Part files of a car block -> [(id, path, slot group name)].

    stock keeps only the first filled slot of each group and skips the
    optional groups (cooling ducts, meters, roll cage, neon, shadow).
    """
    files = []
    for group in CAR_SLOT_GROUPS[car_set]:
        name, first, last = group[:3]
        if stock and len(group) > 3:
            continue
        for i in range(node_id + first, node_id + last + 1):
            p = os.path.join(build_out, '%08d.part' % i)
            if os.path.exists(p) and os.path.getsize(p) > 0:
                files.append((i, p, name))
                if stock:
                    break
    return files


def body_xpr_path(build_out, node_id):
    """A car block's body XPR2 (node id - 2): every part's textures."""
    return os.path.join(build_out, '%08d.xpr' % (node_id - 2))


def wheel_xpr_path(build_out, part_id):
    """A wheel part's XPR2: the one or two ids before it (586 for 587, 588)."""
    for k in (1, 2):
        p = os.path.join(build_out, '%08d.xpr' % (part_id - k))
        if os.path.exists(p):
            return p
    return None
