# TAA in the Vulkan build (plan)

Parked 2026-10-25 by the user's call (FXAA, `--fxaa`, went in instead for
now). Written 2026-10-25, when CMAA2 went in (`--cmaa2`, `src/native/passes.cpp`,
`ApplyCmaa2`). TAA was asked for at the same time; it isn't built yet,
because it needs data the renderer doesn't have per frame yet. This is what
it takes, in order, and what's known.

## Where it would run

The same place as CMAA2: before the main view's second color resolve of a
frame (the whole scene, before the bloom, the motion blur and the combine
read it; ITC's `docs/gpu-functions.md`, "A race frame's passes"). The 2x
MSAA resolve, a history image and the result go through the same steps
`ApplyCmaa2` uses (resolve to one sample, compute, draw back into every
sample). TAA and CMAA2 would be alternatives, or CMAA2 on TAA's input.

## What's needed

1. **Jitter.** A sub-pixel offset of the 3D view's projection, different
   each frame (a Halton 2,3 sequence of 8 or 16), on the main view's draws
   only. The renderer already writes each draw's `ndc_offset` into the
   translated shaders' system constants (`passes.cpp`, from
   `viewport_info.ndc_offset`): adding `jitter * 2 / extent` there for
   draws into the main view (`main_view_draw`) moves everything drawn into
   it, with no game changes. The velocity pass and the shadow and
   reflection passes stay unjittered.
2. **Motion vectors, per rendered frame.** The game already draws them:
   pass 9, a half-size (512x288) 16_16 target cleared to `0x80808000`, 350
   to 470 draws, resolved to `1B053000` and read by the motion blur. Its
   previous view-projection is vertex c100, recorded once a tick by
   `MotionBlur::Record` (`sub_821363F0`, view+8080/+8144). Two problems:
   - with `--subtick` it's the previous *tick's* matrix, so between ticks
     the vectors are wrong for TAA (the "motion blur's previous
     view-projection pulsing at tick rate" item). TAA needs the previous
     *frame's* view-projection: keep the last frame's blended camera
     matrix (the subtick camera code, `src/subtick/`) and write it into
     c100 on every frame;
   - it's camera motion against the current world matrices, so a car
     moving with the camera (the player's) gets the camera's motion, not
     its own (near zero on screen). Unless the pass draws cars with their
     previous world matrices (to check: dump `1B053000` with the camera
     still and a car moving, `--native_renderer_dump`), cars need a mask
     or their own previous transforms (`subtick/car_transforms.cpp` has
     both ticks' car matrices). This is also what per-object motion blur
     needs.
   The vectors' encoding (16_16 drawn as R16G16_SFLOAT by our renderer,
   around 0.5 = still, and their scale) has to be read from the motion
   blur's pixel shader (7EB16456AAD56CBA) first.
3. **The resolve.** A compute pass: reproject the history with the
   motion vector (upsampled from half size, the nearest-depth neighbour's
   vector), clamp it to the current pixel's 3x3 neighbourhood (in YCoCg,
   on the HDR values tonemapped for the clamp), blend about 10% current,
   and write the history for the next frame. A sharpening step after it
   (CAS) takes back some of the blur.
4. **Resets.** Drop the history on camera cuts (replays, the start of a
   race), when the resolution changes, and in menus (no main view).
5. **Flags.** `--taa` (off), `--taa_jitter` (debugging: jitter without the
   resolve, to see it), and the history weight.

## Order of work

1. Read the motion blur's shaders for the vector encoding; dump the
   velocity resolve in a race to see what moving cars get.
2. Per-frame c100 with `--subtick` (fixes the motion blur's pulsing too).
3. Jitter on main view draws, behind `--taa_jitter`.
4. The resolve pass and history; then the resets.
5. Cars: their own motion, or a mask, from step 1's answer.

## Size

Steps 1-2 are investigation with logs and dumps from the user's runs;
3-4 are a few days of renderer work. Each step needs a test run, since
TAA's faults (ghosting, smearing, shimmer) only show in motion.
