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
  the table at `0x820069B0` (`bgm_tmp_01`-`17`, `BGM_SYSTEM`) and loads
  sound bank 22970 + i and wave bank 22988 + i; any other name loads
  `SB_SYSTEM_SE` (22885, 22886). It hands both ids to the sound object at
  `[0x826B5960]` (vtable slot 4).
- **Car effects**: `sub_821744D8` loads `CAR_SE.XWB` (22887).
- **Engine sounds**: the loader of 22888-22969 isn't found yet (the id is
  computed from the car's sound set).

## Not known yet

- Where the engine sound set for each car is chosen (the car specs,
  BUILD 72, are the likely place).
- The sound object at `[0x826B5960]`: its class and its other methods.
- How XACT's runtime variables are fed from the car (revs, throttle,
  load).
