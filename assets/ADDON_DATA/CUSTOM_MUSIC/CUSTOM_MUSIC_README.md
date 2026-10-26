# Custom music

Play your own music in Shutoko Battle X in place of the game's, or just
stop a game song you don't like from playing. Put music files in the folder
for the part of the game you want, set how often they should play, and
start the game.

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

- A folder with no music in it leaves that part of the game alone (unless
  you block a game song, below).
- Each folder has a `metadata.xml` listing the game's own songs for it.
  As it comes, it changes nothing.
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

## Blocking the game's songs

Each folder's `metadata.xml` lists the game's songs that play there, all
at weight 1:

```xml
<Music Category="ROAMING">
  <Game Name="bgm_tmp_06" Weight="1"/>  <!-- the usual highway song -->
  <Game Name="bgm_tmp_07" Weight="1"/>  <!-- picked at random in some game modes -->
  ...
</Music>
```

Set a song's weight to `0` to block it. The game still picks its music as
usual, but when it picks a blocked song, something else from the same
folder plays instead, picked by weight: one of the game songs listed there,
or one of your tracks. This works without any music of your own and
without `--new_music_roll`.

- In the folders other than the menus, you can list any of the game's
  songs, `bgm_tmp_01` to `bgm_tmp_17`, to have it stand in for a blocked
  one. That includes `bgm_tmp_02`, `04`, `12` and `13`, which the game
  never plays on its own. Add a line like
  `<Game Name="bgm_tmp_12" Weight="1"/>`.
- The other weights only matter when standing in for a blocked song: a
  game song that isn't blocked still plays whenever the game picks it.
- The four menu songs share one sound bank, so a menu folder can only list
  its own song (`BGM_SYSTEM`). A blocked menu song can only be replaced by
  your tracks in that folder; with none there, it plays anyway.
- If everything in a folder is blocked, the game's pick plays anyway.

## When a track ends

If the game's music would have looped, the next track from the same folder
plays right after, without a gap (never the same track twice in a row,
unless it's the folder's only one). To loop the same track instead, start
the game with `--custom_music_shuffle=false`.

Music the game plays only once (after a race) plays once.

## metadata.xml (optional)

A folder's `metadata.xml` sets the weights of the game's songs and your
tracks, loop points and that folder's chance. Any part can be left out;
without the file, every track has weight 1 and loops whole.

```xml
<Music Category="ROAMING" ReplaceChance="0.5">
  <Game Name="bgm_tmp_06" Weight="1"/>
  <Game Name="bgm_tmp_05" Weight="0"/>
  <Track File="drive_01.ogg" Weight="2" LoopIn="12.0" LoopOut="184.5"/>
  <Track File="drive_02.flac" Weight="1"/>
  <Track File="drive_03.wav" Weight="0"/>
</Music>
```

| Attribute | On | What it does |
|---|---|---|
| `Category` | `Music` | Optional. If given, it must match the folder's name. |
| `ReplaceChance` | `Music` | Optional, 0 to 1. This folder's chance, instead of `--new_music_roll`. |
| `Name` | `Game` | Required. One of the game's songs: `bgm_tmp_01` to `bgm_tmp_17`, or `BGM_SYSTEM` in a menu folder. |
| `Weight` | `Game` | Optional, 0 or more, 1 by default. 0 blocks the song; otherwise, how likely it is to stand in for a blocked song. |
| `File` | `Track` | Required. A music file in this folder. |
| `Weight` | `Track` | Optional, 0 or more, 1 by default. A track with weight 2 is picked twice as often as one with weight 1; 0 is never picked. |
| `LoopIn`, `LoopOut` | `Track` | Optional, in seconds, both or neither. The track plays from its start, then loops between these two points. Used only with shuffle off (`--custom_music_shuffle=false`); with shuffle on, the whole track plays and the next one follows. |

- Files in the folder but not in the XML are picked with weight 1.
- A game song that isn't listed is never blocked, and never stands in for
  a blocked one.
- Mistakes are logged. A broken XML file or a bad value (a weight of -1,
  say) turns that folder off until fixed. Smaller mistakes (an unknown
  attribute, a file that isn't there) are skipped.

### Adding your tracks to metadata.xml for you

`generate_metadata.py`, in this folder, adds every music file a folder's
`metadata.xml` doesn't list yet, at weight 1, and leaves the rest of the
file as it is. Then edit the weights or add loop points. A folder without a
`metadata.xml` gets a new one (with the game's songs, for the category
folders). It needs Python 3:

```
python generate_metadata.py            add new tracks, make missing files
python generate_metadata.py --dry-run  show what it would change
python generate_metadata.py --force    remake every metadata.xml as it first came
```

`--force` throws away your edits to the `metadata.xml` files.

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

The music you add here is kept out of git, so it never ends up in the
repo. The `metadata.xml` files are part of the repo, so git shows them as
changed once you edit them.
