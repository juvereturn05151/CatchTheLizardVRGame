# VR main menu

Packaged builds start in `/Game/Maps/Map_MainMenu`. The editor still opens the existing gameplay map. Open Map_MainMenu and use VR Preview to try the menu; ordinary PIE also supports mouse pointing and clicking.

## Editable assets

All menu assets live in `/Game/CatchTheLizard/Menu`:

- `WBP_MainMenu` is a real UMG Widget Blueprint derived from `UVRMainMenuWidget`. Edit its Designer tree for the title, layout, fonts, colors, padding and button sizes. StartButton and ExitButton must retain their names and remain variables because native BindWidget properties bind them. StatusText is optional. Button Widget Style contains Normal, Hovered, Pressed and Disabled appearances. Native delegates handle the actions; no Blueprint click graphs are required.
- `BP_MainMenuPawn` derives from `AVRMainMenuPawn`. In Class Defaults, **Main Menu > Travel > Gameplay Map** is the editable soft World reference for Start. It initially points to `/Game/Maps/Map_CatchTheLizard`. If changing it, include the new destination in Project Settings > Packaging > List of maps to include in a packaged build.
- `BP_MainMenuPawn` also exposes panel distance, height offset, tracking settle time, trigger thresholds and ray appearance. Select its inherited MenuPanel component to change Widget Class, Draw Size and scale. Defaults give a 120 x 91 cm panel 180 cm in front of the player's initial view.
- `BP_MainMenuGameMode` selects the menu pawn. Only Map_MainMenu uses this game mode. The gameplay map retains its existing BP_XRGameMode and BP_XRPawn.
- `IMC_MainMenu` contains only the existing left/right index-trigger actions, copied from the project's existing VR hand mappings (Oculus Touch, Valve Index, Vive and PICO). It introduces no grip, movement or gameplay bindings.
- `M_MenuRay` is an unlit material with editable RayColor. Native per-hand instances provide normal, hover and pressed ray colors. The visible rays and hit dots are ordinary low-poly mesh components, including in packaged builds.
- `M_MenuBackdrop` and `M_MenuFloor` provide an unlit background and floor with no realtime lights or effects.

## Runtime behavior

The menu pawn waits for valid headset positional tracking and a short settling period, then places and detaches the world-space panel once using the camera's position and horizontal facing. It does not follow subsequent head movement. Desktop PIE uses a 160 cm camera height and the same menu layout.

Each tracked aim controller has its own Widget Interaction pointer (indices 0 and 1) on the project's existing 3DWidget collision channel. Trigger threshold hysteresis supplies one press/release per pull. Each hand must release to arm; only one pointer can hold a button at once. Tracking loss cancels its captured press. Desktop preview uses a single mouse-driven Widget Interaction pointer; the widget does not also receive hardware clicks.

Start validates the configured map, disables both buttons, and loads the referenced map after Slate finishes routing the click. A missing destination shows an error and allows retry. Exit disables both buttons and calls normal platform QuitGame. In PIE, the editor-only branch requests the end of the play session, never editor shutdown. Loading/quitting is latched against repeated or simultaneous activation.

The menu owns only its input context and removes it when the pawn ends play. Start restores game-only input and hides the desktop cursor. The gameplay map then creates its normal game mode/pawn, whose existing BeginPlay installs its original input contexts. No gameplay behavior or input assets were replaced.

## Project configuration and authoring

`Config/DefaultEngine.ini` sets GameDefaultMap to Map_MainMenu while retaining EditorStartupMap and the global gameplay game mode. `Config/DefaultGame.ini` explicitly includes both maps for cooking. The XRBase plugin/module supports native tracking setup; editor-only UnrealEd use is guarded for packaged builds.

`Scripts/Editor/setup_main_menu.py` creates the original Designer tree, menu-only assets and level inside the existing project after building and restarting the Editor. Rerunning it preserves existing menu layouts/settings rather than rebuilding them. Runtime behavior is entirely C++ in VRMainMenuPawn, VRMainMenuWidget and VRMainMenuGameMode.

## Validation scope

The Win64 Development Editor target built successfully. The menu widget, pawn and game mode, plus the existing XR pawn/game mode, compiled successfully, and all assets were saved. In short desktop PIE sessions, clicking the actual world-space Start button loaded Map_CatchTheLizard with BP_XRPawn and removed IMC_MainMenu. Clicking the actual Exit button ended PIE while leaving Unreal open. These used the menu's single Widget Interaction click route.

The existing gameplay pawn only installs its VR input contexts when an HMD is enabled; they remain absent in ordinary desktop PIE, as before. Physical controller pointing, headset placement, stereo readability and Quest 2 performance require headset testing. No Android build or extensive automated test suite was run. Local build and PIE evidence is in Saved/MainMenu-EditorBuild.log, Saved/MainMenu-AssetVerification.json and Saved/MainMenu-PIEVerification.json.
