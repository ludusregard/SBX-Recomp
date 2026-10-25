"""A part's objects in Blender: its root, its shapes, markers and new ones.

A part root (an empty tagged by the import) holds the part's objects:
shape meshes, marker empties, and what Add to Part joined to it (new
shapes, new markers). A shape can also hang from one of the part's
markers: the import puts the meter needles under their pivot marker
(sm_hari00_Shape under sm_hari), so rotating the marker swings the
needle, as a rig would.

Part space is the root's space: car space for the parts of a car
(frames.CAR_TO_BLENDER). to_root gives an object's transform in it at
rest: a marker between the object and the root counts with its
position only, so a marker rotated to preview a needle's swing doesn't
move the needle in the export (marker rotations aren't exported
either).
"""

import numpy as np


def is_root(obj):
    return obj is not None and obj.type == 'EMPTY' and 'sbx_part' in obj and 'sbx_sidecar' in obj


def _is_marker(obj):
    return obj.type == 'EMPTY' and ('sbx_marker' in obj or 'sbx_new_marker' in obj)


def root_of(obj):
    """The part root obj belongs to (obj itself if it's one), or None."""
    while obj is not None:
        if is_root(obj):
            return obj
        obj = obj.parent
    return None


def members(root):
    """Every object under root that belongs to its part (not into other
    part roots below it), depth first."""
    out = []
    stack = list(root.children)
    while stack:
        obj = stack.pop(0)
        if is_root(obj):
            continue
        out.append(obj)
        stack[0:0] = list(obj.children)
    return out


def _local(obj):
    return np.array(obj.matrix_parent_inverse @ obj.matrix_basis, np.float64)


def to_root(obj, root):
    """obj's transform in part space at rest (4x4, column vectors)."""
    m = _local(obj)
    p = obj.parent
    while p is not None and p is not root:
        lp = _local(p)
        if _is_marker(p):
            t = np.eye(4)
            t[:3, 3] = lp[:3, 3]
            lp = t
        m = lp @ m
        p = p.parent
    if p is None:
        raise ValueError('%s isn\'t under %s' % (obj.name, root.name))
    return m


def parent_keep_transform(obj, parent):
    """Parent obj to parent without moving it."""
    world = obj.matrix_world.copy()
    obj.parent = parent
    obj.matrix_parent_inverse = parent.matrix_world.inverted()
    obj.matrix_basis = world   # world = parent world @ its inverse @ basis


class object_mode:
    """with object_mode(): ... runs in Object Mode, then goes back to Edit
    Mode if that's where it was. A mesh in Edit Mode keeps its latest
    edits out of its data (faces, slots, UVs) until Edit Mode is left."""

    def __enter__(self):
        import bpy
        self.was = None
        obj = bpy.context.view_layer.objects.active if bpy.context.view_layer else None
        if obj is not None and obj.mode == 'EDIT':
            self.was = 'EDIT'
            bpy.ops.object.mode_set(mode='OBJECT')
        return self

    def __exit__(self, *exc):
        import bpy
        if self.was:
            try:
                bpy.ops.object.mode_set(mode=self.was)
            except RuntimeError:
                pass
        return False
