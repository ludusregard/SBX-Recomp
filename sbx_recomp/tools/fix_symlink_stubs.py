#!/usr/bin/env python3
"""Replace git symlink stubs with real files (Windows checkouts).

Without symlink support (core.symlinks=false, the Windows default without
Developer Mode), git checks a symlink out as a small text file holding the
link's target path. rexglue-sdk builds libmspack (the SDK's XEX LZX
decompressor) from thirdparty/libmspack/cabextract/mspack/, where every
file is such a link to ../../libmspack/mspack/<name>, so the SDK build
fails. This walks the repo and all submodules, finds index entries with
mode 120000, and replaces each stub with a copy of what it points to.
Copied paths are marked assume-unchanged so `git status` stays clean.

Usage (from the repo root; setup_repo.bat runs it):
  python sbx_recomp/tools/fix_symlink_stubs.py [repo root]
Safe to rerun: real files and real symlinks are left alone.
"""
import os
import shutil
import subprocess
import sys


def git(cwd, *args):
    return subprocess.run(['git', '-C', cwd, *args], check=True, capture_output=True,
                          text=True).stdout


def repos(root):
    out = [root]
    listing = git(root, 'submodule', 'foreach', '--recursive', '--quiet',
                  'echo "$displaypath"')
    for line in listing.splitlines():
        if line.strip():
            out.append(os.path.normpath(os.path.join(root, line.strip())))
    return out


def fix_repo(repo):
    fixed, failed, hidden = [], [], []
    for line in git(repo, 'ls-files', '-s').splitlines():
        meta, path = line.split('\t', 1)
        mode, sha, _ = meta.split()
        if mode != '120000':
            continue
        stub = os.path.join(repo, path)
        if os.path.islink(stub) or not os.path.isfile(stub):
            continue  # a real symlink, or a stub already replaced by a directory
        target = git(repo, 'cat-file', '-p', sha).strip()
        src = os.path.normpath(os.path.join(os.path.dirname(stub), target))
        with open(stub, 'rb') as f:
            raw = f.read()
        if raw.strip().decode('utf-8', 'replace') != target:
            # Already replaced: hide it from git status if it matches.
            if os.path.isfile(src) and open(src, 'rb').read() == raw:
                hidden.append(path)
            continue
        if os.path.isfile(src):
            shutil.copyfile(src, stub)
        elif os.path.isdir(src):
            os.remove(stub)
            shutil.copytree(src, stub)
        else:
            failed.append((path, target))
            continue
        fixed.append(path)
    if fixed or hidden:
        git(repo, 'update-index', '--assume-unchanged', '--', *fixed, *hidden)
    return fixed, failed


def main():
    root = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else '.')
    total = 0
    for _ in range(3):  # a link may point at another link's stub
        changed = 0
        for repo in repos(root):
            fixed, failed = fix_repo(repo)
            for p in fixed:
                print(f'fixed  {os.path.relpath(os.path.join(repo, p), root)}')
            for p, t in failed:
                print(f'MISSING {os.path.relpath(os.path.join(repo, p), root)} -> {t}')
            changed += len(fixed)
        total += changed
        if not changed:
            break
    print(f'{total} symlink stubs replaced')


if __name__ == '__main__':
    main()
