"""Load an Xbox 360 executable as its in-memory image, with its sections.

Accepts:
  - a XEX2 whose data is unencrypted and either uncompressed or "basic"
    compressed (zero runs), e.g. default.xex.decrypt from xextool -e u -c u;
  - a flat memory dump of the image (pass base=0x82000000 etc.).
An encrypted or LZX-compressed XEX (the retail default.xex) is refused: use
the decrypted copy.

    img = load('assets/default.xex.decrypt')
    img.word(0x82090000); img.sections; img.code_ranges(); img.data_ranges()
"""
import struct
from dataclasses import dataclass, field

XEX_FILE_FORMAT_INFO = 0x000003FF
XEX_ENTRY_POINT = 0x00010100
XEX_IMAGE_BASE = 0x00010201
XEX_EXECUTION_INFO = 0x00040006

IMAGE_SCN_MEM_EXECUTE = 0x20000000


@dataclass
class Section:
    name: str
    start: int
    size: int
    flags: int

    @property
    def end(self):
        return self.start + self.size

    @property
    def executable(self):
        return bool(self.flags & IMAGE_SCN_MEM_EXECUTE)


@dataclass
class Image:
    base: int
    data: bytes
    entry: int = 0
    title_id: int = 0
    sections: list = field(default_factory=list)

    def word(self, addr):
        o = addr - self.base
        return struct.unpack_from('>I', self.data, o)[0]

    def contains(self, addr):
        return 0 <= addr - self.base <= len(self.data) - 4

    def section(self, name):
        return next((s for s in self.sections if s.name == name), None)

    def code_ranges(self, all_executable=False):
        """.text by default; every executable section with all_executable."""
        secs = [s for s in self.sections if s.executable] if all_executable else \
            [s for s in self.sections if s.name == '.text']
        return [(s.start, s.end) for s in secs]

    def pdata_functions(self):
        """(start, size) of every function in .pdata (non-leaf functions;
        leaf functions have no entry)."""
        p = self.section('.pdata')
        if not p:
            return []
        out = []
        for o in range(p.start, p.end - 7, 8):
            begin, packed = self.word(o), self.word(o + 4)
            if begin:
                out.append((begin, ((packed >> 8) & 0x3FFFFF) * 4))
        return sorted(out)

    def data_ranges(self):
        return [(s.start, s.end) for s in self.sections if s.name in ('.rdata', '.data')]


def _parse_pe_sections(image, base):
    if image[:2] != b'MZ':
        return []
    e_lfanew = struct.unpack_from('<I', image, 0x3C)[0]
    if image[e_lfanew:e_lfanew + 4] != b'PE\0\0':
        return []
    nsec, = struct.unpack_from('<H', image, e_lfanew + 6)
    optsz, = struct.unpack_from('<H', image, e_lfanew + 20)
    pe_base, = struct.unpack_from('<I', image, e_lfanew + 24 + 28)
    off = e_lfanew + 24 + optsz
    out = []
    for i in range(nsec):
        s = image[off + i * 40: off + i * 40 + 40]
        name = s[:8].rstrip(b'\0').decode('ascii', 'replace')
        vsize, va = struct.unpack_from('<II', s, 8)
        flags, = struct.unpack_from('<I', s, 36)
        out.append(Section(name, pe_base + va, vsize, flags))
    return out


def _load_xex(raw):
    _, _, pe_off, _, sec_off, nopt = struct.unpack_from('>4sIIIII', raw, 0)
    opts = {}
    for i in range(nopt):
        k, v = struct.unpack_from('>II', raw, 24 + i * 8)
        opts[k] = v
    image_size, = struct.unpack_from('>I', raw, sec_off + 4)
    base = opts.get(XEX_IMAGE_BASE) or struct.unpack_from('>I', raw, sec_off + 0x110)[0]
    ff = opts.get(XEX_FILE_FORMAT_INFO)
    enc, comp = (0, 0)
    if ff:
        info_size, enc, comp = struct.unpack_from('>IHH', raw, ff)
    if enc != 0:
        raise ValueError('XEX data is encrypted; use the decrypted copy (xextool -e u)')
    body = raw[pe_off:]
    if comp == 0:
        image = body[:image_size]
    elif comp == 1:  # basic: (data size, zero size) blocks
        image = bytearray()
        pos = 0
        for i in range((info_size - 8) // 8):
            dsz, zsz = struct.unpack_from('>II', raw, ff + 8 + i * 8)
            image += body[pos:pos + dsz]
            image += b'\0' * zsz
            pos += dsz
        image = bytes(image)
    else:
        raise ValueError(f'XEX compression {comp} (LZX?) not supported; use xextool -c u')
    if len(image) < image_size:
        image += b'\0' * (image_size - len(image))
    title_id = 0
    if XEX_EXECUTION_INFO in opts:
        title_id, = struct.unpack_from('>I', raw, opts[XEX_EXECUTION_INFO] + 12)
    return Image(base, image, opts.get(XEX_ENTRY_POINT, 0), title_id,
                 _parse_pe_sections(image, base))


def load(path, base=0x82000000):
    raw = open(path, 'rb').read()
    if raw[:4] == b'XEX2':
        return _load_xex(raw)
    return Image(base, raw, sections=_parse_pe_sections(raw, base))
