import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import paths
import sys, subprocess, re
lst, out_path, flag, header = sys.argv[1], sys.argv[2], sys.argv[3], open(sys.argv[4]).read()
inl = sys.argv[5] if len(sys.argv) > 5 else ''
extra = open(sys.argv[6]).read() if len(sys.argv) > 6 else '@@FUNCS@@'
funcs, failed = [], []
for line in open(lst):
    if not line.strip() or line.startswith('#'): continue
    a, name = line.split()[:2]
    r = subprocess.run([sys.executable, os.path.join(paths.HERE, 'gen_copy.py'), a, 'X'] + ([inl] if inl else []), capture_output=True, text=True)
    if r.returncode:
        failed.append((a, name, r.stderr.strip()[-120:])); continue
    lines = r.stdout.strip().splitlines()
    ret = lines[-1].replace('// r3: ', '')
    body = lines[:-1]
    if not re.match(r'r\d \+ \d+$', ret):
        failed.append((a, name, ret)); continue
    used = sorted(set(re.findall(r'\b(r[3-6]) \+', '\n'.join(body) + ret)))
    f = [f'// {name}.', f'extern "C" REX_FUNC(sub_{a}) {{', '  REX_FUNC_PROLOGUE();', '  if (!On()) {',
         f'    __imp__sub_{a}(ctx, base);', '    return;', '  }']
    f += [f'  const uint32_t {u} = ctx.{u}.u32;' for u in used]
    f += ['  ' + b for b in body]
    f.append(f'  ctx.r3.u64 = {ret};')
    f.append('}')
    funcs.append('\n'.join(f))
pre, post = extra.split('@@FUNCS@@')
src = header + f'\nnamespace {{\n\nusing guest_memory::CopyBytes;\nusing guest_memory::CopyHalves;\nusing guest_memory::CopyWords;\n\nbool On() {{ return REXCVAR_GET({flag}); }}\n\n' + pre + '}  // namespace\n\n' + '\n\n'.join(funcs) + '\n' + post
open(out_path, 'w').write(src)
print('written', len(funcs), 'failed', len(failed))
for x in failed: print('  FAILED', x)
