"""Final Blender asset validation, not a gameplay automation suite."""
import bpy,numpy as np,json,hashlib
from pathlib import Path
ROOT=Path(r'E:/CatchTheLizardVR/CatchTheLizardVRGame/CTLProject')
results={}
for name,frames in [('Toy',24),('Real',16)]:
 path=ROOT/'SourceArt/Lizards'/name/(name+'_Gecko_Rigged.blend');bpy.ops.wm.open_mainfile(filepath=str(path))
 scene=bpy.context.scene;rig=bpy.data.objects[name+'_GeckoRig'];mesh=bpy.data.objects[name+'_Gecko_Skinned'];rig.animation_data.action=bpy.data.actions[name+'_Crawl']
 low=1000;root_samples=[];poses=[]
 for frame in range(1,frames+2):
  scene.frame_set(frame);bpy.context.view_layer.update();ev=mesh.evaluated_get(bpy.context.evaluated_depsgraph_get());em=ev.to_mesh();v=np.array([(ev.matrix_world@p.co)[:] for p in em.vertices]);assert np.isfinite(v).all();low=min(low,float(v[:,2].min()));poses.append(v);ev.to_mesh_clear();root_samples.append(tuple(rig.matrix_world@rig.pose.bones['root'].matrix.translation))
 seam=float(np.max(np.linalg.norm(poses[0]-poses[-1],axis=1)))
 assert seam<1e-5,(name,seam)
 assert np.max(np.abs(np.diff(np.array(root_samples),axis=0)))<1e-5
 assert low>=-1e-4,(name,'penetration_cm',low)
 assert all(im.packed_file for im in bpy.data.images if im.users)
 weights=[sum(g.weight for g in v.groups) for v in mesh.data.vertices];influences=max(len(v.groups) for v in mesh.data.vertices)
 assert min(weights)>.999 and max(weights)<1.001 and influences<=4
 manifest=json.loads((ROOT/'SourceArt/Lizards/Originals'/(name+'-SHA256.json')).read_text());folder='green+plush+gecko+3d+model' if name=='Toy' else 'beige+gecko+3d+model'
 assert all(hashlib.sha256((Path('C:/Users/drago/Downloads')/folder/p).read_bytes()).hexdigest()==digest for p,digest in manifest.items())
 results[name]={'blend':str(path),'all_frames_sampled':frames+1,'loop_seam_cm':seam,'stationary_root':True,'constant_root_position_cm':root_samples[0],'minimum_mesh_height_cm':low,'max_skin_influences':influences,'packed_textures':True,'original_files_sha256_unchanged':True,'bones':len([b for b in rig.data.bones if b.use_deform]),'actions':[a.name for a in bpy.data.actions]}
(ROOT/'Saved/LizardAnimationCheckpoint/Blender-FinalValidation.json').write_text(json.dumps(results,indent=2));print('BLENDER_FINAL_ASSETS_VERIFIED',json.dumps(results),flush=True)
