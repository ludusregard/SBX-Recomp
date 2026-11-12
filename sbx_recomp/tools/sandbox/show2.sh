#!/bin/bash
# Like show.sh, keeping the functions' name comments.
cd /home/claude/wp/sbx_recomp/src/hooks
for f in "$@"; do echo "######## $f"; python3 /home/claude/tools/decode.py $f.cpp | sed 's/ (\([0-9A-F]*\) from [0-9A-F]*)//' | awk '{printf "%s ", $0} /^==/{print ""} END{print ""}'; sed -n '/^}  \/\/ namespace/,$p' $f.cpp | grep -v "REX_FUNC_PROLOGUE\|if (!On())\|__imp__\|return;$\|^  }$\|^$\|const guest_memory::Args args(ctx);\|^}  // namespace\|^}$\|const uint32_t object = ctx.r3.u32;"; done
