# Broom and container palm grips

Open **BP_Broom** or **BP_CollectionBottle** in the Content Browser under
`/Game/CatchTheLizard`. In Components select **GripPointLeft** or
**GripPointRight**. Move and rotate the arrow in the Blueprint viewport, then
Compile and Save. Its origin is the point on the tool that meets the palm; its
orientation defines how the tool sits in that hand. Each hand can be tuned
independently. Units are centimeters and degrees.

- For the broom, position the arrow on the handle where the fist should close.
  The initial grip is at the existing handle/root position.
- For the container, position the arrow on the outside of the jar body. The
  initial grip is at X=-10, Y=0, Z=4.7, placing the jar beside the palm rather
  than embedding its center in the hand. Rotate the arrow to adjust jar tilt.
- Moving the grip arrow toward the tool's tip moves the held tool in the
  opposite direction so that the new point still meets the palm.

Leave **GrabComponent → Grab Type** at **Free** for these two tools. The optional
native **PalmGrip** component supplies their snap after XRFramework accepts the
grab. It exposes the grip-marker and palm-target component names; normally leave
those names unchanged. **Align To Palm** can reapply the authored alignment to
an already held tool. Do not move the visual mesh, physics root or deposit volume
just to correct a hand offset.

If *both* tools are consistently offset from a hand, open
`/Game/XRFramework/Blueprints/BP_XRPawn` and adjust **PalmGripLeft** or
**PalmGripRight** under the corresponding **MotionController…Grip** component.
Those anchors are shared by the gameplay/tutorial pawns. Their initial positions
come from the palm centers of the project's current MannyXR meshes. Their axes
initially match the tracked Grip pose. Do not move the MotionController itself or
the hand mesh for tool-specific corrections.

The arrows are editor markers, hidden during gameplay, with no collision or
teleport interaction. PalmGrip has no tick, timer or new input bindings. It
listens to the existing OnGrabbed event, aligns the existing root once, and
retains controller attachment, root scale, collision children, pickup audio,
broom contacts and container mouth/storage detection. Release uses the existing
Free grab path, keeping world pose and restoring the original drop physics.
Other grabbable objects and the shared BP_GrabComponent are unchanged.

Change defaults outside Play mode, Compile/Save, then start a fresh attempt to
check each hand. If a placed actor has an overridden component transform, reset
that override to inherit the Blueprint default. Headset playtesting is required
to judge hand comfort and the final angle; numerical alignment alone cannot
establish that the grip feels natural.

Authoring script: `Scripts/Editor/setup_palm_grips.py`. Reruns preserve authored
marker transforms. Runtime: `Source/CTLProject/ToolPalmGripComponent.*`.

Verified: CTLProjectEditor Win64 Development build passed, affected Blueprints
compiled/saved, and desktop PIE exercised both tools with both hands, rotated
controllers and repeated grab/release. Grip frames matched the palm frames,
release retained world position, and root scale and rake/deposit child offsets
were preserved. An additionally rotated grip marker and nonuniform tool scale
also aligned, with container drop physics restored. These were scripted calls
through the existing grab/release functions, not headset comfort tests.
