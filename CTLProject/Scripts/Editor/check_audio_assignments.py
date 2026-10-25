"""Read-only assignment audit; compile/save the affected GameInstance Blueprint.
Run in the editor after building CTLProjectEditor. Does not change the audio mix.
"""
import unreal, json
from pathlib import Path
from editor_toolset.toolsets.blueprint import BlueprintTools as B

bp=unreal.load_asset('/Game/CatchTheLizard/Music/Playback/BP_CTLGameInstance')
B.compile_blueprint(bp, warnings_as_errors=True)
assert bp.get_editor_property('status')==unreal.BlueprintStatus.BS_UP_TO_DATE
assert unreal.EditorAssetLibrary.save_loaded_asset(bp,only_if_is_dirty=False)
instance=B.get_default_object(bp)
music=instance.get_editor_property('MusicLibrary')
sfx=instance.get_editor_property('SoundEffects')
report={'blueprint':bp.get_path_name(),'compiled':True,'music_master':music.get_editor_property('MasterVolume'),'sfx_master':sfx.get_editor_property('MasterVolume'),'maps':[],'effects':{}}
def sound_info(sound):
 assert sound is not None
 return {'asset':sound.get_path_name(),'duration':sound.get_editor_property('duration'),'volume':sound.get_editor_property('volume'),'looping':sound.get_editor_property('looping'),'channels':sound.get_editor_property('num_channels'),'sound_class':str(sound.get_editor_property('sound_class_object'))}
for entry in music.get_editor_property('Maps'):
 report['maps'].append({'assignment':entry.export_text(),'sound':sound_info(entry.get_editor_property('Track'))})
for key in ['UIHover','UIConfirm','ObjectPickup','BroomSwing','SprayLoop','StunActivated','ContainerCapture','TutorialComplete']:
 entry=sfx.get_editor_property(key)
 report['effects'][key]={'assignment':entry.export_text(),'sound':sound_info(entry.get_editor_property('Sound'))}
report['attenuation']=sfx.get_editor_property('WorldAttenuation').get_editor_property('Attenuation').export_text()
output=Path(unreal.Paths.project_saved_dir())/'AudioDiagnosis';output.mkdir(exist_ok=True)
(output/'Assignments.json').write_text(json.dumps(report,indent=2))
print('CTL_AUDIO_ASSIGNMENTS_VALID_AND_BLUEPRINT_SAVED')
