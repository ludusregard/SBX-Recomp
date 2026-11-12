"""Stages every file that differs between the working copy (wp) and the PC
mirror (pc) into a fresh output folder, with the repo's line endings, and
writes the commit list. Lists hooks files the PC still has that wp doesn't
(John deletes those). Usage: batch.py NAME"""
import os, sys, json, hashlib, filecmp
W, P = '/home/claude/wp', '/home/claude/pc'
name = sys.argv[1]; S = f'/mnt/user-data/outputs/{name}'
roots = ['CLAUDE.md', 'docs', 'sbx_recomp/CMakeLists.txt', 'sbx_recomp/src', 'sbx_recomp/tools']
def files(top):
    out = set()
    for r in roots:
        p = os.path.join(top, r)
        if os.path.isfile(p): out.add(r)
        for d, _, fs in os.walk(p):
            if '__pycache__' in d or '/difftest/build' in d: continue
            for f in fs:
                if not f.endswith(('.cpp', '.h', '.md', '.txt', '.py', '.tsv', '.toml', '.inc', '.sh')): continue
                out.add(os.path.relpath(os.path.join(d, f), top))
    return out
def norm(b, crlf):
    b = b.replace(b'\r\n', b'\n')
    return b.replace(b'\n', b'\r\n') if crlf else b
changed = []; hashes = {}
for r in sorted(files(W)):
    new = open(f'{W}/{r}', 'rb').read()
    old = open(f'{P}/{r}', 'rb').read() if os.path.exists(f'{P}/{r}') else None
    crlf = True if old is None else (b'\r\n' in old or old.count(b'\n') == 0)
    new = norm(new, crlf)
    if old == new: continue
    os.makedirs(os.path.dirname(f'{S}/{r}') or S, exist_ok=True)
    open(f'{S}/{r}', 'wb').write(new)
    hashes[r] = hashlib.sha256(new).hexdigest()
    changed.append({'devicePath': 'S:\\Games\\TXR_Modding\\ReSBX\\sbx\\' + r.replace('/', '\\'), 'stagedPath': f'{S}/{r}'})
gone = sorted(r for r in files(P) - files(W) if r.startswith('sbx_recomp/src/'))
json.dump({'hashes': hashes, 'gone': gone}, open(f'/home/claude/tools/{name}.json', 'w'))
print(json.dumps(changed)); print('to delete on the PC:', gone, file=sys.stderr)
