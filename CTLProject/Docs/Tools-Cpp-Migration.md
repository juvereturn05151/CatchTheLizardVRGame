# Broom and spray C++ migration

**Subsequent spray fix (2026-09-28):** `CanExpose` and the lizard's elapsed-time puff polling were removed. Puffs now deliver one wall-checked swept hit per lizard; the lizard owns percentage exposure and fall/grounded stun. See [Spray-Hit-Stun-Fix.md](Spray-Hit-Stun-Fix.md). The results below describe the original migration before that fix.

The existing CTLProject runtime module now implements the tool gameplay. The original asset paths and gameplay map are retained. No new Unreal project, input mappings, pickup framework, or visual components were created.

## Native responsibilities

- `ABroomGameplayActor`: observes the existing `BP_GrabComponent`, resets samples on grab/drop/hand transfer, samples world tip velocity, queries tagged tip overlaps and swept contacts, and latches qualifying contacts until separation. Pawn transform changes reset the sample to prevent teleport/snap-turn rakes. It calls `ALizardGameplayActor::ReceiveRake`; the lizard still owns relocation, cooldown, feedback, daze and forced flee. Other `BPI_Rakeable` receivers retain their interface call.
- `ASprayGameplayActor`: binds the existing left/right index-curl Enhanced Input actions, selects the holding controller using the grab system's existing MotionSource convention, and manages bounded emission cadence. Trigger completion/cancellation and drop stop spawning; drop also resets emission through the grab component's existing dispatcher. Controller-to-nozzle Visibility obstruction still prevents emission with the nozzle through a wall.
- `ASmokePuffGameplayActor`: initializes each puff's radius/range/lifetime, moves forward over 60% of its lifetime, stops travel on radius-matched Visibility sweeps, fades its existing material's Opacity, and expires independently. `CanExpose` reports radius plus Visibility line-of-sight eligibility. It never applies or accumulates exposure.
- `ToolBlueprintBridge`: a small checked adapter for existing grab state, zero-argument grab/drop dispatchers, Pawn victory lock, and named Blueprint components. Grabbing and attachment remain in XRFramework.

`ALizardGameplayActor` is unchanged in this pass. Its existing puff union query samples elapsed time once per lizard, so overlapping puffs do not duplicate exposure. Stun and caught/storage priority remain owned by the lizard. The collection manager's existing native-compatible `UpdateEmission(0, false)` call remains usable on victory.

## Blueprint and designer workflow

`/Game/CatchTheLizard/BP_Broom`, `BP_Spray`, and `BP_SmokePuff` now inherit their corresponding native classes. Their replaced function graphs, tick/input paths and gameplay variables were removed. Only the empty EventGraph and original construction entry remain. Blueprint SCS still supplies the handle collision, tip sphere, meshes, nozzle placement, smoke mesh/material, and unchanged `BP_GrabComponent`. No duplicate SCS components or gameplay paths remain.

Open Class Defaults or select the placed tool to edit inherited settings:

| Asset/category | Settings | Preserved defaults |
|---|---|---|
| BP_Broom / Broom > Swing | RakeVelocityThreshold | 150 cm/s |
| BP_Broom / Components | RakeHead placement/radius; separate GrabCollision | Tip radius 12 cm; handle box (7,7,22.5) cm |
| BP_Spray / Spray > Smoke | SprayRangeCm, SmokeRadiusCm, SmokeLifetimeSeconds | 180 cm, 22 cm, 1.2 s |
| BP_Spray / Spray > Emission | EmissionIntervalSeconds | 0.15 s; runtime minimum 0.08 s, at most one puff per frame |
| BP_Spray / Spray > Input | TriggerThreshold | 0.2 on the 0–1 axis |
| BP_Spray / Spray > Integration (Class Defaults) | SmokePuffClass, LeftTriggerAction, RightTriggerAction | Original puff Blueprint and `/Game/XRFramework/Input/Actions/Hands/IA_Hand_IndexCurl_*` |
| BP_SmokePuff / Smoke > Appearance | OpacityParameter, InitialOpacity | Opacity, 0.3 |
| BP_Lizard / Lizard > Broom | Relocation height, cooldown, daze, threat bonus, forced flee | Existing settings unchanged |
| BP_Lizard / Lizard > Smoke | Exposure threshold/decay and stun duration | 1 s, 0.5 exposure s/s, 3 s |

The puff Blueprint class and trigger assets are explicitly saved in `BP_Spray` defaults. This preserves asset dependencies after removing the original spawn/input graphs. Blueprint defaults and placed-instance gameplay values were captured before deleting variables and restored to their inherited native counterparts.

In `/Game/Maps/Map_CatchTheLizard`, **Broom_HerdingTool** remains at (75,75,120) cm and **Spray_StunTool** at (55,-55,110) cm, with their existing rotations/scales. Use either controller's grip to pick up/release. Swing the broom tip to rake; hold the spray's holding-hand trigger to emit. Existing smoke expires after release/drop.

## Intentional corrections and limits

The previous broom had pickup/transfer suppression but no teleport suppression. Native code now resets velocity/contact samples when the holding Pawn's transform changes (including snap turns). A physical controller swing within an unchanged Pawn transform retains the original world-speed threshold. A drop/re-grab between ticks now resets the sample immediately through the grab dispatchers. The overlap and sweep paths now share an attempted-target set, avoiding two calls for the same target in one sample. Lizard cooldown still owns acceptance.

The sweep radius now follows the existing RakeHead sphere's scaled radius instead of a hardcoded 12 cm. At the saved radius/scale its behavior is unchanged; designer edits to the sphere also update the sweep.

Smoke remains the existing lightweight unlit translucent sphere/puff placeholder. There is no new particle system or art. Existing range/radius/lifetime clamps (1–1000 cm, 2–100 cm, 0.1–5 s) remain. No new exposure or stun behavior was invented.

## Recovery and verification

`Saved/MigrationCheckpoint/20260928-pre-tools-cpp` contains pre-tool-migration Content, Config, Docs, Source, .uproject, authoring scripts, graph exports, class/instance/component snapshots and SHA-256 manifests. Existing uncommitted work was preserved without reset or commit. For recovery, close Unreal and restore the checkpoint's matching Source/Content/Config/Docs/.uproject together; then rebuild the Editor target before reopening.

Build and editor evidence is recorded in `Tools-Cpp-Verification.json` and `Saved/Scripts/tools-cpp-build.log`. Verification is deliberately limited to Editor build, affected Blueprint compilation with warnings treated as errors, saved map reload/reference/default/component/placement audits, and a short desktop PIE integration check. It does not constitute physical controller or headset testing.

The Win64 Development Editor build passed with Unreal 5.8.2, MSVC 14.44.35228 and Windows SDK 10.0.22621.0. All ten involved Blueprints have an explicit `BS_UP_TO_DATE` status with no compiler warnings/errors. Map Check reports zero errors/warnings. Seventeen focused PIE assertions passed: native placed classes and puff initialization, pickup/drop sample resets, held threat query, Pawn movement suppression, both existing trigger bindings and holding-hand selection, release/drop emission stop with surviving puffs, native exposure reporting, single accumulation across overlapping puffs, and blockage by the existing wall. Input values and transforms were scripted on the desktop.

The final compilation check exposed an existing disconnected `GetActorOfClass` execution pin in `WBP_CollectionVictory`. Its Restart click now executes that existing manager lookup before the unchanged cast/RestartRound call. This removes the compiler warning. Restart interaction itself remains a manual test. Other pre-existing assets changed only in the three tool Blueprints and gameplay map; Pawn, grab, input, lizard, bottle and manager asset files are unchanged. Existing generic GameFeatureData Asset Manager and unavailable desktop XR-extension startup diagnostics remain outside this migration; they did not block the map or PIE checks.

Manual Quest 2 checks remain: grip/release and rapid re-grab with either hand; holding-hand trigger selection and release; broom contact feel/separation and teleport/snap-turn suppression; puff travel, fade and walls; smoke overlap accumulation/stun priority/catching; victory emission stop and regression of bottle/hand interactions; performance and tuning. Android packaging and the previously reported Android toolchain issue were outside this task.
