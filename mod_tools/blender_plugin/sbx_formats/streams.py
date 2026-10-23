"""Vertex streams of a PART shape.

A stream is count x stride bytes holding one attribute group. Known types
decode to numpy arrays; any other type is kept as raw bytes only (see
UNKNOWN_TYPES and docs/game_data.md, "Part files").
"""

import numpy as np

COLOR = 0x00          # color                      (4 bytes)
SKIN_POSITION = 0x05  # float4 position, w = 1     (16 bytes)
SKIN_NORMAL = 0x07    # float4 normal, w = 1       (16 bytes)
POSITION = 0x0B       # float3 position            (12 bytes)
NORMAL = 0x0C         # half4 normal, w = 1        (8 bytes)
COLOR_UV = 0x0E       # color, half2 UV            (8 bytes)
COLOR_UV2 = 0x0F      # color, half2 UV, half2 UV  (12 bytes)
COLOR_UV3 = 0x10      # color, 3 x half2 UV        (16 bytes; UV3 = UV1 in every shape)

KNOWN_TYPES = {COLOR: 4, SKIN_POSITION: 16, SKIN_NORMAL: 16, POSITION: 12,
               NORMAL: 8, COLOR_UV: 8, COLOR_UV2: 12, COLOR_UV3: 16}

# Seen in the data, not fully worked out (carried through raw): 0x0D,
# stride 16, in the normal slot of 2 shapes: the normal (half4), then a
# second unit half4 vector of unknown use.
UNKNOWN_TYPES = {0x0D: 16}

CONTENTS = {COLOR: 'color', SKIN_POSITION: 'position (float4)', SKIN_NORMAL: 'normal (float4)',
            POSITION: 'position', NORMAL: 'normal (half4)', COLOR_UV: 'color, UV',
            COLOR_UV2: 'color, UV, UV2', COLOR_UV3: 'color, UV, UV2, UV3'}


def describe(stype, stride):
    """A stream type in words ('color, UV'), or 'not worked out'."""
    if KNOWN_TYPES.get(stype) == stride:
        return CONTENTS[stype]
    return 'not worked out'


class Stream:
    """One vertex stream: its descriptor and raw data.

    raw is (count, stride) uint8, exactly as stored. size is the
    descriptor's byte size (count x stride in every part seen).
    """

    def __init__(self, stype, stride, raw, size=None):
        self.type = stype
        self.stride = stride
        self.raw = raw
        self.size = raw.size if size is None else size

    @property
    def count(self):
        return self.raw.shape[0]

    @property
    def known(self):
        return self.type in KNOWN_TYPES and KNOWN_TYPES[self.type] == self.stride

    def decode(self):
        """Known type -> dict of attribute arrays; unknown type -> {}."""
        if not self.known:
            return {}
        raw = self.raw
        out = {}
        t = self.type
        if t in (COLOR, COLOR_UV, COLOR_UV2, COLOR_UV3):
            out['color'] = raw[:, 0:4].copy()
            if t != COLOR:
                out['uv'] = raw[:, 4:8].copy().view('>f2').astype(np.float32)
            if t in (COLOR_UV2, COLOR_UV3):
                out['uv2'] = raw[:, 8:12].copy().view('>f2').astype(np.float32)
            if t == COLOR_UV3:
                out['uv3'] = raw[:, 12:16].copy().view('>f2').astype(np.float32)
        elif t == POSITION:
            out['position'] = raw.copy().view('>f4').astype(np.float64)
        elif t == NORMAL:
            out['normal'] = raw.copy().view('>f2').astype(np.float64)[:, :3]
        elif t == SKIN_POSITION:
            out['position'] = raw.copy().view('>f4').astype(np.float64)[:, :3]
        elif t == SKIN_NORMAL:
            out['normal'] = raw.copy().view('>f4').astype(np.float64)[:, :3]
        return out


def read_stream(d, off, count, stype, stride, size):
    raw = np.frombuffer(d, np.uint8, count * stride, off).reshape(count, stride).copy()
    return Stream(stype, stride, raw, size)
