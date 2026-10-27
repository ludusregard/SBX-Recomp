# Custom music

Your own music files played in the place of the game's music. Off until a
chance is set. Working in the game (2026-10-26): the menu songs, roaming,
and shuffle.

The user-facing guide is
`assets/ADDON_DATA/CUSTOM_MUSIC/CUSTOM_MUSIC_README.md`; keep it in step
with this doc. The category folders are in the repo, each with a
`metadata.xml` listing the game's own songs for it at weight 1 (which
changes nothing until edited). The repo's `.gitignore` keeps the music
people add out of git but not the `metadata.xml` files. `generate_metadata.py`
there makes those files (`--force` remakes them as shipped) and adds any
music file a folder's `metadata.xml` doesn't list yet. The scan skips hidden
files and folders.

## Using it

Put music files in a folder per category:

```
assets/ADDON_DATA/CUSTOM_MUSIC/
  ROAMING/
    metadata.xml      (optional)
    drive_01.ogg
    drive_02.flac
  BATTLE/
  BOSS/
  RACE_WIN/
  RACE_LOSE/
  MENU_COURSE/
  MENU_MAIN/
  MENU_GENERAL/
  MENU_STORY/
```

Each time the game starts music in a category, a roll decides whether one
of that folder's tracks plays instead. Set the chance with
`--new_music_roll` (0 to 1: 0 never, 0.5 half the time, 1 always; 0 by
default), or per folder with `ReplaceChance` in its `metadata.xml`.

- **Formats:** ogg (vorbis), opus, flac, mp3, wav. Any sample rate, mono
  or stereo (more channels are mixed down).
- **Names:** folder names are matched ignoring case. File names may use
  any characters.
- **Looping:** a track loops whole, unless `metadata.xml` gives it a loop
  region. It loops only if the game's own track would (race-end music
  plays once).
- **Shuffle:** with `--custom_music_shuffle` (on by default), a track that would loop
  (the game's loop count is not 0) plays on to its end and then another
  track of the same folder follows, without a gap. It is picked by weight
  and is never the same track twice in a row unless it is the folder's only
  one. A loop region is ignored (the whole file plays). The folder's
  tracks only: there is no roll against the game's music at this point.
  `--custom_music_shuffle=false` loops the track instead.
- **Logs:** each music start is logged (`custom_music:`), with what the
  game picked, the category, the roll and the track. `--custom_music_log`
  adds when each track starts and the game's loop count.
- **Off:** `--custom_music=false`.

### Categories

| Folder | When | The game's tracks |
|---|---|---|
| `ROAMING` | Driving on the highway | `bgm_tmp_06`, or one of `bgm_tmp_07`, 01, 15, 05, 10 at random |
| `BATTLE` | A battle (rival classes 0-2) | `bgm_tmp_07`, 01 or 15; `bgm_tmp_10`; `bgm_tmp_05` |
| `BOSS` | A battle (rival classes 3-5) | `bgm_tmp_03`, 09, 17, 11, 16 |
| `RACE_WIN` | After a race won | `bgm_tmp_14` |
| `RACE_LOSE` | After a race lost | `bgm_tmp_08` |
| `MENU_COURSE` | The course select | `BGM_SYSTEM`, song 0 |
| `MENU_MAIN` | The main menu | `BGM_SYSTEM`, song 1 |
| `MENU_GENERAL` | The other menus (dialogs, garage, car select) | `BGM_SYSTEM`, song 2 |
| `MENU_STORY` | The dialog after some story events | `BGM_SYSTEM`, song 3 |

`BGM_SYSTEM` holds four songs and the game starts them without loading it
again, so each menu song rolls on its own every time it starts (going into
a submenu and back to the main menu starts the main menu song again, and
rolls again). The four songs are told apart by their data (the first 2048
bytes' CRC-32, else the length; `kMenuSongs` in `custom_music.cpp`). Each
folder is separate: none falls back to another. Race music is split the
same way: `bgm_tmp_14` is the win music and `bgm_tmp_08` the lose music.

These are worked out from the code ("Music" in `docs/audio.md`) and not
yet checked in play: the log names each music start's category, so a wrong
label shows up there. The music select (the player's own pick) is never
replaced.

### metadata.xml

```xml
<Music Category="ROAMING" ReplaceChance="0.5">
  <Game Name="bgm_tmp_06" Weight="1"/>
  <Game Name="bgm_tmp_05" Weight="0"/>
  <Track File="drive_01.ogg" Weight="2" LoopIn="12.0" LoopOut="184.5"/>
  <Track File="drive_02.flac" Weight="1"/>
  <Track File="drive_03.wav" Weight="0"/>
</Music>
```

- `Category` (optional): must match the folder's name.
- `ReplaceChance` (optional, 0 to 1): this folder's chance, instead of
  `--new_music_roll`.
- `Track File` (required): a file in the folder.
- `Weight` (optional, 1 by default, 0 or more): how likely the track is
  picked. 0 keeps it in the folder but never picks it.
- `LoopIn`, `LoopOut` (optional, seconds, both or neither): the loop
  region. The track plays from its start, then loops between the two.
- `Volume` (optional, 0 to 2, 1 by default): the track's level, as a factor
  (0.5 is -6 dB, 2 is +6 dB). Above 1 can clip. It multiplies the adaptive
  gain below.
- `Game Name` (required) and `Weight` (optional, 1 by default): one of the
  game's songs. The game still picks as it does; when it picks a song of
  weight 0 (blocked) and the roll didn't already pick a custom track, one
  of the folder's tracks or another game song it lists plays instead, by
  weight. Unlisted game songs count as weight 1 but aren't stand-ins.
  Outside the menu folders `Name` can be any of `bgm_tmp_01`-`17`
  (including 02, 04, 12, 13, which the game never plays on its own) and a
  game song stands in by handing `Bgm::Load` that name instead. In a menu
  folder only `BGM_SYSTEM` (the folder's own song): the four share one bank
  and are started by cue, so a blocked menu song can only give way to the
  folder's tracks. Nothing to stand in: the blocked song plays anyway.

### Levels

`--custom_music_adaptive_volume` (on by default; a restart to change, not
live) gives each track a gain
that brings its integrated loudness (LUFS, ITU-R BS.1770, gated) to
-11.2 LUFS: the mean of seven game songs measured through the XMA path
(bgm_tmp_06, 07, 14 and the four BGM_SYSTEM songs: -9.6 to -12.6 each). The
change is capped at 12 dB either way.

`Track::Measure` decodes the whole file at its own rate and channels and
computes the BS.1770 integrated loudness (K-weighting, 400 ms blocks, the
-70 LUFS and relative -10 LU gates). It agrees with FFmpeg's `ebur128` to
0.1 LU on the game's songs and the friend's three.

Measured against the target: SUNBURN +3.1 dB, SHAKEDOWN +1.1 dB, HYPNO
-2.9 dB. SHAKEDOWN's boost would hard-clip about 15,500 samples (0.26%),
so the gain is followed by a soft clip above 0.9 of full scale (a tanh
knee into full scale). With it the same boost has no full-scale samples.

The gain is found in `Library::Scan`, once per file at start-up, as is
`metadata.xml`: neither is re-read while the game runs (about
half a second for four minutes of audio across the friend's three tracks),
so shuffle's next track has its gain ready and nothing decodes on the audio
thread. The track's gain (`Track::SetGain`) is applied as its samples are
converted, after the resampler, so the mono-to-stereo mix and the loop
points are unchanged.

Not used: FFmpeg's `dynaudnorm` (DynamicAudioNormalizer). It changes gain
continuously over the track, which is a live leveler, not a fixed match.
It's also in libavfilter, which our FFmpeg build doesn't include.

Files not named in the XML are picked with weight 1 and loop whole. A
mistake in the XML is logged: a bad value or broken XML turns that folder
off; anything else (an unknown attribute, a missing file, a loop region
past the track's end) is skipped or falls back to looping whole.

## How it works

Code: `src/hooks/custom_music.cpp` (the roll), `src/audio/music/` (the
library: `track` decodes a file, `metadata` reads the XML, `library` scans
the folders and picks), and `src/hooks/xma.cpp` (the swap).

1. **Start-up:** `ADDON_DATA/CUSTOM_MUSIC/` is scanned once. Each file is
   opened to check it, and its details are logged.
2. **The roll:** every music change except the menu's goes through `Bgm::Load`
   (`sub_82106540`). Its return address says which category it is (the
   table in `custom_music.cpp`). The roll uses our own random numbers, so
   the game's sequence (and its replays) is unchanged. A hit picks a track
   by weight and opens it. On a miss, a blocked game song is replaced:
   another game song by rewriting r4 to that name in Bgm::Load's own list
   (`0x820069B0`), or a track as on a hit.
3. **The swap:** the music is the only stereo XMA sound. When the next one
   starts, `xma.cpp` reads the custom track instead of decoding the XMA
   data. The game's voice, volume, fades and stop calls are untouched: it
   plays our samples where it would have played its own. Until the next
   `Bgm::Load`, the same sound starting again (the same XMA data) reopens
   the same track; any other stereo sound (a jingle in the same bank, say)
   plays the game's.
4. **Decoding:** FFmpeg (ogg, flac, mp3, wav demuxers; vorbis, opus, flac,
   mp3, PCM decoders; added to `thirdparty/ffmpeg-config/` for this, see
   `cmake/ffmpeg.cmake`) decodes the file, and libswresample converts it
   to the voice's rate and channels as 16-bit big-endian samples, what the
   XMA decoder gives. Decoding keeps just ahead of playback, and decoded
   samples are kept for the loop (about 11 MB a minute at 48 kHz stereo).
5. **Looping:** the game's XMA loop data still sets how often to loop
   (never, n times or forever); the region is the track's own. With
   shuffle, each loop is instead a hand-over to the next track:
   `xma.cpp` asks `custom_music` for one once the playing track is decoded
   to its end, chains it on (`Track::Chain`), and the track takes its place
   at the end (`Track::Adopt`), freeing the old samples. If none can be
   made, the track loops.
6. **Random numbers:** the rolls and picks use their own generator, seeded
   at start-up from the system's random source, the system time (both
   clocks) and an address, so each run differs. A start the game always
   makes the same way (the menu's one track) still rolls fresh each time.

## To check in play

- Each category's label, from the log (the menu songs, roaming and
  shuffle are confirmed; BATTLE, BOSS and the race categories still to
  confirm) (`custom_music: <bank> (from
  <address>, <category>)`).
- Whether any game logic waits for the music to end. Race-end music plays
  once, as the game's does, so a long custom track there could hold up
  whatever waits for it (the race exit waits for the screen fades, not the
  music, as far as known).
- A blocked song's stand-in, in play (the game loads another bank than it
  asked for; nothing else is known to use the name).
- Whether the music ever restarts within one `Bgm::Load` (logged as
  "restarted" with `--custom_music_log`).
- The volume path (ducking) still applies, since the game's voice plays
  the track.
- Adaptive volume and `Volume` in play: the log's `LUFS … adaptive …` and
  `volume …` values, and the level against the game's music. Not yet
  checked by ear.
