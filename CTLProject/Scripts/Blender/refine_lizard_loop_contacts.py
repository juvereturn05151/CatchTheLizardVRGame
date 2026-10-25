"""Smooth swing/stance tangents, preserving the authored rig, weights and source textures."""
import bpy,math,json
import numpy as np
from pathlib import Path
from mathutils import Vector
ROOT=Path(r'E:/CatchTheLizardVR/CatchTheLizardVRGame/CTLProject/SourceArt/Lizards')
for name,frames,stride,lift in [('Toy',24,1.8,.45),('Real',16,1.5,.35)]:
 path=ROOT/name/(name+'_Gecko_Rigged.blend');bpy.ops.wm.open_mainfile(filepath=str(path));scene=bpy.context.scene
 rig=bpy.data.objects[name+'_GeckoRig'];mesh=bpy.data.objects[name+'_Gecko_Skinned']
 idle=bpy.data.actions[name+'_Idle'];crawl=bpy.data.actions[name+'_Crawl'];rig.animation_data.action=idle;scene.frame_set(1);bpy.context.view_layer.update()
 sides=['FL','FR','HL','HR'];neutral={s:rig.pose.bones[s+'_IK'].location.copy() for s in sides}
 rig.animation_data.action=crawl
 for f in range(1,frames+2):
  scene.frame_set(f);phase=(f-1)/frames
  for side in sides:
   u=(phase+(0 if side in ['FL','HR'] else .5))%1
   if u<.6:dx=stride*.5*math.cos(math.pi*u/.6);dz=0
   else:
    t=(u-.6)/.4;dx=-stride*.5*math.cos(math.pi*t);dz=lift*math.sin(math.pi*t)**2
   ctrl=rig.pose.bones[side+'_IK'];ctrl.location=neutral[side]+ctrl.bone.matrix_local.to_3x3().inverted()@Vector((dx,0,dz));ctrl.keyframe_insert(data_path='location',frame=f,group=ctrl.name)
 for layer in crawl.layers:
  for strip in layer.strips:
   for bag in strip.channelbags:
    for fc in bag.fcurves:
     for key in fc.keyframe_points:key.interpolation='LINEAR'
 scene.frame_start=1;scene.frame_end=frames+1;scene.frame_set(1)
 bpy.ops.object.select_all(action='DESELECT');rig.select_set(True);bpy.context.view_layer.objects.active=rig
 bpy.ops.export_scene.fbx(filepath=str(ROOT/name/'Exports'/('A_'+name+'_Crawl.fbx')),use_selection=True,object_types={'ARMATURE'},add_leaf_bones=False,use_armature_deform_only=True,bake_anim=True,bake_anim_use_all_bones=True,bake_anim_use_nla_strips=False,bake_anim_use_all_actions=False,bake_anim_simplify_factor=0,axis_forward='-Y',axis_up='Z',apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS')
 def vertices(f):
  scene.frame_set(f);bpy.context.view_layer.update();ev=mesh.evaluated_get(bpy.context.evaluated_depsgraph_get());em=ev.to_mesh();v=np.array([p.co[:] for p in em.vertices]);ev.to_mesh_clear();return v
 first=vertices(1);last=vertices(frames+1)
 report_path=ROOT/name/'Rig-Report.json';report=json.loads(report_path.read_text());report['loop_endpoint_vertex_error_cm']=float(np.max(np.linalg.norm(first-last,axis=1)));report['foot_curve']='cosine stance, cosine swing, squared-sine lift; stationary root'
 memberships={}
 for s in sides:
  idx=mesh.vertex_groups[s+'_foot'].index;memberships[s]=np.array([v.index for v in mesh.data.vertices if any(g.group==idx and g.weight>.95 for g in v.groups)])
 report['foot_contact_samples']=[]
 for f in [1,1+frames//4,1+frames//2,1+3*frames//4,frames+1]:
  v=vertices(f);report['foot_contact_samples'].append({'frame':f,'foot_min_z_cm':{s:float(v[ids,2].min()) for s,ids in memberships.items()}})
 report_path.write_text(json.dumps(report,indent=2))
 scene.frame_set(1);bpy.ops.wm.save_as_mainfile(filepath=str(path))
 for i,f in enumerate([1,1+frames//4,1+frames//2,1+3*frames//4]):
  scene.frame_set(f);scene.render.filepath=str(ROOT/name/(name+'_Crawl_Preview_'+str(i)+'.png'));bpy.ops.render.render(write_still=True)
 scene.frame_set(1);bpy.ops.wm.save_as_mainfile(filepath=str(path));print('SMOOTH_LOOP_SAVED',name,report['loop_endpoint_vertex_error_cm'],flush=True)
