# Audio

How the game's sound works: the libraries, the files, and the code that
loads them. Formats of other data are in `docs/game_data.md`; function
details in `docs/functions.md`.

## The stack

1. **XACT** (Microsoft's audio engine, linked into the game and
   recompiled like the rest of its code) plays cues from sound banks
   (`.xsb`), whose waves are in wave banks (`.xwb`). One global settings
   file, `TXR_XACT.XGS`, defines the categories and variables; the
   engine sounds depend on XACT's runtime variables.
2. **XAudio** (also linked in) mixes every playing voice on the CPU into
   6-channel frames: 256 samples a channel, big-endian floats, about
   5.3 ms at 48 kHz.
3. **The render driver** takes those frames to the hardware. The game's
   driver object registers its mixer callback and submits each frame
   through kernel calls, which ReXGlue implements:

   | Function | Kernel call |
   |---|---|
   | `AudioRenderDriver::Register` (`sub_8244D0A0`) | `XAudioRegisterRenderDriverClient` |
   | `AudioRenderDriver::Submit` (`sub_8244D128`) | `XAudioSubmitRenderDriverFrame` |
   | `AudioRenderDriver::Destroy` (`sub_8244D188`) | `XAudioUnregisterRenderDriverClient` |

   All three are reached only through the driver object's vtable. In
   ReXGlue, the SDK's "Audio Worker" thread calls the mixer when SDL has
   room for more frames.
4. **XMA**, the console's compressed audio decoder, is emulated by the
   SDK (with FFmpeg). The game imports only `XMACreateContext` and
   `XMAReleaseContext`; XAudio drives the decoder through memory and its
   registers.

Other audio imports: `XAudioGetVoiceCategoryVolume` and
`XAudioGetVoiceCategoryVolumeChangeMask`.

The cleanest place to replace the output is the render driver: keep XACT
and XAudio, override the three functions above, and feed the frames to
our own output. ITC-Recomp did exactly that (`docs/native-audio.md` in
ITC-Recomp, `--native_audio`), because the SDK's Audio Worker spins a
whole core.

## The files

The banks are in `BUILD.DAT`, except four that are loose files on disc
(the loose-file table, see `docs/game_data.md`):

| Id | File | What |
|---|---|---|
| 22884 | `DATA/AUDIO/TXR_XACT.XGS` (loose) | Global settings |
| 22885, 22886 | `DATA/AUDIO/SB_SYSTEM_SE.XSB`, `.XWB` (loose) | Menu sounds |
| 22887 | `DATA/AUDIO/CAR_SE.XWB` (loose) | Car sound effects |
| BUILD 22888-22928 | `<sound>_off` wave banks | Engine sounds, off throttle (41) |
| BUILD 22929-22969 | `<sound>_on` wave banks | Engine sounds, on throttle (41, same order) |
| BUILD 22970-22986 | `bgm_tmp_01`-`17` sound banks | Music |
| BUILD 22987 | `BGM_SYSTEM` sound bank | Menu music |
| BUILD 22988-23004 | `bgm_tmp_01`-`17` wave banks | Music |
| BUILD 23005 | `BGM_SYSTEM` wave bank | Menu music |

The engine sound sets, in order: `AE86_RED`, `CELESIOR`, `CROWN_A`,
`FC3S`, `IMP_NORM`, `IMP_TUNE`, `JZA70_SOARA`, `JZA80`, `MARKX`, `R32`,
`R32_TUNE`, `RX8`, `S13_TUNE`, `S16`, `S16_WHITE`, `V36_SKYLINE`, then
the generic sets `OP_1`-`5`, `RO_1`-`5`, `S4_1`-`5`, `S6_1`-`5`,
`V6_1`-`5` (names from each bank's header). 41 sets for 72 cars, so
cars share sets.

Wave banks are big-endian (`DNBW`), version 37; the bank name is at
+0x38. Per ITC-Recomp's survey of the same files: engine sounds and
`car_se` are 16-bit mono PCM at 44.1 kHz; the music (one wave a bank,
about a minute, with loop points) and `BGM_SYSTEM` are stereo XMA;
`SB_SYSTEM_SE` is mono XMA. The XMA banks are streaming banks, so in a
race the XMA decoder only plays the music.

`DATA/AUDIO/` on the disc also has loose copies of every bank above,
plus some the archive doesn't have: `bgm_tmp_18`, `CELESIOR.xwb` (no
`_on`/`_off`), `Wave Bank.xwb`, `Sound Bank.xsb` and `Sound Bank 2.xsb`.
The game reads the archive copies (the extra ones are probably left over
from development; `bgm_tmp_18` could be a cut 18th track).

## The code

- **Startup**: `sub_82094BB8` names the engine `txr_xact`;
  `sub_82118818` builds `data\audio\%s.xgs` and reads the global settings.
- **Music**: `Bgm::Load` (`sub_82106540`) takes a bank name, finds it in
  its list (`bgm_tmp_01`-`17`, `BGM_SYSTEM`, built on the stack from the
  strings at `0x820069B0`, which are stored in another order) and loads
  sound bank 22970 + i and wave bank 22988 + i; any other name loads
  `SB_SYSTEM_SE` (22885, 22886). It hands both ids to the sound object at
  `[0x826B5960]` (vtable slot 4). Who picks the music: "Music" below.
- **Car effects**: `sub_821744D8` loads `CAR_SE.XWB` (22887).
- **Engine sounds**: the loader of 22888-22969 isn't found yet (the id is
  computed from the car's sound set).

## Music

Every change of music bank is a call to `Bgm::Load` (the menu bank's four
songs are the exception, below). Its callers, by return
address (custom music uses these as its categories, `docs/custom_music.md`;
the labels are from the code, not yet checked in play):

| Return address | Function | Music | Category |
|---|---|---|---|
| `0x82107B38` | `sub_82107A38` | `BGM_SYSTEM` (once; its four songs are started by cue, no more loads) | MENU_COURSE, MENU_MAIN, MENU_GENERAL, MENU_STORY (told apart by the sound's data) |
| `0x82173D80` | `Music::PlayRoaming` (`sub_82173C70`) | `bgm_tmp_06`, or one of 07, 01, 15, 05, 10 at random | ROAMING |
| `0x8217424C` | `Music::PlayRequested` (`sub_821741B8`), bit 0x40 | `bgm_tmp_07`, 01 or 15 by `[[0x826B580C]+32]` (0, 1, 2) | BATTLE |
| `0x8217427C` | the same, bit 0x100 | `bgm_tmp_10` | BATTLE |
| `0x821742AC` | the same, bit 0x80 | `bgm_tmp_05` | BATTLE |
| `0x821742DC` | the same, bit 0x200 | `bgm_tmp_03` | BOSS |
| `0x8217430C` | the same, bit 0x400 | `bgm_tmp_09` | BOSS |
| `0x8217433C` | the same, bit 0x800 | `bgm_tmp_17` | BOSS |
| `0x8217436C` | the same, bit 0x1000 | `bgm_tmp_11` | BOSS |
| `0x8217439C` | the same, bit 0x2000 | `bgm_tmp_16` | BOSS |
| `0x82174C84` | `sub_82174AD0` | `bgm_tmp_08` (the lose music) | RACE_LOSE |
| `0x82262678` | `sub_822624F0` | `bgm_tmp_14` (the win music) | RACE_WIN |
| `0x822626B0` | the same | `bgm_tmp_08` | RACE_LOSE |
| `0x821B8788` | `sub_821B8580` | any of the 17, by a setting (`[[r31+32544]+740]`): the music select | never replaced |
| `0x82110280` | `sub_82110180` | `SB_SYSTEM_SE` (sound effects, at boot) | never replaced |

- **`BGM_SYSTEM`** is loaded once and holds four songs (one wave each, in
  the wave bank's order), started by cue with no further `Bgm::Load`. From
  the disc's `BGM_SYSTEM.xwb` (its cues, in the sound bank's order:
  `COURSEIN`, `GARAGE`, `PA`, `TITLE`):

  | Wave | Wave data (offset, length) | Length | CRC-32 of its first 2048 bytes | Used for (from play) |
  |---|---|---|---|---|
  | 0 | 0x0, 0x1BD000 | 61.8 s | `E3A47B25` | course select |
  | 1 | 0x1BD000, 0xBA800 | 27.2 s | `E66C2C17` | main menu |
  | 2 | 0x277800, 0x18F800 | 49.0 s | `C448EE59` | other menus (dialogs, garage, car select) |
  | 3 | 0x407000, 0x12B000 | 39.0 s | `4FEB9978` | dialog after some story events |

  (Offsets from the wave data's start, 0x800 into the file. The cue names
  don't obviously match the uses, so the wave numbers are the user's
  report of which index each scene starts; check with
  `--custom_music_log`.)
- **The random roll** is in `Music::PlayRoaming`: by the music mode at
  scene+9652 (set by `sub_82175238` from a table indexed by the game
  state), modes 2, 4, 5 and 6 pick entry 1-5 of the table at `0x82609334`
  with `sub_8258F558() % 5`; other modes play entry 0. It's called when
  the highway scene starts (`sub_821744D8`, which also loads `CAR_SE.XWB`)
  and from `sub_82174668`.
- **The table at `0x82609334`**: 13 name pointers, entries 0-12:
  `bgm_tmp_06`, 07, 01, 15, 05, 10, 03, 09, 17, 11, 16, 14, 08.
- **Battle music** is requested by `Music::RequestBattle` (`sub_821743B0`)
  as a bit at scene+9644, by the opponent: its record (`sub_821172E0`)
  byte +35 is the rival's class. No opponent, class 0 or above 5: 0x40;
  class 1: 0x100; class 2: 0x80; class 3: 0x400 for rivals 383-390, 0x800
  for 392-397, else 0x200; class 4: 0x800 for rival 391, 0x1000 for 398,
  else 0x400; class 5: 0x2000. `Music::PlayRequested` then plays it and
  clears the bit.
- The scene is the object these message handlers run on (r3); all of them
  are registered by `sub_82175238`.

## Not known yet

- Where the engine sound set for each car is chosen (the car specs,
  BUILD 72, are the likely place).
- The sound object at `[0x826B5960]`: its class and its other methods.
- How XACT's runtime variables are fed from the car (revs, throttle,
  load).
- Which rival classes are which (wanderers, team members, bosses), to
  check the BATTLE and BOSS labels; and what the music modes 1-6 and
  `[[0x826B580C]+32]` are.
