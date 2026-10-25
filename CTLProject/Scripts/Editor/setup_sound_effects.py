"""Import synthesized placeholders and bind editable defaults; safe to rerun.
Requires the compiled native sound-effect classes. Existing mix edits are preserved.
"""
from pathlib import Path
import json
import unreal
from editor_toolset.toolsets.blueprint import BlueprintTools as B
from editor_toolset.toolsets.actor import ActorTools as A

ROOT = '/Game/CatchTheLizard/Audio/SFX'
assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
assert not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor()
names = ['UIHover', 'UIConfirm', 'ObjectPickup', 'BroomSwing', 'SprayLoop',
         'StunActivated', 'ContainerCapture', 'TutorialComplete']
for name in names:
    path = ROOT + '/SFX_' + name
    if not assets.does_asset_exist(path):
        task = unreal.AssetImportTask()
        task.filename = str(Path(unreal.Paths.project_dir()) / 'SourceAudio/SFX' / ('SFX_' + name + '.wav'))
        task.destination_path = ROOT
        task.destination_name = 'SFX_' + name
        task.automated = True
        task.save = True
        tools.import_asset_tasks([task])
    sound = unreal.load_asset(path)
    assert isinstance(sound, unreal.SoundWave) and sound.get_editor_property('num_channels') == 1
    sound.set_editor_property('looping', name == 'SprayLoop')
    assets.save_loaded_asset(sound)

attenuation_path = ROOT + '/ATT_WorldEffects'
if not assets.does_asset_exist(attenuation_path):
    attenuation = tools.create_asset('ATT_WorldEffects', ROOT, unreal.SoundAttenuation, unreal.SoundAttenuationFactory())
    settings = attenuation.get_editor_property('attenuation')
    settings.set_editor_property('attenuate', True)
    settings.set_editor_property('spatialize', True)
    settings.set_editor_property('attenuation_shape_extents', unreal.Vector(70, 0, 0))
    settings.set_editor_property('falloff_distance', 900.)
    attenuation.set_editor_property('attenuation', settings)
    assets.save_loaded_asset(attenuation)
else:
    attenuation = unreal.load_asset(attenuation_path)

library_path = ROOT + '/DA_SoundEffects'
if not assets.does_asset_exist(library_path):
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.CTLSoundEffectLibrary)
    library = tools.create_asset('DA_SoundEffects', ROOT, unreal.CTLSoundEffectLibrary, factory)
    for prop, name, volume, cooldown in [
        ('ui_hover', 'UIHover', .45, .08), ('ui_confirm', 'UIConfirm', .55, .12),
        ('object_pickup', 'ObjectPickup', .65, .15), ('broom_swing', 'BroomSwing', .5, .4),
        ('spray_loop', 'SprayLoop', .5, 0.), ('stun_activated', 'StunActivated', .65, .35),
        ('container_capture', 'ContainerCapture', .7, .2), ('tutorial_complete', 'TutorialComplete', .6, .8),
    ]:
        entry = unreal.CTLSoundEffectSettings()
        entry.set_editor_property('sound', unreal.load_asset(ROOT + '/SFX_' + name))
        entry.set_editor_property('volume', volume)
        entry.set_editor_property('min_interval_seconds', cooldown)
        library.set_editor_property(prop, entry)
    library.set_editor_property('world_attenuation', attenuation)
    assets.save_loaded_asset(library)
else:
    library = unreal.load_asset(library_path)

gi_path = '/Game/CatchTheLizard/Music/Playback/BP_CTLGameInstance'
gi_bp = unreal.load_asset(gi_path)
B.get_default_object(gi_bp).set_editor_property('sound_effects', library)

for name in ['BP_Broom', 'BP_Spray', 'BP_CollectionBottle']:
    bp = unreal.load_asset('/Game/CatchTheLizard/' + name)
    if not any(isinstance(c, unreal.CTLPickupAudioComponent) for c in A.get_components(B.get_default_object(bp))):
        A.add_component(bp, unreal.CTLPickupAudioComponent.static_class(), 'PickupAudio')

affected = [gi_path, '/Game/CatchTheLizard/BP_Broom', '/Game/CatchTheLizard/BP_Spray',
            '/Game/CatchTheLizard/BP_CollectionBottle', '/Game/CatchTheLizard/BP_Lizard',
            '/Game/CatchTheLizard/Tutorial/BP_TrainingLizardDoll',
            '/Game/CatchTheLizard/Menu/WBP_MainMenu', '/Game/CatchTheLizard/Tutorial/WBP_LevelSelection',
            '/Game/CatchTheLizard/Tutorial/WBP_TutorialDialogue', '/Game/CatchTheLizard/Tutorial/BP_TrainingDirector']
statuses = {}
for path in affected:
    bp = unreal.load_asset(path)
    B.compile_blueprint(bp, warnings_as_errors=True)
    statuses[path] = str(bp.get_editor_property('status'))
    assert bp.get_editor_property('status') == unreal.BlueprintStatus.BS_UP_TO_DATE, path
    assert assets.save_loaded_asset(bp, only_if_is_dirty=False)
assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
checkpoint = Path(unreal.Paths.project_saved_dir()) / 'SFXCheckpoint'
checkpoint.mkdir(exist_ok=True)
(checkpoint / 'AssetVerification.json').write_text(json.dumps({'library': library_path, 'blueprints': statuses,
    'sound_assets': names, 'synthesized_placeholders': True, 'saved': True}, indent=2))
assets.sync_browser_to_objects([library_path])
print('SFX_ASSETS_COMPILED_AND_SAVED', statuses)
