"""Import Blender skeletal assets and configure existing actors without replacing mission references."""
import unreal,json
from pathlib import Path
from editor_toolset.toolsets.blueprint import BlueprintTools as B
from editor_toolset.toolsets.actor import ActorTools as A
ROOT='/Game/CatchTheLizard/Animation';BASE=Path(unreal.Paths.project_dir())
assets=unreal.EditorAssetLibrary;tools=unreal.AssetToolsHelpers.get_asset_tools()
ls=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);aes=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert not ls.is_in_play_in_editor()

def save_bp(bp):
 B.compile_blueprint(bp,warnings_as_errors=True)
 assert bp.get_editor_property('status')==unreal.BlueprintStatus.BS_UP_TO_DATE,bp.get_path_name()
 assert assets.save_loaded_asset(bp,only_if_is_dirty=False)

def mesh_asset(name):return unreal.load_asset(ROOT+'/'+name+'/SK_'+name+'Gecko')
def animation_asset(name,kind):return unreal.load_asset(ROOT+'/'+name+'/A_'+name+'_'+kind)
def anim_bp(name):return unreal.load_asset(ROOT+'/'+name+'/ABP_'+name+'Gecko')

def import_fbx(name,kind=None):
 path=BASE/'SourceArt/Lizards'/name/'Exports'/('A_'+name+'_'+kind+'.fbx' if kind else 'SK_'+name+'Gecko.fbx')
 assert path.exists(),path
 ui=unreal.FbxImportUI();ui.set_editor_property('automated_import_should_detect_type',False)
 ui.set_editor_property('import_materials',False);ui.set_editor_property('import_textures',False)
 ui.set_editor_property('import_as_skeletal',True);ui.set_editor_property('create_physics_asset',False)
 ui.set_editor_property('import_mesh',kind is None);ui.set_editor_property('import_animations',kind is not None)
 ui.set_editor_property('mesh_type_to_import',unreal.FBXImportType.FBXIT_ANIMATION if kind else unreal.FBXImportType.FBXIT_SKELETAL_MESH)
 if kind:ui.set_editor_property('skeleton',mesh_asset(name).get_editor_property('skeleton'))
 data=ui.get_editor_property('anim_sequence_import_data' if kind else 'skeletal_mesh_import_data')
 data.set_editor_property('convert_scene',True);data.set_editor_property('convert_scene_unit',True);data.set_editor_property('force_front_x_axis',False)
 data.set_editor_property('import_uniform_scale',1.)
 if kind:
  data.set_editor_property('animation_length',unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
  data.set_editor_property('use_default_sample_rate',False);data.set_editor_property('custom_sample_rate',30)
 else:
  data.set_editor_property('import_morph_targets',False)
  data.set_editor_property('normal_import_method',unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
 task=unreal.AssetImportTask();task.set_editor_property('filename',str(path));task.set_editor_property('destination_path',ROOT+'/'+name)
 task.set_editor_property('destination_name',path.stem);task.set_editor_property('automated',True);task.set_editor_property('replace_existing',True);task.set_editor_property('save',True)
 task.set_editor_property('factory',unreal.FbxFactory());task.set_editor_property('options',ui)
 tools.import_asset_tasks([task]);print('LIZARD_IMPORTED',name,kind,list(task.get_editor_property('imported_object_paths')))
 result=animation_asset(name,kind) if kind else mesh_asset(name)
 assert result,(name,kind)
 if kind:
  result.set_editor_property('enable_root_motion',False);result.set_editor_property('force_root_lock',True)
 assert assets.save_loaded_asset(result,only_if_is_dirty=False)
 return result

def surface_material(name):
 path=ROOT+'/Materials/M_'+name+'Gecko'
 if assets.does_asset_exist(path):return unreal.load_asset(path)
 original='/Game/CatchTheLizard/GeneratedModel/'+('green+plush+gecko+3d+model' if name=='Toy' else 'beige+gecko+3d+model')+'/'
 base_name='tripo_node_ac6f65c9-e066-458e-a392-e8172d5f84d9_BaseColor' if name=='Toy' else 'beige_gecko_3d_model_basecolor'
 normal_name='tripo_node_ac6f65c9-e066-458e-a392-e8172d5f84d9_Normal_Bake' if name=='Toy' else 'beige_gecko_3d_model_normal'
 base_path=ROOT+'/Textures/T_'+name+'_BaseColor'
 base_tex=unreal.load_asset(base_path) if assets.does_asset_exist(base_path) else assets.duplicate_asset(original+base_name,base_path)
 base_tex.set_editor_property('max_texture_size',2048);assert assets.save_loaded_asset(base_tex,only_if_is_dirty=False)
 mask_path=ROOT+'/Textures/T_'+name+'_SurfaceMasks'
 if not assets.does_asset_exist(mask_path):
  task=unreal.AssetImportTask();task.set_editor_property('filename',str(BASE/'SourceArt/Lizards'/name/'Textures'/('T_'+name+'_SurfaceMasks.png')));task.set_editor_property('destination_path',ROOT+'/Textures');task.set_editor_property('automated',True);task.set_editor_property('save',True);tools.import_asset_tasks([task])
 mask_tex=unreal.load_asset(mask_path);assert mask_tex
 mask_tex.set_editor_property('srgb',False);mask_tex.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_MASKS);assert assets.save_loaded_asset(mask_tex,only_if_is_dirty=False)
 m=tools.create_asset('M_'+name+'Gecko',ROOT+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
 tex=[]
 for i,(texture,sampler) in enumerate([(base_tex,unreal.MaterialSamplerType.SAMPLERTYPE_COLOR),(unreal.load_asset(original+normal_name),unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL),(mask_tex,unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)]):
  node=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionTextureSample,-700,i*220);node.set_editor_property('texture',texture);node.set_editor_property('sampler_type',sampler);tex.append(node)
 color=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionVectorParameter,-700,-200);color.set_editor_property('parameter_name','BodyColor');color.set_editor_property('default_value',unreal.LinearColor(1,1,1,1))
 mul=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionMultiply,-300,0)
 unreal.MaterialEditingLibrary.connect_material_expressions(tex[0],'RGB',mul,'A');unreal.MaterialEditingLibrary.connect_material_expressions(color,'RGB',mul,'B');unreal.MaterialEditingLibrary.connect_material_property(mul,'',unreal.MaterialProperty.MP_BASE_COLOR)
 unreal.MaterialEditingLibrary.connect_material_property(tex[1],'RGB',unreal.MaterialProperty.MP_NORMAL);unreal.MaterialEditingLibrary.connect_material_property(tex[2],'R',unreal.MaterialProperty.MP_ROUGHNESS)
 metal=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionMultiply,-300,500);metal.set_editor_property('const_b',.025);unreal.MaterialEditingLibrary.connect_material_expressions(tex[2],'G',metal,'A');unreal.MaterialEditingLibrary.connect_material_property(metal,'',unreal.MaterialProperty.MP_METALLIC)
 unreal.MaterialEditingLibrary.recompile_material(m);assert assets.save_loaded_asset(m,only_if_is_dirty=False)
 return m

def author_animbp(name):
 sk=mesh_asset(name);path=ROOT+'/'+name+'/ABP_'+name+'Gecko'
 if not assets.does_asset_exist(path):
  factory=unreal.AnimBlueprintFactory();factory.set_editor_property('target_skeleton',sk.get_editor_property('skeleton'));factory.set_editor_property('parent_class',unreal.LizardAnimInstance)
  tools.create_asset('ABP_'+name+'Gecko',ROOT+'/'+name,unreal.AnimBlueprint,factory)
 bp=unreal.load_asset(path)
 assert unreal.LizardAnimInstance.author_animation_graph(bp,animation_asset(name,'Idle'),animation_asset(name,'Crawl')),name
 defaults=B.get_default_object(bp);defaults.set_editor_property('IdleClip',animation_asset(name,'Idle'));defaults.set_editor_property('MovementClip',animation_asset(name,'Crawl'))
 defaults.set_editor_property('root_motion_mode',unreal.RootMotionMode.IGNORE_ROOT_MOTION)
 save_bp(bp)

def configure_actor(d,name,tutorial=False):
 d.set_editor_property('AnimatedSkeletalMesh',mesh_asset(name));d.set_editor_property('AnimationBlueprintClass',anim_bp(name).generated_class())
 d.set_editor_property('IdleAnimation',animation_asset(name,'Idle'));d.set_editor_property('MovementAnimation',animation_asset(name,'Crawl'))
 d.set_editor_property('AnimationMovementThreshold',1.);d.set_editor_property('AnimationBlendSeconds',.12)
 report=json.loads((BASE/'SourceArt/Lizards'/name/'Rig-Report.json').read_text())
 d.set_editor_property('AnimationReferenceSpeed',report['reference_speed_cm_s']);d.set_editor_property('AnimationMinPlayRate',.25);d.set_editor_property('AnimationMaxPlayRate',8)
 d.set_editor_property('AnimatedMeshRotation',unreal.Rotator());d.set_editor_property('AnimatedMeshScale',unreal.Vector(1,1,1));d.set_editor_property('AnimatedMeshContactOffset',unreal.Vector(0,0,0 if tutorial else -9))
 if d.get_name().startswith('Default__'):d.set_editor_property('NormalFeedbackColor',unreal.Vector(1,1,1))

def blueprints_phase():
 for name in ['Toy','Real']:
  mat=surface_material(name);sk=mesh_asset(name);slots=list(sk.get_editor_property('materials'));slots[0].set_editor_property('material_interface',mat);sk.set_editor_property('materials',slots);assert assets.save_loaded_asset(sk,only_if_is_dirty=False);author_animbp(name)
 bp=unreal.load_asset('/Game/CatchTheLizard/BP_Lizard');d=B.get_default_object(bp)
 comps={c.get_name().replace('_GEN_VARIABLE',''):c for c in A.get_components(d)}
 if 'LizardAnimatedBody' not in comps:
  c=A.add_component(bp,unreal.SkeletalMeshComponent.static_class(),'LizardAnimatedBody');A.set_parent_component(c,A.get_root_component(d))
 else:c=comps['LizardAnimatedBody']
 c.set_skeletal_mesh_asset(mesh_asset('Real'));c.set_anim_instance_class(anim_bp('Real').generated_class());c.set_collision_profile_name('NoCollision');c.set_editor_property('generate_overlap_events',False)
 c.set_cast_shadow(False);c.set_editor_property('visibility_based_anim_tick_option',unreal.VisibilityBasedAnimTickOption.ONLY_TICK_POSE_WHEN_RENDERED)
 comps['LizardBody'].set_visibility(False);comps['LizardBody'].set_hidden_in_game(True)
 if 'FacingIndicator' in comps:comps['FacingIndicator'].set_hidden_in_game(True)
 configure_actor(B.get_default_object(bp),'Real');save_bp(bp)
 for path,tutorial in [('/Game/CatchTheLizard/Tutorial/BP_TrainingLizardDoll',True),('/Game/CatchTheLizard/Missions/BP_MovingToyLizard',False)]:
  child=unreal.load_asset(path);configure_actor(B.get_default_object(child),'Toy',tutorial)
  for c in A.get_components(B.get_default_object(child)):
   if c.get_name().replace('_GEN_VARIABLE','')=='WindUpKey':c.set_visibility(False);c.set_hidden_in_game(True)
  save_bp(child)


def maps_phase():
 report={}
 for mapname,tutorial in [('Map_TrainingFacility',True),('Map_CatchTheLizard',False),('Map_CatchTheLizard_Level2',False)]:
  assert ls.load_level('/Game/Maps/'+mapname)
  world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world();unreal.SystemLibrary.execute_console_command(world,'save.FixupStandaloneFlags 1')
  lizards=[a for a in aes.get_all_level_actors() if isinstance(a,unreal.LizardGameplayActor)]
  records=[]
  for a in lizards:
   name='Real' if mapname=='Map_CatchTheLizard_Level2' else 'Toy';configure_actor(a,name,tutorial)
   if not tutorial:a.set_actor_rotation(unreal.MathLibrary.make_rot_from_xz(a.get_editor_property('MoveDirection'),a.get_editor_property('SurfaceNormal')),True)
   # Re-run native construction using the saved root transform; no gameplay actor is replaced.
   c=next(c for c in a.get_components_by_class(unreal.SkeletalMeshComponent) if c.get_name()=='LizardAnimatedBody')
   for old in a.get_components_by_class(unreal.StaticMeshComponent):
    if old.get_name() in ['LizardBody','WindUpKey']:old.set_visibility(False);old.set_hidden_in_game(True)
   c.set_skeletal_mesh_asset(mesh_asset(name));c.set_anim_instance_class(anim_bp(name).generated_class());c.set_relative_location(a.get_editor_property('AnimatedMeshContactOffset'),False,False);c.set_relative_rotation(a.get_editor_property('AnimatedMeshRotation'),False,False);c.set_relative_scale3d(a.get_editor_property('AnimatedMeshScale'))
   for arrow in a.get_components_by_class(unreal.ArrowComponent):
    if arrow.get_name()=='FacingIndicator':arrow.set_hidden_in_game(True)
   assert c.get_skeletal_mesh_asset()==mesh_asset(name)
   records.append({'actor':a.get_path_name(),'model':c.get_skeletal_mesh_asset().get_path_name(),'transform':str(c.get_relative_transform()),'head_socket':str(c.get_socket_location('head')),'root_socket':str(c.get_socket_location('root')),'training':a.get_editor_property('bTrainingDoll')})
  assert ls.save_current_level();report[mapname]=records
 assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
 (BASE/'Saved/LizardAnimationCheckpoint/Unreal-Integration.json').write_text(json.dumps(report,indent=2))
 print('LIZARD_MAPS_SAVED',json.dumps(report))

phase=globals().get('LIZARD_ANIMATION_PHASE','meshes')
if phase=='meshes':
 for name in ['Toy','Real']:import_fbx(name)
elif phase=='animations':
 for name in ['Toy','Real']:
  for kind in ['Idle','Crawl']:import_fbx(name,kind)
elif phase=='blueprints':blueprints_phase()
elif phase=='maps':maps_phase()
else:raise ValueError(phase)
assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
print('LIZARD_ANIMATION_PHASE_SAVED',phase)



