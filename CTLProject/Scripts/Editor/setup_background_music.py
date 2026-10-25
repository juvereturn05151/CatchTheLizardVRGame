"""Run in Unreal Editor after compiling CTLBackgroundMusic.
Imports prepared loops and creates editable music configuration without replacing originals.
Reruns preserve designer edits to existing configuration assets.
"""
from pathlib import Path
import json
import unreal
from editor_toolset.toolsets.blueprint import BlueprintTools as B

ROOT = '/Game/CatchTheLizard/Music/Playback'
assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
assert not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor()

for name in ['TutorialBGM_Loop', 'GameBGM_Loop']:
    if not assets.does_asset_exist(ROOT + '/' + name):
        task = unreal.AssetImportTask()
        task.filename = str(Path(unreal.Paths.project_dir()) / 'SourceAudio' / 'Music' / (name + '.wav'))
        task.destination_path = ROOT
        task.destination_name = name
        task.automated = True
        task.save = True
        tools.import_asset_tasks([task])
    track = unreal.load_asset(ROOT + '/' + name)
    assert isinstance(track, unreal.SoundWave), name
    track.set_editor_property('looping', True)
    track.set_editor_property('virtualization_mode', unreal.VirtualizationMode.PLAY_WHEN_SILENT)
    assets.save_loaded_asset(track)

library_path = ROOT + '/DA_MapMusic'
if not assets.does_asset_exist(library_path):
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.CTLMusicLibrary)
    library = tools.create_asset('DA_MapMusic', ROOT, unreal.CTLMusicLibrary, factory)
else:
    library = unreal.load_asset(library_path)
if not library.get_editor_property('maps'):
    entries = []
    for map_name, group, track_name, volume in [
        ('Map_MainMenu', 'Menu', 'TutorialBGM_Loop', .16),
        ('Map_LevelSelection', 'Menu', 'TutorialBGM_Loop', .16),
        ('Map_TrainingFacility', 'Tutorial', 'TutorialBGM_Loop', .13),
        ('Map_CatchTheLizard', 'Gameplay', 'GameBGM_Loop', .17),
    ]:
        entry = unreal.CTLMapMusic()
        entry.set_editor_property('map', unreal.load_asset('/Game/Maps/' + map_name))
        entry.set_editor_property('continuity_group', group)
        entry.set_editor_property('track', unreal.load_asset(ROOT + '/' + track_name))
        entry.set_editor_property('volume', volume)
        entries.append(entry)
    library.set_editor_property('maps', entries)
assets.save_loaded_asset(library)

bp_path = ROOT + '/BP_CTLGameInstance'
if not assets.does_asset_exist(bp_path):
    factory = unreal.BlueprintFactory()
    factory.set_editor_property('parent_class', unreal.CTLGameInstance)
    bp = tools.create_asset('BP_CTLGameInstance', ROOT, unreal.Blueprint, factory)
    B.get_default_object(bp).set_editor_property('music_library', library)
else:
    bp = unreal.load_asset(bp_path)

for path in [bp_path, '/Game/CatchTheLizard/Menu/BP_MainMenuPawn',
             '/Game/CatchTheLizard/Tutorial/BP_LevelSelectionPawn',
             '/Game/CatchTheLizard/Tutorial/BP_TrainingDirector']:
    item = unreal.load_asset(path)
    B.compile_blueprint(item, warnings_as_errors=True)
    assert item.get_editor_property('status') == unreal.BlueprintStatus.BS_UP_TO_DATE, path
    assert assets.save_loaded_asset(item, only_if_is_dirty=False)

# Apply to this editor session as well as DefaultEngine.ini, set by the authoring task.
settings = unreal.get_default_object(unreal.GameMapsSettings)
settings.set_editor_property('game_instance_class', unreal.SoftClassPath(bp.generated_class().get_path_name()))
assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
checkpoint = Path(unreal.Paths.project_saved_dir()) / 'MusicCheckpoint'
checkpoint.mkdir(exist_ok=True)
report = {'library': library.get_path_name(), 'game_instance': bp.generated_class().get_path_name(),
          'entries': [e.export_text() for e in library.get_editor_property('maps')],
          'originals_preserved': True, 'blueprints_compiled_and_saved': True}
(checkpoint / 'AssetVerification.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
assets.sync_browser_to_objects([library_path])
print('MUSIC_ASSETS_SAVED', report)
