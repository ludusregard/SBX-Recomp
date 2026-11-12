# Native game code

Native game code is SBX's human-readable C++ implementation of the original game's functionality.

It is intended to be a codebase that can be read, debugged, and extended for modifications, not a direct transcription of the original PowerPC instructions.

These requirements apply to all code in `src/game/`.

## Code style

- Write plain, human-readable C++ from the first draft.
- Use named structs, named fields, named functions, ordinary loops, and normal control flow.
- Explain what the game is doing rather than documenting individual instructions.
- Never reproduce PowerPC-style code, register manipulation, raw memory offsets, or instruction-by-instruction translations.
- Use `unknown_<offset>` members for fields whose purpose has not yet been identified. Never access those fields through raw offsets.
- Every struct must include appropriate `GUEST_SIZE` and `GUEST_OFFSET` checks for known layouts.

## Guest memory and functions

- Register access is restricted to thin override adapters: flag checks, argument extraction into typed pointers, and return values in r3 or f1.
- Recompiled functions must be called through `Guest::Call` and related helpers, with comments identifying the called function.
- Only read-only image data below `0x82074000` may be represented as literals.
- Addresses at or above `0x82074000` are writable guest memory and must be read from guest memory when needed, even if the original image contains constants or zeros.
- After converting a file, run `difftest` with `SBX_IMAGE` set. Writable data incorrectly converted into literals will appear as memory differences.

## Behavioral accuracy

- Preserve behavior that the game actually depends on.
- Do not reproduce incidental PowerPC execution quirks such as stack garbage, null-pointer reads, leftover registers, or NaN branch ordering unless a caller is demonstrated to depend on them.
- When behavior is uncertain, investigate the original implementation and its callers before deciding how to reproduce it.

## Project organization

- New native game code belongs in `src/game/`.
- `src/hooks/` is reserved for SBX-specific functionality, such as the patch loader, custom music, autosave, and game fixes.
- Every native game implementation must use the `src/game/` structure.
- The previous register-oriented implementation style using tables and register manipulation in `src/hooks/` is deprecated and must not be reintroduced.

## Generators

- Generators in `sbx_recomp/tools/native_gen/` are tools for investigating and reading original implementations.
- Generated output is reference material, not production code.
- Never commit generator output directly without rewriting it into the required native C++ style.