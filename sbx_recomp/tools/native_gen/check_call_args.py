"""Lists calls in the native code that don't set an argument register the
callee reads (r4 to r10, f1 to f8; live.py works out which).

  python check_call_args.py file [file...]     (names under src/hooks, no .cpp)

A hit isn't always a bug: a helper may set the register for its caller, and
the original may pass the register on from its own caller. Check each
against the original's code before the call."""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import live
import paths

for name in sys.argv[1:]:
    s = open(os.path.join(paths.HOOKS, name + '.cpp')).read()
    for m in re.finditer(
            r'^(?:extern "C" REX_FUNC\(sub_([0-9A-F]{8})\)|[A-Za-z][^\n]*\)\s*(?:const\s*)?)\s*\{\n(.*?)^\}',
            s, re.S | re.M):
        where = m.group(1) or m.group(0).split('\n')[0][:60]
        body = m.group(2)
        for c in re.finditer(r'\b(__imp__)?sub_([0-9A-F]{8})\(ctx, base\)', body):
            if c.group(1):
                continue
            callee, before = c.group(2), body[:c.start()]
            need = [k for k in sorted(live.reads(callee), key=lambda q: (q[0], int(q[1:])))
                    if k != 'r3']
            miss = [k for k in need
                    if not re.search(r'ctx\.' + k + r'\.(u64|f64|u32|s64)\s*=', before)]
            if miss:
                print(name, where, 'calls', callee, 'without', ' '.join(miss))
