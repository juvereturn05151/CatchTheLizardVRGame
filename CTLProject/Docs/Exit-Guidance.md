# Exit guidance

Tutorial, Level 1 and Level 2 reuse `BP_ExitDoor`, its `OutdoorZone`, and the existing mission objective widget. After capture, the door and handle gently glow, the overhead EXIT marker appears, and a pulsing ground ring labelled “Exit Here” identifies the actual completion destination.

The objective reads “Open the highlighted door and head outside.” Below the door’s passable opening angle, this instruction remains in effect. At or above that angle it reads “Head outside to finish the mission.” Closing the door restores the first instruction. A door already open at the last capture immediately uses the outside instruction. The handle’s existing explicit press interaction now toggles opening/closing; it never opens on approach or objective completion.

The ring is derived from the assigned door’s `OutdoorZone`. A one-time floor trace positions it above the real landing; offsets and diameter are clamped inside that zone. It stays visible through opening and closing. Only the existing tracked-head exit check can finish the stage, whether the player walks or teleports there. Completion immediately hides the ring, destination label and exit marker, restores door/handle materials, and leaves the completion dialogue, star result and Next Mission panel in control. Restarting or traveling resets the actor state.

## Editing

Open `/Game/CatchTheLizard/Missions/BP_ExitDoor`, or select **Training Exit** / **House Exit** in a stage. Under **Exit → Guidance**, adjust:

- **Highlight Color**, **Guidance Pulse Speed** (Hz), **Highlight Material**, **Ring Material**.
- **Marker Diameter**, **Marker Ground Clearance**, and **Destination Offset** (door-zone local X/Y, centimetres). Move/resize **OutdoorZone** to change the true completion destination; markers follow its placement on the next attempt.
- **Destination Text** (“Exit Here”), **Destination Text Offset**, **Door Marker Text** (“EXIT”), **Door Marker Offset**, and **Guidance Text Size**.
- **Passable Angle**, plus the existing **Open Angle** and **Open Speed**, control the doorway’s open/closed guidance threshold and motion.

The placed mission director’s **Exit Objective** and **Head Outside Objective** contain the editable two instruction strings. `WBP_MissionObjective` controls their layout. Keep the director’s **Exit Door** reference and the door’s **Mission** reference paired.

`DestinationRing` and `DestinationLabel` are ordinary world-space mesh/text components. `M_ExitGuidanceGlow` is a shared unlit material; `M_ExitGuidanceRing` uses a masked ring on one two-triangle plane. There are no dynamic lights, screen-space outlines, particles, decals or additional guidance timers. Visual components have no collision, overlaps, shadows, or navigation influence, so they cannot intercept movement, grabs, or teleport targeting. Each active stage updates only its own door at the existing 20 Hz door tick.

Runtime changes: `Source/CTLProject/MissionFlow.h/.cpp`. Shared asset/map authoring is recorded in `Scripts/Editor/setup_exit_guidance.py`. Only the shared mission/door assets and the three assigned stage maps are configured. Editor build and check evidence is under `Saved/MissionCheckpoint/ExitGuidance-*`. Quest 2 stereo/controller testing remains for the user; no Android packaging or extensive automated suite is run.

Validated: Win64 Development Editor build succeeded, affected Blueprints compiled without warnings, and all three maps were saved. Floor/nav checks confirm each destination lies inside its own completion zone on reachable ground. A brief desktop runtime pass verified capture gating, closed/open/closed objectives, already-open doors, marker collision settings, material restoration, completion cleanup, restart reset, and preserved completion/rating UI. Rendered views were inspected; the text uses an unlit font material for outdoor readability.
