# CMAA2

`CMAA2.hlsl` is Intel's Conservative Morphological Anti-Aliasing 2.0,
version 2.3, unmodified, from https://github.com/GameTechDev/CMAA2
(Copyright (c) 2018, Intel Corporation; Apache License 2.0, see its header
and http://www.apache.org/licenses/LICENSE-2.0).

`../compile_cmaa2.py` patches it for Vulkan as it compiles it (listed at the
top of that script) into `../../generated_cmaa2.h`, used by `--cmaa2`
(`ApplyCmaa2` in `../../passes.cpp`).
