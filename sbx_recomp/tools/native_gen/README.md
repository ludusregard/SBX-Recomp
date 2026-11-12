# Native code tools

Helpers for understanding the game's code before writing it as our own
C++ (`docs/native-code.md`). They read SBX's generated code
(`generated/default/sbx_recomp.*.cpp`, whose comments carry each
instruction) and print what a function does.

The generators (`gen_fill.py`, `gen_copy.py`, `gen_file.py`,
`gen_copyfile.py`) made the older table-style files. Their output is not
committed any more: native code is written by hand as plain C++ over typed
structs (`CLAUDE.md`, "Native game code: plain C++ only").

Run from this folder with `python <tool>`. Build the index first; it's
large and stays out of git.

| Tool | What |
|---|---|
| `index.py [port_report.tsv]` | Builds `idx.pkl`: each function's generated text, the files overriding it, and `port_native.py`'s grade for it (from its TSV report). Run again after a codegen or when overrides change. |
| `asm.py ADDR...` | A function's instructions, compact, with its grade. |
| `live.py ADDR...` | The argument registers (r3 to r10, f1 to f8) a function reads before writing, following its blocks and its own calls. A call has to set each of these as the original does. |
| `sym.py ADDR` | Runs a function symbolically (constants, image loads, the object and argument pointers, the stack, loops, leaf calls inlined) and lists its stores and calls in order. Fails loudly on what it can't follow. |
| `gen_fill.py ADDR NAME` | From `sym.py`'s run: a `guest_memory::Fill` table and the body that applies it and makes the calls, each call with the registers its callee reads. |
| `gen_copy.py ADDR NAME` | The same for copies: `CopyWords`, `CopyHalves`, `CopyBytes`, `Memcpy` between the argument pointers. |
| `gen_file.py LIST OUT FLAG [HEADER] [EXTRA]`, `gen_copyfile.py` | A whole file of such overrides from a list of `ADDR Name` lines; hand-written code is spliced in at `@@FUNCS@@`. |
| `survey.py FOLDER [LIST]` | Which of a list's functions the generators can do. |
| `la.py HI LO...` | The address a `lis`/`addi` pair makes. |
| `folder.py FOLDER`, `itcfn.py FOLDER ADDR...` | With `SBX_ITC_PORTED` set to `port_native.py`'s `--out` folder: that folder's native functions with their grades, sizes and calls; one function's C++ there. |
| `check_call_args.py FILE...` | Calls in an older `src/hooks/FILE.cpp` that don't set a register the callee reads. |
| `check_consts.py FILE...` | Numbers in the C++ that aren't in the originals' code. |
| `check_float_compares.py` | The overrides whose originals have a float compare, to read their branches against the C++ (`ble` is `!(a > b)`, `bge` is `!(a < b)`: both taken on a NaN). |

| `difftest/dt.py FILE.cpp...` | Linux only. Builds each override in a `src/game` or `src/hooks` file together with the recompiled function it replaces and runs both on the same test memory with stub callees: the memory written, the calls and their arguments, and the results are compared. Optional: for chasing a difference found in play. With `SBX_IMAGE` set to a flat dump of the game image at 0x82000000 (`tools/xex_image.py`), the image's read-only data is in the test memory, so natives may use its constants as literals. `NULL` lines are runs where the test passed address 0 and the native stopped on the null pointer the original read through. |

What the generators got wrong, and `difftest` caught: a global that's
read and then written in the same function (the table would read it after
the write), and a swap written as two copies. A function returning a float
in f1 isn't a table at all (`sym.py` prints f1 at the end: anything but the
register as it came, or a callee's, needs writing by hand).

The checks list things to read, not errors: a helper may set a register
for its caller, and the C++ may add up an offset the original loads in two
steps.
