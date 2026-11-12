// difftest shim: flags are plain globals, all on.
#pragma once
#define REXCVAR_DEFINE_BOOL(name, def, cat, desc) bool cvar_##name = true
#define REXCVAR_DEFINE_INT32(name, def, cat, desc) int cvar_##name = def
#define REXCVAR_DEFINE_DOUBLE(name, def, cat, desc) double cvar_##name = def
#define REXCVAR_DECLARE(type, name) extern type cvar_##name
#define REXCVAR_GET(name) cvar_##name
