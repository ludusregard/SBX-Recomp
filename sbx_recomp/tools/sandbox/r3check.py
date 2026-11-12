import re,os,sys
OLD='/home/claude/workpack/sbx_recomp/src/hooks'; NEW='/home/claude/wp/sbx_recomp/src/game'
def bodies(path):
    s=open(path).read(); out={}
    for m in re.finditer(r'REX_FUNC\((sub_[0-9A-F]+)\)\s*\{',s):
        i=m.end(); d=1
        while d: 
            d+= {'{':1,'}':-1}.get(s[i],0); i+=1
        out[m.group(1)]=s[m.end():i-1]
    return out
old={};new={}
for f in os.listdir(OLD):
    if f.endswith('.cpp'):
        for k,v in bodies(f'{OLD}/{f}').items(): old[k]=(f,v)
for f in os.listdir(NEW):
    if f.endswith('.cpp'):
        for k,v in bodies(f'{NEW}/{f}').items(): new[k]=(f,v)
callre=re.compile(r'\bsub_[0-9A-F]+\(ctx, base\)')
for k,(nf,nb) in sorted(new.items()):
    if k not in old: continue
    of,ob=old[k]
    ob=ob.split('return;\n  }',1)[-1]
    calls=[m.end() for m in callre.finditer(ob)]
    if not calls: continue
    tail=ob[calls[-1]:]
    m=re.search(r'ctx\.r3\.u64 = ([^;]+);',tail)
    if not m: continue
    nb2=nb.split('Guest g(ctx, base);',1)[-1] if 'Guest g' in nb else nb
    ncalls=[m2.end() for m2 in re.finditer(r'\.Call(?:Address|Virtual)?\(|\bsub_[0-9A-F]+\(ctx, base\)|\b[A-Z]\w*\(g,|\(g, ',nb2)]
    ntail=nb2[ncalls[-1]:] if ncalls else nb2
    if 'ctx.r3' not in ntail and 'Return' not in ntail:
        print(nf, k, 'old returns', m.group(1))
print('--- regs set at end in old, never in new')
for k,(nf,nb) in sorted(new.items()):
    if k not in old: continue
    of,ob=old[k]; ob=ob.split('return;\n  }',1)[-1]
    calls=[m.end() for m in callre.finditer(ob)]
    tail=ob[calls[-1]:] if calls else ob
    regs=set(re.findall(r'ctx\.((?:f|r|v)\d+)\.',tail))-{'r3'}
    for r in regs:
        if f'ctx.{r}.' not in nb: print(nf,k,r, re.findall(rf'ctx\.{r}\.[^;]*;',tail)[:2])
