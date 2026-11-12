"""Checks native overrides against the recompiled functions they replace.

  python dt.py FILE.cpp [FILE.cpp...] [--seeds=N] [--only=ADDR,...]

For each REX_FUNC override in a file under src/game or src/hooks, the recompiled
original and the override are built into one program and each is run on the
same guest memory (pages filled with a fixed pattern as they're first
touched). Callees are stubs that record their arguments and return fixed
values; leaves, and callees the override does in line, run for real. What
each run wrote to memory, the calls it made with the registers their callees
read, and r3 and f1 at the end are compared.

Linux only (clang++, mmap, signals); needs the index (index.py). A helper
for writing native code: nothing here is part of the game's build. A
difference isn't always a fault: r3 left by a function that returns nothing,
or an argument register the callee doesn't really read. "Could not run"
means the original ran away on the test's random memory."""
import sys, os, re, subprocess, pickle, hashlib, collections
D = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(D))
import paths
W = paths.REPO
INC = ['-I' + D + '/shim', f'-I{W}/sbx_recomp/src', f'-I{W}/sbx_recomp', f'-I{W}/sbx_recomp/generated/default',
       f'-I{W}/rexglue-sdk/include', f'-I{W}/rexglue-sdk/thirdparty/fmt/include',
       f'-I{W}/rexglue-sdk/thirdparty/spdlog/include', f'-I{W}/rexglue-sdk/thirdparty/simde',
       f'-I{W}/rexglue-sdk/thirdparty', '-DSPDLOG_FMT_EXTERNAL', '-DFMT_HEADER_ONLY', '-DSPDLOG_HEADER_ONLY']
CXX = ['clang++', '-std=c++23', '-O1', '-w', '-mssse3', '-msse4.1', '-fno-strict-aliasing'] + INC

def sh(cmd, **kw):
    return subprocess.run(cmd, capture_output=True, text=True, **kw)

_GEN = None
def helpers(gen):
    """The register save and restore helpers (__savegprlr_14 and the like), from
    the generated code."""
    import glob
    out = []
    for path in sorted(glob.glob(os.path.join(paths.GENERATED, 'sbx_recomp.*.cpp'))):
        t = open(path, encoding='utf-8', errors='replace').read()
        idx = [(m.start(), m.group(1)) for m in re.finditer(r'^DEFINE_REX_FUNC\((\w+)\)', t, re.M)]
        for i, (s, n) in enumerate(idx):
            if n.startswith('__'):
                out.append(t[s:idx[i + 1][0] if i + 1 < len(idx) else len(t)])
    return '\n'.join(out)
def build(native, only=None):
    global _GEN
    if _GEN is None:
        _GEN = paths.load_index()
    gen, ov, grade = _GEN
    src = open(native).read()
    addrs = sorted(set(re.findall(r'REX_FUNC\(sub_([0-9A-F]{8})\)', src)))
    addrs = [a for a in addrs if a in gen]
    test = [a for a in addrs if not only or a in only]
    key = hashlib.md5((src + ','.join(test) + open(D + '/dt.py').read() + open(D + '/driver.cpp').read()).encode()).hexdigest()[:10]
    out = os.path.join(D, 'build', f'{os.path.basename(native)}.{key}')
    os.makedirs(out, exist_ok=True)
    exe = out + '/t'
    if os.path.exists(exe):
        return exe, test
    with open(out + '/orig.cpp', 'w') as f:
        f.write('#include "sbx_init.h"\n')
        f.write(helpers(gen) + '\n')
        def is_leaf(t):
            body = gen.get(t, '')
            return bool(body) and not re.search(r'// (bl 0x|bctrl|b 0x(?!8258c6))', body.replace('// blr', ''))
        leaves = set()
        for a in addrs:
            for t in re.findall(r'// bl? 0x([0-9a-f]{8})', gen[a]):
                t = t.upper()
                # Linked for real: leaves, and callees the native never names (its
                # C++ does their work in line).
                if t not in addrs and t in gen and (is_leaf(t) or not re.search(rf'\bsub_{t}\b', src)):
                    leaves.add(t)
        for a in addrs + sorted(leaves):
            f.write(gen[a].split('\nDEFINE_REX_FUNC(__')[0] + '\n')
        f.write('struct Entry { uint32_t addr; PPCFunc* orig; PPCFunc* native; };\n')
        f.write('Entry g_entries[] = {\n' + ''.join(f'  {{0x{a}, &__imp__sub_{a}, &sub_{a}}},\n' for a in test) + '};\n')
        f.write(f'int g_entry_count = {len(test)};\n')
    objs = []
    for name, path in (('native', native), ('orig', out + '/orig.cpp'), ('driver', D + '/driver.cpp')):
        o = f'{out}/{name}.o'
        r = sh(CXX + ['-c', path, '-o', o])
        if r.returncode:
            raise SystemExit(f'compile {name}: ' + r.stderr[:3000])
        objs.append(o)
    stubs = out + '/stubs.cpp'
    open(stubs, 'w').write('')
    for attempt in range(3):
        r = sh(CXX + objs + ([stubs] if attempt else []) + ['-o', exe])
        if r.returncode == 0:
            return exe, test
        und = set(re.findall(r"undefined (?:reference to|symbol:) [`']?([^\s`']+)", r.stderr))
        subs = sorted(u for u in und if re.fullmatch(r'(__imp__)?sub_[0-9A-F]{8}', u))
        cv = sorted(u for u in und if u.startswith('cvar_'))
        other = sorted(und - set(subs) - set(cv))
        with open(stubs, 'a') as f:
            if attempt == 0:
                f.write('#include "sbx_init.h"\nextern "C" void difftest_stub(uint32_t, PPCContext&, uint8_t*);\n')
            for c in cv:
                f.write(f'bool {c} = true;\n')
            for s in subs:
                f.write(f'extern "C" void {s}(PPCContext& ctx, uint8_t* base) {{ difftest_stub(0x{s[-8:]}, ctx, base); }}\n')
        if other and attempt == 2:
            raise SystemExit('undefined: ' + ' '.join(other)[:2000] + '\n' + r.stderr[-1500:])
        if other and not subs and not cv:
            raise SystemExit('undefined (non-function): ' + ' '.join(other)[:3000])
    raise SystemExit('link failed: ' + r.stderr[-2000:])

MEM_CAP = 100000  # words kept per run; a longer run (a real copy of a random length) is hashed

def parse(lines):
    runs = collections.OrderedDict()
    cur = None
    for l in lines:
        if l.startswith('RUN '):
            m = re.match(r'RUN (\w+) seed=(\d+) (\w+) status=(\d+)', l)
            cur = dict(status=int(m.group(4)), calls=[], regs={}, mem={}, more=None)
            runs[(m.group(1), int(m.group(2)), m.group(3))] = cur
        elif l.startswith('CALL '):
            p = l.split()
            cur['calls'].append((p[1], dict(x.split('=') for x in p[2:])))
        elif l.startswith('REGS'):
            cur['regs'] = dict(x.split('=') for x in l.split()[1:])
        elif l.startswith('MEM '):
            if len(cur['mem']) < MEM_CAP:
                a, v = l[4:].split('=')
                cur['mem'][a] = v.rstrip()
            else:
                if cur['more'] is None:
                    cur['more'] = hashlib.md5()
                cur['more'].update(l.encode())
    for run in runs.values():
        if run['more'] is not None:
            run['mem']['more'] = run.pop('more').hexdigest()
    return runs

nulls = {}  # address -> seeds where the native stopped on a null pointer the original read through

def compare(runs, test, seeds):
    import live
    report = {}
    for a in test:
        diffs = []
        for s in range(seeds):
            o, n = runs.get((a, s, 'orig')), runs.get((a, s, 'native'))
            if o is None or n is None:
                diffs.append(f'seed {s}: missing run'); continue
            if o['status'] or n['status']:
                if o['status'] == 0 and n['status'] == 6:
                    nulls.setdefault(a, []).append(s)
                elif o['status'] != n['status']:
                    diffs.append(f'seed {s}: status orig {o["status"]} native {n["status"]}')
                continue  # both faulted or timed out the same way: nothing to compare
            if [c[0] for c in o['calls']] != [c[0] for c in n['calls']]:
                diffs.append(f'seed {s}: calls orig {[c[0] for c in o["calls"]]} native {[c[0] for c in n["calls"]]}')
                continue
            for i, ((t, oa), (_, na)) in enumerate(zip(o['calls'], n['calls'])):
                need = set(live.reads(t)) if t in live.gen else set(f'r{i}' for i in range(3, 11))
                # Every integer (or float) argument register read: an indirect call or
                # an import inside, where the scan can't tell arguments from leftovers.
                if all(f'r{i}' in need for i in range(3, 11)):
                    need -= {'r7', 'r8', 'r9', 'r10'}
                if all(f'f{i}' in need for i in range(1, 9)):
                    need -= {f'f{i}' for i in range(1, 9)}
                for k in sorted(need, key=lambda q: (q[0], int(q[1:]))):
                    if oa.get(k) != na.get(k):
                        diffs.append(f'seed {s}: call {i} sub_{t} {k} orig {oa.get(k)} native {na.get(k)}')
            if o['mem'] != n['mem']:
                keys = sorted(set(o['mem']) | set(n['mem']))
                bad = [k for k in keys if o['mem'].get(k) != n['mem'].get(k)]
                diffs.append(f'seed {s}: memory differs at {len(bad)} words, first {bad[0]}: orig {o["mem"].get(bad[0])} native {n["mem"].get(bad[0])}')
            if o['regs']['r3'][8:] != n['regs']['r3'][8:]:
                diffs.append(f'seed {s}: r3 orig {o["regs"]["r3"]} native {n["regs"]["r3"]}')
            if o['regs']['f1'] != n['regs']['f1']:
                diffs.append(f'seed {s}: f1 orig {o["regs"]["f1"]} native {n["regs"]["f1"]}')
            if o['regs']['r1'] != n['regs']['r1']:
                diffs.append(f'seed {s}: r1 orig {o["regs"]["r1"]} native {n["regs"]["r1"]}')
        report[a] = diffs
    return report

def clobbers(runs, test, seeds):
    """Volatile registers the native changes that the original leaves as they came."""
    init = None
    out = {}
    for a in test:
        bad = set()
        for s in range(seeds):
            o, n = runs.get((a, s, 'orig')), runs.get((a, s, 'native'))
            if not o or not n or o['status'] or n['status'] or o['calls']:
                continue  # with calls, the stubs scramble everything
            for k, v in o['regs'].items():
                if k in ('r1', 'r31', 'r3', 'f1'): continue
                if n['regs'][k] != v and runs.get((a, s, 'init'), {}).get(k, v) == v:
                    # original value equals native's? no: they differ; was the original's unchanged?
                    bad.add(k)
        out[a] = bad
    return out

def run_one(native, seeds=4, only=None):
    try:
        exe, test = build(native, only)
    except SystemExit as e:
        print(f'{os.path.basename(native)}: BUILD FAILED {str(e)[:600]}'); return
    r = subprocess.Popen([exe, str(seeds)], stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True)
    runs = parse(r.stdout)
    r.wait()
    rep = compare(runs, test, seeds)
    ok = [a for a, d in rep.items() if not d]
    incon = [a for a in test if all(runs.get((a, s, 'orig'), {'status': 1})['status'] for s in range(seeds))]
    print(f'{os.path.basename(native)}: {len(test)} functions, {len(ok) - len([a for a in incon if a in ok])} same, {len(incon)} could not run, {len(test) - len(ok)} differ' + (f' (exit {r.returncode})' if r.returncode else ''), flush=True)
    for a, d in rep.items():
        for x in d[:3]:
            print('  DIFF', a, x)
    for a in test:
        if a in nulls:
            print('  NULL', a, 'seeds', nulls[a], '(the original reads address 0 where ours stops)')
    for a in incon:
        print('  NORUN', a, [runs.get((a, s, 'orig'), {'status': -1})['status'] for s in range(seeds)])

if __name__ == '__main__':
    seeds = 4; only = None; files = []
    for x in sys.argv[1:]:
        if x.startswith('--seeds='): seeds = int(x[8:])
        elif x.startswith('--only='): only = set(x[7:].upper().split(','))
        else: files.append(x)
    for f in files:
        run_one(f, seeds, only)
