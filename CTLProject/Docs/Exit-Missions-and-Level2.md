# Exit missions and Level 2

> Updated flow: see [Manual-Progression.md](Manual-Progression.md). The notes below describe the earlier open-door mission implementation and are retained as historical context.

Level 1 remains `/Game/Maps/Map_CatchTheLizard`, retaining its existing one-lizard target. Level 2 is `/Game/Maps/Map_CatchTheLizard_Level2`, a separately saved copy of the house with exactly three mission lizards at distinct reachable wall positions, the existing jar, broom and spray. The jar has no one-lizard capacity limit: accepted deposits hide and disable each lizard permanently for that play session, so one jar securely holds all three.

Both houses have an opening in the small side room's outer wall at X=730, a reusable BP_ExitDoor, a flush outdoor landing/path and expanded navigation bounds. The shared inner doorway is preserved. Geometry/navigation builds are part of authoring; Quest 2 still needs physical passage/teleport playtesting.

## Editable assets and settings

- Open either gameplay map and select **Mission Director**. Adjust **Capture Target**, **Mission Lizards**, **Containers**, **Mission Tools**, **Collection Manager** and **Exit Door** in its Details panel. Capture Objective can override the automatically generated "Catch N lizards" text. Exit Objective and Early Exit Message are editable text properties.
- **Next Mission** and **Main Menu** on the placed director are soft World references. Level 1 points to Level 2. Level 2 has no Next Mission, so its button is hidden. Add new destinations to Packaging's map list.
- Select **House Exit** to move the entire doorway. Edit BP_ExitDoor's mesh components to replace its open placeholder door/frame. **OutdoorZone** is the editable completion box, positioned beyond the threshold. Its collision is deliberately disabled: the native door tests the player's current camera/head position against its bounds, ignoring controllers, held objects and actor-overlap noise. Move/resize the box to match a changed outside layout. Keep it beyond the doorway and above the outdoor floor.
- New UMG assets are **WBP_MissionComplete** and **WBP_MissionObjective**, under `/Game/CatchTheLizard/Missions`. Fonts, layout, colors, solid backgrounds and button sizes are editable in Designer. Preserve the named bound widgets (CountsText, NextMissionButton, MenuButton; ObjectiveText, ProgressText, HintText). The placed director's ObjectivePanel sets the stationary in-house objective location. CompletionPanel dimensions/scale and Completion Panel Distance configure the large completion display.
- Level-selection names/map references remain in `/Game/CatchTheLizard/Tutorial/DA_LevelList` (Tutorial, Level1, Level2 IDs). WBP_LevelSelection now includes LevelTwoButton/LevelTwoLabel. BP_LevelSelectionPawn has a taller panel to accommodate all entries. Tutorial still leads directly to Level 1.
- `/Game/CatchTheLizard/Music/Playback/DA_MapMusic` includes Level 2 with the existing GameBGM_Loop and Gameplay continuity group. Sound effects and music assets are preserved.

## Runtime logic

AMissionDirector binds each explicitly assigned ALizardGameplayActor's OnStoredInContainer event. It accepts credit only for a registered non-training lizard whose real storage operation set bStored, with a registered container. A native set of unique actor references prevents repeated credit. Grabbing, stunning and touching the bottle do not count. Progress resets when the world is entered again.

Before the target is reached, entering the outdoor zone displays "Catch the remaining lizards before leaving." in a readable panel near the current view. At the target, the objective becomes "Return to the exit and leave the house." and the door frame/sign turn green. Native door checks sample only the cached headset/camera position, including its room-scale offset and pawn teleport transform. Eligibility is checked on zone entry/exit and on successful captures, so satisfying the target while already outside also completes correctly.

The original BP_LizardCollectionManager still mediates bottle deposits and maintains its legacy bookkeeping. Its **bRequireExit** instance flag is true in both gameplay maps; a small guard bypasses only its automatic CompleteVictory path. The flag defaults false for other uses. For a new mission using this system, enable that flag on its manager as well as assigning the director references.

On completion, the director latches completion once, stops registered mission actors and spray emission, blocks normal pawn/tool input, and disables the legacy victory UI/input path. Tool physics/collision also stop so a held jar or broom cannot intercept either UI ray. It never pauses the game or disables XR camera tracking. A stationary high-contrast world panel is placed in front of the current headset view without rotating the camera. UVRPanelInteractor reuses the pawn's two pointers and index triggers, with visible rays, release-to-arm protection and a short cooldown. Buttons lock while traveling. Existing positive completion sound feedback is reused.

Next Mission and Return to Menu use editable map references, camera fades and the existing persistent music transition manager. New mission entry fades in. MainMenu remains the packaged startup map; Level 2 is explicitly included in cooking.

Native additions: AMissionDirector, AMissionExitDoor, UMissionCompleteWidget and UMissionObjectiveWidget in MissionFlow.h/.cpp. VRMainMenuWidget adds the optional Level 2 button. VRPanelInteractor activates the existing pointer components when displaying the completion panel. Scripts/Editor/setup_exit_missions.py records the assets/level authoring phases and the legacy guard insertion; runtime has no Python dependency.

## Validation and remaining playtesting

The Win64 Development Editor target builds successfully. Affected Blueprints compile and all maps/assets are saved. Both maps have saved navigation through the inner door, outer door, landing and path, checked by navigation projection and collision traces. Preview static lighting was rebuilt for both maps.

A short desktop integration pass exercised the existing grab/container storage path: grabbing and stunning do not count, repeated storage/events do not inflate progress, leaving early does not win, controller/object-only presence is ignored, tracked-head and teleport-position exit checks work, satisfying the target while already outside completes, one jar securely stores all three, and Level 1 → Next → Level 2 → Menu works. Level 2 is selectable immediately and replay resets it to 0/3. The completion panel was inspected in a rendered preview. Results are in Saved/MissionCheckpoint.

Quest 2 still needs playtesting for actual physical passage, teleport aiming, stereo readability, both controller pointers and the final audio mix. The new door/exterior are simple editable placeholders. No required asset assignments remain. No Android packaging or extensive automated suite was run. The pre-existing GameFeatureData Asset Manager startup warning is unrelated to missions and remains unchanged.
