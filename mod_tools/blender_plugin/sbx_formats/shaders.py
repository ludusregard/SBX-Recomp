"""The shader section of a PART file, and the shader library.

A part's shader section (docs/game_data.md, "Part files" and "Shaders"):

    +0x00  pair count, then the section size less 0x20 x (pairs + 1),
           then 24 zero bytes
    +0x20  per pair: a 0x20-byte entry (pair size, VS size, PS size, 0,
           then 16 bytes not worked out), the vertex shader, the pixel
           shader

Each shader starts with its magic (0x102A0E01 vertex, 0x102A0E00 pixel),
its header size (the microcode's offset) and the microcode's size.

The shader library (library.py) collects every pair of every part,
keyed by the hashes of its shaders' microcode, so a material can use a
pair another part has.
"""

import hashlib
import struct
import sys

# Short ids for the 39 shaders in the game's part files (11 vertex, 28
# pixel), numbered in order of first use over the parts by id. Keyed by
# shader_hash. A shader not in the table gets 'vs_new'/'ps_new' and its
# hash.
SHADER_IDS = {
    '48439039777d64bf': 'vs_00',
    'b588e1e3ebbe7153': 'vs_01',
    '11af1467d4625931': 'vs_02',
    '22d03b7ac226f511': 'vs_03',
    '326929f21ea15378': 'vs_04',
    '1715fed6bae0f464': 'vs_05',
    '5eeb3870970de693': 'vs_06',
    '73ed0c4b5395015d': 'vs_07',
    '8f69e6f05f9194e8': 'vs_08',
    '67a7abd402f8651c': 'vs_09',
    '0187624dac8d63a9': 'vs_10',
    '726f9680a380465d': 'ps_00',
    '7c9395c88b13e506': 'ps_01',
    'bac6ef2d132bfa50': 'ps_02',
    '2e86071cec3eefcf': 'ps_03',
    '1d6d7ff0df1998fa': 'ps_04',
    '9a07f93a46cf3503': 'ps_05',
    '2cbec30a512a8e95': 'ps_06',
    '0cf4d49cf8ee62bf': 'ps_07',
    '12dd1294caaff2da': 'ps_08',
    '1cdea8fcdb4ff9f8': 'ps_09',
    'dd82887ab5efc12c': 'ps_10',
    '69eb0eab5e84cbab': 'ps_11',
    '5c084ba8d1529ee1': 'ps_12',
    '8ca17c86e8250e68': 'ps_13',
    '8d2c32389936edc6': 'ps_14',
    '3128c3aa698e0847': 'ps_15',
    '27fcd1e65c5b9279': 'ps_16',
    '0362b10ec9572f3e': 'ps_17',
    'd245f50399c46e52': 'ps_18',
    '08c38a5c71b568cd': 'ps_19',
    '49c35b2c7e32eeb3': 'ps_20',
    '288456f26cbb114d': 'ps_21',
    '7e98c0ddbcdc01c4': 'ps_22',
    'e12ffae501fe1117': 'ps_23',
    'f95e96eca4cf7818': 'ps_24',
    '03b87e855809c150': 'ps_25',
    '52541ea75fe7ce80': 'ps_26',
    'e2385bcd640ac8b0': 'ps_27',
}


def shader_id(shader_hash_, kind):
    return SHADER_IDS.get(shader_hash_, '%s_new_%s' % (kind, shader_hash_))


def pair_key(ids):
    """'vs_02 + ps_03' -> 'vs_hash:ps_hash' (the library's key), or None."""
    by_id = {v: k for k, v in SHADER_IDS.items()}
    try:
        vs, ps = [x.strip() for x in ids.split('+')]
    except (AttributeError, ValueError):
        return None
    for x in (vs, ps):
        if x not in by_id and '_new_' in x:
            by_id[x] = x.split('_new_', 1)[1]
    if vs not in by_id or ps not in by_id:
        return None
    return by_id[vs] + ':' + by_id[ps]


PAIR_ENTRY_SIZE = 0x20
SECTION_HEADER_SIZE = 0x20
VS_MAGIC = 0x102A0E01
PS_MAGIC = 0x102A0E00


class ShaderPair:
    """One compiled VS/PS pair: its 0x20-byte entry and the two shaders, raw."""

    def __init__(self, entry, vs, ps):
        self.entry = bytes(entry)
        self.vs = bytes(vs)
        self.ps = bytes(ps)

    @property
    def raw(self):
        return self.entry + self.vs + self.ps

    @property
    def vs_hash(self):
        return shader_hash(self.vs)

    @property
    def ps_hash(self):
        return shader_hash(self.ps)

    @property
    def ids(self):
        """'vs_02 + ps_03'."""
        return '%s + %s' % (shader_id(self.vs_hash, 'vs'), shader_id(self.ps_hash, 'ps'))


def microcode(shader):
    """A shader's microcode (after its header), or the whole thing if the
    header doesn't fit."""
    if len(shader) < 12:
        return shader
    hsize, msize = struct.unpack_from('>2I', shader, 4)
    if hsize + msize > len(shader):
        return shader
    return shader[hsize:hsize + msize]


def shader_hash(shader):
    """Library key: the first 16 hex digits of the microcode's SHA-1."""
    return hashlib.sha1(microcode(shader)).hexdigest()[:16]


def shader_kind(shader):
    magic = struct.unpack_from('>I', shader, 0)[0] if len(shader) >= 4 else 0
    return {VS_MAGIC: 'vs', PS_MAGIC: 'ps'}.get(magic, 'unknown')


def split_section(raw):
    """Shader section -> (header, [ShaderPair], rest).

    rest is whatever follows the last pair (nothing in every part seen).
    join_section(*split_section(raw)) == raw.
    """
    if len(raw) < SECTION_HEADER_SIZE:
        return raw, [], b''
    count = struct.unpack_from('>I', raw, 0)[0]
    header = raw[:SECTION_HEADER_SIZE]
    pairs = []
    o = SECTION_HEADER_SIZE
    for _ in range(count):
        size, vs_size, ps_size = struct.unpack_from('>3I', raw, o)
        e = raw[o:o + PAIR_ENTRY_SIZE]
        a = o + PAIR_ENTRY_SIZE
        pairs.append(ShaderPair(e, raw[a:a + vs_size], raw[a + vs_size:a + vs_size + ps_size]))
        o += PAIR_ENTRY_SIZE + size
    return header, pairs, raw[o:]


def join_section(header, pairs, rest=b''):
    return header + b''.join(p.raw for p in pairs) + rest


# ---------------------------------------------------------------- library

def main(argv=None):
    """The shader library moved to library.py (it now keeps examples)."""
    from . import library
    return library.main(argv)


if __name__ == '__main__':
    sys.exit(main())
