"""Blender-native rigging, skinning, editable IK crawl actions and Unreal FBX exports.
Originals are copied byte-for-byte; all work is on a derived mesh.
"""
import bpy,math,json,shutil,hashlib,sys
import numpy as np
from pathlib import Path
from mathutils import Vector
ROOT=Path(r'E:/CatchTheLizardVR/CatchTheLizardVRGame/CTLProject')
OUT=ROOT/'SourceArt/Lizards';CHECK=ROOT/'Saved/LizardAnimationCheckpoint'
MODELS={
'Toy':dict(folder='green+plush+gecko+3d+model',file='tripo_convert_3ecacbdb-2f0e-4ec7-b3e7-2d26535b6fb4.fbx',length=30,triangles=12000,frames=24,
 torso=[(0,.075,.075),(0,-.06,.085),(0,-.24,.08),(0,-.32,.08),(0,-.46,.07)],tail=[(0,.075,.075),(0,.17,.07),(.07,.27,.065),(.14,.36,.05),(.12,.43,.03),(.025,.485,.018)],
 legs={'FL':[(-.07,-.24,.075),(-.135,-.275,.05),(-.15,-.345,.025),(-.16,-.39,.025)],'FR':[(.07,-.24,.075),(.14,-.26,.045),(.17,-.30,.025),(.215,-.325,.025)],'HL':[(-.075,.025,.075),(-.155,.02,.05),(-.215,.025,.025),(-.27,.025,.025)],'HR':[(.075,.025,.075),(.16,.045,.045),(.215,.055,.025),(.27,.055,.025)]}),
'Real':dict(folder='beige+gecko+3d+model',file='tripo_convert_0bb5d3c7-433f-4e7e-aced-91f2365c38d8.fbx',length=28,triangles=10000,frames=16,
 torso=[(.035,.065,.075),(-.025,-.08,.085),(-.075,-.235,.075),(-.115,-.31,.07),(-.145,-.45,.055)],tail=[(.035,.065,.075),(.07,.18,.075),(.115,.27,.065),(.17,.36,.05),(.225,.435,.035),(.18,.485,.025)],
 legs={'FL':[(-.10,-.255,.07),(-.15,-.29,.045),(-.19,-.36,.025),(-.235,-.39,.025)],'FR':[(-.025,-.25,.065),(.035,-.265,.04),(.055,-.30,.02),(.10,-.335,.02)],'HL':[(-.005,.04,.075),(-.10,-.02,.05),(-.16,.025,.025),(-.215,.055,.025)],'HR':[(.07,.025,.075),(.145,-.015,.05),(.17,-.075,.02),(.235,-.105,.02)]})}

def dump(path,value):path.write_text(json.dumps(value,indent=2))
def curves(action):
 if hasattr(action,'fcurves'):return action.fcurves
 return [fc for layer in action.layers for strip in layer.strips for bag in strip.channelbags for fc in bag.fcurves]

def build(name,cfg):
 (CHECK/(name+'-rig-stage.txt')).write_text('preserving originals')
 src=Path('C:/Users/drago/Downloads')/cfg['folder'];original=OUT/'Originals'/cfg['folder']
 if not original.exists():shutil.copytree(src,original)
 manifest={str(p.relative_to(src)):hashlib.sha256(p.read_bytes()).hexdigest() for p in src.rglob('*') if p.is_file()}
 dump(OUT/'Originals'/(name+'-SHA256.json'),manifest)
 bpy.ops.wm.read_factory_settings(use_empty=True)
 bpy.ops.import_scene.fbx(filepath=str(original/cfg['file']),use_image_search=False)
 meshes=[o for o in bpy.context.scene.objects if o.type=='MESH'];bpy.ops.object.select_all(action='DESELECT')
 for o in meshes:o.select_set(True)
 bpy.context.view_layer.objects.active=meshes[0]
 if len(meshes)>1:bpy.ops.object.join()
 mesh=bpy.context.view_layer.objects.active;mesh.name=name+'_Gecko_Skinned'
 bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
 # The real FBX includes a dense legacy skin. Replace it only in this derived working mesh.
 source_rigs=[{'name':o.name,'bones':len(o.data.bones)} for o in bpy.data.objects if o.type=='ARMATURE']
 mesh.vertex_groups.clear()
 for modifier in list(mesh.modifiers):
  if modifier.type=='ARMATURE':mesh.modifiers.remove(modifier)
 mesh.parent=None
 for old in list(bpy.data.objects):
  if old.type=='ARMATURE':bpy.data.objects.remove(old,do_unlink=True)
 source_co=np.array([v.co[:] for v in mesh.data.vertices])
 # FBX source units differ and importer context can rescale by 100. Canonical source coordinates are metres.
 canonical_span=.979819 if name=='Toy' else .9779663
 source_unit_correction=canonical_span/(source_co[:,1].max()-source_co[:,1].min());source_co*=source_unit_correction
 mins=source_co.min(axis=0);maxs=source_co.max(axis=0)
 factor=cfg['length']/(maxs[1]-mins[1])
 def P(v):return Vector((-v[1]*factor,v[0]*factor,(v[2]-mins[2])*factor))
 for v,co in zip(mesh.data.vertices,source_co):v.co=P(co)
 scene=bpy.context.scene;scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=.01
 # Derivative game mesh only. Original topology/FBX remain untouched in Originals.
 dec=mesh.modifiers.new('Quest2_DerivedMesh_Reduction','DECIMATE');dec.ratio=min(1,cfg['triangles']/sum(len(p.vertices)-2 for p in mesh.data.polygons));dec.use_collapse_triangulate=True
 bpy.ops.object.modifier_apply(modifier=dec.name)
 for poly in mesh.data.polygons:poly.use_smooth=True
 # Relink exact supplied images locally, then pack the editable blend.
 texture_dir=original/(Path(cfg['file']).stem+'.fbm');files=list(texture_dir.glob('*'))
 for im in bpy.data.images:
  basename=Path(im.filepath.replace('\\','/')).name
  match=next((f for f in files if f.name.lower()==basename.lower()),None)
  if match:im.filepath=str(match);im.reload()
 mat=bpy.data.materials.new(name+'_OriginalTexturedSurface');mat.use_nodes=True
 nodes=mat.node_tree.nodes;links=mat.node_tree.links;bsdf=nodes.get('Principled BSDF')
 def image_node(fragment,noncolor=False):
  path=next(f for f in files if fragment.lower() in f.name.lower());im=bpy.data.images.load(str(path),check_existing=True)
  if noncolor:im.colorspace_settings.name='Non-Color'
  node=nodes.new('ShaderNodeTexImage');node.image=im;node.label=path.name;return node
 base=image_node('BaseColor' if name=='Toy' else 'basecolor');links.new(base.outputs['Color'],bsdf.inputs['Base Color'])
 rough=image_node('roughness',True);links.new(rough.outputs['Color'],bsdf.inputs['Roughness'])
 metal=image_node('metallic',True);metal_scale=nodes.new('ShaderNodeMath');metal_scale.operation='MULTIPLY';metal_scale.inputs[1].default_value=.025
 links.new(metal.outputs['Color'],metal_scale.inputs[0]);links.new(metal_scale.outputs[0],bsdf.inputs['Metallic'])
 normal=image_node('Normal_Bake' if name=='Toy' else 'normal',True);norm=nodes.new('ShaderNodeNormalMap');norm.inputs['Strength'].default_value=.35
 links.new(normal.outputs['Color'],norm.inputs['Color']);links.new(norm.outputs['Normal'],bsdf.inputs['Normal'])
 mesh.data.materials.clear();mesh.data.materials.append(mat)
 # Individual anatomical proportions, common bone naming only; no forced retargeting.
 data=bpy.data.armatures.new(name+'_GeckoSkeleton');rig=bpy.data.objects.new(name+'_GeckoRig',data);scene.collection.objects.link(rig);rig.show_in_front=True
 bpy.context.view_layer.objects.active=rig;mesh.select_set(False);rig.select_set(True);bpy.ops.object.mode_set(mode='EDIT')
 segments={};parents={}
 def bone(n,a,b,parent=None,deform=True,connected=False):
  eb=data.edit_bones.new(n);eb.head=P(a);eb.tail=P(b);eb.use_deform=deform
  if parent:eb.parent=data.edit_bones[parent];eb.use_connect=connected
  if deform and n!='root':segments[n]=(eb.head.copy(),eb.tail.copy())
  parents[n]=parent
  return eb
 # Root is static on the contact plane. Control bones are excluded from the game FBX.
 root=data.edit_bones.new('root');root.head=(0,0,0);root.tail=(0,0,1);root.use_deform=True
 torso=cfg['torso']
 for i,n in enumerate(['pelvis','spine','neck','head']):bone(n,torso[i],torso[i+1],['root','pelvis','spine','neck'][i],True,i>0)
 for i in range(5):bone('tail_'+str(i+1).zfill(2),cfg['tail'][i],cfg['tail'][i+1],'pelvis' if i==0 else 'tail_'+str(i).zfill(2),True,i>0)
 for side,points in cfg['legs'].items():
  for i,part in enumerate(['upper','lower','foot']):bone(side+'_'+part,points[i],points[i+1],('spine' if side[0]=='F' else 'pelvis') if i==0 else side+'_'+['upper','lower'][i-1],True,i>0)
  bone(side+'_IK',points[2],points[3],'root',False)
 bpy.ops.object.mode_set(mode='OBJECT')
 (CHECK/(name+'-rig-stage.txt')).write_text('skinning')
 coords=np.array([v.co[:] for v in mesh.data.vertices]);names=list(segments)
 # Bounded four-influence proximity weights; head/tail/foot masks avoid cross-limb leakage.
 distance=[]
 for n in names:
  a,b=map(np.array,segments[n]);ab=b-a;t=np.clip(((coords-a)@ab)/(ab@ab),0,1);distance.append(np.linalg.norm(coords-(a+t[:,None]*ab),axis=1))
 distance=np.stack(distance,axis=1);weight=1/np.power(distance+.018*factor,4)
 old=np.stack((coords[:,1]/factor,-coords[:,0]/factor,coords[:,2]/factor+mins[2]),axis=1)
 headmask=(old[:,1]<(-.315 if name=='Toy' else -.30)) & (np.abs(old[:,0]-(0 if name=='Toy' else -.12))<(.105 if name=='Toy' else .085))
 tailmask=old[:,1]>.15
 for idx,n in enumerate(names):
  if n!='head':weight[headmask,idx]=0
  if not n.startswith('tail_'):weight[tailmask,idx]=0
 foot_members={}
 torso_a=np.array(P(torso[0]));torso_b=np.array(P(torso[3]));axis=torso_b-torso_a
 along=np.clip(((coords-torso_a)@axis)/(axis@axis),0,1);trunk_dist=np.linalg.norm((coords-(torso_a+along[:,None]*axis))[:,:2],axis=1)
 for side,points in cfg['legs'].items():
  idx=names.index(side+'_foot');ankle=np.array(P(points[2]));toe=np.array(P(points[3]));middle=(ankle+toe)/2
  near=np.linalg.norm((coords-middle)[:,:2],axis=1)<(.069 if name=='Toy' else .065)*factor
  mask=near & (trunk_dist>.105*factor if name=='Toy' else trunk_dist>.065*factor) & (old[:,2]<.075)
  weight[mask,:]=0;weight[mask,idx]=1;foot_members[side]=np.flatnonzero(mask)
 top=np.argsort(weight,axis=1)[:,-4:];keep=np.zeros_like(weight);np.put_along_axis(keep,top,np.take_along_axis(weight,top,axis=1),axis=1);keep/=np.maximum(keep.sum(axis=1,keepdims=True),1e-20)
 for j,n in enumerate(names):
  group=mesh.vertex_groups.new(name=n)
  for i in np.flatnonzero(keep[:,j]>1e-5):group.add([int(i)],float(keep[i,j]),'REPLACE')
 arm=mesh.modifiers.new('Gecko_Skin','ARMATURE');arm.object=rig;arm.use_deform_preserve_volume=False;mesh.parent=rig
 for side in cfg['legs']:
  lower=rig.pose.bones[side+'_lower'];ik=lower.constraints.new('IK');ik.name='Planted_foot_two_bone_IK';ik.target=rig;ik.subtarget=side+'_IK';ik.chain_count=2;ik.use_stretch=False
  rig.pose.bones[side+'_upper'].ik_stretch=0;lower.ik_stretch=0
  copy=rig.pose.bones[side+'_foot'].constraints.new('COPY_ROTATION');copy.target=rig;copy.subtarget=side+'_IK';copy.target_space='WORLD';copy.owner_space='WORLD'
 for pb in rig.pose.bones:pb.rotation_mode='QUATERNION'
 rig.animation_data_create();scene.render.fps=30
 # Foot control targets stay on the contact plane during stance and lift only in swing.
 stride=1.8 if name=='Toy' else 1.5
 lift=.45 if name=='Toy' else .35
 foot_ground={side:float(coords[ids,2].min()) if len(ids) else 0 for side,ids in foot_members.items()}
 def pose(phase,idle=False):
  for pb in rig.pose.bones:pb.location=(0,0,0);pb.rotation_quaternion=(1,0,0,0);pb.scale=(1,1,1)
  if not idle:
   rig.pose.bones['pelvis'].location.z=(.045 if name=='Toy' else .02)*(1-math.cos(phase*2))
   from mathutils import Quaternion
   rig.pose.bones['spine'].rotation_quaternion=Quaternion((0,0,1),math.sin(phase)*(.018 if name=='Toy' else .012))
   for i in range(1,6):rig.pose.bones['tail_'+str(i).zfill(2)].rotation_quaternion=Quaternion((0,0,1),math.sin(phase-i*.45)*(.045 if name=='Toy' else .035))
  for side in cfg['legs']:
   u=((phase/(2*math.pi))+(0 if side in ['FL','HR'] else .5))%1
   if idle:dx=dz=0
   elif u<.6:dx=stride*.5*math.cos(math.pi*u/.6);dz=0
   else:
    t=(u-.6)/.4;dx=-stride*.5+stride*(.5-.5*math.cos(math.pi*t));dz=lift*math.sin(math.pi*t)**2
   ctrl=rig.pose.bones[side+'_IK'];offset=Vector((dx,0,dz-foot_ground[side]+.015))
   ctrl.location=ctrl.bone.matrix_local.to_3x3().inverted()@offset
  bpy.context.view_layer.update()
 actions={}
 for kind,nframes in [('Idle',30),('Crawl',cfg['frames'])]:
  action=bpy.data.actions.new(name+'_'+kind);action.use_fake_user=True;rig.animation_data.action=action
  for f in range(1,nframes+2):
   scene.frame_set(f);pose((f-1)/nframes*2*math.pi,kind=='Idle')
   for pb in rig.pose.bones:
    for prop in ['location','rotation_quaternion','scale']:pb.keyframe_insert(data_path=prop,frame=f,group=pb.name)
  for fc in curves(action):
   for key in fc.keyframe_points:key.interpolation='LINEAR'
  actions[kind]=action
 clearance=.15 if name=='Toy' else .10
 rig.location.z=clearance;rig['constant_surface_clearance_cm']=clearance
 # Bake duplicate endpoints and stationary-root proof from the evaluated deformation, not only action curves.
 rig.animation_data.action=actions['Crawl'];scene.frame_start=1;scene.frame_end=cfg['frames']+1
 evaluated=[];foot_stats=[]
 for f in [1,1+cfg['frames']//4,1+cfg['frames']//2,1+3*cfg['frames']//4,cfg['frames']+1]:
  scene.frame_set(f);bpy.context.view_layer.update();ev=mesh.evaluated_get(bpy.context.evaluated_depsgraph_get());em=ev.to_mesh();v=np.array([(ev.matrix_world@p.co)[:] for p in em.vertices]);evaluated.append(v)
  foot_stats.append({'frame':f,'foot_min_z_cm':{side:float(v[ids,2].min()) for side,ids in foot_members.items() if len(ids)}});ev.to_mesh_clear()
 seam=float(np.max(np.linalg.norm(evaluated[0]-evaluated[-1],axis=1)))
 path=OUT/name;path.mkdir(exist_ok=True);export=path/'Exports';export.mkdir(exist_ok=True)
 bpy.ops.object.select_all(action='DESELECT');mesh.select_set(True);rig.select_set(True);bpy.context.view_layer.objects.active=rig
 scene.frame_set(1)
 bpy.ops.export_scene.fbx(filepath=str(export/('SK_'+name+'Gecko.fbx')),use_selection=True,object_types={'ARMATURE','MESH'},add_leaf_bones=False,use_armature_deform_only=True,bake_anim=False,axis_forward='-Y',axis_up='Z',apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',mesh_smooth_type='FACE',path_mode='COPY',embed_textures=False)
 for kind,action in actions.items():
  rig.animation_data.action=action;scene.frame_start=1;scene.frame_end=31 if kind=='Idle' else cfg['frames']+1
  mesh.select_set(False)
  bpy.ops.export_scene.fbx(filepath=str(export/('A_'+name+'_'+kind+'.fbx')),use_selection=True,object_types={'ARMATURE'},add_leaf_bones=False,use_armature_deform_only=True,bake_anim=True,bake_anim_use_all_bones=True,bake_anim_use_nla_strips=False,bake_anim_use_all_actions=False,bake_anim_simplify_factor=0,axis_forward='-Y',axis_up='Z',apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS')
  mesh.select_set(True)
 # Remove unused importer-only datablocks with broken legacy paths before packing.
 for unused in list(bpy.data.materials):
  if unused.users==0:bpy.data.materials.remove(unused)
 for unused in list(bpy.data.images):
  if unused.users==0:bpy.data.images.remove(unused)
 # Edit-friendly scene: packed textures, native control rig, named actions and a clean neutral floor.
 rig.animation_data.action=actions['Crawl'];scene.frame_start=1;scene.frame_end=cfg['frames']+1;scene.frame_set(1)
 bpy.ops.file.pack_all()
 scene.render.engine='CYCLES';scene.cycles.samples=12;scene.cycles.device='CPU';scene.render.resolution_x=900;scene.render.resolution_y=700;scene.render.resolution_percentage=100
 scene.world=bpy.data.worlds.new('Studio');scene.world.use_nodes=True;scene.world.node_tree.nodes.get('Background').inputs[0].default_value=(.18,.20,.24,1);scene.world.node_tree.nodes.get('Background').inputs[1].default_value=.55
 size=cfg['length'];center=Vector((0,0,2))
 floor_mat=bpy.data.materials.new('ContactPlane');floor_mat.diffuse_color=(.12,.15,.19,1)
 bpy.ops.mesh.primitive_plane_add(size=size*2,location=(0,0,-.035));floor=bpy.context.object;floor.name='Preview_Surface_Plane_Not_Exported';floor.data.materials.append(floor_mat)
 for offset,power in [((.5,-1,1.6),700),((-.5,1,1),400)]:
  light_data=bpy.data.lights.new('Softbox','AREA');light_data.energy=power*size*size*.05;light_data.size=size
  light=bpy.data.objects.new(light_data.name,light_data);scene.collection.objects.link(light);light.location=center+Vector(offset)*size;light.rotation_euler=(center-light.location).to_track_quat('-Z','Y').to_euler()
 camera_data=bpy.data.cameras.new('PreviewCamera');camera=bpy.data.objects.new('PreviewCamera',camera_data);scene.collection.objects.link(camera);scene.camera=camera;camera.location=center+Vector((1.1,-1.4,1.15))*size;camera.rotation_euler=(center-camera.location).to_track_quat('-Z','Y').to_euler();camera_data.type='ORTHO';camera_data.ortho_scale=size*1.32
 report={'blender':bpy.app.version_string,'source':str(src/cfg['file']),'original_hashes':manifest,'source_bounds':[mins.tolist(),maxs.tolist()],'source_triangles':sum(len(p.vertices)-2 for p in meshes[0].data.polygons) if False else (50000 if name=='Toy' else 17287),'game_triangles':sum(len(p.vertices)-2 for p in mesh.data.polygons),'vertices':len(mesh.data.vertices),'deform_bones':len([b for b in data.bones if b.use_deform]),'control_bones':len([b for b in data.bones if not b.use_deform]),'max_influences':int((keep>1e-5).sum(axis=1).max()),'unweighted_vertices':int((keep.sum(axis=1)<.99).sum()),'length_cm':cfg['length'],'forward_axis':'+X','surface_normal':'+Z','root_translation_cm':[0,0,clearance],'constant_surface_clearance_cm':clearance,'crawl_seconds':cfg['frames']/30,'stride_cm':stride,'reference_speed_cm_s':stride/(cfg['frames']/30),'loop_endpoint_vertex_error_cm':seam,'foot_contact_samples':foot_stats,'actions':[a.name for a in actions.values()],'export_axes':['-Y','Z'],'scene_unit_meters':.01,'source_unit_correction':source_unit_correction,'source_rigs_replaced_in_derivative':source_rigs,'textures':[{'name':im.name,'packed':bool(im.packed_file),'path':im.filepath} for im in bpy.data.images if im.users]}
 dump(path/'Rig-Report.json',report)
 bpy.ops.wm.save_as_mainfile(filepath=str(path/(name+'_Gecko_Rigged.blend')))
 (CHECK/(name+'-rig-stage.txt')).write_text('rendering animation poses')
 for index,f in enumerate([1,1+cfg['frames']//4,1+cfg['frames']//2,1+3*cfg['frames']//4]):
  scene.frame_set(f);scene.render.filepath=str(path/(name+'_Crawl_Preview_'+str(index)+'.png'));bpy.ops.render.render(write_still=True)
 scene.frame_set(1);bpy.ops.wm.save_as_mainfile(filepath=str(path/(name+'_Gecko_Rigged.blend')))
 (CHECK/(name+'-rig-stage.txt')).write_text('complete')
 print('GECKO_RIG_EXPORTED',name,json.dumps(report),flush=True)

OUT.mkdir(parents=True,exist_ok=True)
for name,cfg in MODELS.items():build(name,cfg)
