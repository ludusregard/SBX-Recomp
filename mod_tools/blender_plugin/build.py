#!/usr/bin/env python3
"""Build the Blender add-on zip.

    python build.py

Copies sbx_blender/, then the format code (sbx_formats/) into it, into
build/sbx_blender/, and zips
that as build/sbx_parts-<version>.zip with blender_manifest.toml at the
zip's top level, which is what Blender's Install from Disk wants. The
format code has one home (sbx_formats/ next to this file); build/ is
not committed.
"""

import os
import shutil
import sys
import tomllib
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
SKIP = shutil.ignore_patterns('__pycache__', '*.pyc')


def main():
    src = os.path.join(HERE, 'sbx_blender')
    with open(os.path.join(src, 'blender_manifest.toml'), 'rb') as f:
        manifest = tomllib.load(f)
    out = os.path.join(HERE, 'build')
    stage = os.path.join(out, 'sbx_blender')
    if os.path.exists(stage):
        shutil.rmtree(stage)
    shutil.copytree(src, stage, ignore=SKIP)
    shutil.copytree(os.path.join(HERE, 'sbx_formats'), os.path.join(stage, 'sbx_formats'), ignore=SKIP)

    zip_path = os.path.join(out, '%s-%s.zip' % (manifest['id'], manifest['version']))
    with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED) as z:
        for folder, _, files in os.walk(stage):
            for name in sorted(files):
                full = os.path.join(folder, name)
                z.write(full, os.path.relpath(full, stage).replace(os.sep, '/'))
    print(zip_path)
    return 0


if __name__ == '__main__':
    sys.exit(main())
