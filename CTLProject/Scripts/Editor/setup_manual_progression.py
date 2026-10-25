"""Author only the three progression maps. Runtime state remains entirely native."""
from pathlib import Path
import json
import unreal
from editor_toolset.toolsets.blueprint import BlueprintTools as B
from editor_toolset.toolsets.actor import ActorTools as A
base=Path(unreal.Paths.project_dir())
exec((base/'Scripts/Editor/setup_exit_missions.py').read_text().split("phase = globals().get('MISSION_PHASE'")[0])
TUTORIAL='/Game/Maps/Map_TrainingFacility'
INTRO1='This is a trial run to test your courage. Your target is only a toy lizardâ€”but this one moves! Use your broom to guide it, then catch it and place it in a container.'
END1='Well done! You handled the practice round. But next time, itâ€™s the real thing.'
INTRO2='Now comes the real test of your courage. This time, youâ€™ll be catching real lizards. If it gets too much, thereâ€™s an emergency button that can give you a helping hand. It becomes available after two minutes and dispenses a stunning spray.'
TRAINING_END='Training complete! Open the door and leave to begin your first assignment.'

def get_widgets(w):
 return {x.widget.get_name(): x.widget for x in ui.call_method('GetWidgets',args=(w,)).widgets if x.widget}

def configure_door(d):
 d.get_editor_property('Hinge').set_relative_location(unreal.Vector(0,-99,0),False,False)
 d.get_editor_property('Hinge').set_relative_rotation(unreal.Rotator(),False,False)
 leaf=d.get_editor_property('DoorMesh')
 leaf.attach_to_component(d.get_editor_property('Hinge'),'',unreal.AttachmentRule.KEEP_RELATIVE,unreal.AttachmentRule.KEEP_RELATIVE,unreal.AttachmentRule.KEEP_RELATIVE,False)
 leaf.set_relative_location(unreal.Vector(0,99,119),False,False)
 leaf.set_relative_rotation(unreal.Rotator(),False,False)
 leaf.set_relative_scale3d(unreal.Vector(.08,1.98,2.38))
 leaf.set_collision_profile_name('BlockAll')
 leaf.set_editor_property('mobility',unreal.ComponentMobility.MOVABLE)
 d.set_editor_property('LockedText',unreal.Text('EXIT\nComplete the capture objective\nHand at handle + index trigger'))
 d.set_editor_property('ReadyText',unreal.Text('EXIT READY\nOpen handle and leave\nHand at handle + index trigger'))
 d.get_editor_property('Handle').set_material(0,material('M_Handle',(.85,.67,.18,1)))

def assets_phase():
 door=unreal.load_asset(ROOT+'/BP_ExitDoor');configure_door(B.get_default_object(door));compile_save(door)
 emergency=bp('BP_EmergencySprayButton',unreal.EmergencySprayButton)
 d=B.get_default_object(emergency);d.set_editor_property('SprayClass',unreal.load_asset('/Game/CatchTheLizard/BP_Spray').generated_class());d.set_editor_property('UnlockDelay',120)
 d.get_editor_property('ButtonMesh').set_material(0,unreal.load_asset(ROOT+'/M_ExitMarker'))
 d.get_editor_property('StatusText').set_world_size(7)
 compile_save(emergency)
 toy=bp('BP_MovingToyLizard',unreal.load_asset('/Game/CatchTheLizard/BP_Lizard').generated_class())
 d=B.get_default_object(toy);d.set_editor_property('bTrainingDoll',False);d.set_editor_property('NormalFeedbackColor',unreal.Vector(.95,.045,.42))
 comps={c.get_name().replace('_GEN_VARIABLE',''):c for c in A.get_components(d)}
 comps['LizardBody'].set_material(0,unreal.load_asset('/Game/CatchTheLizard/Tutorial/M_TrainingToy'))
 # A visible wind-up key makes the practice target unmistakably artificial.
 if 'WindUpKey' not in comps:
  key=A.add_component(toy,unreal.StaticMeshComponent.static_class(),'WindUpKey')
  A.set_parent_component(key,A.get_root_component(d))
  key.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
  key.set_editor_property('relative_location',unreal.Vector(0,0,13));key.set_editor_property('relative_scale3d',unreal.Vector(.06,.22,.04))
  key.set_material(0,material('M_ToyKey',(1,.72,.06,1)));key.set_collision_profile_name('NoCollision')
 compile_save(toy)
 # Reuse the instructor dialogue class and editable UMG layout; enlarge its text area for Level 2.
 dialogue=unreal.load_asset('/Game/CatchTheLizard/Tutorial/WBP_TutorialDialogue')
 if not dialogue:
  candidates=assets.list_assets('/Game/CatchTheLizard/Tutorial',recursive=False,include_folder=False)
  dialogue=next(unreal.load_asset(p) for p in candidates if 'WBP_' in p and 'Dialogue' in p)
 items=get_widgets(dialogue)
 items['DialogueText'].set_text(unreal.Text('Instructor dialogue â€” edit the placed directorâ€™s Introduction.'))
 items['InstructorTitle'].set_text(unreal.Text('HOLOGRAM INSTRUCTOR'))
 compile_save(dialogue)
 mission=unreal.load_asset(ROOT+'/BP_MissionDirector');d=B.get_default_object(mission)
 panel=d.get_editor_property('DialoguePanel');panel.set_editor_property('widget_class',dialogue.generated_class());panel.set_editor_property('draw_size',unreal.IntPoint(1000,1100));panel.set_relative_scale3d(unreal.Vector(.14,.14,.14))
 panel.set_relative_location(unreal.Vector(210,-115,170),False,False);panel.set_relative_rotation(unreal.Rotator(yaw=151),False,False)
 d.set_editor_property('ExitObjective',unreal.Text('Open the exit door using its handle, then leave.'))
 compile_save(mission)
 completion=unreal.load_asset(ROOT+'/WBP_MissionComplete');items=get_widgets(completion);col=items['Content']
 if 'DebriefText' not in items:
  debrief=label(completion,col,'DebriefText','',36,bound=True);ui.call_method('MoveWidget',args=(completion,debrief.widget,col,2))
 if 'StarRow' not in items:
  row=add(completion,unreal.HorizontalBox,'StarRow',col);row.slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_CENTER)
  ui.call_method('MoveWidget',args=(completion,row.widget,col,3))
  for i in range(1,4):
   size=add(completion,unreal.SizeBox,'StarSize'+str(i),row.widget);size.widget.set_width_override(105);size.widget.set_height_override(105)
   item=add(completion,unreal.Image,'Star'+str(i),size.widget);bind(completion,item)
 if 'RatingText' not in items:
  rating=label(completion,col,'RatingText','',35,(1,.85,.35,1),True);ui.call_method('MoveWidget',args=(completion,rating.widget,col,4))
 tasks=[]
 for name in ['T_StarFilled','T_StarEmpty']:
  task=unreal.AssetImportTask();task.set_editor_property('filename',str(base/'SourceArt/MissionUI'/(name+'.png')));task.set_editor_property('destination_path',ROOT+'/UI');task.set_editor_property('automated',True);task.set_editor_property('replace_existing',True);task.set_editor_property('save',True);tasks.append(task)
 tools.import_asset_tasks(tasks)
 d=B.get_default_object(completion)
 for prop,name in [('FilledStar','T_StarFilled'),('EmptyStar','T_StarEmpty')]:
  texture=unreal.load_asset(ROOT+'/UI/'+name);texture.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI);assets.save_loaded_asset(texture);d.set_editor_property(prop,texture)
 compile_save(completion)
 panel_material_path=ROOT+'/M_VRPanelAlwaysVisible'
 panel_material=unreal.load_asset(panel_material_path) if assets.does_asset_exist(panel_material_path) else assets.duplicate_asset('/Engine/EngineMaterials/Widget3DPassThrough',panel_material_path)
 panel_material.set_editor_property('disable_depth_test',True);unreal.MaterialEditingLibrary.recompile_material(panel_material);assets.save_loaded_asset(panel_material)
 d=B.get_default_object(mission);completion_panel=d.get_editor_property('CompletionPanel')
 completion_panel.set_editor_property('draw_size',unreal.IntPoint(1200,1250));completion_panel.set_relative_scale3d(unreal.Vector(.12,.12,.12))
 completion_panel.set_material(0,panel_material);completion_panel.set_editor_property('blend_mode',unreal.WidgetBlendMode.TRANSPARENT);completion_panel.set_editor_property('translucency_sort_priority',100);compile_save(mission)
 td=unreal.load_asset('/Game/CatchTheLizard/Tutorial/BP_TrainingDirector');d=B.get_default_object(td)
 for prop in ['SuccessDialogue','FinishDialogue']:d.set_editor_property(prop,unreal.Text(TRAINING_END))
 d.set_editor_property('BroomDialogue',unreal.Text(''));compile_save(td)


def room_phase(which):
 path=[TUTORIAL,MAP1,MAP2][which]
 assert level.load_level(path)
 unreal.SystemLibrary.execute_console_command(editor.get_editor_world(),'save.FixupStandaloneFlags 1')
 all_actors=actors.get_all_level_actors()
 # Remove only gameplay sprays in these three maps, plus obsolete tutorial props.
 for a in all_actors:
  if isinstance(a,unreal.SprayGameplayActor) or (which==0 and (isinstance(a,unreal.BroomGameplayActor) or 'Broom' in a.get_actor_label())):actors.destroy_actor(a)
 if which==0:
  wall=next(a for a in actors.get_all_level_actors() if a.get_actor_label()=='Back Wall');actors.destroy_actor(wall)
  mat=unreal.load_asset('/Game/CatchTheLizard/Tutorial/M_TrainingWalls')
  prop('Training Exit Wall Left',(450,-250,160),(.1,3,3.2),mat)
  prop('Training Exit Wall Right',(450,250,160),(.1,3,3.2),mat)
  prop('Training Exit Wall Above',(450,0,280),(.1,2,.8),mat)
  prop('Training Outdoor Landing',(700,0,-5),(5,5,.1),material('M_Landing',(.14,.19,.22,1)))
  prop('Training Outdoor Path',(1000,0,-5),(2,2,.1),unreal.load_asset(ROOT+'/M_Landing'))
  if not any(isinstance(a,unreal.NavMeshBoundsVolume) for a in actors.get_all_level_actors()):
   nav=actors.spawn_actor_from_object(unreal.NavMeshBoundsVolume.static_class(),unreal.Vector(350,0,100));nav.set_actor_scale3d(unreal.Vector(11,6,3));nav.set_actor_label('Mission Navigation Bounds')
  door=actor('Training Exit',unreal.load_asset(ROOT+'/BP_ExitDoor').generated_class(),(450,0,0));door.set_actor_label('Training Exit')
  mission=actor('Tutorial Exit Mission',unreal.load_asset(ROOT+'/BP_MissionDirector').generated_class(),(0,0,0));mission.set_actor_label('Tutorial Exit Mission')
  training=next(a for a in actors.get_all_level_actors() if isinstance(a,unreal.TrainingDirector))
  training.set_editor_property('ExitMission',mission);training.set_editor_property('TrainingBroom',None);training.set_editor_property('BroomHighlight',None)
  training.set_editor_property('BroomDialogue',unreal.Text(''))
  for prop_name in ['SuccessDialogue','FinishDialogue']:training.set_editor_property(prop_name,unreal.Text(TRAINING_END))
  lizards=[training.get_editor_property('TrainingDoll')];containers=[training.get_editor_property('CaptureContainer')]
  mission.set_editor_property('bTutorial',True);mission.set_editor_property('NextMission',unreal.load_asset(MAP1))
  mission.set_editor_property('ExitObjective',unreal.Text(TRAINING_END));mission.set_editor_property('CaptureObjective',unreal.Text('Store the stationary toy lizard in the container.'))
  mission.set_editor_property('EarlyExitMessage',unreal.Text('Store the toy lizard in the container before leaving.'))
  mission.set_editor_property('Introduction',unreal.Text(''));mission.set_editor_property('Debrief',unreal.Text('Training complete! You are ready for your first assignment.'))
 else:
  mission=next(a for a in actors.get_all_level_actors() if isinstance(a,unreal.MissionDirector));door=mission.get_editor_property('ExitDoor')
  lizards=list(mission.get_editor_property('MissionLizards'));containers=list(mission.get_editor_property('Containers'))
  if which==1:
   for old in lizards:
    if old.get_class()!=unreal.load_asset(ROOT+'/BP_MovingToyLizard').generated_class():
     tr=old.get_actor_transform();normal=old.get_editor_property('SurfaceNormal');direction=old.get_editor_property('MoveDirection');actors.destroy_actor(old)
     toy=actors.spawn_actor_from_class(unreal.load_asset(ROOT+'/BP_MovingToyLizard').generated_class(),tr.translation,tr.rotation.rotator());toy.set_actor_label('Moving wind-up toy lizard');toy.set_editor_property('SurfaceNormal',normal);toy.set_editor_property('MoveDirection',direction);lizards=[toy]
  mission.set_editor_property('bTutorial',False);mission.set_editor_property('bEmergencyRating',which==2)
  mission.set_editor_property('Introduction',unreal.Text(INTRO1 if which==1 else INTRO2));mission.set_editor_property('Debrief',unreal.Text(END1 if which==1 else 'You completed the real-lizard assignment. Well done!'))
  mission.set_editor_property('CaptureObjective',unreal.Text('Capture 1 moving toy lizard and store it in a container.' if which==1 else 'Capture all 3 real lizards and store them in a container.'))
  mission.set_editor_property('ExitObjective',unreal.Text('Capture complete! Open the exit door using its handle, then leave.'))
  mission.set_editor_property('EarlyExitMessage',unreal.Text('Capture and store the remaining target before leaving.'))
  mission.set_editor_property('NextMission',unreal.load_asset(MAP2) if which==1 else None)
  npc=actor('Hologram Instructor',unreal.load_asset('/Game/CatchTheLizard/Tutorial/BP_TrainingInstructor').generated_class(),(-170,-200,0),unreal.Rotator(yaw=40));npc.set_actor_label('Hologram Instructor')
  panel=mission.get_editor_property('DialoguePanel');panel.set_world_location(unreal.Vector(-50,-100,180),False,False);panel.set_world_rotation(unreal.Rotator(yaw=63),False,False)
  manager=mission.get_editor_property('CollectionManager');manager.set_editor_property('bRequireExit',True)
  if which==2:
   emergency=actor('Emergency Spray Dispenser',unreal.load_asset(ROOT+'/BP_EmergencySprayButton').generated_class(),(305,310,115),unreal.Rotator(yaw=90));emergency.set_actor_label('Emergency Spray Dispenser');emergency.set_editor_property('Mission',mission);emergency.set_editor_property('UnlockDelay',120)
   # The editable scene transform is relative to the physical button. A nearby shelf catches the spray.
   emergency.get_editor_property('SpraySpawnPoint').set_relative_location(unreal.Vector(-45,0,0),False,False)
   prop('Emergency Spray Shelf',(305,265,85),(.8,.7,.08),material('M_DispenserShelf',(.08,.12,.17,1)))
   prop('Emergency Button Pedestal',(305,310,53),(.18,.18,1.06),unreal.load_asset(ROOT+'/M_DispenserShelf'))
 configure_door(door)
 mission.set_editor_property('CaptureTarget',3 if which==2 else 1);mission.set_editor_property('MissionLizards',lizards);mission.set_editor_property('Containers',containers)
 mission.set_editor_property('MissionTools',[a for a in actors.get_all_level_actors() if isinstance(a,unreal.BroomGameplayActor) or a in containers]);mission.set_editor_property('ExitDoor',door);door.set_editor_property('Mission',mission)
 mission.set_editor_property('MainMenu',unreal.load_asset('/Game/Maps/Map_MainMenu'))
 # All stages reuse the saved editable completion widget and the same physical door.
 mission.get_editor_property('CompletionPanel').set_editor_property('draw_size',unreal.IntPoint(1200,1250));mission.get_editor_property('CompletionPanel').set_relative_scale3d(unreal.Vector(.12,.12,.12))
 mission.get_editor_property('CompletionPanel').set_material(0,unreal.load_asset(ROOT+'/M_VRPanelAlwaysVisible'));mission.get_editor_property('CompletionPanel').set_editor_property('blend_mode',unreal.WidgetBlendMode.TRANSPARENT);mission.get_editor_property('CompletionPanel').set_editor_property('translucency_sort_priority',100)
 if which==0:
  panel=mission.get_editor_property('ObjectivePanel');panel.set_world_location(unreal.Vector(210,100,180),False,False);panel.set_world_rotation(unreal.Rotator(yaw=-150),False,False)
 assert unreal.MissionDirector.build_mission_geometry_and_navigation()
 assert level.save_current_level()
 assert not any(isinstance(a,unreal.SprayGameplayActor) for a in actors.get_all_level_actors())
 print('MANUAL_FLOW_MAP_SAVED',path)


def catalog_phase():
 catalog=unreal.load_asset('/Game/CatchTheLizard/Tutorial/DA_LevelList');entries=list(catalog.get_editor_property('Levels'))
 names={'Tutorial':'Tutorial â€” Store a stationary toy','Level1':'Level 1 â€” Capture 1 moving toy','Level2':'Level 2 â€” Capture 3 real lizards'}
 for entry in entries:
  key=str(entry.get_editor_property('Id'))
  if key in names:entry.set_editor_property('DisplayName',unreal.Text(names[key]))
 catalog.set_editor_property('Levels',entries);assets.save_loaded_asset(catalog)
 selection=unreal.load_asset('/Game/CatchTheLizard/Tutorial/WBP_LevelSelection');items=get_widgets(selection)
 items['SelectionSubtitle'].set_text(unreal.Text('Stationary toy â†’ Moving toy â†’ Real lizards'))
 compile_save(selection)
 for path in [ROOT+'/BP_ExitDoor',ROOT+'/BP_EmergencySprayButton',ROOT+'/BP_MovingToyLizard',ROOT+'/BP_MissionDirector',ROOT+'/WBP_MissionComplete',ROOT+'/WBP_MissionObjective','/Game/CatchTheLizard/Tutorial/BP_TrainingDirector','/Game/CatchTheLizard/Tutorial/BP_TrainingLizardDoll','/Game/CatchTheLizard/BP_Lizard','/Game/CatchTheLizard/BP_CollectionBottle','/Game/CatchTheLizard/BP_Spray','/Game/CatchTheLizard/BP_Broom','/Game/CatchTheLizard/BP_LizardCollectionManager']:
  compile_save(unreal.load_asset(path))

phase=globals().get('MANUAL_FLOW_PHASE','assets')
if phase=='assets':assets_phase()
elif phase in ['tutorial','level1','level2']:room_phase(['tutorial','level1','level2'].index(phase))
elif phase=='catalog':catalog_phase()
assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
print('MANUAL_FLOW_PHASE_DONE',phase)
