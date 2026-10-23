# Game data

What's known about the game's data files: how they're stored, how the game
loads them, and the formats inside. The code side (function names and
addresses) is in `docs/functions.md`; this file links to it where a
format's reader is known.

Paths are relative to `assets/`, the user's copy of the game. File ids are
TOC indices, written as the 8-digit names an extractor gives them
(`00000072`). "Confirmed" means checked against the game's code or the
data in this copy; everything else is marked as inferred.

## Byte order

It's mixed, so check each format:

- **Big-endian** (the console's order): the archive indexes (`.TOC`), XPR2
  textures, and the model files (node and mesh files, floats included),
  which go to the GPU as they are. The text in string tables is UTF-16 BE.
- **Little-endian**, byte-swapped by the game after loading: container
  headers, string-table pointers, the parameter tables (counts, floats,
  ints) and light sets (`sub_82292170` and `Lights::FindNearest` swap after
  `lwz`).

## The archives

`DATA/BUILD/TXR/` holds two archives, each a `.TOC` index and a `.DAT`
payload:

| Archive | Entries | `.DAT` size | Ids |
|---|---|---|---|
| `BUILD.TOC` / `BUILD.DAT` | 23,009 | 921,079,808 | 0-23008 |
| `BUILD_CRS.TOC` / `BUILD_CRS.DAT` | 1,359 | 1,688,393,728 | 23009-24367 |

The game treats the two as one id space: an id below 23,009 is in `BUILD`,
anything else is in `BUILD_CRS` at id - 23,009 (`sub_8209D988`,
`sub_8209DF80`).

`BUILD_CRS` is the game world for race mode, and only that: map assets,
with no game logic data such as car stats. It's interchangeable with
Import Tuner Challenge's `BUILD_CRS` (whose world has different textures
and advertising billboards), while `BUILD` isn't. Import Tuner
Challenge's `BUILD_CRS.TOC` is the same size, so it has the same 1,359
entries, while its `.DAT` is about 200 MB bigger. That's why a swap works:
the TOC travels with its `.DAT`, the entry count matches the one built
into the XEX (see below), and the `.DAT` size the XEX also keeps is never
read.

### TOC format (confirmed)

All fields are big-endian u32.

```
+0x00  entry count
+0x04  unknown (2 words; the game never reads them)
+0x0C  size of the matching .DAT in bytes
+0x10  entries, 16 bytes each:
         +0  start sector (x 0x800 = byte offset in the .DAT)
         +4  stored size in bytes
         +8  decompressed size; 0 = stored uncompressed
         +12 0 in every entry
```

Entries are read in place: entry `n` is at `toc + (n + 1) * 16`.
`BUILD.TOC` has 7,893 compressed entries and 15,039 entries with a stored
size of 0 (empty slots), leaving 7,970 entries with data. Every entry in
`BUILD_CRS.TOC` has data.

### Compression (confirmed)

Compressed entries are UCL streams in the `uclpack` layout: the magic
`00 E9 55 43 4C FF 01 1A` ("UCL"), flags, method, level and block size (18
bytes in all), then blocks of `[decompressed size, stored size]` (BE u32)
followed by the data, ended by a block of size 0. A block whose stored
size isn't smaller than its decompressed size is stored as is.
`Ucl::Unpack` (`sub_82292050`) walks the blocks; `sub_82292238`
decompresses one.

### Copies of the TOC headers in the XEX (confirmed)

`default.xex` has the 16-byte header of each TOC built in, in `.data`:

| Address | What |
|---|---|
| `0x825F0A90` | `BUILD.TOC` header: 23,009 entries, the two unknown words, `.DAT` size. |
| `0x825F0AA0` | `BUILD_CRS.TOC` header: 1,359 entries, ..., `.DAT` size. |

Only the counts are read: `Archive::Init` sizes each TOC buffer as
`(count + 1) * 16` from them, then reads the `.TOC` file into it. The
split point 23,009 is also an immediate in the code (`sub_8209D988`,
`sub_8209D9F8`, `sub_8209DE20`, `sub_8209DF80`, `sub_820A0860`).

What this means for mods: entries can be changed or resized in place, but
**adding entries to `BUILD.TOC`** overflows the TOC buffer and moves every
`BUILD_CRS` id, unless the count at `0x825F0A90` and the 23,009
immediates are patched to match. A tool that appends entries (GUT Archive
Tools' `-a`) needs a matching patch in the recomp.

### Loose files (confirmed)

Some TOC entries have a start sector of 0 and no payload: their data is
a loose file on disc, and the entry only gives its size. The table at
`0x825F0AB0` (pairs of id and path, ended by a null path) maps them;
`Archive::FindLooseFile` (`sub_8209D9F8`) looks an id up.

| Id | File |
|---|---|
| 0 | `DATA/DEBUG/DEBFONT.XPR` |
| 22812-22820 | `DATA/TXR/MOVIE/`: `OPENING`, `STAGE1_OPENING`, `SHIBUYA_OPEN`, `STAGE2_OPENING`, `SHINJUKU_OPEN`, `STAGE3_OPENING`, `ENDING`, `STAFFROLL1`, `PERFECT_ENDING` (`.WMV`) |
| 22821 | `DATA/TXR/MOVIE/STAFFROLL2.WMV` |
| 22884 | `DATA/AUDIO/TXR_XACT.XGS` |
| 22885 | `DATA/AUDIO/SB_SYSTEM_SE.XSB` |
| 22886 | `DATA/AUDIO/SB_SYSTEM_SE.XWB` |
| 22887 | `DATA/AUDIO/CAR_SE.XWB` |

The movies and the XACT audio (global settings, a sound bank and two wave
banks) are therefore plain files, not archive entries.

### How the game loads an entry (confirmed)

1. At boot, `Archive::Init` (`sub_8209E348`) allocates the TOC buffers
   (`0x82694E00` for `BUILD`, `0x82694E08` for `BUILD_CRS`), reads both
   `.TOC` files into them, and opens both `.DAT` files (`CreateFile`,
   handles at `0x825F0A84` and `0x825F0A88`). A file that can't be opened
   is fatal.
2. A caller asks for an id with `Archive::Load` (`sub_8209DF80`). It reads
   the TOC entry, checks for a loose file, and queues a 40-byte request
   (`sub_8209DD38`).
3. The loader thread (`sub_8209E250`) takes requests off the queue and
   reads them: from the `.DAT` (`sub_8209DE20`, at start sector x 0x800)
   or from the loose file (`sub_8209E0E0`). Compressed entries are then
   unpacked with `Ucl::Unpack`.
4. A failed read shows the console's dirty-disc screen (`sub_8209AD48` →
   `sub_82292D50` → `XamShowDirtyDiscErrorUI`), which is what a broken
   rebuilt archive will look like.

`Archive::Load` reads the TOC entry on every load, and the callers that
size their buffers read it too (`sub_820C9700` for the parameter tables), so
changing an entry in memory after `Archive::Init` changes what's loaded.
Every caller passes 0 for its optional read size, so the TOC's size is
always the one read.

Callers that pass a constant id: `sub_820CC048` loads 72-83 (the parameter
tables below) and `sub_820C9700` sizes them; `sub_821173A8` loads 84, 94,
95 and 96; `sub_821744D8` loads 22887 (`CAR_SE.XWB`); `sub_8217E1C0` loads
22599. The car loaders compute their ids (see "Car models" below).

### The shared load buffer (confirmed)

`Archive::Init` also allocates one 0xC80000-byte (12.5 MiB) load buffer;
the size is its second argument, passed by `sub_821066D0`. Pointer and
size are at `0x82694E14` and `0x82694E10`; the car code reads them from
`[0x826B2B24]+5008` and `+5012`, which `sub_820C8B88` fills from those
globals (or from another buffer in some cases, not traced). Several loads go into it one at a time, and
the data is copied out before the next one:

- the body XPR2 of a set-b or set-c car (`CarModel::LoadSetB`,
  `CarInterior::LoadSetCBody`): **no size check**, so a larger file
  overruns the buffer;
- the vinyls and stickers (`LoadSlot::LoadShared`): a file that doesn't
  fit is skipped, and its slot is treated as empty.

A set-a car's node file and body XPR2 get their own allocations, sized
from the TOC.

### Patch data (`PATCH_DATA`)

The recomp's patch loader replaces archive entries with loose files
(`src/hooks/patch_loader.cpp`, `--patch_loader`):

```
assets/PATCH_DATA/BUILD/TXR/BUILD/<id>.<any extension>       BUILD ids 0-23008
assets/PATCH_DATA/BUILD/TXR/BUILD_CRS/<id>.<any extension>   BUILD_CRS ids
```

A file is named by its id, as GUT Archive Tools names its dumps
(`00000072.bin`, the car specs); the extension is ignored. In
`BUILD_CRS/` the id is either the game's (23009-24367) or the one within
that archive (0-1358). Files are uncompressed, like the dumps, and may be
larger or smaller than the original; their format is up to the modder.
Files the game loads into the shared load buffer (above) must still fit
in it: set-b and set-c car textures and the decal TGAs.

How: after `Archive::Init` reads the TOCs, each replaced id's TOC entry is
changed in memory to start sector 0, stored size = the file's size and
decompressed size 0, which makes it a loose file stored as is; then
`Archive::FindLooseFile` returns the override's path for it
(`PATCH_DATA\BUILD\TXR\BUILD\00000072.bin`, under `game:\`). The `.TOC`
and `.DAT` files aren't changed. Only existing ids can be replaced
(adding ids needs the counts above patched). The log names each override
at boot and each time one is loaded.

`assets/PATCH_DATA/BUILD/TXR/BUILD/` is a git submodule (`SBXR_MOD`): the
`gut_archive_files` branch of Shutoko Battle X Restored, which holds that
patch's archive files at its root (text tables 22637-22653, fonts 1, 2
and 22654-22657, menu screens). `.gitignore` ignores `assets/` except
`PATCH_DATA` and `ADDON_DATA`. Files added next to the submodule (another archive's folder,
local overrides) show up as untracked.

Not covered yet: the loose files on disc (movies, XACT audio), and new
archives (e.g. new maps).

`assets/ADDON_DATA/` is for new content beside the base game, for the
addon loader (`docs/modding-plan.md`); nothing reads it yet.

## Containers (`.dat` inside the archive)

Many entries are themselves containers of smaller files (GUT Archive
Tools calls them `.dat`):

```
+0   u32 LE  file count
+4   u32 LE  offset of each file from the container's start, count of them
...  the files, back to back (each ends where the next begins)
```

`DataFile::GetEntry` (`sub_82292170`) returns the address of file `n`, or 0
if `n` is out of range; with its third argument set it also returns 0 for
a file whose first word is 0. The light sets `Lights::FindNearest` reads
are containers like this.

## String tables (confirmed)

The game's text is in BUILD entries 22637-22653, one table per entry:

```
+0          u32 LE  string count
+4          u32 LE  offset of each string from the table's start
+4+4n       2 bytes of zero
...         strings, UTF-16 BE, each normally ended by 00 00 00
```

The tables have the same layout as a container, so GUT Archive Tools
writes them as `.dat` (`00022637.dat`). There's no room in the font for
new characters, so the old tools add accented Latin letters by reusing
kanji the text doesn't need (`string_remap.py` in the old research, see
below), presumably with those glyphs redrawn in the font sheet.

| Id | Contents |
|---|---|
| 22637 | Car names and descriptions |
| 22638 | Menu dialogs ("enter a Storage Device..."), shop item descriptions |
| 22639 | Map menu dialogs ("You will return to garage, is that okay?") |
| 22640 | NPC names, rival list descriptions, some dialog |
| 22641 | Japanese city names |
| 22642 | B.A.D. names |
| 22643-22646 | Dialog |
| 22647-22651 | Dialog (rival conversations) |
| 22652 | Online system messages ("Your opponent has stopped responding.") |
| 22653 | Garage notifications ("BRAKE LEVEL 5 has become available") |

Control codes inside the strings (as 16-bit units unless noted):

| Bytes | Meaning | Old tools' text form |
|---|---|---|
| `FF FE` | Start color 1 (character names) | `$\` |
| `FF D6` | Start color 2 (emphasis) | `*\` |
| `FF FF` | Start color 3 (team names) | `+\` |
| `FF FC` | End color | `\*` |
| `FF D1` | Next page of a conversation | newline + `\>` |
| `00 20 00 00 00` | End of a ticker reel (the scrolling garage text); required | `\\` |
| `00 2E 00 00 00` | End of a character bio | `\.\` |
| `30 00 30 00` | A number filled in at run time (times, stats) | `#\` |

## Parameter tables (BUILD 72-83)

Loaded together at boot by `sub_820CC048`. Values are little-endian.

### 72: car specs (partly confirmed)

72 cars x 8,432 bytes (607,104 bytes), in the car id order below. Every
block of a record starts with the car's id (8 ASCII bytes padded with
spaces), and the id appears nowhere else, which makes the layout easy to
check (confirmed for all 72). `mod_tools/car_specs.py` exports the records
to an Excel workbook (one sheet per car; fields not worked out are
UNLABELED) and builds them back.

"Checked" below means the field matched the in-game spec sheets of seven
cars (RX7, Z33, SKY34, GC8, CEL, ECLI, T86) and holds for all 72 records.

| Offset | Size | What |
|---|---|---|
| 0 | 640 | Block 1: the car's physics, mostly floats, mostly not worked out. |
| 640 | 340 | Block 2: the spec sheet the menus show, and the car's lists and options. |
| 980 | 16 x 204 | Engine stages. |
| 4244 | 42 x 92 | Tires: 21 front, then 21 rear. |
| 8108 | 272 | Paint: the id, 13 slots of 20 bytes, then the number of paints (u32). |
| 8380 | 52 | The stock wheel: a 48-byte record in table 73's format (starting with the car's id), then ASCII `0000` in all 72. |

#### Block 1

| Offset | Type | What |
|---|---|---|
| +0x20, +0x24, +0x28 | f32 | Length, height and width (m). |
| +0x40-0x58 | 2 x f32 x 4 | A box: min x, y, z at +0x40, +0x44, +0x48, max at +0x50, +0x54, +0x58 (w unused). Centered on the car, y 0 to the height, as wide as the car, and 0.10-0.16 m shorter than its length (all 72): probably the collision box. |
| +0x174 | f32 | Final drive: RX7 4.300, the real FD's. |
| +0x180 | f32 | Reverse gear, negative (RX7 -3.483). |
| +0x184-0x198 | 6 x f32 | Forward gear ratios, 0 after the last: RX7 3.483, 2.015, 1.391, 1.000, 0.762, the real FD's. The count matches block 2's gears (+100) in 68 of 72 (not the ECLI, LEAD04, WON04 or QUON). |
| +532 | f32 | Mass (kg) the physics uses: about 7-10% below the spec sheet's (Z33 1380, sheet 1490; RX7 1173, sheet 1280). |
| +536 | f32 | Wheelbase (m): RX7 2.425, Z33 2.65, CEL 2.925, the real cars'. |
| +540, +544 | f32 | Front and rear track (m): RX7 1.46 and 1.46, the real FD's. |

The dimensions match block 2's millimeters on most cars, but not on 34
values, mostly rivals' (the L86/T86 are 1.645 m wide here, 1625 mm in
block 2). BOSS04-06's block-2 length and width look shifted by one rival
(BOSS05's block 2 has BOSS04's block-1 width, and so on). The code
confirms the split: the vehicle setup (`sub_820C2608`) takes the
dimensions, mass, wheelbase and tracks from block 1, and block 2 is only
read for the menus and a few settings (below).

#### Block 2 (offsets from 640)

| Offset | Type | What |
|---|---|---|
| +8 | u32 | Displacement (cc). Checked. |
| +12, +16 | f32 | Max power (PS) and its rpm. Checked. |
| +20, +24 | f32 | Max torque (kgm) and its rpm. Checked. |
| +28 | f32 | -0.0 in all 72. |
| +68 | u8 | Engine type: 1 L4, 2 L6, 3 H4, 5 V6, 6 V8, 7 rotary. Checked; 0 and 4 are never used. The code turns it into a cylinder count (4, 6, 4, 6, 8, 2 rotors; 0 is 3 and 4 is 6) for the tuning (`sub_820C0B48`). |
| +69 | u8 | Aspiration: 0 NA, 1 turbo. Checked; the game has no supercharged cars. |
| +70, +71 | u8 | 0 in all 72. |
| +72 | u32 | Mass (kg). Checked. |
| +76, +80, +84 | u32 | Length, height, width (mm). Checked. |
| +88 | u32 | Model year (1969-2006). |
| +92 | u32 | Doors: 2 or 4 for road cars and rivals; traffic also has 1 (the bus) and 5. |
| +96 | u32 | 0 or 1 for road cars and rivals, 0-3 for traffic. Not worked out. |
| +100 | u32 | Gears (4-6): matches the number of ratios in block 1 (+0x184) in 68 of 72. |
| +104 | u32 | Price; 999999 for cars not sold: the cut cars (ECLI, S30, L86, T86), the unused BOSS slots and all traffic. |
| +112, +116, +120 | u32 | 0 or 1. Not worked out. |
| +124 | u32 | 0x1FFF or 0x1FDF (bit 5 clear on the RX7, N180, S30, T86, SUPRA70 and some rivals). Probably a mask; not worked out. |
| +128 | u8 | 0-12. Not worked out. |
| +129 | u8 | Maker: 3 Mazda, 4 Mitsubishi, 5 Nissan, 6 Subaru, 8 Toyota; 0 for CON_X, rivals and traffic. |
| +130 | u8 | Drive type: 0 FF, 1 FR, 4 4WD. Checked; the game has no mid- or rear-engined cars. |
| +131 | u8 | Car list: 0 road cars, 1 CON_X and rivals (the two lists the starter cars and shop build, `docs/functions.md`), 2 traffic. |
| +132-296 | u32 | Small counts (0-6), set only for rivals (all 0 for road cars and traffic). Probably the rival's tuning; not worked out. |
| +300, +308 | f32 | Equal, 40-100, road cars only. Not worked out. |
| +304, +312 | f32 | 50, road cars only. |
| +316 | 24 bytes | Model name, NUL-padded ASCII. Only 7 traffic cars have one (`MARK 2 TOURER V`, `VITZ RS`, `LEGACY TOURING WAGON` ...); the rest have `-` or spaces. |

The other words of block 2 are 0 in all 72.

#### Engine stages (204 bytes each)

| Offset | Type | What |
|---|---|---|
| +8 | u32 | 0. |
| +12, +16 | f32 | 12800 (the end of the torque curve's rpm range). |
| +20 | f32 | Rev limit (rpm). |
| +24 | f32 | Torque, kgm x 10. |
| +28 | 32 x f32 | Torque curve, relative to +24: point k is at k x 400 rpm, the highest point is exactly 1, unused points are 0. |
| +156, +160, +164 | f32 | 0.0475, 0.9, 1 in all stages. |
| +168 | f32 | A fixed fraction of +24 within each car (the same ratio in all its stages 1-10; Z33: 0.1821 to 0.2559 as torque goes 356.9 to 501.6). |
| +172 | u32 | Turbo: 1 for a turbo stage. |
| +176, +180, +184 | f32 | Turbo settings: on a turbo stage +176 x +180 = 3.75 exactly; on an NA stage 2.5, 0, 10. |
| +188, +196 | f32 | 0.22, 10 in all stages. |
| +200 | 4 bytes | First byte 1 on NA stages, 2 on turbo stages; the second byte changes where the rev limit does (Z33: 4, 7, 10). Probably the engine sound; not worked out. |

Stages 1-10 are the 10 engine options. In every car the rev limit goes up
500 rpm at stages 4 and 8, and the Z33's stage 2 has 13.5 more torque
than stage 1 (the game shows +1.4 kgm for the first engine upgrade).
Stages 11-15 have real torque only on ten NA cars (ECLI, CIMA, Z33, S30,
SKY35, CEL, L86, CROWN, T86, WON06), with turbo settings: probably turbo
kits. On every other car, stage 11's torque is about a tenth of stage
10's and falls toward 0 by stage 15 (unused). The turbo flag is set on
stages 1-12 of turbo cars and from stage 11 on NA cars (11-15 on those
ten, 11-12 or only 11 on the rest); the LORRY is the one turbo car with
NA stages 1-10.
Stage 16 is byte for byte the same in every car: rev limit 8500, a flat
curve of 11.8.

Power isn't stored. Torque (kgm) at a curve point is +24 / 10 x the
point, and power (PS) is that x rpm / 716.2. For the Z33's stage 1 that
gives 35.69 kgm and 293.9 PS at 6400 rpm (its spec sheet: 35.7 kgm, 294
PS at 6400), the GC8 280.5 PS (sheet 280). The curve's torque and power
peaks land within one point of the sheet's rpm in all seven cars. The
other NA cars come within a few percent; the turbo cars compute higher
than their sheets (SKY34 335 PS, sheet 280), so the sheet's numbers are
typed in. The in-game power/torque graph is drawn as a line at any
resolution, so it's probably drawn from the curve.

#### Tires (92 bytes each)

| Offset | Type | What |
|---|---|---|
| +8-40 | f32 | 99750, 10000, 0.1, 0, 70000, 10000, 0.1, 0, 0 in all tires. |
| +44 | f32 | Radius (m): rim diameter / 2 + width x aspect ratio, exactly, in all 3,024 tires. |
| +48 | f32 | Width (m). |
| +52 | f32 | 25.5-27.5. Not worked out. |
| +56, +60 | f32 | 1, 0.2 in all tires. |
| +64, +68 | f32 | Equal in every tire; probably grip. |
| +72 | f32 | 1.2 in all tires. |
| +76 | f32 | About 5000-7000, larger for wider tires (Z33: 5876 front, 6790 rear). Not worked out. |
| +80 | f32 | Rim diameter (in). |
| +84 | f32 | Aspect ratio. |
| +88 | f32 | Rim width (in). |

The size fields are the same in all 21 tires of a side. The Z33 has
225/45R18 on 7.5J front and 245/45R18 on 8.5J rear (the game shows 18-inch
wheels); the RX7 235/45R17 and 255/40R17. Grip runs in four groups (tires
1-6, 7-11, 12-16, 17-21) on the 39 road and traffic cars; the Z33's
front: 1.05 up to 1.512, 1.155-1.47, 1.113-1.428, 1.082-1.207. On rivals
it never steps down.

The 21 are the stock tire and 4 tire types x 5 grades:
`CarSpec::GetTire` (`sub_820C96C8`) picks tire 0 for type 0, else
(type - 1) x 5 + grade, from the front or rear 21. On the road and
traffic cars, grip is the stock grip times a fixed factor: type 1 1.14,
1.24, 1.34, 1.44, 1.44; type 2 1.10-1.40; type 3 1.06-1.36; type 4 1.03,
1.06, 1.09, 1.12, 1.15 (the RX7 has no type 4: zeros). Their stock grip
is 0.525 x (1 + 3.67 x sqrt(radius x width)) to within 0.2%; the rivals'
is set by hand (1.0-1.6; Devil Z 1.6 front, 1.5 rear).

`Tire::InchUp` (`sub_820C44C8`) changes a tire for bigger rims, by n
inches: the rim diameter grows by n, the width by 5% an inch (rounded
down to 5 mm), the aspect ratio keeps the outer diameter, the rim width
becomes 3.5 + (width mm - 135) / 20 inches (rounded down to 0.5; 0.5 less
under 175 mm), and grip scales with 1 + 3.67 x sqrt(radius x width).
How many inch-ups the wheel shop offers is fixed per car id in the code
(`sub_821ED5D8`): 20 minus the stock rim diameter, at most 4, and 0 for
the cut cars (the recomp gives them 3 and 4, `--cut_car_wheels`).

#### Paint (20-byte slots)

```
+0   u16  type: 0 solid, 1 metallic, 2 (only rivals)
+2   2 bytes of zero
+4   u16 hue (0-359), u8 saturation, u8 value: the color
+8   a second color, same layout: only the SUPRA70 and one WON03 slot
+12  the paint's second shade, same layout
+16  a fourth color field, zero in all 72
```

The swap after loading (`sub_820CB1F8`) fills in the second shade at
+12 for types 0 and 1: solid gets hue 0, saturation 0, value 25;
metallic gets the color's hue, 60% of its saturation and value 204. Type
2 keeps the shade from the data, so rivals' paints set their own (BOSS02:
234/92/64 with 208/230/178); probably a pearl. Slots after the paint count
are all zero. The Z33's seven, in the order
silver, white, gray, black, red, blue, orange: `1, 0, 0, 75`;
`0, 55, 20, 220`; `1, 290, 10, 30`; `0, 0, 0, 25`; `0, 0, 245, 130`;
`1, 240, 245, 60`; `1, 27, 250, 80`. Most cars (53) have one paint; the
RX7 has 5.

Compared with Import Tuner Challenge's copy, only the prices seemed to
differ (old notes).

Car ids, in file order:

```
 0 RX7      1 RX8      2 CON_X    3 ECLI     4 EVO9     5 N180
 6 CIMA     7 Z33      8 Z32      9 S30     10 SIL15   11 SKY35
12 SKY34   13 SKY32   14 IMP_GDB 15 IMP_GC8 16 ARISTO  17 CEL
18 L86     19 CROWN   20 T86     21 SUPRA80 22 SUPRA70 23-46 BOSS00-BOSS23
47 LEAD01  48 LEAD02  49 LEAD04  50 LEAD06  51 WON01   52 WON02
53 WON03   54 WON04   55 MK100   56 MK110   57 VITZ    58 TOWN
59 ESTIMA  60 CEFIRO  61 AD      62 MARCH   63 REGACY  64 TRACK01
65 TRACK02 66 BUS     67 LORRY   68 TRAILER 69 FORESTER 70 QUON
71 WON06
```

The rival cars, by id: BOSS00 Devil Z, BOSS01 Snake Eyes, BOSS02 Emperor,
BOSS03 Crimson Devil, BOSS04 White Charisma, BOSS05 Melancholy Angel,
BOSS06 The Rook, BOSS07 The Bishop, BOSS08 The Knight, BOSS09 Blood
Hound, BOSS10 Midnight Cinderella, BOSS11 Silvery Nobleman, BOSS20 Skull
Bullet (BOSS12-19 and 21-23 are unused), LEAD01 Rolling Guy No. 1, LEAD02
Rolling Gal No. 1, LEAD04 White Fairy, LEAD06 Naniwa's Warship, WON01
Death Driver, WON02 Matterhorn, WON03 Rolling Master, WON04 Gold License,
WON06 Stalking Hammer. `CON_X` is the player's Skull Bullet. 55-70
(MK100 to QUON) are traffic.

### How the game uses the car specs (from the code)

The parameter tables are reached through the object at `[0x826B2B28]`:
+8 table 73, +12 table 72, +16 to +48 tables 74-82, +56 the number of
cars (72). `Params::GetCarSpec` (`sub_820C9C38`) returns a car's record.
Table 72 is byte-swapped once after loading; 74-82 are swapped as each
record is copied (`sub_820CB468`).

A car in play (probably every garage car, rival and race car, from
the callers) is a car object of about 3,880 bytes: +12 the car id, +1644 the tuning (below),
then copies of its spec. `Car::Refresh` (`sub_820C47A8`, 50+ callers)
rebuilds the copies from the car id and the tuning:

| Car object | From |
|---|---|
| +1888 | Block 2 (340 bytes) |
| +2240 | Block 1 (640 bytes) |
| +2880 | The engine stage selected by +1652 |
| +3084, +3176 | The front and rear tire (type and grade at +1708/+1712 and +1716/+1720), then inched up by +1728/+1736 |
| +3268, +3316 | The front and rear wheel: the spec's stock wheel, or table 73's wheel number +1724/+1732 |
| +3364 | Paint (272 bytes) |
| +3636 to +3875 | The aero parts (tables 74-82) |
| +3876, +3880 | How much the inch-up widened the front and rear tires |

The tuning at +1644 holds a level per part: +1652 the engine stage, +1660
to +1704 the power and drive-train parts (7-option categories), +1708 to
+1736 tires and wheels, +1740 to +1780 the aero parts (below).

`Vehicle::SetupFromCar` (`sub_820C2608`) copies the physics values from
block 1, the engine stage (rev limit, torque, +156-196) and the tires into
the vehicle; `Vehicle::ApplyTuning` (`sub_820C0B48`) then applies the
parts. The power and drive-train parts aren't in the data: their effects
are tables of multipliers built in the code (7 entries each, separate ones
for NA and turbo cars), and several scale block 1's mass, so a part's
weight change is probably a fraction of the car's mass. That would explain
why the weight changes (Z33: cooling -9 kg, body -69 kg) differ between
cars; the Z33's brake prices (298 and 894, 10% and 30% of its 2980)
suggest prices work the same way (the price code isn't traced).

### 73: wheels

8,544 bytes: 178 records of 48 bytes, each starting with an 8-byte id
(`AD00` ..., probably Advan; `TA02`, `GL00`, `GRP02` ...). The swap
(`sub_820CB328`) shows the layout: +8 and +20 two 12-byte color records
(each two colors laid out like paint's, then a word), then four words at
+32-44; their meaning isn't worked out. These are the aftermarket wheels: `Params::GetWheel`
(`sub_820C9C50`) returns record n - 1 for n >= 1 (0 is the stock wheel,
which each car's spec holds at +8380 in the same format).

### 74-82: aero parts

Per road car (23 records each; other cars use car 0's), reached through
`Params::GetCarParts` (`sub_820C9BA0`). Each part level is a 28-byte record
(36 in 81 and 82), level n = record n - 1, level 0 = no part:

```
+0, +4    f32  front, rear weight change (probably kg)
+8, +12   f32  front, rear lift change (front bumper -0.15 front, spoiler -0.1 rear)
+16, +20  f32  two multipliers (0.95-0.99)
+24       u32  a flag (set on the Z33's rear spoilers 3 and 4)
+28, +32  f32  81 and 82 only: front, rear widening (probably mm)
```

| Table | Records a car | Tuning level | Part (Z33 options, stock included) |
|---|---|---|---|
| 74 | 10 | +1740 | Front bumper (5) |
| 75 | 10 | +1744 | Side skirt (5) |
| 76 | 10 | +1748 | Rear bumper (5) |
| 77 | 10 | +1752 | Rear spoiler (5) |
| 78, 79 | 6 | +1756 | Bonnet (5); +1760 picks 79, 2 kg lighter (probably carbon) |
| 80 | 3 | +1764 | Mirror (4) |
| 81 | 3 | +1776 | Over fender (4): 16/10, 10/19, 20/10 |
| 82 | 2 | +1780 | Body kit (2; 3 on the SKY34): 20/20 |

The head and tail lights (+1768, +1772) have no table. Which table is
which part is inferred: the tuning levels are in the order of the game's
aero menu, the record counts fit the Z33's options, and the Z33 has data
in the first 4 records of each 10.

### 83 (inferred)

920 bytes: 230 floats, all 1.0 in this copy. Not worked out.

## Other formats

| Format | Magic | Notes |
|---|---|---|
| XPR2 (`.xpr`) | `XPR2` | Xbox packed resource (the XDK's format, not Genki's); here only bitmaps. Header (BE): magic, header size, data size, texture count; then 16-byte `TX2D` records; then names. Formats seen: `0x52` DXT1, `0x53` DXT3, `0x54` DXT5, `0x71` DXT5 packed normal map, `0x7C` DXT1 packed normal map, `0x86` A8R8G8B8. Texture data is tiled. Rebuilt with the XDK's `Bundler`/`UnBundler` from an `.rdf` (XML) list. How the game loads them: below. |
| XMD (`.xmd`) | `XMD\0` | Menu and HUD screens; the bitmaps for a screen are XPR2 nested inside. |
| XMDL (`.xmdl`) | `0IVX00.1` | Models (name from GUT Archive Tools). |
| XFN (`.xfn`) | `FF AA FF AA` | Probably fonts (name only). |
| XACT | `.xgs`, `.xsb` (`KBDS`), `.xwb` (`DNBW`) | Audio: global settings, sound banks, wave banks. See `docs/audio.md`. |
| XPU (`.xpu`) | `10 2A 0E 00` (pixel), `10 2A 0E 01` (vertex) | Compiled shaders; see "Shaders" below. GUT Archive Tools names both kinds `.xpu`. |
| Loose TGA | `TRUEVISION-XFILE` footer | Car decals are plain TGA. |

Bitmaps seen in the old research (BUILD ids): 22070 matches the `TSPFONT`
font sheet by size (576x928 A8R8G8B8 XPR2, 2,140,172 bytes); the old
notes put the car shop pictures, the Genki load splashes, the race HUD (exit
signs, tachometers) and the rival names in kanji in XMDs between 22554
and 22634, but the exact ids need checking against this copy.

### XPR2 loading (confirmed)

`XPR::Create` (`sub_8257C880`) takes the file as loaded:

1. Checks the magic `XPR2`; otherwise prints `Invalid Xbox Packed
   Resource` and fails.
2. Copies the header (header-size bytes from +12) into normal memory and
   the data block (data-size bytes after the header) into physical (GPU)
   memory (`Memory::AllocPhysical`; it prints `Xpr::Create XPhysicalAlloc Error` if
   that fails).
3. Turns each 16-byte record's name offset into a pointer and, for every
   record whose type starts with `TX`, adds the data block's address to
   the texture header's base-address field (`TextureHeader::Relocate`:
   header +0x20, and the mip address at +0x30 when the texture has mips).

It never checks or converts a texture's format, size or tiling: the GPU
gets the header's fetch constant as stored. A replacement texture works
only if its header describes its data exactly (format, size, pitch,
tiling, endian swap, mip count). The loader itself doesn't refuse a
format change, e.g. DXT5 to A8R8G8B8; such a texture is four times
larger, which matters where the shared load buffer is used.

Lookups: `XPR::GetTexture` (`sub_8257CA30`) returns texture n's header,
and **an index past the end returns texture 0**, silently;
`XPR::FindTexture` (`sub_8257C7A0`) finds one by name.

Models hold a list of XPR2s (`XPRList`, in `XLRes_Tex.cpp`; growing it
past its reserved size asserts with `XPR LIST RESERVE !!!`).
`Model::Load` (`sub_8257C238`) parses the mesh file, creates the
model's own XPR2 if one is given, and puts it at index 0. Each draw's
material record (0x200 bytes) names its textures (`Model::RecordShape`,
`sub_8257AE50`, confirmed against the RX7's meshes):

```
+0x88  number of textures bound (0-2 on the RX7)
+0x98  8 x s32: for each, the index of the XPR2 in the model's list
+0xB8  8 x s32: for each, the texture's index in that XPR2 (-1 = none)
```

### Car decals (TGA) and the vinyl sheet (confirmed)

The vinyls and stickers (BUILD 145-580) are plain TGA files that the game
reads itself, and draws into each car's vinyl sheet **on the CPU**:

1. `CarModel::LoadDecals` (`sub_820D57F8`) loads the car's 6 vinyl layers
   (layer records at car model +208, 64 bytes each; id = the vinyl's
   index + 162 for layers 0-2, + 188 for layers 3-5) and 48 stickers (8
   groups of 6 records, from car model +592; id = index + 382, or the
   record's absolute id when its type at +8 is 9, the team emblem). Each
   goes into the shared load buffer.
2. As each one arrives, `Vinyl::DrawDecals` (`sub_820D3D18`) decodes it
   (`TGA::CreateImage`):
   - `TGA::ParseHeader` reads the 18-byte header, swapping the 16-bit
     fields from little-endian.
   - `TGA::Decode` accepts 24- or 32-bit images only. Image type 2
     (uncompressed) is copied, type 10 (RLE) decoded; any other type
     prints an error ("unsupported data type", in Japanese) and fails.
     **Pixels are always read from byte 18**: the ID length and the
     color map are ignored, so a TGA with an ID string or a color map
     loads shifted.
   - `TGA::ToARGB` flips the rows to top-down and makes 32-bit ARGB
     (24-bit pixels get alpha 255).
3. `Vinyl::DrawDecal` (`sub_820EE4A8`) draws it as a quad into the
   sheet, an A8R8G8B8 canvas made at load (`Texture::Create`; texture
   and pixels at car model +6684 and +6688). The quad's size is the
   **TGA's own width and height in pixels**, centered, then rotated,
   scaled and moved by the layer or sticker record, so a TGA twice the
   size draws twice as large. A software rasterizer fills it
   (`SoftRaster::DrawTriangle` → `DrawSpan` → `BlendPixel`), blending
   each pixel as (src x a + dst x (256 - a)) >> 8.
4. When every slot is done, `Vinyl::ClearCorner` clears the 16 x 16
   block at x 0-15, y 1008-1023, and `Vinyl::Compress` (`sub_820ECCC8`)
   compresses the sheet to a DXT5 texture (`0x1A200054`), which replaces
   the canvas at car model +6684.
5. `CarModel::SetVinylTextures` hands it to the car's six models
   (`Model::SetVinylTextures`: model +444). `Model::RecordMaterials`
   (`sub_820CEA60`) binds it in place of the material's own texture for
   materials whose shared shader pair is 1000 or 1004: the paint. The
   code reads +392 from a pointer to the material record's +8, which is
   the record's +0x190 ("Part files"); the value is in the files (1000
   on the RX7's `vinyl_emp` draws). 1001-1003 take their own branches
   (not traced).

Where each decal lands is a region of the sheet, in pixels, from a table
at `0x825F2A00` (x0, y0, x1, y1):

| Region | Rectangle | Region | Rectangle |
|---|---|---|---|
| 0 | 0,0 - 1024,512 | 5 | 0,640 - 256,832 |
| 1 | 0,0 - 1024,256 | 6 | 0,832 - 256,1024 |
| 2 | 0,256 - 1024,512 | 7 | 256,640 - 640,1024 |
| 3 | 0,512 - 512,640 | 8 | 640,640 - 1024,1024 |
| 4 | 512,512 - 1024,640 | 9 | 0,0 - 1024,1024 |

Vinyl layers 0-2 use one region each; layers 3-5 are drawn twice, into
regions 1 and 2 (probably the car's two sides, inferred). A vinyl that is
exactly 1024 x 1024 is drawn as the whole sheet (region 9). Stickers take
their region from their record (+8).

The sheet's size is data, `1024, 1024, 16, 16` at `0x825F2520`, not a
constant in the code, and the models sample it with 0-1 UVs. A larger
sheet for HD decals would also need the pixel-based parts scaled: the
quad size in `Vinyl::DrawDecal`, the region table, the 1024 x 1024 check
in `Vinyl::DrawDecals` and the cleared corner (inferred).

### Shaders (confirmed)

Each shader is one archive entry, big-endian:

```
+0   u32  magic: 0x102A0E00 pixel shader, 0x102A0E01 vertex shader
+4   u32  header size = offset of the microcode
+8   u32  microcode size
...       constant table: names like `matWVPVS`, `PS_CONSTANT_PARAM_0`,
          then the target (`vs_3_0`/`ps_3_0`) and "Microsoft (R) Xbox
          Shader Compiler"
+hdr      the microcode
```

`Shaders::LoadAll` (`sub_820A0A88`) loads them in pairs at boot, a
vertex shader (odd id) and its pixel shader (the next id):

| Ids | Pair indices | What |
|---|---|---|
| BUILD 3-70 | 0-33 | Post-processing and effects |
| BUILD 22800-22811 | 1000-1005 | Material shaders (lighting: `DLightDir`, `PntLightPos`, `EyePos`, ...) |
| BUILD_CRS 24256-24367 | 2000-2055 | The course's shaders |

Hashing the microcode with XXH3-64 (the SDK's shader dump names) gives
the same hashes ITC-Recomp found for these effects at run time, so the
shaders ITC-Recomp has named are identical here:

| Id | Hash | What |
|---|---|---|
| 8, 70 | `7EB16456AAD56CBA` | Motion blur (pixel; `PS_CONSTANT_PARAM_POSTBLUR`, `_POSTBLUR2`). Velocities from its vertex shader, 9 or 67 (`matOldWVPVS`, `matWVPVS`: this and the previous view-projection; c100 is the previous, see `View::UploadPreviousViewProjection`). |
| 12, 18, 20 | `E0D070FB58A4EA03` | The glow history's cross-fade (pixel, no constants; the weight is the quad's vertex alpha). |
| 40 | `9750599DF5ED3B39` | The bloom combine (pixel; `PS_CONSTANT_PARAM_0`, `_1`). |
| 38 | `61DB6B3F91E0ED87` | A blur with `g_avSampleOffsets`, `g_avSampleWeights` (the DirectX sample's names). |
| 24-32 | | Filters with `PS_CONSTANT_PARAM_0`, `PS_CONSTANT_TEX_SIZE` (bloom chain, likely). |
| 64 | `D8928E2407A68256` | Depth of field (`PS_CONSTANT_PARAM_DOF`, `_CAMERA`). |
| 66 | `C5EDA6DC536A2095` | Final color: gamma, HSV scale, calibration (`PS_CONSTANT_PARAM_GAMMA`, `_HSV_SCALE`, `_CARIBRATE`). |
| 16 | `6E9AB15C8DAAAB83` | Lighting (`DLightDir`, `DLightSpecularColor`, `EyePos`, ...). |

`DATA/SHADERS/` on the disc holds 118 loose shaders with names
(`BloomPS.xpu`, `DofLv1PS.xpu`, `SYSTEM_SHADER_DOF.xvu`, ...), but only
`z_test_vs.xvu` matches an archive shader (BUILD 3): they're an older set,
and the game loads its shaders from the archive.

### Models

A model is an XPR2 with its textures, one node file, and one or more part
files (mesh files). The small models are containers (`.dat`) holding
them; the cars are runs of ids instead (see "Car models" below). All
big-endian.

- **Node file**, starting `00 02 00 00`: the model's nodes (parts or
  bones). See "Node files" below.
- **Part file**, starting `00 15 00 00`: shapes named after their Maya
  shape nodes (`fender`, `fn00punch_Shape`, `polySurfaceShape3`), with
  their vertex data, materials and shaders. These are the
  `polySurfaceShape` files; the game's models were exported from Maya
  with the shape names kept. `Mesh::Parse` (`sub_8257BD58`) checks the
  first word is `00 15 00 00` (otherwise `Version Error %x`) and loads the
  file's shaders (`Mesh::LoadShaders`). See "Part files" below.

| Id | Contents |
|---|---|
| 22532 | A car: textures `advan_bd`, `advan_pun`, `ad_eff`, `ad_ex`, `ad_num` (number plate), `BIGB`/`BIGD`/`BIGF`/`BIGL`/`BIGR`/`BIGT`, `whites`; 20 nodes (`door`, `engine_room`, `fender`, `muffler`, `neon`, `room`, `interior`, `tire_pos`, `tire_fl`, `tire_fr`, ...); 6 meshes. The `ad` prefix matches the `AD` car id (the Nissan AD, a traffic van), so `advan` is probably "AD van". |
| 23006 | The omamori (the charm hanging in the car): an XPR2 with `omamori` to `omamori04`, 12 nodes (`root`, `model`, `bone`, `A_yure`, `A_yure_00` ...; *yure* = sway), and one mesh, `polySurfaceShape3`. |

#### Node files (`00 02 00 00`, partly confirmed)

Big-endian. Header:

```
+0x00  00 02 00 00
+0x04  node count n
+0x08  0 for the cars (8 in the omamori)
+0x0C  n (4 in the omamori)
+0x10  offset of the nodes (0x40)
+0x14  offset of n x float4: local translations, w = 1
+0x18  offset of n x float4: rotations (quaternions; 0, 0, 0, 1 in all the RX7's nodes)
+0x1C  offset of n x float4: scales (1, 1, 1, 1)
+0x20  offset of n x 4x4: world matrices, the bind pose (row vectors, translation in the last row)
+0x24  offset of n x 4x4: identity in all the RX7's nodes
+0x28  offset of n x 4x4: inverses of the world matrices (inverse bind matrices)
+0x2C  offset of a 16-byte tail
+0x30  end of the file
+0x34  12 bytes of leftover text from the exporter's memory
```

Each node is 160 bytes: the name from +0 (zero-padded up to +0x5C), then
mostly -1 and 0, with the parent's index at +0x80 (-1 for the root) and
the depth at +0x98 (0 for the root). In the RX7's three node files the
only nodes away from the origin are the tires, the seats (`seat_L`,
`seat_R`), the door hinges (`dl00_rot_*`, `dr00_rot_*`), the bonnet
hinge (`bn_NN`) and `acce_null`; the translations are in the node frame
described under "Car models". The translation, quaternion, scale and
matrix arrays were checked against each other (confirmed). The
translations are local to the parent and the +0x20 matrices are world
matrices, the parents included: the omamori's bone chain shows it
(`A_yure_01` has translation y -0.317 and world y -0.565, its parent
-0.248), where the car nodes can't, since their parents sit at the origin
(confirmed). What the second matrix array is for isn't known.

**Markers.** Most of a car's nodes sit at the origin and only name an
attach point (`room`, `steering`, `mainmeter`, `addmeter`, `roll_tower`,
`fb`, `rb` ...): those parts are modeled in place. The ones with a
position, across the 45 set-a node files (positions in the node frame,
see "Car models"):

| Node | What | Cars |
|---|---|---|
| `tire_fl`, `tire_fr`, `tire_rl`, `tire_rr` | Wheel centers | All 45 |
| `seat_L`, `seat_R` | Seat positions | 42 have them, 39 placed |
| `acce_null` | The accessory (the charm) | 41 have it, 36 placed |
| `dlNN_rot_00`/`01`, `drNN_rot_00`/`01` | Two points per door variant: the hinge axis | 43 have them, 32 placed |
| `bn_NN` | Bonnet hinge, one per bonnet variant | 40 have `bn_00`, 32 placed |
| `sm_L_NN`, `sm_R_NN` | Mirrors | Placed only on the N180 |

The car loaders (`sub_820D4448`, `sub_820D4C28`) find the wheel, seat and
accessory nodes by name and copy their matrices into the car model:
`tire_fl`/`fr`/`rl`/`rr` to +6720, +6784, +6848, +6912, `seat_L`/`seat_R`
to +7040/+7104, and a pointer to `acce_null`'s at +7200 (confirmed). The
door and bonnet hinges presumably drive the garage's opening doors and
bonnet (not traced).

The ECLI (a cut car) has no `seat_L`/`seat_R`, `bn_00`, `addmeter` or
`roll_tower`, and its door hinges sit at the origin; the WON04 and LEAD04
rivals have the same gaps, the same filled part slots and the ECLI's gear
ratios, so they probably share its model. Its set-a block also has no
bonnet, engine bay, meters or roll cage parts, and its door parts are
placeholders: two hinge nodes each, no shapes (1,568 bytes). The doors
are part of the body, a single 25,318-triangle shape, which is why they
can't open in the garage. The ECLI's broken garage camera when choosing
interior parts, which stays on the current camera, may come from the
missing seat nodes (inferred; the camera code isn't traced). No node in any car is a camera: the cameras' positions
aren't in the node files, the parts or the car specs (block 1), and
where they are isn't known yet.

#### Part files (`00 15 00 00`, confirmed against the data)

A part is a small model: its own node list (the first node names what it
attaches to: `fb`, `rb`, `door_L_00`, `engine_room`, `room`, `fender`),
the shapes, and the materials with their shaders. Big-endian.
`mod_tools/blender_plugin/sbx_formats` reads and writes them (every byte
kept, so an untouched part writes back identical), and the Blender
add-on there imports and exports them. All 6,430 part files in BUILD
read, and each one writes back byte for byte (`blender_plugin/tests`).

```
+0x00  00 15 00 00
+0x04  0
+0x08  node count
+0x0C  shape count
+0x10  0
+0x14  offset of the nodes (0x50), 192 bytes each, name first
+0x18  0x50 as well
+0x1C  offset of the shapes, 0x220 bytes each
+0x20  offset of the first material block
+0x24  offset of an embedded node file (00 02 00 00 in every part but
       BUILD 141, where it starts 00 01 00 00 and isn't a node file; also
       00 01 00 00 in the omamori)
+0x28  6 offsets of small sections, 0x20 bytes each, starting 00 01 00 00;
       usually the first two are the same offset (5 sections). Not worked out.
+0x40  offset of the vertex data (stream data, then the index buffers)
+0x44  size of the vertex data: the streams plus the index buffers
       (each padded to 4 bytes), not the padding before the shaders (all
       6,430 parts). The game uses it to copy the vertex data: an edited
       part that grew but kept the old size loaded with its mesh broken
       (confirmed in game)
+0x48  offset of the shader section (below). It's always the last section.
+0x4C  1 (0 in BUILD 141 and the omamori)
```

A shape (0x220 bytes):

```
+0x000  Maya shape name, 32 bytes
+0x020  4x4 matrix (identity in every RX7 part)
+0x060  bounding box: 8 corners, float4 each (w = 1)
+0x0E0  bounding sphere: center xyz, radius
+0x0F0  3 stream descriptors, 16 bytes each: vertex count, type, stride, size
          (count 0 = unused)
+0x120  3 x 32 bytes, one per stream: 1, 1, 0, 0, 0, FFFF0000, then the
          Xenos vertex fetch constant: (offset from the vertex data) | 3,
          size in bytes | 2 (endian swap 8in32)
+0x180  offsets of the stream 0, 1 and 2 data
+0x18C  offset of the shape's material block
+0x190  offsets of the skinned position and normal arrays (each a 16-byte
          descriptor like the ones at +0x0F0, then the data; count 0 when
          the shape isn't skinned)
+0x198  offset of the shape's skin block
+0x1BC  4
```

The bounds follow one rule in all 19,396 shapes (confirmed against the
data): the box spans the vertices **and the origin** (lo = min(0, min
of the vertices), hi = max(0, max)), as if the exporter started its
min/max at 0. Corner i has x = hi if bit 0 of i is clear (else lo),
z = hi if bit 1 is set, y = hi if bit 2 is set. The sphere is the box's
center with half its diagonal as radius (exact in 18,626 shapes, within
float rounding in the rest). The skin block's first word is the vertex
count, all of a shape's streams have that count, and a skin array, when
present, is a copy of the vertex stream of the same type (all shapes).

The streams hold one attribute group each. Types seen across all parts:

| Type | Stride | Contents |
|---|---|---|
| `0x00` | 4 | Color (4 bytes; white on the cars) |
| `0x0E` | 8 | Color, UV (half2) |
| `0x0F` | 12 | Color, UV (half2), second UV (half2) |
| `0x0B` | 12 | Position (float3) |
| `0x0C` | 8 | Normal (half4: x, y, z, 1) |
| `0x05` | 16 | Position (float4, w = 1): skinned shapes, in the skin arrays |
| `0x07` | 16 | Normal (float4, w = 1): skinned shapes, in the skin arrays |
| `0x10` | 16 | Color, UV, second UV, third UV (half2 each). The third UV equals the first in all 263 shapes (226 parts: windows and light covers, e.g. BUILD 6280), whose draws bind 3 textures, the first and third the same. Colors and UVs read like 0x0F's; re-encoded from that layout all 263 come out byte-identical (confirmed against the data). |
| `0x0D` | 16 | Normal (half4: x, y, z, 1), then a second unit half4 vector (w = 1) with only 12 distinct values, use not worked out. In the normal slot of 2 shapes, the same bonnet model in BUILD 2618 and 12585 (sets a and b of one car), with `0x0F` in the color/UV slot; the draw breaking the +0x06C rule is on it. Carried through raw. |

A static shape is a color/UV stream, a position stream and usually a
normal stream (the indicators, neon, LEDs and needles have none). UVs
use Direct3D's orientation (v = 0 at the top of the texture), the same
as glTF's.
The omamori's skinned shape has only the color/UV stream in the vertex
data; its bind-pose positions and normals are in the skin arrays. The
wheel parts (rims, brakes, tires) use the float4 types (0x05, 0x07) in
their vertex streams and repeat them in the skin arrays, but their skin
blocks are static: no bones.

The skin block: vertex count, block size, then the offsets of the
per-vertex influence counts (u32), the weights (one float4 per influence,
the weight repeated in xyz) and the bone indices (u32 per influence). A
static shape's block is 0x20 bytes with all three offsets pointing at the
block itself. The bone indices are indices into the model's node file
(the omamori uses 3-11, `root` to `A_yure_06`, up to five per vertex),
and the bind pose is that file's world matrices (confirmed: converted
with them, the omamori bends with its bones in Blender).

The tire parts (581, 583; the only parts with morphs) hold `base_Shape`
and `target_00_Shape` to `target_04_Shape`: the same vertex count, UVs,
stream layout, index buffers and material records, with the
positions moved by up to 0.11 m, so they're morph targets the game
blends between (what each target is for isn't known). In the rim and
brake parts the `_12` and `_20` shapes (`A0_12_Shape`, `A0_20_Shape`,
`B0_12_Shape` ...) are identical in positions, normals and indices;
what the suffix means isn't known.

A material block: material count n, block size (n x 0x200 + 0x10), then n
material records of 0x200 bytes from +8. One record is one draw:

```
+0x000  0-10, not worked out
+0x004  1 on blended draws: light covers, cooling ducts, glows, LEDs (inferred);
          2 and 3 on a few draws, not worked out
+0x008  float4: diffuse color (inferred; alpha 0.5 on the light covers, 0 on
          the glow overlays that are only drawn lit)
+0x018  float4: 0, 0, 0, 1 (ambient or emissive, inferred)
+0x028  float4: specular color (0.5 gray on paint and trim; inferred)
+0x058  specular power: 2 on the paint, 20 on trim, 100 on glass (inferred)
+0x05C  two floats (0.5 and 0.8 on most; 0.2 on glass), not worked out
+0x068  index of one of the part's own shader pairs (below the pair count
          at the shader section; checked on all 1,007 RX7 draws)
+0x06C  3 x the number of textures bound, in 27,400 of 27,402 draws (the
          other two, in BUILD 2618, the part with stream type 0x0D, have 1
          for one texture). Not worked out beyond that.
+0x070  0, 1, 3 or 5, not worked out
+0x074  5 words (counted over all 27,402 draws):
          +0x074, +0x07C: always equal, 0 or 1. 1 on about 10,000 of the
            11,300 blended draws. With blend 1 and diffuse alpha 0 on the
            unlit shader ps_15 (6,106 draws) they're the light glow
            planes: side markers, LEDs, indicators, whose textures have
            black backgrounds. Probably additive blending, black adding
            nothing (inferred; the code that reads it isn't traced).
          +0x078: 0.
          +0x080: -1, except 0 or 1 on the flags-5 draws (the sphere-map
            shaders ps_02 and ps_05, the wheels).
          +0x084: -1 exactly when +0x070 is 3 (all 6,071: the cube-map
            shaders ps_08, ps_14, ps_16, ps_17), otherwise 0.
+0x088  number of textures bound (see "XPR2 loading")
+0x098  8 x s32: for each, the XPR2's index in the model's list (0 on the RX7)
+0x0B8  8 x s32: for each, the texture's index in that XPR2 (-1 = none).
          Most draws bind one; 22 of the RX7's 1,007 bind two (the light
          covers and windows, e.g. `fb00cover`: `rx7_ex` and `rx7_hl_00`).
          Across all parts 0 to 3; 3 only with stream type 0x10.
          The Blender import shows the first.
+0x0D8  128 bytes: one -1 word per bound texture first (all 27,402 draws),
          the rest not worked out (mostly zero, a 1.0 float at +0x0F8)
+0x158  index count
+0x15C  primitive count (index count - 2)
+0x160  primitive type: 6, triangle strip, in every RX7 draw
+0x164  6
+0x168  0x20000002
+0x16C  1, 0, 0, 0, FFFF0000
+0x180  offset of the indices from the vertex data
+0x184  size of the indices, rounded up to 4 bytes
+0x188  offset of the indices in the file
+0x190  shared material shader pair (1000-1005, BUILD 22800-22811) or 0
+0x194  -1, then mostly zero, with a small number at +0x1A0 (0-10). It
          groups draws by kind: on the SKY35 3 is the interior-textured
          shapes, 4 the `punch` shapes (grille and mesh covers), 6 the
          opaque body and engine bay, 10 the cube-mapped trim (ps_08).
          Across all parts it's less clean (4 is also on 602 other draws),
          so probably a category or sort group; not worked out.
```

The shared pair is how the car paint works: on the RX7, 1000 is the
paint (`vinyl_emp`, replaced by the car's vinyl sheet), 1001 the bonnets
and `rx7_ccparts`, 1002 the light covers and 1003 the trim (`rx7_ex`,
carbon, engine bay, lights). Those shaders' constants (`BaseColor`,
`spheretx0`, `cubetx0`, `refmasktx0`) are a base color and sphere and
cube environment maps with a reflection mask. The interior, plates,
glows and glass have 0 and use the part's own shader (+0x068).
`Model::RecordMaterials` switches on the value (confirmed); where the
paint color and the environment maps are bound isn't traced.

Indices are big-endian u16 triangle strips; FFFF restarts the strip.
Indices start at 0 for each shape. In a strip, triangle k is
(i[k], i[k+1], i[k+2]), with the first two swapped when k is odd; that
order is counter-clockwise against the stored normals (98% or more of the
faces agree on the body parts). Degenerate triangles join strips. A few
shapes repeat a triangle or draw it both ways (36 triangles of the RX7
roll cage's `tower01_Shape`).

The shape names keep their Maya suffixes (`fb00cover_Shape`,
`polySurfaceShape3`), which is how the car part slots were identified.

The shader section (confirmed against the data):

```
+0x00  number of compiled VS/PS pairs
+0x04  the section's size less 0x20 x (pairs + 1)
+0x08  24 zero bytes
+0x20  per pair: a 0x20-byte entry, then the vertex shader, then the
       pixel shader (each as in "Shaders": magic, header size,
       microcode size ...)
         +0x00  size of the two shaders
         +0x04  vertex shader size
         +0x08  pixel shader size
         +0x0C  0
         +0x10  16 bytes not worked out (they look like leftover text from
                the exporter's memory, as in the node files)
```

Across all 6,430 parts the 14,149 pairs hold only 39 different shaders
(11 vertex, 28 pixel; compared by microcode) in 28 different pairs.
`mod_tools/blender_plugin/sbx_formats/shaders.py` names them `vs_00`-`vs_10` and
`ps_00`-`ps_27`, in order of first use over the parts by id. Each
shader's constant table names its inputs, which tell them apart: the
lighting ones (`DLightDir`, `PntLightPos`, `SpotLightDir`), specular
(`DLightSpecularColor`, `MatSpecularColor`, `EyePos`), environment maps
(`spheretx0`, `cubetx0`), textures (`tx0`-`tx2`, `sptx0`, `btx0`) and
`ModTexAlpha`, `UseProjTex`, `BaseColor`; `vs_08` has `shad`. So `ps_15`
(`ModTexAlpha`, `UseProjTex`, `tx0` only) is unlit and textured, `ps_03`
lit and textured, `ps_16` lit with specular, a cube map and a texture
(inferred from the names). The
sections differ from part to part mostly in those 16 leftover bytes. The
same section turns up at many file offsets, so it holds no file offsets.
`python -m sbx_formats.shaders BUILD_OUT -o ../../assets/shader_library.json`
(from `mod_tools/blender_plugin/`)
collects them (the library is game data, so it stays under `assets/`).

The material and shape records have more bytes than the fields above;
`mod_tools/blender_plugin/sbx_formats/layout.py` names every byte, with the ones not
worked out as `unknown_*`. Across all 27,402 draws most of those are
constant (`mod_tools/blender_plugin/tests/coverage.py` lists their values). Carried
through raw.

**Layout** (all 6,430 parts, confirmed against the data). Sections in
file order: header, nodes, shapes, material blocks (one per shape, in
shape order), skin blocks, skin arrays, embedded node file, small
sections, vertex streams, index buffers, shader section. A part without
shapes (42 of them) has only the header, nodes, embedded node file, small
sections and shaders. Each index buffer is padded to 4 bytes and the
shader section starts on 0x20; all bytes between sections are zero. A
material block ends with 8 zero bytes after its records, and a node file
embedded in a part is followed by 16 zero bytes. The shape's unused stream
slots point at the end of its used streams (fetch constant size 0). BUILD
141 (`flareShape`, a lens-flare sprite) is the exception: no nodes, and
its six small offsets point at its embedded section (which isn't a node
file) and its first stream instead of small sections.
`mod_tools/blender_plugin/sbx_formats/part_write.py` lays parts out by these rules: all
6,430 write back byte for byte, and still read correctly when every
section is moved.

**Markers.** Every marker node in the parts' embedded node files is top-level
(parent -1), and its world matrix is its rotation with its translation
as the last row, its inverse matrix that matrix's inverse (all 3,921;
602 have a rotation, the rest identity).

The part's own node list is 192-byte records at +0x14:
the name (32 bytes), then at +0x20 a type (1 for a shape's attach node,
0 for a marker), at +0x24 the index of its node in the embedded node
file, and at +0x28 the index of its shape (-1 for a marker; the shapes
aren't always in node order, e.g. 2, 0, 1 in the RX7's body part). The
positions are in the embedded node file (+0x24), in the same node frame
as the car's. The RX7's parts carry effect markers:

| Part | Markers |
|---|---|
| Mufflers | `mf00_00_ef`, `mf00_01_ef`: the exhaust tips, per muffler variant |
| Tail lights | `br00_00`-`03_ef` (brake) and `tl00_00`-`03_ef` (tail): glow points |
| Headlights | `hl00ef_h00`/`h01` (high beam), `hl00ef_l00`/`l01` (low beam), and `hl_00`, probably the pop-up lights' pivot |
| Doors | `dl00_rot_00`/`01`: the hinge, as in the car's node file |

What the game does with them isn't traced (probably the exhaust and light
effects).

### Car models: sets a, b and c (confirmed from the code)

The cars are runs of ids: textures, a node file, then the part slots.
Each car has three sets:

| Set | Body XPR2 | Node file | Stride | Loaded by |
|---|---|---|---|---|
| a | 890 + 226 x car | 892 + 226 x car | 226 | `CarModel::LoadSetA` (`sub_820D5680`), own allocations |
| b | 11060 + 197 x car | 11062 + 197 x car | 197 | `CarModel::LoadSetB` (`sub_820D5768`), shared load buffer |
| c | 19925 + 55 x car | 19927 + 55 x car | 55 | `CarInterior::LoadSetCBody` (`sub_820D6498`), shared load buffer |

(car = the car's block index, at car model +32.)

The block order isn't the car spec order (BUILD 72). Each node file's
root node names its car and set, `<code>_<set>_tr` (`rx7_a_tr`,
`sky35_b_tr`; all 134 node files in the dump), which gives the order
(confirmed against the data; the same in sets a, b and c):

```
 0 RX7      1 RX8      2 EVO9     3 CIMA     4 N180     5 SIL15
 6 SKY32    7 SKY34    8 SKY35    9 Z32     10 Z33     11 IMP_GC8
12 IMP_GDB 13 ARISTO  14 CEL     15 CROWN   16 SUPRA70 17 SUPRA80
18 CON_X   19 ECLI    20 L86     21 S30     22 T86     23 BOSS00
24 BOSS01  25 BOSS02  26 BOSS03  27 BOSS04  28 BOSS05  29 BOSS06
30 BOSS07  31 BOSS08  32 BOSS09  33 BOSS10  34 BOSS11  35 BOSS20
36 WON01   37 WON02   38 WON03   39 WON04   40 WON06   41 LEAD01
42 LEAD02  43 LEAD04  44 LEAD06
```

Several rivals reuse another car's textures (BOSS00 the S30's, WON04 and
LEAD04 the ECLI's), so texture names don't always tell. Blocks 0-17 are
the full cars (110-137 set-a parts, with every tuning part); 18-44 have
16-41. `mod_tools/blender_plugin/car_models.xlsx`
lists every block's nodes and parts.
So the SKY35's node files are 2700 (a), 12638 (b) and 20367 (c); 20367
is the one node file missing from this dump (its interior textures,
20366, are there).

- **Outside**: `CarModel::LoadExterior` (`sub_820D5B70`) loads set a
  when a mode value (the word at +8 of the object at car model +7256) is
  0, set b when it's 1, then the decals. What the two modes are in game
  (presumably garage and race) isn't traced.
- **Interior**: `CarInterior::Load` (`sub_820D7660`) loads six
  consecutive ids, the meters, roll cage, interior, gauges, seats and
  steering wheel (`CarInterior::LoadParts`): from node id + 217 (set a),
  + 188 (set b) or + 47 (set c). It uses set c when a flag at +3088 of
  its object is set, otherwise set a or b by the same mode.
- **Set c** (the cockpit view): `CarInterior::LoadSetCBody` loads the
  node file, the body XPR2 and six exterior parts, each picked by the
  car's tuning choices: bonnet (node id + 1 + choice), door (+ 14 +
  choice), mirrors (+ 17 + choice), headlights (+ 29 + choice), fenders
  (+ 34 + choice) and the cabin (+ 33).

The interior textures just before each node file (891, 11061, 19926 for
the RX7: `rx7_inte_off*` in set a, `rx7_inte_on*` in b and c, the same
six names as in `N-2`) aren't loaded by any id constant; how they're used
isn't known.

#### Part slots (confirmed against the data)

The part slots follow each node file `N`; a car without a part leaves its
slot empty, which is where all 15,039 empty entries come from (7,240 in
set a, 6,259 in set b, 1,540 in set c). Set-a slots (offset from `N`,
from the RX7 and BOSS00; part names are the Maya shape names, `NN` the
variant):

| Offset | Part |
|---|---|
| +1-13 | Front bumper `fbNN` (cover, plate, indicators, fog lamps) |
| +14-104 | `coolNN_MM`: 13 x 7 (per front bumper, 7 pieces; probably the radiator/intercooler openings) |
| +105-117 | Rear bumper `rbNN` |
| +118-130 | Side skirts `ssmgNN` |
| +131-143 | Rear spoiler `rsmgNN` |
| +144-151 | Bonnet `bnmgNN` |
| +152-157 | Doors: +152.. left (`dlmg`, windows), +155.. right |
| +158-169, +170-181 | Mirrors left, right (`smNN_L/R_MM`) |
| +182-185 | Headlights `hlNN` |
| +186-189 | Tail lights `tlNN` / brake lights `brNN` |
| +190 | Cabin `cpmg00` with windows |
| +191-196 | Fenders (the body) `fnmgNN` |
| +197-211 | Engine bay `enmgNN` |
| +212-216 | Mufflers `mfNN` |
| +217 | Meters (`d_meter`, `p_meter`, `r_meter`) |
| +218 | Roll cage (`roll`, `tower`) |
| +219 | Interior `inmg00` |
| +220 | Speedo and tacho |
| +221 | Seats |
| +222 | Steering wheels |
| +223 | Neon (`fb_led01`...) |

Variant numbers aren't contiguous: the RX7 fills fb 00-04 and 11 of the
13 front-bumper slots. For most parts the slot is the first slot plus the
variant (`fb11` at +12, `rb11` at +116, `fn11` at +195); the bonnets
don't follow it (`bnmg11` at +150).

Set-b slots (from the RX7): +1-13 front bumper, +14-104 `coolNN_MM`,
+105-117 rear bumper, +118-130 side skirts, +131-143 rear spoiler,
+144-151 bonnet, +152 door (`domg00`: both doors in one shape), +155-166
mirrors (`smNN_MM`, both sides in one shape), +167-170 headlights,
+171-174 tail lights, +175 cabin, +176-181 fenders, +182-186 mufflers,
+187 shadow (`shadow_a_00`-`02`, flat; `shadow_b_00`, a hull), +188
meters (with needles, `*_hari`), +189 roll cage, +190 interior, +191
gauges, +192 seats, +193 steering wheels, +194 neon.

Set-c slots: +1-13 bonnet, +14 door, +17-28 mirrors, +29-32 headlights,
+33 cabin, +34-46 fenders, +47 meters, +48 roll cage, +49 interior, +50
gauges, +51 seats, +52 steering wheel. These match the ids the loaders
compute (above).

Traffic models (22400-22527, 8 ids each: textures, node file, 6 parts):
+1 body (`fnmg00`, `fn00punch`), +2 windows, +3 plates, +4 interior, +5
headlights, +6 tail lights. Order: `lorry`, `track01`, `track02`, `bus`,
`ad`, `cefiro`, `march`, `quon`, `forester`, `legacy`, `estima`, `mk100`,
`mk110`, `town`, `Trailer`, `vitz`. The containers 22528-22543 hold the
same 8 files per model in the same order, byte for byte, each followed by
up to 32 bytes of zero padding.

#### Wheels (confirmed against the data)

The wheel models are parts below the first car block, three ids each:
the textures (XPR2), the rim with its wheel face, the brake and hub.
Wheel i (0-99) is XPR2 586 + 3i, rim 587 + 3i, brake 588 + 3i. Both parts
name the wheel in their first node:

- 0-19: the stock wheels of car blocks 0-19, in block order (`RX7` ...
  `ECLI`);
- 20-35: the traffic (`LORRY` ... `VITZ`);
- 36-99: the aftermarket wheels `AD00`-`AD08`, `EN00`-`EN05`,
  `TA00`-`TA09`, `VR00`-`VR07`, `GL00`-`GL05`, `VE00`, `VE01`, `TE00`,
  `GA00`, `GA01`, `TDR00`, `TDR01`, `GG00`-`GG02`, `AX00`,
  `ZIG00`-`ZIG04`, `JP00`, `WAL00`, `TOM00`-`TOM03`, `GRP00`-`GRP02`:
  the ids of table 73's wheel records.

A rim holds `A0_12_Shape`, `A0_20_Shape` (the same mesh; see "Part files"),
`NUT_n_x_Shape` (the nuts: 4 or 5) and `punch_12/20_Shape`; a brake
holds `B0`-`B2_12/20_Shape` and `B_space_Shape`. The tires are 581
(`tire_00`) and 583 (`tire_01`), the same size (0.61 m across, 0.255 m
wide), each a base and five morph targets. 886 holds brake calipers
(`c_normal_09/18`, `c_tune_09/18`); 888 is a 6 m `wheelShape`, not a car
part. Which tire each car uses isn't known.

The code picks the wheel models (`sub_820D8DD8` calls `sub_820D8860` or
`sub_820D8900`, by the word at +8 of `[car model+3268]`). The front and
rear wheel numbers are at car model+44 and +60: -1 is the stock wheel, n
the aftermarket wheel 36 + n. The XPR2 goes to +2144 (front) and +2168
(rear), the part to +1168 and +1192: the rim (587 + 3i) from
`sub_820D8860`, the brake (588 + 3i) from `sub_820D8900`. A stock wheel
is the car's block's (wheel i = block) only for blocks 0-19 (signed
compare); any other block gets AD00 (wheel 36). Traffic (car model+28 =
3) uses wheel 20 + its traffic index in `sub_820D8900`.

Cars from block 18 up never reach the stock path: when a car is set up
(`sub_820E6CA8`) with both wheel numbers -1, `sub_820D3620` gives them a
fixed wheel per block: CON_X wheel 18 (its own), BOSS00 TA03 (n = 18),
other rivals their own aftermarket wheels, and the four cut cars (blocks
19-22) AD00. So the ECLI's own stock wheel (wheel 19, 643-645) is never
used. The recomp (`src/hooks/cut_car_wheels.cpp`) keeps the ECLI at -1 so
it gets its own wheel, and gives the L86, S30 and T86 the BOSS00's wheel
(`--cut_car_wheels`), or the LORRY's steel wheels (wheel 20) through the
stock path (`--cut_car_steelies`).

#### Car space and the sets compared

All parts of a block are modeled in place: their vertices are already in
car space, so building the car needs no transforms except for the
wheels. Car space is meters with x forward, y up (0 at the bottom of the
body) and z to the car's left; the RX7 spans x -2.25 to 2.05 and z -0.88
to 0.88, its real length and width (confirmed against the data).

The wheels are placed at the node file's `tire_fl`, `tire_fr`, `tire_rl`
and `tire_rr` nodes. Node translations are in decimeters with x to the
car's left and z to the rear, so a node at (x, y, z) is at
(-z/10, y/10, x/10) in car space: the RX7's (±7.30, 1.60, ±12.12) gives
its 2.43 m wheelbase and 1.46 m track (inferred from the fit). Wheel
parts are built around their own origin facing +z, so the right-side
wheels are mirrored. Set c's node file has no tire nodes.

Textures: a car part's texture indices point into the block's body XPR2
(`N-2`), which also holds the interior textures; wheel parts use their
wheel set's XPR2 (586 for 587 and 588). Some draws bind an index one
past the body XPR2's last texture (49 on the SKY35, whose XPR2 holds
0-48), always with the shared paint shader 1000 or the light-cover
shader 1002: probably the car's paint or vinyl sheet, bound at run time
(inferred; the UVs line up with the vinyl template). Confirmed by decoding the
textures and rendering the car: the default vinyl `vinyl_emp`, a
template colored by side, lands on the matching sides.

The RX7's sets compared (counted from the data):

| Part | Set a | Set b | Set c |
|---|---|---|---|
| Body (`fnmg00`) | 4,743 triangles | 3,876 | 3,150, front half only (x > -1.14) |
| Bonnet (`bnmg00`) | 752 | 502 | 448 |
| Doors | left and right parts, 1,961 each, with hinge nodes | one part, 1,888 for both | one part, 2,538 |
| Interior (`inmg00`) | 4,207 | 2,317 | 3,617 |
| Seats | 1,152 each | 396 each | 1,152 each |
| Bumpers, skirts, spoilers, mufflers | yes | the same parts as a | none |
| Engine bay (`enmgNN`) | yes | none | none |
| Wheels (tire nodes) | yes | yes | no tire nodes |
| Shadow | none | yes | none |
| First part of each group, with rims | 49,516 triangles | 38,340 | 15,503 |

So the sets aren't three detail levels of one model. **a** is the full
car (separate opening doors and bonnet, engine bay) and **b** a lighter
outside model with the same bolt-on parts, merged doors, no engine bay, a
cheaper interior and a shadow; the game picks one by its mode
(presumably garage and race; inferred). **c** is the cockpit-view model:
the front half of the body, the bonnet, mirrors, headlights and a
detailed interior, with no rear end and no wheels.

### Unidentified (`.bin`)

GUT Archive Tools names anything it doesn't recognize `.bin`, including
the 15,039 empty entries (written as 0-byte files). It doesn't recognize
the node and mesh files, so they come out as `.bin` when a container is
expanded.

## Saves

The save is `TXR_DATA_1` inside `SystemData.bin` (title `474507D1`); the
game opens it as `TXR360:\TXR_DATA_1` (`sub_8228CEE8`, `sub_8228CFA8`,
`sub_8228D228`, `sub_8228D370`). It is 129,200 bytes:

| Offset | Size | What |
|---|---|---|
| 0 | 6,336 | Header |
| 6,336 | 50 x 1,964 | Garage, one block per car slot |
| end - 24,672 | 24,016 | Unlocked cars and rivals defeated |
| end - 656 | 656 | Controller settings |

(From the old save converter. Its garage and unlock regions overlap by 8
bytes, so one of those boundaries is slightly off.)

Car colors in the save presumably use the same 20-byte paint record as
the car specs (above): `paintmaker.py` writes that layout but with the hue
big-endian, while the specs store it little-endian, as `car_color.py`
does. Check a save before relying on either. The shop-unlock bits for the cut cars are three bytes at `0x1985D`.

Import Tuner Challenge's save is 129,400 bytes with 1,968-byte car blocks;
the two convert into each other except for licence plates, which work
differently.

## Sources

- **This copy's data and code**: everything marked confirmed above was
  checked against `assets/DATA/BUILD/TXR/*.TOC`, an extraction of
  `BUILD.DAT`, and `default.xex` / the generated code.
- **Old research**: the user's own earlier scripts and notes (string,
  bitmap and save tools), kept locally in `docs/old_research/` and not
  committed.
- **GUT Archive Tools** (github.com/igorciz777/GUTArchiveTools, switch
  `-2` for this game): an extractor for Genki's GUT archives, used as a
  read-only reference. It's GPL-3.0, so its code isn't copied into this
  repo. Where it and this file differ, this file has been checked against
  the game:
  - it names string tables `.dat` and empty entries `.bin` (above);
  - it skips the loose-file entries but doesn't say which loose file each
    stands for (the table above);
  - it doesn't account for the TOC counts built into the XEX when
    appending.

## Not documented yet

- XACT audio: what's needed to make it work in the recomp (known to the
  user, to be written down).
- The car spec fields beyond those above, the wheel records (73), 83 and
  84-96, and which tuning level is which power or drive-train part.
- The rest of the part format (the material record's flags and the
  fields marked inferred, the small sections at +0x28-0x3C, what the
  tire morph targets and the `_12`/`_20` wheel shapes are for), and the
  rest of the unknown `.bin`s.
- The car modes (what sets a and b are used for), where the paint color
  and the environment maps are bound, and the interior textures
  (891 ...).
- `BUILD_CRS` contents in detail.
