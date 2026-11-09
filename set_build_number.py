#!/usr/bin/env python3
"""Set the SBX build number, shown in the window title (for example v0.6.0 Vulkan).

Usage:
    python set_build_number.py 0.6.0
    python set_build_number.py            (asks for the number)

What it does:
  1. Changes the title line in patches/rexglue-sdk-vulkan/0002-vulkan-title.patch.
  2. Resets include/rex/version.h.in in the rexglue-sdk submodule, so the
     file is back to clean source.
  3. Re-applies the patch to the submodule.

Then run build_vulkan.bat to rebuild.
"""
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
SDK = ROOT / "rexglue-sdk"
PATCH = ROOT / "patches" / "rexglue-sdk-vulkan" / "0002-vulkan-title.patch"
SDK_FILE = "include/rex/version.h.in"
TITLE_RE = re.compile(r'^(\+#define REXGLUE_BUILD_TITLE[ \t]+")v[^"]*(")', re.MULTILINE)


def git(*args):
    result = subprocess.run(
        ["git", "-C", str(SDK), *args], capture_output=True, text=True
    )
    if result.returncode != 0:
        sys.exit(f"git {' '.join(args)} failed:\n{result.stderr or result.stdout}")


def main():
    number = sys.argv[1] if len(sys.argv) > 1 else input("Build number (e.g. 0.6.0): ")
    number = number.strip().lstrip("vV")
    if not re.fullmatch(r"\d+(\.\d+)*", number):
        sys.exit("The build number must look like 0.6.0")

    with open(PATCH, encoding="utf-8", newline="") as f:
        # The SDK source is checked out with LF line endings, so the patch
        # must use LF too, or git apply cannot match its context lines.
        text = f.read().replace("\r\n", "\n")
    new_text, count = TITLE_RE.subn(
        lambda m: f"{m.group(1)}v{number} Vulkan{m.group(2)}", text
    )
    if count != 1:
        sys.exit(f"Expected one title line in the patch, found {count}.")
    with open(PATCH, "w", encoding="utf-8", newline="") as f:
        f.write(new_text)

    git("checkout", "--", SDK_FILE)
    git("apply", "--check", str(PATCH))
    git("apply", str(PATCH))

    print(f"Build number set to {number} (title: v{number} Vulkan).")
    print("The patch is re-applied to rexglue-sdk. Run build_vulkan.bat to rebuild.")


if __name__ == "__main__":
    main()
