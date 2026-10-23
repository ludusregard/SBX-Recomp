#!/usr/bin/env python3
"""Round-trip test for PART files.

    python tests/roundtrip.py BUILD_OUT [--mode rebuild] [--limit N] [--report FILE]

Reads every .part file in the folder (or the files given) and checks it
comes back byte for byte. Modes:

- rebuild: model -> its pieces placed at their source offsets. Proves the
  model holds every byte of the file (anything left out must be zero
  padding). Milestone 1.
- write: model -> part_write.write_PART: the file laid out again from
  the model. Milestone 3.
- relocate: written with 0x40 extra bytes after the shape records
  (moving every section after them), read back, written again: must give
  the original. Catches offsets the writer doesn't recompute.
- sidecar: model -> sidecar -> its two JSON texts -> sidecar -> model (with the
  first model's geometry, standing in for Blender, and again with the
  geometry stored in the sidecar) -> write_PART. Milestone 4.

Mismatches are named with sbx_formats.diff. Exit code 1 if any file fails.
"""

import argparse
import glob
import os
import sys

import numpy as np

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))

from sbx_formats.diff import compare_parts  # noqa: E402
from sbx_formats.part_read import read_PART  # noqa: E402


def rebuild(part):
    """Place every piece of the model at its source offset."""
    out = np.zeros(part.source_size, np.uint8)
    placed = np.zeros(part.source_size, bool)
    for off, raw, label in part.regions():
        end = off + len(raw)
        if end > len(out):
            raise ValueError('%s runs past the end of the file' % label)
        piece = np.frombuffer(raw, np.uint8)
        # Pieces may overlap (one part has its small sections on top of
        # each other); overlapping pieces must agree.
        clash = placed[off:end] & (out[off:end] != piece)
        if clash.any():
            raise ValueError('%s disagrees with an overlapping piece at 0x%X'
                             % (label, off + int(np.flatnonzero(clash)[0])))
        out[off:end] = piece
        placed[off:end] = True
    return out.tobytes()


def write(part):
    from sbx_formats.part_write import write_PART
    return write_PART(part)


def relocate(part):
    from sbx_formats.part_write import write_PART
    moved = write_PART(part, _test_gap=0x40)
    return write_PART(read_PART(moved))


def sidecar(part):
    import json
    from sbx_formats import sidecar as sc
    from sbx_formats.part_write import write_PART
    readable, raw = sc.split(sc.to_sidecar(part))
    text_r, text_raw = sc.readable_text(readable), sc.raw_text(raw)
    whole = sc.join(json.loads(text_r), json.loads(text_raw))
    back = sc.from_sidecar(whole, sc.geometry_of(part))
    if sc.edited_shapes(whole, sc.geometry_of(back)):
        raise ValueError('unedited geometry reported as edited')
    out = write_PART(back)
    # The same from the geometry stored in the sidecar alone.
    if write_PART(sc.from_sidecar(whole)) != out:
        raise ValueError('stored geometry writes a different file')
    return out


MODES = {'rebuild': rebuild, 'write': write, 'relocate': relocate, 'sidecar': sidecar}


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('inputs', nargs='+', help='a folder of .part files, or .part files')
    ap.add_argument('--mode', default='rebuild', choices=sorted(MODES))
    ap.add_argument('--limit', type=int, help='stop after N files')
    ap.add_argument('--report', help='write the per-file results here')
    args = ap.parse_args()
    files = []
    for p in args.inputs:
        files += sorted(glob.glob(os.path.join(p, '*.part'))) if os.path.isdir(p) else [p]
    files = [f for f in files if os.path.getsize(f) > 0][:args.limit]

    fn = MODES[args.mode]
    failed = []
    lines = []
    for f in files:
        with open(f, 'rb') as fh:
            d = fh.read()
        name = os.path.basename(f)
        try:
            out = fn(read_PART(d))
        except Exception as e:  # report and keep going
            failed.append(name)
            lines.append('%s\tERROR %s' % (name, e))
            continue
        if out == d:
            lines.append('%s\tok' % name)
            continue
        failed.append(name)
        diffs = compare_parts(d, out)
        lines.append('%s\t%s' % (name, diffs[0] if diffs else 'differs'))

    if args.report:
        with open(args.report, 'w') as fh:
            fh.write('\n'.join(lines) + '\n')
    for line in lines:
        if not line.endswith('\tok'):
            print(line)
    print('%s: %d files, %d byte-exact, %d failed'
          % (args.mode, len(files), len(files) - len(failed), len(failed)))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
