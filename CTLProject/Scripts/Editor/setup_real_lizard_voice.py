"""Assign the imported lizard call to real lizards; explicitly silence toy subclasses."""
import unreal,json
from pathlib import Path
from editor_toolset.toolsets.blueprint import BlueprintTools as B

assets=unreal.EditorAssetLibrary
sound=unreal.load_asset('/Game/CatchTheLizard/Sfx/lizard_sound')
assert sound is not None
assert not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor()
report={}
for path,enabled in [('/Game/CatchTheLizard/BP_Lizard',True),
                     ('/Game/CatchTheLizard/Missions/BP_MovingToyLizard',False),
                     ('/Game/CatchTheLizard/Tutorial/BP_TrainingLizardDoll',False)]:
 bp=unreal.load_asset(path)
 assert bp is not None,path
 defaults=B.get_default_object(bp)
 defaults.set_editor_property('bEnableRealLizardVoice',enabled)
 defaults.set_editor_property('LizardVoiceSound',sound if enabled else None)
 if enabled and not defaults.get_editor_property('LizardVoiceAttenuation'):
  defaults.set_editor_property('LizardVoiceAttenuation',unreal.load_asset('/Game/CatchTheLizard/Audio/SFX/ATT_WorldEffects'))
 B.compile_blueprint(bp,warnings_as_errors=True)
 assert bp.get_editor_property('status')==unreal.BlueprintStatus.BS_UP_TO_DATE,path
 assert assets.save_loaded_asset(bp,only_if_is_dirty=False)
 report[path]={'enabled':defaults.get_editor_property('bEnableRealLizardVoice'),
               'sound':str(defaults.get_editor_property('LizardVoiceSound')),
               'minimum_interval':defaults.get_editor_property('LizardVoiceMinInterval'),
               'maximum_interval':defaults.get_editor_property('LizardVoiceMaxInterval'),
               'volume':defaults.get_editor_property('LizardVoiceVolume'),
               'attenuation':str(defaults.get_editor_property('LizardVoiceAttenuation'))}
assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
out=Path(unreal.Paths.project_saved_dir())/'AudioDiagnosis';out.mkdir(exist_ok=True)
(out/'RealLizardVoiceAssignments.json').write_text(json.dumps(report,indent=2))
print('REAL_LIZARD_VOICE_SAVED',report)
