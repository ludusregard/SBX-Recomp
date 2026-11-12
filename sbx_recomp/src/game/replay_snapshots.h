// The race's snapshots: what the replay saves of a race's state and puts
// back, as copies of the live objects' fields. The live objects' fields are
// named by their offsets (field_<offset>), and each snapshot's fields after
// the live field they hold, so a save and a restore are the same list of
// copies run the two ways. What each field is isn't worked out.

#pragma once

#include "game/guest_types.h"
#include "game/vehicle.h"

namespace game {

using guest::u32_be;

// A vehicle's track state: its world flags and track block, and its body's
// and wheels' route positions.
struct TrackSnapshot {
  u32_be in_world;
  u32_be unknown_6152;
  uint8_t unknown_8[8];
  uint8_t track_state[112];
  RoutePosition routes[5];  // The body's, then the four wheels'.
};
GUEST_OFFSET(TrackSnapshot, track_state, 16);
GUEST_OFFSET(TrackSnapshot, routes, 128);
GUEST_SIZE(TrackSnapshot, 608);

// A body's state: the live state and its snapshot share the layout.
struct BodyState {
  uint8_t field_0;
  uint8_t unknown_1[3];
  uint8_t field_4[8];
  uint8_t unknown_12[4];
  uint8_t field_16[36];
  uint8_t unknown_52[12];
  uint8_t field_64[52];
  uint8_t unknown_116[12];
  uint8_t field_128[40];
  uint8_t unknown_168[8];
  uint8_t field_176[212];
  uint8_t unknown_388[12];
  uint8_t field_400[116];
};
GUEST_OFFSET(BodyState, field_176, 176);
GUEST_OFFSET(BodyState, field_400, 400);

// One of a member's four wheel blocks (the same in the live member and its
// snapshot).
struct MemberWheelState {
  uint8_t field_0[2];
  uint8_t unknown_2[2];
  uint8_t field_4[20];
  uint8_t unknown_24[8];
  uint8_t field_32[52];
  uint8_t unknown_84[12];
  uint8_t field_96[128];
  uint8_t field_224[4];
  uint8_t field_228[116];
  uint8_t unknown_344[8];
  uint8_t field_352[16];
};
GUEST_SIZE(MemberWheelState, 368);
GUEST_OFFSET(MemberWheelState, field_228, 228);

// A car's member at +1136 (3,242 bytes copied) and its snapshot (2,938).
struct Member1136 {
  uint8_t field_0[56];
  uint8_t unknown_56[8];
  uint8_t field_64[208];
  uint8_t unknown_272[4];
  uint8_t field_276[1];
  uint8_t unknown_277[3];
  uint8_t field_280[56];
  uint8_t unknown_336[12];
  uint8_t field_348[84];
  MemberWheelState wheels[4];
  uint8_t field_1904[104];
  uint8_t unknown_2008[8];
  uint8_t field_2016[120];
  uint8_t unknown_2136[40];
  uint8_t field_2176[28];
  uint8_t field_2204[144];  // Copied with memcpy.
  uint8_t field_2348[32];
  uint8_t field_2380[8];
  uint8_t field_2388[12];
  uint8_t field_2400[2];
  uint8_t unknown_2402[2];
  uint8_t field_2404[12];
  uint8_t field_2416[8];
  uint8_t field_2424[1];
  uint8_t unknown_2425[3];
  uint8_t field_2428[12];
  uint8_t field_2440[4];
  uint8_t field_2444[2];
  uint8_t unknown_2446[2];
  uint8_t field_2448[8];
  uint8_t field_2456[1];
  uint8_t unknown_2457[7];
  uint8_t field_2464[40];
  uint8_t field_2504[1];
  uint8_t unknown_2505[3];
  uint8_t field_2508[4];
  uint8_t field_2512[1];
  uint8_t unknown_2513[3];
  uint8_t field_2516[108];
  uint8_t field_2624[2];
  uint8_t unknown_2626[2];
  uint8_t field_2628[36];
  uint8_t field_2664[1];
  uint8_t unknown_2665[259];
  uint8_t field_2924[4];
  uint8_t field_2928[1];
  uint8_t unknown_2929[3];
  uint8_t field_2932[16];
  uint8_t field_2948[1];
  uint8_t unknown_2949[3];
  uint8_t field_2952[8];
  uint8_t field_2960[1];
  uint8_t unknown_2961[3];
  uint8_t field_2964[20];
  uint8_t field_2984[60];  // Copied with memcpy.
  uint8_t field_3044[60];  // Copied with memcpy.
  uint8_t field_3104[24];
  uint8_t field_3128[1];
  uint8_t unknown_3129[3];
  uint8_t field_3132[24];
  uint8_t unknown_3156[12];
  uint8_t field_3168[72];
  uint8_t field_3240[2];
};
GUEST_OFFSET(Member1136, field_0, 0);
GUEST_OFFSET(Member1136, field_64, 64);
GUEST_OFFSET(Member1136, field_276, 276);
GUEST_OFFSET(Member1136, field_280, 280);
GUEST_OFFSET(Member1136, field_348, 348);
GUEST_OFFSET(Member1136, wheels, 432);
GUEST_OFFSET(Member1136, field_1904, 1904);
GUEST_OFFSET(Member1136, field_2016, 2016);
GUEST_OFFSET(Member1136, field_2176, 2176);
GUEST_OFFSET(Member1136, field_2204, 2204);
GUEST_OFFSET(Member1136, field_2348, 2348);
GUEST_OFFSET(Member1136, field_2380, 2380);
GUEST_OFFSET(Member1136, field_2388, 2388);
GUEST_OFFSET(Member1136, field_2400, 2400);
GUEST_OFFSET(Member1136, field_2404, 2404);
GUEST_OFFSET(Member1136, field_2416, 2416);
GUEST_OFFSET(Member1136, field_2424, 2424);
GUEST_OFFSET(Member1136, field_2428, 2428);
GUEST_OFFSET(Member1136, field_2440, 2440);
GUEST_OFFSET(Member1136, field_2444, 2444);
GUEST_OFFSET(Member1136, field_2448, 2448);
GUEST_OFFSET(Member1136, field_2456, 2456);
GUEST_OFFSET(Member1136, field_2464, 2464);
GUEST_OFFSET(Member1136, field_2504, 2504);
GUEST_OFFSET(Member1136, field_2508, 2508);
GUEST_OFFSET(Member1136, field_2512, 2512);
GUEST_OFFSET(Member1136, field_2516, 2516);
GUEST_OFFSET(Member1136, field_2624, 2624);
GUEST_OFFSET(Member1136, field_2628, 2628);
GUEST_OFFSET(Member1136, field_2664, 2664);
GUEST_OFFSET(Member1136, field_2924, 2924);
GUEST_OFFSET(Member1136, field_2928, 2928);
GUEST_OFFSET(Member1136, field_2932, 2932);
GUEST_OFFSET(Member1136, field_2948, 2948);
GUEST_OFFSET(Member1136, field_2952, 2952);
GUEST_OFFSET(Member1136, field_2960, 2960);
GUEST_OFFSET(Member1136, field_2964, 2964);
GUEST_OFFSET(Member1136, field_2984, 2984);
GUEST_OFFSET(Member1136, field_3044, 3044);
GUEST_OFFSET(Member1136, field_3104, 3104);
GUEST_OFFSET(Member1136, field_3128, 3128);
GUEST_OFFSET(Member1136, field_3132, 3132);
GUEST_OFFSET(Member1136, field_3168, 3168);
GUEST_OFFSET(Member1136, field_3240, 3240);

struct Member1136Snapshot {
  uint8_t field_0[56];
  uint8_t unknown_56[8];
  uint8_t field_64[208];
  uint8_t field_276[1];
  uint8_t unknown_273[3];
  uint8_t field_280[56];
  uint8_t field_348[84];
  MemberWheelState wheels[4];
  uint8_t field_1904[104];
  uint8_t unknown_1992[8];
  uint8_t field_2016[120];
  uint8_t unknown_2120[8];
  uint8_t field_2176[28];
  uint8_t field_2204[144];  // Copied with memcpy.
  uint8_t field_2348[32];
  uint8_t field_2380[8];
  uint8_t field_2388[12];
  uint8_t field_2400[2];
  uint8_t unknown_2354[2];
  uint8_t field_2404[12];
  uint8_t field_2428[12];
  uint8_t field_2416[8];
  uint8_t field_2424[1];
  uint8_t unknown_2389[3];
  uint8_t field_2440[4];
  uint8_t field_2444[2];
  uint8_t unknown_2398[2];
  uint8_t field_2448[8];
  uint8_t field_2456[1];
  uint8_t unknown_2409[7];
  uint8_t field_2464[40];
  uint8_t field_2504[1];
  uint8_t unknown_2457[3];
  uint8_t field_2508[4];
  uint8_t field_2512[1];
  uint8_t unknown_2465[3];
  uint8_t field_2516[108];
  uint8_t field_2624[2];
  uint8_t unknown_2578[2];
  uint8_t field_2628[36];
  uint8_t field_2664[1];
  uint8_t unknown_2617[3];
  uint8_t field_2924[4];
  uint8_t field_2928[1];
  uint8_t unknown_2625[3];
  uint8_t field_2932[16];
  uint8_t field_2948[1];
  uint8_t unknown_2645[3];
  uint8_t field_2952[8];
  uint8_t field_2960[1];
  uint8_t unknown_2657[3];
  uint8_t field_2964[20];
  uint8_t field_2984[60];  // Copied with memcpy.
  uint8_t field_3044[60];  // Copied with memcpy.
  uint8_t field_3104[24];
  uint8_t field_3128[1];
  uint8_t unknown_2825[3];
  uint8_t field_3132[24];
  uint8_t unknown_2852[12];
  uint8_t field_3168[72];
  uint8_t field_3240[2];
};
GUEST_OFFSET(Member1136Snapshot, field_0, 0);
GUEST_OFFSET(Member1136Snapshot, field_64, 64);
GUEST_OFFSET(Member1136Snapshot, field_276, 272);
GUEST_OFFSET(Member1136Snapshot, field_280, 276);
GUEST_OFFSET(Member1136Snapshot, field_348, 332);
GUEST_OFFSET(Member1136Snapshot, wheels, 416);
GUEST_OFFSET(Member1136Snapshot, field_1904, 1888);
GUEST_OFFSET(Member1136Snapshot, field_2016, 2000);
GUEST_OFFSET(Member1136Snapshot, field_2176, 2128);
GUEST_OFFSET(Member1136Snapshot, field_2204, 2156);
GUEST_OFFSET(Member1136Snapshot, field_2348, 2300);
GUEST_OFFSET(Member1136Snapshot, field_2380, 2332);
GUEST_OFFSET(Member1136Snapshot, field_2388, 2340);
GUEST_OFFSET(Member1136Snapshot, field_2400, 2352);
GUEST_OFFSET(Member1136Snapshot, field_2404, 2356);
GUEST_OFFSET(Member1136Snapshot, field_2428, 2368);
GUEST_OFFSET(Member1136Snapshot, field_2416, 2380);
GUEST_OFFSET(Member1136Snapshot, field_2424, 2388);
GUEST_OFFSET(Member1136Snapshot, field_2440, 2392);
GUEST_OFFSET(Member1136Snapshot, field_2444, 2396);
GUEST_OFFSET(Member1136Snapshot, field_2448, 2400);
GUEST_OFFSET(Member1136Snapshot, field_2456, 2408);
GUEST_OFFSET(Member1136Snapshot, field_2464, 2416);
GUEST_OFFSET(Member1136Snapshot, field_2504, 2456);
GUEST_OFFSET(Member1136Snapshot, field_2508, 2460);
GUEST_OFFSET(Member1136Snapshot, field_2512, 2464);
GUEST_OFFSET(Member1136Snapshot, field_2516, 2468);
GUEST_OFFSET(Member1136Snapshot, field_2624, 2576);
GUEST_OFFSET(Member1136Snapshot, field_2628, 2580);
GUEST_OFFSET(Member1136Snapshot, field_2664, 2616);
GUEST_OFFSET(Member1136Snapshot, field_2924, 2620);
GUEST_OFFSET(Member1136Snapshot, field_2928, 2624);
GUEST_OFFSET(Member1136Snapshot, field_2932, 2628);
GUEST_OFFSET(Member1136Snapshot, field_2948, 2644);
GUEST_OFFSET(Member1136Snapshot, field_2952, 2648);
GUEST_OFFSET(Member1136Snapshot, field_2960, 2656);
GUEST_OFFSET(Member1136Snapshot, field_2964, 2660);
GUEST_OFFSET(Member1136Snapshot, field_2984, 2680);
GUEST_OFFSET(Member1136Snapshot, field_3044, 2740);
GUEST_OFFSET(Member1136Snapshot, field_3104, 2800);
GUEST_OFFSET(Member1136Snapshot, field_3128, 2824);
GUEST_OFFSET(Member1136Snapshot, field_3132, 2828);
GUEST_OFFSET(Member1136Snapshot, field_3168, 2864);
GUEST_OFFSET(Member1136Snapshot, field_3240, 2936);

// A part's points (in the live part, 32 bytes apart).
struct PartPoint {
  uint8_t field_0[20];
  uint8_t unknown_20[12];
};
GUEST_SIZE(PartPoint, 32);

struct Part464 {
  uint8_t unknown_0[16];
  uint8_t field_16[20];
  uint8_t unknown_36[12];
  uint8_t field_48[68];
  uint8_t unknown_116[12];
  uint8_t field_128[32];
  uint8_t unknown_160[16];
  uint8_t field_176[132];
  uint8_t unknown_308[12];
  PartPoint points[5];
  uint8_t field_480[16];
};
GUEST_OFFSET(Part464, field_128, 128);
GUEST_OFFSET(Part464, points, 320);
GUEST_OFFSET(Part464, field_480, 480);

struct PartSnapshot {
  uint8_t field_128[32];
  uint8_t field_176[132];
  uint8_t unknown_164[12];
  PartPoint points[5];  // Packed: each point's 20 bytes, then its 12.
  uint8_t field_480[16];
  uint8_t field_16[20];
  uint8_t unknown_372[12];
  uint8_t field_48[68];
  uint8_t unknown_452[12];
};
GUEST_OFFSET(PartSnapshot, points, 176);
GUEST_OFFSET(PartSnapshot, field_480, 336);
GUEST_OFFSET(PartSnapshot, field_16, 352);
GUEST_OFFSET(PartSnapshot, field_48, 384);
GUEST_SIZE(PartSnapshot, 464);

struct PartShapes {
  PartSnapshot parts[2];
  uint8_t field_928[100];
};
GUEST_OFFSET(PartShapes, field_928, 928);

struct Member4080 {
  uint8_t field_0[4];
  uint8_t unknown_4[12];
  PartShapes shapes;
  uint8_t unknown_1044[12];
  uint8_t field_1056[36];
  uint8_t unknown_1092[12];
  uint8_t field_1104[24];
  uint8_t unknown_1128[8];
  uint8_t field_1136[16];
};
GUEST_OFFSET(Member4080, shapes, 16);
GUEST_OFFSET(Member4080, field_1056, 1056);
GUEST_OFFSET(Member4080, field_1136, 1136);

// One of the replay's records (Record5232::Record5232): its own vectors,
// a block made like a Member1136 at +1136, then a Member4080.
struct Record5232 {
  uint8_t unknown_0[1136];
  uint8_t member_1136[2944];  // Its vectors made as Member1136's are.
  Member4080 member_4080;
};
GUEST_SIZE(Record5232, 5232);
GUEST_OFFSET(Record5232, member_4080, 4080);

// The input recordings at [0x826B582C]: two runs of seven records.
struct InputRecordings {
  uint8_t unknown_0[2162880];
  Record5232 records_a[7];
  uint8_t unknown_2199504[2864];
  Record5232 records_b[7];
  u32_be unknown_2238992;  // Cleared once made.
  uint8_t unknown_2238996[12];
};
GUEST_OFFSET(InputRecordings, records_a, 2162880);
GUEST_OFFSET(InputRecordings, records_b, 2202368);
GUEST_OFFSET(InputRecordings, unknown_2238992, 2238992);
GUEST_SIZE(InputRecordings, 2239008);

// The replay at [0x826B5830].
struct Replay {
  uint8_t unknown_0[215372];
  u32_be unknown_215372;  // 1 once made.
  uint8_t unknown_215376[2864];
  Record5232 records[7];
  uint8_t unknown_254864[1080016];
};
GUEST_OFFSET(Replay, unknown_215372, 215372);
GUEST_OFFSET(Replay, records, 218240);
GUEST_SIZE(Replay, 1334880);

}  // namespace game
