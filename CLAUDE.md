# Working on this repo

A static recompilation of Shutoko Battle X (Xbox 360) on ReXGlue
(`rexglue-sdk/`, submodule pinned to v0.10.0; SDK changes go in
`patches/rexglue-sdk-vulkan/`).

## Docs

Read the relevant doc before making changes; update it as you learn.

- `docs/status.md`: what works, fixes and experiments (with their flags),
  what's next. Check when a task depends on it; update when one changes.
- `docs/functions.md`: known functions, addresses, data, overrides. Check
  before naming, hooking or overriding a function; add every function or
  address worked out, and a row for every override (run
  `sbx_recomp/tools/check_function_statuses.py` after adding or removing
  one).
- `docs/native-code-rules.md`: rules for writing new code for the project,
  check before writing any game related code whether hook or native.
- `docs/native-code.md`: the game's subsystems rewritten as our C++, one
  `--native_<name>` flag each. Check before writing native code; add each
  subsystem's row and test notes.
- `docs/menus.md`: the front end's screens, cursors and menu messages.
- `docs/game_data.md`: archives, how they load, data formats. Check
  before work on file loading, assets or mods; add every format detail
  worked out.
- `docs/codegen.md`: how the code is generated and the Windows build's
  gotchas. Check for codegen or build problems; update when either changes.
- `docs/build_run.md`: building, running and testing.
- `docs/audio.md`: XACT and XAudio, the sound banks, and the code that
  loads them. Check before work on sound; add what's worked out.
- `docs/custom_music.md`: custom music (tracks from
  `ADDON_DATA/CUSTOM_MUSIC/` in the place of the game's), how to use it and
  how it works. Check before changing it or the XMA layer.
- `docs/rexglue-removal.md`: ITC-Recomp's plan for a port without
  ReXGlue, kept as a reference (its addresses are ITC's). Check before
  planning any native subsystem.
- `docs/modding-plan.md`: the plan for mod support (patch and addon
  loaders, adding cars). Check before work on mod loading or new content.
- `docs/cut_car_wheels.md`: suggested stock tire sizes and wheel
  positions for the cut cars. Check before changing their wheels or specs.
- The data tools (car specs workbook, cut car wheels, the font upscaler,
  the Blender add-on and its format code, `blender_plugin/sbx_formats/`)
  moved to their own repo, sbxr_tools
  (https://github.com/ludusregard/sbxr_tools), on 2026-11-08. Paths written
  `sbxr_tools/...` in these docs are in that repo. Check its README before
  writing a new data tool, and its `blender_plugin/README.md` and `PLAN.md`
  before work on the add-on or part writing.
- `docs/itc-parity.md`: what came over from ITC-Recomp, what's left, and
  how each of its folders reads at SBX's addresses.
- `docs/native-renderer-port.md`: the native Vulkan renderer, and how
  SBX's graphics library differs from ITC's.
- `docs/porting-from-itc.md`: how SBX relates to ITC-Recomp (Import Tuner
  Challenge, the US release), matching its functions to SBX, and the last
  reviewed ITC-Recomp commit. Check before porting anything from it.

## Session start

ALWAYS check ITC-Recomp for changes since "Last reviewed" in
`docs/porting-from-itc.md`:

1. Check the current local ITC-Recomp checkout. Do not fetch, pull, or otherwise update it.
2. If the checkout appears to be behind, inform the user and let them update it manually.
3. Inspect the commit history since `Last reviewed`, including changes to `CLAUDE.md` and `docs/*.md`.
4. Report new or renamed functions, fixes, features, and changed conventions, identifying their respective commits.
5. For each new function or fix, ask whether the user wants it ported. Use `itc_to_sbx_functions.tsv` and `port_addrs.py` to identify corresponding SBX addresses.
6. Update `Last reviewed` after the changes have been reviewed.

# Development rules

- **Native game code:** Before writing or modifying anything in `src/game/`, read `docs/native-code.md` and `docs/native-code-rules.md` following their requirements.
- **ITC-Recomp:** Never edit or commit anything in the ITC-Recomp checkout. Never port functionality without the user's explicit approval.
- **Repository operations:** Git operations are performed manually by the user. Do not fetch, pull, checkout, reset, stash, commit, push, create branches, or modify Git history. Read-only inspection is permitted when necessary.
- **File operations:** Never delete files or directories. Ask the user to handle deletions manually. Do not use commands or scripts that implicitly remove or overwrite existing files.
- **Uncommitted work:** Never revert, discard, or overwrite existing user changes.
- **External files:** Only search inside this repository. Ask the user for the location of anything outside it.
- **Machine-specific paths:** Never put drive letters, user folders, or machine-specific ITC-Recomp locations in code, documentation, or configuration. Use repository-relative paths.
- **Logging:** Find problems using logs, not guesses. Add appropriate logging, have the user run the game, inspect the resulting logs, and fix what they show.
- **Agents and automation:** Ask before starting background agents or scheduled tasks.
- **Git branches and releases:** Ask before creating branches, pushing to `main`, or pushing tags. All Git operations remain user-controlled.

# Project conventions

- Follow ITC-Recomp's layout, naming, and documentation conventions unless an SBX document specifies otherwise.
- Documentation and comments describe SBX on its own terms. Reference ITC-Recomp only when the origin of something matters.
- Use American English (color, gray).
- Keep code, comments, documentation, and commit messages plain, concise, and readable.
- Never use `-march=native`.