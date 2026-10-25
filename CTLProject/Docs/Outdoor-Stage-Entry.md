# Outdoor entry for Level 1 and Level 2

Both gameplay maps start outside, facing the house. Their existing Player Starts,
hologram actors and stage-specific opening dialogue were relocated to the
forecourt. The landing/path were extended and navigation rebuilt. The VR pawn
uses LocalFloor tracking, so the Player Start origin is placed on the outdoor
floor rather than at a character capsule's center height.

The existing Mission Director owns an explicit sequence:

1. **Awaiting Entry**: closed, unlocked door; acknowledge the existing introduction
   and open either handle manually.
2. **Closing After Entry**: the headset crossed the doorway while it was open,
   then reached the safe interior zone. The entry closure waits for clear space.
3. **Inside Locked**: the door closed smoothly and locks until the capture target
   has actually been stored. Locked handles show “Catch all required lizards to
   unlock the exit.”
4. **Exit Unlocked**: all captures are stored; the door stays closed and highlights
   activate. The objective says “All lizards captured! Open the door and head
   outside.” Open it manually, then enter the existing outdoor completion zone.
5. **Complete**: the existing dialogue, sound, star result and Next Mission panel
   run once. Further doorway crossings cannot relock the door.

Hands and held objects never count as entry or exit. They can defer closure,
along with the headset/body clearance and blocking movable objects. The proposed
door-leaf pose is also checked against blocking geometry. Closing pauses when
obstructed and resumes only after the configured clear interval. If the player
backs outdoors before the entry closure commits, it cancels the pending lock
instead of stranding the player outside. No reopening is automatic.

The Level 2 emergency timer still starts when its introduction is acknowledged,
including time spent outside afterward. It is not restarted by entry or locking.
Capture targets, assistance/stars, tools, music and storage behavior are unchanged.

## Editing the setup

Open **Map_CatchTheLizard** or **Map_CatchTheLizard_Level2**:

- **PlayerStart_CatchTheLizard**: move/rotate to edit the outdoor spawn and facing.
  Keep it on a navigable floor outside the swing/clearance volumes.
- **Hologram Instructor**: edit its placed transform. **Mission Director →
  DialoguePanel** controls opening UI placement; Introduction retains the existing
  per-stage dialogue. While awaiting entry, the existing ObjectivePanel uses that
  outdoor position; after entry it returns to its authored indoor position.
- **Mission Director → Mission → Entry**: `Start Outside`, entry/closing objective
  wording and locked-feedback duration. Runtime `Entry State` and `Has Entered
  House` are read-only diagnostics.
- **House Exit / BP_ExitDoor → EntryThreshold**: the doorway crossing plane and
  opening bounds. Positive local X is outdoors; negative X is indoors.
- **SafeInteriorZone**: where the headset must reach before closure is requested.
  Keep its nearest edge safely inside and cover the intended interior destinations
  for players teleporting through the open doorway.
- **ClosingClearance**: cover the threshold and full outward door-swing area.
  Tune `Player Clearance Radius`, `Hand Clearance Radius`, and `Clear Before
  Closing Seconds` under Exit → Entry. Clearance radii account for instance scale.
- **Handle / ExteriorHandle**: edit reachable physical positions. Each associated
  HandleInteraction component exposes Reach Radius and the two controller input
  actions. A fresh index-trigger press activates a nearby handle; either controller
  works. Keep both handles parented to Hinge. `Open Angle` and `Open Speed` tune the
  hinged motion, with collision following the leaf.
- **OutdoorZone** and existing guidance properties still define final completion
  and its marker. Starting in that zone cannot finish the stage without the entry
  and capture states.

The shared door and mission classes gate this behavior on `Start Outside &&
!Tutorial`. The tutorial map is not modified, its external handle is hidden and
inactive, and its original manual exit sequence remains in use. For a new gameplay
map, enable Start Outside on its Mission Director and author the same relative
door volumes and outdoor floor/navigation.

Runtime: `MissionFlow.h/.cpp`, reusing `PhysicalPressInteraction` and the existing
CanTeleportTo guard. Authoring: `Scripts/Editor/setup_outdoor_entry.py`; this
verifies the tutorial map's SHA-256 remains unchanged. Quest 2 playtesting is still
needed for physical reach, room-scale clearance and perceived UI placement.
