import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import paths
import sys
# lis/addi pairs: la.py HI LO [HI LO...]  (signed decimal as in the asm)
a=sys.argv[1:]
for i in range(0,len(a),2):
    hi=int(a[i]); lo=int(a[i+1]) if i+1<len(a) else 0
    print(f'lis {hi} addi {lo} -> 0x{((hi<<16)+lo)&0xFFFFFFFF:08X}')
