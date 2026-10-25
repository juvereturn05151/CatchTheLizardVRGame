"""Outdoor entry for the two gameplay stages. Tutorial map/flow is intentionally untouched."""
import unreal,json,hashlib
from pathlib import Path
from editor_toolset.toolsets.blueprint import BlueprintTools as B

base=Path(unreal.Paths.project_dir());assets=unreal.EditorAssetLibrary
ls=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);ae=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert not ls.is_in_play_in_editor()
tutorial=base/'Content/Maps/Map_TrainingFacility.umap'
before_tutorial=hashlib.sha256(tutorial.read_bytes()).hexdigest()
def compile_save(path):
 bp=unreal.load_asset(path);B.compile_blueprint(bp,warnings_as_errors=True)
 assert bp.get_editor_property('status')==unreal.BlueprintStatus.BS_UP_TO_DATE,path
 assert assets.save_loaded_asset(bp,only_if_is_dirty=False)

compile_save('/Game/CatchTheLizard/Missions/BP_ExitDoor')
compile_save('/Game/CatchTheLizard/Missions/BP_MissionDirector')
report={}
for name in ['Map_CatchTheLizard','Map_CatchTheLizard_Level2']:
 assert ls.load_level('/Game/Maps/'+name)
 actors=ae.get_all_level_actors();mission=next(a for a in actors if isinstance(a,unreal.MissionDirector));door=mission.get_editor_property('ExitDoor')
 intro=str(mission.get_editor_property('Introduction'));target=mission.get_editor_property('CaptureTarget')
 transform=door.get_actor_transform()
 def position(x,y,z):return unreal.MathLibrary.transform_location(transform,unreal.Vector(x,y,z))
 start=next(a for a in actors if isinstance(a,unreal.PlayerStart))
 # The XR pawn uses LocalFloor tracking: its origin belongs on the floor,
 # rather than at a character capsule's center height.
 start.set_actor_location(position(440,-15,0),False,True)
 start.set_actor_rotation(unreal.Rotator(yaw=unreal.MathLibrary.find_look_at_rotation(start.get_actor_location(),door.get_actor_location()).yaw),True)
 npc=next(a for a in actors if a.get_actor_label()=='Hologram Instructor')
 npc.set_actor_location(position(300,130,0),False,True)
 npc.set_actor_rotation(unreal.Rotator(yaw=unreal.MathLibrary.find_look_at_rotation(npc.get_actor_location(),start.get_actor_location()).yaw),True)
 dialogue=mission.get_editor_property('DialoguePanel')
 dialogue.set_world_location(position(300,-15,170),False,False)
 dialogue.set_world_rotation(unreal.MathLibrary.find_look_at_rotation(position(300,-15,170),position(440,-15,165)),False,False)
 mission.set_editor_property('bStartOutside',True)
 mission.set_editor_property('EntryObjective',unreal.Text('Open the entrance door and step inside.'))
 mission.set_editor_property('EntryClosingObjective',unreal.Text('Step clear of the doorway so the door can close.'))
 mission.set_editor_property('ExitObjective',unreal.Text('All lizards captured! Open the door and head outside.'))
 door.set_editor_property('LockedHandleFeedback',unreal.Text('Catch all required lizards to unlock the exit.'))
 door.get_editor_property('SafeInteriorZone').set_box_extent(unreal.Vector(700,800,165))
 # The original flush pivot lets the thick leaf graze the wall while rotating.
 # Move the pivot to the frame's outside face while preserving the closed leaf
 # and handle positions. Do this only on gameplay instances, not the tutorial.
 hinge=door.get_editor_property('Hinge');leaf=door.get_editor_property('DoorMesh')
 inside=door.get_editor_property('Handle');outside=door.get_editor_property('ExteriorHandle')
 hinge_position=hinge.get_editor_property('relative_location')
 shift=10.-hinge_position.x
 if abs(shift)>.001:
  hinge.set_relative_location(hinge_position+unreal.Vector(shift,0,0),False,False)
  for part in [leaf,inside]:part.set_relative_location(part.get_editor_property('relative_location')-unreal.Vector(shift,0,0),False,False)
 p=inside.get_editor_property('relative_location');leaf_x=leaf.get_editor_property('relative_location').x
 outside.set_relative_location(unreal.Vector(2*leaf_x-p.x,p.y,p.z),False,False)
 outside.set_relative_rotation(inside.get_editor_property('relative_rotation'),False,False)
 outside.set_relative_scale3d(inside.get_editor_property('relative_scale3d'));outside.set_static_mesh(inside.get_editor_property('static_mesh'))
 for i,material in enumerate(inside.get_materials()):outside.set_material(i,material)
 interaction=door.get_editor_property('ExteriorHandleInteraction');interaction.set_editor_property('PressPoint',outside)
 original=door.get_editor_property('HandleInteraction')
 for prop in ['ReachRadius','LeftAction','RightAction']:interaction.set_editor_property(prop,original.get_editor_property(prop))
 # Broaden the existing landing into a clear forecourt, extending the existing path
 # beyond it without overlapping coplanar floor meshes. All placement is door-relative.
 for actor in actors:
  label=actor.get_actor_label()
  if label=='Outdoor Landing':actor.set_actor_location(position(300,0,-10),False,True);actor.set_actor_scale3d(unreal.Vector(6.4,6.9,.2))
  elif label=='Outdoor Path':actor.set_actor_location(position(720,0,-10),False,True)
  elif label.startswith('Landing Edge '):
   actor.set_actor_location(position(300,-340 if '-' in label else 340,4),False,True);actor.set_actor_scale3d(unreal.Vector(6.4,.08,.08))
  elif label.startswith('Path Edge '):actor.set_actor_location(position(720,-350 if '-' in label else 350,4),False,True)
 assert str(mission.get_editor_property('Introduction'))==intro
 assert mission.get_editor_property('CaptureTarget')==target
 assert unreal.MissionDirector.build_mission_geometry_and_navigation()
 assert ls.save_current_level()
 report[name]={'player_start':str(start.get_actor_transform()),'hologram':str(npc.get_actor_transform()),'dialogue':str(dialogue.get_world_transform()),'capture_target':target,'intro_preserved':True,'outdoor_entry':True}

assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
assert hashlib.sha256(tutorial.read_bytes()).hexdigest()==before_tutorial,'Tutorial map unexpectedly changed'
out=Path(unreal.Paths.project_saved_dir())/'OutdoorEntryCheckpoint';out.mkdir(exist_ok=True)
(out/'Assets.json').write_text(json.dumps({'stages':report,'tutorial_sha256':before_tutorial,'tutorial_map_unchanged':True},indent=2))
print('OUTDOOR_ENTRY_MAPS_SAVED',report)
