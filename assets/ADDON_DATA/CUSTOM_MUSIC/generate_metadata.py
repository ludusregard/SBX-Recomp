#!/usr/bin/env python3
"""Generate a basic metadata.xml for every folder of music files.

Put this script in the CUSTOM_MUSIC folder and run it:

    python generate_metadata.py

It walks CUSTOM_MUSIC and all nested subfolders, and in each folder that holds
music files writes a metadata.xml listing them, every track at weight 1 with
no loop marks (the whole track loops). Edit the result to set weights, loop
points (LoopIn/LoopOut, in seconds) or a ReplaceChance.

Options:
    --root PATH   folder to scan (default: the folder this script is in)
    --force       overwrite metadata.xml files that already exist
    --dry-run     show what would be written without writing anything

A folder that already has a metadata.xml is skipped unless --force is given.
Category is set only for folders directly under the root, where it names the
music category (ROAMING, BATTLE, ...); deeper folders leave it out.
"""

import argparse
import os
import sys
from xml.sax.saxutils import quoteattr

MUSIC_EXTENSIONS = {".ogg", ".oga", ".opus", ".flac", ".mp3", ".wav"}
METADATA_NAME = "metadata.xml"


def music_files(folder_files):
    names = [n for n in folder_files if os.path.splitext(n)[1].lower() in MUSIC_EXTENSIONS]
    return sorted(names, key=lambda n: (n.lower(), n))


def build_xml(category, files):
    attrs = f" Category={quoteattr(category)}" if category else ""
    lines = [
        '<?xml version="1.0" encoding="UTF-8"?>',
        "<!--",
        "  Optional: ReplaceChance (0 to 1) on <Music> for this folder's chance.",
        "  Per track: Weight (0 or more, 1 by default; 0 never picks it),",
        "  LoopIn and LoopOut (seconds, both together) for a loop region.",
        "-->",
        f"<Music{attrs}>",
    ]
    for name in files:
        lines.append(f'  <Track File={quoteattr(name)} Weight="1"/>')
    lines.append("</Music>")
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description="Generate basic metadata.xml files.")
    parser.add_argument("--root", default=os.path.dirname(os.path.abspath(__file__)),
                        help="folder to scan (default: this script's folder)")
    parser.add_argument("--force", action="store_true",
                        help="overwrite existing metadata.xml files")
    parser.add_argument("--dry-run", action="store_true",
                        help="show what would be written, change nothing")
    args = parser.parse_args()

    root = os.path.abspath(args.root)
    if not os.path.isdir(root):
        print(f"Not a folder: {root}", file=sys.stderr)
        return 1

    written = skipped = 0
    for folder, subfolders, files in os.walk(root):
        subfolders.sort(key=str.lower)
        tracks = music_files(files)
        if not tracks:
            continue
        rel = os.path.relpath(folder, root)
        label = "." if rel == "." else rel
        target = os.path.join(folder, METADATA_NAME)
        if os.path.exists(target) and not args.force:
            print(f"skipped  {label}: {METADATA_NAME} already exists")
            skipped += 1
            continue
        # Category only for the folders directly under the root.
        parent = os.path.dirname(folder)
        category = os.path.basename(folder) if os.path.normcase(parent) == os.path.normcase(root) else None
        if args.dry_run:
            print(f"would write  {label}: {len(tracks)} track(s)")
        else:
            with open(target, "w", encoding="utf-8", newline="\n") as f:
                f.write(build_xml(category, tracks))
            print(f"wrote    {label}: {len(tracks)} track(s)")
        written += 1

    verb = "would write" if args.dry_run else "wrote"
    print(f"Done: {verb} {written}, skipped {skipped}.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
