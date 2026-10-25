# Quest Link audio and headset loading-screen diagnosis

## Audio finding and fix

On 24 October 2026 the VR Preview log reported `Speakers (Realtek(R) Audio)`
as Unreal's output, while Oculus was only Windows' communications default.
Menu BGM was playing, application/primary volume were 1, and a non-spatial
`SFX_TutorialComplete` probe started successfully at volume 0.5. A manual Unreal
device swap to `Headphones (Oculus Virtual Audio Device)` succeeded, and the
subsequent output query confirmed that device. A mixer recording contained
nonzero audio. This verifies generation and device routing, not sound heard
through the headset; the user was not wearing it during the probe.

`BP_CTLGameInstance` (Content/CatchTheLizard/Music/Playback) now inherits these
editable Class Defaults under **Audio → Quest Link**:

- **Prefer Quest Link Audio**: automatically route this game's mixer to Quest
  headphones on Windows when an Oculus/Quest HMD is actually enabled.
- **Quest Link Output Name**: match the output name, initially
  `Oculus Virtual Audio Device`. Change this if the endpoint is renamed.

The check runs at startup and after map loading. It does not change Windows
defaults, desktop PIE, standalone Android audio, music/SFX gains or assignments.
If the endpoint is absent it keeps the current device and logs a warning.
Disable the preference if intentionally monitoring through PC speakers.
`CTLAudioStatus` logs output, listener, focus, volumes and BGM.
`CTLAudioTest` plays the existing non-spatial test chime at volume 0.5.

## Black headset with location/arrow icon

This project contained a concrete cause matching the reported symptom. The
inherited VR Template `WBP_Menu` Restart button displayed an XR loading splash
using `/Engine/EditorResources/AI/S_NavLink`, a location pin and curved return
arrow. After a three-second delay it tried to load the missing `VRTemplateMap`.
The log recorded that failed travel at 14:39:12 UTC and fallback to Main Menu.
Only the gameplay XR pawn hid the splash; the C++ menu pawn did not. XR splashes
replace headset rendering while the desktop mirror can continue to render.
The exported icon is in `Saved/VRDiagnosis/S_NavLink.png` for comparison.

The Restart button now calls native `RestartCurrentVRStage`: validate and reload
the actual current map, suppress duplicate requests, and use no permanent XR
splash. Existing GameInstance startup/map-load handling hides leftover splashes;
travel failure also hides/clears them and removes a camera fade. The old missing
map, icon and delay are removed from that restart chain. Gameplay/progression,
music, grabbing and controller bindings are retained.

## Input/camera/configuration inspection

Read-only Blueprint/input snapshot: `Saved/VRDiagnosis/Inspection.json`, recreated
with `Scripts/Editor/inspect_vr_black_screen.py` in the Editor.

| Quest control | Existing gameplay binding |
| --- | --- |
| Left Menu or Y | `IA_Menu_Toggle_Left`: opens/closes the in-game template menu |
| Left stick X | `IA_Turn`: snap turning |
| Right stick Y | `IA_Move`: teleport aim; release attempts teleport |
| Left/right grip | Pressed/released grab actions |
| Left/right trigger | Hand index curl; existing UI/spray/physical-press interactions |
| Left/right sticks with template menu open | Menu cursor actions |
| X and stick clicks | No gameplay menu/teleport bindings found |

The menu includes explicit Reset Orientation, Restart and Passthrough buttons;
the Menu/Y press itself does not switch cameras, restart, or toggle passthrough.
No teleport camera fade was found. Teleport retains projectile/nav projection
and the existing closed/locked-door guard. No teleportation system was removed.

Inspected: `BP_XRPawn`, `BP_LizardVRPawn`, `BP_XRGameMode`,
`BP_TeleportVisualizer`, `BP_Menu`, `WBP_Menu`, `BP_Passthrough`, all IMC assets,
`BP_MainMenuPawn`, `BP_LevelSelectionPawn`, `VRMainMenuPawn.cpp`, `MissionFlow.cpp`,
`TrainingDirector.cpp`, the project descriptor and DefaultEngine/Game/Input.ini.
The live main-menu session had the expected possessed pawn and view target,
active Camera attached to VROrigin, Lock to HMD enabled, and valid Quest 2 tracking.
Gameplay uses the existing map GameMode/pawn. Start in VR is already enabled.
OpenXR/XRBase, OpenXR hands/eye support and PICOController are enabled; no Meta XR
plugin is explicitly configured in the project. No XR plugin was changed.
The registered Windows runtime was Oculus at
`D:/Oculus/Support/oculus-runtime/oculus_openxr_64.json`. An earlier session also
logged `xrCreateSession: XR_ERROR_INITIALIZATION_FAILED`; that is a separate
Link/runtime startup failure, not evidence that a game input caused it.

## Opt-in VR diagnostics

In a Development/Editor session, enter `CTLVRDebug 1`. Disable with
`CTLVRDebug 0`; `CTLVRStatus` prints one snapshot. Filter Output Log for
`LogCTLVR`. This passively logs controller buttons/axis threshold changes,
active input mappings, pawn/view target/camera changes, teleport state/marker,
large pawn relocations, camera fade, XR splash, tracking/focus/worn state and
pause state. State snapshots repeat every five seconds while enabled. It does
not consume input or change the camera. Diagnostics are off by default and
their implementation is excluded from Shipping/Test builds. System-reserved
Quest inputs may never reach Unreal and cannot be logged by the game.

## Headset check

1. Connect Link/Air Link and confirm the headset is connected in the PC Link app.
   Start VR Preview. Run `CTLAudioStatus` and `CTLAudioTest`: output should be
   Oculus Virtual Audio Device and the chime/music should be audible.
2. Test Menu/Y open/close, X, grip/trigger, left snap turn and right-stick
   teleport. The same gameplay pawn/view should remain active. Restart from the
   template menu: the current stage should restart without the location icon.
3. Test main menu → selection → tutorial/Level 1/Level 2. BGM should persist or
   change according to its existing assignments; gameplay SFX remain spatial.
4. If output is Oculus but still silent, check headset volume and the Link PC
   app's device audio setting. In Windows Volume Mixer, ensure Unreal and the
   Oculus headphones endpoint are not muted. We do not override OS mute/volume.
5. If the icon is different, or a Boundary prompt persists outside the game,
   return physically to the safe play area. Use the right Meta/Oculus button →
   Quick controls → Settings → Environment setup → Boundary → Adjust boundary
   (older Quest firmware may call this Physical Space/Guardian). Redraw and set
   floor height if needed, then return to Link and the running app.
6. If VR Preview fails to start, stop Preview, reconnect Link and retry. In the
   PC Meta Link app, Settings → General → OpenXR Runtime should show Meta active.
   Restart the Editor after changing runtimes; avoid running a second VR app.

Official references: [Meta boundary setup](https://www.meta.com/help/quest/463504908043519/)
and [Meta Link development/runtime setup](https://developers.meta.com/vr/documentation/unity/unity-link/).
Menu names can differ by headset/Link software version. No Boundary or runtime
settings are changed by the game's fix.
