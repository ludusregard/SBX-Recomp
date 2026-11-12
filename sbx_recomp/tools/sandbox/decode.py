"""Lists a hooks file's Fill tables as plain stores (offset, size, value),
with image words shown as floats, to read when laying out structs."""
import re, sys, struct
img = open('/home/claude/img/image.bin', 'rb').read()
def word(a): return struct.unpack_from('>I', img, a - 0x82000000)[0]
def fl(w): return struct.unpack('>f', struct.pack('>I', w))[0]
src = open(sys.argv[1]).read()
for m in re.finditer(r'constexpr Fill (\w+)\[\] = \{(.*?)\};', src, re.S):
    print('==', m.group(1))
    for e in re.finditer(r'\{([^{}]+)\}', m.group(2)):
        p = [x.strip() for x in e.group(1).split(',')]
        off = int(p[0], 0); kind = p[1]; val = int(p[2], 0)
        cnt = int(p[3], 0) if len(p) > 3 else 1; stride = int(p[4], 0) if len(p) > 4 else 0
        for i in range(cnt):
            o = off + i * stride
            if kind.endswith('Image'):
                w = word(val); v = f'{fl(w):.9g}f ({w:08X} from {val:08X})'
            elif kind.endswith('Arg'): v = f'r{val}'
            elif kind.endswith('Self'): v = f'self+{val}'
            else: v = f'{val:#x}'
            sz = {'kHalf': 2, 'kByte': 1, 'kAbsHalf': 2, 'kAbsByte': 1}.get(kind, 4)
            print(f'  {o:#x} ({o})' if kind.startswith('kAbs') else f'  +{o}', sz, v)
