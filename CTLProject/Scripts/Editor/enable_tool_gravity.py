"""Enable free/drop physics on existing tool roots without simulating visual or overlap children."""
import json
from pathlib import Path
import shutil
import unreal
from editor_toolset.toolsets.blueprint import BlueprintTools as B
from editor_toolset.toolsets.actor import ActorTools as A

names = ['BP_Broom', 'BP_CollectionBottle', 'BP_Spray']
level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert not level.is_in_play_in_editor(), 'Stop PIE before editing tool defaults'
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert world.get_path_name() == '/Game/Maps/Map_CatchTheLizard.Map_CatchTheLizard', 'Open the gameplay map first'
checkpoint = Path(unreal.Paths.project_saved_dir()) / 'GravityCheckpoint'
checkpoint.mkdir(parents=True, exist_ok=True)
project = Path(unreal.Paths.project_dir())
for relative in [f'Content/CatchTheLizard/{name}.uasset' for name in names] + ['Content/Maps/Map_CatchTheLizard.umap']:
    backup = checkpoint / Path(relative).name
    if not backup.exists():
        shutil.copy2(project / relative, backup)

def enable(component):
    component.modify()
    body = component.get_editor_property('body_instance')
    body.set_editor_property('simulate_physics', True)
    body.set_editor_property('enable_gravity', True)
    # Three small moving tools: CCD prevents a fast drop passing through room geometry.
    body.set_editor_property('use_ccd', True)
    component.set_editor_property('body_instance', body)
    component.set_collision_profile_name('PhysicsActor')
    component.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS)

for name in names:
    bp = unreal.load_asset('/Game/CatchTheLizard/' + name)
    roots = [c for c in A.get_components(B.get_default_object(bp)) if c.get_name() == 'GrabCollision_GEN_VARIABLE']
    assert len(roots) == 1 and isinstance(roots[0], unreal.BoxComponent), name
    enable(roots[0])
    B.compile_blueprint(bp)
    assert bp.get_editor_property('status') in [unreal.BlueprintStatus.BS_UP_TO_DATE, unreal.BlueprintStatus.BS_UP_TO_DATE_WITH_WARNINGS], name
    assert unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)

results = []
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if actor.get_class().get_name() not in [n + '_C' for n in names]:
        continue
    assert actor.root_component.get_name() == 'GrabCollision', actor.get_name()
    enable(actor.root_component)
    results.append({'actor': actor.get_actor_label(), 'simulate_physics': actor.root_component.get_editor_property('body_instance').simulate_physics, 'gravity': actor.root_component.get_editor_property('body_instance').enable_gravity})
assert len(results) == 3, results
assert level.save_current_level()
(checkpoint / 'enabled.json').write_text(json.dumps(results, indent=2))
print('TOOL_GRAVITY_SAVED', json.dumps(results))
