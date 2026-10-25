"""Replace the obsolete template restart/splash chain with current-map C++ travel."""
import unreal
from editor_toolset.toolsets.blueprint import BlueprintTools as B

assert not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor()
bp=unreal.load_asset('/Game/XRFramework/Blueprints/WBP_Menu')
g=B.get_graph(bp,'EventGraph')
event=B.find_nodes(g,title='On Clicked (RestartButton)')[0]
existing=B.find_nodes(g,title='RestartCurrentVRStage')
if existing:
    restart=existing[0]
else:
    types=B.find_node_types(g,'RestartCurrentVRStage')
    assert len(types)==1,types
    restart=B.create_node(g,types[0],unreal.IntPoint(640,0))
source=next(p for p in B.get_node_infos([event])[0].output_pins if p.name=='then')
for link in source.connected_pins:B.break_pins(source.pin_id,link)
target=next(p for p in B.get_node_infos([restart])[0].input_pins if p.name=='execute')
B.connect_pins(source.pin_id,target.pin_id)
for n in list(B.find_nodes(g)):
    if n.get_node_title() in ['SetLoadingScreen','Delay','Open Level (by Name)']:
        B.delete_node(n)
B.compile_blueprint(bp,warnings_as_errors=True)
assert bp.get_editor_property('status')==unreal.BlueprintStatus.BS_UP_TO_DATE
assert unreal.EditorAssetLibrary.save_loaded_asset(bp,only_if_is_dirty=False)
for path in ['/Game/CatchTheLizard/Music/Playback/BP_CTLGameInstance','/Game/CatchTheLizard/Menu/BP_MainMenuPawn','/Game/CatchTheLizard/Tutorial/BP_LevelSelectionPawn']:
    asset=unreal.load_asset(path)
    B.compile_blueprint(asset,warnings_as_errors=True)
    assert asset.get_editor_property('status')==unreal.BlueprintStatus.BS_UP_TO_DATE
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset,only_if_is_dirty=False)
print('VR_RESTART_AND_QUEST_AUDIO_BLUEPRINTS_COMPILED_SAVED')
