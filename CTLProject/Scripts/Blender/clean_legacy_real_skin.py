import bpy,json,numpy as np
from pathlib import Path
root=Path(r'E:/CatchTheLizardVR/CatchTheLizardVRGame/CTLProject/SourceArt/Lizards');name='Real';path=root/name/(name+'_Gecko_Rigged.blend')
bpy.ops.wm.open_mainfile(filepath=str(path));scene=bpy.context.scene;rig=bpy.data.objects[name+'_GeckoRig'];mesh=bpy.data.objects[name+'_Gecko_Skinned']
removed_groups=[]
for group in list(mesh.vertex_groups):
 if group.name not in rig.data.bones:removed_groups.append(group.name);mesh.vertex_groups.remove(group)
for modifier in list(mesh.modifiers):
 if modifier.type=='ARMATURE' and modifier.object!=rig:mesh.modifiers.remove(modifier)
for obj in list(bpy.data.objects):
 if obj.type=='ARMATURE' and obj!=rig:bpy.data.objects.remove(obj,do_unlink=True)
bpy.ops.object.select_all(action='DESELECT');mesh.select_set(True);rig.select_set(True);bpy.context.view_layer.objects.active=rig
scene.frame_set(1)
kwargs=dict(use_selection=True,add_leaf_bones=False,use_armature_deform_only=True,axis_forward='-Y',axis_up='Z',apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS')
bpy.ops.export_scene.fbx(filepath=str(root/name/'Exports'/('SK_'+name+'Gecko.fbx')),object_types={'ARMATURE','MESH'},bake_anim=False,mesh_smooth_type='FACE',path_mode='COPY',embed_textures=False,**kwargs)
mesh.select_set(False)
for kind,end in [('Idle',31),('Crawl',17)]:
 rig.animation_data.action=bpy.data.actions[name+'_'+kind];scene.frame_start=1;scene.frame_end=end
 bpy.ops.export_scene.fbx(filepath=str(root/name/'Exports'/('A_'+name+'_'+kind+'.fbx')),object_types={'ARMATURE'},bake_anim=True,bake_anim_use_all_bones=True,bake_anim_use_nla_strips=False,bake_anim_use_all_actions=False,bake_anim_simplify_factor=0,**kwargs)
rig.animation_data.action=bpy.data.actions[name+'_Crawl'];scene.frame_end=17;scene.frame_set(1);bpy.ops.wm.save_as_mainfile(filepath=str(path))
report_path=root/name/'Rig-Report.json';report=json.loads(report_path.read_text());report['source_legacy_skin_groups_removed']=len(removed_groups);report['max_influences']=max(len(v.groups) for v in mesh.data.vertices);report_path.write_text(json.dumps(report,indent=2))
for i,f in enumerate([1,5,9,13]):
 scene.frame_set(f);scene.render.filepath=str(root/name/(name+'_Crawl_Preview_'+str(i)+'.png'));bpy.ops.render.render(write_still=True)
scene.frame_set(1);bpy.ops.wm.save_as_mainfile(filepath=str(path));print('DERIVED_REAL_SKIN_CLEANED',len(removed_groups),flush=True)
