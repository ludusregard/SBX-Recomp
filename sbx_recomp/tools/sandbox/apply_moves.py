"""Moves converted files: every src/game/X.cpp that still has a src/hooks/X.cpp
removes the old one, and each merged hooks file (MERGED, old -> new) is
removed too. CMake, the docs and comments are pointed at the new files."""
import os, re, glob, json
W = '/home/claude/wp'
MERGED = json.load(open('/home/claude/tools/merged.json')) if os.path.exists('/home/claude/tools/merged.json') else {}
game = sorted(os.path.basename(p) for p in glob.glob(W + '/sbx_recomp/src/game/*.cpp'))
moved = [n for n in game if os.path.exists(f'{W}/sbx_recomp/src/hooks/{n}')]
gone = [o for o in MERGED if os.path.exists(f'{W}/sbx_recomp/src/hooks/{o}')]
for n in moved + gone:
    os.remove(f'{W}/sbx_recomp/src/hooks/{n}')
texts = [W + '/sbx_recomp/CMakeLists.txt', W + '/CLAUDE.md'] + [d for d in glob.glob(W + '/docs/*.md') if not d.endswith(('rexglue-removal.md', 'itc-parity.md', 'porting-from-itc.md'))] + \
    [p for p in glob.glob(W + '/sbx_recomp/src/**/*', recursive=True) if p.endswith(('.cpp', '.h'))] + \
    glob.glob(W + '/sbx_recomp/tools/**/*.md', recursive=True)
for p in texts:
    raw = open(p, encoding='utf-8', newline='').read(); s = raw
    if p.endswith('CMakeLists.txt'):
        for o in MERGED:
            s = re.sub(r'[ \t]*src/hooks/' + re.escape(o) + r'\r?\n', '', s)
    for o, n in MERGED.items():
        s = re.sub(r'(?<![\w/])((?:sbx_recomp/)?src/)?hooks/' + re.escape(o), lambda m: (m.group(1) or '') + 'game/' + n, s)
    for n in game:
        s = re.sub(r'(?<![\w/])((?:sbx_recomp/)?src/)?hooks/' + re.escape(n), lambda m: (m.group(1) or '') + 'game/' + n, s)
    if s != raw:
        open(p, 'w', encoding='utf-8', newline='').write(s); print('updated', os.path.relpath(p, W))
print('moved now:', moved, 'merged now:', gone)
