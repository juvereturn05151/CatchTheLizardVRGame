"""Migrate existing interaction assets; run after building CTLProjectEditor."""
import json
import shutil
from pathlib import Path
import unreal
from editor_toolset.toolsets.blueprint import BlueprintTools as B
from editor_toolset.toolsets.actor import ActorTools as A

assets = unreal.EditorAssetLibrary
level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
# This migration is run as a Python commandlet, where no PIE session exists.
project = Path(unreal.Paths.project_dir())
checkpoint = Path(unreal.Paths.project_saved_dir()) / 'GameplayFixValidation'
checkpoint.mkdir(exist_ok=True)

def backup(path, extension='.uasset'):
    source = project / ('Content/' + path.removeprefix('/Game/') + extension)
    target = checkpoint / 'Before' / source.relative_to(project)
    target.parent.mkdir(parents=True, exist_ok=True)
    if not target.exists():
        shutil.copy2(source, target)

def save(bp):
    B.compile_blueprint(bp, warnings_as_errors=True)
    assert bp.get_editor_property('status') == unreal.BlueprintStatus.BS_UP_TO_DATE
    assert assets.save_loaded_asset(bp, only_if_is_dirty=False)

def component(bp, name):
    return next(c for c in A.get_components(B.get_default_object(bp))
                if c.get_name().removesuffix('_GEN_VARIABLE') == name)

def connect(source, target):
    assert source.try_create_connection(target), (source, target)

# The manager still owns registration, AddUnique, counting and mission reporting.
bottle_path = '/Game/CatchTheLizard/BP_CollectionBottle'
unreal.log('VR_FIX: migrate bottle graph and capture region')
backup(bottle_path)
bottle = unreal.load_asset(bottle_path)
graph = B.get_graph(bottle, 'UpdateDeposits')
ge = unreal.BlueprintGraphEditor.get_graph_editor(graph)
if assets.get_metadata_tag(bottle, 'UnheldContainerDeposits') != '1':
    held = next(n for n in ge.list_all_nodes() if n.get_node_title() == 'Get bIsHeld')
    gate = held.find_output_pin('bIsHeld').list_connected_pins()[0].get_owning_node()
    incoming = list(gate.find_input_pin('execute').list_connected_pins())
    outgoing = list(gate.find_output_pin('then').list_connected_pins())
    assert len(incoming) == len(outgoing) == 1
    gate.find_input_pin('execute').break_pin_links()
    gate.find_output_pin('then').break_pin_links()
    connect(incoming[0], outgoing[0])
    B.delete_node(gate)
    B.delete_node(held)
    assets.set_metadata_tag(bottle, 'UnheldContainerDeposits', '1')

# Previously this radius-16 sphere was centered above the jar at Z=21.
# Keep capture inside the transparent body, away from its exterior/neck.
mouth = component(bottle, 'DepositVolume')
mouth.set_relative_location(unreal.Vector(0, 0, 0), False, False)
mouth.set_sphere_radius(8.)
save(bottle)

pawn_path = '/Game/XRFramework/Blueprints/BP_XRPawn'
unreal.log('VR_FIX: migrate teleport preview')
backup(pawn_path)
pawn = unreal.load_asset(pawn_path)
graph = B.get_graph(pawn, 'IsValidTeleportLocation')
ge = unreal.BlueprintGraphEditor.get_graph_editor(graph)
if assets.get_metadata_tag(pawn, 'DoorRoutePreview') != '1':
    entry = next(n for n in ge.list_all_nodes() if n.get_class().get_name() == 'K2Node_FunctionEntry')
    result = next(n for n in ge.list_all_nodes() if n.get_class().get_name() == 'K2Node_FunctionResult')
    nav_valid = result.find_input_pin('ReturnValue').list_connected_pins()[0]
    projected = result.find_input_pin('ProjectedLocation').list_connected_pins()[0]
    get_pawn = ge.add_call_function_node('/Script/Engine.GameplayStatics.GetPlayerPawn')
    preview = ge.add_call_function_node('/Script/CTLProject.MissionDirector.UpdateTeleportPreview')
    connect(get_pawn.find_output_pin('ReturnValue'), preview.find_input_pin('Pawn'))
    connect(projected, preview.find_input_pin('Destination'))
    connect(nav_valid, preview.find_input_pin('NavigationValid'))
    assert preview.find_input_pin('Colors').set_pin_value('/Game/XRFramework/VFX/NPC_VRTemplate.NPC_VRTemplate')
    entry.find_output_pin('then').break_pin_links()
    result.find_input_pin('ReturnValue').break_pin_links()
    connect(entry.find_output_pin('then'), preview.find_input_pin('execute'))
    connect(preview.find_output_pin('then'), result.find_input_pin('execute'))
    connect(preview.find_output_pin('ReturnValue'), result.find_input_pin('ReturnValue'))
    assets.set_metadata_tag(pawn, 'DoorRoutePreview', '1')
save(pawn)

for path in ['/Game/CatchTheLizard/BP_Lizard', '/Game/CatchTheLizard/Missions/BP_MovingToyLizard',
             '/Game/CatchTheLizard/Tutorial/BP_TrainingLizardDoll']:
    backup(path)
    bp = unreal.load_asset(path)
    defaults = B.get_default_object(bp)
    defaults.set_editor_property('LizardVoiceMinInterval', 45.)
    defaults.set_editor_property('LizardVoiceMaxInterval', 60.)
    if path != '/Game/CatchTheLizard/BP_Lizard':
        defaults.set_editor_property('bEnableRealLizardVoice', False)
        defaults.set_editor_property('LizardVoiceSound', None)
    save(bp)

report = {}
for name in ['Map_TrainingFacility', 'Map_CatchTheLizard', 'Map_CatchTheLizard_Level2']:
    path = '/Game/Maps/' + name
    backup(path, '.umap')
    assert level.load_level(path)
    data = {'lizards': [], 'starts': [], 'containers': []}
    for actor in actors.get_all_level_actors():
        if isinstance(actor, unreal.LizardGameplayActor):
            actor.set_editor_property('LizardVoiceMinInterval', 45.)
            actor.set_editor_property('LizardVoiceMaxInterval', 60.)
            data['lizards'].append({'actor': actor.get_name(), 'voice': actor.get_editor_property('bEnableRealLizardVoice'), 'min': 45, 'max': 60})
        if actor.get_class().get_name() == 'BP_CollectionBottle_C':
            volume = next(c for c in actor.get_components_by_class(unreal.SphereComponent) if c.get_name() == 'DepositVolume')
            volume.set_relative_location(unreal.Vector(0, 0, 0), False, False)
            volume.set_sphere_radius(8.)
            data['containers'].append(actor.get_name())
        if isinstance(actor, unreal.PlayerStart):
            location = actor.get_actor_location()
            if name == 'Map_TrainingFacility':
                # The Pawn's root IS VROrigin; there is no centered Character capsule.
                location.z = 0.
                actor.set_actor_location(location, False, True)
            data['starts'].append(str(actor.get_actor_transform()))
    assert level.save_current_level()
    report[name] = data
assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
(checkpoint / 'Migration.json').write_text(json.dumps(report, indent=2))
unreal.log('VR_INTERACTION_MIGRATION_SAVED')
