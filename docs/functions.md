# Known functions

Names for the generated `sub_XXXXXXXX` functions, worked out so far. The
address is the function's location in the original `default.xex`, so it also
matches Ghidra or IDA. Names are our own guesses; the game shipped without
symbols. The data files the code reads are described in
`docs/game_data.md`.

Status:

- **Native**: plain C++ in `sbx_recomp/src/` that doesn't use the
  PowerPC context (`ctx`): normal arguments and host structs (it may
  still read and write guest memory through `src/guest.h` while
  recompiled code shares the data). The game reaches it without `ctx`,
  from native or overridden callers, and the recompiled original is no
  longer called (except behind a fallback flag or to check against it).
  The end state for every function.
- **Overridden**: an `extern "C" REX_FUNC(sub_XXXXXXXX)` override in
  `sbx_recomp/src/` (or a macro defining one) does the work instead of
  the original: a thin adapter in front of native code, or a body that
  still uses `ctx` (calling guest code, reading registers, or a copy of
  the generated body with an edit, marked `PATCH:`). It may call the
  original when its flag is off, or to check itself against it.
- **Wrapped**: an override that does something of its own (skipping
  render-only passes, logging, timing) and still calls the original
  (`__imp__sub_XXXXXXXX`) for the work.

The generated code is never edited in place: every build regenerates it.
A function usually moves from recompiled or Wrapped to Overridden to
Native as the `ctx` edge moves up through its callers.
`sbx_recomp/tools/check_function_statuses.py` checks this column against
`src/` (an override marked Native, a status with no override, an override
with no row); run it after changing either.

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_82090108` | `main` |  | Guest entry point on the main thread. |
| `sub_825739E0` | `Thread::Start` |  | Entry point of the game's threads: calls vtable slot 1 of the object at start context + 8. |
| `sub_82095FD8` |  |  | A worker thread's loop (started through `sub_825739E0`): waits on an event, runs jobs through function pointers, signals events; it opens the game's data files at boot. Busy only around loads. |
| `sub_8209A8F0` | `App::Run` |  | Main loop: calls every handler in the tick list at this+4, repeats until the quit flag at this+12 is set. |
| `sub_8209A400` | `ForEachTickHandler` |  | Calls each handler in a list through a member function pointer. |
| `sub_82093280` | `Renderer::Init` |  | Sets the vblank interval (2 = 30 fps) via `sub_82573100`, then subscribes renderer callbacks to numbered events (2, 3, 5, 6, 16). |
| `sub_82092E60` | `Renderer::WaitForFrame` | Wrapped | Tick handler on the main thread. Blocks until the render thread signals the frame is done (event at this+392), swaps, and starts the next frame. With `--subtick`, then paces the coming pass (`src/subtick/renderer_hooks.cpp`, see "Subtick"). |
| `sub_82092050` | `Renderer::ThreadLoop` |  | Render thread loop; started through `sub_825739E0`. Draws one frame each time the main thread asks, after a 2-vblank wait. |
| `sub_82091B60` | `Renderer::Present` |  | Flushes, waits for the vblank interval (`sub_825737E8` → `sub_82573638`), then swaps buffers. Event 5. |
| `sub_82091FB0` | `Renderer::DrawFrame` | Wrapped | Render thread: calls each draw listener in the list at renderer+28. |
| `sub_82091CC0` |  |  | Draw listener between the scene draw and present: resolves the frame into a front buffer (`sub_82573538`) and waits for the GPU to go idle (`sub_8229CB08`). |
| `sub_82098DF0` | `Scene::Update` | Wrapped | Tick handler on the scene object (0x40026FE0 at the menu). Calls the current mode object at +64 (vtable slots 6 and 2). |
| `sub_82098E68` | `Scene::Draw` | Wrapped | Draw listener on the same object. Calls the mode object's draw (vtable slot 5), reading live game state. |
| `sub_8216F0F8` | `Delegate::Invoke` |  | Calls the function at +8 with the argument at +4. Used for tick and draw listeners. |
| `sub_8222E468` | `MemberDelegate::Invoke` |  | Calls the member function at +8 on the object at +4, adjusted by +12. |
| `sub_82573100` |  |  | Receives the frame interval from `sub_82093280` in r6 (2) and stores it at +0x130 (0 and 1 store 1, 2 stores 2, 4 stores 3, 0x80000000 stores 0); `VideoOutput::WaitForVblank` waits that many vblanks before each present. The `--tick_rate_60` switch that set it to 1 was removed 2026-10-25 (see "Tick rate experiments"). |
| `sub_82573638` | `VideoOutput::WaitForVblank` | Overridden | r3 the video output: calls the device (`sub_8229DD58` with [+140]), then waits for the vblank interval at +0x130 (+304). With `--subtick` only the device call runs; the pacer holds the rate (`src/subtick/renderer_hooks.cpp`). |
| `sub_82573920` | `Event::Wait` |  | r3 = event, r4 = pointer to timeout (-1 = forever). |
| `sub_82573908` | `Event::Signal` |  | r3 = event. |
| `sub_822AE7E8` |  |  | Graphics driver present bookkeeping (frame counter at +20024). Library code, not game timing. |
| `sub_8259F788` | `QueryPerformanceCounter` |  | System library. |
| `sub_820BFF60` | `Vehicle::GetSpeedKmh` |  | Returns the float at vehicle+48 (m/s) times 3.6. |
| `sub_820943D8` |  |  | First tick handler: polls input, then walks listener lists that run both game logic and the draw step. |
| `sub_82106478` | `GameMessages::Dispatch` | Overridden | Delivers the game's queued messages to their handlers: 0 = Update (all game logic), 1 = Draw. r3 = the dispatcher: handlers +0 (array), handler count +200, queue of 50 12-byte messages at +204, queued count +804, read index +808. The game has several dispatchers (40 call sites). Replaced with the same steps (`src/subtick/game_messages.cpp`) with `--subtick` (Update skipped on render-only passes); otherwise the original. |
| `sub_82106410` | `StateTable::Dispatch` | Overridden | Picks the handler object a message goes to; its method 0 handles it. r3 = a handler object, r4 = the message: returns the entry for (the object's state at +9604, the message id), `0x82695848` when there is none, and moves +9604 to the entry's next state (`docs/menus.md`). Wrapped by `src/hooks/autosave.cpp` to watch the garage menus (from `src/hooks/title_screen.cpp`'s override), which calls our C++ for it with `--native_game_modes` (`src/hooks/game_modes.cpp`). |
| `sub_82099700` | `Input::Update` | Wrapped | r3 the input manager: physical pads (+988, 4 x 212, `PhysicalPad::Read`), logical pads (+8, 5 x 196, `LogicalPad::Build`), processors (+1840, 5 x 416, vtable slot 1), listener slots (+3924, 5 x 12; the first node's object, vtable slot 1). Skipped on render-only passes (`src/subtick/input.cpp`). |
| `sub_82159540` | `Race::DrawStep` | Wrapped | The race's draw step (Draw message). Writes a field at +64 of the object its calls return. Noted to tell races from menus (`src/subtick/game_messages.cpp`). |
| `sub_820A3B98` | `CameraPass::Update` | Wrapped | Scales the main camera's projection after the update (`--camera_fov`, `CarTransforms_CameraFov`), then rebuilds the view-projection. Rebuilds a camera pass's view (+0x10) from its eye, target and up vector (+0x190, +0x1A0, +0x1B0), and its view-projection (+0x90 = view * projection at +0x50). Called several times per Draw for pass 0. On pass 0 in a race, built from the camera blended between ticks; also run on pass 0 before `Race::DrawStep` on render-only passes, so the cars are drawn with the frame's view-projection (`src/subtick/car_transforms.cpp`, see "Subtick"). |
| `sub_8209BE70` | `Matrix::Multiply` |  | out (r3) = a (r4) * b (r5), 4x4. |
| `sub_8257ACD0` | `Recorder::RecordObject` | Wrapped | The bulk per-object recorder, ~150 calls per pass in a race: records command 7 with vertex constants 0-8: world (r5) * camera view-projection (r4) in c0-c3, the world matrix in c4-c7 (for some objects a projective matrix instead), and the camera's world position in c8. The camera is often a stack copy, not a camera pass. |
| `sub_8257AAE0` |  |  | Multiplies a world matrix and camera into a stack temporary; not recorded (likely a visibility test). |
| `sub_825758A0` | `Command::SetShaderConstants` | Wrapped | Render thread, command 7: r3 = payload, r4 = register count, r5 = stage << 24 \| type << 16 \| start << 8 \| count. |
| `sub_82572638` | `CommandList::AllocateSlots` |  | r3 = list, r4 = slot count; returns the first 16-byte slot. |
| `sub_820BFF88` | `Vehicle::GetDistancePerFrame` |  | Speed (m/s) times a hard-coded 1/30 s. |
| `sub_820C2E40` | `Vehicle::Tick` | Original | Once a tick per car: r3 = car, r4 = its index in the control record table at `0x826B5810`, r5 and r6 for `sub_820C2208`. Stores speed x `[0x820029B8]` at car+6160, copies car+5236 (4 bytes) to car+6164, then twice: `Vehicle::ApplyControls`, clears the record's one-shot bits (`0x00010003`), `Vehicle::Step` with `[0x825F17E8]` (1/60), `sub_820C1B98`. Then `sub_820C2208` once. Called by `sub_82163CC8`, `sub_82164188`, `sub_82171950` (race modes) and `sub_8216F398` (every car). See "Vehicle stepping". The game's own (the weighted input is written at `PlayerControls::Build` instead). |
| `sub_820B6330` | `Vehicle::Step` | Original | r3 = car, f1 = step length. `Vehicle::SetStepLength`, then the car's physics parts (car+0x930 read by 9 of them), with an inner loop of car+0x940 (10) iterations (`sub_820B0600`, `sub_820B0720`, `sub_820B3E18`, `sub_820B07F0`, `sub_820B25E0`, `sub_820B4610`). Called by `Vehicle::Tick` and, once each with 1/60 from `0x82609328`, by `sub_8216E590` and `sub_821703B8` (placing cars). |
| `sub_820AF9D0` | `Vehicle::SetStepLength` |  | Stores f1 at car+0x930 and 1/f1 at car+0x934. Nothing under `Vehicle::Step` reads +0x934 or the 60.0 `sub_820B0DD0` stores at +0x938. |
| `sub_820BFB68` | `Vehicle::ApplyControls` | Original | r3 = car, r4 = its control record: steering +8, throttle +4, brake +6, +10, flags +0 (bits pick digital or analog handling of each), bytes +14..+17 (+17 the control type). Rates are scaled by `[0x825F17E8]`. Bits 0x1 and 0x2 (one-shot) and 0x2000 only set request bytes (car+2284, +2285, +2286). |
| `sub_820B1678` |  | Original | Part of `Vehicle::Step` (a leaf): counts car+0x958 up by 1 per step while a slip test holds (to 30, the 30.0 at `0x82000D54`, which is also the test's threshold), down otherwise, and scales car+284 by 1 - counter x 1/30 (car+284 is worked out again later in the step by `sub_820B1EB0`). Counts steps, so it's only right at the game's two steps a tick; the car physics doesn't change its step count. |
| `sub_820C1B98` |  | Overridden | Each step: updates five tracked positions (the car, `sub_82249148` of car+336, into car+5232+16; then 4 wheels, 368 bytes apart) through `sub_820F8350`. |
| `sub_820C2208` |  |  | Once a tick (r3 = car+6160, r4 = car): the car's course position, then the odometer: speed x 1/30 x a constant added to +84 and +88. |
| `sub_8216F398` |  |  | Calls `Vehicle::Tick` for every car in `[0x826C7D9C]`+0xB34 (count +0xB6C). |
| `sub_8215DBF8` | `PlayerControls::Build` | Wrapped | r3 the control record, r5 the pad, r6 a message dispatcher: builds the record from the action mappers through `Input::QueryAbstract` (ids from the table `[0x826B5838]` + pad x 120 + 0x1F7C0; throttle +4, brake +8, steering +0 after abstract id 1, +12, +48), each an integer curve with a dead zone; also posts messages 30 and 29 for one button. Reached through a vtable (`sub_8215E390`). Its record is the tick's last input sample (`src/subtick/input.cpp`). |
| `sub_821024D8` | `Input::Query` |  | r3 the pad, r4 an action id (151-180), r5 the kind (0 held, 1 pressed, 2 fired, 3 a u16 analog value, 4 a float analog value), r6 the result: the action mapper's read slot. |
| `sub_82102610` | `Input::QueryAbstract` |  | r3 the pad, r4 an abstract id (0-30), r5, r6 as `Input::Query`: maps it to one or two actions (e.g. 155 and 156 combined, one negated). |
| `sub_82105790` |  |  | Returns `[0x826B5838 + 4 x r3]`. |
| `sub_82577498` | `PhysicalPad::Read` |  | r3 the pad, r4 a pointer to the user index: XInput state into 40 floats at +8 (cleared each read), flags +4, rumble motors +168 and +180 (ticked down each read). |
| `sub_82577150` | `LogicalPad::Build` |  | Sums its bound pads' 40 values and clamps them. |
| `sub_8210C218` | `ActionMapper::UpdateActions` |  | r3 the mapper + 448: `ActionMapper::UpdateAnalog`, then `ActionMapper::UpdateHysteresis` per action and `ActionMapper::UpdateButtons`. |
| `sub_8210B908` | `ActionMapper::UpdateAnalog` |  | The analog value of each action: u16s at mapper+1072, floats at +1132, through response curve objects (vtable slot 1). |
| `sub_8210B850` | `ActionMapper::UpdateHysteresis` |  | An action's on/off thresholds and state: 20 bytes each at mapper+1852. |
| `sub_8210BCD8` | `ActionMapper::UpdateButtons` |  | Held, held before, pressed, released and fired words at mapper+448..+467; steps each action's repeat object (pointers at +472, vtable slot 1). |
| `sub_8210B6A0` | `Repeat::Step` |  | A repeat object: count +4, delay +8, period +12. |
| `sub_8215C0A0` |  |  | The race overlays' Update handler: `RaceOverlays::Run` on r3 + 9608. |
| `sub_8215C0A8` |  |  | The race overlays' Update handler while paused (ITC's `8215E158`): `RaceOverlays::Record` on r3 + 9608. |
| `sub_8226F9D8` | `Color::HsvToRgb` |  | A leaf: hue (degrees) x 1/60 (`0x820029C0`) gives the sector, then saturation and value. Likely converts the car paint colors (`docs/game_data.md`, car specs): halving its 1/60 swapped them. `sub_821E6F28` is a copy. |
| `sub_8210F260` |  |  | Wind sound; scales with speed. |
| `sub_8213DE38` |  |  | Parent of the speed trail object (switches mode at about 88 km/h). Calls `sub_8213DBE8`, `sub_8213DA48` and `sub_8213DDB8`. |
| `sub_8213BEB8` |  |  | Resets the trail history; runs on a camera switch. |
| `sub_820F1038` | `TextureAnimations::Advance` | Overridden | From the draw step part `sub_820F5F48`: advances the flipbook animation set at r3+1060 (`sub_821453B0`). Skipped on render-only passes (`src/subtick/per_frame_logic.cpp`); otherwise our C++ for it with `--native_course_scenery` (`src/hooks/course_scenery.cpp`). |
| `sub_821453B0` |  | Overridden | Calls `sub_82145350` on every animation in a set: count at +0, then offsets to each animation. |
| `sub_82145350` |  | Overridden | Advances one flipbook animation: frame count +0, current frame +4, tick counter +8 (wraps at the length at +12), frame start ticks from +16. |
| `sub_82145410` |  |  | Returns the current frame's entry of animation r4 in set r3 (texture id at +4). |
| `sub_821452C8` |  |  | Finds the animation in set r3 whose texture is r4; -1 if none. |
| `sub_82146F00` |  |  | Draws the light groups: for each group, builds a light list on the stack (count +32, 48-byte lights at +36, from static level data) and picks its texture's current animation frame. Calls `sub_8209CED0` → `sub_8209CD58`. |
| `sub_820F5610` | `Minimap::Draw` | Wrapped | From the draw step part `sub_820F5F48`; steps the minimap blink clock at `0x826B5784`/`0x826B5788` after drawing. Both put back on render-only passes (`src/subtick/per_frame_logic.cpp`). |
| `sub_82107A38` |  | Wrapped | From Draw, every pass; adds one to the play time frame counter at `0x826B5870` and runs `ActionMappers::Update` (call at `0x82107B48`). The one is taken back on render-only passes, and every other pass with `--tick_rate_60` (`src/subtick/per_frame_logic.cpp`). |
| `sub_82110928` |  |  | Turns the play time frame counter into hours, minutes and seconds (30, 1800, 108000 frames). |
| `sub_82107CB8` | `Boot::StartMovie` |  | Message handler: plays the next boot movie, id from the table at `0x826B588C` (index at `0x826B58A8`), via `sub_820A45D0` (name table at `0x8233AF8C`). |
| `sub_82107D80` | `Boot::WaitForMovie` | Wrapped | Message handler, every tick while a boot movie plays; ends the step on a button press if skippable (`0x826B58A4`) or when the movie has finished. |
| `sub_82107D20` | `Boot::EndMovie` |  | Message handler: stops the movie; the front end (Genki logo, title) follows. |
| `sub_820A4678` | `Movie::IsFinished` | Wrapped | 1 when the playing movie has finished. |
| `sub_82292F48` | `CreateFile` | Wrapped | The only caller of `NtCreateFile`; r3 = path. |
| `sub_8221FC48` | `TitleScreen::Update` | Wrapped | Counts idle ticks on the title screen and starts the attract race at 600. |
| `sub_820F3FF8` | `Lights::FindNearest` | Overridden | Collects the lights nearest a point, nearest first. Arguments: r3 the light set owner (list at r3+216: count +0, array +8 of 272-byte entries, each's +12 → +80 a light set), r4 the output, r5 the point, r6 the most lights to keep (at most 32; 0 returned above), r9 a direction vector, f1 a merge radius, f2 the farthest squared distance, f3 the least dot product. For each light (48 bytes: position xyz at +0, 32 bytes copied out): skipped beyond f2, or when the direction to it dotted with r9 is below f3 (a cone); within f1 of a light already kept, it replaces that one if nearer, else is dropped; otherwise insertion-sorted by squared distance into a 33-entry array on its ~2 KB guest stack frame. Writes 32 bytes a light to r4 (position with w = the float at `0x82000A20`, then the light's +16..+31) and returns the count. A light set's count (+0) is little-endian (byte-swapped after `lwz`); `sub_82292170` gives each of its entries, whose +0 count says how many times the entry's one light group (count +60, lights from +76) is scanned: the group pointer isn't advanced, so a count above 1 scans the same lights again (the merge then drops the copies). Only caller: `sub_820C8F10`. |
| `sub_820C8F10` |  |  | Asks `sub_820F3FF8` for up to 10 lights near a point (r1+112) in a cone (cosine-like value from `sub_8258E218` of a setting at `[r27+11844]+5500` times a constant) of the course scenery manager at `[0x826B51FC]` (its visible areas' light sets). Called from `0x820E19B8`, `0x820E66F0` and `0x820DC028`. |
| `sub_82292170` | `DataFile::GetEntry` | Overridden | Reads a container (`docs/game_data.md`): r3 the container, r4 an index; returns r3 plus the r4-th of the little-endian offsets at r3+4 (the count at +0 bounds r4; 0 beyond it); with r5's low byte set, 0 for an entry whose first word is 0. Light sets are containers; `sub_820F3FF8` passes r5 = 0. |
| `sub_8209E348` | `Archive::Init` | Wrapped | At boot: allocates the two TOC buffers, sized from the entry counts built into the XEX (`0x825F0A90`, `0x825F0AA0`), reads `DATA\BUILD\TXR\BUILD.TOC` and `BUILD_CRS.TOC` into them, and opens `game:\DATA\BUILD\TXR\BUILD.DAT` and `BUILD_CRS.DAT` (`docs/game_data.md`). Called from `sub_821066D0`, which passes the size of the shared load buffer it allocates (0xC80000). Reads both TOCs before it returns; the patch loader then changes the TOC entries of the ids it replaces (`src/hooks/patch_loader.cpp`, `--patch_loader`). |
| `sub_8209D4D0` | `Archive::Shutdown` |  | Frees the TOC and read buffers and closes the two `.DAT` handles. Called from `sub_82106800`. |
| `sub_8209D988` | `Archive::GetToc` |  | r3 a file id: returns the `BUILD` TOC buffer for ids below 23009, else the `BUILD_CRS` one. |
| `sub_8209DF80` | `Archive::Load` |  | r3 a file id, r4 the destination, r5-r8 options: reads the id's TOC entry (start sector, stored and decompressed size), checks `Archive::FindLooseFile`, and queues a read for the loader thread (`sub_8209DD38`). Ids at or above 23009 are in `BUILD_CRS`. r8 is a read size that overrides the TOC's stored size; every caller passes 0. A decompressed size of 0 means the entry is stored as is (no unpacking). |
| `sub_8209D9F8` | `Archive::FindLooseFile` | Wrapped | r3 a file id (the game's, 0-24367): returns the loose file's path from the table at `0x825F0AB0`, or 0. Paths are relative (`DATA\TXR\MOVIE\OPENING.WMV`); the loose read (`sub_8209E0E0`) opens them under `game:\`. Only caller: `Archive::Load`. Returns a PATCH_DATA override's path first (`src/hooks/patch_loader.cpp`). |
| `sub_8258C000` | `operator new` |  | r3 a size; returns the block (retries through the new handler when out of memory). |
| `sub_8209DD38` |  |  | Queues a 40-byte read request for the loader thread. |
| `sub_8209E250` |  |  | Loader thread loop (no direct callers): takes requests off the queue, reads them from a `.DAT` (`sub_8209DE20`) or a loose file (`sub_8209E0E0`), then unpacks compressed ones (`Ucl::Unpack`). |
| `sub_8209DE20` |  |  | Reads a request from `BUILD.DAT` or `BUILD_CRS.DAT` (byte offset = start sector << 11); a failed read calls `sub_8209AD48`. |
| `sub_8209E0E0` |  |  | Reads a request from its loose file. |
| `sub_82292050` | `Ucl::Unpack` |  | Unpacks a UCL stream (`uclpack` layout: 18-byte header, then blocks of BE decompressed size, stored size, data; a stored block is copied as is). Calls `sub_82292238` per block. |
| `sub_82292238` |  |  | Decompresses one UCL block. |
| `sub_820A0860` | `Archive::GetSize` |  | r3 a file id: the TOC entry's decompressed size, or its stored size when that is 0. |
| `sub_820F2E58` | `Scenery::Create` |  | The 248-byte course scenery manager (`docs/game_data.md`, "Courses"), stored at `0x826B51FC`; then `Scenery::Init`. From `sub_82159798` (races) and `sub_82225548` (front end). |
| `sub_820F11F0` | `Scenery::Init` |  | r3 the manager: stores it at `0x826B51F4` and clears its lists and fields. |
| `sub_820F1310` | `Scenery::ClearGlobal` | Overridden | Sets `0x826B51F4` to 0. |
| `sub_82159798` |  |  | Race: makes the scenery manager and sets it up for the race's course: the index from `sub_8228C478` (place 1 for race modes 8-10, else place 0 and the race data's +32), the course table at `0x825F2E40`, streaming mode; then `Scenery::Setup` and `Scenery::StartLoad`. |
| `sub_8228C478` |  |  | r3 x 3 + r4: the race's course index (place, variant). |
| `sub_8228C488` |  |  | (r3 + 2) x 3 + r4: the front end's course index (scene, variant). From `sub_82229DD8`. |
| `sub_820F2EF0` | `Scenery::Setup` |  | r3 the manager, r4 the 44-byte configuration: `Scenery::SetupPools`, then one 272-byte visible-area record per area at +224. |
| `sub_820F25B0` | `Scenery::SetupPools` |  | Copies the configuration to +8; builds the light key table (+140, count +128) and the model key table (+168, count +156) from the course record at `[+36]`; sets up the five pools (sizes 60, 60, 3824, 88, 560). Pool 4's buffer is the shared load buffer (+188) when it's large enough. |
| `sub_820F1EC0` | `Scenery::PoolSizes` |  | The area and model counts and each pool's data size (mode 0: the sum, else the largest; models at most 16). |
| `sub_820F13C0` | `SceneryConfig::ModelAsset` | Overridden | r3 the configuration, r4 an area, r5 a model: the model's id (the area's first id + r5), or -1 past its last. |
| `sub_820F1C90` | `Scenery::StartLoad` |  | Loads the course set (pool 2); in mode 0 lists every area and model to load. |
| `sub_820F1DF8` | `Scenery::IsLoaded` |  | Polled until the course is in. |
| `sub_820F23B0` | `CourseSet::Load` |  | Loads set file 1 (`[+36]+12`) into a block of its own and set file 2 (`[+36]+8`) into the object, the last with the callback `CourseSet::OnLoaded`. |
| `sub_820F2148` | `CourseSet::OnLoaded` |  | `Archive::Load` callback: `CourseSet::Parse`, then frees set file 1's block. |
| `sub_820F1900` | `CourseSet::Parse` |  | Set file 1: entry 0 an XPR2, 1 a model, 2-7 lists of models. Set file 2: entry 0 the areas, 1 the light groups' textures and animations, 2 the light groups. |
| `sub_820F2188` | `AreaLights::Load` |  | Loads an area's light file (id from the light key table by the object's key at +84) with the callback `AreaLights::OnLoaded` (`sub_820F1AC0`). |
| `sub_820F1050` | `AreaLights::Parse` |  | +80 = the light file's entry 0: the light set `Lights::FindNearest` reads. |
| `sub_820F2250` | `AreaModel::Load` |  | Loads an area model (id from the model key table by the key at +64) into the manager's +188 (the shared load buffer) with the callback `AreaModel::OnLoaded` (`sub_820F1B78`). |
| `sub_820F10E8` | `AreaModel::Parse` |  | Entry 0 into the model at +80, entry 1 into the XPR2 at +528. |
| `sub_820F4428` | `Scenery::Update` |  | Each update: the areas and models near each camera (`Scenery::CollectAreas` `sub_820F2F68`, `Scenery::CollectModels` `sub_820F31A0`), streamed through `SceneryList::Stream` (`sub_820F1550`). |
| `sub_820F5F48` | `Scenery::Draw` | Wrapped | The race draw step's part that draws the course (also calls `Minimap::Draw`). |
| `sub_820FF4B8` | `TrackCollision::Load` |  | Once a race: the same course index as `sub_82159798`; loads the course record's mesh (+0) and route (+4). |
| `sub_820F7680` | `TrackMesh::Load` |  | r3 `0x826B57FC`, r4 the mesh id: into its own block, callback `TrackMesh::OnLoaded` (`sub_820F6890`). |
| `sub_820F9D08` | `TrackRoute::Load` |  | r3 `0x826B57F0`, r4 the route id: into its own block, callback `TrackRoute::OnLoaded`. |
| `sub_820F8F30` | `TrackRoute::OnLoaded` |  | Turns the route file's offsets into pointers (+12 sections, +16 corners, +20 12 lists); for each set bit of the race data's +24 (bits 0-11), narrows the listed slices' floats +24 and +28 to the listed values. |
| `sub_820FD5A8` | `WallQuery::IgnoresWall` |  | Skips collision walls by their kind, the race mode and stage flags. Not read here. |
| `sub_8213D278` | `AiPath::AddCornersAt` |  | The AI path's corners from a route slice's corner list. Not read here. |
| `sub_8213ADD8` | `AiPath::PreferredSections` |  | The sections an AI profile prefers at forks (a table in `.data`). Not read here. |
| `sub_82289118` | `TrackZones::Update` | Overridden | The HUD's 70 route zones (hard-coded ranges): road names and signs. Calls each zone's update in list order. |
| `sub_8209AD48` |  |  | Read error: ends in `sub_82292D50`, which shows the dirty-disc screen (`XamShowDirtyDiscErrorUI`). |
| `sub_82572370` |  |  | Reads a whole opened file into a buffer (r3 the file, r4 the buffer, r5 a pointer to the size); used for the TOCs and `DEBFONT.XPR`. |
| `sub_820CC048` |  |  | Loads the parameter tables, BUILD 72-83 (72 = car specs) through `Archive::Load`; `sub_820C9700` sizes them first. |
| `sub_821744D8` |  |  | Loads BUILD 22887, the loose `DATA\AUDIO\CAR_SE.XWB`, and starts the music (`Music::PlayRoaming`): a message handler of the scene (`sub_82175238`). |
| `sub_82094BB8` |  |  | Startup; names the XACT engine `txr_xact` and calls `sub_821066D0` (which calls `Archive::Init`). |
| `sub_8228CEE8` | `SaveGame::Write` | Wrapped | One of four functions (with `sub_8228CFA8`, `sub_8228D228`, `sub_8228D370`) that open the save, `TXR360:\TXR_DATA_1`. |
| `sub_8228CDD0` | `SaveGame::DeviceState` | Wrapped | The save device's state. Logged with `--log_autosave` (`src/hooks/autosave.cpp`), as are the four below. |
| `sub_8228CE08` | `SaveGame::Status` | Wrapped | Logged with `--log_autosave`. |
| `sub_8228CE40` | `SaveGame::Pump` | Wrapped | The save's pump. Logged with `--log_autosave`. |
| `sub_8228CD98` | `SaveGame::Finish` | Wrapped | Logged with `--log_autosave`. |
| `sub_8228CFA8` | `SaveGame::Read` | Wrapped | Opens and reads the save. Logged with `--log_autosave`. |
| `sub_82105790` | `GameData::Get` |  | r3 an index: returns the pointer at `0x826B5838` + 4 x r3. Index 0 is the save (`docs/game_data.md`). |
| `sub_82113D28` | `SaveData::Clear` | Overridden | r3 the save: zeroes it and sets its defaults. |
| `sub_82113FB0` | `SaveData::NewGame` | Wrapped | r3 the save: clears it (`SaveData::Clear`) and sets a new game's state, including the car unlock bits at +0x1985C (every car but CON_X, ECLI, S30, L86, T86 and BOSS00). Run by the title menu's new game and at boot on the empty saves. Sets the cut cars' bits after it (`src/hooks/unlock_cut_cars.cpp`, `--unlock_cut_cars`). |
| `sub_82114278` | `SaveData::NewGameKeepCars` | Wrapped | r3 the save: a new game that keeps the garage (+6120), the word at +6116 and the unlock bits at +0x1985C (copies them out, runs `SaveData::NewGame`, copies them back). From the title menu (`sub_8221F568`). Sets the cut cars' bits after it (`src/hooks/unlock_cut_cars.cpp`). |
| `sub_82114D88` | `SaveData::Unlock` |  | r3 the save, r5 what to unlock (a switch of 42 cases), r6 its id. One case sets car r6's bit at +0x1985C if it isn't set yet; new unlocks are queued for the garage notifications. |
| `sub_8217F2A0`, `sub_82272370`, `sub_8224A0E8` |  |  | Build the car lists (the starter cars and the shop): for each of the 72 cars with its bit set at save +0x1985C, copy its specs (`sub_820C9C38`) into list 0 or 1 by spec byte +771 (0 for the road cars, 1 for CON_X and the rival cars). |
| `sub_820C9C38` | `Params::GetCarSpec` | Overridden | r3 a car id: returns its 8,432-byte record in the car specs (BUILD 72). |
| `sub_820C9C50` | `Params::GetWheel` | Overridden | r3 a wheel number: returns record r3 - 1 of BUILD 73 (the wheels), or 0 for 0 (the stock wheel). |
| `sub_820C9BA0` | `Params::GetCarParts` | Overridden | r3 the output (9 pointers), r4 a road car id: points into BUILD 74-82, the aero parts (strides 280, 280, 280, 280, 168, 168, 84, 108, 72). |
| `sub_820CBFE8` |  | Overridden | Load callback for BUILD 72: stores the car count (size / 8432) at params+56 and byte-swaps the records (`sub_820CBF90` → `sub_820CBF08`, one swap per block: `sub_820CA2A0` block 1, `sub_820CBC10` block 2, `sub_820CA780` engine stage, `sub_820CAA48` tire, `sub_820CB1F8` paint, `sub_820CB328` wheel). |
| `sub_820CB1F8` |  |  | Swaps a car's paint block, and fills in each slot's second shade (+12) for solid and metallic paints (`docs/game_data.md`). |
| `sub_820C96C8` | `CarSpec::GetTire` | Overridden | r3 the spec, r4 the tire type (0-4), r5 the grade (1-5), r6 1 for the rear: returns tire 0 for type 0, else (type - 1) x 5 + grade. |
| `sub_821ED5D8` | `WheelShop::MaxInchUp` | Wrapped | r3 the tuning menu: how many inch-ups the wheel shop offers for the garage car it shows, by car id: 20 minus the stock rim diameter, at most 4, on the road cars (RX8 2, RX7 3, N180 4); 0 for CON_X, the cut cars and any id over 22. The cut cars get the same rule (ECLI 3, L86/S30/T86 4) with `--cut_car_wheels` (`src/hooks/cut_car_wheels.cpp`). |
| `sub_820C44C8` | `Tire::InchUp` |  | r3 a tire (a copy), r4 inches: bigger rims with the same outer diameter; recomputes width, aspect ratio, rim width and grip (`docs/game_data.md`, "Tires"). |
| `sub_820C4698` | `Car::CopySpec` |  | r3 a car object, r4 its spec: copies block 1 to car+2240, block 2 to +1888, the engine stage at level [car+1652] to +2880, paint to +3364, and the front and rear tire (with inch-up) to +3084 and +3176. |
| `sub_820C47A8` | `Car::Refresh` | Overridden | r3 a car object (car id at +12): `Car::CopySpec`, then the aero parts (`Car::ApplyAero`) and the front and rear wheels (+3268, +3316). 50+ callers (garage, shop, race setup). |
| `sub_820C4378` | `Car::ApplyAero` |  | r3 a car object, r4 its `Params::GetCarParts` pointers: copies the aero part records at the levels in car+1740 to +1780 to car+3636 to +3875, through one getter per part (`sub_820CB5E8` ... `sub_820CBA30`; level 0 = no part). |
| `sub_820CB468` |  |  | Byte-swaps a 28-byte part record after it's copied. |
| `sub_820C4360` | `Car::GetSpecSheet` |  | r3 a car object: returns car+1888 (its copy of block 2), or 0. |
| `sub_820C2608` | `Vehicle::SetupFromCar` |  | r3 a vehicle, r4 a car object: copies the physics values from block 1 (dimensions, mass, wheelbase, tracks ...), the engine stage (rev limit, torque, +156-196) and the tires into the vehicle, then `Vehicle::ApplyTuning`. |
| `sub_820C0B48` | `Vehicle::ApplyTuning` |  | r3 a vehicle, r4 a car object: applies the tuning at car+1644 using tables of multipliers built in the code (7 entries per part, separate NA and turbo tables), the engine type as a cylinder count, and the aero parts. |
| `sub_822B8A90` |  | Overridden | Returns [r3+8] (with r3 = car+1644: the engine stage level). |
| `sub_820A0A88` | `Shaders::LoadAll` |  | At boot (from `sub_821067A0`): loads every shader pair through `sub_820A09D8`: BUILD 3-70 as indices 0-33, BUILD 22800-22811 as 1000-1005, and BUILD_CRS ids 24256-24367 as 2000-2055. A pair is a vertex shader (the odd id) and its pixel shader (the next id). See "Shaders" in `docs/game_data.md`. |
| `sub_820A09D8` |  |  | r3 the vertex shader's file id, r4 the pair's index: loads the vertex shader and the pixel shader (r3 + 1) with `Archive::Load`. |
| `sub_821364B0` | `Glow::Record` | Wrapped | The glow (bloom) recorder in races: builds the cross-fade quad (pixel shader `E0D070FB58A4EA03`, the vertex color's alpha is the history weight at +8460) and the glow constants c33 (+8436/+8440, +8452 the glow strength, +8456, +8476). |
| `sub_82134EC8` | `View::UploadPreviousViewProjection` |  | r3 the view. Records vertex c100 x4, the motion blur's previous view-projection: copied from view+8080 (this tick's) when view+48 is set (a pass that ran the game logic) and view+44 is 0, else from view+8144 (the previous tick's). The view moves the pair on after the logic pass, so it's the same matrix all tick. Read by the motion blur's vertex shader (`matOldWVPVS`, BUILD 9/67). |
| `sub_820A3088` | `Matrix::RotateZ` |  | r3 out, r4 in, r5 &angle: out = in * RotZ. |
| `sub_820A0E18` | `Matrix::Copy` |  | r3 destination, r4 source, 4x4. |
| `sub_8257CC90` | `Matrix::LookAt` |  | r3 out, r4 eye, r5 target, r6 up: a row-vector view matrix, forward = target - eye. |
| `sub_8257CBC0` | `Matrix::Perspective` |  | r3 out, f1 field of view, f2 aspect, f3 near, f4 far. |
| `sub_8257CF28` | `Projector::Build` |  | r3 out, r4 eye, r5 target, r6 up, f1-f4 field of view, aspect, near, far: look-at * perspective * a fixed bias (a projected texture's matrix, e.g. the headlights). |
| `sub_820D02A8` | `Model::Draw` |  | Draws a model: a visibility test (`sub_8257AAE0`), then `Recorder::RecordObject`. |
| `sub_820D04B0` | `CarPart::Draw` |  | r3 the part (nothing if its model at +4 is 0), r4 the camera, r5 the world matrix; calls `Model::Draw`. |
| `sub_8215A788` | `Car::TickUpdate` |  | The tick's car update: copies the car's world matrix to car+16 and stores the gauge values (speed, revs, boost) at car+696/700/704. |
| `sub_820DA930` | `Car::DrawUpdate` | Wrapped | Draw's per-car update (r3 = car - 4288), on every pass: takes 1.0 off car+704 (the boost; the tick sets it again), then draws the car's parts. The boost is replayed from the logic pass on render-only passes (`src/subtick/per_frame_logic.cpp`). |
| `sub_820E4DC0` |  |  | r3 the dash: calls `Dash::PoseTachometer` when [dash+20] is set, then `sub_820E35C0` when [dash+2180] is. |
| `sub_820E3EB0` | `Dash::PoseTachometer` | Wrapped | r3 the dash, r4 the car, r5 the camera passes: poses the tachometer needle from `Dial::TachometerAngles`. |
| `sub_820E4178` | `Dash::PoseDials` | Wrapped | r3 the dash, r4 the car, r5 the camera passes: poses up to seven needles, each from `Dial::Angle`. |
| `sub_820D32D0` | `Dial::Angle` |  | A needle's angle from a gauge value. |
| `sub_820D3388` | `Dial::TachometerAngles` |  | The tachometer's two angles from the revs. |
| `sub_8211D4B8` | `UiMovie::Update` | Wrapped | r3 the movie, r4 0 = advance a frame and draw, 1 = draw only. Runs the HUD, the race start countdown and the pause menus. Advancing is replayed from the logic pass on render-only passes (`src/subtick/per_frame_logic.cpp`). |
| `sub_82585FD8` | `UiMovie::ApplyAnimation` |  | Applies each playing animation's tracks with t = (time - k0) / (k1 - k0), clamped. |
| `sub_82585C40` | `UiMovie::ApplyTrack` | Wrapped | r3 the node, r4 the track (+0 its property id), f1 t: writes one word into the node. Some tracks add to the value instead of setting it. |
| `sub_82585560` | `UiMovie::TrackProperty` |  | r3 a property id: its 12-byte descriptor (+0 node offset, +4 type). |
| `sub_82582D50` | `UiMovie::Child` |  | r3 the node, r4 an index: that child (count at node+112). |
| `sub_82119D50` | `UiMovie::RestorePose` |  | Copies a movie's rest pose back onto its live nodes. |
| `sub_8210B140` | `ActionMappers::Update` | Wrapped | r3 the mapper list: updates every action mapper (`ActionMapper::Update` each): held, pressed and fired words, and the menu repeat. Counts calls, so it must run once a tick: skipped on render-only passes, except when `src/subtick/input.cpp` runs it for the analog values. |
| `sub_8210C4A8` | `ActionMapper::Update` | Wrapped | r3 the mapper: raw axes from the logical pad (`[+2880]`) into +388..+446, then `ActionMapper::UpdateActions`. Digital state put back after it when run for the analog values (`src/subtick/input.cpp`). |
| `sub_82209F78` | `MenuList::Update` |  | A menu list's update. |
| `sub_8209A1C0` | `TickHandler::Invoke` | Wrapped | r3 the app, r4 the handler (target at +8). |
| `sub_820DC5C8` | `BlinkTimers::Advance` | Wrapped | Steps the blink timers; once a tick: skipped on render-only passes (`src/subtick/per_frame_logic.cpp`). |
| `sub_8215BE00` | `RaceOverlays::Run` | Overridden | The race overlays (25 objects at r3, e.g. the marker above opponents, the light trails): update (vtable slot 4) and record (slot 5). With `--subtick`, the update here on logic passes, the recording moved to just before `Race::DrawStep` on every pass (`src/subtick/per_frame_logic.cpp`, "Race overlays"); otherwise the original. |
| `sub_8215BE80` | `RaceOverlays::Record` | Overridden | Records the 25 overlays (slot 5) without updating them; from the paused handler `sub_8215C0A8`. With `--subtick` in a race, on logic passes it only saves the list, and the recording happens before `Race::DrawStep` on every pass, as for `RaceOverlays::Run`. |
| `sub_821281B0` | | Wrapped | World-space sprite list recorder (r3 = list, r4 = camera pass, its +144 used): the light trails. Corners blended between ticks. ITC's `sub_82128750`. |
| `sub_82127C08` | | Wrapped | Single world-space sprite recorder (r3 = sprite, r4 = camera pass), reached through `sub_82128CF8`. Corners blended between ticks, except for the marker (blended by its centre in `sub_8212BDA0`). ITC's `sub_821281A8`. |
| `sub_8212BDA0` | | Wrapped | The opponent marker's overlay record (vtable slot 5; r3 = overlay, r4 = camera pass index; ITC's `sub_8212CE40`): builds the sprite on the stack from its centre (overlay +176..+184, set once a tick) and axes facing the camera pass's eye. With `--subtick` the centre is blended between ticks for the call. |
| `sub_82102B30`, `sub_82102BB0` | | | Replay: store the player's control record (r3 = record, r4 = pad), and replace it from the replay while one plays; from `sub_8215E390` right after `PlayerControls::Build`. |
| `sub_821573E0` |  | Wrapped | Probably the rival intro cutscene's Update handler: 107 of its 126 expected instructions line up, but it's 210 long and makes calls the expected one doesn't. Not checked. |
| `sub_8220F7B0` | `RivalLogo::Draw` | Overridden | Draws the rival's logo: with its entry's flag 8, its UI movie looked up and run, and flag 16 set when it's over. Kept for render-only passes by `src/subtick/per_frame_logic.cpp`, which calls our C++ for it with `--native_race_hud` (`src/hooks/race_hud.cpp`). |
| `sub_8246BD88` | `MoviePlayer::OpenFile` |  | The movie player's file open (through a vtable); calls `CreateFile`. |
| `sub_820AE840` |  | Overridden | The render thread's command that starts the boot movie. |
| `sub_82106540` | `Bgm::Load` | Wrapped | r4 a bank name: looks it up in the table at `0x820069B0` (`bgm_tmp_01`-`17`, `BGM_SYSTEM`) and loads its sound bank (BUILD 22970 + i) and wave bank (22988 + i), or `SB_SYSTEM_SE` (22885, 22886) for any other name, through the sound object at `[0x826B5960]` (vtable slot 4). See `docs/audio.md`. Who calls it, by return address: "Music" in `docs/audio.md`. Custom music rolls first, by the caller, and for a blocked song hands it another name from the same list (`src/hooks/custom_music.cpp`). |
| `sub_82173C70` | `Music::PlayRoaming` |  | r3 the scene: by its music mode (+9652), modes 2, 4, 5, 6 play one of entries 1-5 of `0x82609334` with `sub_8258F558() % 5` (the game's random roll), other modes entry 0 (`bgm_tmp_06`). From `sub_821744D8` and `sub_82174668`. |
| `sub_821741B8` | `Music::PlayRequested` |  | r3 the scene: plays the music each bit at +9644 requests and clears it (0x40, 0x80, 0x100, 0x200-0x2000: "Music" in `docs/audio.md`). |
| `sub_821743B0` | `Music::RequestBattle` |  | r3 the scene: sets the music bit at +9644 for the opponent, by its record's (`sub_821172E0`) class byte (+35) and its id ("Music" in `docs/audio.md`). |
| `sub_82174AD0` |  |  | Message handler of the scene: plays entry 12 of `0x82609334` (`bgm_tmp_08`, race-end music) under conditions. |
| `sub_822624F0` |  |  | Plays `bgm_tmp_14` or `bgm_tmp_08` by comparing +9672 and +9668 of r31 (race end, by the custom music categories; not checked). |
| `sub_821B8580` |  |  | Plays one of the 17 `bgm_tmp_` tracks by a setting (`[[r31+32544]+740]`, 0 = `bgm_tmp_01`): the music select. |
| `sub_82175238` |  |  | Registers the scene's message handlers (the music ones above, `sub_821744D8`, ...) and sets its music mode (+9652) from a table indexed by the game state. |
| `sub_82118818` |  |  | Builds `data\audio\%s.xgs` and reads the XACT global settings (`TXR_XACT.XGS`). |
| `sub_8244D0A0` | `AudioRenderDriver::Register` | Overridden | r3 the driver object (callback argument +12, handle +16), r4 the mixer callback: `XAudioRegisterRenderDriverClient`. Reached only through a vtable. |
| `sub_8244D128` | `AudioRenderDriver::Submit` | Overridden | r4 the mixed buffer (descriptor from `sub_82448738`): `XAudioSubmitRenderDriverFrame`. |
| `sub_8244D188` | `AudioRenderDriver::Destroy` | Wrapped | Unregisters the client (`XAudioUnregisterRenderDriverClient`). |
| `sub_8257C880` | `XPR::Create` |  | r3 the XPR object, r4 the loaded file: checks `XPR2`, copies the header to normal memory and the data to physical memory, relocates each `TX` resource (`docs/game_data.md`, "XPR2 loading"). Object: +0 header, +4 its size, +8 data, +12 its size, +16 records, +20 record count. |
| `sub_8257CA30` | `XPR::GetTexture` |  | r3 the XPR object, r4 an index: that resource's header; an index past the end gives resource 0. |
| `sub_8257C7A0` | `XPR::FindTexture` |  | r3 the XPR object, r4 a name: the resource with that name, or 0. |
| `sub_8257C9C0` | `XPR::Release` |  | Frees the header and data. |
| `sub_82354AC8` | `TextureHeader::Relocate` |  | r3 a texture header, r4 the data's address, r5 the mips' address: adds them to the base (+0x20) and mip (+0x30) address fields. |
| `sub_82573B80` | `Memory::Alloc` |  | r3 a size: page-aligned memory, tagged `XLMM`. |
| `sub_82573CC8` | `Memory::AllocPhysical` |  | r3 a size, r4 flags: physical (GPU-visible) memory, tagged `XLMM`. |
| `sub_8257A150` | `XPRList::XPRList` |  | A model's list of XPR objects (`XLRes_Tex.cpp`; vtable `0x82069A94`). r4 the reserved size (-1 = 64). |
| `sub_8257A018` | `XPRList::Set` |  | r4 an index, r5 an XPR object. Growing past the reserved size asserts (`XPR LIST RESERVE !!!`). Vtable +16. |
| `sub_82579FB8` | `XPRList::Get` |  | r4 an index: that XPR object, or 0. Vtable +12. |
| `sub_825797A8` | `XPRList::GetTexture` |  | r4 the XPR's index, r5 the texture's: `XPR::GetTexture` on that XPR. Vtable +20 (and +24, +28). |
| `sub_82579808` | `XPRList::FindTexture` |  | r4 the XPR's index, r5 a name: `XPR::FindTexture`. Vtable +32. |
| `sub_8257C238` | `Model::Load` |  | r3 the model, r4 the mesh file, r5 an XPR2 file or 0: `Mesh::Parse`, then `XPR::Create` into model+56, which goes into the model's XPR list (model+80, pointer at +48) at index 0. |
| `sub_8257BD58` | `Mesh::Parse` |  | Checks the `00 15 00 00` version (`Version Error %x`), loads the shaders, copies the file up to its vertex data into normal memory. |
| `sub_8257A620` | `Mesh::LoadShaders` | Overridden | Loads the mesh file's vertex and pixel shaders (`XPhysicalAlloc Error Vertex Shader` / `Pixel  Shader`). |
| `sub_8257AE50` | `Model::RecordShape` |  | Records one shape's draws: shaders, constants, and for each material its textures through `XPRList::GetTexture` (material +0x98 the XPR, +0xB8 the texture). |
| `sub_8257BA38` | `Model::RecordShapeUntextured` |  | Like `Model::RecordShape` without textures (probably a depth or shadow pass). Called from `Model::Draw`. |
| `sub_820CEA60` | `Model::RecordMaterials` |  | Called from `Model::Draw`. Switches on each material's shared shader pair (+392 from the record's +8, the record's +0x190: 1000-1004; `docs/game_data.md`, "Part files") and binds the car's vinyl sheet (model+444) for 1000 and 1004. |
| `sub_820CE7B8` | `Model::SetVinylTextures` |  | r4, r5: stored at model+444 and +448. |
| `sub_820D5B70` | `CarModel::LoadExterior` |  | Loads set a (mode 0) or set b (mode 1), the mode being the word at +8 of `[car model+7256]`, then `CarModel::LoadDecals`. Also creates the vinyl sheet (`Texture::Create`, 1024 x 1024 A8R8G8B8). |
| `sub_820D5680` | `CarModel::LoadSetA` |  | Node file 892 + 226 x car and body XPR2 890 + 226 x car (car = car model+32), each in its own allocation. |
| `sub_820D5768` | `CarModel::LoadSetB` |  | Node file 11062 + 197 x car and body XPR2 11060 + 197 x car, the XPR2 into the shared load buffer (no size check). |
| `sub_820D57F8` | `CarModel::LoadDecals` |  | Loads the 6 vinyl layers and 48 stickers (TGA) into slots at car model+4092 and +4380 (`docs/game_data.md`, "Car decals"). |
| `sub_820D53F8` | `CarModel::OnDecalLoaded` |  | Load callback: marks the slot done and calls `Vinyl::DrawDecals`; when all are done, `Vinyl::ClearCorner` and `Vinyl::Compress`. |
| `sub_820D5C30` | `CarModel::SetVinylTextures` |  | Gives the vinyl sheet (car model+6684, +6692) to the six models at car model+688 (464 bytes apart). |
| `sub_820D0620` | `LoadSlot::LoadShared` |  | r3 a 24-byte slot (+0 id, +4 request, +8 size, +16 buffer, +20 state: 1 empty, 2 loading, 3 done), r4 an id: loads it into the shared load buffer, or marks the slot empty if it doesn't fit. |
| `sub_820D0588` | `LoadSlot::LoadAllocated` |  | The same into a buffer allocated for the file. |
| `sub_820D7660` | `CarInterior::Load` |  | Loads the interior parts from set c when +3088 is set, else set a or b by mode (`CarInterior::LoadParts` with the body XPR2's id, the stride and the interior's offset: 890/226/219, 11060/197/190, 19925/55/49). |
| `sub_820D73E0` | `CarInterior::LoadParts` |  | Loads six consecutive part ids: meters, roll cage, interior, gauges, seats, steering wheel. |
| `sub_820E6CA8` | `Car::SetupSettings` |  | r3 a car object, r4 its settings (156 bytes: copied to r3+32, filled in, copied back; `Wheels::Setup` later copies them to car model+28); when the front and rear wheel numbers (settings +16, +32) are both -1 and the model block is 18 or more, calls `Wheels::BlockDefault`. |
| `sub_820D3620` | `Wheels::BlockDefault` | Wrapped | r3 a model block (18-44), r4 and r5 the front and rear wheel numbers: sets them to a fixed wheel per block (AD00 for the four cut cars, TA03 for BOSS00, wheel 18 for CON_X). The ECLI keeps -1 (its own stock wheel), and the L86, S30 and T86 get the BOSS00's wheel, or -1 with `--cut_car_steelies` (`src/hooks/cut_car_wheels.cpp`, `--cut_car_wheels`). |
| `sub_820D8DD8` | `Wheels::Setup` |  | r3 the car model: clears +28 (156 bytes), then picks the wheel models with `sub_820D8860` if the word at +8 of `[r3+3268]` is 0, else `sub_820D8900` (`docs/game_data.md`, "Wheels"). |
| `sub_820D8860` | `Wheels::PickRims` | Wrapped | Front and rear wheel XPR2 (+2144, +2168) and rim part (+1168, +1192) from the wheel numbers at +44 and +60 (-1 stock: wheel = model block if <= 19, else AD00). With `--cut_car_steelies`, the L86, S30 and T86's stock wheel becomes the LORRY's after it (`src/hooks/cut_car_wheels.cpp`). |
| `sub_820D8900` | `Wheels::PickBrakes` | Wrapped | The same with the brake part (588 + 3i) at +1168 and +1192; traffic (+28 = 3) uses wheel 20 + `[[r3+3264]]`. Wrapped like `sub_820D8860`. |
| `sub_820D6498` | `CarInterior::LoadSetCBody` |  | Set c: node file 19927 + 55 x car, body XPR2 19925 + 55 x car (shared load buffer), and six exterior parts picked by the tuning choices. |
| `sub_820D3D18` | `Vinyl::DrawDecals` |  | Decodes each loaded vinyl and sticker (`TGA::CreateImage`) and draws it into the vinyl sheet (`Vinyl::DrawDecal`); a 1024 x 1024 vinyl is drawn as the whole sheet. |
| `sub_820EE4A8` | `Vinyl::DrawDecal` |  | r3 the region (table at `0x825F2A00`), f1 the rotation, r5-r9 the image and the transform: a quad the TGA's pixel size, drawn on the CPU. |
| `sub_820D3920` | `Vinyl::ClearCorner` |  | Clears the 16 x 16 block at the sheet's bottom-left (sizes from `0x825F2520`). |
| `sub_820ECCC8` | `Vinyl::Compress` |  | Compresses the A8R8G8B8 sheet into a new DXT5 texture (`0x1A200054`) and frees the canvas. |
| `sub_820ECB40` | `Texture::Create` |  | r3 a D3D format, r4/r5 the size: a texture and its pixels. |
| `sub_820ECC58` | `TGA::CreateImage` |  | r3 a 20-byte image descriptor (out), r4 the file size, r5 a pointer to the file, r6 the pixels (out): `TGA::Parse`, then `TGA::ToARGB`. |
| `sub_820EE8E8` | `TGA::Parse` |  | `TGA::ParseHeader`, then `TGA::Decode`. |
| `sub_820EEA68` | `TGA::ParseHeader` |  | Copies the 18-byte header, swapping the 16-bit fields from little-endian (width at +14, height +16, bits per pixel +18). |
| `sub_820EEFD8` | `TGA::Decode` |  | 24 or 32 bits per pixel only; type 2 copied from byte 18, type 10 through `TGA::DecodeRle`, others fail. |
| `sub_820EEF28` | `TGA::DecodeRle` |  | Run-length decoding (type 10). |
| `sub_820EED78` | `TGA::ToARGB` |  | Top-down 32-bit ARGB; 24-bit pixels get alpha 255. |
| `sub_820EDF38` | `SoftRaster::DrawTriangle` |  | CPU triangle setup for the vinyl sheet. |
| `sub_820EDC70` | `SoftRaster::DrawSpan` |  | One span of a triangle. |
| `sub_820ED0D0` | `SoftRaster::BlendPixel` |  | Samples the decal and blends it into the sheet: (src x a + dst x (256 - a)) >> 8. |
| `sub_8218C240` | `PostRace::Check` | Overridden | Post-race check: once loaded (+9608), waits for a counter (global at `0x826C7EC4`) to reach 60 before setting the next state at +9604. The 60 is 1 here (`src/hooks/post_race.cpp`, `--post_race_check_fix`): with 60, the screen stays black after quest races on fast hardware. It's also the garage top's load step, run on every entry to the garage (`docs/menus.md`); r3 is the garage top's handler object, which autosave notes. |

## Font

The text font (BUILD 22656, `docs/game_data.md`, "Font"). The font object
holds its descriptions (one per font, picked by the style's +60, and +108
for the second pass a style can ask for at +112), its style at +16, the
current description at +36, and the shaders: vertex declaration +8368,
vertex shader +8372, a pixel shader per channel at +8376.

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_821089D8` | `Font::CreateShaders` |  | Compiles the font's HLSL (vertex shader source at `[0x825F32C0]`, a pixel shader per channel from the table at `0x826B58E4`) through `sub_822B3678`. |
| `sub_82109280` | `Font::LoadSheet` | Wrapped | r4 the container, r6 the description it fills: +0 the glyph table's header, +4 its entries, +8 the XPR, +12 the texture, +16/+20 the texture's width and height (`sub_822A3160`, level 0), +24 1. Reached only through a pointer. The wrapper puts 576 x 928 there for a sheet that is an exact multiple of that size, so a larger sheet maps like the original (`src/hooks/font.cpp`). |
| `sub_822A3160` | `Texture::GetLevelDesc` |  | r3 the texture, r4 the level, r5 the description: width at +24, height at +28. |
| `sub_8210A530` | `Font::DrawText` |  | r5 the text (UTF-16): sets the description from the style's +60 and draws (`sub_8210A020`); again with +108 when the style's +112 is set. |
| `sub_8210A020` | `Font::BuildText` |  | Counts the glyphs per channel (`sub_82108330`), lays them out (`sub_82109EB0` or `sub_82109BD0`) and records a draw per channel with that channel's pixel shader. |
| `sub_82108330` | `Font::CountGlyphs` |  | Counts the text's glyphs in each channel (low 2 bits of the entry's second word), skipping spaces, line breaks and color codes. |
| `sub_82109BD0` | `Font::LayOutText` |  | Lays out a line at a time: each glyph's pen moves by (advance >> 2) times the style's +36, then +8256 and the style's +28. Handles line breaks and the color codes (`FFD6`, `FFF6`-`FFFF`). |
| `sub_82109EB0` | `Font::LayOutPositioned` |  | Like `Font::LayOutText` with each glyph's position taken from the font's table at +64. |
| `sub_82109680` | `Font::AddGlyph` |  | r6 the character, r7 the color, f1/f2 the pen: six vertices (x, y, z, color, u, v; 24 bytes) into its channel's buffer, the texture coordinates from the index, the cell size and the description's +16/+20 (`docs/game_data.md`, "Font"). Clips through `sub_82108DB0` or `sub_82109088`. |
| `sub_82108DB0` | `Font::ClipGlyph` |  | Clips a glyph's quad to the style's rectangle (+44 to +56), moving the texture coordinates by the same amount in texels of the description's +16/+20. 0 when nothing is left. |

## Fixed time step

The game never measures elapsed time. Each pass of `App::Run`
(`sub_8209A8F0`) is assumed to be 1/30 s, so running the loop at 60 fps
doubles game speed. Functions that load the step as a float constant from
`.rdata` (`1/30` at `0x820029C4`, `1/60` at `0x820029C0`):

- 1/30: `sub_820B1678`, `sub_820B8BC0`, `sub_820BFF88`, `sub_820C2208`,
  `sub_8210DBE8`, `sub_8210F390`, `sub_8211F6F0`, `sub_82129DE0`
- 1/60: `sub_820B0DD0`, `sub_8226F9D8`, `sub_82153E40`, `sub_821E6F28`

Not all of these are time steps. `sub_8226F9D8` and `sub_821E6F28` are HSV to
RGB conversions: 1/60 turns a hue in degrees into its sector, so halving
`0x820029C0` swapped the car paint colors (green and blue). `sub_820B0DD0`
is the car's creation (its first step length, car+0x930). `sub_820B1678`
uses 1/30 as "per counted step".

Every load of those two floats in `.text` was checked (2026-10-17, a
`lis`/`addi`/`lfs` scan of `default.xex.decrypt`): only these functions read
them, each with one `lfs` (`sub_820B1678` at `0x820B173C`, `sub_820BFF88` at
`0x820BFF90`, `sub_8226F9D8` at `0x8226FA30`, `sub_821E6F28` at
`0x821E6F80`). The neighboring floats (`0x820029B0` to `0x820029CC`) are
separate constants.

Also loaded, and not yet checked:

- 30.0 at `0x82000D54` (22 functions): `sub_820A9DE0`, `sub_820AA040`,
  `sub_820C5BA0`, `sub_820DC8E8`, `sub_820DCD88`, `sub_820DD298`,
  `sub_82110C18`, `sub_821121F0`, `sub_82121920`, `sub_82164500`,
  `sub_8222BE40`, `sub_82450A78`, `sub_82467398`, `sub_824673F8`,
  `sub_8246ACB0`, `sub_82482410`, `sub_82568948`, `sub_8256D8F8`, and leaf
  functions (no `.pdata` entry) with the load at `0x820B16E0`, `0x820DC154`,
  `0x82143D18` and `0x8256B76C`. 30.0 also at `0x82002B80` (`sub_820C0B48`),
  `0x82002BAC` (`sub_8210EAB0`), `0x82007F50` (`sub_8210F390`), `0x82008744`
  (`sub_8212F278`, `sub_8212F5D8`), `0x8200A108` (`sub_8218FF88`) and
  `0x82010028` (`sub_82121920`).
- 60.0 at `0x82000F0C` (9 functions): a leaf ending at `0x82095790`
  (returns a / b * 60), `sub_820A7C08` (compares against 60),
  `sub_82110C18` and `sub_821110B0` (copied into tables on the stack), a
  leaf with the load at `0x821441A0` (compares against 60), `sub_821447D8`
  and `sub_82144958` (a value times a constant, clamped to 60),
  `sub_8217B770`, `sub_8217DFF0` (passed as f1). 60.0 also at
  `0x82002950` (`sub_820B0DD0`, stored into the object at +0x934 and
  +0x938). Some of these are likely not times (60
  degrees, a speed limit), so these constants are shared and can't be
  patched as a whole.
- 1/30 and 1/60 as `.data` variables (writable, so possibly tuning values):
  1/60 at `0x825F17E8` (`sub_820BFB68`, `sub_820C2E40` and a leaf with the
  load at `0x820BFF54`),
  `0x82609328` (`sub_8216E590`, `sub_821703B8`) and `0x82609370`
  (`sub_82151B18`); 1/30 at `0x8260707C` (`sub_82126C10`, `sub_82126D10`,
  `sub_82126EA8`, `sub_82127428`, `sub_821275E0`), `0x825F3204`
  (`sub_820FF1A0`), `0x82607690` (`sub_82144558`), `0x82609304`
  (`sub_82160FE8`) and `0x82609374` (`sub_82151B18`); many more are only
  written by static initializers (`0x825BE8CC` to `0x825BF1F4`).

Integer frame counts (for example `cmpwi r11,60`) aren't included.

## Tick rate experiments (removed 2026-10-25)

A true tick rate upgrade: all game logic at 60 ticks a second. Parked: the
game keeps its 30 Hz tick (see "Subtick"), as the original replays depend
on it. The two switches below (`src/tick_rate/tick_rate.cpp`, both off by default) were removed on 2026-10-25 with the code (`cull_emulated_build.ps1` deletes `src/tick_rate/`); the findings are kept:

- `--tick_rate_60`: the present interval is 1 vblank instead of 2
  (`sub_82573100`), so the main loop and all game logic run 60 times a
  second. Play time counts every other tick (`sub_82107A38`).
- `--tick_rate_half_step`: halves the step lengths once the XEX is loaded:
  1/30 -> 1/60 at `0x820029C4` (`.rdata`, mapped read-only, so the page is
  made writable for the write and put back) and the cars' step length at
  `0x825F17E8` 1/60 -> 1/120. `--tick_rate_half_step_mask` picks which (bit
  0 = 1/30, bit 2 = the cars' step; default 5, both). The 1/60 at
  `0x820029C0` was halved too until 2026-10-18; that swapped the car paint
  colors (it's the HSV conversion's 1/60), so it's left alone.

With `--tick_rate_60` alone (2026-10-17, before the cars' step length was
added) the game ran at twice the speed. So did halving only the two `.rdata`
constants: the cars are stepped by `0x825F17E8`, not by them (see "Vehicle
stepping"), and some of their users aren't step lengths (`sub_820B1678`).

On 2026-10-18: halving only the cars' step length (mask 4) at 30 ticks
halved the car's speed and nothing else. With `--tick_rate_60` too, the
car's speed was right (timed with a stopwatch), but grip and the rest of the
handling felt doubled: what runs once a tick (`sub_820C2208`, the rest of
game logic) ran twice as often.

What a real upgrade would also need: everything that counts ticks
(`sub_820B1678`, the action mappers' repeats, UI movies, the title screen's
600, the 30.0 and 60.0 users above, integer frame counts) scaled.

## Vehicle stepping

The game already steps the cars twice a tick. `Vehicle::Tick`
(`sub_820C2E40`), once a tick per car, runs two steps of 1/60 s
(`[0x825F17E8]`, a `.data` float only ever read): each applies the car's
control record (`Vehicle::ApplyControls`, `sub_820BFB68`), clears the
record's one-shot bits (`0x00010003`: the first step sees them, the second
doesn't), steps the physics (`Vehicle::Step`, `sub_820B6330`) and calls
`sub_820C1B98`. Then it calls `sub_820C2208` (the odometer, which reads
1/30) once.

`Vehicle::Step` stores the step length at car+0x930 and its inverse at
car+0x934 (`sub_820AF9D0`), then runs the car's physics parts and an inner
loop of car+0x940 iterations (10, set when the car is created in
`sub_820B0DD0`). Checked 2026-10-23 in the generated code: nine parts under
it read +0x930, none reads +0x934 or +0x938 (60.0 from creation), so the
physics scales with the step length. The one exception is `sub_820B1678`,
which counts steps (a ramp to 30). `Vehicle::ApplyControls` scales its rates
by `[0x825F17E8]`, not by the step it's given.

The control records: a table of pointers at `0x826B5810`, one per car index.
The race modes (`sub_82163CC8`, `sub_82164188`, `sub_82171950`) build a
car's record on the stack and copy its 5 words into the table's record just
before calling `Vehicle::Tick`. The player's record comes from
`PlayerControls::Build` (`sub_8215DBF8`), which reads the action mappers:
pads (`Input::Update`) -> action mappers (`ActionMappers::Update`, from
`sub_82107A38`) -> `Input::QueryAbstract` / `Input::Query` -> the record.

## Subtick

Frames between the game's 30 Hz ticks, with the cars' input sampled
between ticks and the cars' drawn matrices blended between ticks. Game
logic and car physics keep their 30 Hz tick, so the original replays stay
compatible. `src/subtick/`, all behind `--subtick` (on by default). The only
other switches are `--max_fps`, `--subtick_input` (on with it),
and `--subtick_input_sensitivity`; everything else here is always on with `--subtick`:

- **Passes** (`renderer_hooks.cpp`, `frame_pacer.cpp`,
  `game_messages.cpp`): after `Renderer::WaitForFrame`, the pacer picks the
  coming main loop pass. When the next 30 Hz tick is due it's a logic pass;
  otherwise, in races, it waits for the next frame slot (`--max_fps`, 60 by
  default, 0 = unlimited) and the pass is render-only: `GameMessages::Dispatch`
  consumes Update messages without delivering them (main thread only), so
  the pass draws the same state again. Menus stay at 30 fps (they record
  their UI and read input during Update). A pass is in a race when the last
  logic pass ran `Race::DrawStep`. The present's 2-vblank wait is dropped.
- **Car input** (`input.cpp`, `input_weights.cpp`, `--subtick_input`):
  the player's record gets one input for the tick: the samples taken over
  the last tick's interval, weighted as a whole (all five axes together) by
  `Input_Aggregate`. The weights are (1 - s) x [newest] + s x ([newest] +
  biggest change) / 2, normalized, where the change is a sample's move from
  the one before it over all axes, relative to the largest in the interval.
  The first sample (last tick's) gets no weight. `--subtick_input_sensitivity`
  (s, 0.0 to 1.0): 0 is the tick's own input only, as the game reads it; 1
  is half newest, half the biggest changes (flicks and brake blips count).
  The presence bits come from the newest sample. Physics is unchanged.
  Substepping the physics (N steps a tick) was tried and broke the car's feel
  from 4 steps up (the physics assumes its step count), so it was removed.
- **Input** (`input.cpp`, `--subtick_input`, on with `--subtick`):
  `Input::Update` is skipped and `ActionMappers::Update` runs once a tick,
  so presses, menu repeats and the input
  listeners count ticks. At the start of each render-only pass in a race,
  the pads are read again (`PhysicalPad::Read` with the flags and rumble
  timers put back, `LogicalPad::Build`) and the mappers updated with their
  digital state put back (+448..+471, +1852..+2451 and the 16 bytes of each
  repeat object), so only the analog values move. The record's five axes
  are then worked out as `PlayerControls::Build` does (the same integer
  curves and dead zones) and kept as a sample. The tick's own record is the
  last sample; the last one carries over as the next tick's first.
  The weighted input (see "Car input") is written into the record by the
  `PlayerControls::Build` override, right as it's built, before the race
  mode copies it anywhere. The race mode's caller (`sub_8215E390`) then
  hands the record to the replay recorder (`sub_82102B30`) or, playing a
  replay, replaces it from the replay (`sub_82102BB0`). So a replay stores
  exactly the input the physics used and plays back 1:1. (Before, the
  weighted input was swapped in inside `Vehicle::Tick` only, so replays
  stored the tick's own input and played back "sloppy".) `Vehicle::Tick` is
  the game's own. AI cars and traffic get no samples and run as the game
  does.
- **Cars** (`car_transforms.cpp`): the world matrix at car+16 is set once
  a tick. `Car::DrawUpdate` (`sub_820DA930`, once per view) records it as
  the tick's pose on logic passes, once per car per tick (the pacer's tick
  count). Before `Race::DrawStep` (`sub_82159540`) on every pass, every car
  recorded this tick is blended between its last two poses by the frame's
  progress, and put back after the step if it still holds the blend. On a
  logic pass, cars whose matrix already holds the new pose are recorded
  first; the game sets the rest during the draw, so `Car::DrawUpdate` also
  writes the blend for its own draw when the matrix isn't blended yet.
  Blended: position and rotation rows (each row keeps its length). Only
  consecutive ticks blend; over 8 m in a tick is a cut (a traffic car
  recycled elsewhere). Wheel spin and steering still step per tick.
- **Main camera** (`car_transforms.cpp`): camera pass 0 (array at
  `0x826AA8E0`, 592 bytes a pass, as in ITC) holds the eye (+400), target
  (+416), up (+432) and roll (+448). `CameraPass::Update` (`sub_820A3B98`)
  builds the view (+16) and view-projection (+144 = view x projection +80)
  from them, several times a Draw. On pass 0 in a race each call is matched
  by its order within the pass; the logic pass records what each is given,
  and on every pass a call given exactly the tick's values builds from the
  blend. Calls given other values are left alone. Over 8 m between ticks is
  a cut. The blend stays in the pass for the rest of the frame and the
  tick's values go back when the next pass starts
  (`CarTransforms_EndFrame`, before any game logic). The draw reads the eye
  and target besides the view: the camera position the cars' paint
  reflections are worked out against (`0x8264EA40`, copied from the eye per
  view by `sub_820C5BA0` and by `sub_820F3398`, and passed in each part's
  upload, c8, by `sub_8257ACD0`), and the direction the light glows'
  billboards face (`sub_8209C800`). Put back right after each call, those
  got the tick's eye against a blended view: the reflections and the head
  and tail light glows shook while the geometry didn't.
- **`--camera_fov`** (`car_transforms.cpp`, `CarTransforms_CameraFov`, called in the `sub_820A3B98` wrapper after the game's update, with or without `--subtick`): scales pass 0's projection, P00 (+80) and P11 (+100, the row-major diagonal), by one factor, so P11 = cot(fov / 2) and the aspect is kept. Then view-projection (+144) is rebuilt as view (+16) times projection, as `sub_8209BE70` does. Values 10 to 170 degrees; 0 is the game's own. The game's values are kept and put back when the flag is 0, and taken again when the game writes the projection. Before the update, the projection was not used: the update calls a virtual method on the pass (slot 0 of its vtable, `f1` = the constant at `0x82000A24`, `r4`/`r5` = +504/+508) before it builds the view and the view-projection, so a change made earlier can be overwritten. Not yet checked in the game; the recorder sometimes draws from a stack copy of the camera (see "Recorder"), so the race world may not follow pass 0.
- **Cars and the view-projection** (`CarTransforms_RebuildCamera`): the
  cars' parts are drawn with pass 0's +144 as it is when `Car::DrawUpdate`
  runs (the part draw multiplies it with car+16 into the frame's draw
  packets). `Race::DrawStep` draws the cars (`sub_820C5BA0`) before its own
  camera updates (`sub_820F5F48`). On a logic pass the Update message has
  already run `CameraPass::Update` on pass 0 for the frame (5 calls against
  4 on render-only passes), but render-only passes skip Update, so the cars
  were drawn with the previous frame's camera while the world used the new
  one (every car jumped ahead and back, ABAB, with the world smooth). On
  render-only passes in a race, `CameraPass::Update` now runs on pass 0
  just before `Race::DrawStep`.
- **One progress per frame** (`FramePacer::FrameProgress`): the blend's
  progress through the tick is taken once, when the pass starts, and used
  for everything blended in that frame. Read at each use, a logic pass built
  its camera early in Update and drew the cars ~10 ms later at a larger
  progress, so the cars were a few cm out of step with the camera on logic
  frames (plain in the cockpit view).
- **Per-frame logic** (`per_frame_logic.cpp`): what runs from Draw and
  counts passes is kept to once a tick on render-only passes: blink timers,
  texture animations, the minimap blink clock, play time, UI movies and the
  boost value (replayed from the logic pass's state).
- **Race overlays** (`per_frame_logic.cpp`): the marker above opponents,
  the tire smoke and the light trails behind head and tail lights,
  world-space sprites recorded with a camera pass's view-projection (+144)
  as it is when they record. The game records them during Update
  (`RaceOverlays::Run`, `sub_8215BE00`, from handler `sub_8215C0A0`), which
  render-only passes skip; they used to be recorded on render-only passes
  with the logic pass's cameras, so they were drawn with an older camera
  than the world and shook against it. Now the update (vtable slot 4) runs
  in Update on logic passes and the recording (slot 5) on every pass just
  before `Race::DrawStep`, once pass 0 holds the frame's camera
  (`PerFrameLogic_RecordRaceOverlays`; the other passes as the update saw
  them). A pass records if the overlays were updated on the latest tick or
  the one before (a logic pass whose update came after its draw step still
  draws). While paused the overlays aren't updated: the paused Update
  handler `sub_8215C0A8` only records them (`RaceOverlays::Record`,
  `sub_8215BE80`), so they were drawn on logic frames and not between them
  (the marker and the tire smoke flickering every other frame). That
  recording is moved before `Race::DrawStep` on every pass too.
- **World-space sprite corners** (`per_frame_logic.cpp`, ported from
  ITC-Recomp): a sprite's four corners
  (16 floats at +80) are written once a tick, so with the camera blended
  they ran ahead of their car early in each tick. The two recorders,
  `sub_821281B0` (sprite lists, the light trails; count at +0x109F0, sprite
  i at +80 + 64i) and `sub_82127C08` (single sprites, the marker), blend
  the corners between their last two ticks by the frame's progress while
  they record, then put them back. Lists are matched by list and order in
  the pass, single sprites by sprite, caller and order. A corner that moved
  over 10 units in a tick is a new sprite. The list
  follows ITC-Recomp's findings for the same game; more may turn up.
- **Opponent marker** (`per_frame_logic.cpp`, `sub_8212BDA0`): its record
  builds the sprite each call from a centre set once a tick (overlay
  +176..+184) and axes facing the camera's eye. With the camera blended the
  axes, and so the corners, change every frame, so the corner blending
  above never matched it and the centre stepped once a tick against the
  blended car. The centre is blended between its last two ticks instead.

Not done yet: wheel spin and steering change once a tick; the lights on
car models (headlight and tail light sources, not the glow billboards) are
unchecked and may need the same camera-position fix as the paint
reflections; UI movie tracks that add to values
(ITC-Recomp's pause sub-menu arrow fix) aren't ported.

## Native game code

The functions `docs/native-code.md` rewrites as our own C++, by flag. With
a flag off, each override calls the recompiled function. The car specs'
other functions (`Params::GetCarSpec`, `Car::Refresh` and the rest) are in
the first table, by the cars' other functions.

### `native_framework`

The getters are shared by the linker across classes (one function for
every class with a word at that offset), so they're named by offset.

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_82090840` |  | Overridden | Getter: r3's byte at +133 (the framework's flag `App::Run` reads). |
| `sub_82090848` |  | Overridden | Getter: r3's byte at +134. |
| `sub_82467570` |  | Overridden | Getter: r3's word at +60. |
| `sub_82090850` |  | Overridden | Getter: r3's word at +64. |
| `sub_82090858` |  | Overridden | Getter: r3's word at +80. |
| `sub_82472750` |  | Overridden | Getter: r3's word at +68. |
| `sub_8246CD28` |  | Overridden | Getter: r3's word at +44. |
| `sub_82090908` |  | Overridden | Thunk, the +36 interface's slot 1: the input manager (r3 - 12, its word at +64). |
| `sub_82090900` |  | Overridden | Thunk, the +52 interface's slot 2: the renderer (r3 - 44, +68). |
| `sub_82090910` |  | Overridden | Thunk, the +52 interface's slot 1: the worker pool (r3 - 28, +44). |
| `sub_820908B8` |  | Overridden | Thunk, the +24 interface's slot 4 (r3 - 16, +68). |
| `sub_82090938` |  | Overridden | Thunk, the +52 interface's slot 3: the scene (r3 - 32, +60). |
| `sub_820909B0` |  | Overridden | Thunk, the +36 interface's slot 2: the renderer (r3 - 28, +68). |
| `sub_820909B8` |  | Overridden | Thunk, the +36 interface's slot 4: the worker pool (r3 - 12, +44). |
| `sub_820909D8` |  | Overridden | Thunk, the +36 interface's slot 3: the scene (r3 - 16, +60). |
| `sub_82091120` | `DebugText::SetPosition` | Overridden | Vtable slot 2: r4, r5 the x and y words' addresses; sets the debug font's cursor and line start (`0x8296E140`), and leaves r3, r4 = x, y and r11 as the original does. |
| `sub_82571988` | `DebugFont::SetPosition` | Overridden | r3, r4 = x, y: the debug font's cursor and line start. |

### `native_menu_sounds`

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_82110318` | `MenuSound::Play` | Overridden | r3 the cue: plays it from bank 0 at volume 1.0 (`Sound::Play`'s three arguments by address, in a 112-byte frame). |
| `sub_82173710` | `Sound::Play` | Overridden | r3 the bank's address, r4 the cue's: the sound manager's 2D play slot at volume 1.0; the slot call returns to `0x8217374C`. |
| `sub_821B80C8` | `SoundOptions::SetEffectsVolume` | Overridden | r3 the setting (0-15): sets the effects category's volume and its two globals. |
| `sub_821B8198` | `SoundOptions::SetMusicVolume` | Overridden | r3 the setting (0-15): sets the music category's volume and its global. |

### `native_car_specs`

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_820C9B78` | `Params::Exists` | Overridden | 1 when the car parameters are loaded. |
| `sub_820C9C80` | `Params::FirstWheelOfModel` | Overridden | r3 a wheel model: its first record + 1, or 0. |
| `sub_820C9B90` |  | Overridden | Load callback for BUILD 74 to 82 (the aero parts): nothing to swap (their getters swap a copy); counts the load down. |
| `sub_820CBB30` |  | Overridden | Load callback for BUILD 83: swaps its 40-byte records. |
| `sub_820CBE48` |  | Overridden | Load callback for BUILD 73 (the wheels): the wheel count, each wheel swapped, each record's model (+40) up by one, and each model's first record at params +64 + 4 x model. |
| `sub_820CB5E8` |  | Overridden | Aero part getter for BUILD 74: r3 the output (left in r3), r4 the parts (9 pointers), r5 the level; copies the record and swaps the copy. Called from `Car::ApplyAero`. |
| `sub_820CB670` |  | Overridden | Aero part getter for BUILD 75: r3 the output (left in r3), r4 the parts (9 pointers), r5 the level; copies the record and swaps the copy. Called from `Car::ApplyAero`. |
| `sub_820CB6F8` |  | Overridden | Aero part getter for BUILD 76: r3 the output (left in r3), r4 the parts (9 pointers), r5 the level; copies the record and swaps the copy. Called from `Car::ApplyAero`. |
| `sub_820CB780` |  | Overridden | Aero part getter for BUILD 77: r3 the output (left in r3), r4 the parts (9 pointers), r5 the level; copies the record and swaps the copy. Called from `Car::ApplyAero`. |
| `sub_820CB808` |  | Overridden | Aero part getter for BUILD 78: r3 the output (left in r3), r4 the parts (9 pointers), r5 the level; copies the record and swaps the copy. Called from `Car::ApplyAero`. |
| `sub_820CB890` |  | Overridden | Aero part getter for BUILD 79: r3 the output (left in r3), r4 the parts (9 pointers), r5 the level; copies the record and swaps the copy. Called from `Car::ApplyAero`. |
| `sub_820CB918` |  | Overridden | Aero part getter for BUILD 80: r3 the output (left in r3), r4 the parts (9 pointers), r5 the level; copies the record and swaps the copy. Called from `Car::ApplyAero`. |
| `sub_820CB9A0` |  | Overridden | Aero part getter for BUILD 81: r3 the output (left in r3), r4 the parts (9 pointers), r5 the level; copies the record and swaps the copy. Called from `Car::ApplyAero`. |
| `sub_820CBA30` |  | Overridden | Aero part getter for BUILD 82: r3 the output (left in r3), r4 the parts (9 pointers), r5 the level; copies the record and swaps the copy. Called from `Car::ApplyAero`. |
| `sub_820CBAC0` |  | Overridden | r3 the parts: how many of BUILD 81's first two records (the body kits) have a first float other than 0.0. |

### `native_menu_helpers`

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_82184468` | `MenuCursor::Update` | Overridden | Moves the menu cursor from the player's input, with the repeat while a direction is held. The action mapper it reads stays recompiled. |
| `sub_821843C8` | `MenuCursor::Pressed` | Overridden | Whether the player's entry is pressed this tick, from the action mapper's slots. |

### `native_race_hud`

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_82211F50` | `RaceHud::RunMovie` | Overridden | Runs the UI movie for the race HUD's state: the countdown, the timer, the race-end overlays. The movies stay recompiled. |

### `native_data_tables`

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_82116F18` | `DataTables::Init` | Overridden | Sets up the item records and the tables' blocks. |
| `sub_821173A8` | `DataTables::Load` | Overridden | Runs the twelve loads into the tables Init set up. |
| `sub_82117240` |  | Overridden | Load callback for asset 94: the item count (400) and the loads pending. |
| `sub_82117228` |  | Overridden | Load callback for asset 84: nothing to swap. |
| `sub_82117258` |  | Overridden | Swap callback for the eight 56-byte tables. |
| `sub_82117158` |  | Overridden | Swap callback for asset 95's 20-byte table. |
| `sub_821171C0` |  | Overridden | Swap callback for asset 96's 40-byte table. |
| `sub_82111A70` |  | Overridden | Byte swap of one data-table entry on its own. |
| `sub_82111AC0` |  | Overridden | Byte swap of one data-table entry on its own. |
| `sub_82112A30` |  | Overridden | Byte swap of one data-table entry on its own. |
| `sub_821172C0` | `DataTables::IsLoading` | Overridden | 1 while loads are pending. |
| `sub_821172E0` | `DataTables::Item` | Overridden | Returns the item's record. |
| `sub_82116F10` | `ItemRecords::Item` | Overridden | Returns the item's record; the item is in r4. |
| `sub_821172F8` | `DataTables::Entry56` | Overridden | An entry of one of the eight 56-byte tables. |
| `sub_82117318` | `DataTables::Find20` | Overridden | The index of the first matching entry in asset 95's table, or -1. |
| `sub_82117378` |  | Overridden | Entry lookup in the 20-byte table (Entry20) or the 40-byte table (Entry40). |
| `sub_82117390` |  | Overridden | Entry lookup in the 20-byte table (Entry20) or the 40-byte table (Entry40). |

### `native_file_layer`

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_82572488` | `File::File` | Overridden | Constructor: sets the object up and returns it in r3. |
| `sub_82572130` | `File::~File` | Overridden | Destructor. |
| `sub_82579070` | `AsyncFile::AsyncFile` | Overridden | Constructor. |
| `sub_825790B0` | `AsyncFile::~AsyncFile` | Overridden | Destructor. |
| `sub_820A69B8` | `GameFile::GameFile` | Overridden | Constructor. |
| `sub_820A69F8` | `GameFile::~GameFile` | Overridden | Destructor. |
| `sub_825791D8` | `Path::Path` | Overridden | Constructor. |
| `sub_825791F8` | `Path::~Path` | Overridden | Destructor; writes the vtable again. |
| `sub_820A6B98` | `GamePath::GamePath` | Overridden | Constructor, also used as its destructor. |
| `sub_82579250` | `Path::Build` | Overridden | Builds a path from the path (r3), the kind (r4) and the name (r5). |
| `sub_820A6BA8` | `GamePath::SetGame` | Overridden | Sets the game's folder name in the path; a name of 0 isn't checked for. |

### `native_track_zones`

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_82151288` |  | Overridden | List Link (List16, List64 or List128): the position's low halfword is linked in. |
| `sub_820FE150` |  | Overridden | List Link (List16, List64 or List128). |
| `sub_82288988` |  | Overridden | List Link (List16, List64 or List128). |
| `sub_820FE1B8` |  | Overridden | List Insert (List16, List64 or List128). |
| `sub_82289000` |  | Overridden | List Insert (List16, List64 or List128). |
| `sub_820FDE20` | `RouteRange::Init` | Overridden | Takes the start and the end, empties both lists, and sets the state and +1060 to 1. |
| `sub_820FE750` |  | Overridden | Stores a position in the caller's frame and inserts it into a list. |
| `sub_820FDD60` | `RouteRange::ToNeighbor` | Overridden | Writes the position out for a neighboring section. |
| `sub_820FDEE8` |  | Overridden | A RouteRange test (WithinEnds, OnPath, Test or Update); the position's address is in r4. |
| `sub_820FE220` |  | Overridden | A RouteRange test (WithinEnds, OnPath, Test or Update); the position's address is in r4. |
| `sub_820FE580` |  | Overridden | A RouteRange test (WithinEnds, OnPath, Test or Update); the position's address is in r4. |
| `sub_820FE700` |  | Overridden | A RouteRange member; not yet matched to its name. |
| `sub_82288928` | `Zone::Reset` | Overridden | Empties the zone's range list, then sets the index, the flag (0) and the vehicle. |
| `sub_822888E0` | `Zone::IsInside` | Overridden | Whether the zone holds the position. |
| `sub_82288F90` | `ZoneRanges::Insert` | Overridden | Links a range into the list and copies it into the new node's slot. |
| `sub_82289068` |  | Overridden | A zone's answer: the last range's, or the copy's with no ranges. |
| `sub_820C94B8` | `Racers::Vehicle` | Overridden | The racer's vehicle, or 0 when there's no racer or its flag 0x40 isn't set. |
| `sub_822888E8` | `TrackZones::Destroy` | Overridden | Deletes the owner when there is one, and clears it. |
| `sub_822891B8` | `TrackZones::IsInside` | Overridden | The zone's IsInside, by its place in the list. |

### `native_race_scene`

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_8215B628` | `RaceScene::RaceScene` | Overridden | Constructor: sets up the scene's light sets and post-effect values. |
| `sub_8215B698` | `RaceScene::Create` | Overridden | Makes the scene, or 0 when the allocation fails. |
| `sub_8215B4A8` | `RaceScene::DestroyCars` | Overridden | Frees the cars' records from the scene's +16. |
| `sub_8215B6F0` | `RaceScene::Destroy` | Overridden | Frees the cars' records and deletes the scene. |
| `sub_8215A370` | `RaceLights::InitEntry` | Overridden | Sets up one light entry, by its index. |
| `sub_8215AF70` | `RaceLights::SetUp` | Overridden | Sets up the light entries and counts the two kinds. |
| `sub_8228C498` | `Glows::SetParams` | Overridden | Sets the 11 glow floats. |
| `sub_8228C9A8` | `RaceScene::SetEffectParams` | Overridden | Sets the color filters' parameters; split screen uses filters 6 and 10, else filter 2. |
| `sub_8215A758` | `RaceSceneMachine::TogglePause` | Overridden | Toggles pause and the post effects' pause state on the scene. |
| `sub_8215B0D0` | `RaceSceneMachine::Resume` | Overridden | Clears the resume flag (+9608) and runs the post effects' pause toggle when it was set. |

### `native_shader_objects`

The graphics library's shader objects. A file's object is 592 bytes when its magic's low 7 bits are set (a vertex shader), else 56.

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_822A5B08` | `ShaderFile::Parse` | Overridden | r3 the file, r4 a 16-byte record: the file, the object's size, the microcode and its size. |
| `sub_822A5CD0` | `VertexShader::Init` | Overridden | r3 the object (cleared, 592 bytes), r4 the microcode: the fetch count at +60, the fetch words at +68, their constants at +196. |
| `sub_822A5B58` | `PixelShader::Init` | Overridden | r3 the object (cleared, 56 bytes), r4 the microcode at +24. |
| `sub_822A7348` | `VertexShader::CopyHeader` | Overridden | The 592 bytes cleared, the file's header copied after them (XMemCpy). |
| `sub_822A7210` | `PixelShader::CopyHeader` | Overridden | The 56 bytes cleared, the file's header copied after them (XMemCpy). |
| `sub_822A7388` | `VertexShader::Create` | Overridden | r3 the file: the object (XMemAlloc 0x24800000) and its microcode (0xA5800000, a 0x0BADF00D word after it, flushed); the object or 0. |
| `sub_822A7250` | `PixelShader::Create` | Overridden | As the vertex shader's, the microcode XMemAlloc'd 0xB5800000, no flush. |
| `sub_8257A620` | `Mesh::LoadShaders` | Overridden | r3 the model, r4 its shader section: the pairs' objects and microcode (AllocPhysicalEx flags 4 vertex, 0x404 pixel; the vertex microcode copied with XMemCpy, the pixel one with memcpy). |
| `sub_820A08B8` | `Shaders::OnPairLoaded` | Overridden | The archive's callback for a shared pair (r6 the context): the tables at 0x826AF998 (vertex) and 0x826AD978 (pixel), then the context deleted. |

### `native_screen_objects`

The race's screen object: +52 its strength, +76096 its UvMap. The UvMap and UvColumn trees and the rings' set-up stay recompiled.

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_8212E928` | `ScreenObject::SetSpeed` | Overridden | f1 the strength, clamped to [0.0, 1.0], into +52 (+27 = 1); when running (+24), sub_8212E440 with 0.0 first. |
| `sub_8212DDE0` | `ScreenObject::Release` | Overridden | The vertex declaration (+0) and texture (+4) released with the device locked. |
| `sub_82131B98` | `ScreenObject::~ScreenObject` | Overridden | Release, then UvMap::~UvMap on +76096. |

### `native_scene_models`

The scene models' records and links. The records' defaults and copies are tables read from SBX's code.

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_820F0CE0` | `Accessories::Get` | Overridden | [0x826B51F0], the accessories' set. |
| `sub_820D9480` | `SceneModel::Accessory` | Overridden | r3 an interior: the set's entry by its +140 (3: none), + 48, or 0. |
| `sub_82173DA0` | `SceneModel::OnSoundLoaded` | Overridden | The archive's callback for SetUp's two sounds: the slot's byte (0x826C7DF8) up, its word (0x826C7E54) = 1 at 2. |
| `sub_820E6F60` | `SceneWheels::SetRecord` | Overridden | +16776 = r4. |
| `sub_820DC710` | `SetCarModel` | Overridden | +24 = the car model. |
| `sub_820DEF70` | `SceneParts::Link` | Overridden | +24 the car model, +28 its skeleton ([model + 24] + 3760). |
| `sub_820DEF88` | `SceneParts::SetRecord` | Overridden | +37248 = r4. |
| `sub_820E3398` | `SceneInterior::Link` | Overridden | +24 the car model, +28 and +32 its skeleton and node block. |
| `sub_820E3220` |  | Overridden | +11452 = r4. |
| `sub_820D3910` | `CarModel::SetRecord` | Overridden | memcpy of the 3,696-byte car record to +32. |
| `sub_820D3780` | `Record12::Reset` | Overridden | +0 = -1, +4 and +8 the float at 0x82001BD8. |
| `sub_820D07D0` | `Layer::Reset` | Overridden | A 64-byte vinyl layer or sticker record's defaults. |
| `sub_820D0848` | `CarRecord::Reset` | Overridden | The 3,696-byte car record's defaults (its 54 layers inline). |
| `sub_820D31A0` | `PartsRecord::Reset` | Overridden | The 60-byte parts record cleared, +52 = 3. |
| `sub_820D3488` | `InteriorRecord::Reset` | Overridden | The 120-byte interior record cleared, then its defaults. |
| `sub_820D3100` | `CockpitRecord::Reset` | Overridden | +0 to +16 cleared, the interior record at +24. |
| `sub_820D3548` | `WheelsRecord::Reset` | Overridden | The 156-byte wheels record's defaults. |
| `sub_820D3128` | `Pool4Record::Reset` | Overridden | The 52-byte record cleared, then 1.0 at nine floats. |
| `sub_820D4250` | `CarRecord::SetW` | Overridden | 1.0 into the car record's rows and layers. |
| `sub_820D94D8` | `SceneModelRecord::Reset` | Overridden | The 4,244-byte record: kind 1, then each part's defaults. |
| `sub_820DAF08` | `SceneModelRecord::SceneModelRecord` | Overridden | CarRecord::SetW on +16, Record12::Reset on +4080 and +4092. |
| `sub_820D7B98` | `CarRecord::Copy` | Overridden | r3 to, r4 from: the fields word by word (SBX's own offsets) and a 44-byte memcpy of +3632. |
| `sub_820DAF58` | `SceneModelRecord::Copy` | Overridden | +0, the car record, then the other records (memcpy). |
| `sub_820C3AB0` | `CarModel::AllocPages` | Overridden | VirtualAlloc(r5 bytes) into [r4]. |
| `sub_820D9E48` | `SceneModel::Update` | Overridden | Every frame: the deferred build's countdown (+5044, +5040), then the interior's accessory anchored when there is one. |
| `sub_820E0458` | `SceneParts::Apply` | Overridden | Unless its record is kind 1, r4 copied to +108 (60 bytes) and the parts rebuilt (sub_820E0188). |
| `sub_820E3E20` | `SceneInterior::Apply` | Overridden | r4 (120 bytes) copied to +40 and the cockpit's +28, the six lights' +452, then sub_820E3500. |
| `sub_820D9A10` | `SceneModel::ReloadDecals` | Overridden | With +5036 set (not kind 3): the car record set from +48 and the decals rebuilt. |
| `sub_820D9AC8` | `SceneModel::AddSecondWheels` | Overridden | Once (+5032): the scene's pool 5 entry made and linked; 1, or 0 when done before. |

### `native_save_data`

The save data's defaults. The resets are tables read from SBX's code. Progress::SetDefaults (`sub_82113FB0`) is `unlock_cut_cars`' override and is called.

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_82105830` | `Stats::Stats` | Overridden | The stats' records' defaults. |
| `sub_82113828` | `Stats::ResetRecords` | Overridden | The 15 records' defaults, then +0 = 0. |
| `sub_82113880` | `Stats::Reset` | Overridden | The stats cleared and their defaults. |
| `sub_82113D28` | `SaveData::Clear` | Overridden | Progress::Clear: the 119,908 bytes cleared, its fields, Stats::Reset on +48. |
| `sub_821134F8` | `BitSet400::SetAll` | Overridden | Bits 0 to 399 set. |
| `sub_82105A18` | `ControlMap::SetDefault` | Overridden | The 15 actions' buttons. |
| `sub_82105AB8` | `Options::SetDefaults` | Overridden | The options and their four control maps. |
| `sub_82105E98` | `CourseRecords::Reset` | Overridden | The header cleared, then the 22 records' own reset (sub_82105E20). |
| `sub_82105748` | `SaveData::Delete` | Overridden | The slot's save (0x826B5838 + 4 x slot) deleted and cleared. |
| `sub_82105938` | `SaveData::Create` | Overridden | A save (129,200 bytes) made for the slot and its defaults set. |
| `sub_8209F718` | `Players::ResetControllers` | Overridden | Controller i = i (0x826AA884), then 0x826B73F8 = 1. |
| `sub_82209868` | `MenuList::Init` | Overridden | A menu list: its count, shown, cursor words, and its flags VirtualAlloc'd anew; the 9th and 10th arguments at the caller's r1 + 84 and + 92. |
| `sub_8221F7A8` | `TitleScreen::ResetPlayer` | Overridden | The title's menu list set up, the player flags, save 0 made again, the controllers reset. |
| `sub_820CCA60` | `Defaults::Table50A8` | Overridden | 0x826B4D88. |
| `sub_820CCA70` | `Defaults::Table3044` | Overridden | 0x826B2D24. |
| `sub_820CCA80` | `Defaults::Table3E34` | Overridden | 0x826B3B14. |
| `sub_820CCA90` | `Defaults::Table3C24` | Overridden | 0x826B3904. |
| `sub_820CCAA0` | `Defaults::Table50E4` | Overridden | 0x826B4DC4. |

### `native_accessories`

The interior accessories' set at [0x826B51F0] and their 36-byte files. The archive, the entries' and accessories' own constructors and destructors stay recompiled.

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_820F0AE0` | `AccessoryFile::AccessoryFile` | Overridden | The vtable (0x82006710) and the defaults. |
| `sub_820F0A80` | `AccessoryFile::SetData` | Overridden | r4 the data, r5 whether it's the file's own; loaded. |
| `sub_820F0B20` | `AccessoryFile::Load` | Overridden | r4 the asset, r5 a buffer (0: its own pages), r6 a callback and r7 its context: Archive::Load with OnArchiveDone; the serial. |
| `sub_820F0AB0` | `AccessoryFile::OnArchiveDone` | Overridden | For the file's serial: loaded, then its callback with r6 its context. |
| `sub_820F0760` | `AccessoryFile::FreeData` | Overridden | [r3] VirtualFree'd and cleared. |
| `sub_820F0BC0` | `AccessoryFile::Reset` | Overridden | Its own data cancelled or freed, its defaults. |
| `sub_820F0648` | `AccessoriesBase::AccessoriesBase` | Overridden | The vtable (0x82006704), the three entries 0. |
| `sub_820F0CF0` | `Accessories::Accessories` | Overridden | The base, the vtable (0x82006718), the scale 1.0. |
| `sub_820F0DD8` | `Accessories::New` | Overridden | 32 bytes of pages made a set, or 0. |
| `sub_820F0EC8` | `Accessories::Create` | Overridden | The set made once. |
| `sub_820F0D90` | `Accessories::SetScale` | Overridden | r4 the scale to each entry's +1072 and the set's +16. |
| `sub_820F0E20` | `Accessories::Free` | Overridden | The set at [r3] destroyed (vtable slot 0) and its pages freed. |
| `sub_820F0F08` | `Accessories::Destroy` | Overridden | The set freed, if there is one. |
| `sub_820F0968` | `Accessories::ReleaseAll` | Overridden | Each held entry released once. |
| `sub_820F0920` | `Accessories::Release` | Overridden | r4 the index (3: none): its count down, destroyed at 0. |
| `sub_820F0870` | `Accessories::AcquireEntry` | Overridden | r4 the index, r5 the texture: the entry made (its file loaded) or its count up; the accessory, or 0. |
| `sub_820F0D48` | `Accessories::Acquire` | Overridden | AcquireEntry, then the set's scale to the accessory's +1024. |

### `native_strings_and_font`

The string table at [0x826B5914] (2,256 bytes: +0 the 17 files, +68 their loaded flags, +136 the mask, +2188 the counts). The font's drawing and the formatting stay recompiled.

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_8210A990` | `Strings::Init` | Overridden | The table made once (VirtualAlloc). |
| `sub_8210A980` | `Strings::OnLoaded` | Overridden | The archive's callback: [r6] = 1. |
| `sub_8210A668` | `Strings::Release` | Overridden | r4 a mask: each loaded file's count down, freed at 0. |
| `sub_821082C0` | `Font::IsLoaded` | Overridden | 1 when a page is asked for and each one asked for is loaded. |
| `sub_8210A898` | `Strings::IsLoaded` | Overridden | 1 when a file is asked for, each one is loaded, and the font is. |
| `sub_8220FA18` | `Text::IsLoaded` | Overridden | The table and the font. |
| `sub_8210A930` | `Strings::Get` | Overridden | r4 the id: its file by the bounds at 0x82007DB0, DataFile::GetEntry of the rest. |
| `sub_8210A6F0` | `Strings::PageCount` | Overridden | r4 the text: its page breaks (0xFFD1). |
| `sub_8210A718` | `Strings::CopyPage` | Overridden | r4 out, r5 the text, r6 the page, r7 the lines to skip: the page appended (wcscat). |

### `native_game_modes`

The modes' state machines: up to 200 entries of 48 bytes (a 16-byte key, the handler at +16), the count at +9600. StateMachine::Handle (`sub_82106410`) keeps `title_screen.cpp`'s override, which calls ours.

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_82106980` | `Modes::Modes` | Overridden | r4 the owner: the vtable and the modes' globals. |
| `sub_821061D0` | `StateKey::Equals` | Overridden | The two keys' four words. |
| `sub_82106230` | `StateMachine::IndexOf` | Overridden | The key's entry, or -1. |
| `sub_821062E8` | `StateMachine::Add` | Overridden | r4 the key, r5 the handler: replaced, or a new entry. |
| `sub_82106380` | `StateMachine::Find` | Overridden | The key's handler, or the default one (0x826B5848, made once). |
| `sub_82107230` | `StateMachine::StateMachine` | Overridden | The 200 entries cleared, their handlers empty (0x82006954). |
| `sub_82106D98` | `Mode::Mode` | Overridden | The vtables. |
| `sub_8215BCB8` | `RaceOverlaysMachine::SetFlags` | Overridden | The 25 overlays' +772 and +776 = 1. |
| `sub_8215BDA0` | `RaceOverlaysMachine::ClearFlags` | Overridden | +772, +776 and +780 = 0. |
| `sub_8215BCF0` | `RaceOverlaysMachine::SetFlag820` | Overridden | +772 = 1, +776 = 0. |
| `sub_8215BD30` | `RaceOverlaysMachine::ResetOverlays` | Overridden | Each overlay's vtable slot 3, then +780 = 1. |

### `native_screen_constructors`

The front end's screen constructors: their stores are tables read from SBX's code, their members' constructors called in order.

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_82256928` | `NodeBase::NodeBase` | Overridden | Constructor. |
| `sub_8224C888` | `PanelGroupBase::PanelGroupBase` | Overridden | Constructor. |
| `sub_8224B148` | `PanelCBase::PanelCBase` | Overridden | Constructor. |
| `sub_8224A9F0` | `PanelDBase::PanelDBase` | Overridden | Constructor. |
| `sub_8224C958` | `PanelEBase::PanelEBase` | Overridden | Constructor. |
| `sub_8222E2A0` | `PanelE::PanelE` | Overridden | Constructor. |
| `sub_8224D468` | `PanelFBase::PanelFBase` | Overridden | Constructor. |
| `sub_8222F730` | `Panel6C::Panel6C` | Overridden | Constructor. |
| `sub_8223F270` | `Panel7C::Panel7C` | Overridden | Constructor. |
| `sub_8222F8A0` | `Panel6D::Panel6D` | Overridden | Constructor. |
| `sub_8223F450` | `Panel7D::Panel7D` | Overridden | Constructor. |
| `sub_8222E388` | `Panel6F::Panel6F` | Overridden | Constructor. |
| `sub_8223DE58` | `Panel7F::Panel7F` | Overridden | Constructor. |
| `sub_8222E128` | `PanelGroup::PanelGroup` | Overridden | Constructor. |
| `sub_8222F6C8` | `PanelGroup6B::PanelGroup6B` | Overridden | Constructor. |
| `sub_8223F218` | `PanelGroup7B::PanelGroup7B` | Overridden | Constructor. |
| `sub_82248770` | `PanelSetBase::PanelSetBase` | Overridden | Constructor. |
| `sub_822477C0` | `GroupPair::GroupPair` | Overridden | Constructor. |
| `sub_8224A9A8` | `MachineScreen::MachineScreen` | Overridden | Constructor. |
| `sub_82233FE8` | `Type6Screen0::Type6Screen0` | Overridden | Constructor. |
| `sub_82243B48` | `Type7Screen0::Type7Screen0` | Overridden | Constructor. |
| `sub_822457A0` | `NodeTable::NodeTable` | Overridden | Constructor. |
| `sub_82245818` | `PairedNodeTable::PairedNodeTable` | Overridden | Constructor. |
| `sub_822458D0` | `ColorNodeTable::ColorNodeTable` | Overridden | Constructor. |
| `sub_8224FC08` | `ListScreen::ListScreen` | Overridden | Constructor. |
| `sub_8224FD70` | `ListPartBase::ListPartBase` | Overridden | Constructor. |
| `sub_8224FF58` | `ListPartInnerBase::ListPartInnerBase` | Overridden | Constructor. |
| `sub_82250018` | `ListPartInner::ListPartInner` | Overridden | Constructor. |
| `sub_82250280` | `ListPart::ListPart` | Overridden | Constructor. |
| `sub_822549B0` | `NodeListScreen::NodeListScreen` | Overridden | Constructor. |
| `sub_82254B38` | `TitledListScreen::TitledListScreen` | Overridden | Constructor. |
| `sub_82257D50` | `NodeScreen::NodeScreen` | Overridden | Constructor. |
| `sub_82235090` | `Type6NodeScreen::Type6NodeScreen` | Overridden | Constructor. |
| `sub_82234B50` | `Type6Screens::Type6Screens` | Overridden | Constructor. |
| `sub_822446C8` | `Type7Screens::Type7Screens` | Overridden | Constructor. |

### `native_collision`

The collision shapes: their constructors, getters, bounds and placements. The box-box tests, impacts and pair solver stay recompiled.

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_820BD000` | `Query::Query` | Overridden | Constructor (a table). |
| `sub_820BAE70` | `Shape::Shape` | Overridden | Constructor (a table). |
| `sub_820BAFD0` | `BoxShape::BoxShape` | Overridden | Constructor: Shape::Shape, its table, BoxShape::SetCorners. |
| `sub_820B9D38` | `BoxShape::Corner` | Overridden | r3 + 176 + 16 x r4. |
| `sub_820B9D48` | `BoxShape::Face` | Overridden | r3 + 304 + 32 x r4. |
| `sub_820BA4C8` | `BoxShape::SetCacheWord` | Overridden | +160 + 4 x r4 = r5. |
| `sub_820BA4D8` | `BoxShape::CacheWord` | Overridden | +160 + 4 x r4. |
| `sub_820BA8A8` | `BoxShape::CopyCache` | Overridden | r4's 16-byte cache into r3's. |
| `sub_820BA998` | `Shape::Radius` | Overridden | f1 = +32. |
| `sub_820BA1C8` | `BoxShape::LowerCorner` | Overridden | r4 the axis: the corner of the axis' face with the lower distance. |
| `sub_820BA210` | `BoxShape::FaceDistance` | Overridden | r4 the face, r5 a point: the dot with its direction less its distance. |
| `sub_820BA150` | `BoxShape::Translate` | Overridden | r4 added to the corners and center, the faces' distances moved. |
| `sub_822B8A90` | `WordAt8` | Overridden | r3's +8 (shared). |
| `sub_820BA978` | `CollisionShape::CurrentCenter` | Overridden | +8's +48. |
| `sub_820BA988` | `CollisionShape::PreviousCenter` | Overridden | +4's +48. |
| `sub_820BAB08` | `CollisionShape::Type` | Overridden | The shape's vtable slot 4. |
| `sub_820BAC48` | `CollisionShape::Previous` | Overridden | The shape's slot 4, then +4. |
| `sub_820BAC88` | `CollisionShape::Current` | Overridden | The shape's slot 4, then +8. |
| `sub_820BAA50` | `CollisionShape::UpdateBounds` | Overridden | Every tick: the box around the previous and current centers, grown by the radius. |
| `sub_820BA8F8` | `CollisionShape::~CollisionShape` | Overridden | The two placements deleted and cleared. |
| `sub_822DF518` | `Query::SetShapes` | Overridden | +0, +4 = r4, r5 (shared). |
| `sub_820BC740` | `Contact::Set` | Overridden | r4 to +0, r5 to +16 and +32, f1 and f2 to +48 and +52. |
| `sub_820BC690` | `Contact::Flip` | Overridden | +16 and +32 scaled by -1. |
| `sub_82468988` | `BoxShape::CornerCount` | Overridden | 8 (a constant getter the linker shares). |
| `sub_82469648` | `BoxShape::FaceCount` | Overridden | 6 (shared). |
| `sub_820B9D58` | `BoxShape::EdgeCount` | Overridden | 12 (shared). |
| `sub_820B9DF8` | `BoxShape::EdgeStart` | Overridden | The edge r4's first corner (0x825F0D04). |
| `sub_820B9E10` | `BoxShape::EdgeEnd` | Overridden | The edge r4's second corner (0x825F0D34). |
| `sub_820B9E28` | `BoxEdges::Start` | Overridden | The edge r3's first corner. |
| `sub_820B9E40` | `BoxEdges::End` | Overridden | The edge r3's second corner. |

### `native_car_physics`

The car physics' set-ups and accessors. Float steps are the originals'. The vehicle's step and the collision world stay recompiled.

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_820AEBA0` | `CarData::SetDefaults` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820AEE68` | `CarData::CarData` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820B4160` | `Vehicle::Vehicle` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820BC280` | `CollisionSurfaces::Reset` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820BC398` | `CollisionWorld::ResetSurfaces` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820BA8D0` | `CollisionShape::Init` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820B7988` | `CollisionObject::CollisionObject` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820B79E0` | `CollisionObject::~CollisionObject` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820B77C8` | `RigidBody::RigidBody` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820B6180` | `Vehicle::RefreshFromBody` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820B3AB8` | `Vehicle::SetValue2388` | Overridden | +2388 = f1 clamped, +2385 = r4's byte, +2396 = 1.0 - f1 x k clamped (fused). |
| `sub_820B3B28` | `Vehicle::SetGearValues` | Overridden | The gear's ratios into +2304 and +2308, +2312 to +2324. |
| `sub_820B0360` | `Vehicle::RampReverseThrottle` | Overridden | +288 ramped by the car's rate (fused) up to 1.0, or 0.0. |
| `sub_820B03A8` | `Vehicle::RampHandbrake` | Overridden | +296 the same. |
| `sub_820B03F0` | `Vehicle::SurfaceValue` | Overridden | r4 the wheel: its surface record's +0, or 0.0. |
| `sub_820B0938` | `Wheel::Copy` | Overridden | r4 into r3, field by field. |
| `sub_820B0BA8` | `WheelRecord2::Copy` | Overridden | The 120 bytes. |
| `sub_820B7050` | `RigidBody::SetCenterOfMassPosition` | Overridden | sub_82291258 on +224, then UpdateOrigin. |
| `sub_820B7088` | `RigidBody::SetOrientation` | Overridden | r4 to +80 normalized, then UpdateOrientation and UpdateOrigin. |
| `sub_820B70D8` | `RigidBody::SetVelocity` | Overridden | r4 scaled by +4 into +176, then UpdateVelocity. |
| `sub_820B7120` | `RigidBody::SetInertia` | Overridden | +352 and their inverses at +368, then UpdateOrientation. |
| `sub_820BC040` | `CollisionSurface::A` | Overridden | f1 by r3's bits (0x01000000, 0x02000000, 0x04000000, else the default). |
| `sub_820BC088` | `CollisionSurface::B` | Overridden | As A. |
| `sub_820BC0D0` | `CollisionSurface::C` | Overridden | As A. |
| `sub_820BC118` | `CollisionSurface::D` | Overridden | As A. |
| `sub_820BC378` | `CollisionSurface::OfC` | Overridden | C. |
| `sub_820BC380` | `CollisionSurface::OfD` | Overridden | D. |
| `sub_820BC388` | `CollisionSurface::OfA` | Overridden | A. |
| `sub_820BC390` | `CollisionSurface::OfB` | Overridden | B. |
| `sub_820B3CA0` | `Vehicle::SetBrake` | Overridden | f1 clamped to [0.0, 1.0] into +292. |
| `sub_820B3CD0` | `Vehicle::SetHandbrake` | Overridden | f1 clamped into +296. |
| `sub_820B3D00` | `Vehicle::SetThrottle` | Overridden | f1 clamped into +284. |
| `sub_820B0448` | `Vehicle::SurfaceValue20` | Overridden | The record's +20 (0.0), into the wheel's +612. |
| `sub_820B04A0` | `Vehicle::SurfaceValue12` | Overridden | The record's +12 (1.0), into +600. |
| `sub_820B04F8` | `Vehicle::SurfaceValue16` | Overridden | The record's +16 (1.0), into +604. |
| `sub_820BC160` | `CollisionSurface::SetA` | Overridden | r4's bits, f1. |
| `sub_820BC1A8` | `CollisionSurface::SetB` | Overridden | As SetA. |
| `sub_820BC1F0` | `CollisionSurface::SetC` | Overridden | As SetA. |
| `sub_820BC238` | `CollisionSurface::SetD` | Overridden | As SetA; its default slot differs from D's getter, as in the original. |

### `native_vehicle`

The vehicles' construction, refresh, removal and route positions.

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_820BFF50` | `Vehicle::Refresh` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820C2000` | `Vehicles::DestroyAll` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820C2EF8` | `Vehicle::Construct` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820C1F70` | `TrackPosition::Copy` | Overridden | r4 into r3. |
| `sub_820C1B98` | `Vehicle::UpdateTrackPositions` | Overridden | Every tick: the body's and the wheels' points into their route positions, each updated. |
| `sub_820C1F00` | `Vehicle::RemoveFromWorld` | Overridden | The collision object taken out of the world, the other object deleted. |

### `native_race_states`

The race states' simple handlers, the progress' reset and the route helpers. The race's ticks stay recompiled.

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_82161218` | `RaceStates::ClearStep` | Overridden | Its stores as a table, its calls in their place. |
| `sub_82161238` | `RaceStates::OnQuit` | Overridden | Its stores as a table, its calls in their place. |
| `sub_82161248` | `RaceStates::FadeInFree` | Overridden | Its stores as a table, its calls in their place. |
| `sub_82161278` | `RaceStates::FadeOut` | Overridden | Its stores as a table, its calls in their place. |
| `sub_821617F0` | `RaceStates::Unload` | Overridden | Its stores as a table, its calls in their place. |
| `sub_82162280` | `RaceStates::OnStandingStart` | Overridden | Its stores as a table, its calls in their place. |
| `sub_821627B0` | `RaceProgress::Reset` | Overridden | Its stores as a table, its calls in their place. |
| `sub_82162C90` | `RaceStates::Begin` | Overridden | Its stores as a table, its calls in their place. |
| `sub_821600F8` | `Time::Sub` | Overridden | [r3] - [r4] in milliseconds, a time word again (sub_82101850). |
| `sub_82160170` | `RoutePosition::NotAfter` | Overridden | 1 when r3's position isn't after r4's. |
| `sub_82160CB0` | `RaceProgress::CountTime` | Overridden | r5's count up and its time (sub_821018B0). |
| `sub_82160D18` | `RouteSections::Before` | Overridden | The section (r3 or a neighbor) whose link ahead is r4, or -1. |
| `sub_82160E20` | `RouteSections::Beside` | Overridden | 1 when r3 is r4 or beside it. |

### `native_material_passes`

The part models' materials: counts, records, alpha setters, the paint table's lookups.

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_820D3218` | `TableVector` | Overridden | r3 = the paint table's vector (r4, r5). |
| `sub_820D3248` | `IsBlockSetA` | Overridden | 1 for 1, 26, 28, 33 and 36. |
| `sub_820D3280` | `IsBlockSetB` | Overridden | 1 for 10 and 37. |
| `sub_820D3370` | `RecordAt` | Overridden | The paint record r3 (0x82003660, 48 bytes). |
| `sub_8257A3C0` | `Model::MaterialTotal` | Overridden | r4 the mesh: its material count. |
| `sub_8257A400` | `Model::MaterialAt` | Overridden | r4 the mesh, r5 the material. |
| `sub_820D1848` | `PartModel::SetAlpha` | Overridden | f1 into each material's +12, both passes. |
| `sub_820D1780` | `PartModel::SetPair1002Alpha` | Overridden | The same for the materials of kind 1002. |

### `native_replay_snapshots`

The race snapshots' copies, field by field in the originals' order. The replay's own save and restore stay recompiled.

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_82104480` | `Record5232::RestoreTrack` | Overridden | Copy, field by field. |
| `sub_82103320` | `BodyState::Restore` | Overridden | Copy, field by field. |
| `sub_82102DE8` | `Member1136::Restore` | Overridden | Copy, field by field. |
| `sub_82102C30` | `Part464::Restore` | Overridden | Copy, field by field. |
| `sub_821044F0` | `Record5232::SaveTrack` | Overridden | Copy, field by field. |
| `sub_82103B68` | `BodyState::Save` | Overridden | Copy, field by field. |
| `sub_82103638` | `Member1136::Save` | Overridden | Copy, field by field. |
| `sub_82103480` | `Part464::Save` | Overridden | Copy, field by field. |
| `sub_82104560` | `Record5232::CopyTrack` | Overridden | Copy, field by field. |
| `sub_82103CC8` | `BodyState::Copy` | Overridden | Copy, field by field. |
| `sub_82104330` | `Part464::Copy` | Overridden | Copy, field by field. |
| `sub_821045D0` | `Member4080::CopyShapes` | Overridden | Copy, field by field. |
| `sub_82104708` | `Member4080::Copy` | Overridden | Copy, field by field. |

### `native_ui_movies`

The UI movies' file lookups. The movies' update, text, tracks and drawing stay recompiled (`sub_8211D4B8` and `sub_82585C40` are subtick's).

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_8211D378` | `UiMovie::DestroyNamedAnimations` | Overridden | Its stores and calls as the original. |
| `sub_8211E380` | `UiMovie::UiMovie` | Overridden | Its stores as a table, its calls in their place. |
| `sub_8258BCC8` | `Animation::NodeTrack` | Overridden | r3 + 16 x (r4 + 1). |
| `sub_8258BCD8` | `Track::Key` | Overridden | r3 + 32 x r4 + 16. |
| `sub_82582CF8` | `VertexOffset` | Overridden | 16 x (r3 + 2 x (r4 + 4)). |
| `sub_82586358` | `IsXmd` | Overridden | "XMD\0001\0" at r3. |
| `sub_82586538` | `IsMovieFile` | Overridden | "X2D\0005\0" at r3 + 8. |
| `sub_8211B988` | `UiMovie::NodeWord24` | Overridden | The node r4's +24, or 0. |
| `sub_8211BB88` | `UiMovie::NodeWord4` | Overridden | The node r4's +4, or 0. |
| `sub_8258BCE8` | `MovieFile::NodeTrackList` | Overridden | The file's table entry r4 (+32 its count). |
| `sub_8258BD00` | `MovieFile::KeyList` | Overridden | The table entry r4 (+40). |
| `sub_8258BD18` | `MovieFile::SubAnimation` | Overridden | The table entry r4 (+48). |
| `sub_825865D0` | `MovieFile::Animation` | Overridden | The table entry r4 (+16). |

### `native_data_load`

The boot data's set-up.

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_82245750` | `LoadingScreen::Load` | Overridden | Its stores as a table, its calls in their place. |
| `sub_82105618` | `InputRecordings::InputRecordings` | Overridden | Its stores as a table, its calls in their place. |
| `sub_825BF238` | `Replay826D8520::StaticInit` | Overridden | Its stores as a table, its calls in their place. |
| `sub_825BF340` | `Replay82816E50::StaticInit` | Overridden | Its stores as a table, its calls in their place. |
| `sub_82104410` | `Part464::Part464` | Overridden | Its stores as a table, its calls in their place. |
| `sub_82104800` | `Member4080::Member4080` | Overridden | Its stores as a table, its calls in their place. |
| `sub_82102A10` | `Member1136::Member1136` | Overridden | Its stores as a table, its calls in their place. |
| `sub_82104D90` | `Record5232::Record5232` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820CCAB0` | `Bits224::Fill` | Overridden | Bits 0 to 198 set. |
| `sub_820CCB08` | `Bits480::Fill` | Overridden | Bits 0 to 453 set. |
| `sub_82102390` | `RaceData::Create` | Overridden | The race data (827,904 bytes) made into [0x826B580C]. |
| `sub_82105678` | `InputRecordings::Create` | Overridden | Made into [0x826B582C]. |
| `sub_821056D0` | `Replay::Create` | Overridden | Made with its seven records into [0x826B5830]. |
| `sub_821059D8` | `BootData::CreateObjects` | Overridden | The boot's objects, in order. |
| `sub_821067A0` | `BootData::Load` | Overridden | Once: the boot's steps. |

### `native_race_sounds`

The race sounds' handlers. The sounds' ticks stay recompiled.

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_82173B18` | `RaceSounds::ResumeCars` | Overridden | Its stores as a table, its calls in their place. |
| `sub_82173D90` | `RaceSounds::OnEffectsBankLoaded` | Overridden | Its stores as a table, its calls in their place. |
| `sub_82173760` | `RaceSounds::SetListener` | Overridden | The sound manager's slots 26, 25 and 27. |
| `sub_82173BE0` | `RaceSounds::OnStageCountdown` | Overridden | The effects' volume back, the cars set up. |
| `sub_82173BB8` | `RaceSounds::OnAbortDone` | Overridden | The same, the volume last. |
| `sub_82173C08` | `RaceSounds::OnRollingStartDone` | Overridden | The music stopped once during the rolling start, the cars set up. |
| `sub_82174828` | `RaceSounds::OnCountdown` | Overridden | The music back, the rival music picked. |
| `sub_82174D20` | `RaceSounds::OnLeaveAfterRace` | Overridden | The cars' sounds updated, the music and effects back. |
| `sub_82174D88` | `RaceSounds::OnNextStage` | Overridden | The music stopped and back, the rival music picked. |
| `sub_82173B60` | `RaceSounds::OnAbort` | Overridden | The music stopped and back. |
| `sub_82174F80` | `RaceSounds::OnRollingStart` | Overridden | The cars' sounds set up, the music back, the rival music picked. |

### `native_race_screens`

The race screens' small parts. The screens' ticks and draws stay recompiled.

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_82209840` | `ScreenPages::Init` | Overridden | Its stores as a table, its calls in their place. |
| `sub_822159C8` | `RaceDialog::Destroy` | Overridden | Its stores as a table, its calls in their place. |
| `sub_82218140` | `AfterRaceMenu::Close` | Overridden | Its stores as a table, its calls in their place. |
| `sub_8220B5C8` | `PauseMenu::Close` | Overridden | Its stores as a table, its calls in their place. |
| `sub_82209810` | `ScreenPages::Go` | Overridden | r4 the page, r5 the next (-1 none), r6 its own. |
| `sub_822097D0` | `ScreenPages::Finish` | Overridden | The pending page taken. |
| `sub_822099C8` | `MenuList::SetSelection` | Overridden | +16 = r4, +20 the old one. |
| `sub_822099A8` | `MenuList::SetEnabled` | Overridden | The entry r4's flag = r5. |
| `sub_822099B8` | `MenuList::IsEnabled` | Overridden | The entry r4's flag. |
| `sub_8220A368` | `PauseMenu::AnswerPage` | Overridden | [[0x826C7F7C] + 140]. |
| `sub_822183C8` | `AfterRaceMenu::AnswerPage` | Overridden | [[0x826C7FB4] + 12]. |
| `sub_82215A38` | `RaceDialog::Answer` | Overridden | +32 cleared; 1 for choice 1, else 2. |
| `sub_82216E08` | `CareerResults::Create` | Overridden | 212 bytes of pages into [0x826C7FA8]. |
| `sub_8221CF18` | `RaceResults::Create` | Overridden | 52 bytes into [0x8280DE28]. |
| `sub_8221D5B8` | `SplitResults::Create` | Overridden | 40 bytes into [0x8280DE2C]. |
| `sub_82219508` | `AfterRaceMenu::Start` | Overridden | Its set-up, its pages to page 1. |
| `sub_82216EA0` | `CareerResults::Start` | Overridden | Its pages to page 1. |

### `native_race_hud_object`

The race HUD object. The UI movies and the HUD's set-up stay recompiled.

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_82210028` | `RaceHud::ClearWarning` | Overridden | Its stores as a table, its calls in their place. |
| `sub_8220FFF8` | `RaceHud::StartCountdown` | Overridden | Its stores as a table, its calls in their place. |
| `sub_822104A0` | `RaceHud::StartMovie1` | Overridden | Its stores as a table, its calls in their place. |
| `sub_82210600` | `RaceHud::FinishMovie1` | Overridden | Its stores as a table, its calls in their place. |
| `sub_82210880` | `RaceHud::RunMovie1` | Overridden | Its stores as a table, its calls in their place. |
| `sub_8220FA90` | `RaceScreens::DrawPauseMenu` | Overridden | Its stores as a table, its calls in their place. |
| `sub_8220FA98` | `RaceScreens::DrawAfterRaceMenu` | Overridden | Its stores as a table, its calls in their place. |
| `sub_82210BA0` | `RaceHud::SetBoost` | Overridden | Node 5's +36 = the boost. |
| `sub_82210A90` | `RaceHud::ShowGauge` | Overridden | r5 into nodes 12 + r4 and 9 + r4. |
| `sub_82210B38` | `RaceHud::SetGear` | Overridden | The gear into node 11's frame. |
| `sub_82210418` | `RaceHud::SetPlayer` | Overridden | +16 the player, +440 its vehicle. |
| `sub_82213608` | `RaceHud::Touching` | Overridden | Whether r4's vehicle touches groups 0x07000000. |
| `sub_82210850` | `RaceHud::DrawWarning` | Overridden | With +468 set, the warning movie drawn. |
| `sub_82210630` | `RaceHud::Draw` | Overridden | The movie run, then node 19 drawn with +472 and +12 set. |
| `sub_82211FA0` | `RaceHud::UpdateItem` | Overridden | The race data's item shown when it's changed. |
| `sub_82211F08` | `RaceHud::TearDown` | Overridden | sub_822888E8, then its node list freed. |
| `sub_822119A8` | `PowInt` | Overridden | f1 to the power r4 (doubles). |

### `native_course_scenery`

The course scenery's pool objects and lookups. `sub_820F1038` keeps subtick's override, which calls ours.

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_820FB8B8` | `PoolObject::Reset` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820F18B0` | `ModelList::Init` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820F1310` | `Scenery::ClearGlobal` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820F20C0` | `CourseSet::InitParts` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820F0FC8` | `CourseSet::Reset` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820FB798` | `PoolObject::AllocData` | Overridden | r4 bytes (to 64) from its data. |
| `sub_820FB878` | `PoolObject::ClearLoadFlags` | Overridden | 0x3C cleared. |
| `sub_820FB888` | `PoolObject::MarkLoaded` | Overridden | 0x2 cleared, 0x8 set. |
| `sub_824A7668` | `PoolObject::Key` | Overridden | No key (shared). |
| `sub_820F1AB0` | `AreaLights::Key` | Overridden | 4 bytes at +84. |
| `sub_820F1B68` | `AreaModel::Key` | Overridden | 4 bytes at +64. |
| `sub_820FB9F0` | `PoolObject::PoolObject` | Overridden | Constructor. |
| `sub_820FB8D0` | `PoolObject::SetKey` | Overridden | The key copied where its Key slot says. |
| `sub_820FB9E0` | `ObjectPools::Pool` | Overridden | The pool r4. |
| `sub_820FBFB8` | `ObjectPools::Object` | Overridden | The pool r4's object r5. |
| `sub_820FBEA0` | `ObjectPools::Load` | Overridden | The object, its slot 1. |
| `sub_820F13C0` | `SceneryConfig::ModelAsset` | Overridden | The model r4's r5-th asset, or -1. |
| `sub_820F1418` | `SceneryList::Clear` | Overridden | r4's +4 and +8 cleared. |
| `sub_820F1320` | `Scenery::AllocPhysical` | Overridden | Physical memory, counted in (0x826B51F8). |
| `sub_820F1378` | `Scenery::FreePhysical` | Overridden | Counted out, then freed. |
| `sub_82145350` | `Animation::Advance` | Overridden | Every frame: one step on (frame, key). |
| `sub_821453B0` | `AnimationSet::Advance` | Overridden | Each animation one step on. |
| `sub_820FB7D0` | `PoolObject::IsLoaded` | Overridden | Flags 0x9. |
| `sub_820FB7E8` | `PoolObject::IsVisible` | Overridden | Flags 0x19. |
| `sub_820FB800` | `PoolObject::IsExpiring` | Overridden | Flags 0x29. |
| `sub_820FB818` | `PoolObject::SetVisible` | Overridden | 0x10 cleared, or set once loaded. |
| `sub_820FB848` | `PoolObject::SetExpiring` | Overridden | 0x20 cleared, or set once loaded. |

### `native_scene_loaders`

The scene loaders' constructors, destructors, resets and load checks. The loads stay recompiled.

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_820D3A80` | `CarModelLoads::CarModelLoads` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820D3AE0` | `CarModelLoads::~CarModelLoads` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820D5CB8` | `CockpitLoads::CockpitLoads` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820D5D38` | `CockpitLoads::~CockpitLoads` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820D78F0` | `TrafficLoads::TrafficLoads` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820D7970` | `TrafficLoads::~TrafficLoads` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820D8C28` | `WheelsLoads::WheelsLoads` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820D8CD8` | `WheelsLoads::~WheelsLoads` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820D6C38` | `CarPartsLoads::CarPartsLoads` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820D69B0` | `CarParts::Release` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820D6C98` | `CarPartsLoads::~CarPartsLoads` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820D8810` | `Wheels::Reset` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820D65F8` | `Pool4Object::Reset` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820D66D0` | `Pool4Object::Release` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820D8B30` | `Wheels::Release` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820D6710` | `Pool4Object::SetRecord` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820D38C8` | `CarModel::IsLoaded` | Overridden | 0 while either load is 2; else its state isn't 2. |
| `sub_820D66B8` | `Pool4Object::IsLoaded` | Overridden | 1 unless +100 is 2. |
| `sub_8257C838` | `XPR::Count` | Overridden | +20, or 0 without a header. |

### `native_scene`

The scene's set-ups. Its updates stay recompiled.

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_820D0AE0` | `SceneSetUp::SceneSetUp` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820D0750` | `FreePagesIn` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820C4E00` | `SceneAssets::SceneAssets` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820C4F58` | `SceneAssets::~SceneAssets` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820C8760` | `CarScene::CarScene` | Overridden | Its stores as a table, its calls in their place. |
| `sub_82579750` | `List::~List` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820C4A80` | `SceneAssets::Release` | Overridden | Its stores as a table, its calls in their place. |

### `native_second_set`

The scene parts' point tables, the models' streams and the morph's run.

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_820DF160` | `PartsPoints::InitLeftDoor` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820DF1B8` | `PartsPoints::InitRightDoor` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820E0F48` | `PartsPoints::InitHeadlights` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820DFBB8` | `PartsPoints::InitMufflers` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820E25D8` | `PartsPoints::PartsPoints` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820DBE00` | `CockpitPoints::CockpitPoints` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820E4BC8` | `InteriorPoints::InteriorPoints` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820DD970` | `LightPoints::LightPoints` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820DC518` | `LightPoints::Reset` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820E64D8` | `TrafficPoints::TrafficPoints` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820E5470` | `TrafficPoints::Reset` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820E8668` | `WheelPoints::InitTire` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820E95A8` | `WheelPoints::InitRimA` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820EA4D8` | `WheelPoints::InitRimB` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820EAF18` | `WheelPoints::InitBrakes` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820EC880` | `WheelPoints::WheelPoints` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820E81B0` | `WheelPoints::Reset` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820E27D8` | `PartsPoints::~PartsPoints` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820DBEC8` | `CockpitPoints::~CockpitPoints` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820E4CB0` | `InteriorPoints::~InteriorPoints` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820DDA08` | `LightPoints::~LightPoints` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820EC8F0` | `WheelPoints::~WheelPoints` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820E6570` | `TrafficPoints::~TrafficPoints` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820DE2B0` | `PartsPoints::FindRearSpoiler` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820DE378` | `PartsPoints::FindBonnet` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820DE430` | `PartsPoints::FindLeftDoor` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820DE6C8` | `PartsPoints::FindRightDoor` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820DE7E8` | `PartsPoints::FindDoors` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820DEA20` | `PartsPoints::FindTailLights` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820DEAE8` | `PartsPoints::FindFenders` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820DECA0` | `PartsPoints::FindCabin` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820DB778` | `CockpitPoints::FindHeadlights` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820E2E60` | `InteriorPoints::FindMeters` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820E3070` | `InteriorPoints::FindSeats` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820DC468` | `LightPoints::FindGroup` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820DC570` | `LightPoints::Find` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820E5128` | `TrafficPoints::FindBody` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820E5328` | `TrafficPoints::FindHeadlights` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820E53F8` | `TrafficPoints::FindTailLights` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820E00B8` | `SceneParts::Reset` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820DBA90` | `SceneCockpit::Reset` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820E3310` | `SceneInterior::Reset` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820DC6B0` | `SceneLights::Reset` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820E5520` | `SceneTraffic::Reset` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820E8490` | `SceneWheels::Reset` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820D9DD0` | `SceneModelBlock::SceneModelBlock` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820D0030` | `PointRecord::PointRecord` | Overridden | Its stores as a table, its calls in their place. |
| `sub_8257A610` | `Morph::Run` | Overridden | r4's +8 and +12 = r5 and r6, then Morph::Blend. |
| `sub_820CE698` | `PointRecord::SetNode` | Overridden | r4's name to +160. |
| `sub_820CE6C0` | `PointRecord::SetShape` | Overridden | r4's name to +192. |
| `sub_8257A360` | `Model::StreamData` | Overridden | The mesh r4's +388. |
| `sub_8257A378` | `Model::StreamSize` | Overridden | +268. |
| `sub_8257A390` | `Model::StreamData2` | Overridden | +392. |
| `sub_8257A3A8` | `Model::StreamSize2` | Overridden | +284. |

### `native_load_poll`

The records and paints set as loads finish, the car model's load checks.

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_820D8B88` | `Wheels::SetRecord` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820D8B98` | `Wheels::SetCaliperTexture` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820DC7E8` | `SceneLights::SetRecord` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820DC838` | `SceneLights::ApplyRecord` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820E04B0` | `SceneParts::ApplyPaint` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820E3E18` | `SceneInterior::ApplyPaint` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820DBDA8` | `SceneCockpit::ApplyPaint` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820E5DC0` | `SceneTraffic::ApplyPaint` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820D32A0` | `GaugeTableEntry` | Overridden | The gauge r3's entry for level r4. |
| `sub_820D3CE8` | `CarModel::IsReady` | Overridden | IsLoaded for a record of kind 0 or 1, else 0. |

### `native_car_makers`

The garage cars' resets.

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_82110370` | `GarageCar::Reset` | Overridden | Its stores as a table, its calls in their place. |
| `sub_825BF2C8` | `GarageCars::StaticInit` | Overridden | Its stores as a table, its calls in their place. |
| `sub_820D06C0` | `CarRecord::ResetBytes` | Overridden | Its stores as a table, its calls in their place. |

### `native_texture_compression`

The blitter's constructor and the codec's destructor.

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_8235F798` | `Blitter::Blitter` | Overridden | Its stores as a table, its calls in their place. |
| `sub_823D6F20` | `ImageCodec::~ImageCodec` | Overridden | Its stores as a table, its calls in their place. |

### `native_save_interface`

The save interface's queries. The content calls stay recompiled.

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_820949C8` | `App::SaveDevices` | Overridden | [+8]'s slot 10. |
| `sub_820949E0` | `App::Users` | Overridden | [+8]'s slot 9. |
| `sub_82099E00` | `Users::SignedIn` | Overridden | The user r4's controller when signed in, else -1. |
| `sub_820A5970` | `SaveSlot::RequestDevice` | Overridden | The slot's +960 = 1. |
| `sub_820A59A0` | `SaveSlot::FreeBytes` | Overridden | The doubleword at +40. |
| `sub_820A59D0` | `SaveSlot::SaveAt` | Overridden | r5's save. |
| `sub_820A5A28` | `SaveSlot::Pending` | Overridden | The six requests' sum. |
| `sub_820A57A0` | `SaveDevices::SetRootName` | Overridden | r4's name to +16. |
| `sub_82112988` | `PlayTime::Update` | Overridden | sub_82110928 with its frame and r3 + 48. |
| `sub_820A07C0` | `SaveInterface::UserCount` | Overridden | The users' slot 1. |
| `sub_820A0808` | `SaveInterface::UserAt` | Overridden | The users' slot 2. |
| `sub_8228CB68` | `SaveSlots::OfController` | Overridden | The first user the controller is, or the count. |
| `sub_820A5988` | `SaveSlot::Device` | Overridden | +36. |
| `sub_820A59B8` | `SaveSlot::SaveCount` | Overridden | +984. |
| `sub_820A5A10` | `SaveSlot::State` | Overridden | +988. |
| `sub_820A03F0` | `SaveInterface::RequestDevice` | Overridden | The save devices' slot 1. |
| `sub_820A04B0` | `SaveInterface::SaveCount` | Overridden | Slot 4. |
| `sub_820A0508` | `SaveInterface::FreeBytes` | Overridden | Slot 3. |
| `sub_820A0560` | `SaveInterface::Pending` | Overridden | Slot 8. |
| `sub_820A05B8` | `SaveInterface::Device` | Overridden | Slot 2. |
| `sub_820A0610` | `SaveInterface::SaveAt` | Overridden | Slot 5. |

## Render commands

The game records its drawing into command lists, which the render thread
replays through the handler table at `0x8264EB40` (ids 1-35; 0, 36 and 37
do nothing). Every handler is wrapped (`REPLAY_COUNTED_HANDLER` in
`src/native/replay_hooks.cpp`) so the native renderer sees each command
before and after it runs. The names are from what each handler does to
the graphics library's device. Single commands come through a second
table at `0x8264EAA8` with the same ids.

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_82575740` | `Command::SetRenderState` | Wrapped | Command 1. [state, value]: the device's setter for that render state. |
| `sub_825757A0` | `Command::SetSamplerState` | Wrapped | Command 2. [sampler, state, value]. |
| `sub_82575800` | `Command::SetBlendState` | Wrapped | Command 3. [render target, blend control]: the RB_BLENDCONTROL register for that target. |
| `sub_82575850` | `Command::SetTexture` | Wrapped | Command 4. [sampler, texture]: the texture's fetch constant into the shadow. |
| `sub_82574608` | `Command::SetHelperVertices` | Wrapped | Command 5. Inline vertex data for the helper object (24 bytes a vertex). |
| `sub_82574630` | `Command::SetHelperIndices` | Wrapped | Command 6. Inline index data for the helper object (2 or 4 bytes an index). |
| `sub_82575A58` | `Command::SetVertexShader` | Wrapped | Command 8. [shader]. |
| `sub_82575AA8` | `Command::SetPixelShader` | Wrapped | Command 9. [shader]. |
| `sub_82575AF8` | `Command::SetVertexDeclaration` | Wrapped | Command 10. [declaration]. |
| `sub_82575B48` | `Command::SetStreamSource` | Wrapped | Command 11. [stream, buffer, offset, stride]: the buffer's fetch constant. |
| `sub_82575BA0` | `Command::SetIndices` | Wrapped | Command 12. [index buffer]; once per indexed draw (command 18). |
| `sub_82574668` | `Command::HelperSetUp` | Wrapped | Command 13. Ids 13 and 14 (one handler): stores three words for the helper object and calls its slot 2. |
| `sub_82575BF0` | `Command::HelperDraw` | Wrapped | Command 15. The helper object's draw (slot 3) from its inline data. The native renderer draws those vertices. |
| `sub_82575C98` | `Command::HelperDrawIndexed` | Wrapped | Command 16. The same, slot 4 (indexed). |
| `sub_82575D50` | `Command::DrawVertices` | Wrapped | Command 17. [type, start, count]: `DrawVertices` (`sub_825A3B00`). |
| `sub_82575DC8` | `Command::DrawIndexedVertices` | Wrapped | Command 18. [type, base and start, count]: `DrawIndexedVertices` (`sub_825A3ED0`). |
| `sub_82575E48` | `Command::DrawPrimitiveUP` | Wrapped | Command 19. [type, count, stride, data]: `BeginVertices` (`sub_825A31A0`), a copy, `End` (`sub_825A3AE0`). |
| `sub_82575EC0` |  | Wrapped | Command 20. [object], skipped if 0. Not identified. |
| `sub_825746E0` | `Command::Skip` | Wrapped | Command 21. Skips its payload (inline data or padding). |
| `sub_82577018` |  | Wrapped | Command 22. [object, flag]: asks the object (slot 2); not ready with the flag set, runs its slots 1 and 3. Not identified. |
| `sub_82575F10` | `Command::SetRenderTarget` | Wrapped | Command 23. [index, surface]. |
| `sub_82575F60` | `Command::SetDepthStencilSurface` | Wrapped | Command 24. [surface]. |
| `sub_82575FB0` | `Command::Resolve` | Wrapped | Command 25. EDRAM to a texture: flags, rects, destination, clear values. The native renderer handles it from its items. |
| `sub_82576108` | `Command::Clear` | Wrapped | Command 26. [count, rects, flags, color, Z, stencil]. |
| `sub_82576180` | `Command::DrawIndexedPrimitiveUP` | Wrapped | Command 27. Inline indices and vertices: `BeginIndexedVertices` (`sub_825A35F0`), copies, `End`. |
| `sub_825746F0` | `Command::Callback` | Wrapped | Command 28. [object]: its slot 0 with the device. Runs the boot movies' frame (`sub_820AE840`). |
| `sub_82576250` | `Command::RestoreTargets` | Wrapped | Command 29. Sets render targets 0-3 and depth from the saved set, by bit. |
| `sub_82576348` | `Command::SetViewport` | Wrapped | Command 30. [x, y, width, height, min Z, max Z]. |
| `sub_825763C8` | `Command::SetScissorRect` | Wrapped | Command 31. [left, top, right, bottom]. |
| `sub_82576450` | `Command::BeginTiling` | Wrapped | Command 32. Predicated tiling: [flags, tile count, tile rects]. |
| `sub_825764D0` | `Command::EndTiling` | Wrapped | Command 33. [flags, destination]: resolves each tile (`sub_822AC858`, passed on to the native renderer). |
| `sub_82576550` | `Command::SaveTargets` | Wrapped | Command 34. Saves the viewport, render targets 0-3 and depth. |
| `sub_825765B8` | `Command::RestoreSaved` | Wrapped | Command 35. Restores what 34 saved. |
| `sub_82575148` |  | Wrapped | Single command 15 (the helper object's draw) from the second table: marked as command 15 so the native renderer draws its inline vertices. In a race only 5, 7, 13 and 15 come this way (the minimap's player triangle). |
| `sub_825A31A0` | `Device::BeginVertices` | Wrapped | r3 the device, r4 the primitive type, r5 the vertex count, r6 the stride: where in the command buffer the caller copies the vertices. Noted for `Device::EndVertices`. |
| `sub_825A3AE0` | `Device::EndVertices` | Wrapped | After the copy: the native renderer draws the inline vertices (commands 15 and 19; others are logged once). |
| `sub_825A3B00` | `Device::DrawVertices` | Wrapped | Logged once per caller when called outside command 17. |
| `sub_825A35F0` | `Device::BeginIndexedVertices` | Wrapped | Logged once per caller when called outside command 27. |
| `sub_825A3ED0` | `Device::DrawIndexedVertices` | Overridden | r3 the device, r4 the primitive type, r5 the base vertex, r6 the start index, r7 the index count: writes the dirty state's packets and DRAW_INDX (`native::DrawPackets_IndexedDraw`, which runs the original to check itself with `--native_renderer_check`). |
| `sub_822AB028` | `Device::PatchVertexShader` | Wrapped | r3 the shader, r4 the microcode, r5 the declaration: patches the shader for the declaration. Counted; when the GPU may still be using the shader's microcode the library patches a copy in the command buffer, which is kept for the native renderer. |
| `sub_822AC858` | `Device::Resolve` | Wrapped | r3 the device, r4 flags, r5 the source rect, r6 the destination texture, r7 the destination point, r10 the clear color, f1 the clear Z. In command 33 one per tile (the tile count at device +12772), passed to the native renderer; logged outside 25 and 33. |
| `sub_82573538` | `VideoOutput::ResolveFrontBuffer` | Wrapped | r3 the video output: resolves render target 0 into one of its two front buffer textures (+4 x (68 + [+268])) outside any command; the native renderer shows that target. |
| `sub_8229C8D8` |  | Wrapped | Ends the command segment when the buffer is full (and kicks it); counted, so the draw check can leave those calls out. |
| `sub_822AADE8` |  | Wrapped | The draw's float constant packet writer (r3 the device). Checked against the native renderer with `--native_renderer_check`. |
| `sub_822AABF0` |  | Wrapped | The draw's register block writer: r4 the mask, r5 the first register; the blocks are marked dirty for the native renderer, then written. |
| `sub_822AACB0` |  | Wrapped | The draw's fetch constant writer: r4 the mask; marked dirty for the native renderer, then written. |
| `sub_8229BDA8` | `GpuLink::Kick` | Overridden | r3 the device, r4 pairs of (size in words, address), r5 the pair count. No GPU: the buffers' packets are walked and only their effects outside the GPU are made (memory writes, interrupts, fences into the progress block), so the library sees each kick done at once. Logged with `--native_renderer_log_gpu`. |
| `sub_8229C1F8` | `GpuLink::DeferredKick` | Overridden | r3 the device, r5 the address, r6 the size, r7 the interrupts the buffer raises. The library queues a buffer while interrupts are outstanding; with every kick done at once nothing is deferred, so it kicks at once and leaves the count at 0. |
| `sub_822B1888` |  | Wrapped | EndTiling's (`sub_825A47F0`) insert of the worker callback into the command stream. Logged with `--native_renderer_log_gpu`. |
| `sub_822B1748` |  | Wrapped | The callback that wakes CPU 2's library worker (`sub_822B1678`), which submits deferred kicks. Logged. |
| `sub_822B1450` |  | Wrapped | The worker's job. Logged. |
| `sub_8258CB90` | `memcpy` | Wrapped | The library's memcpy (r3 the destination, r4 the source, r5 the size): copies into the physical views are recorded for the native renderer's buffers once done; with `--native_renderer_log_buffers`, copies into a known buffer header are logged. |
| `sub_822B3678` | `D3DXCompileShader` | Overridden | r3 the source, r4 its length, r8 the profile, r10 where the shader goes: a shader compiled ahead of time (`src/graphics/compiled_shaders.h`) when there's one for the source; otherwise the original compiles it (and it's logged). |

## Audio library

The audio library's XMA layer, over our own decoding (`src/hooks/xma.cpp`,
`src/audio/xma_stream.h`): there are no XMA contexts, registers or
hardware. A source voice's XMA object (voice +180) is a handle for an
object of ours; each stream decodes its sound a little ahead of the
reading and answers how many samples are ready, where and how many were
taken. Custom music reads its track there instead (`docs/custom_music.md`).

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_82445678` |  | Overridden | Reads the XMA registers' context array address at boot: nothing to read. |
| `sub_82445690` | `XmaObject::Create` | Overridden | r3 the stream count, r4 their descriptions (12 bytes: sample rate, output ring samples, channels), r5 flags (bit 0: contexts made later), r6 out: the object's handle. |
| `sub_824458C0` | `XmaObject::Destroy` | Overridden | r3 the object. |
| `sub_82445910` | `XmaObject::ReleaseContexts` | Overridden | Only the contexts' made state is kept. |
| `sub_824460E0` | `XmaObject::MakeContexts` | Overridden | The same. |
| `sub_82445988` | `XmaObject::ContextsMade` | Overridden | 1 when made. |
| `sub_82445998` | `XmaObject::SetInputBuffer` | Overridden | (object, stream, data, size in bytes): starts the sound, or queues it as the second input. |
| `sub_82445A70` | `XmaObject::IsPlaying` | Overridden | (object, stream, address): 1 if the sound at that address is still playing or queued. |
| `sub_82445AD0` | `XmaObject::HasFreeInput` | Overridden | (object, stream): 1 unless both input slots are taken. |
| `sub_82445B28` | `XmaObject::IsIdle` | Overridden | (object, stream): 1 if no sound is playing. |
| `sub_82445B90` | `XmaObject::ReadyCount` | Overridden | (object, stream): samples ready, at most the output ring the 360 would have had. |
| `sub_82445DC0` | `XmaObject::ReadPosition` | Overridden | (object, stream, out): where to read (a staging buffer in guest memory) and how many samples there are readable in one go. |
| `sub_82445C08` | `XmaObject::Take` | Overridden | (object, stream, samples, out): takes up to that many; where they were read from; returns how many. |
| `sub_82445FF8` | `XmaObject::TakenCount` | Overridden | (object, stream): samples taken so far. |
| `sub_82445E68` | `XmaObject::ResetStream` | Overridden | (object, stream): the sound stopped. |
| `sub_82445F30` | `XmaObject::ErrorStatus` | Overridden | (object, stream): none. |
| `sub_82445F80` | `XmaObject::SetLoopData` | Overridden | (object, stream, the loop data: start and end bits, subframes, count). A custom track takes only the loop count. |
| `sub_82446088` | `XmaObject::SetStart` | Overridden | (object, stream, the start's bit offset, subframes to skip). Not for a custom track. |
| `sub_82446018` | `XmaObject::InputPosition` | Overridden | (object, stream, out): the input buffer, the read offset and a word; zeros (logged once). Called by the voice's flush (`sub_824539F0`). |
| `sub_824461B0` | `XmaObject::Lock` | Overridden | Nothing to wait for: 0. |
| `sub_82446310` | `XmaObject::IsLocked` | Overridden | 1. The hardware wait polls it. |
| `sub_82446248` | `XmaObject::WaitForHardware` | Overridden | r3 the object: nothing to wait for, 0. |
| `sub_82446490` | `XmaObject::Unlock` | Overridden | Decodes ahead and moves finished streams on. |
| `sub_825613F8` | `Voice::Worker` | Overridden | The voice chat library's worker (r3 the engine): signals its started event (engine +1304) and returns without running; in play it found no talkers and fed nothing. The engine itself is still made by `Voice::Init`. |

## Movies

The game's movie class on our own FFmpeg player (`src/hooks/movies.cpp`,
`src/movies/movie.h`), behind `--play_movies_with_ffmpeg` (on); with it
off, the recompiled functions run. `Movie::RenderFrame` (`sub_820AE840`)
is in the first table.

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_820AEAF8` | `Movie::Play` | Overridden | r3 the movie, r4 the file. |
| `sub_820AE3C8` | `Movie::Start` | Overridden | r3 the movie. |
| `sub_820AE710` | `Movie::Stop` | Overridden | r3 the movie. |
| `sub_820AE780` | `Movie::Pause` | Overridden | r3 the movie. |
| `sub_820AE7E0` | `Movie::Resume` | Overridden | r3 the movie. |
| `sub_820AE968` | `Movie::GetState` | Overridden | r3 the movie: its state (the only one whose r3 the callers read). |
| `sub_820AE410` | `Movie::Release` | Overridden | r3 the movie. |

## Debugging: per-frame state

Parts of the draw step wrapped for `--find_per_frame_state`
(`src/debug/frame_state_diff.cpp`), which finds game state that advances on
subtick's render-only passes: each is diffed before and after it runs. Off,
they only call the original. `sub_820F5F48` and `TickHandler::Invoke`
(`sub_8209A1C0`) are in the first table.

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_820A0B40` |  | Wrapped | Per camera pass, in a loop over the passes. |
| `sub_820AA160` |  | Wrapped | A part of the draw step. |
| `sub_8209F998` |  | Wrapped | A part of the draw step. |
| `sub_8209F9F8` |  | Wrapped | A part of the draw step. |
| `sub_820C93A8` |  | Wrapped | A part of the draw step. |
| `sub_820C5BA0` |  | Wrapped | Draws the cars. |
| `sub_82132438` |  | Wrapped | A part of the draw step. |
| `sub_821325C0` |  | Wrapped | A part of the draw step. |
| `sub_8212F5D8` |  | Wrapped | A part of the draw step. |

## Known data

| Address | What | Notes |
|---|---|---|
| vehicle+0x30 | Speed (float, m/s) | Read by `Vehicle::GetSpeedKmh`. |
| vehicle+0xD0 | Vehicle world matrix | 4x4 row-major: rotation rows, then position row (w = 1). The car's forward axis is -row 0. Not yet checked in SBX. |
| vehicle+0xCF0 | Copy of the world matrix | Not yet checked in SBX. |
| `0x826AA8F0` | Camera view matrix | Pass 0's view (+0x10). The camera position is -t·Rᵀ. Draw rebuilds it from the eye and target. |
| `0x826AA8E0` | Camera pass array | 21 objects of 592 bytes: view +0x10, projection +0x50, view-projection +0x90, eye +0x190, look-at target +0x1A0, up +0x1B0. Pass settings at +564 to +588, active flag at +588. Pass 0 is the main 1280x720 view; pass 10 a 1024x1024 reflection and 11 a 512x512 shadow map. |
| `[0x8280DE44]` | Title screen object | +4 state (5 = start the attract race), +36 idle ticks. |
| `0x826C7EC4` | Post-race check counter | Counted up by `sub_8218C240` (`PostRace::Check`). |
| `0x826B5784`, `0x826B5788` | Minimap blink clock | Stepped by `Minimap::Draw`. |
| `0x826B5870` | Play time frame counter | Counted by `sub_82107A38`; `sub_82110928` turns it into hours, minutes and seconds. |
| `0x826B588C` | Boot movie id table | Index at `0x826B58A8`; skippable flag at `0x826B58A4`. |
| `0x826B51FC`, `0x826B51F4` | The course scenery manager (248 bytes) | Both point to it: set by `Scenery::Create` and `Scenery::Init`. +8 the configuration (+8 the mode, +32 the header, +36 the course record), +140 and +168 the key tables, +188 the area models' buffer, +216 to +224 the visible areas (`Lights::FindNearest`'s list). `docs/game_data.md`, "Courses". |
| `0x825F2E40` | The course table | 24 entries of 8 bytes by course index: the header and the course record (`docs/game_data.md`, "Courses"). |
| `0x82626478` | The front end's scenes | 16 bytes an entry: three `BUILD` ids, then the scene number (0-5). |
| `0x826B57FC`, `0x826B57F0` | The course's collision mesh and route | Loaded by `TrackCollision::Load`. |
| `[0x826B580C]` | The race data | +4 the race mode (8-10 use course place 1), +24 the route bits, +32 the course variant. |
| `0x820029C4`, `0x820029C0` | 1/30 and 1/60 | Float step constants (see "Fixed time step"). |
| `0x825F17E8` | The cars' step length | Float, 1/60 (`.data`). Passed to `Vehicle::Step` twice a tick by `Vehicle::Tick`, and read by `Vehicle::ApplyControls` and a leaf at `0x820BFF54`. Not changed by the subtick code. |
| `0x826B5810` | Control record table | One pointer per car index to its control record (5 words; see `Vehicle::ApplyControls`). |
| `0x826B5838` | Input tables | `[+0]` + pad x 120 + 0x1F7C0: the abstract input ids `PlayerControls::Build` queries. |
| `0x820029B8` | Float | `Vehicle::Tick` stores speed times it at car+6160. |
| car+0x930, +0x934 | Step length and its inverse | Written by `Vehicle::SetStepLength` each step. |
| car+0x940 | `Vehicle::Step` inner iterations | 10, set in `sub_820B0DD0`. |
| car+0x958 | `sub_820B1678`'s ramp counter | 0 to 30. |
| `0x82000A20` | 1.0f | w of the light positions `Lights::FindNearest` writes. |
| `0x825F0A84`, `0x825F0A88` | `BUILD.DAT`, `BUILD_CRS.DAT` file handles | Opened by `Archive::Init`. |
| `0x825F0A90`, `0x825F0AA0` | Built-in copies of the `BUILD.TOC` and `BUILD_CRS.TOC` headers | Count (23009, 1359), two unknown words, `.DAT` size. Only the counts are read; adding archive entries needs these patched (`docs/game_data.md`). |
| `0x825F0AB0` | Loose file table | Pairs of file id and path, ended by a null path. |
| `0x82694E00`, `0x82694E08` | `BUILD` and `BUILD_CRS` TOC buffers | Sizes at `0x82694E04`, `0x82694E0C`. |
| `0x82694E14`, `0x82694E10` | The shared load buffer and its size | 0xC80000 bytes, allocated by `Archive::Init` (`docs/game_data.md`). |
| `[0x826B2B24]+5008`, `+5012` | The shared load buffer, as the car code reads it | Set by `sub_820C8B88`. |
| `0x825F2520` | Vinyl sheet size | 1024, 1024, then 16, 16 (the cleared corner). |
| `0x825F2A00` | Vinyl sheet regions | 10 rectangles (x0, y0, x1, y1 as u32), indexed by `Vinyl::DrawDecal`'s r3. |
| `0x82069A94` | `XPRList` vtable | |
| car model+32 | The car's block index | Times the set's stride gives the car's ids. |
| car model+208, +592 | Vinyl layer and sticker records | 64 bytes each: 6 layers; 8 groups of 6 stickers, 384 bytes apart. |
| car model+4092, +4380 | Vinyl and sticker load slots | 48 bytes apart. |
| car model+6684, +6688 | Vinyl sheet texture and pixels | The A8R8G8B8 canvas, then the DXT5 texture. |
| model+48, +80 | The model's XPR list | +80 the `XPRList`, +48 a pointer to it; index 0 is the model's own XPR2. |
| model+444, +448 | Vinyl sheet textures | Set by `Model::SetVinylTextures`. |
| car+16 | Car's world matrix, for drawing | Copied by `Car::TickUpdate` each tick. |
| car+696, +700, +704 | Speed (km/h), revs, boost | Set each tick by `Car::TickUpdate`; read by the dash dials. `Car::DrawUpdate` takes 1.0 off +704 every pass. |
| view+8080, view+8144 | This tick's and the previous tick's view-projection | Copied into vertex c100 by `View::UploadPreviousViewProjection` (the motion blur). |
| `0x820069B0` | Music bank names | 18 strings, 12 bytes apart (`BGM_SYSTEM`, `bgm_tmp_17` down to 01 without 10, then 10), read by `Bgm::Load`. |
| `[0x826B5960]` | The sound object | `Bgm::Load` calls its vtable slot 4 to load a bank pair. |
| `0x82609334` | Music table | 13 name pointers: `bgm_tmp_06`, 07, 01, 15, 05, 10, 03, 09, 17, 11, 16, 14, 08. Read by `Music::PlayRoaming`, `Music::PlayRequested`, `sub_82174AD0`. |
| scene+9644, +9652 | Music request bits, music mode | The scene the music handlers run on (`sub_82175238`): +9644 the bits `Music::RequestBattle` sets and `Music::PlayRequested` plays; +9652 the mode `Music::PlayRoaming` reads. |
| `[0x826B2B28]` | Parameter tables | +8 BUILD 73 (wheels), +12 BUILD 72 (car specs), +16 to +48 BUILD 74-82 (aero parts), +56 the car count. Built by `sub_820C9700`, loaded by `sub_820CC048`. |
| car+1644 | A car object's tuning | +8 engine stage, +16 to +60 the power and drive-train part levels, +64 to +92 tires and wheels, +96 to +136 the aero part levels (`docs/game_data.md`, "How the game uses the car specs"). |

## Known issues

- None recorded yet.
