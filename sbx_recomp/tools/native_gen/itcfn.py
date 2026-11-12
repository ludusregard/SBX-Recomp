# print the ITC ported C++ function(s) whose body names sub_X in ITC_NATIVE_RUNNING
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import paths
import re,sys,glob
d=sys.argv[1]; addrs=[a.upper() for a in sys.argv[2:]]
for f in sorted(glob.glob(os.path.join(paths.itc_ported(), d, '*.cpp'))):
    t=open(f).read()
    for a in addrs:
        for m in re.finditer(r'ITC_NATIVE_RUNNING\("sub_'+a, t, re.I):
            # walk back to function start: previous line at column 0 that ends with '{' 
            s=t.rfind('\n}\n',0,m.start()); s=0 if s<0 else s+3
            # include preceding comment
            e=t.find('\n}\n',m.start()); print(f'--- {f.split("/")[-1]} {a}'); print(t[s:e+2])
