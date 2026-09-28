# Lizard C++ migration

**Subsequent spray fix (2026-09-28):** The historical smoke settings/checks below were superseded by authoritative percentage exposure per bubble, gravity fall, grounded stun and an editable world-space status widget. See [Spray-Hit-Stun-Fix.md](Spray-Hit-Stun-Fix.md) for current behavior, settings and verification.

The existing CTLProject now has a runtime C++ module and Game/Editor targets for its installed **Unreal Engine 5.8.2**. Open CTLProject.uproject or the generated CTLProject.sln. No replacement project was created.

## Responsibilities

- `ALizardGameplayActor` implements the lizard's existing surface traces/movement, hand and held broom threat sensing, alert/flee behavior, left/right catching, grip-release dropping, bottle storage eligibility, height-limited rake relocation, daze/forced flee, smoke exposure/stun, feedback restoration, and state/timer priority.
- `LizardBlueprintBridge` is a small internal helper that calls the existing Pawn/tool Blueprint functions using checked reflected parameters. It contains no broom, spray, pickup, teleportation, or input-mapping gameplay.
- `BP_Lizard` remains at its original asset path, now inheriting `ALizardGameplayActor`. It owns the original scene root, body mesh/materials and their transforms, catch collision, facing indicator, and stun halo. Its only functional graph is a three-node `BPI_Rakeable.Raked` adapter calling native `ReceiveRake` and returning `Accepted`. Construction and EventGraph contain no migrated gameplay. The Blueprint has no duplicate gameplay variables/functions.
- The subsequent tool migration added native broom, spray and smoke-puff behavior; see `Tools-Cpp-Migration.md`. Bottle manager/victory UI, VR Pawn, grabbing, teleportation, and input assets remain Blueprint systems. Lizard callers remain compatible with inherited native members.

## Designer editing

Select the existing lizard in `/Game/Maps/Map_CatchTheLizard`, or open `BP_Lizard` Class Defaults. Native settings appear under **Lizard**:

| Category | Settings | Preserved defaults |
|---|---|---|
| Movement | WanderSpeed, FleeSpeed | 15, 70 cm/s |
| Surface | SurfaceNormal, MoveDirection | (1,0,0), (0,1,0) |
| Threat | DetectionRadius, FleeReevaluateInterval, AlertDuration | 80 cm, 0.2 s, 0.3 s |
| Catch | CatchRadius | 12 cm; preserved prototype value, actual eligibility uses CatchSphere/hand overlap |
| Broom | RakeCooldown, MaxRakeHeight, DazedDuration | 0.5 s, 140 cm above VR supporting floor, 0.4 s |
| Broom | BroomDetectionRadiusBonus, ForcedFleeDuration | 20 cm, 1 s |
| Smoke | SmokeExposureThresholdSeconds, SmokeExposureDecayPerSecond, SmokeStunDurationSeconds | 1 s, 0.5 exposure s/s, 3 s |
| Feedback | BodyColor parameter, normal/catch/daze/stun colors and scales, pulse duration | Existing values; pulse 0.2 s |
| Integration | Existing broom/puff classes and four grip input actions | Existing project assets |

Keep editing meshes, materials, effect components, component transforms, and collision in the Blueprint. Native code creates no duplicate visual/collision components. The placed lizard remains at **(-391, 0, 140) cm**. Broom/spray settings are edited in their existing Blueprint subclasses or placed instances, using inherited native settings.

Stored/caught states block movement and recovery first. Stun supersedes daze and forced flee. Overlapping smoke accumulates elapsed time once per lizard; exposure resets at stun and cannot extend its deadline. Grip release drops a held lizard using the existing implemented fall/landing fallback. Bottle storage still requires the real opposite-hand attachment and mouth overlap.

## Build

The Win64 Development Editor target was built before migration and again with the native lizard. The accepted installed toolchain is **MSVC 14.44.35228** and **Windows SDK 10.0.22621.0**, using the engine's bundled .NET SDK. There is no missing Windows C++ requirement.

From PowerShell, build with:

```powershell
& 'E:\UnrealEngine5.3.2\UE_5.8\Engine\Build\BatchFiles\Build.bat' CTLProjectEditor Win64 Development '-Project=E:\CatchTheLizardVR\CatchTheLizardVRGame\CTLProject\CTLProject.uproject' -WaitMutex -NoHotReloadFromIDE
```

Close the editor before a full build that replaces its loaded game DLL. Project files were generated with the same Build.bat using `-projectfiles`, `-Game`, and `-Engine`. Editor authoring/test Python is not a runtime dependency.

## Recovery checkpoint

`Saved/MigrationCheckpoint/20260928-pre-cpp` contains the complete pre-migration Content, Config, Docs and .uproject, the prior authoring scripts, SHA-256 manifest, graph/parameter exports, defaults/placed-instance snapshots, and build evidence. No existing uncommitted files were reset or discarded. For rollback, close Unreal and copy the checkpoint's Content/Config/Docs and CTLProject.uproject back to the same project, preserving any later work separately first. The original descriptor is Blueprint-only; do not load migrated assets with that descriptor without also restoring their matching originals.

## Verification and remaining device checks

Detailed current results are in `Lizard-Cpp-Verification.json`. Checks use the real migrated actor and existing tool/Pawn Blueprints in desktop PIE; controller transforms and Enhanced Input values are scripted. They are not physical controller tests.

The final audit passed **148 runtime checks**, compiled all nine involved Blueprints with warnings treated as errors, reloaded the saved map with zero map-check errors/warnings, and repeated native actor/hand-reference checks after a cold editor restart. It compared all prior lizard defaults and placed-instance values, component templates, meshes/materials/collision, asset referencers, and placed gameplay components/transforms against the checkpoint. Only automatically regenerated editor billboard icons were excluded from the placed-component comparison. Existing broom, spray, smoke puff, input, grabbing, and room asset files were unchanged. The lizard, three inherited-member callers, gameplay map, and project descriptor were saved.

An additional Android SDK verification attempt could not complete: the installed engine's Turnkey `VerifySdk` command raised `System.ArgumentNullException` in `GetPlatformsAndDevicesFromCommandLineOrUser`. Its process still returned success, so that exit code is **not** treated as successful SDK validation. Android SDK/NDK installation has not been confirmed. This engine's `Android_SDK.json` specifies NDK r27c (`27.2.12479018`), platform `android-36`, and build tools `36.0.0`. This does not block the completed Windows Editor migration, but Android toolchain validation remains before packaging for Quest. Existing victory Restart button execution was not validated in this migration.

Quest 2 testing remains required for controller grip/release and trigger input, hand occupancy/transfer, catch during daze/stun, drop and bottle deposit reach, broom contact feel, smoke/halo stereo readability, teleportation with tools, and Android frame rate. No Android package, headset deployment, or device performance result is claimed. Broom and spray migration was completed in the subsequent tool pass; its separate verification evidence is in `Tools-Cpp-Verification.json`.
