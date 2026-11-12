#!/bin/bash
# Shows a hooks file's tables (decoded) and its bodies without boilerplate.
cd /home/claude/wp/sbx_recomp/src/hooks
for f in "$@"; do echo "######## $f"; grep -m1 "REXCVAR_DEFINE_BOOL\|REXCVAR_DECLARE" -A3 $f.cpp | head -4; python3 /home/claude/tools/decode.py $f.cpp | sed 's/ (\([0-9A-F]*\) from [0-9A-F]*)//' | awk '{printf "%s ", $0} /^==/{print ""} END{print ""}'; sed -n '/^}  \/\/ namespace/,$p' $f.cpp | grep -v "REX_FUNC_PROLOGUE\|if (!On())\|__imp__\|return;$\|^  }$\|^$\|const guest_memory::Args args(ctx);\|^}  // namespace"; done
