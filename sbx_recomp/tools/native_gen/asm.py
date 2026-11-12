import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import paths
import re,sys,pickle
gen,ov,grade=paths.load_index()
def asm(a):
    out=[]
    for l in gen[a.upper()].splitlines():
        s=l.strip()
        if s.startswith('// '):
            x=s[3:]
            if re.match(r'(mflr r12|stw r12,-8\(r1\)|lwz r12,-8\(r1\)|mtlr r12|std r(2\d|3[01]),-\d+\(r1\)|ld r(2\d|3[01]),-\d+\(r1\)|stfd f\d+,-\d+\(r1\)|lfd f\d+,-\d+\(r1\)|bl 0x8258c(5|6)..|b 0x8258c6..)$',x): continue
            out.append('  '+x)
        elif re.match(r'loc_[0-9A-F]+:',s): out.append(s)
        elif s.startswith('DEFINE_REX_FUNC'): out.append(s)
    return '\n'.join(out)
if __name__=='__main__':
    for a in sys.argv[1:]:
        g=grade.get(a.upper(),('?','',''))
        print(f'=== {a} grade {g[0]} {g[1]} itc {g[2]}'); print(asm(a))
