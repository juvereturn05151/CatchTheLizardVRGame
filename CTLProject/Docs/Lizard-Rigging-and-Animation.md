# Rigged lizards and movement animation

Both supplied models were opened, rigged, skinned, animated, rendered and exported using **Blender 5.2.2 LTS** (`E:/Blender/blender.exe`). The project uses **Unreal Engine 5.8.2**. This is an implemented visual-layer update to the existing project; no new movement or mission manager replaces the native systems.

## Editable source files and exports

| Model | Editable Blender file | Skeletal mesh FBX | Animation FBXs |
|---|---|---|---|
| Green plush toy | `SourceArt/Lizards/Toy/Toy_Gecko_Rigged.blend` | `SourceArt/Lizards/Toy/Exports/SK_ToyGecko.fbx` | `A_Toy_Idle.fbx`, `A_Toy_Crawl.fbx` in that Exports folder |
| Beige real gecko | `SourceArt/Lizards/Real/Real_Gecko_Rigged.blend` | `SourceArt/Lizards/Real/Exports/SK_RealGecko.fbx` | `A_Real_Idle.fbx`, `A_Real_Crawl.fbx` in that Exports folder |

Original Downloads FBXs and texture folders are preserved. Byte-for-byte copies and SHA-256 manifests are in `SourceArt/Lizards/Originals`. The real FBX contained a dense legacy skin; its unused groups and original armature modifier are removed only from the derived working mesh. The two new rigs use their own proportions, with common naming for convenience, rather than forcing incompatible geometry onto one skeleton.

Each Blender rig has **22 deform bones**: root, pelvis/spine, neck/head, five tail segments and three segments per leg. Four non-deforming IK foot controls remain editable in Blender. Unreal imports 23 bones including the stationary FBX armature carrier; the four controls do not export. Source geometry is reduced only in the derived game meshes: **12,000 triangles for the toy**, **9,999 for the real gecko**, with at most four normalized skin influences per vertex.

Select `Toy_GeckoRig` or `Real_GeckoRig`, then use the Action Editor to choose the named `Toy_Idle` / `Toy_Crawl` or `Real_Idle` / `Real_Crawl` action. The saved timeline opens on the crawl loop. Toy crawl is 0.8 seconds; real crawl is approximately 0.533 seconds. Both neutral idle clips are one second. Diagonal leg pairs alternate through planted stance and raised swing, with restrained spine/bounce and tail movement. Root translation is constant, never animated travel. A fixed 1.5 mm toy / 1 mm real carrier offset provides surface clearance without changing the bone rest shapes or moving the actor. The roots stay stationary throughout every loop.

Units are centimetres (`scene.unit_settings.scale_length = 0.01`), character forward is +X and surface normal is local +Z. FBX export axes are -Y forward / Z up with unit conversion, no leaf bones and no exported control bones. Imported dimensions and bone directions were checked in Unreal: toy length 30 cm; real length 28 cm; heads point along actor +X.

Textures are relinked to the supplied local files and packed in each blend. Original texture sources are preserved. Game materials reuse the supplied base color and normal data, with derived 2K base-color assets and 1K linear roughness/metallic mask packs. Metallic response is reduced for the non-metal plush/skin surfaces. `BodyColor` preserves the existing catch/stun feedback; the new normal tint defaults to white so the original beige/green colors remain visible. Texture packing does not overwrite the original files.

Four textured Blender crawl renders per model are beside each `.blend` as `*_Crawl_Preview_0.png` through `*_Crawl_Preview_3.png`. The source/contact validation report is in `Saved/LizardAnimationCheckpoint/Blender-FinalValidation.json`.

## Unreal assets and Blueprint assignments

All new game assets are under `/Game/CatchTheLizard/Animation`:

- `Toy/SK_ToyGecko`, `Toy/SK_ToyGecko_Skeleton`, `Toy/A_Toy_Idle`, `Toy/A_Toy_Crawl`, `Toy/ABP_ToyGecko`.
- `Real/SK_RealGecko`, `Real/SK_RealGecko_Skeleton`, `Real/A_Real_Idle`, `Real/A_Real_Crawl`, `Real/ABP_RealGecko`.
- `Materials/M_ToyGecko`, `Materials/M_RealGecko`, plus the derived game texture assets in `Textures`.

`BP_TrainingLizardDoll` uses the plush mesh and toy Animation Blueprint; **bTrainingDoll stays true**, so it has no autonomous surface locomotion. `BP_MovingToyLizard` uses the same plush mesh/Animation Blueprint with its existing moving/fleeing behavior. `BP_Lizard` uses the beige mesh and real Animation Blueprint for Level 2.

The existing lizard actor instances remain in `Map_TrainingFacility`, `Map_CatchTheLizard` and `Map_CatchTheLizard_Level2`. Capture targets remain 1 / 1 / 3. Existing mission-lizard references, capture spheres, grabbing, bottle storage, broom effects, spray exposure/stun, status widgets, exit guidance, dialogue, stars, audio and completion travel are retained. The old `LizardBody` component remains hidden for compatibility with existing component references and collision. The new `LizardAnimatedBody` skeletal component has no collision. Old facing debug arrows and the temporary wind-up block are hidden in play.

## Designer controls

Open a lizard Blueprint or select a placed lizard. Under **Lizard → Animation**, edit:

- **Idle Animation** and **Movement Animation**: use clips compatible with that model's skeleton.
- **Animation Movement Threshold**: minimum supported surface speed to blend out of idle (default 1 cm/s).
- **Animation Blend Seconds**: normal start/stop blend duration (default 0.12 s). Held, stored, stunned, falling and disabled-challenge states stop the crawl immediately.
- **Animation Reference Speed**, **Animation Min Play Rate**, **Animation Max Play Rate**: speed-to-playback conversion and limits. Initial reference speeds follow the authored stride/cycle; minimum rate is 0.25 and maximum is 8. High flee speeds can reach this cap, so foot-slip/playback feel at those speeds remains a headset tuning item.

Under **Lizard → Animation → Visual**, edit **Animated Skeletal Mesh**, **Animation Blueprint Class**, **Animated Mesh Rotation**, **Animated Mesh Scale** and **Animated Mesh Contact Offset**. Use these native editable properties for inherited Blueprint variants, rather than modifying the inherited component template in the parent. Actor-local Z is the supporting surface normal. Ordinary surface movement has a 9 cm native stand-off, so the moving variants use contact offset `(0,0,-9)` cm. The tutorial tabletop doll uses `(0,0,0)`. A changed model/scale may need contact offset adjustment.

`ABP_ToyGecko` and `ABP_RealGecko` contain ordinary editable AnimGraphs: two looping Sequence Players feed a pose blend, with native instance values for clip selection, blend and movement playback rate. Root motion is disabled/ignored. The native `ULizardAnimInstance` supplies presentation data; it does not translate or orient the gameplay actor.

`ALizardGameplayActor::SurfaceLocomotionSpeed` measures the actual supported displacement inside `MoveOnSurface`, projected onto the current surface plane. It never uses inter-frame actor velocity. Front/corner snaps, broom relocations, hand motion, teleport displacement, gravity falls and unsupported states do not generate a locomotion-speed spike. The native actor still owns all world movement and surface alignment.

## Verification and remaining headset work

Completed checks:

- Blender: both rigs/actions saved with packed textures; all 25 toy / 17 real crawl frames evaluated. Loop endpoint vertex error is zero; roots remain stationary; all vertices have normalized weights with at most four influences; no sampled world-space vertex crosses the local contact plane. SHA-256 checks confirm original source files are unchanged. Textured poses and contact were visually inspected.
- Unreal: FBX size, axes, skeletons, clips and material playback checked. Both editable Animation Blueprints and affected gameplay Blueprints compiled; all changed maps/assets saved.
- Win64 Development Editor C++ build succeeded. Evidence: `Saved/LizardAnimationCheckpoint/Editor-Build-Final.log`.
- A brief desktop PIE check verified actual skeletal foot/tail animation, supported speed on walls/floors/ceilings, blend/rate data, neutral held/stunned/falling/stored states, real grip/container storage and counting, stationary tutorial behavior after acknowledgement, moving plush behavior in Level 1 and ignored root motion. Evidence: `Saved/LizardAnimationCheckpoint/Unreal-PIE.json`. Scripted controller positions are not physical device testing.

**Quest 2 testing is still pending**: stereo appearance, perceived model size, contact at surface corners, controller reach, rate-cap foot sliding at the fastest inherited flee speeds, and device frame time. No Android packaging, headset performance result or extensive gameplay automation suite is claimed.

Authoring scripts: `Scripts/Blender/rig_and_animate_lizards.py`, `pack_lizard_surface_masks.py`, optional `refine_lizard_loop_contacts.py`, and `validate_lizard_animation_assets.py`; Unreal asset setup is recorded in `Scripts/Editor/setup_lizard_animation.py` with meshes / animations / blueprints / maps phases. Runtime has no Blender or Python dependency.
