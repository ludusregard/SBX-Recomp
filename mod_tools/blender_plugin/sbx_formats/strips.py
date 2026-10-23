"""Triangle strips as the PART files store them.

Indices are big-endian u16; 0xFFFF restarts the strip. Triangle k is
(i[k], i[k+1], i[k+2]) with the first two swapped when k is odd, which is
counter-clockwise against the stored normals. Degenerate triangles join
strips. See docs/game_data.md, "Part files".
"""

import numpy as np

RESTART = 0xFFFF
PRIM_TRIANGLE_STRIP = 6


def strip_to_triangles(indices):
    """Triangle strips with 0xFFFF restarts -> (n, 3) array.

    Degenerate triangles (the ones joining strips) are dropped.
    """
    idx = np.asarray(indices, np.int64)
    out = []
    cuts = np.flatnonzero(idx == RESTART)
    start = 0
    for end in list(cuts) + [len(idx)]:
        s = idx[start:end]
        start = end + 1
        if len(s) < 3:
            continue
        t = np.stack([s[:-2], s[1:-1], s[2:]], 1)
        odd = np.arange(len(t)) & 1 == 1
        t[odd, 0], t[odd, 1] = t[odd, 1].copy(), t[odd, 0].copy()
        out.append(t)
    if not out:
        return np.zeros((0, 3), np.uint32)
    t = np.concatenate(out)
    keep = (t[:, 0] != t[:, 1]) & (t[:, 1] != t[:, 2]) & (t[:, 0] != t[:, 2])
    return t[keep].astype(np.uint32)
