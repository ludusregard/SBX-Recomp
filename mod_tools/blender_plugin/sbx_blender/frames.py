"""Coordinate frames: game spaces to Blender's.

Car space (docs/game_data.md, "Car space and the sets compared") is
meters, x forward, y up, z to the car's left. In Blender the car stands
on z up and faces -y, with its left side on +x (the placement Blender's
glTF importer gave the earlier glTF route). Car
space is left-handed, so the map is a mirror: triangle winding flips.

Matrices are for row vectors (p_blender = p_car @ CAR_TO_BLENDER), like
the game's data.
"""

import numpy as np

from .sbx_formats.cars import NODE_TO_CAR

CAR_TO_BLENDER = np.array([[0.0, -1.0, 0.0],    # car x (forward) -> -y
                           [0.0, 0.0, 1.0],     # car y (up)      -> z
                           [1.0, 0.0, 0.0]])    # car z (left)    -> x

# Node frame (decimeters, x left, y up, z rear) -> Blender.
NODE_TO_BLENDER = NODE_TO_CAR @ CAR_TO_BLENDER


def flip_winding(triangles):
    """The frame is a mirror: swap two corners to keep the faces' front."""
    return triangles[:, [1, 0, 2]]


def _quat_to_matrix(q):
    """Quaternion (x, y, z, w) -> 3x3 rotation for column vectors."""
    x, y, z, w = q
    n = x * x + y * y + z * z + w * w
    if n < 1e-12:
        return np.eye(3)
    s = 2.0 / n
    return np.array([
        [1 - s * (y * y + z * z), s * (x * y - z * w), s * (x * z + y * w)],
        [s * (x * y + z * w), 1 - s * (x * x + z * z), s * (y * z - x * w)],
        [s * (x * z - y * w), s * (y * z + x * w), 1 - s * (x * x + y * y)]])


def node_local(node, frame=NODE_TO_BLENDER):
    """A node's local transform (node frame) -> Blender 4x4 (column vectors).

    The node file's local transform is p' = p * scale * R + t (row
    vectors); frame maps the node frame to Blender.
    """
    a = np.eye(4)
    a[:3, :3] = _quat_to_matrix(node.rotation) * np.asarray(node.scale)
    a[:3, 3] = node.translation
    g = np.eye(4)
    g[:3, :3] = frame.T
    return g @ a @ np.linalg.inv(g)
