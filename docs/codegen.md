# Codegen

How the recompiled code in `sbx_recomp/generated/default` is made. All
commands from `sbx_recomp/`.

1. **Function hints.** `python tools\scan_ptrs.py ..\assets\default.xex.decrypt
   sbx_manifest.toml --toml sbx_functions.toml` scans `.rdata`/`.data` for
   pointers into `.text` and `.text` for `lis`/`addi` constants, keeping
   those that look like function entries. (The game has no RTTI, so the
   SDK's vtable scan finds nothing.)
2. **Codegen.** `codegen.bat` (configures, builds `rexglue`, runs
   `sbx_codegen`, configures again). Log: `out\codegen.log`.
3. **Unresolved branches.** `python tools\prune.py sbx_functions.toml
   generated\default` drops the hints implicated in `Unresolved branch from
   X to Y` comments in the generated code; rerun codegen.
4. **Tail calls.** Branch targets the analyzer still doesn't treat as
   functions go in `sbx_manifest.toml` under `[entrypoint.functions]` as
   `0x82XXXXXX = {}`; rerun codegen. Repeat 3-4 until clean.
5. **Indirect-call targets found at runtime** (a call through a pointer to a
   function that was never generated): add the address to
   `[entrypoint.functions]` too.

## The first codegen (2026-10-17)

1. `scan_ptrs.py`: 4,826 candidates (259 inside `.pdata` functions
   dropped, see below).
2. Codegen: 2 unresolved tail calls (`0x82582840`, `0x82582820`: thunks
   after a `bctr`, no `.pdata`), added to the manifest.
3. Codegen: 82 unresolved branches; `prune.py` dropped 11 hints; then 0.
4. One unresolved call into `0x822DC638`: a leaf function at `0x822DC628`
   (leaves have no `.pdata` entry) with a byte-offset switch whose jump
   base `0x822DC670` (a `lis`/`addi` constant) `scan_ptrs.py` took for a
   function. Fixed with `0x822DC628 = { size = 0x190 }` and the hint
   removed.
5. Clean: 4,814 hints, 15,964 functions.
6. First Windows link (2026-10-17): undefined `sub_825C0F1C`, `sub_825C0F2C`,
   `sub_825C0F4C`. They're XEX import thunks (two import records, then
   `mtctr r11; bctr`) at the end of `.text`; codegen logs them as
   "outranked by import" and emits calls to an unhinted thunk as
   `__imp__<Export>`, but a hinted one stays a call to a `sub_` it never
   defines. `scan_ptrs.py` now drops import thunks; 4,811 hints, 15,961
   functions, every declared function defined.
7. First run: `Call to invalid or unregistered function at guest address
   0x82383C30`. A leaf function (a qsort-style comparator, address loaded
   with `lis`/`addi` at `0x82384C9C`) right after a `.pdata` function whose
   last instruction is a `bl` that doesn't return; `scan_ptrs.py` only
   accepted a candidate after `blr`/`bctr`/`b`/padding. `scan_ptrs.py` now also
   accepts candidates on a `.pdata` function's start or end: +123 hints,
   then `prune.py` -11, 4,922 hints, 15,963 functions, clean.

## Runtime: "Call to invalid or unregistered function at guest address X"

An indirect call (function pointer, vtable, callback) to an address that
was never generated. Check the address: if it's a function entry (look in
Ghidra, or `.pdata`), add `0xX = {}` to `[entrypoint.functions]` in
`sbx_manifest.toml` (a hand-found entry belongs there, not in the
generated `sbx_functions.toml`), rerun codegen and build. A sweep after
step 7 found no pointer to code that isn't a function apart from switch
labels and import thunks, so the rest only show up at runtime.

**The `.pdata` filter.** Without it, `scan_ptrs.py` accepts any candidate right after a `blr`/`bctr`, so switch-case labels
whose jump-table base is a `lis`/`addi` constant become "functions" and
split the real one. Codegen doesn't always complain (the first run gave
1,160 unresolved branches; with the filter, 82). `.pdata` lists every
non-leaf function with its size, so candidates strictly inside one are
now dropped (`--keep-inside-pdata` keeps them). Bounds for a
function the analyzer gets wrong also come from `.pdata`
(`0xSTART = { size = 0x... }` in the manifest).

## generated/rexglue.cmake

`rexglue codegen` rewrites `generated/rexglue.cmake` to the SDK template
every run if it differs (`RefreshGeneratedGlue` in the SDK). The stock
template runs codegen as part of the build when its inputs change. That's
what's used for now. If the generated code is ever committed and edited
in place, a changed version (a manual `sbx_codegen` target the build never
runs) has to be restored from git after each codegen run; until then,
changes to generated functions go in overrides (`src/hooks/`).

The manifest points at the retail `default.xex` (encrypted, basic
compression), which the SDK loads itself. The Python
tools need the decrypted copy, `default.xex.decrypt`.

## Windows build gotchas

- **Symlink stubs in the SDK.** Git on Windows checks symlinks out as text
  files holding the target path (unless Developer Mode and
  `core.symlinks=true`). The SDK builds libmspack, its XEX LZX
  decompressor, from `thirdparty/libmspack/cabextract/mspack/`, where all
  15 files are links to `../../libmspack/mspack/`, so the SDK build fails
  early. `tools/fix_symlink_stubs.py` (run by `setup_repo.bat`) copies the
  real files over the stubs and marks them assume-unchanged. Rerun it after
  re-cloning or updating the submodule. The other symlinks (MoltenVK) are
  only used on macOS.
- **Logs.** `codegen.bat` and `build.bat` log everything to
  `out\codegen.log` / `out\build.log` and stop with the window open on a
  failure.
- Known to work (2026-10-17): clang 22.1.7 targeting MSVC 19.51, CMake
  4.3.3, Ninja. The SDK's own CI uses older clang, so new-compiler errors
  in third-party code are possible; check the log first.
