import bpy,numpy as np
from pathlib import Path
root=Path(r'E:/CatchTheLizardVR/CatchTheLizardVRGame/CTLProject/SourceArt/Lizards')
for name,folder,prefix in [('Toy','green+plush+gecko+3d+model','green_plush_gecko_3d_model'),('Real','beige+gecko+3d+model','beige_gecko_3d_model')]:
 files=list((root/'Originals'/folder).rglob('*.PNG'));channels=[]
 for kind in ['roughness','metallic']:
  path=next(p for p in files if kind in p.name.lower());im=bpy.data.images.load(str(path),check_existing=False);im.colorspace_settings.name='Non-Color';im.scale(1024,1024)
  values=np.empty(1024*1024*4,dtype=np.float32);im.pixels.foreach_get(values);channels.append(values.reshape((-1,4))[:,0].copy());bpy.data.images.remove(im)
 pixels=np.ones((1024*1024,4),dtype=np.float32);pixels[:,0]=channels[0];pixels[:,1]=channels[1]
 im=bpy.data.images.new('T_'+name+'_SurfaceMasks',width=1024,height=1024,alpha=True);im.colorspace_settings.name='Non-Color';im.pixels.foreach_set(pixels.ravel());out=root/name/'Textures';out.mkdir(exist_ok=True);im.filepath_raw=str(out/('T_'+name+'_SurfaceMasks.png'));im.file_format='PNG';im.save();print('MASKS_PACKED',name,flush=True)
