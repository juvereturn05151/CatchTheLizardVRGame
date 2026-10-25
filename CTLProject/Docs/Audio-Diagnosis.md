# PC VR audio diagnosis

The confirmed project defect was background-focus muting. The inherited engine
`[Audio] UnfocusedVolumeMultiplier=0` muted the entire mixer when the Unreal
window lost focus, including VR Preview while Link remained connected. Components
still reported playing. The project now overrides this value to `1.0` in
`Config/DefaultEngine.ini`. This preserves the existing mix while the window is
in the background; audio will also continue when switching desktop apps. Restart
the editor after changing this setting. Normal game pause behavior is unchanged.
Editor PIE/VR Preview also has an independent override: Editor Preferences →
Level Editor → Miscellaneous → Sound → **Allow Background Audio**. This was false
and forced application volume to zero even with the project multiplier set to 1.
It is now enabled in this user's editor settings and in
`Config/DefaultEditorPerProjectUserSettings.ini` for fresh project settings.
Existing team members' saved editor preferences take precedence over defaults;
enable this preference on those PCs too.

No sound assets, music assignments, SFX gains, or Windows default devices were
replaced. Actual VR Preview automatically selected **Headphones (Oculus Virtual
Audio Device)**; desktop PIE selected **Speakers (USB Audio B2)**. These are
different expected routes. Do not hardcode this PC's endpoint GUID into shared
project configuration.

## Optional diagnostics

In a running game/editor console:

- `CTLAudioStatus`: log selected output, application/focus and primary volume,
  paused state, music asset/component state, library gains, and listener transform.
- `CTLAudioTest`: play the assigned completion chime in 2D at 0.5 and log the
  requested asset and whether the component started. Repeated tests replace the
  previous diagnostic component. The component is cleaned up on shutdown.

Search the Output Log for `CTL audio`. The music component's volume multiplier
does not include its FadeIn/AdjustVolume fader; the existing `CTL music` log reports
the target map gain.

Editable assignments remain in:

- `/Game/CatchTheLizard/Music/Playback/DA_MapMusic`
- `/Game/CatchTheLizard/Audio/SFX/DA_SoundEffects`
- `/Game/CatchTheLizard/Audio/SFX/ATT_WorldEffects`
- `/Game/CatchTheLizard/Music/Playback/BP_CTLGameInstance`

`Scripts/Editor/check_audio_assignments.py` audits these assignments and compiles
and saves the GameInstance Blueprint. Diagnostic captures/reports are local to
`Saved/AudioDiagnosis`; they are mixer recordings, not recordings made by the
headset speakers.

## Checks completed on this PC

- Built CTLProjectEditor Win64 Development successfully and compiled/saved
  BP_CTLGameInstance; saved dirty project content.
- Audited both music tracks and all eight SFX assignments, duration, looping,
  sound volume, class reference, library gain and world attenuation.
- Recorded nonzero BGM after travel through Main Menu, Level Selection,
  Tutorial, Level 1, Level 2, and back to Main Menu. The same music component
  survived Menu → Level Selection and Level 1 → Level 2 as intended.
- Invoked seven one-shot effects through the existing subsystem with BGM
  temporarily faded out. All started and produced nonzero mixer recordings;
  nearby world effects retained spatial playback and UI effects remained 2D.
- Actual OpenXR Oculus VR Preview selected the Oculus output. With focus moved
  to Link, the original mixer recording was completely zero. After both settings
  were fixed and persisted through editor restart, the same background VR check
  had peak 0.1083 and RMS 0.01959 (normalized PCM), with application/primary volume
  1 and `au.DisableAppVolume=0`. No debug mute bypass is left enabled.
- VR listener and headset-camera position matched at the observed pose. Physical
  head movement and perceived stereo positioning remain headset checks.
- Scripted calls to the existing grab/release and emission functions verified
  spray start, trigger-release fade/stop, restart and drop stop with one audio
  component. The emergency delay was overridden only on the temporary PIE actor;
  the saved map still uses 120 seconds. Controller trigger hardware was not tested.
- Confirmed the diagnostic console commands work; repeated test calls replace
  the previous diagnostic voice.

These checks establish generation, routing and state changes, not audible output
from the physical headset speakers. No Android packaging was performed.

## Headset confirmation

Launch Quest Link/Air Link, wear the headset, and use actual **VR Preview**. Check
menu music and a UI click, then a nearby pickup/capture. Switch desktop focus away
from Unreal and confirm sound continues. In Level 2, confirm spray sound starts
only while held and triggered, and stops on release/drop. Confirm music through
level transitions and Return to Menu.

If the headset remains silent, use Meta Horizon Link → Devices → Quest 2 → Audio:
the output should read **Quest 2 Headphones**, with its speaker icon unmuted and
slider above zero. Check the physical headset volume and Windows Volume Mixer for
the running Unreal preview on **Headphones (Oculus Virtual Audio Device)**. At
inspection the Link output was selected/unmuted, Windows endpoint volume was
approximately 74%, and Unreal's session was unmuted at 100%. Headset speaker
audibility still requires the user's physical listening check.
