# Tools

Ported from ITC-Recomp's `itc_recomp/tools/` unless noted. Run with
`python <tool>` from `sbx_recomp/`. Tools for the game's data files are in
`mod_tools/`.

| Tool | Use now? | What |
|---|---|---|
| `xex_image.py` | yes (new) | Loads an XEX (unencrypted; uncompressed or basic) or a flat dump as its memory image, with sections. Used by the others. |
| `xex_info.py` | yes (new) | Prints base, entry, title ID and sections. |
| `scan_ptrs.py` | yes | Function entry hints (see `docs/codegen.md`). Section ranges come from the image, `--toml` writes `sbx_functions.toml`, and candidates inside `.pdata` functions are dropped (`--keep-inside-pdata` for ITC's behavior). |
| `fix_symlink_stubs.py` | yes (new, Windows) | Replaces git symlink stubs (text files holding a path) in the repo and submodules with real files; the SDK's libmspack sources are all symlinks. Run it after a fresh clone or submodule update. |
| `scan_ptrs_itc_original.py` | reference | ITC's script unchanged, with ITC's hard-coded section ranges. |
| `match_funcs.py` | yes (new) | Matches functions between two generated trees (ITC -> SBX) by masked instructions, call graph, order and locality; writes a TSV. See `docs/functions-sbx.md`. |
| `match_pins.tsv` | yes (new) | Hand-checked matches `match_funcs.py` applies first. |
| `port_addrs.py` | yes (new) | Ports function, code and data addresses (e.g. everything in ITC's `docs/functions.md`) using the matches. |
| `check_function_statuses.py` | yes | Checks the Status column of `docs/functions.md` against the overrides in `src/`; reports only. Run after changing either. |
| `prune.py` | yes | Drops hints implicated in "Unresolved branch" sites. |
| `pfm_to_png.py`, `rainbow_draws.py` | later | Read ITC's native renderer dumps; need that renderer ported. |
| `read_capture.py` | later | Reads `--capture_dir` state captures (ITC's `src/debug/state_capture.cpp`). |
| `profile_render_thread.sh` | later, Linux | `perf` profile of the render thread. |
| `fetch_vc_redist.py`, `xwin_case_links.py` | Linux cross-builds | For building Windows zips on Linux with xwin; not needed on Windows. |
