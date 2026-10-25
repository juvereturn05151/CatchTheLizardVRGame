"""Author assets, then level1, level2, and catalog phases inside the built editor.
Set MISSION_PHASE before exec. Reruns preserve room and UI designer edits.
"""
import json
import shutil
from pathlib import Path
import unreal
from editor_toolset.toolsets.blueprint import BlueprintTools as B

ROOT = '/Game/CatchTheLizard/Missions'
MAP1 = '/Game/Maps/Map_CatchTheLizard'
MAP2 = '/Game/Maps/Map_CatchTheLizard_Level2'
assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
ui = unreal.get_default_object(unreal.load_class(None, '/Script/UMGToolSet.UMGToolSet'))
checkpoint = Path(unreal.Paths.project_saved_dir()) / 'MissionCheckpoint'
checkpoint.mkdir(exist_ok=True)
assert not level.is_in_play_in_editor()

def compile_save(bp):
    B.compile_blueprint(bp, warnings_as_errors=True)
    assert bp.get_editor_property('status') == unreal.BlueprintStatus.BS_UP_TO_DATE, bp.get_path_name()
    assert assets.save_loaded_asset(bp, only_if_is_dirty=False)

def bp(name, cls):
    path = ROOT + '/' + name
    if not assets.does_asset_exist(path):
        f = unreal.BlueprintFactory(); f.set_editor_property('parent_class', cls)
        tools.create_asset(name, ROOT, unreal.Blueprint, f)
    return unreal.load_asset(path)

def add(w, cls, name, parent=None):
    return ui.call_method('AddWidget', args=(w, cls.static_class(), name, parent, -1))

def bind(w, item):
    ui.call_method('ToggleWidgetAsVariable', args=(w, item.widget, True))

def label(w, parent, name, value, size, color=(.96, .99, 1, 1), bound=False):
    item = add(w, unreal.TextBlock, name, parent)
    item.widget.set_text(unreal.Text(value))
    font = item.widget.get_editor_property('font'); font.size = size; font.typeface_font_name = 'Bold'
    item.widget.set_font(font); item.widget.set_color_and_opacity(unreal.SlateColor(specified_color=unreal.LinearColor(*color)))
    item.widget.set_editor_property('justification', unreal.TextJustify.CENTER)
    item.widget.set_editor_property('auto_wrap_text', True)
    item.widget.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    if isinstance(item.slot, unreal.VerticalBoxSlot): item.slot.set_padding(unreal.Margin(0, 8, 0, 16))
    if bound: bind(w, item)
    return item

def button(w, parent, name, text, label_name, height=145):
    size = add(w, unreal.SizeBox, name + 'Size', parent); size.widget.set_height_override(height)
    size.slot.set_padding(unreal.Margin(30, 9, 30, 12))
    item = add(w, unreal.Button, name, size.widget)
    item.widget.set_editor_property('is_focusable', False)
    item.widget.set_click_method(unreal.ButtonClickMethod.DOWN_AND_UP)
    style = item.widget.get_editor_property('widget_style')
    for key, color in [('normal', (.008, .055, .035, 1)), ('hovered', (.016, .12, .06, 1)), ('pressed', (.003, .02, .012, 1)), ('disabled', (.025, .035, .04, 1))]:
        brush = getattr(style, key); brush.tint_color = unreal.SlateColor(specified_color=unreal.LinearColor(*color)); brush.draw_as = unreal.SlateBrushDrawType.IMAGE; setattr(style, key, brush)
    style.normal_padding = unreal.Margin(18, 14, 18, 14); style.pressed_padding = unreal.Margin(18, 22, 18, 6)
    item.widget.set_style(style)
    text_item = label(w, item.widget, label_name, text, 44, bound=True)
    text_item.slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_CENTER)
    text_item.slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_CENTER)
    bind(w, item)
    return size.widget

def widget(name, cls, completion):
    path = ROOT + '/' + name
    if not assets.does_asset_exist(path):
        w = ui.call_method('CreateWidgetBlueprint', args=(ROOT, name, cls))
        border = add(w, unreal.Border, 'SolidBackground'); border.widget.set_brush_color(unreal.LinearColor(.006, .014, .022, 1))
        border.widget.set_padding(unreal.Margin(65, 35, 65, 35))
        column = add(w, unreal.VerticalBox, 'Content', border.widget)
        column.slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
        column.slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_CENTER)
        if completion:
            label(w, column.widget, 'MissionTitle', 'MISSION COMPLETE', 76, (.5, 1, .68, 1))
            label(w, column.widget, 'CountsText', 'Lizards captured: 0 / 3', 48, bound=True)
            button(w, column.widget, 'NextMissionButton', 'Next Mission', 'NextLabel')
            button(w, column.widget, 'MenuButton', 'Return to Menu', 'MenuLabel')
            label(w, column.widget, 'ControllerHint', 'Point either controller and press the index trigger.', 28, (.75, .85, .9, 1))
        else:
            label(w, column.widget, 'ObjectiveText', 'Catch 3 lizards', 43, bound=True)
            label(w, column.widget, 'ProgressText', '0/3', 64, (.5, 1, .68, 1), True)
            label(w, column.widget, 'HintText', '', 34, (1, .82, .4, 1), True)
        compile_save(w)
    return unreal.load_asset(path)

def material(name, color, parameter=False):
    path = ROOT + '/' + name
    if assets.does_asset_exist(path): return unreal.load_asset(path)
    m = tools.create_asset(name, ROOT, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    e = unreal.MaterialEditingLibrary.create_material_expression(m, unreal.MaterialExpressionVectorParameter if parameter else unreal.MaterialExpressionConstant3Vector, -250, 0)
    if parameter: e.set_editor_property('parameter_name', 'BodyColor'); e.set_editor_property('default_value', unreal.LinearColor(*color))
    else: e.set_editor_property('constant', unreal.LinearColor(*color))
    unreal.MaterialEditingLibrary.connect_material_property(e, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(m); assets.save_loaded_asset(m)
    return m

def actor(label_name, cls, position, rotation=None):
    existing = next((a for a in actors.get_all_level_actors() if a.get_actor_label() == label_name), None)
    return existing or actors.spawn_actor_from_class(cls, unreal.Vector(*position), rotation or unreal.Rotator())

def prop(label_name, position, scale, mat):
    a = actor(label_name, unreal.StaticMeshActor, position)
    a.set_actor_label(label_name)
    a.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
    a.static_mesh_component.set_material(0, mat); a.set_actor_scale3d(unreal.Vector(*scale))
    a.static_mesh_component.set_collision_profile_name('BlockAll')
    return a

def add_wayfinding():
    sign = actor('Side Room Exit Wayfinding', unreal.TextRenderActor, (385, 0, 260), unreal.Rotator(yaw=180)); sign.set_actor_label('Side Room Exit Wayfinding')
    text_component = sign.get_component_by_class(unreal.TextRenderComponent)
    text_component.set_text(unreal.Text('EXIT\nTHROUGH THIS ROOM')); text_component.set_world_size(24)
    text_component.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
    text_component.set_text_render_color(unreal.Color(180, 255, 205, 255))

def author_assets():
    completion = widget('WBP_MissionComplete', unreal.MissionCompleteWidget, True)
    objective = widget('WBP_MissionObjective', unreal.MissionObjectiveWidget, False)
    door = bp('BP_ExitDoor', unreal.MissionExitDoor)
    d = B.get_default_object(door)
    frame = material('M_ExitMarker', (.22, .3, .32, 1), True)
    for name in ['LeftPost', 'RightPost', 'Lintel']: d.get_editor_property(name).set_material(0, frame)
    d.get_editor_property('DoorMesh').set_material(0, material('M_Door', (.23, .12, .06, 1)))
    compile_save(door)
    director = bp('BP_MissionDirector', unreal.MissionDirector)
    d = B.get_default_object(director)
    d.get_editor_property('ObjectivePanel').set_editor_property('widget_class', objective.generated_class())
    d.get_editor_property('CompletionPanel').set_editor_property('widget_class', completion.generated_class())
    for hand in ['Left', 'Right']: d.set_editor_property(hand + 'TriggerAction', unreal.load_asset('/Game/XRFramework/Input/Actions/Hands/IA_Hand_IndexCurl_' + hand))
    d.set_editor_property('RayMaterial', unreal.load_asset('/Game/CatchTheLizard/Menu/M_MenuRay'))
    compile_save(director)
    manager = unreal.load_asset('/Game/CatchTheLizard/BP_LizardCollectionManager')
    if assets.get_metadata_tag(manager, 'ExitMissionGuard') != '1':
        try: B.get_default_object(manager).get_editor_property('bRequireExit')
        except Exception: B.add_variable(manager, 'bRequireExit', 'bool')
        B.set_variable_instance_editable(manager, 'bRequireExit', True)
        graph = B.get_graph(manager, 'CompleteVictory'); ge = unreal.BlueprintGraphEditor.get_graph_editor(graph)
        entry = ge.find_graph_entry_pin(); previous = list(entry.list_connected_pins()); assert len(previous) == 1
        getter = ge.add_get_member_variable_node('bRequireExit')
        branch = ge.add_branch_node(); result = ge.add_return_node()
        entry.break_pin_links()
        assert entry.try_create_connection(branch.find_execute_pin())
        assert getter.find_output_pin('bRequireExit').try_create_connection(branch.find_input_pin('Condition'))
        assert branch.find_output_pin('then').try_create_connection(result.find_execute_pin())
        assert branch.find_output_pin('else').try_create_connection(previous[0])
        assets.set_metadata_tag(manager, 'ExitMissionGuard', '1'); compile_save(manager)
    selection = unreal.load_asset('/Game/CatchTheLizard/Tutorial/WBP_LevelSelection')
    tree = ui.call_method('GetWidgets', args=(selection,)); items = {i.widget.get_name(): i.widget for i in tree.widgets if i.widget}
    if 'LevelTwoButton' not in items:
        size = button(selection, items['SelectionContent'], 'LevelTwoButton', 'Level 2 - Catch 3 Lizards', 'LevelTwoLabel', 130)
        ui.call_method('MoveWidget', args=(selection, size, items['SelectionContent'], 4))
    items['SelectionSubtitle'].set_text(unreal.Text('All stages are available'))
    compile_save(selection)
    selection_pawn = unreal.load_asset('/Game/CatchTheLizard/Tutorial/BP_LevelSelectionPawn')
    B.get_default_object(selection_pawn).get_editor_property('MenuPanel').set_editor_property('draw_size', unreal.IntPoint(1100, 1000))
    compile_save(selection_pawn)

def author_room(is_second):
    path = MAP2 if is_second else MAP1
    if is_second and not assets.does_asset_exist(MAP2):
        assert level.load_level(MAP1)
        assert unreal.EditorLoadingAndSavingUtils.save_map(editor.get_editor_world(), MAP2)
        assert level.load_level(MAP2)
    elif editor.get_editor_world().get_path_name().split('.')[0] != path:
        assert level.load_level(path)
    assert editor.get_editor_world().get_path_name().split('.')[0] == path
    original = next((a for a in actors.get_all_level_actors() if a.get_name() == 'Brush_10'), None)
    if original: assert actors.destroy_actor(original)
    ground = material('M_Landing', (.14, .19, .22, 1)); edge = material('M_PathEdges', (.28, .42, .4, 1))
    wall = material('M_ExteriorWall', (.45, .43, .39, 1))
    prop('Exit Wall Left', (730, -135, 150), (.2, .7, 3), wall)
    prop('Exit Wall Right', (730, 135, 150), (.2, .7, 3), wall)
    prop('Exit Wall Above Door', (730, 0, 275), (.2, 2, .5), wall)
    prop('Outdoor Landing', (940, 0, -10), (4.4, 4.6, .2), ground)
    prop('Outdoor Path', (1250, 0, -10), (2, 1.8, .2), ground)
    sky_mat = material('M_OutdoorSky', (.07, .14, .24, 1))
    sky_mat.set_editor_property('two_sided', True); unreal.MaterialEditingLibrary.recompile_material(sky_mat); assets.save_loaded_asset(sky_mat)
    sky = actor('Outdoor Sky Backdrop', unreal.StaticMeshActor, (0, 0, 0)); sky.set_actor_label('Outdoor Sky Backdrop')
    sky.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Sphere')); sky.static_mesh_component.set_material(0, sky_mat)
    sky.set_actor_scale3d(unreal.Vector(100, 100, 100)); sky.static_mesh_component.set_editor_property('use_default_collision', False)
    sky.static_mesh_component.set_collision_profile_name('NoCollision'); sky.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION); sky.static_mesh_component.set_cast_shadow(False)
    for y in [-230, 230]: prop('Landing Edge ' + str(y), (940, y, 4), (4.4, .08, .08), edge)
    for y in [-90, 90]: prop('Path Edge ' + str(y), (1250, y, 4), (2, .08, .08), edge)
    nav = next(a for a in actors.get_all_level_actors() if a.get_class().get_name() == 'NavMeshBoundsVolume')
    nav.set_actor_location(unreal.Vector(250, 0, 0), False, True); nav.set_actor_scale3d(unreal.Vector(2, 1.2, 1))
    door = actor('House Exit', unreal.load_asset(ROOT + '/BP_ExitDoor').generated_class(), (730, 0, 0)); door.set_actor_label('House Exit')
    add_wayfinding()
    director = actor('Mission Director', unreal.load_asset(ROOT + '/BP_MissionDirector').generated_class(), (0, 0, 0)); director.set_actor_label('Mission Director')
    current_lizards = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.LizardGameplayActor) and not a.get_editor_property('bTrainingDoll')]
    if is_second:
        assert len(current_lizards) <= 3
        positions = [((-391, -150, 125), (1, 0, 0), (0, 1, 0)), ((0, 391, 110), (0, -1, 0), (1, 0, 0)), ((570, -141, 120), (0, 1, 0), (1, 0, 0))]
        while len(current_lizards) < 3:
            current_lizards.append(actors.spawn_actor_from_class(unreal.load_class(None, '/Game/CatchTheLizard/BP_Lizard.BP_Lizard_C'), unreal.Vector(0, 0, 100)))
        for i, (lizard, (position, normal, direction)) in enumerate(zip(current_lizards, positions)):
            lizard.set_actor_label('Mission Lizard ' + str(i + 1)); lizard.set_actor_location(unreal.Vector(*position), False, True)
            lizard.set_editor_property('SurfaceNormal', unreal.Vector(*normal)); lizard.set_editor_property('MoveDirection', unreal.Vector(*direction))
            lizard.set_actor_rotation(unreal.MathLibrary.make_rot_from_xz(unreal.Vector(*direction), unreal.Vector(*normal)), True)
    assert len(current_lizards) == (3 if is_second else 1)
    all_actors = actors.get_all_level_actors()
    containers = [a for a in all_actors if a.get_class().get_name() == 'BP_CollectionBottle_C']
    manager = next(a for a in all_actors if a.get_class().get_name() == 'BP_LizardCollectionManager_C')
    manager.set_editor_property('bRequireExit', True)
    director.set_editor_property('CaptureTarget', 3 if is_second else len(current_lizards))
    director.set_editor_property('MissionLizards', current_lizards); director.set_editor_property('Containers', containers)
    director.set_editor_property('MissionTools', [a for a in all_actors if a.get_class().get_name() in ['BP_CollectionBottle_C', 'BP_Broom_C', 'BP_Spray_C']])
    director.set_editor_property('CollectionManager', manager); director.set_editor_property('ExitDoor', door); door.set_editor_property('Mission', director)
    director.set_editor_property('MainMenu', unreal.load_asset('/Game/Maps/Map_MainMenu'))
    if is_second: director.set_editor_property('NextMission', None)
    elif assets.does_asset_exist(MAP2): director.set_editor_property('NextMission', unreal.load_asset(MAP2))
    panel = director.get_editor_property('ObjectivePanel'); panel.set_world_location(unreal.Vector(350, 170, 205), False, True); panel.set_world_rotation(unreal.Rotator(yaw=180), False, True)
    assert unreal.MissionDirector.build_mission_geometry_and_navigation()
    assert level.save_current_level()
    (checkpoint / ('Level2Layout.json' if is_second else 'Level1Layout.json')).write_text(json.dumps({'map':path,'lizards':[a.get_path_name() for a in current_lizards], 'capture_target':director.get_editor_property('CaptureTarget'), 'container_count':len(containers), 'door':door.get_path_name()},indent=2))

def author_catalog():
    catalog = unreal.load_asset('/Game/CatchTheLizard/Tutorial/DA_LevelList')
    entries = list(catalog.get_editor_property('levels'))
    if not any(str(e.get_editor_property('id')) == 'Level2' for e in entries):
        entry = unreal.CTLLevelEntry(); entry.set_editor_property('id', 'Level2'); entry.set_editor_property('display_name', unreal.Text('Level 2 — Catch 3 Lizards')); entry.set_editor_property('map', unreal.load_asset(MAP2)); entries.append(entry)
        catalog.set_editor_property('levels', entries); assets.save_loaded_asset(catalog)
    music = unreal.load_asset('/Game/CatchTheLizard/Music/Playback/DA_MapMusic')
    entries = list(music.get_editor_property('maps'))
    if not any(MAP2 in e.export_text() for e in entries):
        entry = unreal.CTLMapMusic(); entry.set_editor_property('map', unreal.load_asset(MAP2)); entry.set_editor_property('continuity_group','Gameplay'); entry.set_editor_property('track',unreal.load_asset('/Game/CatchTheLizard/Music/Playback/GameBGM_Loop')); entry.set_editor_property('volume',.17); entries.append(entry)
        music.set_editor_property('maps',entries); assets.save_loaded_asset(music)

phase = globals().get('MISSION_PHASE', 'assets')
if phase == 'assets':
    for relative in ['Content/Maps/Map_CatchTheLizard.umap', 'Content/CatchTheLizard/BP_LizardCollectionManager.uasset']:
        backup = checkpoint / ('BeforeExit_' + Path(relative).name)
        if not backup.exists(): shutil.copy2(Path(unreal.Paths.project_dir()) / relative, backup)
    author_assets()
elif phase == 'level1': author_room(False)
elif phase == 'level2': author_room(True)
elif phase == 'catalog': author_catalog()
elif phase == 'signs': add_wayfinding()
else: raise ValueError(phase)
assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
print('MISSION_PHASE_SAVED', phase)
