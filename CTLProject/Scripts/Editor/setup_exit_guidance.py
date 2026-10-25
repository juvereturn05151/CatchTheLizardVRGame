"""Author shared exit guidance without rebuilding or replacing gameplay actors."""
import unreal,json
from pathlib import Path
from editor_toolset.toolsets.blueprint import BlueprintTools as B
ROOT='/Game/CatchTheLizard/Missions'
assets=unreal.EditorAssetLibrary
tools=unreal.AssetToolsHelpers.get_asset_tools()
ls=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
aes=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ed=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
assert not ls.is_in_play_in_editor()

def save_bp(bp):
 B.compile_blueprint(bp,warnings_as_errors=True)
 assert bp.get_editor_property('status')==unreal.BlueprintStatus.BS_UP_TO_DATE
 assert assets.save_loaded_asset(bp,only_if_is_dirty=False)

def guidance_material(name,ring=False):
 path=ROOT+'/'+name
 if assets.does_asset_exist(path):return unreal.load_asset(path)
 m=tools.create_asset(name,ROOT,unreal.Material,unreal.MaterialFactoryNew())
 m.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
 color=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionVectorParameter,-400,-200)
 color.set_editor_property('parameter_name','GuidanceColor');color.set_editor_property('default_value',unreal.LinearColor(.15,1,.55,1))
 assert unreal.MaterialEditingLibrary.connect_material_property(color,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
 if ring:
  m.set_editor_property('blend_mode',unreal.BlendMode.BLEND_MASKED);m.set_editor_property('two_sided',True)
  uv=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionTextureCoordinate,-800,100)
  center=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionConstant2Vector,-800,300)
  center.set_editor_property('r',.5);center.set_editor_property('g',.5)
  masks=[]
  for radius,y in [(.48,100),(.385,300)]:
   mask=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionSphereMask,-500,y)
   mask.set_editor_property('attenuation_radius',radius);mask.set_editor_property('hardness_percent',100)
   assert unreal.MaterialEditingLibrary.connect_material_expressions(uv,'',mask,'A')
   assert unreal.MaterialEditingLibrary.connect_material_expressions(center,'',mask,'B')
   masks.append(mask)
  diff=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionSubtract,-200,200)
  assert unreal.MaterialEditingLibrary.connect_material_expressions(masks[0],'',diff,'A')
  assert unreal.MaterialEditingLibrary.connect_material_expressions(masks[1],'',diff,'B')
  assert unreal.MaterialEditingLibrary.connect_material_property(diff,'',unreal.MaterialProperty.MP_OPACITY_MASK)
 unreal.MaterialEditingLibrary.recompile_material(m)
 assert assets.save_loaded_asset(m,only_if_is_dirty=False)
 return m

text_path=ROOT+'/M_ExitGuidanceText'
text_material=unreal.load_asset(text_path) if assets.does_asset_exist(text_path) else assets.duplicate_asset('/Engine/EngineMaterials/DefaultTextMaterialOpaque',text_path)
assert text_material
text_material.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
color_source=unreal.MaterialEditingLibrary.get_material_property_input_node(text_material,unreal.MaterialProperty.MP_BASE_COLOR)
if not color_source:color_source=unreal.MaterialEditingLibrary.create_material_expression(text_material,unreal.MaterialExpressionVertexColor,-300,-300)
assert unreal.MaterialEditingLibrary.connect_material_property(color_source,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
unreal.MaterialEditingLibrary.recompile_material(text_material);assert assets.save_loaded_asset(text_material,only_if_is_dirty=False)

glow=guidance_material('M_ExitGuidanceGlow')
ring=guidance_material('M_ExitGuidanceRing',True)
door_bp=unreal.load_asset(ROOT+'/BP_ExitDoor');door_defaults=B.get_default_object(door_bp)

def configure_door(d):
 d.set_editor_property('HighlightMaterial',glow);d.set_editor_property('RingMaterial',ring)
 d.set_editor_property('HighlightColor',unreal.LinearColor(.15,1,.55,1))
 d.set_editor_property('GuidancePulseSpeed',.65);d.set_editor_property('MarkerDiameter',80)
 d.set_editor_property('MarkerGroundClearance',3)
 d.set_editor_property('DestinationOffset',unreal.Vector2D(0,0))
 d.set_editor_property('DoorMarkerOffset',unreal.Vector(-20,0,260))
 d.set_editor_property('DestinationTextOffset',unreal.Vector(0,0,65))
 d.set_editor_property('GuidanceTextSize',18)
 d.set_editor_property('DoorMarkerText',unreal.Text('EXIT'))
 d.set_editor_property('DestinationText',unreal.Text('Exit Here'))
 d.get_editor_property('ExitSign').set_text_material(text_material)
 d.get_editor_property('DestinationLabel').set_text_material(text_material)
 for name in ['DestinationRing','DestinationLabel']:
  c=d.get_editor_property(name);c.set_collision_profile_name('NoCollision');c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
  c.set_editor_property('generate_overlap_events',False);c.set_cast_shadow(False);c.set_visibility(False)
 d.get_editor_property('DestinationRing').set_material(0,ring)

configure_door(door_defaults);save_bp(door_bp)
mission_bp=unreal.load_asset(ROOT+'/BP_MissionDirector')
def configure_objectives(m):
 m.set_editor_property('ExitObjective',unreal.Text('Open the highlighted door and head outside.'))
 m.set_editor_property('HeadOutsideObjective',unreal.Text('Head outside to finish the mission.'))
configure_objectives(B.get_default_object(mission_bp));save_bp(mission_bp)
report={}
for name in ['Map_TrainingFacility','Map_CatchTheLizard','Map_CatchTheLizard_Level2']:
 assert ls.load_level('/Game/Maps/'+name)
 unreal.SystemLibrary.execute_console_command(ed.get_editor_world(),'save.FixupStandaloneFlags 1')
 m=next(a for a in aes.get_all_level_actors() if isinstance(a,unreal.MissionDirector))
 d=m.get_editor_property('ExitDoor');assert d and d.get_editor_property('Mission')==m
 configure_door(d);configure_objectives(m)
 zone=d.get_editor_property('OutdoorZone');center=zone.get_world_location()
 hit=unreal.SystemLibrary.line_trace_single(ed.get_editor_world(),center,center-unreal.Vector(0,0,700),unreal.TraceTypeQuery.ECC_VISIBILITY,False,[d],unreal.DrawDebugTrace.NONE)
 assert hit and hit.to_dict()['blocking_hit'],name
 floor=hit.to_dict()['impact_point'];assert d.contains_head_position(floor+unreal.Vector(0,0,130)),name
 projected=unreal.NavigationSystemV1.project_point_to_navigation(ed.get_editor_world(),floor+unreal.Vector(0,0,10),None,None,unreal.Vector(25,25,80))
 assert projected is not None and (projected-floor).length()<30,(name,projected)
 assert ls.save_current_level()
 report[name]={'door':d.get_path_name(),'zone':zone.get_path_name(),'ground':str(floor),'nav':str(projected),'capture_target':m.get_editor_property('CaptureTarget')}
for path in [ROOT+'/WBP_MissionObjective',ROOT+'/WBP_MissionComplete','/Game/CatchTheLizard/Tutorial/BP_TrainingDirector']:
 save_bp(unreal.load_asset(path))
assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
(Path(unreal.Paths.project_saved_dir())/'MissionCheckpoint/ExitGuidance-Assets.json').write_text(json.dumps(report,indent=2))
print('EXIT_GUIDANCE_ASSETS_SAVED',json.dumps(report))

