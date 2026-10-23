# Interactable tool gravity

BP_Broom, BP_CollectionBottle and BP_Spray now simulate physics with gravity on their GrabCollision root components, both in Blueprint defaults and the existing gameplay level instances. CCD is enabled on these three bodies to reduce fast-drop tunneling. Visual meshes and gameplay overlap volumes remain attached children without independent simulation.

The existing BP_GrabComponent caches the root's initial physics state, disables simulation on grab and restores it on release. No replacement grab logic or gameplay C++ changes are needed.

To edit: open the relevant Blueprint, select GrabCollision, and adjust Physics > Simulate Physics / Enable Gravity. New instances inherit these defaults. Objects fall when Play or Simulate runs; the editor's normal placement view remains stationary.

The three affected Blueprints compiled and the gameplay level was saved. A short desktop PIE check confirmed initial falling and floor collision for all three tools, physics paused while grabbed, and falling resumed after release. Local evidence and pre-change asset backups are in Saved/GravityCheckpoint. Scripts/Editor/enable_tool_gravity.py applies the same settings to the existing project assets.
