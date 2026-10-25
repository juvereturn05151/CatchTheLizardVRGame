"""Read-only XR/input/Blueprint snapshot for black-screen diagnosis."""
import unreal, json
from pathlib import Path
from editor_toolset.toolsets.blueprint import BlueprintTools as B
from editor_toolset.toolsets.actor import ActorTools as A

out=Path(unreal.Paths.project_saved_dir())/'VRDiagnosis';out.mkdir(exist_ok=True)
assets=unreal.EditorAssetLibrary
report={'contexts':{},'blueprints':{},'runtime':{}}
content=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()))
for filename in content.rglob('IMC_*.uasset'):
    path='/Game/'+filename.relative_to(content).with_suffix('').as_posix()
    obj=unreal.load_asset(path)
    mappings=obj.get_editor_property('default_key_mappings').get_editor_property('mappings')
    report['contexts'][path]=[{'key':str(unreal.InputLibrary.key_get_display_name(m.key)),'action':m.action.get_name(),'triggers':str(m.triggers),'action_triggers':str(m.action.get_editor_property('triggers')),'modifiers':str(m.modifiers)} for m in mappings]
paths=['/Game/XRFramework/Blueprints/BP_XRPawn','/Game/CatchTheLizard/BP_LizardVRPawn','/Game/XRFramework/Blueprints/BP_TeleportVisualizer','/Game/XRFramework/Blueprints/BP_XRGameMode','/Game/XRFramework/Blueprints/BP_Passthrough','/Game/XRFramework/Blueprints/BP_Menu','/Game/XRFramework/Blueprints/WBP_Menu','/Game/VRSpectator/BP_VRSpectator','/Game/CatchTheLizard/Menu/BP_MainMenuPawn','/Game/CatchTheLizard/Tutorial/BP_LevelSelectionPawn']
for path in paths:
    bp=unreal.load_asset(path)
    if not bp: continue
    info={'graphs':{},'parent':str(B.get_parent(bp)),'components':[]}
    for graph in B.list_graphs(bp):
        rows=[]
        for n in B.get_node_infos(B.find_nodes(graph)):
            rows.append({'node':n.node.get_name(),'title':n.node.get_node_title(),'type':n.type_id,
                         'inputs':[{ 'name':p.name,'value':str(p.value),'links':[str(c) for c in p.connected_pins]} for p in n.input_pins],
                         'outputs':[{ 'name':p.name,'links':[str(c) for c in p.connected_pins]} for p in n.output_pins]})
        info['graphs'][graph.get_name()]=rows
    defaults=B.get_default_object(bp)
    if isinstance(defaults,unreal.GameModeBase):info['pawn']=str(defaults.get_editor_property('DefaultPawnClass'))
    try:
        info['components']=[{'name':c.get_name(),'class':c.get_class().get_name(),'parent':str(c.get_attach_parent()) if isinstance(c,unreal.SceneComponent) else ''} for c in A.get_components(defaults)]
    except Exception as e:info['components_error']=str(e)
    report['blueprints'][path]=info
w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
if w:
    pc=unreal.GameplayStatics.get_player_controller(w,0);p=unreal.GameplayStatics.get_player_pawn(w,0)
    report['runtime']={'world':w.get_path_name(),'pawn':str(p),'view_target':str(pc.get_view_target()) if pc else None,'hmd_enabled':unreal.HeadMountedDisplayFunctionLibrary.is_head_mounted_display_enabled(),'hmd_connected':unreal.HeadMountedDisplayFunctionLibrary.is_head_mounted_display_connected(),'tracking_valid':unreal.HeadMountedDisplayFunctionLibrary.has_valid_tracking_position(),'device':str(unreal.HeadMountedDisplayFunctionLibrary.get_hmd_device_name())}
    if p:
        report['runtime']['cameras']=[{'name':c.get_name(),'active':c.is_active(),'lock_to_hmd':c.get_editor_property('lock_to_hmd'),'parent':str(c.get_attach_parent()),'position':str(c.get_world_location())} for c in p.get_components_by_class(unreal.CameraComponent)]
(out/'Inspection.json').write_text(json.dumps(report,indent=2))
print('VR_INSPECTION_SAVED',str(out/'Inspection.json'),report['runtime'])
