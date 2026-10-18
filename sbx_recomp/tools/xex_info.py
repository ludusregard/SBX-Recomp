#!/usr/bin/env python3
"""Print an Xbox 360 image's base, entry point, title ID and sections.

Usage: xex_info.py <default.xex.decrypt | image dump> [more images...]
Handy for comparing the layout of two builds before porting addresses.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import xex_image  # noqa: E402


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    for path in sys.argv[1:]:
        img = xex_image.load(path)
        print(f'{path}: base {img.base:08X} entry {img.entry:08X} '
              f'title {img.title_id:08X} size {len(img.data):X}')
        for s in img.sections:
            print(f'  {s.name:9} {s.start:08X}-{s.end:08X} size {s.size:08X} '
                  f'{"x" if s.executable else "-"} flags {s.flags:08X}')


if __name__ == '__main__':
    main()
