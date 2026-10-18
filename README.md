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

## Requirements

- Windows 10 or 11 on a 64-bit x86 CPU with SSE4.2.
- Your own copy of the game, extracted from a backup.

## Install

Not packaged yet; build it from source (below). The game's files go in
`assets/` (`default.xex` and `DATA/`).

## Running it

| Command | What it does |
|---|---|
| `run.bat` | The game, on the emulated Xbox 360 GPU. |
| `run-debug.bat` | The same, with debug logging (written to `logs/`). |

Options can be added after either, e.g. `run.bat --fullscreen=true`.

## Known limitations

- Early: only the base port works so far. Most of what ITC-Recomp adds
  (its own renderer, audio, frame interpolation, resolutions) isn't here
  yet.

## Reporting problems

Play with `run-debug.bat`, then open an issue describing what you saw, and
attach the newest file from the `logs` folder.

## Building from source

You need git, clang 19 or newer (`clang`/`clang++` on `PATH`), CMake (3.25
or newer), Ninja, Python 3 and the Vulkan SDK.

```bat
setup_repo.bat
cd sbx_recomp
codegen.bat
build.bat
run.bat
```

`setup_repo.bat` fetches the SDK submodule and replaces its symlink stubs
with real files (git on Windows checks symlinks out as text files). This
builds `sbx_recomp/out/build/win-amd64-relwithdebinfo/sbx.exe`.

## Documentation (This is written by LLMs primarily for LLMs)

- [docs/functions.md](docs/functions.md): what's known about the game's
  code.
- [docs/codegen.md](docs/codegen.md): how the recompiled code is
  generated, and the Windows build.
- [docs/porting-from-itc.md](docs/porting-from-itc.md): how ITC-Recomp's
  findings are matched to this game's code.
