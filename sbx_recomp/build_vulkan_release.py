
from __future__ import annotations

import re
import shutil
import sys
import zipfile
from pathlib import Path


# ------------------------------------------------------------
# Configuration
# ------------------------------------------------------------

ROOT = Path(__file__).resolve().parent

PRESET = (
    sys.argv[1]
    if len(sys.argv) > 1
    else "win-amd64-relwithdebinfo"
)

VERSION = input("Enter release version (e.g. 0.1.0): ").strip()

if not VERSION:
    VERSION = "0.0.0"

if not re.fullmatch(r"[A-Za-z0-9._-]+", VERSION):
    print("ERROR: Invalid version. Use only letters, numbers, dots, _ or -.")
    input("Press Enter to close...")
    raise SystemExit(1)


BUILD = ROOT / "out" / "build" / f"{PRESET}-vulkan"

# The source assets directory is a sibling of sbx_recomp.
SOURCE_ASSETS = ROOT.parent / "assets"

PATCH_DATA = SOURCE_ASSETS / "PATCH_DATA"
ADDON_DATA = SOURCE_ASSETS / "ADDON_DATA"

RELEASE_DIR = ROOT / "release"

VULKAN_DIR = (
    RELEASE_DIR / f"sbx-v{VERSION}-win-amd64-vulkan"
)
VULKAN_ZIP = (
    RELEASE_DIR / f"sbx-v{VERSION}-win-amd64-vulkan.zip"
)
ASSETS_ZIP = RELEASE_DIR / f"sbx-v{VERSION}-assets.zip"

# Existing test deployment.
TEST_DIR = ROOT / "release_test" / "sbx"
TEST_ASSETS = TEST_DIR / "assets"

# Relative to PATCH_DATA.
EXCLUDED_PATCH_FILE = Path("BUILD") / "TXR" / "BUILD" / ".git"

# These existing game files/directories must never be deleted.
PRESERVED_ITEMS = {
    "data",
    "$systemupdate",
    "default.xex",
}


# ------------------------------------------------------------
# General helpers
# ------------------------------------------------------------

def remove_path(path: Path) -> None:
    """Remove a file, symlink, or directory if it exists."""
    if path.is_symlink() or path.is_file():
        path.unlink()
    elif path.is_dir():
        shutil.rmtree(path)


def copy_tree_contents(
    source: Path,
    destination: Path,
    exclude_git: bool = False,
) -> None:
    """
    Copy a directory's contents into destination.

    When exclude_git=True, exclude the specific PATCH_DATA .git file.
    The source directory is never modified.
    """
    if not source.is_dir():
        raise FileNotFoundError(
            f"Source directory not found: {source}"
        )

    destination.mkdir(parents=True, exist_ok=True)

    for item in sorted(source.rglob("*")):
        if item.is_symlink():
            raise RuntimeError(
                f"Refusing to copy symlink: {item}"
            )

        relative = item.relative_to(source)

        if exclude_git and relative == EXCLUDED_PATCH_FILE:
            print(f"Excluded: {relative}")
            continue

        target = destination / relative

        if item.is_dir():
            target.mkdir(parents=True, exist_ok=True)

        elif item.is_file():
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(item, target)


def create_run_bat(destination: Path) -> None:
    content = r"""@echo off

rem Set the working directory to the folder containing this script.
cd /d "%~dp0"
set "ARGS=--game_data_root assets"
rem Launch SBX with the default arguments and any additional arguments supplied.
rem Example: "sbx.exe --fullscreen=false --width=2560 --height=1440 --new_music_roll=0.5"
sbx.exe
"""

    launcher = destination / "run.bat"
    launcher.write_text(content, encoding="utf-8")
    print(f"Created: {launcher}")


def validate_inputs() -> None:
    """Check required build, source asset, and test deployment paths."""
    required_paths = [
        BUILD / "sbx.exe",
        PATCH_DATA,
        ADDON_DATA,
        TEST_ASSETS / "default.xex",
        TEST_ASSETS / "DATA",
        TEST_ASSETS / "$SystemUpdate",
    ]

    missing = [
        path for path in required_paths
        if not path.exists()
    ]

    if missing:
        details = "\n".join(f"  - {path}" for path in missing)
        raise FileNotFoundError(
            f"Required paths are missing:\n{details}"
        )

    if not (BUILD / "sbx.exe").is_file():
        raise FileNotFoundError(
            f"Executable is not a file: {BUILD / 'sbx.exe'}"
        )

    if not PATCH_DATA.is_dir():
        raise NotADirectoryError(PATCH_DATA)

    if not ADDON_DATA.is_dir():
        raise NotADirectoryError(ADDON_DATA)

    if not (TEST_ASSETS / "default.xex").is_file():
        raise FileNotFoundError(
            TEST_ASSETS / "default.xex"
        )

    if not (TEST_ASSETS / "DATA").is_dir():
        raise NotADirectoryError(TEST_ASSETS / "DATA")

    if not (TEST_ASSETS / "$SystemUpdate").is_dir():
        raise NotADirectoryError(
            TEST_ASSETS / "$SystemUpdate"
        )


# ------------------------------------------------------------
# Vulkan release package
# ------------------------------------------------------------

def package_vulkan() -> None:
    """Create the standalone Vulkan release directory and ZIP."""
    print("\nPackaging Vulkan release...")

    remove_path(VULKAN_DIR)
    VULKAN_DIR.mkdir(parents=True, exist_ok=True)

    # Copy the executable.
    shutil.copy2(
        BUILD / "sbx.exe",
        VULKAN_DIR / "sbx.exe",
    )

    # Copy DLL dependencies located beside the built executable.
    dlls = sorted(BUILD.glob("*.dll"))

    for dll in dlls:
        if dll.is_symlink():
            raise RuntimeError(
                f"Refusing to copy symlink: {dll}"
            )

        shutil.copy2(dll, VULKAN_DIR / dll.name)

    print(f"Copied executable and {len(dlls)} DLL(s).")

    # Keep the existing generated launcher, if present.
    optional_launcher = BUILD / "run_sbx.bat"

    if optional_launcher.is_file():
        shutil.copy2(
            optional_launcher,
            VULKAN_DIR / optional_launcher.name,
        )

    # Generate our simple editable launcher before creating the ZIP.
    create_run_bat(VULKAN_DIR)

    # Replace the ZIP for this version.
    remove_path(VULKAN_ZIP)

    with zipfile.ZipFile(
        VULKAN_ZIP,
        mode="w",
        compression=zipfile.ZIP_DEFLATED,
        compresslevel=9,
    ) as archive:
        for item in sorted(VULKAN_DIR.rglob("*")):
            if item.is_symlink():
                raise RuntimeError(
                    f"Refusing to package symlink: {item}"
                )

            if item.is_file():
                archive.write(
                    item,
                    item.relative_to(VULKAN_DIR).as_posix(),
                )

    print(f"Created: {VULKAN_ZIP}")


# ------------------------------------------------------------
# Assets ZIP
# ------------------------------------------------------------

def add_assets_to_zip(
    archive: zipfile.ZipFile,
    source: Path,
    archive_folder: str,
    exclude_git: bool = False,
) -> None:
    """
    Add files under assets/<archive_folder>/ to the ZIP.

    The excluded .git file is matched relative to PATCH_DATA.
    """
    for item in sorted(source.rglob("*")):
        if item.is_symlink():
            raise RuntimeError(
                f"Refusing to package symlink: {item}"
            )

        if not item.is_file():
            continue

        relative = item.relative_to(source)

        if exclude_git and relative == EXCLUDED_PATCH_FILE:
            print(f"Excluded from assets ZIP: {relative}")
            continue

        archive_name = (
            Path("assets") / archive_folder / relative
        ).as_posix()

        archive.write(item, archive_name)


def package_assets() -> None:
    """Create the versioned assets ZIP."""
    print("\nPackaging assets...")

    remove_path(ASSETS_ZIP)

    with zipfile.ZipFile(
        ASSETS_ZIP,
        mode="w",
        compression=zipfile.ZIP_DEFLATED,
        compresslevel=9,
    ) as archive:
        add_assets_to_zip(
            archive,
            PATCH_DATA,
            "PATCH_DATA",
            exclude_git=True,
        )

        add_assets_to_zip(
            archive,
            ADDON_DATA,
            "ADDON_DATA",
        )

    print(f"Created: {ASSETS_ZIP}")


# ------------------------------------------------------------
# Test deployment cleanup
# ------------------------------------------------------------

def clean_test_directory() -> None:
    """
    Clean the existing test deployment while preserving:
      assets/DATA/
      assets/$SystemUpdate/
      assets/default.xex

    The contents of DATA and $SystemUpdate are not modified.
    """
    print("\nCleaning test deployment...")

    if not TEST_DIR.is_dir():
        raise NotADirectoryError(TEST_DIR)

    if not TEST_ASSETS.is_dir():
        raise NotADirectoryError(TEST_ASSETS)

    # Remove everything in the deployment root except assets/.
    for item in TEST_DIR.iterdir():
        if item.name.lower() == "assets":
            continue

        print(f"Removing: {item}")
        remove_path(item)

    # Remove everything inside assets except the three preserved items.
    for item in TEST_ASSETS.iterdir():
        if item.name.lower() in PRESERVED_ITEMS:
            print(f"Preserving: {item}")
            continue

        print(f"Removing: {item}")
        remove_path(item)

    # Verify that the preserved game data still exists.
    assert (TEST_ASSETS / "default.xex").is_file()
    assert (TEST_ASSETS / "DATA").is_dir()
    assert (TEST_ASSETS / "$SystemUpdate").is_dir()


# ------------------------------------------------------------
# Test deployment
# ------------------------------------------------------------

def deploy_test_build() -> None:
    """Deploy the release files and source assets to release_test/sbx."""
    print("\nDeploying test build...")

    TEST_DIR.mkdir(parents=True, exist_ok=True)
    TEST_ASSETS.mkdir(parents=True, exist_ok=True)

    # Copy the packaged executable, DLLs, and launchers.
    for item in sorted(VULKAN_DIR.iterdir()):
        if item.is_symlink():
            raise RuntimeError(
                f"Refusing to deploy symlink: {item}"
            )

        if item.is_file():
            shutil.copy2(item, TEST_DIR / item.name)

    # Copy PATCH_DATA, excluding the specified .git file.
    copy_tree_contents(
        PATCH_DATA,
        TEST_ASSETS / "PATCH_DATA",
        exclude_git=True,
    )

    # Copy ADDON_DATA normally.
    copy_tree_contents(
        ADDON_DATA,
        TEST_ASSETS / "ADDON_DATA",
    )

    # Ensure the preserved game files/directories still exist.
    assert (TEST_ASSETS / "default.xex").is_file()
    assert (TEST_ASSETS / "DATA").is_dir()
    assert (TEST_ASSETS / "$SystemUpdate").is_dir()

    print(f"Test deployment ready: {TEST_DIR}")


# ------------------------------------------------------------
# Main
# ------------------------------------------------------------

def main() -> int:
    print("SBX Release Packager")
    print(f"Version: {VERSION}")
    print(f"Preset:  {PRESET}")
    print(f"Build:   {BUILD}")
    print()

    validate_inputs()
    RELEASE_DIR.mkdir(parents=True, exist_ok=True)

    package_vulkan()
    package_assets()

    clean_test_directory()
    deploy_test_build()

    print("\nRelease packaging complete.")
    print(f"Vulkan folder: {VULKAN_DIR}")
    print(f"Vulkan ZIP:    {VULKAN_ZIP}")
    print(f"Assets ZIP:    {ASSETS_ZIP}")
    print(f"Test build:    {TEST_DIR}")
    print(f"Launcher:      {TEST_DIR / 'run.bat'}")

    return 0


if __name__ == "__main__":
    try:
        exit_code = main()
    except Exception as exc:
        print(f"\nERROR: {exc}")
        exit_code = 1

    if exit_code != 0:
        input("Press Enter to close...")

    raise SystemExit(exit_code)
