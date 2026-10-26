# Custom music

Play your own music in Shutoko Battle X in place of the game's. Put music
files in the folder for the part of the game you want, set how often they
should play, and start the game.

## Quick start

1. Copy some music files into one of the folders below, for example
   `ROAMING/`.
2. Start the game with `--new_music_roll=1` on its command line (your
   music then plays every time, wherever a folder has tracks).
3. Play. If something doesn't play, look in the log (`logs/`, next to
   `sbx.exe`) for lines starting with `custom_music:`.

## The folders

| Folder | Plays in place of |
|---|---|
| `ROAMING` | The music while driving on the highway |
| `BATTLE` | Battle music against regular rivals |
| `BOSS` | Battle music against the stronger rivals and bosses |
| `RACE_WIN` | The music after a race you won |
| `RACE_LOSE` | The music after a race you lost |
| `MENU_MAIN` | The main menu |
| `MENU_GENERAL` | The other menus: dialogs, the garage, car select |
| `MENU_COURSE` | The course select |
| `MENU_STORY` | The dialog after some story events |

- A folder with no music in it leaves that part of the game alone.
- Folder names can be upper or lower case. Any other folder here is
  ignored (the log says so).
- The music select (when you pick a track yourself in the game) is never
  replaced.
- BATTLE vs BOSS is the game's own split by rival; it isn't confirmed for
  every rival yet.

## Music files

- **Formats:** `.ogg` (Vorbis), `.opus`, `.flac`, `.mp3`, `.wav`. Any
  sample rate, mono or stereo.
- **Names:** anything, including spaces and non-English characters.
- Only files directly in a category folder are read, not subfolders.
- Other files (cover art, text) are skipped.
- Each track is held in memory while it plays, about 11 MB per minute of
  music, so very long tracks (an hour-long mix) use a lot of memory.

## How often your music plays

Each time the game starts a piece of music, a roll decides whether one of
your tracks plays instead:

- `--new_music_roll` sets the chance for every folder: `0` never (the
  default, so nothing changes until you set it), `0.5` half the time, `1`
  always.
- A folder's `metadata.xml` can set its own chance with `ReplaceChance`
  (below), which overrides `--new_music_roll` for that folder.

Which track plays is picked at random from the folder, by weight (all
equal unless `metadata.xml` says otherwise). The game's own fades, stops and
volume settings still apply to your music.

## When a track ends

If the game's music would have looped, the next track from the same folder
plays right after, without a gap (never the same track twice in a row,
unless it's the folder's only one). To loop the same track instead, start
the game with `--custom_music_shuffle=false`.

Music the game plays only once (after a race) plays once.

## metadata.xml (optional)

A `metadata.xml` in a folder sets weights, loop points and that folder's
chance. Without one, every track has weight 1 and loops whole.

```xml
<Music Category="ROAMING" ReplaceChance="0.5">
  <Track File="drive_01.ogg" Weight="2" LoopIn="12.0" LoopOut="184.5"/>
  <Track File="drive_02.flac" Weight="1"/>
  <Track File="drive_03.wav" Weight="0"/>
</Music>
```

| Attribute | On | What it does |
|---|---|---|
| `Category` | `Music` | Optional. If given, it must match the folder's name. |
| `ReplaceChance` | `Music` | Optional, 0 to 1. This folder's chance, instead of `--new_music_roll`. |
| `File` | `Track` | Required. A music file in this folder. |
| `Weight` | `Track` | Optional, 0 or more, 1 by default. A track with weight 2 is picked twice as often as one with weight 1; 0 is never picked. |
| `LoopIn`, `LoopOut` | `Track` | Optional, in seconds, both or neither. The track plays from its start, then loops between these two points. Used only with shuffle off (`--custom_music_shuffle=false`); with shuffle on, the whole track plays and the next one follows. |

- Files in the folder but not in the XML are picked with weight 1.
- Mistakes are logged. A broken XML file or a bad value (a weight of -1,
  say) turns that folder off until fixed. Smaller mistakes (an unknown
  attribute, a file that isn't there) are skipped.

### Making metadata.xml files for you

`generate_metadata.py`, in this folder, writes a starting `metadata.xml` in
every folder that has music, listing each file at weight 1. Edit the
results to change weights or add loop points. It needs Python 3:

```
python generate_metadata.py            write the files
python generate_metadata.py --dry-run  show what it would write
python generate_metadata.py --force    also replace existing metadata.xml files
```

A folder that already has a `metadata.xml` is left alone unless you use
`--force`.

## All the options

Add these to the game's command line:

| Option | Default | What it does |
|---|---|---|
| `--custom_music` | `true` | `false` turns custom music off entirely. |
| `--new_music_roll` | `0` | The chance (0 to 1) your music plays, for folders without a `ReplaceChance`. |
| `--custom_music_shuffle` | `true` | When a track ends, play another from the folder. `false` loops the same track. |
| `--custom_music_log` | `false` | Log more detail: when each track starts, shuffles and the game's loop settings. |

## When something doesn't work

Every line about custom music in the log starts with `custom_music:`.

- **At start-up**, every track found is listed with its format and length,
  and anything skipped says why.
- **Each time music starts**, the log says what the game picked, which
  folder it counts as, the roll and the chance, and which of your tracks
  plays (or that the game's own music plays).

Common problems:

- *Nothing plays:* the chance is 0. Set `--new_music_roll` or a folder's
  `ReplaceChance`.
- *"isn't a category":* the folder name is misspelled.
- *"isn't a format we read":* the file is damaged or isn't really the
  format its extension says. Converting it to `.ogg` or `.flac` usually fixes
  this.
- *"this folder is turned off":* `metadata.xml` has a mistake; the line
  says where.

## Git

Each folder's `.gitignore` keeps your music and `metadata.xml` files out of
git, so they never end up in the repo. Don't delete those files: they keep
the empty folders in place.
