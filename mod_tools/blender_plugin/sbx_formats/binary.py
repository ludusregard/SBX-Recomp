"""Big-endian helpers shared by the format modules."""

import struct


def u32(d, o):
    return struct.unpack_from('>I', d, o)[0]


def s32(d, o):
    return struct.unpack_from('>i', d, o)[0]


def cstr(b):
    """Zero-terminated name; bytes outside printable ASCII become '_'."""
    b = b.split(b'\0')[0]
    return ''.join(chr(c) if 32 <= c < 127 else '_' for c in b)


class Record:
    """A fixed-size record kept as its raw bytes, with named fields on top.

    fields is a layout table: (name, offset, struct format) in offset
    order. Names starting with 'unknown_' are byte ranges not worked out;
    they're only ever carried through. get() decodes a field from the raw
    bytes, so the bytes stay the single source of truth.
    """

    FIELDS = ()
    SIZE = 0

    def __init__(self, raw):
        if len(raw) != self.SIZE:
            raise ValueError('%s: %d bytes, expected %d' % (type(self).__name__, len(raw), self.SIZE))
        self.raw = bytes(raw)

    @classmethod
    def field(cls, name):
        for f in cls.FIELDS:
            if f[0] == name:
                return f
        raise KeyError(name)

    def get(self, name):
        _, off, fmt = self.field(name)
        v = struct.unpack_from('>' + fmt, self.raw, off)
        return v[0] if len(v) == 1 else v

    def set(self, name, value):
        _, off, fmt = self.field(name)
        b = bytearray(self.raw)
        if not isinstance(value, (tuple, list)) or fmt.endswith('s'):
            value = (value,)
        struct.pack_into('>' + fmt, b, off, *value)
        self.raw = bytes(b)

    def field_bytes(self, name):
        _, off, fmt = self.field(name)
        return self.raw[off:off + struct.calcsize('>' + fmt)]


def check_layout(fields, size):
    """Fields must tile the record exactly: no gaps, no overlaps."""
    pos = 0
    for name, off, fmt in fields:
        if off != pos:
            raise AssertionError('layout: %s at 0x%X, expected 0x%X' % (name, off, pos))
        pos += struct.calcsize('>' + fmt)
    if pos != size:
        raise AssertionError('layout ends at 0x%X, expected 0x%X' % (pos, size))
