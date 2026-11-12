"""Cross-checks the numbers in the native code against the originals: for
each override, every literal above 8 in its body that isn't a number in the
original's code (an immediate, a lis/addi or lis/ori address, a load's
offset) is listed; then, per file, the same for its helpers and named
constants against all of the file's originals.

  python check_consts.py file [file...]     (names under src/hooks, no .cpp)

Loop strides and offsets the C++ adds up itself show up too: each hit is
something to check by hand, not an error."""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import paths

gen, ov, grade = paths.load_index()
H = paths.HOOKS + os.sep
def asm_consts(a, seen=None):
    seen = seen if seen is not None else set()
    if a in seen or a not in gen: return set()
    seen.add(a)
    out=set(); hi={}
    for l in gen[a].splitlines():
        s=l.strip()
        if not s.startswith('// '): continue
        x=s[3:]; op,_,rest=x.partition(' ')
        ops=[o.strip() for o in rest.split(',')]
        nums=[int(n,0) for n in re.findall(r'(?<![\w])(-?(?:0x[0-9a-f]+|\d+))(?![\w])',rest)]
        for n in nums: out.add(n & 0xFFFFFFFF); out.add(n)
        if op=='lis': hi[ops[0]]=(int(ops[1],0)<<16)&0xFFFFFFFF; out.add(hi[ops[0]])
        elif op in('addi','ori','addis') and len(ops)==3 and ops[1] in hi:
            v=int(ops[2],0)
            if op=='addi': r=(hi[ops[1]]+v)&0xFFFFFFFF
            elif op=='ori': r=hi[ops[1]]|(v&0xFFFF)
            else: r=(hi[ops[1]]+(v<<16))&0xFFFFFFFF
            out.add(r); hi[ops[0]]=r
        elif re.match(r'(lwz|lbz|lhz|lha|lfs|lfd|stw|stb|sth|stfs|stfd|ld|std)u?$',op) and len(ops)==2:
            m=re.match(r'(-?\d+)\((r\d+)\)',ops[1])
            if m and m.group(2) in hi: out.add((hi[m.group(2)]+int(m.group(1)))&0xFFFFFFFF)
        # addis on non-lis base: offset of a large struct
        if op=='addis' and len(ops)==3: out.add((int(ops[2],0)<<16)&0xFFFFFFFF)
    # also allow sums of addis+addi chains on the same reg (struct offsets)
    return out
files = sys.argv[1:]
for f in files:
    s=open(H+f+'.cpp').read()
    # split overrides; helpers above count for every override, so check per override body only
    for m in re.finditer(r'extern "C" REX_FUNC\(sub_([0-9A-F]{8})\) \{\n(.*?)^\}', s, re.S|re.M):
        a,body=m.group(1),m.group(2)
        if 'guest_memory::Apply' in body and 'sub_' not in body.replace('__imp__sub_','') : pass
        body=re.sub(r'__imp__sub_\w+|sub_[0-9A-F]{8}|//[^\n]*','',body)
        lits=set(int(n,0) for n in re.findall(r'(?<![\w.])(0x[0-9A-Fa-f]+|\d+)(?:u|ull|ULL|U)?(?![\w.])',body))
        ac=asm_consts(a)
        # include constants of called helpers' named constants? only literals here
        bad=sorted(n for n in lits if n>8 and n not in ac and (n|0xFFFF0000) not in ac and ((-n)&0xFFFFFFFF) not in ac)
        if bad: print(f,a,bad[:12])

print('==== named constants / helpers')
for f in files:
    s=open(H+f+'.cpp').read()
    addrs=set(re.findall(r'REX_FUNC\(sub_([0-9A-F]{8})\)',s))
    # include callees referenced (helpers call into them) for broader asm set
    ac=set()
    for a in addrs: ac|=asm_consts(a)
    # strip overrides and Fill tables
    rest=re.sub(r'extern "C" REX_FUNC\(sub_[0-9A-F]{8}\) \{\n.*?^\}', '', s, flags=re.S|re.M)
    rest=re.sub(r'constexpr Fill \w+\[\] = \{.*?\};', '', rest, flags=re.S)
    rest=re.sub(r'//[^\n]*|REXCVAR_DEFINE_BOOL\(.*?\);|#include[^\n]*|__imp__sub_\w+|sub_[0-9A-F]{8}', '', rest, flags=re.S)
    lits=set(int(n,0) for n in re.findall(r'(?<![\w.])(0x[0-9A-Fa-f]+|\d+)(?:u|ull|ULL|U|f)?(?![\w.])',rest))
    bad=sorted(n for n in lits if n>8 and n not in ac and (n|0xFFFF0000) not in ac and ((-n)&0xFFFFFFFF) not in ac)
    if bad: print(f,[hex(b) if b>0xFFFF else b for b in bad])
