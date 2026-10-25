import bpy,json,sys,math
from pathlib import Path
from mathutils import Vector
OUT=Path(r'E:/CatchTheLizardVR/CatchTheLizardVRGame/CTLProject/Saved/LizardAnimationCheckpoint')
sources={'Toy':r'C:/Users/drago/Downloads/green+plush+gecko+3d+model/tripo_convert_3ecacbdb-2f0e-4ec7-b3e7-2d26535b6fb4.fbx','Real':r'C:/Users/drago/Downloads/beige+gecko+3d+model/tripo_convert_0bb5d3c7-433f-4e7e-aced-91f2365c38d8.fbx'}
import faulthandler
fault_log=open(OUT/'Blender-Stack.txt','w');faulthandler.dump_traceback_later(30,repeat=True,file=fault_log)
report={}
for name,path in sources.items():
 bpy.ops.wm.read_factory_settings(use_empty=True)
 (OUT/(name+'-stage.txt')).write_text('importing')
 bpy.ops.import_scene.fbx(filepath=path,use_image_search=False)
 (OUT/(name+'-stage.txt')).write_text('imported')
 meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
 coords=[o.matrix_world@v.co for o in meshes for v in o.data.vertices]
 lo=Vector(tuple(min(v[i] for v in coords) for i in range(3)));hi=Vector(tuple(max(v[i] for v in coords) for i in range(3)));center=(lo+hi)/2;size=max(hi-lo)
 report[name]={'source':path,'bounds_min':list(lo),'bounds_max':list(hi),'objects':[{'name':o.name,'vertices':len(o.data.vertices),'triangles':sum(len(p.vertices)-2 for p in o.data.polygons),'location':list(o.location),'rotation':list(o.rotation_euler),'scale':list(o.scale),'materials':[m.name if m else None for m in o.data.materials],'uv_layers':[l.name for l in o.data.uv_layers]} for o in meshes],'images':[{'name':im.name,'filepath':im.filepath,'size':list(im.size)} for im in bpy.data.images]}
 (OUT/(name+'-inspection.json')).write_text(json.dumps(report[name],indent=2))
 (OUT/(name+'-stage.txt')).write_text('rendering')
 scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=8;scene.cycles.device='CPU';scene.render.resolution_x=850;scene.render.resolution_y=850;scene.render.resolution_percentage=100
 scene.world=bpy.data.worlds.new('InspectionWorld');scene.world.use_nodes=True;scene.world.node_tree.nodes.get('Background').inputs[0].default_value=(.15,.15,.15,1)
 scene.world.node_tree.nodes.get('Background').inputs[1].default_value=.7
 for offset,energy in [((2,-2,3),1000),((-2,1,2),700)]:
  data=bpy.data.lights.new('InspectionLight','AREA');data.energy=energy*size*size;data.shape='DISK';data.size=size*2
  light=bpy.data.objects.new(data.name,data);scene.collection.objects.link(light);light.location=center+Vector(offset)*size;light.rotation_euler=(center-light.location).to_track_quat('-Z','Y').to_euler()
 camdata=bpy.data.cameras.new('InspectionCamera');cam=bpy.data.objects.new('InspectionCamera',camdata);scene.collection.objects.link(cam);scene.camera=cam;camdata.type='ORTHO';camdata.ortho_scale=size*1.3
 for view,offset in [('top',(0,0,3)),('perspective',(2,-2,2))]:
  cam.location=center+Vector(offset)*size;cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler();scene.render.filepath=str(OUT/(name+'-source-'+view+'.png'));bpy.ops.render.render(write_still=True)
 (OUT/(name+'-inspection.json')).write_text(json.dumps(report[name],indent=2))
print('LIZARD_SOURCE_INSPECTION',json.dumps(report))
