#!/usr/bin/env python3
"""Make or update the metadata.xml files in CUSTOM_MUSIC.

Put this script in the CUSTOM_MUSIC folder and run it:

    python generate_metadata.py

It walks CUSTOM_MUSIC and its subfolders. For each folder with music files:

- If the folder has a metadata.xml, the file is kept as it is except:
  any music file it doesn't list yet is appended as <Track File="..."
  Weight="1"/>, and any <Track> whose file is no longer in the folder is
  commented out (so its weight and loop points are kept, just switched off).
- If not, a new metadata.xml is written: the game's own songs for that
  folder (category folders only) and every music file, all at weight 1.

Then edit the files to change weights, block songs (weight 0) or add loop
points. See CUSTOM_MUSIC_README.md.

Options:
    --root PATH   folder to scan (default: the folder this script is in)
    --force       rewrite every metadata.xml from scratch (your edits to
                  them are lost)
    --dry-run     show what would change without writing anything
"""

import argparse
import os
import re
import sys
import xml.etree.ElementTree as ET
from xml.sax.saxutils import quoteattr, unescape

MUSIC_EXTENSIONS = {".ogg", ".oga", ".opus", ".flac", ".mp3", ".wav"}
METADATA_NAME = "metadata.xml"

# The game's songs for each category folder, with where each plays.
GAME_SONGS = {
    "ROAMING": [
        ("bgm_tmp_06", "the usual highway song"),
        ("bgm_tmp_07", "picked at random in some game modes"),
        ("bgm_tmp_01", "picked at random in some game modes"),
        ("bgm_tmp_15", "picked at random in some game modes"),
        ("bgm_tmp_05", "picked at random in some game modes"),
        ("bgm_tmp_10", "picked at random in some game modes"),
    ],
    "BATTLE": [
        ("bgm_tmp_07", "regular battles (one of 07, 01, 15)"),
        ("bgm_tmp_01", "regular battles (one of 07, 01, 15)"),
        ("bgm_tmp_15", "regular battles (one of 07, 01, 15)"),
        ("bgm_tmp_10", "rival class 1"),
        ("bgm_tmp_05", "rival class 2"),
    ],
    "BOSS": [
        ("bgm_tmp_03", "rival class 3"),
        ("bgm_tmp_09", "some class 3 and 4 rivals"),
        ("bgm_tmp_17", "some class 3 and 4 rivals"),
        ("bgm_tmp_11", "one class 4 rival"),
        ("bgm_tmp_16", "rival class 5"),
    ],
    "RACE_WIN": [("bgm_tmp_14", "after a race you won")],
    "RACE_LOSE": [("bgm_tmp_08", "after a race you lost")],
    "MENU_COURSE": [("BGM_SYSTEM", "the course select song")],
    "MENU_MAIN": [("BGM_SYSTEM", "the main menu song")],
    "MENU_GENERAL": [("BGM_SYSTEM", "the other menus' song")],
    "MENU_STORY": [("BGM_SYSTEM", "the story dialog song")],
}

def music_files(names):
    found = [n for n in names if os.path.splitext(n)[1].lower() in MUSIC_EXTENSIONS]
    return sorted(found, key=lambda n: (n.lower(), n))


def track_line(name):
    return f'  <Track File={quoteattr(name)} Weight="1"/>'


def build_xml(category, tracks):
    """A new metadata.xml. `category` is None for folders that aren't one."""
    songs = GAME_SONGS.get(category.upper()) if category else None
    lines = ['<?xml version="1.0" encoding="UTF-8"?>']
    if category:
        lines.append(f"<!-- {category.upper()}: see ../CUSTOM_MUSIC_README.md -->")
    attrs = f" Category={quoteattr(category)}" if category else ""
    lines.append(f"<Music{attrs}>")
    for name, where in songs or []:
        lines.append(f'  <Game Name="{name}" Weight="1"/>  <!-- {where} -->')
    for name in tracks:
        lines.append(track_line(name))
    lines.append("</Music>")
    return "\r\n".join(lines) + "\r\n"


# A <Track> start tag (its quoted attribute values may hold '>'), and comments.
TRACK_TAG = re.compile(r"<Track\b(?:[^>\"']|\"[^\"]*\"|'[^']*')*>", re.IGNORECASE)
FILE_ATTR = re.compile(r"""\bFile\s*=\s*(?:"([^"]*)"|'([^']*)')""", re.IGNORECASE)
COMMENT = re.compile(r"<!--.*?-->", re.DOTALL)


def comment_out_missing(text, present):
    """`text` with each self-closing <Track> whose File isn't in `present`
    (lower-case names) commented out. Everything else is left as it is.
    Returns the text and the names commented out."""
    comments = [m.span() for m in COMMENT.finditer(text)]
    edits, gone = [], []
    for m in TRACK_TAG.finditer(text):
        if any(s <= m.start() < e for s, e in comments):
            continue  # already commented out
        tag = m.group(0)
        if not tag.endswith("/>"):
            continue  # an open and close pair: left for the author
        f = FILE_ATTR.search(tag)
        if not f:
            continue
        name = unescape(f.group(1) if f.group(1) is not None else f.group(2),
                        {"&quot;": '"', "&apos;": "'"})
        if name.lower() in present:
            continue
        edits.append((m.start(), m.end(), name))
        gone.append(name)
    for start, end, name in reversed(edits):
        tag = text[start:end].replace("--", "- -")
        text = text[:start] + f"<!-- {tag} (no such file in this folder) -->" + text[end:]
    return text, gone


def listed_files(text):
    """The File names of the <Track> entries in an existing metadata.xml."""
    root = ET.fromstring(text)
    names = set()
    for element in root:
        if element.tag.lower() == "track":
            for key, value in element.attrib.items():
                if key.lower() == "file":
                    names.add(value.lower())
    return names


def add_tracks(text, new):
    """`text` with a <Track> line for each of `new` before </Music>."""
    newline = "\r\n" if "\r\n" in text else "\n"
    end = None
    for m in re.finditer(r"</\s*Music\s*>", text, re.IGNORECASE):
        end = m
    if end is None:
        raise ValueError("no </Music>")
    start = text.rfind("\n", 0, end.start()) + 1  # the start of that line
    if text[start:end.start()].strip():  # </Music> isn't on its own line
        start = end.start()
        added = newline + newline.join(track_line(n) for n in new) + newline
    else:
        added = newline.join(track_line(n) for n in new) + newline
    return text[:start] + added + text[start:]


def main():
    parser = argparse.ArgumentParser(description="Make or update metadata.xml files.")
    parser.add_argument("--root", default=os.path.dirname(os.path.abspath(__file__)),
                        help="folder to scan (default: this script's folder)")
    parser.add_argument("--force", action="store_true",
                        help="rewrite every metadata.xml from scratch")
    parser.add_argument("--dry-run", action="store_true",
                        help="show what would change, change nothing")
    args = parser.parse_args()

    root = os.path.abspath(args.root)
    if not os.path.isdir(root):
        print(f"Not a folder: {root}", file=sys.stderr)
        return 1

    changed = 0
    for folder, subfolders, files in os.walk(root):
        subfolders[:] = sorted((s for s in subfolders if not s.startswith(".")), key=str.lower)
        if folder == root:
            continue
        tracks = music_files(files)
        rel = os.path.relpath(folder, root)
        target = os.path.join(folder, METADATA_NAME)
        exists = os.path.exists(target)
        # Category only for the folders directly under the root.
        is_category = os.path.normcase(os.path.dirname(folder)) == os.path.normcase(root)
        category = os.path.basename(folder) if is_category else None

        if exists and not args.force:
            with open(target, encoding="utf-8-sig", newline="") as f:
                text = f.read()
            try:
                present = {t.lower() for t in tracks}
                text, gone = comment_out_missing(text, present)
                new = [t for t in tracks if t.lower() not in listed_files(text)]
                if not new and not gone:
                    continue
                text = add_tracks(text, new) if new else text
            except (ET.ParseError, ValueError) as e:
                print(f"skipped  {rel}: its {METADATA_NAME} can't be read ({e})")
                continue
            parts = []
            if new:
                parts.append(f"added {len(new)} track(s)")
            if gone:
                parts.append(f"commented out {len(gone)} missing track(s): " + ", ".join(gone))
            verb, done, detail = "update", "updated", "; ".join(parts)
        else:
            if not tracks and not (category and category.upper() in GAME_SONGS):
                continue
            text = build_xml(category, tracks)
            verb, done, detail = "write", "wrote", f"new file, {len(tracks)} track(s)"

        if args.dry_run:
            print(f"would {verb:5}  {rel}: {detail}")
        else:
            with open(target, "w", encoding="utf-8", newline="") as f:
                f.write(text)
            print(f"{done:6}  {rel}: {detail}")
        changed += 1

    print(f"Done: {changed} file(s) {'would change' if args.dry_run else 'changed'}.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
