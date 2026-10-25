"""The shader library: every shader pair in the game's parts, with
examples, so a material can use a pair its part doesn't have.

    python -m sbx_formats.library BUILD_OUT -o LIBRARY.json

It's made from the game files, so it lives with them or in the add-on's
own folder (the add-on can build it, see its preferences), never in the
repo. Per pair (keyed 'vs_hash:ps_hash', shaders.shader_hash):

- the two shaders and the pair's 0x20-byte entry, as in the first part
  that has it (what a part's shader section gets when the pair is
  added);
- examples: one material record for each different way the game uses
  the pair (shared shader, texture count, blend, flags), each with the
  shape it's from as a layout template (sidecar.template_entry) and the
  part, shape and draw it came from.

describe() names what a shader does from its constant table (the names
the compiler kept: DLightDir, cubetx0, tx0 ...). Inferred from the names
only.
"""

import argparse
import base64
import glob
import json
import os
import re
import sys

from . import shaders
from .sidecar import template_entry, template_from_entry

FORMAT = 'sbx-shader-library'
VERSION = 2
MAX_EXAMPLES = 8


def b64(b):
    return base64.b64encode(bytes(b)).decode('ascii')


def constant_names(shader):
    """The names in a shader's constant table."""
    hsize = int.from_bytes(shader[4:8], 'big') if len(shader) >= 8 else 0
    names = re.findall(rb'[A-Za-z_][A-Za-z0-9_]{2,}', shader[:hsize])
    skip = {b'Microsoft', b'Shader', b'Compiler', b'Xbox', b'vs_3_0', b'ps_3_0'}
    return [n.decode() for n in names if n not in skip]


def describe(pair):
    """'lit, specular, cube map, 1 texture' for a pair, from the pixel
    shader's constant names (inferred)."""
    names = set(constant_names(pair.ps)) | set(constant_names(pair.vs))
    out = []
    out.append('lit' if 'DLightDir' in names else 'unlit')
    if 'MatSpecularColor' in names:
        out.append('specular')
    if 'cubetx0' in names:
        out.append('cube map')
    if 'spheretx0' in names:
        out.append('sphere map')
    tex = sorted(n for n in names if re.fullmatch(r'tx\d', n))
    if tex:
        out.append('%d texture%s' % (len(tex), 's' if len(tex) > 1 else ''))
    elif 'btx0' not in names:
        out.append('untextured')
    if 'btx0' in names:
        out.append('btx0 texture')
    if 'shad' in names:
        out.append('shadow')
    return ', '.join(out)


def _example_key(rec):
    return (rec.get('shared_shader'), rec.get('texture_count'), rec.get('blend'), rec.get('flags'))


def build_library(folder, read_PART, progress=None):
    """Every .part in folder -> library dict. progress(done, total) is
    called now and then."""
    paths = sorted(glob.glob(os.path.join(folder, '*.part')))
    lib = {'format': FORMAT, 'version': VERSION,
           'hash': 'sha1 of the microcode, first 16 hex digits', 'shaders': {}, 'pairs': {}}
    for n, path in enumerate(paths):
        if progress and n % 200 == 0:
            progress(n, len(paths))
        if os.path.getsize(path) == 0:
            continue
        with open(path, 'rb') as f:
            part = read_PART(f.read())
        name = os.path.splitext(os.path.basename(path))[0]
        _, pairs, _ = shaders.split_section(part.shaders_raw)
        for p in pairs:
            for shader in (p.vs, p.ps):
                h = shaders.shader_hash(shader)
                e = lib['shaders'].setdefault(h, {'kind': shaders.shader_kind(shader), 'size': len(shader),
                                                  'parts': 0, 'first_part': name, 'raw': b64(shader)})
                e['parts'] += 1
            key = p.vs_hash + ':' + p.ps_hash
            e = lib['pairs'].setdefault(key, {'vs': p.vs_hash, 'ps': p.ps_hash, 'ids': p.ids,
                                             'what': describe(p), 'parts': 0, 'first_part': name,
                                             'entry': b64(p.entry), 'examples': []})
            e['parts'] += 1
        for i, s in enumerate(part.shapes):
            for k, rec in enumerate(s.draws):
                sh = rec.get('shader')
                if sh >= len(pairs):
                    continue
                e = lib['pairs'][pairs[sh].vs_hash + ':' + pairs[sh].ps_hash]
                ex = e['examples']
                if len(ex) >= MAX_EXAMPLES or any(tuple(x['key']) == _example_key(rec) for x in ex):
                    continue
                ex.append({'key': list(_example_key(rec)), 'part': name, 'shape': i, 'draw': k,
                           'record': b64(rec.raw), 'template': template_entry(s)})
    if progress:
        progress(len(paths), len(paths))
    return lib


def load_library(path):
    with open(path, encoding='utf-8') as f:
        lib = json.load(f)
    if lib.get('format') != FORMAT or lib.get('version') != VERSION:
        raise ValueError('%s isn\'t a version %d shader library (build it again)' % (path, VERSION))
    return lib


def library_pair(lib, key):
    """The shaders.ShaderPair for key from the library, or None."""
    e = lib['pairs'].get(key)
    if e is None:
        return None
    vs = base64.b64decode(lib['shaders'][e['vs']]['raw'])
    ps = base64.b64decode(lib['shaders'][e['ps']]['raw'])
    return shaders.ShaderPair(base64.b64decode(e['entry']), vs, ps)


def library_example(lib, key, index):
    """(record bytes, template Shape, 'part:shape:draw') of an example."""
    ex = lib['pairs'][key]['examples'][index]
    return (base64.b64decode(ex['record']), template_from_entry(ex['template']),
            '%s:%d:%d' % (ex['part'], ex['shape'], ex['draw']))


def write_library(lib, path):
    with open(path, 'w', encoding='utf-8') as f:
        json.dump(lib, f, indent=1, sort_keys=True)


def main(argv=None):
    from .part_read import read_PART
    ap = argparse.ArgumentParser(description='Build the shader library from the part files.')
    ap.add_argument('folder', help='BUILD_OUT (the .part files)')
    ap.add_argument('-o', '--output', required=True, help='library JSON (keep it out of the repo)')
    args = ap.parse_args(argv)
    lib = build_library(args.folder, read_PART)
    write_library(lib, args.output)
    kinds = {}
    for s in lib['shaders'].values():
        kinds[s['kind']] = kinds.get(s['kind'], 0) + 1
    print('%s: %d shaders (%s), %d pairs, %d examples' % (
        args.output, len(lib['shaders']), ', '.join('%d %s' % (v, k) for k, v in sorted(kinds.items())),
        len(lib['pairs']), sum(len(p['examples']) for p in lib['pairs'].values())))
    for key, p in sorted(lib['pairs'].items(), key=lambda kv: kv[1]['ids']):
        print('  %-15s %-45s %5d parts, %d examples' % (p['ids'], p['what'], p['parts'], len(p['examples'])))
    return 0


if __name__ == '__main__':
    sys.exit(main())
