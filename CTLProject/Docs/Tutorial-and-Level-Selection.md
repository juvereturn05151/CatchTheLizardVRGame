# Tutorial and level selection

Packaged flow: **Map_MainMenu → Start → Map_LevelSelection → Tutorial or Level 1**. Back returns to the main menu. Both entries are available immediately. The tutorial's final **Begin Level 1** button fades the view and loads the existing Map_CatchTheLizard directly. Exit on the original main menu retains its normal packaged quit / safe PIE-end behavior.

## Maps and editable assets

- `/Game/Maps/Map_LevelSelection` uses BP_LevelSelectionGameMode and BP_LevelSelectionPawn, which reuse the native main-menu pawn and its two controller pointers. `/Game/Maps/Map_TrainingFacility` uses the original BP_XRGameMode and BP_XRPawn with their existing grab/input systems. Level 1 is the original `/Game/Maps/Map_CatchTheLizard`; no duplicate gameplay stage was made.
- New assets live in `/Game/CatchTheLizard/Tutorial`. `DA_LevelList` is the shared editable list of IDs, display names and soft World references. Keep the IDs **Tutorial** and **Level1** when changing their labels or maps. MainMenuMap and SelectionMap configure navigation. BP_MainMenuPawn, BP_LevelSelectionPawn and BP_TrainingDirector reference the same list. If changing a map, include it in Project Settings → Packaging → List of maps to include in a packaged build.
- `WBP_LevelSelection`, `WBP_TutorialDialogue` and `WBP_TutorialObjective` are real UMG Widget Blueprints. Their Designer trees own fonts, sizes, layout, colors and button feedback. Preserve the named bound widgets: TutorialButton, LevelOneButton, BackButton and their TutorialLabel/LevelOneLabel; DialogueText, ActionLabel and ContinueButton; ObjectiveText, ControlsText and StatusText. Visual labels not bound to native properties can be freely renamed.
- Edit all tutorial dialogue, the persistent objective, controller instructions and button/status text in **BP_TrainingDirector → Class Defaults → Tutorial → Dialogue / Text**. Placed director instances can override them. No C++ edits are needed. Adjust its DialoguePanel and ObjectivePanel components to move/resize the stationary world-space UI. The panels sit above and to either side of the working table rather than in the player's hand space.
- The training level owns the room layout and explicit references from its Tutorial Director to the doll, container, broom and three highlights. Move these actors in the level as needed. BP_TrainingInstructor is a stationary geometric mannequin; replace its visual components with an NPC model later. BP_Table is reused. BP_TrainingContainer is an open, stationary training receptacle with a named DepositVolume. BP_TrainingLizardDoll inherits the existing BP_Lizard visual/catch setup and uses its original green plush material.

## Progression and capture confirmation

ATrainingDirector owns explicit states: Welcome, Instructions, Capture, Success, BroomExplanation, Finish and Traveling. Every new tutorial world starts at Welcome. Continue advances the dialogue; Capture has no skip button. The broom step is informational and has no challenge or use counter.

BP_TrainingLizardDoll is a child of the existing BP_Lizard, so the pawn's existing class-based hand-occupancy checks recognize it. Its placed mesh uses the original green plush material. ALizardGameplayActor has a training-doll flag. Only this variant suppresses wandering, fleeing, broom reactions and spray reactions. Its original hand-overlap/grip capture and dropped-motion code remain active. The doll cannot be picked up until capture practice starts. A periodic check against configured recovery bounds resets an uncaught, unstored doll to its original table transform if it leaves the room. Ordinary drops remain retryable.

Storage still goes through **ALizardGameplayActor::TryStoreInBottle**. A training-only branch permits the explicitly assigned stationary training container instead of requiring the normal opposite-hand held bottle. The doll must still be caught and attached to a real player hand, its catch component must overlap DepositVolume, and its center must be within 70% of that volume's sphere radius. A nearby doll, edge contact, a dropped doll or another container cannot complete the objective. The normal gameplay bottle rules are unchanged.

The container overlap and the doll's caught event start a short local check only while the known doll overlaps the known container. The director advances exclusively from **OnStoredInContainer**, and checks the exact doll/container references and terminal bStored flag. It does not count contacts, independently set bStored, or repeatedly search the world for actors. The original collection victory manager is absent from the tutorial, so it cannot trigger a victory overlay or lock the pawn mid-lesson.

On confirmed storage, the objective is marked complete and the doll/container cues disappear. The broom cue appears only during its explanation. The adjusted wording reflects the actual held-tip threat/flee behavior, fast-contact relocation and brief daze, plus grip pickup/release. No spray training is included.

## VR input and implementation

UVRMainMenuWidget/AVRMainMenuPawn handle the selection buttons using the existing menu input path. UVRPanelInteractor reuses the gameplay pawn's existing WidgetInteractionLeft/Right components on the project's 3DWidget trace channel. It uses the same index-trigger thresholds, separate pointer indices and visible mesh rays/hit dots as the menu. Grip bindings remain available to the gameplay pawn and doll. No duplicate hardware click path or victory-menu actor is added.

Every dialogue transition cancels pointer capture, requires release to rearm, and applies a short native advancement cooldown. Only one pointer may hold a UI button at once. Desktop PIE supports mouse dialogue/selection clicks through the same Widget Interaction route; the tutorial sets a desktop camera height solely for preview. Desktop hand controls are not introduced.

New native classes: UCTLLevelList/FCTLLevelEntry, ATrainingDirector, UTutorialDialogueWidget, UTutorialObjectiveWidget and UVRPanelInteractor. Existing AVRMainMenuPawn/UVRMainMenuWidget gain selection navigation; ALizardGameplayActor gains opt-in doll behavior and capture/storage events. Room visuals and references remain assets/level data. Python in Scripts/Editor is editor authoring only, not runtime logic.

## Packaging and manual checks

GameDefaultMap remains Map_MainMenu. Packaging includes MainMenu, LevelSelection, TrainingFacility and the original gameplay map. There are no unlock requirements or persistent tutorial saves.

Quest 2 playtesting is still required for reach to the table, stereo readability, pointer aiming, grip behavior, dropped-doll recovery and the fade/transition. Art and instructor animation are placeholders; voice acting is intentionally absent. No Android packaging or extensive automated test suite is part of this change.

The Win64 Development Editor target builds successfully. A short desktop PIE integration pass confirmed the welcome/capture gates and duplicate-Continue guard; out-of-bounds recovery; existing grip pickup, occupied-hand protection, drop and re-catch; rejection of uncaught/nearby dolls; the real storage callback; the informational broom step; and final loading of Level 1 with BP_XRPawn. Native Start → selection, Back → main menu, immediate Level 1 selection, and the unchanged main-menu Exit binding were also checked. Evidence is in Saved/TutorialCheckpoint/EditorBuild.log, AssetVerification.json and PIESmoke.json. The pre-existing GameFeatureData Asset Manager startup warning is unrelated and remains outside this feature.
