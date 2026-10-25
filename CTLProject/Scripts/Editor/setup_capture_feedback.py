"""Author stored-lizard display slots, clear glass and the exit-completion cue."""
import unreal,json
from pathlib import Path
from editor_toolset.toolsets.blueprint import BlueprintTools as B
from editor_toolset.toolsets.actor import ActorTools as A

assets=unreal.EditorAssetLibrary;tools=unreal.AssetToolsHelpers.get_asset_tools()
ls=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert not ls.is_in_play_in_editor()
def components(bp):return A.get_components(B.get_default_object(bp))
def find(bp,name):return next((c for c in components(bp) if c.get_name().removesuffix('_GEN_VARIABLE')==name),None)
def save_bp(bp):
 B.compile_blueprint(bp,warnings_as_errors=True)
 assert bp.get_editor_property('status')==unreal.BlueprintStatus.BS_UP_TO_DATE,bp.get_path_name()
 assert assets.save_loaded_asset(bp,only_if_is_dirty=False)

sound_path='/Game/CatchTheLizard/Audio/SFX/SFX_MissionComplete'
if not assets.does_asset_exist(sound_path):
 task=unreal.AssetImportTask();task.set_editor_property('filename',str(Path(unreal.Paths.project_dir())/'SourceAudio/Feedback/SFX_MissionComplete.wav'))
 task.set_editor_property('destination_path','/Game/CatchTheLizard/Audio/SFX');task.set_editor_property('automated',True);task.set_editor_property('save',True)
 tools.import_asset_tasks([task])
sound=unreal.load_asset(sound_path);assert sound
library=unreal.load_asset('/Game/CatchTheLizard/Audio/SFX/DA_SoundEffects')
entry=library.get_editor_property('MissionComplete')
if not entry.get_editor_property('Sound'):
 entry.set_editor_property('Sound',sound);entry.set_editor_property('Volume',.7);entry.set_editor_property('MinIntervalSeconds',.8)
 library.set_editor_property('MissionComplete',entry)
assert assets.save_loaded_asset(library,only_if_is_dirty=False)

mat_path='/Game/CatchTheLizard/Materials/MI_CollectionBottle_Contents'
if assets.does_asset_exist(mat_path):glass=unreal.load_asset(mat_path)
else:
 glass=tools.create_asset('MI_CollectionBottle_Contents','/Game/CatchTheLizard/Materials',unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew())
 unreal.MaterialEditingLibrary.set_material_instance_parent(glass,unreal.load_asset('/Game/CatchTheLizard/Materials/M_CollectionBottle_Glass'))
 unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(glass,'GlassOpacity',.045)
 unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(glass,'GlassEdgeOpacity',.16)
 unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(glass,'GlassRoughness',.18)
 assert assets.save_loaded_asset(glass,only_if_is_dirty=False)

bottle=unreal.load_asset('/Game/CatchTheLizard/BP_CollectionBottle')
root=A.get_root_component(B.get_default_object(bottle))
for i,z in enumerate([-7.,0.,7.]):
 name='StoredSlot'+str(i+1)
 if not find(bottle,name):
  slot=A.add_component(root,unreal.ArrowComponent.static_class(),name)
  slot.set_editor_property('relative_location',unreal.Vector(0,0,z))
  slot.set_editor_property('relative_rotation',unreal.Rotator(yaw=i*120))
  slot.set_editor_property('arrow_size',.25);slot.set_editor_property('arrow_color',unreal.Color(80,220,130));slot.set_editor_property('hidden_in_game',True)
if not find(bottle,'StoredContents'):A.add_component(bottle,unreal.StoredLizardDisplayComponent.static_class(),'StoredContents')
body=find(bottle,'BottleBody');body.set_material(0,glass);body.set_cast_shadow(False)
save_bp(bottle)

training_jar=unreal.load_asset('/Game/CatchTheLizard/Tutorial/BP_TrainingContainer')
root=A.get_root_component(B.get_default_object(training_jar))
new_display=find(training_jar,'StoredContents') is None
for i,z in enumerate([-8.,0.,8.]):
 name='StoredSlot'+str(i+1)
 if not find(training_jar,name):
  slot=A.add_component(root,unreal.ArrowComponent.static_class(),name)
  slot.set_editor_property('relative_location',unreal.Vector(0,0,z));slot.set_editor_property('relative_rotation',unreal.Rotator(yaw=i*120))
  slot.set_editor_property('arrow_size',.3);slot.set_editor_property('hidden_in_game',True)
if new_display:
 display=A.add_component(training_jar,unreal.StoredLizardDisplayComponent.static_class(),'StoredContents')
 display.set_editor_property('StoredModelScale',unreal.Vector(.75,.75,.75))
for name in ['LeftWall','RightWall','FrontWall','BackWall']:
 wall=find(training_jar,name);wall.set_material(0,glass);wall.set_cast_shadow(False)
save_bp(training_jar)

paths=['/Game/CatchTheLizard/BP_Lizard','/Game/CatchTheLizard/Missions/BP_MovingToyLizard','/Game/CatchTheLizard/Tutorial/BP_TrainingLizardDoll',
       '/Game/CatchTheLizard/Tutorial/BP_TrainingDirector','/Game/CatchTheLizard/Missions/BP_MissionDirector',
       '/Game/CatchTheLizard/Music/Playback/BP_CTLGameInstance','/Game/CatchTheLizard/BP_LizardCollectionManager',training_jar.get_path_name()]
for path in paths:save_bp(unreal.load_asset(path))

map_report={}
for name in ['Map_TrainingFacility','Map_CatchTheLizard','Map_CatchTheLizard_Level2']:
 ls.load_level('/Game/Maps/'+name)
 jars=[a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors() if a.get_class().get_name() in ['BP_CollectionBottle_C','BP_TrainingContainer_C']]
 for jar in jars:
  for body in jar.get_components_by_class(unreal.StaticMeshComponent):
   if body.get_name() in ['BottleBody','LeftWall','RightWall','FrontWall','BackWall']:body.set_material(0,glass);body.set_cast_shadow(False)
  assert jar.get_component_by_class(unreal.StoredLizardDisplayComponent),jar
 map_report[name]=len(jars)
 assert ls.save_current_level()
assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
out=Path(unreal.Paths.project_saved_dir())/'CaptureFeedbackCheckpoint';out.mkdir(exist_ok=True)
(out/'Assets.json').write_text(json.dumps({'maps':map_report,'completion_sound':sound_path,'glass':mat_path,'blueprints':paths+[bottle.get_path_name()]},indent=2))
print('CAPTURE_FEEDBACK_ASSETS_SAVED',map_report)
