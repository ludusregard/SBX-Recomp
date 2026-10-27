# Shutoko Battle X Static Recompilation

A native Windows port of *Shutoko Battle X* (Xbox 360, 2006), made by
statically recompiling the game's PowerPC code to x86-64 with
[ReXGlue](https://github.com/rexglue/rexglue-sdk). It follows
[ITC-Recomp](https://github.com/Spooky098529/ITC-Recomp), the recompilation
of the game's US release, *Import Tuner Challenge*.

Built with assistance of Claude with manual review and testing.

## Features

- **Fix for the black screen after quest races** (on by default;
  `--post_race_check_fix=false` turns it off).
- **Patch loader**: game archive files are replaced by loose files in
  `assets/PATCH_DATA/` (on by default; `--patch_loader=false` turns it off).
- **Shutoko Battle X Restored** archive files, loaded through the patch
  loader from a submodule in `assets/PATCH_DATA/BUILD/TXR/BUILD/`.
- **Cut cars**: the ECLI, S30, L86 and T86 are offered from the start of a
  new game (`--unlock_cut_cars=false` turns it off), and come on their own
  stock wheels instead of AD00 rims: the ECLI its own, the L86, S30 and T86
  the LORRY's steel wheels (`--cut_car_steelies=false` gives them the
  BOSS00's wheels; `--cut_car_wheels=false` turns it all off).

## Requirements

- Windows 10 or 11 on a 64-bit x86 CPU with SSE4.2.
- Your own copy of the game, extracted from a backup.

## Install

Build it from source (below), or run `sbx_recomp\package_release.bat` to make
`release\sbx-vulkan.zip`: the program only, with no game files. The game's files go in
`assets/` (`default.xex` and `DATA/`); `assets/PATCH_DATA/` (replacements)
and `assets/ADDON_DATA/` (new content, not used yet) are part of this repo.

## Running it

| Command | What it does |
|---|---|
| `run_vulkan.bat` | The game, on the native Vulkan renderer. |
| `run_vulkan.bat --log_level=debug` | The same, with debug logging (written to `logs/`). |

Options can be added after either, e.g. `run_vulkan.bat --fullscreen=true`.

## Known limitations

- Early: only the base port works so far. Most of what ITC-Recomp adds
  (its own renderer, audio, frame interpolation, resolutions) isn't here
  yet.

## Reporting problems

Play with `run_vulkan.bat --log_level=debug`, then open an issue describing what you saw, and
attach the newest file from the `logs` folder.

## Building from source

You need git, clang 19 or newer (`clang`/`clang++` on `PATH`), CMake (3.25
or newer), Ninja, Python 3 and the Vulkan SDK.

```bat
python sbx_recomp\tools\fix_symlink_stubs.py
cd sbx_recomp
codegen.bat
build_vulkan.bat
run_vulkan.bat
```

Clone with its submodules. `fix_symlink_stubs.py` replaces the SDK's
symlink stubs with real files (git on Windows checks symlinks out as text
files). This builds `sbx_recomp/out/build/win-amd64-relwithdebinfo-vulkan/sbx.exe`.

## Documentation (This is written by LLMs primarily for LLMs)

- [docs/functions.md](docs/functions.md): what's known about the game's
  code.
- [docs/status.md](docs/status.md): what works now, fixes,
  experiments, and what's next.
- [docs/game_data.md](docs/game_data.md): what's known about the game's
  data files.
- [docs/codegen.md](docs/codegen.md): how the recompiled code is
  generated, and the Windows build.
- [docs/porting-from-itc.md](docs/porting-from-itc.md): how ITC-Recomp's
  findings are matched to this game's code.
- [docs/build_run.md](docs/build_run.md): info on building, running and testing.
- [docs/audio.md](docs/audio.md): the sound banks and the code that loads
  them.
- [docs/modding-plan.md](docs/modding-plan.md): the plan for mod support
  (the patch and addon loaders, adding cars).
- [docs/cut_car_wheels.md](docs/cut_car_wheels.md): stock wheel and tire
  baselines for the cut cars.
- [mod_tools/README.md](mod_tools/README.md): tools for editing the game's
  data files.
- [mod_tools/blender_plugin/README.md](mod_tools/blender_plugin/README.md):
  the Blender add-on for the game's car models.
