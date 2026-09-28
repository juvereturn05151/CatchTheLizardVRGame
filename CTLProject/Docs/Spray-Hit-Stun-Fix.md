# Hit-based spray exposure, fall and stun

## Cause and fix

The old implementation did not credit bubble collision. Puffs used a blocking Visibility sphere sweep and stopped at the previous position when blocked. The imported LizardBody has a custom collision profile that blocks Visibility; CatchSphere overlaps Visibility. After stopping, a separate lizard poll required the lizard actor center to be within the puff radius and accumulate a full second of continuous exposure. Contact with the lizard mesh/catch volume was not equivalent to that center/dwell test. Short moving contacts and mesh/wall obstruction could therefore show a bubble near the lizard without meaningful exposure or stun feedback.

The old `CanExpose`/`SampleSmokeExposure` path and seconds-based exposure fields were removed. Each native gameplay bubble now sweeps a radius matching the visible puff through the unobstructed part of its movement. Wall sweeps ignore lizards so their bodies cannot prematurely stop a bubble as an occluder. A separate WorldDynamic sphere sweep detects the existing lizard collision volumes. A Visibility line-of-sight check rejects credit across walls even if the sphere overlaps through geometry. Multiple components and repeated contact are merged by per-bubble/per-lizard identity, with a second native receiver guard. The holding-controller/nozzle wall check also treats lizards as targets rather than occluders.

The lizard receives a configurable percentage-point gain per bubble, capped at 100%. No elapsed-time overlap accumulation remains. Independent puffs still move/fade/expire using the existing lightweight unlit smoke mesh/material and range/radius/lifetime settings. Trigger/grab behavior is unchanged.

## State and display

At 100%, `ALizardGameplayActor` clears broom daze/forced flee and feedback recovery, stops normal surface movement, detaches and enters stunned falling. Gravity with substepped static-world sphere sweeps moves/slides the lizard until it finds ground (normal Z at least 0.65). Wall contacts cannot reattach it. A lizard already supported by ground lands immediately. There is no return-to-wall timeout in stunned fall.

`bSmokeStunned` covers both falling and grounded incapacitation; `bSmokeFalling` distinguishes the fall. The recovery deadline is created only when ground is reached. Stun then expires at the configured deadline, resets exposure/UI/feedback, and resumes surface movement using the current supporting ground normal and a valid tangent direction. Further bubbles cannot restart or extend the fall/stun. Existing catches remain allowed during either phase; catching or storage cancels the deadline and hides the status widget before any recovery path can run. Both existing hand-catch functions and tool occupancy checks remain.

`WBP_LizardSprayStatus` is an editable UMG Widget Blueprint derived from `ULizardStatusWidget`. Its Border/VerticalBox/TextBlock/ProgressBar tree is visual only; C++ updates `ExposureBar` and `StatusText`. It shows a 0–100% exposure bar above zero, `STUNNED` during falling, and `STUNNED — 3.0s` (remaining time) after landing. It hides on recovery, zero exposure, catch or storage. The Blueprint's `SprayStatusWidget` component uses world space, no collision/hardware input/shadows, 640×160 draw size and 0.055 scale (about 35×9 cm). C++ keeps its offset in world coordinates and faces it toward the player's camera/headset. Layout, fonts, colors, draw size/scale and materials remain editable.

## Settings

Open `BP_Lizard` Class Defaults or select the existing placed lizard in `/Game/Maps/Map_CatchTheLizard`:

| Category | Setting | Default/unit |
|---|---|---|
| Lizard > Spray Exposure | SmokeExposurePerHitPercent | 25 percentage points per bubble (four hits to 100%) |
| Lizard > Spray Exposure | SmokeExposureDecayPercentPerSecond | 50 percentage points/second |
| Lizard > Spray Exposure | SmokeExposureDecayDelaySeconds | 0.75 seconds since last accepted hit |
| Lizard > Smoke | SmokeStunDurationSeconds | Existing 3 seconds, starting at landing |
| Lizard > Spray UI | SprayWidgetWorldOffset | (0,0,32) cm above the actor in world coordinates |
| Lizard > Stun Fall | SmokeFallCollisionRadiusCm / SmokeFallGravityScale | 8 cm / 1× world gravity |
| Lizard > Debug | bDebugSprayState | Off; logs hit, exposure, activation, landing and recovery |

The old decay rate (0.5 exposure seconds per second, with a 1-second threshold) was converted to the equivalent 50 percentage points/second. Saved stun-duration defaults/instance overrides were preserved. The delay is newly editable. Spray range, radius, lifetime, cadence and trigger threshold remain in `BP_Spray` defaults/instances. Stun halo/mesh feedback and the existing lizard components/collision settings remain in `BP_Lizard`. Do not add exposure bindings/gameplay to the widget; its native `OnStatusChanged` event is available for visual styling/animation only.

## Build, recovery and manual checks

Recovery files are in `Saved/MigrationCheckpoint/20260928-pre-spray-hit-fix`: matching Source, Content, Config, Docs, .uproject, SHA-256 manifest and defaults/component/placement inspection. Existing uncommitted work was preserved.

Win64 Development Editor build evidence is in `Saved/Scripts/spray-hit-build.log` and `spray-hit-final-build.log`. Final compilation, preservation and focused desktop PIE checks are recorded in `Spray-Hit-Stun-Verification.json`. No extensive automated gameplay suite, Android package or headset test was run.

The final Editor build succeeded. Twelve related Blueprints (including the status widget and existing interaction callers) compiled with warnings treated as errors and reported up-to-date status. The saved gameplay map reloaded, all 23 original actor placements matched the checkpoint, existing lizard component transforms/assets/collision matched, and Map Check returned 0 errors and 0 warnings. Game asset dependencies resolved. Only BP_Lizard, the map, the listed native source and migration documentation changed; existing broom/spray/puff visuals, grab/Pawn/input assets and project configuration matched the checkpoint hashes.

Sixteen focused PIE checks passed: swept contact adds 25%; repeated bubble/component contact adds nothing; the actual bar updates immediately; wall-overlap credit is rejected; four distinct hits trigger falling stun; no deadline exists during falling; additional hits do not extend it; gravity moves down without reattachment; landing starts the full duration and time label; grounded stun starts immediately; old feedback/rake paths cannot override stun; repeated activation does not extend the deadline; actual three-second expiry resets UI/exposure and resumes on the ground; existing left-hand catch during grounded stun and right-hand catch during falling cancel recovery/UI. These used native calls and simulated hand positions in PIE, not physical controller input.

Quest 2 checks remain: real trigger aim and hit feel, meter/stun text stereo readability and headset facing, tuning exposure/decay, ceilings and room edges, catching with either physical controller during fall/stun, recovery from floors/slopes, broom state priority, and bottle/teleport regressions. Falling collision intentionally uses static room geometry; it does not simulate ragdoll physics or land on held tools/hands.
