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
| `sub_82091FB0` | `Renderer::DrawFrame` |  | Render thread: calls each draw listener in the list at renderer+28. |
| `sub_82091CC0` |  |  | Draw listener between the scene draw and present: resolves the frame into a front buffer (`sub_82573538`) and waits for the GPU to go idle (`sub_8229CB08`). |
| `sub_82098DF0` | `Scene::Update` |  | Tick handler on the scene object (0x40026FE0 at the menu). Calls the current mode object at +64 (vtable slots 6 and 2). |
| `sub_82098E68` | `Scene::Draw` |  | Draw listener on the same object. Calls the mode object's draw (vtable slot 5), reading live game state. |
| `sub_8216F0F8` | `Delegate::Invoke` |  | Calls the function at +8 with the argument at +4. Used for tick and draw listeners. |
| `sub_8222E468` | `MemberDelegate::Invoke` |  | Calls the member function at +8 on the object at +4, adjusted by +12. |
| `sub_82573100` |  | Wrapped | Receives the frame interval from `sub_82093280` in r6 (2) and stores it at +0x130 (0 and 1 store 1, 2 stores 2, 4 stores 3, 0x80000000 stores 0); `VideoOutput::WaitForVblank` waits that many vblanks before each present. 1 with `--tick_rate_60` (`src/tick_rate/tick_rate.cpp`, see "Tick rate experiments"). |
| `sub_82573638` | `VideoOutput::WaitForVblank` | Overridden | r3 the video output: calls the device (`sub_8229DD58` with [+140]), then waits for the vblank interval at +0x130 (+304). With `--subtick` only the device call runs; the pacer holds the rate (`src/subtick/renderer_hooks.cpp`). |
| `sub_82573920` | `Event::Wait` |  | r3 = event, r4 = pointer to timeout (-1 = forever). |
| `sub_82573908` | `Event::Signal` |  | r3 = event. |
| `sub_822AE7E8` |  |  | Graphics driver present bookkeeping (frame counter at +20024). Library code, not game timing. |
| `sub_8259F788` | `QueryPerformanceCounter` |  | System library. |
| `sub_820BFF60` | `Vehicle::GetSpeedKmh` |  | Returns the float at vehicle+48 (m/s) times 3.6. |
| `sub_820943D8` |  |  | First tick handler: polls input, then walks listener lists that run both game logic and the draw step. |
| `sub_82106478` | `GameMessages::Dispatch` | Overridden | Delivers the game's queued messages to their handlers: 0 = Update (all game logic), 1 = Draw. r3 = the dispatcher: handlers +0 (array), handler count +200, queue of 50 12-byte messages at +204, queued count +804, read index +808. The game has several dispatchers (40 call sites). Replaced with the same steps (`src/subtick/game_messages.cpp`) with `--subtick` (Update skipped on render-only passes); otherwise the original. |
| `sub_82106410` |  |  | Picks the handler object a message goes to; its method 0 handles it. |
| `sub_82099700` | `Input::Update` | Wrapped | r3 the input manager: physical pads (+988, 4 x 212, `PhysicalPad::Read`), logical pads (+8, 5 x 196, `LogicalPad::Build`), processors (+1840, 5 x 416, vtable slot 1), listener slots (+3924, 5 x 12; the first node's object, vtable slot 1). Skipped on render-only passes (`src/subtick/input.cpp`). |
| `sub_82159540` | `Race::DrawStep` | Wrapped | The race's draw step (Draw message). Writes a field at +64 of the object its calls return. Noted to tell races from menus (`src/subtick/game_messages.cpp`). |
| `sub_820A3B98` | `CameraPass::Update` | Wrapped | Scales the main camera's projection after the update (`--camera_fov`, `CarTransforms_CameraFov`), then rebuilds the view-projection. Rebuilds a camera pass's view (+0x10) from its eye, target and up vector (+0x190, +0x1A0, +0x1B0), and its view-projection (+0x90 = view * projection at +0x50). Called several times per Draw for pass 0. On pass 0 in a race, built from the camera blended between ticks; also run on pass 0 before `Race::DrawStep` on render-only passes, so the cars are drawn with the frame's view-projection (`src/subtick/car_transforms.cpp`, see "Subtick"). |
| `sub_8209BE70` | `Matrix::Multiply` |  | out (r3) = a (r4) * b (r5), 4x4. |
| `sub_8257ACD0` | `Recorder::RecordObject` |  | The bulk per-object recorder, ~150 calls per pass in a race: records command 7 with vertex constants 0-8: world (r5) * camera view-projection (r4) in c0-c3, the world matrix in c4-c7 (for some objects a projective matrix instead), and the camera's world position in c8. The camera is often a stack copy, not a camera pass. |
| `sub_8257AAE0` |  |  | Multiplies a world matrix and camera into a stack temporary; not recorded (likely a visibility test). |
| `sub_825758A0` | `Command::SetShaderConstants` |  | Render thread, command 7: r3 = payload, r4 = register count, r5 = stage << 24 \| type << 16 \| start << 8 \| count. |
| `sub_82572638` | `CommandList::AllocateSlots` |  | r3 = list, r4 = slot count; returns the first 16-byte slot. |
| `sub_820BFF88` | `Vehicle::GetDistancePerFrame` |  | Speed (m/s) times a hard-coded 1/30 s. |
| `sub_820C2E40` | `Vehicle::Tick` | Original | Once a tick per car: r3 = car, r4 = its index in the control record table at `0x826B5810`, r5 and r6 for `sub_820C2208`. Stores speed x `[0x820029B8]` at car+6160, copies car+5236 (4 bytes) to car+6164, then twice: `Vehicle::ApplyControls`, clears the record's one-shot bits (`0x00010003`), `Vehicle::Step` with `[0x825F17E8]` (1/60), `sub_820C1B98`. Then `sub_820C2208` once. Called by `sub_82163CC8`, `sub_82164188`, `sub_82171950` (race modes) and `sub_8216F398` (every car). See "Vehicle stepping". The game's own (the weighted input is written at `PlayerControls::Build` instead). |
| `sub_820B6330` | `Vehicle::Step` | Original | r3 = car, f1 = step length. `Vehicle::SetStepLength`, then the car's physics parts (car+0x930 read by 9 of them), with an inner loop of car+0x940 (10) iterations (`sub_820B0600`, `sub_820B0720`, `sub_820B3E18`, `sub_820B07F0`, `sub_820B25E0`, `sub_820B4610`). Called by `Vehicle::Tick` and, once each with 1/60 from `0x82609328`, by `sub_8216E590` and `sub_821703B8` (placing cars). |
| `sub_820AF9D0` | `Vehicle::SetStepLength` |  | Stores f1 at car+0x930 and 1/f1 at car+0x934. Nothing under `Vehicle::Step` reads +0x934 or the 60.0 `sub_820B0DD0` stores at +0x938. |
| `sub_820BFB68` | `Vehicle::ApplyControls` | Original | r3 = car, r4 = its control record: steering +8, throttle +4, brake +6, +10, flags +0 (bits pick digital or analog handling of each), bytes +14..+17 (+17 the control type). Rates are scaled by `[0x825F17E8]`. Bits 0x1 and 0x2 (one-shot) and 0x2000 only set request bytes (car+2284, +2285, +2286). |
| `sub_820B1678` |  | Original | Part of `Vehicle::Step` (a leaf): counts car+0x958 up by 1 per step while a slip test holds (to 30, the 30.0 at `0x82000D54`, which is also the test's threshold), down otherwise, and scales car+284 by 1 - counter x 1/30 (car+284 is worked out again later in the step by `sub_820B1EB0`). Counts steps, so it's only right at the game's two steps a tick; the car physics doesn't change its step count. |
| `sub_820C1B98` |  |  | Each step: updates five tracked positions (the car, `sub_82249148` of car+336, into car+5232+16; then 4 wheels, 368 bytes apart) through `sub_820F8350`. |
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
| `sub_820F1038` | `TextureAnimations::Advance` | Wrapped | From the draw step part `sub_820F5F48`: advances the flipbook animation set at r3+1060 (`sub_821453B0`). Skipped on render-only passes (`src/subtick/per_frame_logic.cpp`). |
| `sub_821453B0` |  |  | Calls `sub_82145350` on every animation in a set: count at +0, then offsets to each animation. |
| `sub_82145350` |  |  | Advances one flipbook animation: frame count +0, current frame +4, tick counter +8 (wraps at the length at +12), frame start ticks from +16. |
| `sub_82145410` |  |  | Returns the current frame's entry of animation r4 in set r3 (texture id at +4). |
| `sub_821452C8` |  |  | Finds the animation in set r3 whose texture is r4; -1 if none. |
| `sub_82146F00` |  |  | Draws the light groups: for each group, builds a light list on the stack (count +32, 48-byte lights at +36, from static level data) and picks its texture's current animation frame. Calls `sub_8209CED0` → `sub_8209CD58`. |
| `sub_820F5610` | `Minimap::Draw` | Wrapped | From the draw step part `sub_820F5F48`; steps the minimap blink clock at `0x826B5784`/`0x826B5788` after drawing. Both put back on render-only passes (`src/subtick/per_frame_logic.cpp`). |
| `sub_82107A38` |  | Wrapped | From Draw, every pass; adds one to the play time frame counter at `0x826B5870` and runs `ActionMappers::Update` (call at `0x82107B48`). The one is taken back on render-only passes, and every other pass with `--tick_rate_60` (`src/subtick/per_frame_logic.cpp`). |
| `sub_82110928` |  |  | Turns the play time frame counter into hours, minutes and seconds (30, 1800, 108000 frames). |
| `sub_82107CB8` | `Boot::StartMovie` |  | Message handler: plays the next boot movie, id from the table at `0x826B588C` (index at `0x826B58A8`), via `sub_820A45D0` (name table at `0x8233AF8C`). |
| `sub_82107D80` | `Boot::WaitForMovie` |  | Message handler, every tick while a boot movie plays; ends the step on a button press if skippable (`0x826B58A4`) or when the movie has finished. |
| `sub_82107D20` | `Boot::EndMovie` |  | Message handler: stops the movie; the front end (Genki logo, title) follows. |
| `sub_820A4678` | `Movie::IsFinished` |  | 1 when the playing movie has finished. |
| `sub_82292F48` | `CreateFile` |  | The only caller of `NtCreateFile`; r3 = path. |
| `sub_8221FC48` | `TitleScreen::Update` |  | Counts idle ticks on the title screen and starts the attract race at 600. |
| `sub_820F3FF8` | `Lights::FindNearest` |  | Collects the lights nearest a point, nearest first. Arguments: r3 the light set owner (list at r3+216: count +0, array +8 of 272-byte entries, each's +12 → +80 a light set), r4 the output, r5 the point, r6 the most lights to keep (at most 32; 0 returned above), r9 a direction vector, f1 a merge radius, f2 the farthest squared distance, f3 the least dot product. For each light (48 bytes: position xyz at +0, 32 bytes copied out): skipped beyond f2, or when the direction to it dotted with r9 is below f3 (a cone); within f1 of a light already kept, it replaces that one if nearer, else is dropped; otherwise insertion-sorted by squared distance into a 33-entry array on its ~2 KB guest stack frame. Writes 32 bytes a light to r4 (position with w = the float at `0x82000A20`, then the light's +16..+31) and returns the count. A light set's count (+0) is little-endian (byte-swapped after `lwz`); `sub_82292170` gives each of its entries, whose +0 count says how many times the entry's one light group (count +60, lights from +76) is scanned: the group pointer isn't advanced, so a count above 1 scans the same lights again (the merge then drops the copies). Only caller: `sub_820C8F10`. |
| `sub_820C8F10` |  |  | Asks `sub_820F3FF8` for up to 10 lights near a point (r1+112) in a cone (cosine-like value from `sub_8258E218` of a setting at `[r27+11844]+5500` times a constant) of the light set at `[0x826B51FC]`. Called from `0x820E19B8`, `0x820E66F0` and `0x820DC028`. |
| `sub_82292170` | `DataFile::GetEntry` |  | Reads a container (`docs/game_data.md`): r3 the container, r4 an index; returns r3 plus the r4-th of the little-endian offsets at r3+4 (the count at +0 bounds r4; 0 beyond it); with r5's low byte set, 0 for an entry whose first word is 0. Light sets are containers; `sub_820F3FF8` passes r5 = 0. |
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
| `sub_8209AD48` |  |  | Read error: ends in `sub_82292D50`, which shows the dirty-disc screen (`XamShowDirtyDiscErrorUI`). |
| `sub_82572370` |  |  | Reads a whole opened file into a buffer (r3 the file, r4 the buffer, r5 a pointer to the size); used for the TOCs and `DEBFONT.XPR`. |
| `sub_820CC048` |  |  | Loads the parameter tables, BUILD 72-83 (72 = car specs) through `Archive::Load`; `sub_820C9700` sizes them first. |
| `sub_821744D8` |  |  | Loads BUILD 22887, the loose `DATA\AUDIO\CAR_SE.XWB`. |
| `sub_82094BB8` |  |  | Startup; names the XACT engine `txr_xact` and calls `sub_821066D0` (which calls `Archive::Init`). |
| `sub_8228CEE8` |  |  | One of four functions (with `sub_8228CFA8`, `sub_8228D228`, `sub_8228D370`) that open the save, `TXR360:\TXR_DATA_1`. |
| `sub_82105790` | `GameData::Get` |  | r3 an index: returns the pointer at `0x826B5838` + 4 x r3. Index 0 is the save (`docs/game_data.md`). |
| `sub_82113D28` | `SaveData::Clear` |  | r3 the save: zeroes it and sets its defaults. |
| `sub_82113FB0` | `SaveData::NewGame` | Wrapped | r3 the save: clears it (`SaveData::Clear`) and sets a new game's state, including the car unlock bits at +0x1985C (every car but CON_X, ECLI, S30, L86, T86 and BOSS00). Run by the title menu's new game and at boot on the empty saves. Sets the cut cars' bits after it (`src/hooks/unlock_cut_cars.cpp`, `--unlock_cut_cars`). |
| `sub_82114278` | `SaveData::NewGameKeepCars` | Wrapped | r3 the save: a new game that keeps the garage (+6120), the word at +6116 and the unlock bits at +0x1985C (copies them out, runs `SaveData::NewGame`, copies them back). From the title menu (`sub_8221F568`). Sets the cut cars' bits after it (`src/hooks/unlock_cut_cars.cpp`). |
| `sub_82114D88` | `SaveData::Unlock` |  | r3 the save, r5 what to unlock (a switch of 42 cases), r6 its id. One case sets car r6's bit at +0x1985C if it isn't set yet; new unlocks are queued for the garage notifications. |
| `sub_8217F2A0`, `sub_82272370`, `sub_8224A0E8` |  |  | Build the car lists (the starter cars and the shop): for each of the 72 cars with its bit set at save +0x1985C, copy its specs (`sub_820C9C38`) into list 0 or 1 by spec byte +771 (0 for the road cars, 1 for CON_X and the rival cars). |
| `sub_820C9C38` | `Params::GetCarSpec` |  | r3 a car id: returns its 8,432-byte record in the car specs (BUILD 72). |
| `sub_820C9C50` | `Params::GetWheel` |  | r3 a wheel number: returns record r3 - 1 of BUILD 73 (the wheels), or 0 for 0 (the stock wheel). |
| `sub_820C9BA0` | `Params::GetCarParts` |  | r3 the output (9 pointers), r4 a road car id: points into BUILD 74-82, the aero parts (strides 280, 280, 280, 280, 168, 168, 84, 108, 72). |
| `sub_820CBFE8` |  |  | Load callback for BUILD 72: stores the car count (size / 8432) at params+56 and byte-swaps the records (`sub_820CBF90` → `sub_820CBF08`, one swap per block: `sub_820CA2A0` block 1, `sub_820CBC10` block 2, `sub_820CA780` engine stage, `sub_820CAA48` tire, `sub_820CB1F8` paint, `sub_820CB328` wheel). |
| `sub_820CB1F8` |  |  | Swaps a car's paint block, and fills in each slot's second shade (+12) for solid and metallic paints (`docs/game_data.md`). |
| `sub_820C96C8` | `CarSpec::GetTire` |  | r3 the spec, r4 the tire type (0-4), r5 the grade (1-5), r6 1 for the rear: returns tire 0 for type 0, else (type - 1) x 5 + grade. |
| `sub_821ED5D8` | `WheelShop::MaxInchUp` | Wrapped | r3 the tuning menu: how many inch-ups the wheel shop offers for the garage car it shows, by car id: 20 minus the stock rim diameter, at most 4, on the road cars (RX8 2, RX7 3, N180 4); 0 for CON_X, the cut cars and any id over 22. The cut cars get the same rule (ECLI 3, L86/S30/T86 4) with `--cut_car_wheels` (`src/hooks/cut_car_wheels.cpp`). |
| `sub_820C44C8` | `Tire::InchUp` |  | r3 a tire (a copy), r4 inches: bigger rims with the same outer diameter; recomputes width, aspect ratio, rim width and grip (`docs/game_data.md`, "Tires"). |
| `sub_820C4698` | `Car::CopySpec` |  | r3 a car object, r4 its spec: copies block 1 to car+2240, block 2 to +1888, the engine stage at level [car+1652] to +2880, paint to +3364, and the front and rear tire (with inch-up) to +3084 and +3176. |
| `sub_820C47A8` | `Car::Refresh` |  | r3 a car object (car id at +12): `Car::CopySpec`, then the aero parts (`Car::ApplyAero`) and the front and rear wheels (+3268, +3316). 50+ callers (garage, shop, race setup). |
| `sub_820C4378` | `Car::ApplyAero` |  | r3 a car object, r4 its `Params::GetCarParts` pointers: copies the aero part records at the levels in car+1740 to +1780 to car+3636 to +3875, through one getter per part (`sub_820CB5E8` ... `sub_820CBA30`; level 0 = no part). |
| `sub_820CB468` |  |  | Byte-swaps a 28-byte part record after it's copied. |
| `sub_820C4360` | `Car::GetSpecSheet` |  | r3 a car object: returns car+1888 (its copy of block 2), or 0. |
| `sub_820C2608` | `Vehicle::SetupFromCar` |  | r3 a vehicle, r4 a car object: copies the physics values from block 1 (dimensions, mass, wheelbase, tracks ...), the engine stage (rev limit, torque, +156-196) and the tires into the vehicle, then `Vehicle::ApplyTuning`. |
| `sub_820C0B48` | `Vehicle::ApplyTuning` |  | r3 a vehicle, r4 a car object: applies the tuning at car+1644 using tables of multipliers built in the code (7 entries per part, separate NA and turbo tables), the engine type as a cylinder count, and the aero parts. |
| `sub_822B8A90` |  |  | Returns [r3+8] (with r3 = car+1644: the engine stage level). |
| `sub_820A0A88` | `Shaders::LoadAll` |  | At boot (from `sub_821067A0`): loads every shader pair through `sub_820A09D8`: BUILD 3-70 as indices 0-33, BUILD 22800-22811 as 1000-1005, and BUILD_CRS ids 24256-24367 as 2000-2055. A pair is a vertex shader (the odd id) and its pixel shader (the next id). See "Shaders" in `docs/game_data.md`. |
| `sub_820A09D8` |  |  | r3 the vertex shader's file id, r4 the pair's index: loads the vertex shader and the pixel shader (r3 + 1) with `Archive::Load`. |
| `sub_821364B0` | `Glow::Record` |  | The glow (bloom) recorder in races: builds the cross-fade quad (pixel shader `E0D070FB58A4EA03`, the vertex color's alpha is the history weight at +8460) and the glow constants c33 (+8436/+8440, +8452 the glow strength, +8456, +8476). |
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
| `sub_820E3EB0` | `Dash::PoseTachometer` |  | r3 the dash, r4 the car, r5 the camera passes: poses the tachometer needle from `Dial::TachometerAngles`. |
| `sub_820E4178` | `Dash::PoseDials` |  | r3 the dash, r4 the car, r5 the camera passes: poses up to seven needles, each from `Dial::Angle`. |
| `sub_820D32D0` | `Dial::Angle` |  | A needle's angle from a gauge value. |
| `sub_820D3388` | `Dial::TachometerAngles` |  | The tachometer's two angles from the revs. |
| `sub_8211D4B8` | `UiMovie::Update` | Wrapped | r3 the movie, r4 0 = advance a frame and draw, 1 = draw only. Runs the HUD, the race start countdown and the pause menus. Advancing is replayed from the logic pass on render-only passes (`src/subtick/per_frame_logic.cpp`). |
| `sub_82585FD8` | `UiMovie::ApplyAnimation` |  | Applies each playing animation's tracks with t = (time - k0) / (k1 - k0), clamped. |
| `sub_82585C40` | `UiMovie::ApplyTrack` |  | r3 the node, r4 the track (+0 its property id), f1 t: writes one word into the node. Some tracks add to the value instead of setting it. |
| `sub_82585560` | `UiMovie::TrackProperty` |  | r3 a property id: its 12-byte descriptor (+0 node offset, +4 type). |
| `sub_82582D50` | `UiMovie::Child` |  | r3 the node, r4 an index: that child (count at node+112). |
| `sub_82119D50` | `UiMovie::RestorePose` |  | Copies a movie's rest pose back onto its live nodes. |
| `sub_8210B140` | `ActionMappers::Update` | Wrapped | r3 the mapper list: updates every action mapper (`ActionMapper::Update` each): held, pressed and fired words, and the menu repeat. Counts calls, so it must run once a tick: skipped on render-only passes, except when `src/subtick/input.cpp` runs it for the analog values. |
| `sub_8210C4A8` | `ActionMapper::Update` | Wrapped | r3 the mapper: raw axes from the logical pad (`[+2880]`) into +388..+446, then `ActionMapper::UpdateActions`. Digital state put back after it when run for the analog values (`src/subtick/input.cpp`). |
| `sub_82209F78` | `MenuList::Update` |  | A menu list's update. |
| `sub_8209A1C0` | `TickHandler::Invoke` |  | r3 the app, r4 the handler (target at +8). |
| `sub_820DC5C8` | `BlinkTimers::Advance` | Wrapped | Steps the blink timers; once a tick: skipped on render-only passes (`src/subtick/per_frame_logic.cpp`). |
| `sub_8215BE00` | `RaceOverlays::Run` | Overridden | The race overlays (25 objects at r3, e.g. the marker above opponents, the light trails): update (vtable slot 4) and record (slot 5). With `--subtick`, the update here on logic passes, the recording moved to just before `Race::DrawStep` on every pass (`src/subtick/per_frame_logic.cpp`, "Race overlays"); otherwise the original. |
| `sub_8215BE80` | `RaceOverlays::Record` | Overridden | Records the 25 overlays (slot 5) without updating them; from the paused handler `sub_8215C0A8`. With `--subtick` in a race, on logic passes it only saves the list, and the recording happens before `Race::DrawStep` on every pass, as for `RaceOverlays::Run`. |
| `sub_821281B0` | | Wrapped | World-space sprite list recorder (r3 = list, r4 = camera pass, its +144 used): the light trails. Corners blended between ticks. ITC's `sub_82128750`. |
| `sub_82127C08` | | Wrapped | Single world-space sprite recorder (r3 = sprite, r4 = camera pass), reached through `sub_82128CF8`. Corners blended between ticks, except for the marker (blended by its centre in `sub_8212BDA0`). ITC's `sub_821281A8`. |
| `sub_8212BDA0` | | Wrapped | The opponent marker's overlay record (vtable slot 5; r3 = overlay, r4 = camera pass index; ITC's `sub_8212CE40`): builds the sprite on the stack from its centre (overlay +176..+184, set once a tick) and axes facing the camera pass's eye. With `--subtick` the centre is blended between ticks for the call. |
| `sub_82102B30`, `sub_82102BB0` | | | Replay: store the player's control record (r3 = record, r4 = pad), and replace it from the replay while one plays; from `sub_8215E390` right after `PlayerControls::Build`. |
| `sub_821573E0` |  |  | Probably the rival intro cutscene's Update handler: 107 of its 126 expected instructions line up, but it's 210 long and makes calls the expected one doesn't. Not checked. |
| `sub_8220F7B0` |  |  | Draws the rival's logo. |
| `sub_8246BD88` | `MoviePlayer::OpenFile` |  | The movie player's file open (through a vtable); calls `CreateFile`. |
| `sub_820AE840` |  |  | The render thread's command that starts the boot movie. |
| `sub_82106540` | `Bgm::Load` |  | r4 a bank name: looks it up in the table at `0x820069B0` (`bgm_tmp_01`-`17`, `BGM_SYSTEM`) and loads its sound bank (BUILD 22970 + i) and wave bank (22988 + i), or `SB_SYSTEM_SE` (22885, 22886) for any other name, through the sound object at `[0x826B5960]` (vtable slot 4). See `docs/audio.md`. |
| `sub_82118818` |  |  | Builds `data\audio\%s.xgs` and reads the XACT global settings (`TXR_XACT.XGS`). |
| `sub_8244D0A0` | `AudioRenderDriver::Register` |  | r3 the driver object (callback argument +12, handle +16), r4 the mixer callback: `XAudioRegisterRenderDriverClient`. Reached only through a vtable. |
| `sub_8244D128` | `AudioRenderDriver::Submit` |  | r4 the mixed buffer (descriptor from `sub_82448738`): `XAudioSubmitRenderDriverFrame`. |
| `sub_8244D188` | `AudioRenderDriver::Destroy` |  | Unregisters the client (`XAudioUnregisterRenderDriverClient`). |
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
| `sub_8257A620` | `Mesh::LoadShaders` |  | Loads the mesh file's vertex and pixel shaders (`XPhysicalAlloc Error Vertex Shader` / `Pixel  Shader`). |
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
| `sub_8218C240` | `PostRace::Check` | Overridden | Post-race check: once loaded (+9608), waits for a counter (global at `0x826C7EC4`) to reach 60 before setting the next state at +9604. The 60 is 1 here (`src/hooks/post_race.cpp`, `--post_race_check_fix`): with 60, the screen stays black after quest races on fast hardware. |

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
on it. The two switches below (`src/tick_rate/tick_rate.cpp`, both off by default) were removed on 2026-10-25 with the code (`src/tick_rate/` is to be deleted by hand); the findings are kept:

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
compatible. `src/subtick/`, all behind `--subtick` (off by default). The only
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
| `0x826B51FC` | Light set pointer | Read by `sub_820C8F10`. |
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
| `0x820069B0` | Music bank names | 18 strings, 12 bytes apart, read by `Bgm::Load`. |
| `[0x826B5960]` | The sound object | `Bgm::Load` calls its vtable slot 4 to load a bank pair. |
| `[0x826B2B28]` | Parameter tables | +8 BUILD 73 (wheels), +12 BUILD 72 (car specs), +16 to +48 BUILD 74-82 (aero parts), +56 the car count. Built by `sub_820C9700`, loaded by `sub_820CC048`. |
| car+1644 | A car object's tuning | +8 engine stage, +16 to +60 the power and drive-train part levels, +64 to +92 tires and wheels, +96 to +136 the aero part levels (`docs/game_data.md`, "How the game uses the car specs"). |

## Known issues

- None recorded yet.
