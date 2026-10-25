# Initial sound effects

The imported `/Game/CatchTheLizard/Sfx/lizard_sound` is a periodic spatial
call for real lizards. `BP_Lizard` → Class Defaults → Lizard → Audio enables it
and assigns the clip; **Lizard Voice Min Interval** (5 seconds), **Max Interval**
(10 seconds), **Volume** and **Attenuation** are editable there. Each native actor
owns its own random stream and one-shot timer. The first delay and every later
delay are independently randomized, measured between playback starts. The existing
SFX master gain still applies. Attached spatial playback follows the lizard and
cannot overlap another call on the same lizard. Stun, stun falling, ordinary drop
falling, holding, storage and inactive challenges cancel the timer and voice;
becoming active again starts a fresh random delay. Destruction/unloading clears
the timer. `BP_MovingToyLizard` and
`BP_TrainingLizardDoll` explicitly disable this voice and clear the sound reference.
Their existing pickup/capture feedback is unchanged. Reapply the assignments with
`Scripts/Editor/setup_real_lizard_voice.py` after compiling the Editor target.

These are original **procedurally synthesized placeholders**, not realistic recordings. No dedicated audio generator was available. Generate the mono 48 kHz PCM16 source WAVs with `Scripts/Audio/generate_placeholder_sfx.py` using Python 3 (standard library only). It uses a fixed random seed and writes WAVs plus measured peak/RMS levels to SourceAudio/SFX. One-shots have softened attacks/releases; the noise-based spray has a circular crossfade and no baked start/end fade. Runtime handles its start/stop envelope. All source peaks are below -11 dBFS, leaving additional headroom before the editable playback gains.

Assets live in `/Game/CatchTheLizard/Audio/SFX`:

| Sound Wave | Actual binding | Space |
| --- | --- | --- |
| SFX_UIHover | Main menu/level selection button OnHovered; tutorial Continue OnHovered | 2D |
| SFX_UIConfirm | Main menu/selection accepted click; tutorial dialogue state advancement | 2D |
| SFX_ObjectPickup | Successful lizard/doll TryCatch; existing tool OnGrabbed signal on broom, collection bottle, spray | 3D at object |
| SFX_BroomSwing | Held broom tip crosses its existing rake-speed threshold; rearms below half the threshold | 3D at tip |
| SFX_SprayLoop | Spray emission enters active state; follows the nozzle | 3D attached |
| SFX_StunActivated | BeginSmokeStun accepted activation, once per stun | 3D at lizard |
| SFX_ContainerCapture | TryStoreInBottle actually accepts a deposit | 3D at container |
| SFX_TutorialComplete | Legacy short chime retained for diagnostics; not triggered on tutorial storage | 2D |
| SFX_MissionComplete | Accepted outdoor exit after the capture objective; once with the completion panel | 2D |

Open **DA_SoundEffects** to replace each Sound reference or change its Volume and Min Interval Seconds. Master Volume defaults to 0.8. Spray Fade In Seconds is 0.08; Spray Fade Out Seconds is 0.12. **ATT_WorldEffects** controls spatialization, the 70 cm inner radius and 900 cm falloff. This uses ordinary lightweight panning/attenuation and does not require an external spatializer plugin.

`BP_CTLGameInstance` → Class Defaults → Sound Effects references DA_SoundEffects. Its existing Music Library property and persistent music playback are unchanged. The SFX folder is explicitly included in cooking and referenced by this asset chain.

Native UCTLSoundEffectsSubsystem provides the shared mix and per-source/effect cooldowns. UI cooldowns are shared between both controller pointers. World sounds retain their native acceptance guards. UCTLPickupAudioComponent listens to the existing BP_GrabComponent signals on the three tool Blueprints, tracks actual held-state transitions and adds no gameplay tick/input bindings. Lizard pickup is bound directly in C++ and has no extra pickup component.

The broom uses its existing motion sample, ignoring pickup/holder changes and pawn teleport/snap-turn samples. It plays one whoosh on threshold crossing, with a default 0.4-second cooldown, rather than every tick/contact. Stun feedback does not play on every spray bubble, and capture feedback does not play on rejected proximity/overlap checks.

SprayAudio is a single non-auto-activating native Audio Component on BP_Spray, attached to its Nozzle. Releasing the trigger, dropping the tool, or gameplay input locking ends emission and fades the hiss out. Rapid re-presses stop the fading voice before restarting that same component; actor destruction/map teardown stops it immediately. Existing puffs remain governed by the original gameplay logic. UI one-shots can finish across immediate menu map transitions and auto-destroy afterward.

The existing BGM and imported weapon/fire effects remain intact. `Scripts/Editor/setup_sound_effects.py` imports the generated files, creates the mix/attenuation assets, adds pickup components and compiles affected assets. Reruns preserve existing library and attenuation edits. Reimport generated WAVs in the editor after regenerating them.

Quest 2 testing and listening determine the final sound quality, mix and spatial falloff. These intentionally stylized placeholders are replaceable through the library; runtime code has no dependency on Python.

Validation: the Win64 Development Editor target builds successfully and all affected Blueprints compiled and saved. A short desktop PIE pass used the real tool grab/drop signals and emission API to confirm the original GrabCollision root, one pickup listener, silence at startup, spatial nozzle attachment, looping playback, single-component reuse, release/stop/restart and drop-stop behavior. The emission API was called directly because desktop preview has no XR trigger device. Source level measurements and results are in SourceAudio/SFX/GenerationReport.json and Saved/SFXCheckpoint. No Android packaging or extensive automated suite was run.
