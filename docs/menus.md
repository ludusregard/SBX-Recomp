# Menus and settings

The game's own options: where they're stored, the menus that edit them, and
how those menus are built. The groundwork for adding our settings to the
game's menus (plan: "Adding our settings", below).

Traced statically from `generated/` on 2026-11-04 (no runs yet). Addresses are
guest addresses; "work" is the System menu's work object (below). Offsets are
in bytes. Items marked "to confirm" need a run with logging.

## The settings record in the save

The save object (`GameData::Get(0)`, 129,200 bytes, made by `sub_82105938`)
ends with a 284-byte settings record at save + `0x1F794`. `sub_82105AB8`
sets its defaults when the save object is made.

| Offset | Size | Default | What |
|---|---|---|---|
| +0, +4, +8, +12 | 4 x u32 | 1 | Game config: four on/off HUD items (see "Game config") |
| +16 | u32 | 1 | Not read by the menus; to confirm |
| +20 | u32 | 1 | Written before a race by `sub_821A09D0` (1 when a race setting at `[obj]+632` is 0); to confirm |
| +24 | u32 | 2 | The last race camera (0-2): written by `sub_82151160` and `sub_821A09D0`, read back by `sub_82157728` |
| +28 | u8 | 15 | BGM volume, 0-15 |
| +29 | u8 | 15 | SE volume, 0-15 |
| +32 | u32 | 0 | Not read by the menus |
| +36 to +41 | 6 x u8 | 0 | Controller options; +36 to +38 are the three items of System page 2 (to confirm what each is) |
| +44, +104, +164, +224 | 4 x 60 | `sub_82105A18` | Button maps, 15 words each (see "Button maps") |

The 12 bytes before it (save + `0x1F788`) are zeroed by `sub_82105B48`; not
part of the menus.

`docs/game_data.md`'s "Controller settings, end - 656" (from the old save
converter) overlaps this record; the record's real start is end - 284.

## Volumes

| Function | Name | What |
|---|---|---|
| `sub_821B80C8` | `Sound::SetSEVolume` | r3 = level 0-15: 0 gives 0.0, 15 gives 1.0, else level / 15. Sets XACT category `Default` through the audio object `[0x826B5960]` (vtable +116 the category lookup, +120 the volume) and keeps the value at `0x82606BC0` and `0x82606BC4`. |
| `sub_821B8198` | `Sound::SetBGMVolume` | The same for category `Music`; keeps the value at `0x82606B74`. |
| `sub_82110318` | `Sound::PlaySystemSE` | r3 = a system sound (0 decide, 2 cancel, 4 cursor move) through the audio object's +40. |

Called from the System menu's sound page (`sub_821BF9E8`), the pause sound
pages (`sub_8220C6C0`, `sub_8220EBE0`) and the System menu's load
(`sub_821BA450`).

## The System menu (garage, `TXR_MenuSystem`)

The garage's top menu items are, in order (their debug names, a table at
`0x826261C4`): `COURSE_IN`, `TUNING_SHOP`, `SETTING` (the car setup menu,
`TXR_MenuSetting`, `sub_821B4D30`), `CAR_SHOP`, `GARAGE_CAR`, `RIVAL_LIST`,
`SYSTEM`, `END_MODE`. The options are under `SYSTEM`.

### Globals

| Address | Size | What |
|---|---|---|
| `[0x826C7F18]` | 28 | State machine: +0 state, +4 next state, +8 previous state, +12 step (the update table's index), +16, +20, +24 |
| `[0x826C7F1C]` | 68 | Cursor: +0 row, +16 row count, +36 left/right this tick (-1, 0, 1), +8 set when closing |
| `[0x826C7F20]` | 764 | Work object (below) |
| `[0x826C7F24]` to `[0x826C7F30]` | | A second state machine and cursor (the page's sub-lists) |

All are made by `sub_821B9ED8` (`MenuSystem::Init`) and freed by
`sub_821B8318` (`MenuSystem::Exit`).

### Functions

| Function | Name | What |
|---|---|---|
| `sub_821B9ED8` | `MenuSystem::Init` | r3 = the scene. Allocates the globals above and clears the work object (`sub_821B8260`). |
| `sub_821B8318` | `MenuSystem::Exit` | Frees them. |
| `sub_821BA450` | `MenuSystem::Load` | Copies the save's settings record into the work object (applied copy), decodes the button maps (`sub_821B8E18`, `sub_821B8FA8`) and fills the defaults copy. Sets work +4 from the game mode at `0x825F3264` (2 gives 1, 4 gives 0, 6 gives 2, 7 gives 3, 8 gives 4). |
| `sub_821B92F0` | `MenuSystem::Store` | The reverse: the work object's applied copy back into the save's record (`sub_821B8EE0` encodes the maps). From `sub_821BB390`. |
| `sub_821C6630` | `MenuSystem::Input` | Input dispatch: calls the handler for the state at `[[0x826C7F18]+0]` (table below). |
| `sub_821C6728` | `MenuSystem::Update` | Update dispatch: a 49-entry table indexed by the step (+12): each page's open, draw and close steps. Its first call prints `TXR_MenuSystem` (`sub_821611F0`, a debug text print). |
| `sub_821C56C8` | `MenuSystem::OpenPage` | Step 7: plays the page's open animation and sets the next state from the row (table below). |
| `sub_82184468` | `MenuCursor::Update` | r3 = a cursor: reads the action mapper for up, down, left and right (actions 153-155, 167-169, ...), moves +0, sets +36. |
| `sub_821843C8` | `MenuCursor::Pressed` | r3 = a cursor, r4 = an action: 159 accept, 158 cancel. |

### States (`MenuSystem::Input`'s table)

| State | Handler | What |
|---|---|---|
| 0 | `sub_821BABB8` | The page list. Accept keeps the row at work +712 and opens it (step 7); cancel leaves. |
| 1 | `sub_821BF7C0` | Page 0, game config (four toggles and OK / Default) |
| 2 | `sub_821BF9E8` | Page 1, sound (BGM, SE, OK / Default, music select) |
| 3, 4 | `sub_821BFDD8`, `sub_821BFFF0` | Page 2 (three items, work +276 to +284; two layouts by mode) |
| 5, 6 | `sub_821C0258`, `sub_821C0428` | Page 3 (button maps, work +300 and +508) |
| 7, 8 | `sub_821C06F8`, `sub_821C08E8` | Page 4 (button maps) |
| 9 | `sub_821C5498` | Page 5 (reads work +756; to confirm, probably saving) |
| 10 | `sub_821BB780` | Page 6 (work +756; to confirm) |
| 11 | `sub_821B84A0` | Music select: starts the preview |
| 12 | `sub_821B8580` | Music select: plays `bgm_tmp_` track work +740 |
| 13 | `sub_821B8810` | Music select: stops it |
| 14 | `sub_821BB888` | Closing |

The rows each mode offers, as page numbers (`0x8200A498`, 7 per mode, -1 none;
mode = work +4):

| Mode | Rows |
|---|---|
| 0 | 0, 1, 2, 3, 4, 5 |
| 1, 2 | 0, 1, 2, 3, 4, 5, 6 |
| 3 | 0, 1, 2, 3, 4, 5 |
| 4 | 1, 2, 3, 4 |

### The work object (`[0x826C7F20]`, 764 bytes)

Three copies of the settings, 208 bytes each: **applied** at +44, **edited**
at +252, **defaults** at +460. Within a copy:

| Offset in copy | Save field | What |
|---|---|---|
| +0 to +12 | +0 to +12 | Game config toggles |
| +16 | +28 | BGM volume |
| +20 | +29 | SE volume |
| +24 to +44 | +36 to +41 | Controller options (one word each) |
| +48 to +96 | +44 | Button map (decoded) |
| +100 to +148 | +164 | Button map |
| +152 to +176 | +104 | Button map |
| +180 to +200 | +224 | Button map |

Other fields: +4 mode, +8 a flag from the save (+4, or bit 5 of +6092),
+708 the OK / Default choice, +712 the page list's row, +736 the music
select's highlighted track (0-16, wraps), +740 the track playing, +748 the
preview's state, +756, +760 the scene to go to on exit (21).

How a page works (sound page, `sub_821BF9E8` with `sub_821BCB00`):

- Left and right change the edited copy's value for the row: row 0 BGM and
  row 1 SE (clamped 0-15), row 2 the OK / Default pair (+708), row 3 the
  music select. Changed volumes are applied at once (`Sound::Set*Volume`) so
  they can be heard.
- Accept on row 2: OK (+708 = 0) copies edited to applied and closes;
  Default (+708 = 1) copies defaults to edited. Accept on any other row moves
  to OK.
- Cancel copies applied to edited (drops the changes), restores the volumes
  and closes.
- `sub_821BCD28` and `sub_821BCDB0` show the values: the UI movie (`sub_8211B5C0`
  id 7), sub-scene 6, children 6 and 7, set by `sub_821BCA70` and
  `sub_8220E420` (a gauge's animation).

The game config page (`sub_821BF7C0`, with `sub_821BC938`) works the same
way with four on/off rows and the pair on row 4, where OK is +708 = 1.

The applied copy reaches the save through `MenuSystem::Store`; the save is
written to the storage device by the save slot steps
(`docs/functions.md`, "Saves").

### Game config

The four words are HUD items: `sub_822104F0` (from the race set-ups
`sub_8216A4F0`, `sub_8216B2F8`, `sub_8216BA18`) copies them to a HUD object
and shows or hides nodes of its UI movie (+608) by them (+80 the node's
visible flag):

| Word | Nodes (sub-scene, child) |
|---|---|
| +0 | (-1, 3) |
| +4 | (-1, 1) |
| +8 | (0, 3), (0, 4), and (0, 1) with the HUD's +476 |
| +12 | (-1, 4), when the HUD's +472 is set |

Which HUD element each is (map, meters, rival gauge...) is to confirm in
play.

### Button maps

Each map is 15 words. `sub_821B8E18` decodes a stored value (0-30, a jump
table) to the menu's button index; `sub_821B8EE0` encodes it back.
`sub_821B8FA8` decodes the other two maps. `sub_8215DBF8` (from
`sub_8215E390`) reads the maps for a race.

## The pause menu (races)

| Address | What |
|---|---|
| `[0x826C7F7C]` | The pause object: +132 the pause type, +136 the item count; also holds its own copy of the game config (+0 to +12) |
| `[0x826C7F80]`, `[0x826C7F84]` | The cursor's UI movie and the item list (`MenuList`) |
| `[0x826C7F8C]` | The sound config page's object (+0 BGM, +4 SE) |

Items by pause type (`0x8200C8A8`, 6 per type; debug names at
`0x826262D8`): 0 = `CONTINUE`, 1 = `GAMECONFIG`, 2 = `SOUNDCONFIG`,
3 = `RESPONSE`, 4 = `RETIRE`, 5 = `RETURN_GARAGE`, 6 = `RETRY`,
7 = `RETURN_BASE`, 8 = `END`, 9 = `EXIT`.

| Type | Items |
|---|---|
| 0 | CONTINUE, GAMECONFIG, SOUNDCONFIG, RESPONSE, RETURN_GARAGE |
| 1 | CONTINUE, GAMECONFIG, SOUNDCONFIG, RESPONSE, RETIRE |
| 2 | CONTINUE, GAMECONFIG, SOUNDCONFIG, RESPONSE, RETURN_GARAGE, RETRY |
| 3 | CONTINUE, GAMECONFIG, SOUNDCONFIG, RESPONSE, RETURN_BASE, RETRY |
| 4 | CONTINUE, GAMECONFIG, SOUNDCONFIG, RESPONSE, RETURN_BASE |
| 5 | CONTINUE, SOUNDCONFIG, END, RETRY |
| 6 | CONTINUE, GAMECONFIG, SOUNDCONFIG, RESPONSE, EXIT, RETRY |
| 7 | CONTINUE, RETIRE |
| 8 | CONTINUE, SOUNDCONFIG, 10 (not named) |

| Function | Name | What |
|---|---|---|
| `sub_8220B5D0` | `Pause::Input` | The item list: `MenuList::Update` (`sub_82209F78`) on `[0x826C7F84]`, then accept, cancel and start (`sub_82209AE8`, `sub_82209B78`, `sub_82209C08`); the chosen item from the table above. |
| `sub_8220D578` | `PauseConfig::Load` | Copies the save's game config, volumes and +36 to +38 into the pause objects. From `sub_8220D7C0`. |
| `sub_8220B8D0` | `PauseGameConfig::Input` | The game config page; stores through `sub_8220B478` (game config) and `sub_8220B3D8` (all). |
| `sub_8220C6C0`, `sub_8220EBE0` | | The sound config page's input (two layouts); stores through `sub_8220B4D8` and `sub_8220E5A8` (from `sub_8220E698`). |
| `sub_8220E540` | | The sound page's load: the save's volumes into `[0x826C7F8C]`. |

The pause menu writes the save's record directly; the System menu goes
through its work object.

## Other front-end menus

Named by the debug text their update prints first (`sub_821611F0`):
`TXR_MenuCarShop`, `TXR_MenuCmnWindow`, `TXR_MenuGarage`,
`TXR_MenuGarageCar`, `TXR_MenuNumberPlate`, `TXR_MenuRivalList`,
`TXR_MenuSaveLoad`, `TXR_MenuSetting`, `TXR_MenuSpec`, `TXR_MenuSystem`,
`TXR_MenuSystemSub` (`sub_821C64D8`, a 12-entry update table),
`TXR_MenuUserInfo`. Their pattern is the System menu's: globals in
`0x826C7Fxx`, a state machine with input and update tables built on the
stack each call, `MenuCursor` or `MenuList` for the rows, a UI movie for the
look.

## How the garage menus open and close

### State tables

Each menu is driven by game messages (`GameMessages::Dispatch`,
`sub_82106478`, already overridden in `src/subtick/game_messages.cpp`). A
message is 12 bytes, `{id, 0, 0}`, queued in the dispatcher's ring (+204,
50 entries; count +804). For each handler object, `sub_82106410` looks up
the pair (the object's state at +9604, the message id) in its table and
calls the entry's function; when the entry has a next state, +9604 becomes
it.

| Function | Name | What |
|---|---|---|
| `sub_821062E8` | `StateTable::Add` | r3 = the handler object, r4 = the key `{state, message id, 0, 0}`, r5 = the entry (+0 the callback, +28 the next state; 0 keeps the state). Entries are 48 bytes at +16; the count is at +9600 (up to 200). |
| `sub_82106410` | `StateTable::Dispatch` | r3 = the handler object, r4 = the message: finds the entry for (+9604, the message id), returns it, and sets +9604 to its next state. |
| `sub_82194E48` | `GarageMode::Init` | Makes the garage's menus and registers each one's table (below). |

Messages are posted inline (no shared function): the poster bumps the
count at +804 and writes the id at the ring's next slot.

### The garage's menus

Every garage menu has the same states: 0 to 4 set-up and loading, 5
active, 8 closing, 9 dormant, 7 freeing. Message 1 is the tick and 0 the
draw in state 5. A dormant menu opens on its open message; an active one
starts closing on its close message; message 3 in state 8 runs its
"closed" step, which posts the next message.

| Menu (registration) | Open (state 9) | Close (state 5) | "Closed" step (8, 3) |
|---|---|---|---|
| Garage top, `TXR_MenuGarage` (`sub_8218E720`) | 6, 26, 45 (`sub_8218D6E0`) | 5 | `sub_8218C3D0`, posts 8 |
| Course in (`sub_8218AC78`) | 8 | 7 | `sub_82188DE8` (posts a variable message) |
| Tuning shop (`sub_821E5928`) | 12 | 11 | `sub_8219A7C8`, posts 6 |
| Car setup, `TXR_MenuSetting` (`sub_821B54D8`) | 18, 19 | 17 | `sub_821AE1A0`, posts 6 |
| Car shop, `TXR_MenuCarShop` (`sub_821837D0`) | 10 | 9 | `sub_8217F6B0`, posts 6 |
| Garage car, `TXR_MenuGarageCar` (`sub_821940F0`) | 23 | 22 | `sub_8219A7C8`, posts 6 |
| Number plate, `TXR_MenuNumberPlate` (`sub_821997B0`) | 28 | | `sub_8219A7C8`, posts 6 |
| Rival list, `TXR_MenuRivalList` (`sub_8219FBE0`) | 21 | 20 | `sub_8219A7C8`, posts 6 |
| System, `TXR_MenuSystem` (`sub_821C8340`) | 14 | 13 | `sub_821BB390`: `MenuSystem::Store`, posts 6 |
| System sub (`sub_821C6AB8`) | 16 | 15 | |
| Spec, `TXR_MenuSpec` (`sub_821B7580`) | 24 | 5 | |
| User info (`sub_822091B0`) | 27 | 5 | |
| Save / load, `TXR_MenuSaveLoad` (`sub_821A5360`) | 43, 46 | | |
| Common window, `TXR_MenuCmnWindow` (`sub_82185FE0`) | 34-37 | 5 | |

Message ids are per dispatcher: other contexts (title, races, the map) reuse
the same numbers for other things.

So **message 6 is "back to the garage top"**: every shop or garage
sub-menu posts it when it has closed, and `sub_8218D6E0` reopens the top's
list. Which menu just closed is the last open message the garage
dispatcher delivered.

Returning from a race goes through the garage mode's set-up again: the
garage top starts at state 0 and passes its load step, state 2,
`sub_8218C240` (the repo's `PostRace::Check`, which is this load step, run
on every entry to the garage), before state 5.

### Saving

| Function | Name | What |
|---|---|---|
| `sub_8228CEE8` | `SaveGame::Write` | Starts writing the whole save object (`GameData::Get(0)`, 129,200 bytes) as `TXR360:\TXR_DATA_1`, display name from the date (`sub_82294300`), through `sub_820A0678` (the save slots' request, `[0x826AD970]` vtable +60). Asynchronous. |
| `sub_8228CFA8` | `SaveGame::Read` | The same for loading. |
| `sub_8228CE08` | `SaveGame::Status` | r3 = 0: the request's state, 0-5 (`sub_820A02E0`). Which value is "done" is to confirm. |
| `sub_8228CD98` | `SaveGame::Finish` | r3 = 0: acknowledges the finished request (`sub_820A03F0`). |
| `sub_8228D228`, `sub_8228D370` | | Replay save and load (`rep_%d_...`, 1,334,880 bytes). |

The game's own saves: the System menu's save confirmation
(`sub_821BAFB8`, a yes / no dialog on `[0x826C7F2C]`: yes calls
`SaveGame::Write` after showing the saving window, `sub_821B9C38`), and the
save / load menu (`sub_821A0E00`, `sub_821A1958`). Shops, tuning and races
change the save object in memory; nothing reaches the storage device until
one of those runs.

`src/hooks/autosave.cpp` (`--autosave`, off by default; built 2026-11-06, not
yet tested) saves through the same calls when the garage top is active
again after the tuning shop, car setup, car shop, garage car or number plate
menu, or after a day of racing. It waits for `SaveGame::Status` 4 (what the
System menu's save step `sub_821C0B80` waits for), calling `sub_8228CE40` after
each read as that step does, then `SaveGame::Finish`; it skips when
`sub_8228CDD0` answers 1 (the System menu shows an error then) or a save is
already running. `--log_autosave` logs every state change and every message
other than 0 and 1 that a handler object takes, and the save's status values:
the run that confirms the ids above.

## Adding our settings (plan)

See the project plan for the reasoning; in short:

1. A new page in the System menu: a new row in the mode table, and states
   from 15 up handled by our code before `MenuSystem::Input` and
   `MenuSystem::Update` (both dispatch through tables they build, so a
   wrapper can take states it owns and pass the rest on).
2. Its look reuses an existing page's UI movie sub-scene (the sound page:
   two gauges, a pair of buttons, a selector) with our own strings.
3. Our settings stay in `sbx.toml` (`src/base/settings_file.cpp`): OK
   writes them, as the F4 Save button does, so the file stays the one place
   to edit by hand.

To confirm first, with logging: the page list's row count and text in the
System menu's UI movie (id 7), what pages 2, 5 and 6 are, the HUD items'
names, and the rows' strings (their string ids).
