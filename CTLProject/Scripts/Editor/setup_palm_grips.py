"""Author palm and tool grip markers. Reruns preserve designer marker transforms."""
import unreal,json
from pathlib import Path
from editor_toolset.toolsets.blueprint import BlueprintTools as B
from editor_toolset.toolsets.actor import ActorTools as A

assets=unreal.EditorAssetLibrary
assert not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor()
def components(bp):return A.get_components(B.get_default_object(bp))
def find(bp,name):return next((c for c in components(bp) if c.get_name().removesuffix('_GEN_VARIABLE')==name),None)
def marker(bp,parent,name,location,color):
 c=find(bp,name)
 if c:return c
 c=A.add_component(parent,unreal.ArrowComponent.static_class(),name)
 c.set_editor_property('relative_location',unreal.Vector(*location))
 c.set_editor_property('relative_rotation',unreal.Rotator())
 c.set_editor_property('arrow_color',color)
 c.set_editor_property('arrow_size',.35)
 c.set_editor_property('hidden_in_game',True)
 return c
def save(bp):
 B.compile_blueprint(bp,warnings_as_errors=True)
 assert bp.get_editor_property('status')==unreal.BlueprintStatus.BS_UP_TO_DATE,bp.get_path_name()
 assert assets.save_loaded_asset(bp,only_if_is_dirty=False)

pawn=unreal.load_asset('/Game/XRFramework/Blueprints/BP_XRPawn')
# Midpoint of hand bone and middle-finger first knuckle, measured in Grip-controller
# space from the project's existing MannyXR hand meshes and their saved offsets.
for side,y,color in [('Left',-4.000287,unreal.Color(40,230,110)),('Right',4.000287,unreal.Color(50,150,255))]:
 marker(pawn,find(pawn,'MotionController'+side+'Grip'),'PalmGrip'+side,(-.903802,y,.306455),color)
save(pawn)

broom=unreal.load_asset('/Game/CatchTheLizard/BP_Broom')
free_type=find(broom,'GrabComponent').get_editor_property('GrabType')
assert 'FREE' in str(free_type),free_type
for bp,position in [(broom,(0,0,0)),(unreal.load_asset('/Game/CatchTheLizard/BP_CollectionBottle'),(-10,0,4.7))]:
 root=A.get_root_component(B.get_default_object(bp))
 assert root.get_name().startswith('GrabCollision'),root
 for side,color in [('Left',unreal.Color(40,230,110)),('Right',unreal.Color(50,150,255))]:
  marker(bp,root,'GripPoint'+side,position,color)
 if not find(bp,'PalmGrip'):
  A.add_component(bp,unreal.ToolPalmGripComponent.static_class(),'PalmGrip')
 # Preserve XRFramework ownership, haptics, physics and OnGrabbed/OnDropped signals.
 # The optional native component performs the authored snap after the accepted grab.
 find(bp,'GrabComponent').set_editor_property('GrabType',free_type)
 save(bp)

for path in ['/Game/CatchTheLizard/BP_LizardVRPawn']:
 bp=unreal.load_asset(path)
 if bp:save(bp)
assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
print('PALM_GRIPS_COMPILED_AND_SAVED')
