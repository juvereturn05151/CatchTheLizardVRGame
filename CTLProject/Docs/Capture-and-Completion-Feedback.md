# Stored lizards and completion feedback

Successful storage now keeps the **original lizard actor** visible inside its
container. No extra gameplay actors or visual copies are spawned. Its existing
stored state still owns all capture/counting guards. The original plush/gecko
skeletal mesh and materials are retained, frozen in the neutral idle pose, with
input, movement, overlap, physics and collision disabled. Gameplay indicators,
spray UI, and the legacy mesh are hidden. The actor attaches to an interior slot
and follows the container. Container destruction cleans up its occupants; map
unloading/restart resets the normal actors and display component.

## Placement and scale

In `/Game/CatchTheLizard/BP_CollectionBottle` select **StoredSlot1**, **StoredSlot2**,
or **StoredSlot3** in Components and move/rotate the arrows in the Blueprint
viewport. Three vertically separated slots support the current three-lizard
mission. The arrows are hidden during gameplay and have no collision.

Select **StoredContents** → **Contents** to edit:

- **Interior Slots**: ordered names of slot components.
- **Stored Model Scale**: default `(0.48, 0.48, 0.48)` for the handheld jar.
- **Stored Model Offset**: visual offset relative to each slot, in centimeters.
- **Overflow Slot Step**: fallback spacing if a future design permits more
  captures than authored display slots. Add proper interior slots when expanding
  a mission; this fallback does not change storage capacity or capture targets.

The tutorial's separate stationary box is
`/Game/CatchTheLizard/Tutorial/BP_TrainingContainer`, with the same component and
slot controls. Its larger box uses stored scale `0.75`. Compile/Save Blueprint
changes and restart the attempt to see defaults applied. Existing pickup and
opposite-hand storage rules are unchanged.

## Container visibility

`/Game/CatchTheLizard/Materials/MI_CollectionBottle_Contents` reuses the existing
glass material, with **GlassOpacity = 0.045**, **GlassEdgeOpacity = 0.16**, and
**GlassRoughness = 0.18**. These parameters and GlassTint are editable in the
material instance. The jar's BottleBody and tutorial box's four walls use it;
their shadows are disabled so they do not obscure the occupants. No refraction,
scene captures, or additional transparency layers were added. The original
material and container collision/deposit geometry remain intact.

## Sound settings

Open `/Game/CatchTheLizard/Audio/SFX/DA_SoundEffects`:

- **Container Capture**: existing `SFX_ContainerCapture`, a short synthetic chime.
  Sound, Volume and Min Interval Seconds remain editable. The single existing
  native successful-storage hook plays spatially at the container; rejected or
  repeated overlaps cannot retrigger it. Distinct accepted lizards each get a cue.
- **Mission Complete**: new `SFX_MissionComplete`, an original **procedurally
  synthesized effect, not a recording**. It is a cheerful 2.4-second major-key
  jingle, imported as mono 48 kHz PCM16 with smooth envelopes and a -11.7 dBFS
  source peak. Volume defaults to 0.7 before the existing SFX master multiplier.
- **Completion Music Duck Multiplier**: defaults to 0.35. Only a successfully
  started completion cue triggers ducking; BGM restores after the cue with a
  short fade. Travel cancels/restores the duck before reusing/fading music.

Mission completion plays the cue once in 2D alongside the existing completion
panel, after the objective and actual outdoor exit are accepted. Merely storing
the last lizard does not play it. The tutorial's former post-storage completion
chime was removed. Dialogue, star calculation and Next Mission are unchanged.

Recreate the jingle with `Scripts/Audio/generate_mission_jingle.py` (standard
Python only); source WAV and measured levels are in `SourceAudio/Feedback`.
`Scripts/Editor/setup_capture_feedback.py` imports/assigns assets, authors slots,
compiles affected Blueprints and saves the three maps. Reruns preserve authored
slot transforms and existing sound/mix edits.

Runtime code lives in `StoredLizardDisplayComponent.*`, the existing
`ALizardGameplayActor::TryStoreInBottle`, `UCTLSoundEffectsSubsystem`,
`UCTLGameInstance`, and `AMissionDirector::CompleteMission`.

## Verification

The Editor target builds and the affected Blueprints/maps are compiled/saved.
Short desktop PIE checks exercise the real catch/storage APIs, unique counting,
distinct slots and container attachment, rejected repeated deposits, and a single
2D completion cue only after manual door opening and actual player-zone entry.
The unassisted Level 2 result still awards three stars. The jar contents are also
checked in an Unreal-rendered preview; tutorial and Level 1 storage preserve the
plush model, and the tutorial's existing overlap handler confirms its deposit.
Container destruction cleaned up its occupant, a fresh Level 2 attempt reset to
zero captures, and stored actors rejected further catch/stun actions. These checks do not establish Quest 2
visibility, perceived audio balance, or headset performance; those remain the
user's playtest. No Android packaging or extensive automated suite was run.
