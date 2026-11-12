import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import paths
import re,sys,pickle,glob,os
gen,ov,grade=paths.load_index()
d=sys.argv[1]
seen={}
for f in sorted(glob.glob(os.path.join(paths.itc_ported(), d, '*'))):
    for m in re.finditer(r'ITC_NATIVE_RUNNING\("sub_([0-9A-Fa-f]{8}) ([^"]*)"',open(f).read()):
        a=m.group(1).upper(); seen.setdefault(a,(m.group(2),os.path.basename(f)))
for a,(n,f) in seen.items():
    g=grade.get(a,('?','',''))
    body=gen.get(a,'')
    calls=sorted(set(re.findall(r'\bsub_([0-9A-F]{8})\(ctx',body)))
    print(f"{a} {g[0]} {g[1]:12} lines={body.count(chr(10)):4} calls={len(calls):2} {'OVR:'+','.join(ov[a]) if a in ov else ''} {n} [{f}]")
print(len(seen),'native functions')
