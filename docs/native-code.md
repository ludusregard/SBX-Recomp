# Native game code

The game's functions rewritten as our own C++, one subsystem at a time,
each behind its own `--native_<name>` flag (on the F4 menu under Hooks /
Native Code). Every flag is on by default. With a flag off the recompiled
function runs,
so a flag can be turned on and off between runs to compare. Each
function has a row in `docs/functions.md`, "Native game code".

`build_screen` has no flag: its functions are the build screen's lists
and draws, none a plain set-up or getter.

Stays recompiled, by rule: the boot-once set-ups that copy stack words
they never write (RaceSceneMachine::Init, RaceFlow::Init `sub_8215E9A8`:
only the original reproduces them), and the C++ library's containers the
game instantiated on its heap (std::list, std::string, and std::map as the
screen object's UvMap and UvColumn).

## How each one is made

ITC-Recomp has converted much of its build of the game to C++. We use it as
a reference only; nothing of ITC's is compiled in.

1. `tools/port_native.py` rewrites ITC's source to SBX's addresses, and
   grades each function: A, the same instructions (addresses masked); B or
   C, similar code that differs somewhere.
2. For each function, SBX's generated code (`generated/default`) is read
   and our C++ written from it. ITC's C++ says what the function does; for
   A functions it can be followed closely, for B and C the differences are
   found in SBX's code and written down here.
3. Struct offsets aren't addresses, so the rewrite doesn't change them: a
   function that reads a struct SBX lays out differently is graded B or C
   and written from SBX's code.
4. Where ITC's code calls into a part of its native layer we don't have
   (its C runtime, its heap, its archive), ours calls the recompiled
   function instead.
5. It's compiled here; then tested in play with the flag on and off.

Float code follows the generated code's steps: a single add, subtract,
multiply or divide is done in float (the same result as the generated
code's double rounded to float), the fused ones (fmadds, fnmsubs) as
`float(std::fma(...))` over the doubles, and every compare branches as the
original's does: an unordered compare (a NaN) takes neither ble nor bge.
Arguments in f1 are the callers' doubles, used as they are.

Long runs of stores (records' defaults, constructors) are written as
tables (`src/hooks/guest_memory.h`, `Fill`), and long copies between two
records as runs (`CopyWords`, `CopyHalves`, `CopyBytes`): each one read from SBX's
generated code in its order by a script that follows the registers
(constants, the image's floats by their address, the object's own offsets,
the arguments), with the calls between them made in their place. A
function the script can't follow to the end (a branch on a value it
doesn't know, a store through a loaded pointer) is written by hand or left
recompiled.

A call passes every argument register its callee reads, not only r3 to r8:
r9 and r10, and f1 to f8 for floats. Which ones a callee reads is found by
following its code (its blocks and its own calls); each is set as the
original sets it before the call. A call that drops one (the countdown's
movie start with no r9, the point tables' calls with no f1) leaves the
callee whatever the register last held.

## Subsystems

| Flag | File | What | Differences from ITC's build |
|---|---|---|---|
| `native_menu_sounds` | `src/hooks/menu_sounds.cpp` | MenuSound::Play `sub_82110318`, Sound::Play `sub_82173710`, SoundOptions::SetEffectsVolume `sub_821B80C8` and SetMusicVolume `sub_821B8198`; the sound manager's calls stay recompiled | Sound::Play's slot call returns to `0x8217374C` (ITC `0x82173750`) |
| `native_data_files` | `src/hooks/data_file.cpp` | DataFile::GetEntry `sub_82292170` (the containers' entries: light sets, course and car data, the font) | none |
| `native_framework` | `src/hooks/framework.cpp` | The framework's getters and its interfaces' thunks, DebugText::SetPosition `sub_82091120`, DebugFont::SetPosition `sub_82571988` | `sub_82090858` reads +80 (ITC +88). ITC's run loop containers and constructors need its heap: not done |
| `native_car_specs` | `src/hooks/car_specs.cpp` | The car parameter tables (assets 72-83): the loads' byte swaps, the getters, the aero parts; Car::Refresh `sub_820C47A8` with CopySpec and CopyAeroParts. Tire::InchUp and memcpy stay recompiled | SBX's car struct has its settings 4 bytes earlier from +1652 on (engine stage, tires, inch-ups, wheels, aero levels); Car::Refresh and its two callees are written from SBX's code |
| `native_menu_helpers` | `src/hooks/menu_helpers.cpp` | MenuCursor::Update `sub_82184468` and MenuCursor::Pressed `sub_821843C8` (`docs/menus.md`); the action mapper's slots stay recompiled | none |
| `native_race_hud` | `src/hooks/race_hud.cpp` | RaceHud::RunMovie `sub_82211F50` (which UI movie the race HUD runs by its state); the movies stay recompiled | none. RivalLogo::Draw `sub_8220F7B0` too, called from subtick's override (`src/hooks/native_calls.h`) |
| `native_data_tables` | `src/hooks/data_tables.cpp` | The data tables at `[0x826B5980]` (the item records, asset 94, and the 11 tables loaded with them): DataTables::Init and Load, the loads' callbacks and byte swaps, the lookups | The item records are 3,004 bytes (ITC 3,008). SwapWord `sub_820C9578` is shared with other loaders and stays recompiled; the swaps here swap the integer |
| `native_file_layer` | `src/hooks/file_layer.cpp` | The file classes' constructors and destructors (File, AsyncFile, GameFile), Path and GamePath, Path::Build `sub_82579250`, GamePath::SetGame `sub_820A6BA8`; opening and reading files stay recompiled | SBX's own vtables and format strings |
| `native_track_zones` | `src/hooks/track_zones.cpp` | The HUD's route zones each tick: the fixed-size lists, RouteRange's tests, Zone::Update, TrackZones::Update and IsInside, Racers::Vehicle `sub_820C94B8`. The once-a-race set-up (TrackZones::TrackZones, Create, the 70 zones' set-ups) stays recompiled | Racers::Vehicle reads the racers at `[0x826C7D9C]` + 4 x (racer + 717) (ITC +577) |
| `native_race_scene` | `src/hooks/race_scene.cpp` | The race scene at `[0x826C7D7C]`: RaceScene::RaceScene, Create, Destroy, DestroyCars; RaceLights::InitEntry and SetUp; Glows::SetParams `sub_8228C498` and RaceScene::SetEffectParams `sub_8228C9A8` (through the effects' vtables, so `--bloom` still applies); RaceSceneMachine::TogglePause and Resume. RaceSceneMachine::Init stays recompiled (it copies stack words it never writes) | none |
| `native_shader_objects` | `src/hooks/shader_objects.cpp` | The graphics library's shader objects: ShaderFile::Parse, the vertex and pixel shaders' Init, CopyHeader and Create, Mesh::LoadShaders `sub_8257A620`, Shaders::OnPairLoaded `sub_820A08B8`; the allocations, XMemCpy and memcpy stay recompiled | The object's size test masks 0x7F (ITC 0x3F); VertexShader::Init's fetch words at +68 and constants at +196 (ITC +64, +192); the vertex microcode XMemAlloc'd 0xA5800000 (ITC 0xB5800000) with a 0x0BADF00D word after each microcode; the loaders' vertex microcode copied with XMemCpy and AllocPhysicalEx'd with flags 4 (ITC memcpy, 0x404) |
| `native_screen_objects` | `src/hooks/screen_objects.cpp` | The race's screen object: SetSpeed `sub_8212E928` (every tick), Release, the destructor. Its UvMap and UvColumn trees (the game's std::map, on its heap) and the rings' set-up stay recompiled | The strength at +52 (ITC +76), the UvMap at +76096 (ITC +76124); SetSpeed calls `sub_8212E440` with 0.0 when the object is running and goes on (ITC returned) |
| `native_scene_models` | `src/hooks/scene_models.cpp` | The scene models' records (the car, parts, interior, cockpit, wheels and pool 4 records' defaults and copies), the parts' links, the accessories' getter, the sound loads' callback, SceneModel::Update `sub_820D9E48` (every frame) and the Apply steps. memcpy and the models' builds stay recompiled | CarRecord::Copy's memcpy is 44 bytes (ITC 48) and its fields are at SBX's own offsets; SceneModel::Update tests the scene's +5000 |
| `native_save_data` | `src/hooks/save_data.cpp` | The save data's defaults (stats, progress, options, control maps, course records), a slot's create and delete, the players' controllers, MenuList::Init `sub_82209868`, TitleScreen::ResetPlayer. Progress::SetDefaults (`unlock_cut_cars`) is called, not replaced | The stats' and options' layouts as SBX's code has them (the tables); ControlMap::SetDefault's 15 buttons |
| `native_accessories` | `src/hooks/accessories.cpp` | The interior accessories' set at `[0x826B51F0]` (create, scale, acquire, release) and the 36-byte AccessoryFile (its load and callback). The archive, the entries' and accessories' own constructors and destructors, their skeletons and textures stay recompiled | none found |
| `native_strings_and_font` | `src/hooks/strings_and_font.cpp` | The string table at `[0x826B5914]`: Init, Release, the load checks (the font's too), Strings::Get `sub_8210A930`, a text's pages. The font's glyph, layout and draw code and the formatting (swprintf) stay recompiled | The table is 2,256 bytes with its counts at +2188 (ITC 4,304 and +4236) |
| `native_screen_constructors` | `src/hooks/screen_constructors.cpp` | 35 of the front end's screen constructors (panels, groups, node tables, list and node screens, the type 6 and 7 screens' sets). Six stay recompiled: the two panel screens (they allocate and test a member), the two list screens and PanelSet (they store through a pointer they load or get), ColorListScreen (it copies a word it loads) | The stores are SBX's own (tables) |
| `native_game_modes` | `src/hooks/game_modes.cpp` | The modes' state machines (StateKey::Equals, StateMachine::IndexOf, Add, Find, the constructor), Modes::Modes, Mode::Mode, the race overlays machine's flags. StateMachine::Handle `sub_82106410` too, called from `title_screen.cpp`'s override (`src/hooks/native_calls.h`); the race mode's and the overlays machine's set-ups stay recompiled | none found |
| `native_collision` | `src/hooks/collision.cpp` | The collision shapes: the box shape's getters and edge tables, the shapes' and queries' constructors, CollisionShape::UpdateBounds `sub_820BAA50` (every tick), the placements, a contact's set and flip, the shape's translate. The vector library, the box-box tests, the impacts and the pairs' response and solve stay recompiled | none found in these; three of the getters are shared by the linker with other classes (the same constant or field) |
| `native_car_physics` | `src/hooks/car_physics.cpp` | The car physics' set-ups and accessors: the car data's, vehicle's, rigid bodies' and collision objects' constructors and resets (tables), the collision surfaces' values, the vehicle's input setters and ramps, the wheels' surface values, the wheel records' copies, the rigid body's setters. The vehicle's step (suspension, tires, drivetrain, gearbox, aero, the rigid body's integration), the collision world and the tuning stay recompiled | The collision surface D's setter writes another default slot than its getter reads (as the original) |
| `native_vehicle` | `src/hooks/vehicle.cpp` | The vehicles' construction, refresh and removal, Vehicle::UpdateTrackPositions `sub_820C1B98` (every tick), TrackPosition::Copy, the teardown. The route positions' update, the input and the set-ups stay recompiled | none found |
| `native_race_states` | `src/hooks/race_states.cpp` | The race state machine's simple handlers (its step, the fades, quit, unload, the standing start, begin), RaceProgress::Reset and CountTime, and the route helpers each racer's progress uses every tick (Time::Sub, RoutePosition::NotAfter, RouteSections::Before and Beside). The race's ticks, the route following, laps, checks, finish and ranking stay recompiled | none found |
| `native_material_passes` | `src/hooks/material_passes.cpp` | The part models' materials: counts and records, the alpha setters, the paint table's lookups, the two block-set tests. The paints, palette, textures and culling stay recompiled | none found |
| `native_replay_snapshots` | `src/hooks/replay_snapshots.cpp` | 13 of the race snapshots' copies (the bodies' states, the cars' parts and members, the track positions), field by field. The replay's own save and restore, the copies that loop over counts they read, and Member1136::Copy stay recompiled | The copies are SBX's own (generated from its code) |
| `native_ui_movies` | `src/hooks/ui_movies.cpp` | The UI movies' file lookups (animations, node tracks, keys, sub-animations, vertices), the two magic checks, two node lookups, the movie's constructor and its named animations' destroy. The movies' update, text, tracks and drawing stay recompiled; the update and the track step are subtick's | none found |
| `native_data_load` | `src/hooks/data_load.cpp` | The boot data's set-up: BootData::Load and CreateObjects, the race data's, input recordings' and replay's creates and their records' constructors, the defaults' bit sets, the loading screen's load. The heap, the archive and the other boot steps stay recompiled | none found |
| `native_race_sounds` | `src/hooks/race_sounds.cpp` | The race sounds' handlers (the listener, the countdowns, the rolling start, the next stage, an abort, leaving after the race) and two of the cars' sound steps. The sounds' ticks, the music's pick and the cars' updates stay recompiled | none found |
| `native_race_screens` | `src/hooks/race_screens.cpp` | The race screens' page steps, the menu lists' selection and entries, the menus' and dialog's answers, the results screens' creates and starts, four closes and inits. The screens' ticks and draws stay recompiled | none found |
| `native_race_hud_object` | `src/hooks/race_hud_object.cpp` | The race HUD object: its gauges, gear, boost, player, warning, draw, item, touching test and teardown, the movies' starts and runs, PowInt. The UI movies, the HUD's set-up and its racers' ticks stay recompiled; RaceHud::RunMovie is `native_race_hud`'s | none found |
| `native_course_scenery` | `src/hooks/course_scenery.cpp` | The scenery pools' objects (flags, keys, data, constructor) and lookups, the scenery's physical allocations, a config's model asset, the course set's resets, Animation::Advance `sub_82145350` and AnimationSet::Advance (every frame). TextureAnimations::Advance is subtick's; the scenery's lists, parse, loads and draw stay recompiled | none found |
| `native_scene_loaders` | `src/hooks/scene_loaders.cpp` | The scene loaders' constructors and destructors (the car model's, cockpit's, traffic's, wheels' and parts' loads), the pool 4 objects' and wheels' resets, records and releases, the load checks (CarModel::IsLoaded, Pool4Object::IsLoaded, XPR::Count). The loads, the builds, the palettes and the searches stay recompiled | none found |
| `native_scene` | `src/hooks/scene.cpp` | The scene set-up's, scene assets' and car scene's constructors, the assets' destructor and release, a list's destructor, the pages freed in a range. The scene's updates and teardown stay recompiled | none found |
| `native_second_set` | `src/hooks/second_set.cpp` | The scene parts' point tables (the parts', cockpit's, interior's, lights', traffic's and wheels' points: constructors and defaults), the point records' names, the models' stream getters, Morph::Run. The draws, the morph's blend and the parts' releases stay recompiled | none found |
| `native_load_poll` | `src/hooks/load_poll.cpp` | The wheels' and lights' records and caliper texture, the parts', interior's, cockpit's and traffic's paints as their loads finish, CarModel::IsReady, the gauges' table. The polls themselves stay recompiled | none found |
| `native_car_makers` | `src/hooks/car_makers.cpp` | A garage car's reset, the garage cars' static set-up, a car record's bytes reset. The garage stage's set-ups and reapplies stay recompiled | none found |
| `native_texture_compression` | `src/hooks/texture_compression.cpp` | The blitter's constructor and the image codec's destructor. The encoders stay recompiled | none found |
| `native_save_interface` | `src/hooks/save_interface.cpp` | The save slots' getters, a device's request, the signed-in users, the save interface's calls into the app's devices and users, SaveSlots::OfController, PlayTime::Update, the devices' root name. The content calls and the interface's set-up stay recompiled | none found |

## Testing a subsystem

Set its flag in F4 (or `--native_<name>=true` or `=false`), restart (the
loads run at boot), and play what it touches. Subtick is on by default, so
the tests below run with it on:

- `native_menu_sounds`: menu sounds, the sound and music volume options.
- `native_data_files`: boot, a race (course and car data), the menus' text.
- `native_framework`: boot to a race; the debug text if shown.
- `native_car_specs`: the garage (car stats, tuning shop, tires, aero parts,
  wheels) and a race. A wrong swap or offset shows as wrong stats or a
  crash at boot.
- `native_menu_helpers`: up and down in the main, garage and pause menus
  (hold a direction to repeat), left and right on pages that step with them,
  accept and cancel.
- `native_race_hud`: a race's HUD movies: the countdown, the timer, the
  race-end overlays.
- `native_data_tables`: boot, then the shop's and the parts lists' names and
  prices, a rival battle's music. A wrong swap shows as wrong names or
  prices, or a crash at boot.
- `native_file_layer`: boot, a race, an autosave and a reload; the patch
  loader's files still load (its log).
- `native_track_zones`: a free run across several highway sections and a
  rival battle: the HUD's road names and signs.
- `native_race_scene`: start a race, pause and resume a few times, exit to
  the garage, start another: the cars' lighting, the glow, no hang on exit.
- `native_shader_objects`: boot (the font's and the menus' shaders), a race
  (the cars' and the course's meshes): a wrong shader shows as missing or
  garbled drawing, or a crash at a load.
- `native_screen_objects`: a race at speed (the screen effect's strength),
  then exit to the garage (its release).
- `native_scene_models`: the garage (a car's parts, interior, wheels and
  decals as tuned), the tuning shop's changes, a race (the cars' models).
- `native_save_data`: a new game (the defaults), the options and the
  controller settings, a save and a reload, the title screen after a
  return to it.
- `native_accessories`: a car with an interior accessory in the garage and
  a race (cockpit view).
- `native_strings_and_font`: boot, the menus' text in each language, the
  pages of the help and the messages (paged text).
- `native_screen_constructors`: every front-end screen (type 6 and 7
  menus): the main menu, the garage, the shop, the tuning pages.
- `native_game_modes`: boot to a race and back (every mode change goes
  through the state machines), pause and resume, the race overlays.
- `native_collision`, `native_car_physics`, `native_vehicle`: a race on a
  few cars (the handling, the throttle, brake and handbrake, the surfaces:
  road, grass, walls), contact with the walls and other cars, a reset onto
  the road after a crash. A wrong value shows as a car that drives
  differently, goes through a wall or flips.
- `native_race_states`: a race from the start (rolling and standing), its
  countdown, laps, the finish and its fade; a quit from the pause menu.
  The HUD's positions and times.
- `native_material_passes`: the garage and a race: the cars' paint, their
  parts' transparency (windows, lights).
- `native_replay_snapshots`: a race, then its replay from the start to the
  end; restart a race (the state restored).
- `native_ui_movies`, `native_race_hud_object`, `native_race_screens`: the
  menus' and the race HUD's movies (the gauges, the gear, the boost, the
  timer, the warning), the pause and after-race menus and their answers,
  the results screens.
- `native_data_load`: boot (every object it makes), a race and its
  replay.
- `native_race_sounds`: a race from the countdown to the end and back to
  the garage, a rolling start, an abort from the pause menu, a rival
  battle's next stage: the music's start, stop and volume.
- `native_course_scenery`: a free run over the whole highway: the
  scenery loading in and out, the animated textures (signs, lights).
- `native_scene_loaders`, `native_scene`, `native_second_set`,
  `native_load_poll`, `native_car_makers`: the garage (every car, its
  parts, interior, wheels, lights and paint as they load), the tuning
  shop's changes, a race with traffic (the traffic cars' models and
  lights), back to the garage.
- `native_texture_compression`: a car's custom decals or paint in the
  garage (the blitter and the codec).
- `native_save_interface`: the save and load screens (a device chosen, the
  saves listed, free space), an autosave, the play time shown.
