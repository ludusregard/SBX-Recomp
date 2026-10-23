# Cut car wheels: baselines

Suggested stock tire sizes and wheel positions for the cut cars (ECLI,
L86, S30, T86), measured against their own bodies and against cars that
are set up well: the numbers to put in the car specs and the node
files. `mod_tools/cut_car_wheels.py` applies them.

## How the cars were measured

For each tire node, the set-a body (`fnmg00`, node file + 191) was cut
around the axle to find:

- **Arch gap**: the smallest distance from the axle to the wheel arch
  (side view), minus the tire's radius. The space above the tire.
- **Tuck**: the arch lip's outer edge minus the wheel's outermost point
  (rear view). How far the wheel sits inside the fender.

Car space and node units are in `docs/game_data.md` ("Car space and the
sets compared"): a node's x is the half track and z the axle position,
in decimeters.

The well set-up cars:

| Car | Tires | Arch gap F / R (mm) | Tuck F / R (mm) |
|---|---|---|---|
| RX7 | 235/45R17, 255/40R17 | 32 / 42 | 28 / 19 |
| Z33 | 225/45R18, 245/45R18 | 43 / 45 | 15 / 24 |
| CROWN | 225/45R18 | 33 / 35 | 9 / 9 |
| SUPRA70 | 225/50R16 | 44 / 16 | 9 / 19 |

So the targets are an arch gap of 25-45 mm and a tuck of about 15 mm.

The cut cars today:

| Car | Tires | Wheel | Arch gap F / R | Tuck F / R |
|---|---|---|---|---|
| ECLI | 225/50R17 7.5J | AD00 (its own with `cut_car_wheels`) | 44 / 69 | 23 / 23 |
| L86, T86 | 195/60R14 6.0J | AD00 (TA03 or LORRY with `cut_car_wheels`) | 32 / 25 | 34 / 54 by the tire; 20 / 40 by the LORRY wheel |
| S30 | 175/82R14 5.0J | AD00 (TA03 or LORRY with `cut_car_wheels`) | 25 / 20 | 47 / 37 by the tire; 23 / 13 by the LORRY wheel |

The LORRY wheel's face sits 0.112 m out from its center, further than
the stock wheels (0.069-0.085 m) and further than a 14-inch tire's
sidewall, so with `cut_car_steelies` on, the wheel face is the outermost
point. Its rim has a 12-inch and a 20-inch shape (`A0_12_Shape`,
`A0_20_Shape`, radius 0.140 and 0.228 m), which the game presumably
blends by rim diameter, so it should fit any rim size. The stock wheels
(ECLI's included) have one size in both shapes, so the ECLI has to stay
on 17-inch rims.

## Baselines

| Car | Tires (F and R) | Rim width | Radius (m) | Stock grip | Arch gap F / R |
|---|---|---|---|---|---|
| ECLI | 225/50R17 (unchanged) | 7.5J | 0.3284 | 1.05 | 44 / 44 |
| L86, T86 | 185/60R14 (the AE86's own size) | 5.5J | 0.2888 | 0.9704 | 38 / 31 |
| S30 | 195/70R14 (the later Z's size) | 6.0J | 0.3143 | 1.0020 | 33 / 27 |

Tire node positions (decimeters, as stored; left side shown, the right
side is the same with x negated):

| Car | Node | Now (x, y, z) | Baseline (x, y, z) |
|---|---|---|---|
| ECLI | `tire_fl` | 7.85, 1.834, -12.875 | unchanged |
| ECLI | `tire_rl` | 7.85, 1.834, 12.875 | 7.85, **2.084**, 12.875 |
| L86, T86 | `tire_fl` | 6.775, 1.574, -12.0 | **6.825**, 1.574, -12.0 |
| L86, T86 | `tire_rl` | 6.725, 1.574, 12.0 | **6.975**, 1.574, 12.0 |
| S30 | `tire_fl` | 7.102, 1.668, -11.75 | **7.18**, 1.668, -11.75 |
| S30 | `tire_rl` | 7.102, 1.668, 11.75 | **7.08**, 1.668, 11.75 |

- **ECLI**: the size is right (the real car's), but the rear sits 25 mm
  higher in its arch than the front. Raising the rear tire nodes by
  0.25 dm evens it out.
- **L86, T86**: the real AE86 size; the smaller tire gives the rear more
  room. The rear moves out 25 mm (the rear arch lip is 15 mm wider than
  the front). The front node then matches the spec's front track (1.365).
- **S30**: 175/82R14 is the real early car's size, but it nearly fills
  the arch (20 mm at the rear). 195/70R14 is lower and wider. With the
  LORRY wheel, the positions barely change.

The positions are for `cut_car_steelies` on (the LORRY wheel's face 15 mm
inside the lip). With the BOSS00's TA03 wheels (`--cut_car_steelies=false`;
face 0.089 m out, like AD00's 0.087), the tire is the outermost point:
L86/T86 x 7.015 front, 7.165 rear; S30 x 7.325 front, 7.225 rear.

## Applying them

```
python mod_tools/cut_car_wheels.py BUILD_OUT OUT [--specs 00000072.bin]
```

writes `00000072.bin` and the eight node files below to `OUT`; copy them
into `assets/PATCH_DATA/BUILD/TXR/BUILD/`. With `--specs` it starts from
another car specs file (e.g. one already in `PATCH_DATA`) instead of
`BUILD_OUT`'s. The physics track isn't changed. By hand:

**Car specs** (BUILD 72, `docs/game_data.md`, "Tires"), in all 21 front
and all 21 rear tires (the size fields are the same in all 21):

| Field | L86, T86 | S30 |
|---|---|---|
| +44 radius (m) | 0.2888 | 0.3143 |
| +48 width (m) | 0.185 | 0.195 |
| +80 rim diameter (in) | 14 | 14 |
| +84 aspect ratio | 0.60 | 0.70 |
| +88 rim width (in) | 5.5 | 6.0 |
| +64, +68 grip | x 0.9832 | x 1.0205 |

The grip factor keeps each tire's grade: the stock grip follows
0.525 x (1 + 3.67 x sqrt(radius x width)) on these cars, and the upgrades
are fixed multiples of it. +52 and +76, which also grow with tire size,
aren't worked out and are left as they are.

Physics track (block 1 +540, +544): the visual and physics tracks already
differ on the S30 (nodes 1.420, spec 1.355). Matching the spec to the
baselines (L86/T86 1.365 / 1.395, S30 1.436 / 1.416) changes how the car
handles, so it's a separate choice.

**Node files**: the set-a and set-b node files both have the tire nodes:
ECLI 5186 and 14805, L86 5412 and 15002, S30 5638 and 15199, T86 5864
and 15396. The tire nodes are children of `tire_pos`, which sits at the
origin with no rotation, so moving one means writing its translation, the
translation row of its world matrix and its inverse
(`cut_car_wheels.py`, `move_node`).

Not checked in game yet: whether the game places the wheels from the
nodes or from the spec's track, and how far the LORRY wheel blends
between its two sizes.
