# Working on this repo

A static recompilation of Shutoko Battle X (Xbox 360) on ReXGlue
(`rexglue-sdk/`, submodule pinned to v0.10.0; SDK changes go in
`patches/rexglue-sdk/`, none yet).

## Docs

Read the relevant doc before making changes; update it as you learn.

- `docs/status.md`: what works, fixes and experiments (with their flags),
  what's next. Check when a task depends on it; update when one changes.
- `docs/functions.md`: known functions, addresses, data, overrides. Check
  before naming, hooking or overriding a function; add every function or
  address worked out.
- `docs/game_data.md`: archives, how they load, data formats. Check
  before work on file loading, assets or mods; add every format detail
  worked out.
- `docs/codegen.md`: how the code is generated and the Windows build's
  gotchas. Check for codegen or build problems; update when either changes.
- `docs/build_run.md`: building, running and testing.
- `docs/audio.md`: XACT and XAudio, the sound banks, and the code that
  loads them. Check before work on sound; add what's worked out.
- `docs/rexglue-removal.md`: ITC-Recomp's plan for a port without
  ReXGlue, kept as a reference (its addresses are ITC's). Check before
  planning any native subsystem.
- `docs/porting-from-itc.md`: how SBX relates to ITC-Recomp (Import Tuner
  Challenge, the US release), matching its functions to SBX, and the last
  reviewed ITC-Recomp commit. Check before porting anything from it.

## Session start

ALWAYS check ITC-Recomp for changes since "Last reviewed" in
`docs/porting-from-itc.md`:

1. Fetch and fast-forward the checkout if it's behind.
2. Read `git log` since that commit, and the diffs of its `CLAUDE.md` and
   `docs/*.md`.
3. Report new or renamed functions, new fixes or features, and changed
   conventions, each with its commit.
4. For each new function or fix, ask whether to port it (SBX address via
   `itc_to_sbx_functions.tsv` / `port_addrs.py`).
5. Update "Last reviewed" to the new commit.

## Rules

- **NEVER edit or commit in the ITC-Recomp checkout.** Fetching and
  fast-forwarding it is the only exception.
- **NEVER port anything from ITC-Recomp without the user's approval.**
- **NEVER put machine-specific paths** (drives, user folders, the
  ITC-Recomp location) in code, docs or config. Paths are relative to
  the repo.
- **NEVER revert or `git checkout` uncommitted work** without committing
  it first. Commit often.
- **ALWAYS check `git diff` before committing** a doc another session may
  have edited.
- **ASK before** creating a branch, pushing to `main` or pushing tags.
  Otherwise stay on the current branch.
- **ASK before** starting background agents or scheduled runs.
- **Find problems with logs, not guesses:** add logging, have the user run
  the game, read the log, fix what it shows.
- Only search inside this repo; ask for the location of anything outside it.
- Docs and comments describe SBX on its own terms; name ITC-Recomp only
  where the origin matters.
- Follow ITC-Recomp's layout, naming and doc conventions unless a doc
  here says otherwise.
- American English (color, gray), plain and short, in code, comments,
  docs and commit messages.
- No `-march=native`.