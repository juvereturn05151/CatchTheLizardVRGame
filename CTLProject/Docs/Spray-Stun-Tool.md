# Spray stun tool

Open `/Game/Maps/Map_CatchTheLizard`. The placed actor is **Spray_StunTool**, beside PlayerStart at **(55, -55, 110) cm**. The broom remains at (75, 75, 120) cm.

Pick up the bottle with either controller's grip. Hold that hand's trigger to emit smoke along the nozzle's local +X direction. Release the trigger or grip to stop emission. Existing puffs continue fading and expire naturally. Keep the lizard in the smoke until its purple halo appears, then catch it using the existing hand overlap and grip interaction.

## Editable settings

Select the placed spray or open BP_Spray Class Defaults. Settings are instance-editable in the **Spray** categories, with units in their names/categories.

| Blueprint | Setting | Default | Meaning |
|---|---|---:|---|
| BP_Spray | SprayRangeCm | 180 cm | Maximum puff-center travel from the nozzle; the sphere extends another radius |
| BP_Spray | SmokeRadiusCm | 22 cm | Visible puff radius and gameplay exposure radius |
| BP_Spray | SmokeLifetimeSeconds | 1.2 s | Individual puff fade/expiry time |
| BP_Spray | EmissionIntervalSeconds | 0.15 s | Time between new puffs while the trigger is held |
| BP_Spray | TriggerThreshold | 0.2 | Trigger-axis threshold, on a 0–1 scale |
| BP_Lizard | SmokeExposureThresholdSeconds | 1.0 s | Accumulated smoke exposure required to stun |
| BP_Lizard | SmokeExposureDecayPerSecond | 0.5 s/s | Exposure removed each second outside smoke |
| BP_Lizard | SmokeStunDurationSeconds | 3.0 s | Movement pause after reaching the exposure threshold |

Edit BP_Spray's **Nozzle** component transform to change the emission position and direction. BottleMesh and NozzleMesh are replaceable placeholder cubes. GrabCollision and the existing BP_GrabComponent handle pickup separately from smoke exposure.

Puff initialization clamps range to 1–1000 cm, radius to 2–100 cm, and lifetime to 0.1–5 seconds. Emission has a minimum 0.08-second interval and spawns at most once per frame. These bounds limit active puff counts; they do not replace profiling on Quest 2. Exposure threshold and stun duration have a 0.01-second minimum; negative decay becomes zero.

## State and exposure behavior

- Priority is **Caught > Smoke Stun > Broom Daze > ordinary movement/fleeing**. The prototype uses Blueprint flags and movement gates rather than a state enum; the integration follows that design.
- BP_Lizard samples the union of active smoke spheres once per Tick. Overlapping puffs do not multiply exposure. Accumulation and decay use DeltaSeconds.
- Each puff checks distance and a Visibility trace to the lizard. A radius-matched movement sweep stops forward propagation at obstacles. The tool also traces from the holding controller to the nozzle before emission. Walls must block Visibility, as they do in the gameplay level.
- Stun resets exposure, cancels broom daze/forced flee, clears the old feedback timer, and blocks threat sensing and further rakes. Smoke cannot restart or extend the current stun.
- The movement gate preserves location, rotation and surface normal during stun. Expiry restores feedback and immediately reevaluates normal threats. It never resumes movement when Caught is true.
- RestoreCatchFeedback is guarded so a stale broom callback cannot hide the stun feedback. A separate purple SmokeStunHalo makes stun visible with the imported lizard material, which lacks the prototype's BodyColor parameter. Normal catch feedback can still restore and hide the halo.
- Both original TryCatchLeft and TryCatchRight function graphs are unchanged. BP_GrabComponent, BP_XRPawn and the existing input assets are unchanged. BP_Spray listens to the existing non-consuming left/right index-trigger actions and selects the currently holding hand.

## Assets

All gameplay remains in Blueprints under `/Game/CatchTheLizard`:

- `BP_Spray`: pickup, per-hand trigger handling, nozzle and emission.
- `BP_SmokePuff`: independent lifetime, fade, forward motion and unobstructed exposure query.
- `BP_Lizard`: exposure, stun state, feedback halo, and guards around broom/normal behavior.
- `M_SpraySmoke`, `MI_SmokeStunHalo`: lightweight unlit translucent sphere effects with soft edges; no Niagara simulation, dynamic lighting, or effect shadows.
- `/Game/Maps/Map_CatchTheLizard`: placed spray actor.

Editor authoring/test scripts are in Saved/Scripts; they are not runtime dependencies. Pre-spray lizard and level backups are in Saved/SprayBackup.

## Verification and headset checks

Desktop PIE checks cover threshold accumulation, frame subdivision, overlapping puffs, decay, geometry obstruction, movement blocking, state priority, repeated-exposure suppression, both original catches during stun, feedback guards, and expiry. A running PIE input test injects left/right Enhanced Input trigger-axis values to exercise the real Blueprint input events. This is **not a physical controller test**. The existing broom regression checks are also rerun. Detailed results are in `Spray-Verification.json`.

Final verification passed 39 spray gameplay checks, 10 running PIE input/timing checks, 5 smoke movement/wall checks, and 26 broom regression checks. All five involved Blueprints compiled with warnings treated as errors. The level was saved and reloaded, and the placed lizard's halo mesh/material assignments were verified again. A broken smoke material input and stale empty halo assignments on the existing level instance were repaired. The rendered result was inspected in `Spray-Preview.png`.

Quest 2 testing remains required for physical grip/trigger input, tracking and hand transfer, comfortable placement, smoke/halo readability in stereo, timing and range feel, and Android frame rate/overdraw. No headset deployment or Android performance measurement was performed.
