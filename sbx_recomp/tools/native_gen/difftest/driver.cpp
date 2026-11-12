// Differential test driver: runs each function's recompiled original and its
// native override on the same lazily filled guest memory and prints what
// each did (calls with arguments, memory written, registers left).
#include "sbx_init.h"
#include <csignal>
#include <cstdio>
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>
#include <csetjmp>
#include <vector>
#include <string>

struct Entry { uint32_t addr; PPCFunc* orig; PPCFunc* native; };
extern Entry g_entries[]; extern int g_entry_count;

static uint8_t* g_base; static uint64_t g_seed;
static std::vector<uint32_t> g_pages; static sigjmp_buf g_jmp; static volatile int g_in_run;
static const uint64_t kSize = 0x100000000ull + 0x10000;

static inline uint32_t Mix(uint64_t x) { x ^= x >> 33; x *= 0xff51afd7ed558ccdULL; x ^= x >> 33; x *= 0xc4ceb9fe1a85ec53ULL; x ^= x >> 33; return uint32_t(x); }
// The game image's constants (.rdata), when SBX_IMAGE names a flat dump of
// the image at 0x82000000 (tools/xex_image.py): floats and vtables as the
// game has them, so natives may use their values.
static std::vector<uint8_t> g_image; static const uint32_t kImageBase = 0x82000000u, kRdataEnd = 0x82074000u;
static void FillPage(uint32_t page) {
  uint32_t* p = (uint32_t*)(g_base + (uint64_t(page) << 12));
  uint64_t at = uint64_t(page) << 12;
  if (at >= kImageBase && at < kRdataEnd && at - kImageBase + 4096 <= g_image.size()) {
    memcpy(p, g_image.data() + (at - kImageBase), 4096);
    return;
  }
  for (int i = 0; i < 1024; i++) {
    uint32_t h = Mix((uint64_t(page) << 12 | i * 4) ^ (g_seed << 40));
    // Mostly pointer-like words (so loaded pointers can be followed), some small.
    uint32_t v = (h & 7) == 0 ? (h >> 28) : (0x40000000u | ((h >> 3) & 0x0FFFFFF0u));
    p[i] = __builtin_bswap32(v);
  }
}
static void OnSegv(int, siginfo_t* si, void*) {
  uint8_t* a = (uint8_t*)si->si_addr;
  if (a >= g_base && a < g_base + kSize) {
    uint32_t page = uint32_t((a - g_base) >> 12);
    mprotect(g_base + (uint64_t(page) << 12), 4096, PROT_READ | PROT_WRITE);
    FillPage(page); g_pages.push_back(page);
    if (g_pages.size() > 16384 && g_in_run) siglongjmp(g_jmp, 4);  // 64 MB touched: a runaway
    return;
  }
  // Status 6: a null host pointer (a native given guest address 0, which
  // the original reads as memory).
  if (g_in_run) siglongjmp(g_jmp, (uintptr_t)a < 0x100000 ? 6 : 1);
  _exit(3);
}
static void OnAlarm(int) { if (g_in_run) siglongjmp(g_jmp, 2); }
static void ResetPages() {
  for (uint32_t page : g_pages) mprotect(g_base + (uint64_t(page) << 12), 4096, PROT_NONE);
  // Dropping the pages gives fresh zero pages; the fault handler refills.
  for (uint32_t page : g_pages) madvise(g_base + (uint64_t(page) << 12), 4096, MADV_DONTNEED);
  g_pages.clear();
}

static std::string g_trace; static int g_calls; static uint32_t g_indirect;
static uint32_t HashAt(uint32_t address) {
  if (address >= 0x70000000u && address < 0x70010000u) return 0;  // the stack: saved registers differ
  uint64_t h = 1469598103934665603ull;
  for (int i = 0; i < 48; i++) { h ^= g_base[uint64_t(address) + i]; h *= 1099511628211ull; }
  return uint32_t(h ^ (h >> 32));
}
extern "C" void difftest_stub(uint32_t target, PPCContext& ctx, uint8_t* base) {
  if (target == 0x8258C950) {  // memset, done for real: natives clear memory themselves
    for (uint32_t i = 0; i < ctx.r5.u32 && i < (1u << 24); i++) base[uint64_t(ctx.r3.u32) + i] = uint8_t(ctx.r4.u32);
    return;
  }
  char line[900]; int n = snprintf(line, sizeof line, "CALL %08X", target);
  uint64_t regs[8] = {ctx.r3.u64, ctx.r4.u64, ctx.r5.u64, ctx.r6.u64, ctx.r7.u64, ctx.r8.u64, ctx.r9.u64, ctx.r10.u64};
  for (int i = 0; i < 8; i++) n += snprintf(line + n, sizeof line - n, " r%d=%08X:%08X", i + 3, uint32_t(regs[i]), HashAt(uint32_t(regs[i])));
  PPCRegister* f[8] = {&ctx.f1, &ctx.f2, &ctx.f3, &ctx.f4, &ctx.f5, &ctx.f6, &ctx.f7, &ctx.f8};
  for (int i = 0; i < 8; i++) n += snprintf(line + n, sizeof line - n, " f%d=%016llX", i + 1, (unsigned long long)f[i]->u64);
  n += snprintf(line + n, sizeof line - n, " r1=%08X\n", ctx.r1.u32);
  g_trace += line;
  int k = g_calls++;
  if (k > 3000 && g_in_run) siglongjmp(g_jmp, 5);  // a loop the stubs never end
  ctx.r3.u64 = 0x60000000u + 0x2000u * k + 0x100;
  PPCRegister* v[] = {&ctx.r0, &ctx.r4, &ctx.r5, &ctx.r6, &ctx.r7, &ctx.r8, &ctx.r9, &ctx.r10, &ctx.r11, &ctx.r12};
  for (int i = 0; i < 10; i++) v[i]->u64 = 0x5A000000u + 0x10000u * i + 0x800 + 16 * k;
  ctx.f1.f64 = 1.25 + k;
  PPCRegister* fv[] = {&ctx.f0, &ctx.f2, &ctx.f3, &ctx.f4, &ctx.f5, &ctx.f6, &ctx.f7, &ctx.f8, &ctx.f9, &ctx.f10, &ctx.f11, &ctx.f12, &ctx.f13};
  for (int i = 0; i < 13; i++) fv[i]->f64 = 100.5 + i + 0.125 * k;
}
static void IndirectStub(PPCContext& ctx, uint8_t* base) { difftest_stub(g_indirect, ctx, base); }
namespace rex { LogCategoryId RegisterLogCategory(const char*) { return LogCategoryId(0); } spdlog::logger* GetLoggerRaw(LogCategoryId) { return nullptr; } }
namespace rex::runtime { PPCFunc* ResolveIndirectFunction(uint32_t a) { g_indirect = a; return &IndirectStub; } }

static const uint32_t kStack = 0x70008000;
static void Init(PPCContext& ctx) {
  memset((void*)&ctx, 0, sizeof ctx);
  ctx.r1.u64 = kStack; ctx.r3.u64 = 0x50000100; ctx.r4.u64 = 0x51000200; ctx.r5.u64 = 0x52000300;
  ctx.r6.u64 = 0x53000400; ctx.r7.u64 = 0x54000500; ctx.r8.u64 = 0x55000600; ctx.r9.u64 = 0x56000700;
  ctx.r10.u64 = 0x57000800; ctx.r11.u64 = 0x58000900; ctx.r12.u64 = 0x59000A00; ctx.r0.u64 = 0x4F000B00;
  ctx.r13.u64 = 0x7E000000; ctx.r2.u64 = 0x4E000000;
  PPCRegister* f[] = {&ctx.f0, &ctx.f1, &ctx.f2, &ctx.f3, &ctx.f4, &ctx.f5, &ctx.f6, &ctx.f7, &ctx.f8, &ctx.f9, &ctx.f10, &ctx.f11, &ctx.f12, &ctx.f13};
  for (int i = 0; i < 14; i++) f[i]->f64 = double(float(0.37 * (i + 1) + 0.01 * (g_seed % 7)));
  if (g_seed & 1) { ctx.r4.u64 = 3 + g_seed; ctx.r5.u64 = 1; ctx.r6.u64 = 0; }
  if (g_seed & 2) { ctx.r4.u64 = 0; }
}
static void Dump(const char* who, PPCContext& ctx, int status) {
  printf("%s status=%d\n%s", who, status, g_trace.c_str());
  PPCRegister* r[] = {&ctx.r0, &ctx.r3, &ctx.r4, &ctx.r5, &ctx.r6, &ctx.r7, &ctx.r8, &ctx.r9, &ctx.r10, &ctx.r11, &ctx.r12};
  const char* rn[] = {"r0", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12"};
  printf("REGS");
  for (int i = 0; i < 11; i++) printf(" %s=%016llX", rn[i], (unsigned long long)r[i]->u64);
  PPCRegister* f[] = {&ctx.f0, &ctx.f1, &ctx.f2, &ctx.f3, &ctx.f4, &ctx.f5, &ctx.f6, &ctx.f7, &ctx.f8, &ctx.f9, &ctx.f10, &ctx.f11, &ctx.f12, &ctx.f13};
  for (int i = 0; i < 14; i++) printf(" f%d=%016llX", i, (unsigned long long)f[i]->u64);
  printf(" r1=%08X r31=%016llX\n", ctx.r1.u32, (unsigned long long)ctx.r31.u64);
  // Memory that differs from the fill, outside the stack's window.
  std::vector<uint32_t> pages = g_pages;
  std::sort(pages.begin(), pages.end());
  for (uint32_t page : pages) {
    uint64_t a0 = uint64_t(page) << 12;
    if (a0 >= kStack - 0x8000 && a0 < kStack + 0x8000) continue;
    uint32_t copy[1024]; memcpy(copy, g_base + a0, 4096);
    FillPage(page);  // back to the fill, to compare
    uint32_t* p = (uint32_t*)(g_base + a0);
    for (int i = 0; i < 1024; i++) if (copy[i] != p[i]) printf("MEM %08llX=%08X\n", (unsigned long long)(a0 + i * 4), __builtin_bswap32(copy[i]));
  }
  printf("END\n");
}
int main(int argc, char** argv) {
  setvbuf(stdout, nullptr, _IOFBF, 1 << 20);
  g_base = (uint8_t*)mmap(nullptr, kSize, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
  if (g_base == MAP_FAILED) { perror("mmap"); return 1; }
  static uint8_t altstack[1 << 16]; stack_t ss{altstack, 0, sizeof altstack}; sigaltstack(&ss, nullptr);
  struct sigaction sa{}; sa.sa_sigaction = OnSegv; sa.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_NODEFER; sigaction(SIGSEGV, &sa, nullptr);
  struct sigaction sb{}; sb.sa_handler = OnAlarm; sb.sa_flags = SA_NODEFER; sigaction(SIGALRM, &sb, nullptr);
  int seeds = argc > 1 ? atoi(argv[1]) : 4;
  if (const char* path = getenv("SBX_IMAGE")) {
    if (FILE* f = fopen(path, "rb")) {
      g_image.resize(kRdataEnd - kImageBase);
      g_image.resize(fread(g_image.data(), 1, g_image.size(), f));
      fclose(f);
    }
  }
  for (int e = 0; e < g_entry_count; e++) {
    for (int s = 0; s < seeds; s++) {
      g_seed = s;
      for (int which = 0; which < 2; which++) {
        PPCContext ctx; Init(ctx); g_trace.clear(); g_calls = 0;
        int status = sigsetjmp(g_jmp, 1);
        if (status == 0) { g_in_run = 1; alarm(2); (which ? g_entries[e].native : g_entries[e].orig)(ctx, g_base); alarm(0); }
        g_in_run = 0; alarm(0);
        printf("RUN %08X seed=%d ", g_entries[e].addr, s);
        Dump(which ? "native" : "orig", ctx, status);
        ResetPages();
      }
    }
  }
  return 0;
}
