import bpy
from pathlib import Path
ROOT=Path(r'E:/CatchTheLizardVR/CatchTheLizardVRGame/CTLProject/SourceArt/Lizards')
for name,frames in [('Toy',24),('Real',16)]:
 path=ROOT/name/(name+'_Gecko_Rigged.blend');bpy.ops.wm.open_mainfile(filepath=str(path));scene=bpy.context.scene;rig=bpy.data.objects[name+'_GeckoRig'];rig.animation_data.action=bpy.data.actions[name+'_Crawl'];scene.frame_start=1;scene.frame_end=frames;scene.frame_set(1)
 controls=rig.data.collections.get('Foot Controls') or rig.data.collections.new('Foot Controls');deform=rig.data.collections.get('Deform Skeleton') or rig.data.collections.new('Deform Skeleton')
 for bone in rig.data.bones:
  (controls if bone.name.endswith('_IK') else deform).assign(bone);bone.color.palette='THEME03' if bone.name.endswith('_IK') else 'DEFAULT'
 for screen in bpy.data.screens:
  for area in screen.areas:
   if area.type=='VIEW_3D':
    area.spaces.active.region_3d.view_perspective='CAMERA';area.spaces.active.shading.type='MATERIAL'
 bpy.ops.object.select_all(action='DESELECT');rig.select_set(True);bpy.context.view_layer.objects.active=rig;bpy.ops.object.mode_set(mode='POSE')
 note=bpy.data.texts.get('README_RIG') or bpy.data.texts.new('README_RIG');note.clear();note.write('Catch The Lizard: '+name+' rig\nSpace: play the in-place crawl loop.\nAction Editor: choose '+name+'_Idle or '+name+'_Crawl.\nGreen foot IK controls: FL_IK, FR_IK, HL_IK, HR_IK.\n22 deform bones; controls are excluded from FBX export. Root is stationary.\nThe last duplicate endpoint key is retained in the action and FBX, but excluded from the preview playback range.\nTextures are packed. Original FBX/textures remain in ../Originals.\nSource scripts and tuning guide: Docs/Lizard-Rigging-and-Animation.md\n')
 bpy.ops.wm.save_as_mainfile(filepath=str(path));print('EDITABLE_PREVIEW_READY',name,flush=True)
