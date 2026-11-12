"""Generate an override file from fill tables: gen_file.py LISTFILE OUT FLAG
LISTFILE lines: ADDR Name [comment]"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import paths
import sys, subprocess, re
lst, out_path, flag = sys.argv[1], sys.argv[2], sys.argv[3]
header = open(sys.argv[4]).read() if len(sys.argv) > 4 else ''
extra = open(sys.argv[5]).read() if len(sys.argv) > 5 else ''
tables, funcs, failed = [], [], []
for line in open(lst):
    line = line.strip()
    if not line or line.startswith('#'): continue
    a, name = line.split()[:2]
    ident = re.sub(r'\W', '', name.replace('::~', '_Destroy').replace('::', '_'))
    r = subprocess.run([sys.executable, os.path.join(paths.HERE, 'gen_fill.py'), a, ident], capture_output=True, text=True)
    o = r.stdout
    if r.returncode or 'FAIL' in o or '// CALL' in o:
        failed.append((a, name, (r.stderr or o).strip().splitlines()[-1:])); continue
    body_i = o.index('// body:')
    tbl = o[:body_i].strip()
    body = [l[2:] for l in o[body_i:].splitlines()[1:] if l.startswith('  ')]
    r3 = [l for l in o.splitlines() if l.startswith('// r3')][0]
    uses_args = 'args' in '\n'.join(body)
    m = re.match(r'// r3: object \+ (\d+)', r3)
    ret = None
    if m:
        ret = 'object' if m.group(1) == '0' else f'object + {m.group(1)}'
    else:
        mc = re.match(r'// r3 = (0x[0-9A-F]+)$', r3)
        if mc:
            ret = f'{mc.group(1)}ull'
        elif 'ret' in r3:
            ret = 'keep'  # r3 as the last call leaves it
        else:
            failed.append((a, name, [r3])); continue
    tables.append(f'// {name} (sub_{a}).\n{tbl}' if tbl else '')
    f = [f'// {name}.', f'extern "C" REX_FUNC(sub_{a}) {{', '  REX_FUNC_PROLOGUE();',
         '  if (!On()) {', f'    __imp__sub_{a}(ctx, base);', '    return;', '  }',
         '  const uint32_t object = ctx.r3.u32;']
    if uses_args: f.append('  const guest_memory::Args args(ctx);')
    f += ['  ' + b for b in body]
    if ret != 'keep': f.append(f'  ctx.r3.u64 = {ret};')
    f.append('}')
    funcs.append('\n'.join(f))
src = header.replace('@FLAG@', flag) + '\nnamespace {\n\nusing guest_memory::Fill;\nusing guest_memory::kAbsArg;\nusing guest_memory::kAbsByte;\nusing guest_memory::kAbsHalf;\nusing guest_memory::kAbsImage;\nusing guest_memory::kAbsSelf;\nusing guest_memory::kAbsWord;\nusing guest_memory::kArg;\nusing guest_memory::kByte;\nusing guest_memory::kHalf;\nusing guest_memory::kImage;\nusing guest_memory::kSelf;\nusing guest_memory::kWord;\n\n' + '\n\n'.join(t for t in tables if t) + f'\n\nbool On() {{ return REXCVAR_GET({flag}); }}\n\n' + extra.split('@@FUNCS@@')[0] + '}  // namespace\n\n' + '\n\n'.join(funcs) + '\n' + (extra.split('@@FUNCS@@')[1] if '@@FUNCS@@' in extra else '')
open(out_path, 'w').write(src)
print('written', len(funcs), 'failed', len(failed))
for x in failed: print('  FAILED', x)
