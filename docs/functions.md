# Known functions

Names for the generated `sub_XXXXXXXX` functions, worked out so far. The
address is the function's location in the original `default.xex`, so it also
matches Ghidra or IDA. Names are our own guesses; the game shipped without
symbols. The data files the code reads are described in
`docs/game_data.md`.

"Rewritten" means a readable replacement lives in `sbx_recomp/src/` and
overrides the generated code; "Wrapped" means a replacement there runs extra
code around the original (`__imp__sub_XXXXXXXX`). "Patched" means the
function is overridden in `sbx_recomp/src/hooks/` with its generated code
and an edit (search for `PATCH:`); the generated code itself is regenerated
by every build, so it isn't edited in place.

| Address | Name | Status | Notes |
|---|---|---|---|
| `sub_82090108` | `main` |  | Guest entry point on the main thread. |
| `sub_825739E0` | `Thread::Start` |  | Entry point of the game's threads: calls vtable slot 1 of the object at start context + 8. |
| `sub_82095FD8` |  |  | A worker thread's loop (started through `sub_825739E0`): waits on an event, runs jobs through function pointers, signals events; it opens the game's data files at boot. Busy only around loads. |
| `sub_8209A8F0` | `App::Run` |  | Main loop: calls every handler in the tick list at this+4, repeats until the quit flag at this+12 is set. |
| `sub_8209A400` | `ForEachTickHandler` |  | Calls each handler in a list through a member function pointer. |
| `sub_82093280` | `Renderer::Init` |  | Sets the vblank interval (2 = 30 fps) via `sub_82573100`, then subscribes renderer callbacks to numbered events (2, 3, 5, 6, 16). |
| `sub_82092E60` | `Renderer::WaitForFrame` |  | Tick handler on the main thread. Blocks until the render thread signals the frame is done (event at this+392), swaps, and starts the next frame. |
| `sub_82092050` | `Renderer::ThreadLoop` |  | Render thread loop; started through `sub_825739E0`. Draws one frame each time the main thread asks, after a 2-vblank wait. |
| `sub_82091B60` | `Renderer::Present` |  | Flushes, waits for the vblank interval (`sub_825737E8` → `sub_82573638`), then swaps buffers. Event 5. |
| `sub_82091FB0` | `Renderer::DrawFrame` |  | Render thread: calls each draw listener in the list at renderer+28. |
| `sub_82091CC0` |  |  | Draw listener between the scene draw and present: resolves the frame into a front buffer (`sub_82573538`) and waits for the GPU to go idle (`sub_8229CB08`). |
| `sub_82098DF0` | `Scene::Update` |  | Tick handler on the scene object (0x40026FE0 at the menu). Calls the current mode object at +64 (vtable slots 6 and 2). |
| `sub_82098E68` | `Scene::Draw` |  | Draw listener on the same object. Calls the mode object's draw (vtable slot 5), reading live game state. |
| `sub_8216F0F8` | `Delegate::Invoke` |  | Calls the function at +8 with the argument at +4. Used for tick and draw listeners. |
| `sub_8222E468` | `MemberDelegate::Invoke` |  | Calls the member function at +8 on the object at +4, adjusted by +12. |
| `sub_82573100` |  |  | Receives the frame interval from `sub_82093280`. |
| `sub_82573638` | `VideoOutput::WaitForVblank` |  | Waits for the 2-vblank interval after a device call (`sub_8229DD58`). |
| `sub_82573920` | `Event::Wait` |  | r3 = event, r4 = pointer to timeout (-1 = forever). |
| `sub_82573908` | `Event::Signal` |  | r3 = event. |
| `sub_822AE7E8` |  |  | Graphics driver present bookkeeping (frame counter at +20024). Library code, not game timing. |
| `sub_8259F788` | `QueryPerformanceCounter` |  | System library. |
| `sub_820BFF60` | `Vehicle::GetSpeedKmh` |  | Returns the float at vehicle+48 (m/s) times 3.6. |
| `sub_820943D8` |  |  | First tick handler: polls input, then walks listener lists that run both game logic and the draw step. |
| `sub_82106478` | `GameMessages::Dispatch` |  | Delivers the game's queued messages to their handlers: 0 = Update (all game logic), 1 = Draw. |
| `sub_82106410` |  |  | Picks the handler object a message goes to; its method 0 handles it. |
| `sub_82099700` | `Input::Update` |  | Input manager update. |
| `sub_82159540` | `Race::DrawStep` |  | The race's draw step (Draw message). Writes a field at +64 of the object its calls return. |
| `sub_820A3B98` | `CameraPass::Update` |  | Rebuilds a camera pass's view (+0x10) from its eye, target and up vector (+0x190, +0x1A0, +0x1B0), and its view-projection (+0x90 = view * projection at +0x50). Called several times per Draw for pass 0. |
| `sub_8209BE70` | `Matrix::Multiply` |  | out (r3) = a (r4) * b (r5), 4x4. |
| `sub_8257ACD0` | `Recorder::RecordObject` |  | The bulk per-object recorder, ~150 calls per pass in a race: records command 7 with vertex constants 0-8: world (r5) * camera view-projection (r4) in c0-c3, the world matrix in c4-c7 (for some objects a projective matrix instead), and the camera's world position in c8. The camera is often a stack copy, not a camera pass. |
| `sub_8257AAE0` |  |  | Multiplies a world matrix and camera into a stack temporary; not recorded (likely a visibility test). |
| `sub_825758A0` | `Command::SetShaderConstants` |  | Render thread, command 7: r3 = payload, r4 = register count, r5 = stage << 24 \| type << 16 \| start << 8 \| count. |
| `sub_82572638` | `CommandList::AllocateSlots` |  | r3 = list, r4 = slot count; returns the first 16-byte slot. |
| `sub_820BFF88` | `Vehicle::GetDistancePerFrame` |  | Speed (m/s) times a hard-coded 1/30 s. |
| `sub_8210F260` |  |  | Wind sound; scales with speed. |
| `sub_8213DE38` |  |  | Parent of the speed trail object (switches mode at about 88 km/h). Calls `sub_8213DBE8`, `sub_8213DA48` and `sub_8213DDB8`. |
| `sub_8213BEB8` |  |  | Resets the trail history; runs on a camera switch. |
| `sub_820F1038` | `TextureAnimations::Advance` |  | From the draw step part `sub_820F5F48`: advances the flipbook animation set at r3+1060 (`sub_821453B0`). |
| `sub_821453B0` |  |  | Calls `sub_82145350` on every animation in a set: count at +0, then offsets to each animation. |
| `sub_82145350` |  |  | Advances one flipbook animation: frame count +0, current frame +4, tick counter +8 (wraps at the length at +12), frame start ticks from +16. |
| `sub_82145410` |  |  | Returns the current frame's entry of animation r4 in set r3 (texture id at +4). |
| `sub_821452C8` |  |  | Finds the animation in set r3 whose texture is r4; -1 if none. |
| `sub_82146F00` |  |  | Draws the light groups: for each group, builds a light list on the stack (count +32, 48-byte lights at +36, from static level data) and picks its texture's current animation frame. Calls `sub_8209CED0` → `sub_8209CD58`. |
| `sub_820F5610` | `Minimap::Draw` |  | From the draw step part `sub_820F5F48`; steps the minimap blink clock at `0x826B5784`/`0x826B5788` after drawing. |
| `sub_82107A38` |  |  | From Draw; adds one to the play time frame counter at `0x826B5870`. |
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
| `sub_8209E348` | `Archive::Init` |  | At boot: allocates the two TOC buffers, sized from the entry counts built into the XEX (`0x825F0A90`, `0x825F0AA0`), reads `DATA\BUILD\TXR\BUILD.TOC` and `BUILD_CRS.TOC` into them, and opens `game:\DATA\BUILD\TXR\BUILD.DAT` and `BUILD_CRS.DAT` (`docs/game_data.md`). Called from `sub_821066D0`. |
| `sub_8209D988` | `Archive::GetToc` |  | r3 a file id: returns the `BUILD` TOC buffer for ids below 23009, else the `BUILD_CRS` one. |
| `sub_8209DF80` | `Archive::Load` |  | r3 a file id, r4 the destination, r5-r8 options: reads the id's TOC entry (start sector, stored and decompressed size), checks `Archive::FindLooseFile`, and queues a read for the loader thread (`sub_8209DD38`). Ids at or above 23009 are in `BUILD_CRS`. |
| `sub_8209D9F8` | `Archive::FindLooseFile` |  | r3 a file id: returns the loose file's path from the table at `0x825F0AB0`, or 0. |
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
| `sub_82118818` |  |  | Builds the path `data\audio\%s.xgs`. |
| `sub_8228CEE8` |  |  | One of four functions (with `sub_8228CFA8`, `sub_8228D228`, `sub_8228D370`) that open the save, `TXR360:\TXR_DATA_1`. |
| `sub_82105790` | `GameData::Get` |  | r3 an index: returns the pointer at `0x826B5838` + 4 x r3. Index 0 is the save (`docs/game_data.md`). |
| `sub_82113D28` | `SaveData::Clear` |  | r3 the save: zeroes it and sets its defaults. |
| `sub_82113FB0` | `SaveData::NewGame` | Wrapped | r3 the save: clears it (`SaveData::Clear`) and sets a new game's state, including the car unlock bits at +0x1985C (every car but CON_X, ECLI, S30, L86, T86 and BOSS00). Run by the title menu's new game and at boot on the empty saves. Sets the cut cars' bits after it (`src/hooks/unlock_cut_cars.cpp`, `--unlock_cut_cars`). |
| `sub_82114278` | `SaveData::NewGameKeepCars` | Wrapped | r3 the save: a new game that keeps the garage (+6120), the word at +6116 and the unlock bits at +0x1985C (copies them out, runs `SaveData::NewGame`, copies them back). From the title menu (`sub_8221F568`). Sets the cut cars' bits after it (`src/hooks/unlock_cut_cars.cpp`). |
| `sub_82114D88` | `SaveData::Unlock` |  | r3 the save, r5 what to unlock (a switch of 42 cases), r6 its id. One case sets car r6's bit at +0x1985C if it isn't set yet; new unlocks are queued for the garage notifications. |
| `sub_8217F2A0`, `sub_82272370`, `sub_8224A0E8` |  |  | Build the car lists (the starter cars and the shop): for each of the 72 cars with its bit set at save +0x1985C, copy its specs (`sub_820C9C38`) into list 0 or 1 by spec byte +771 (0 for the road cars, 1 for CON_X and the rival cars). |
| `sub_820C9C38` |  |  | r3 a car id: returns its 8,432-byte record in the car specs (BUILD 72). |
| `sub_8218C240` | `PostRace::Check` | Patched | Post-race check: once loaded (+9608), waits for a counter (global at `0x826C7EC4`) to reach 60 before setting the next state at +9604. The 60 is 1 here (`src/hooks/post_race.cpp`, `--post_race_check_fix`): with 60, the screen stays black after quest races on fast hardware. |

## Fixed time step

The game never measures elapsed time. Each pass of `App::Run`
(`sub_8209A8F0`) is assumed to be 1/30 s, so running the loop at 60 fps
doubles game speed. Functions that load the step as a float constant from
`.rdata` (`1/30` at `0x820029C4`, `1/60` at `0x820029C0`):

- 1/30: `sub_820B1678`, `sub_820B8BC0`, `sub_820BFF88`, `sub_820C2208`,
  `sub_8210DBE8`, `sub_8210F390`, `sub_8211F6F0`, `sub_82129DE0`
- 1/60: `sub_820B0DD0`, `sub_8226F9D8`, `sub_82153E40`, `sub_821E6F28`

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
| `0x82000A20` | 1.0f | w of the light positions `Lights::FindNearest` writes. |
| `0x825F0A84`, `0x825F0A88` | `BUILD.DAT`, `BUILD_CRS.DAT` file handles | Opened by `Archive::Init`. |
| `0x825F0A90`, `0x825F0AA0` | Built-in copies of the `BUILD.TOC` and `BUILD_CRS.TOC` headers | Count (23009, 1359), two unknown words, `.DAT` size. Only the counts are read; adding archive entries needs these patched (`docs/game_data.md`). |
| `0x825F0AB0` | Loose file table | Pairs of file id and path, ended by a null path. |
| `0x82694E00`, `0x82694E08` | `BUILD` and `BUILD_CRS` TOC buffers | Sizes at `0x82694E04`, `0x82694E0C`. |

## Known issues

- None recorded yet.
