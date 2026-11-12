"""Struct layout helper: python3 layout.py SPEC prints C++ structs.
SPEC lines:  struct Name SIZE  /  OFFSET TYPE NAME [COUNT] [// comment]  /  end
Gaps become unknown_<offset> byte arrays; each named field gets a
GUEST_OFFSET, the struct a GUEST_SIZE."""
import sys, re
SIZES = {'FixedList<16,Entry20>': 396, 'GameObject': 336, 'GameObject384': 384, 'GameObject1104': 1104, 'Buffer28': 28, 'XprHolder': 56, 'StateMachine': 9608, 'CollisionShape': 48, 'Vector': 16, 'Matrix': 64, 'f32_be': 4, 'u32_be': 4, 'i32_be': 4, 'u16_be': 2,
         'i16_be': 2, 'uint8_t': 1, 'int8_t': 1, 'char': 1, 'u32_le': 4}
out = []; cur = None
def flush():
    name, size, fields = cur
    lines = [f'struct {name} {{']; at = 0; checks = []
    for off, typ, fname, cnt, com in sorted(fields):
        if off < at: raise SystemExit(f'{name}: {fname} at {off} overlaps (at {at})')
        if off > at: lines.append(f'  uint8_t unknown_{at}[{off - at}];')
        sz = SIZES.get(typ) or (4 if typ.startswith('ptr<') else None)
        if sz is None: raise SystemExit(f'size of {typ}?')
        lines.append(f'  {typ} {fname}' + (f'[{cnt}]' if cnt > 1 else '') + ';' + (f'  // {com}' if com else ''))
        at = off + sz * cnt
        if not fname.startswith('unknown_'): checks.append(f'GUEST_OFFSET({name}, {fname}, {off});')
    if size and size > at: lines.append(f'  uint8_t unknown_{at}[{size - at}];')
    if size and size < at: raise SystemExit(f'{name}: fields end at {at}, past {size}')
    lines.append('};')
    if size: lines.append(f'GUEST_SIZE({name}, {size});')
    lines += checks
    out.append('\n'.join(lines)); SIZES[name] = size
for raw in open(sys.argv[1]):
    l, _, com = raw.partition('//'); l = l.split(); com = com.strip()
    if not l: continue
    if l[0] == 'struct': cur = (l[1], int(l[2], 0) if len(l) > 2 else 0, []); continue
    if l[0] == 'end': flush(); cur = None; continue
    cur[2].append((int(l[0], 0), l[1], l[2], int(l[3]) if len(l) > 3 else 1, com))
print('\n\n'.join(out))
