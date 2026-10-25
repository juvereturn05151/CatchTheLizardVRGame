import unreal
from editor_toolset.toolsets.blueprint import BlueprintTools as B
from pathlib import Path
bp=unreal.load_asset('/Game/XRFramework/Blueprints/BP_XRPawn')
if unreal.EditorAssetLibrary.get_metadata_tag(bp,'ManualExitTeleportGuard')!='1':
 graph=B.get_graph(bp,'TryTeleport');ge=unreal.BlueprintGraphEditor.get_graph_editor(graph)
 branch=next(n for n in ge.list_all_nodes() if n.get_class().get_name()=='K2Node_IfThenElse')
 condition=branch.find_input_pin('Condition');previous=list(condition.list_connected_pins());assert len(previous)==1
 pawn=ge.add_call_function_node('/Script/Engine.GameplayStatics.GetPlayerPawn')
 can=ge.add_call_function_node('/Script/CTLProject.MissionDirector.CanTeleportTo')
 dest=ge.add_get_member_variable_node('ProjectedTeleportLocation')
 both=ge.add_call_function_node('/Script/Engine.KismetMathLibrary.BooleanAND')
 assert pawn.find_output_pin('ReturnValue').try_create_connection(can.find_input_pin('Pawn'))
 assert dest.find_output_pin('ProjectedTeleportLocation').try_create_connection(can.find_input_pin('Destination'))
 condition.break_pin_links()
 assert previous[0].try_create_connection(both.find_input_pin('A'))
 assert can.find_output_pin('ReturnValue').try_create_connection(both.find_input_pin('B'))
 assert both.find_output_pin('ReturnValue').try_create_connection(condition)
 unreal.EditorAssetLibrary.set_metadata_tag(bp,'ManualExitTeleportGuard','1')
B.compile_blueprint(bp,warnings_as_errors=True)
assert unreal.EditorAssetLibrary.save_loaded_asset(bp,only_if_is_dirty=False)
print('CLOSED_DOOR_TELEPORT_GUARD_SAVED')
