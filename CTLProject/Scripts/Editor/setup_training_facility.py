"""Run with TRAINING_AUTHORING_PHASE='selection' while opening Map_LevelSelection,
then restart into Map_TrainingFacility and run with phase='room'. Keeping editor
world switches outside the asset-authoring pass avoids UE's world-cleanup guard.
Reruns preserve existing designer trees and completed room layout.
"""
import json
from pathlib import Path
import unreal
from editor_toolset.toolsets.blueprint import BlueprintTools as B
from editor_toolset.toolsets.actor import ActorTools as A

ROOT = '/Game/CatchTheLizard/Tutorial'
MENU = '/Game/CatchTheLizard/Menu'
MAP_TUTORIAL = '/Game/Maps/Map_TrainingFacility'
MAP_SELECTION = '/Game/Maps/Map_LevelSelection'
MAP_MAIN = '/Game/Maps/Map_MainMenu'
MAP_LEVEL1 = '/Game/Maps/Map_CatchTheLizard'
assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
ui = unreal.get_default_object(unreal.load_class(None, '/Script/UMGToolSet.UMGToolSet'))
assert not level.is_in_play_in_editor()
assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
phase = globals().get('TRAINING_AUTHORING_PHASE', 'room')
if not (Path(unreal.Paths.project_content_dir()) / 'Maps/Map_TrainingFacility.umap').exists():
    assert level.new_level(MAP_TUTORIAL)
if phase == 'selection' and not (Path(unreal.Paths.project_content_dir()) / 'Maps/Map_LevelSelection.umap').exists():
    assert level.new_level(MAP_SELECTION)

def native(name):
    value = unreal.load_class(None, '/Script/CTLProject.' + name)
    assert value, name
    return value

def compile_save(bp):
    B.compile_blueprint(bp, warnings_as_errors=True)
    assert bp.get_editor_property('status') == unreal.BlueprintStatus.BS_UP_TO_DATE, bp.get_path_name()
    assert assets.save_loaded_asset(bp, only_if_is_dirty=False)

def make_bp(path, parent):
    fresh = not assets.does_asset_exist(path)
    if fresh:
        B.create(path.rsplit('/', 1)[0], path.rsplit('/', 1)[1], parent)
    return unreal.load_asset(path), fresh

def components(bp):
    return {c.get_name().replace('_GEN_VARIABLE', ''): c for c in A.get_components(B.get_default_object(bp))}

def add_component(bp, cls, name, parent=None):
    c = A.add_component(bp, cls.static_class(), name)
    A.set_parent_component(c, parent or A.get_root_component(B.get_default_object(bp)))
    return c

def material(name, color, plastic=False, opacity=None):
    path = ROOT + '/' + name
    if assets.does_asset_exist(path):
        return unreal.load_asset(path)
    m = tools.create_asset(name, ROOT, unreal.Material, unreal.MaterialFactoryNew())
    me = unreal.MaterialEditingLibrary
    color_node = me.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -300, 0)
    color_node.set_editor_property('parameter_name', 'BodyColor')
    color_node.set_editor_property('default_value', unreal.LinearColor(*color))
    if plastic:
        assert me.connect_material_property(color_node, '', unreal.MaterialProperty.MP_BASE_COLOR)
        rough = me.create_material_expression(m, unreal.MaterialExpressionConstant, -300, 180)
        rough.set_editor_property('r', .3)
        assert me.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
    else:
        m.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
        assert me.connect_material_property(color_node, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if opacity is not None:
        m.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
        m.set_editor_property('two_sided', True)
        alpha = me.create_material_expression(m, unreal.MaterialExpressionConstant, -300, 180)
        alpha.set_editor_property('r', opacity)
        assert me.connect_material_property(alpha, '', unreal.MaterialProperty.MP_OPACITY)
    me.recompile_material(m)
    assert assets.save_loaded_asset(m)
    return m

def mesh_component(bp, name, location, scale, mat, mesh='/Engine/BasicShapes/Cube', collision=True):
    c = add_component(bp, unreal.StaticMeshComponent, name)
    c.set_static_mesh(unreal.load_asset(mesh))
    c.set_editor_property('relative_location', unreal.Vector(*location))
    c.set_editor_property('relative_scale3d', unreal.Vector(*scale))
    if mat:
        c.set_material(0, mat)
    c.set_collision_profile_name('BlockAll' if collision else 'NoCollision')
    c.set_cast_shadow(False)
    return c

def widget(name, parent):
    path = ROOT + '/' + name
    if assets.does_asset_exist(path):
        return unreal.load_asset(path), False
    return ui.call_method('CreateWidgetBlueprint', args=(ROOT, name, parent)), True

def add(w, cls, name, parent=None):
    return ui.call_method('AddWidget', args=(w, cls.static_class(), name, parent, -1))

def variable(w, item):
    ui.call_method('ToggleWidgetAsVariable', args=(w, item.widget, True))

def content(w, name, padding=(45, 35, 45, 30)):
    root = add(w, unreal.Border, name + 'Background')
    root.widget.set_brush_color(unreal.LinearColor(.012, .028, .04, 1))
    root.widget.set_padding(unreal.Margin(*padding))
    col = add(w, unreal.VerticalBox, name + 'Content', root.widget)
    col.slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
    col.slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_CENTER)
    return col

def text(w, parent, name, value, size, color=(.95, .99, 1, 1), padding=(0, 0, 0, 14), bind=False):
    item = add(w, unreal.TextBlock, name, parent.widget)
    item.widget.set_text(unreal.Text(value))
    f = item.widget.get_editor_property('font'); f.size = size; f.typeface_font_name = 'Bold' if size >= 42 else 'Regular'
    item.widget.set_font(f)
    item.widget.set_color_and_opacity(unreal.SlateColor(specified_color=unreal.LinearColor(*color)))
    item.widget.set_editor_property('justification', unreal.TextJustify.CENTER)
    item.widget.set_editor_property('auto_wrap_text', True)
    item.widget.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    item.slot.set_padding(unreal.Margin(*padding))
    if bind:
        variable(w, item)
    return item

def button(w, parent, name, label, label_name, height=110, size=34):
    box = add(w, unreal.SizeBox, name + 'Size', parent.widget)
    box.widget.set_height_override(height)
    box.slot.set_padding(unreal.Margin(20, 6, 20, 14))
    item = add(w, unreal.Button, name, box.widget)
    item.widget.set_editor_property('is_focusable', False)
    item.widget.set_click_method(unreal.ButtonClickMethod.DOWN_AND_UP)
    style = item.widget.get_editor_property('widget_style')
    for key, color in [('normal', (.025, .27, .20, 1)), ('hovered', (.05, .55, .35, 1)), ('pressed', (.014, .13, .09, 1)), ('disabled', (.06, .10, .12, 1))]:
        brush = getattr(style, key)
        brush.tint_color = unreal.SlateColor(specified_color=unreal.LinearColor(*color))
        brush.draw_as = unreal.SlateBrushDrawType.IMAGE
        setattr(style, key, brush)
    style.normal_padding = unreal.Margin(16, 12, 16, 12)
    style.pressed_padding = unreal.Margin(16, 18, 16, 6)
    item.widget.set_style(style)
    label_item = text(w, item, label_name, label, size, padding=(8, 0, 8, 0), bind=True)
    label_item.slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_CENTER)
    label_item.slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_CENTER)
    variable(w, item)
    return item

# Real editable UMG designer trees; C++ owns only state and event handling.
selection_widget, fresh = widget('WBP_LevelSelection', native('VRMainMenuWidget'))
if fresh:
    col = content(selection_widget, 'Selection', (70, 35, 70, 30))
    text(selection_widget, col, 'SelectionTitle', 'Choose your assignment', 48)
    text(selection_widget, col, 'SelectionSubtitle', 'Both stages are available', 25, (.6, .8, .8, 1))
    button(selection_widget, col, 'TutorialButton', 'Tutorial - Training Facility', 'TutorialLabel', 130, 34)
    button(selection_widget, col, 'LevelOneButton', 'Level 1 - Catch The Lizard', 'LevelOneLabel', 130, 34)
    button(selection_widget, col, 'BackButton', 'Back', 'BackLabel', 100, 34)
    text(selection_widget, col, 'StatusText', '', 23, (.9, .7, .3, 1), bind=True)
compile_save(selection_widget)

dialogue_widget, fresh = widget('WBP_TutorialDialogue', native('TutorialDialogueWidget'))
if fresh:
    col = content(dialogue_widget, 'Dialogue')
    text(dialogue_widget, col, 'InstructorTitle', 'INSTRUCTOR / TRAINING FACILITY', 27, (.3, .95, .7, 1))
    text(dialogue_widget, col, 'DialogueText', 'Instructor dialogue appears here. Edit text in BP_TrainingDirector defaults.', 33, padding=(8, 12, 8, 25), bind=True)
    button(dialogue_widget, col, 'ContinueButton', 'Continue', 'ActionLabel', 105, 36)
    text(dialogue_widget, col, 'PointerHint', 'Point either controller. Press index trigger.', 22, (.6, .77, .8, 1), padding=(0, 8, 0, 0))
compile_save(dialogue_widget)

objective_widget, fresh = widget('WBP_TutorialObjective', native('TutorialObjectiveWidget'))
if fresh:
    col = content(objective_widget, 'Objective', (28, 20, 28, 20))
    text(objective_widget, col, 'StatusText', 'CAPTURE PRACTICE', 25, (.4, 1, .65, 1), bind=True)
    text(objective_widget, col, 'ObjectiveText', 'Pick up the training lizard and place it inside the container.', 29, bind=True)
    text(objective_widget, col, 'ControlsText', 'Grip to pick up. Keep holding grip while placing it inside.', 24, (.73, .86, .9, 1), padding=(0, 4, 0, 0), bind=True)
compile_save(objective_widget)

toy = material('M_TrainingToy', (.95, .045, .42, 1), plastic=True)
floor_mat = material('M_TrainingFloor', (.07, .11, .15, 1))
wall_mat = material('M_TrainingWalls', (.19, .25, .30, 1))
dark_mat = material('M_TrainingTrim', (.025, .06, .08, 1))
uniform_mat = material('M_InstructorUniform', (.03, .30, .28, 1))
head_mat = material('M_InstructorHead', (.72, .79, .73, 1))
glass_mat = material('M_TrainingContainerGlass', (.12, .7, .9, 1), opacity=.18)
cue_mat = material('M_TrainingHighlight', (.3, 1, .45, 1))

doll_path = ROOT + '/BP_TrainingLizardDoll'
base_lizard = unreal.load_asset('/Game/CatchTheLizard/BP_Lizard')
if assets.does_asset_exist(doll_path) and unreal.BlueprintEditorLibrary.get_blueprint_parent_class(unreal.load_asset(doll_path)) != base_lizard.generated_class():
    assert not (Path(unreal.Paths.project_saved_dir()) / 'TutorialCheckpoint/RoomLayout.complete').exists()
    assert assets.delete_asset(doll_path)  # Replace only this pass's unused provisional duplicate.
fresh = not assets.does_asset_exist(doll_path)
if fresh:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property('parent_class', base_lizard.generated_class())
    doll_bp = tools.create_asset('BP_TrainingLizardDoll', ROOT, unreal.Blueprint, factory)
else:
    doll_bp = unreal.load_asset(doll_path)
assert unreal.BlueprintEditorLibrary.get_blueprint_parent_class(doll_bp) == base_lizard.generated_class()
if fresh:
    d = B.get_default_object(doll_bp)
    d.set_editor_property('bTrainingDoll', True)
    d.set_editor_property('bTrainingPickupEnabled', False)
    d.set_editor_property('NormalFeedbackColor', unreal.Vector(.95, .045, .42))
    d.set_editor_property('SurfaceNormal', unreal.Vector(0, 0, 1))
compile_save(doll_bp)

container_bp, fresh = make_bp(ROOT + '/BP_TrainingContainer', unreal.Actor.static_class())
if fresh:
    mesh_component(container_bp, 'ContainerBase', (0, 0, -11), (.36, .36, .02), dark_mat)
    for name, pos, scale in [('LeftWall', (0, -17, 2), (.36, .015, .26)), ('RightWall', (0, 17, 2), (.36, .015, .26)), ('BackWall', (17, 0, 2), (.015, .36, .26)), ('FrontWall', (-17, 0, 2), (.015, .36, .26))]:
        mesh_component(container_bp, name, pos, scale, glass_mat)
    mouth = add_component(container_bp, unreal.SphereComponent, 'DepositVolume')
    mouth.set_sphere_radius(11)
    mouth.set_editor_property('relative_location', unreal.Vector(0, 0, 3))
    mouth.set_collision_profile_name('OverlapAllDynamic')
    mouth.set_editor_property('generate_overlap_events', True)
compile_save(container_bp)

instructor_bp, fresh = make_bp(ROOT + '/BP_TrainingInstructor', unreal.Actor.static_class())
if fresh:
    # Deliberately simple, stationary humanoid mannequin; replace components with an NPC model later.
    mesh_component(instructor_bp, 'Torso', (0, 0, 122), (.3, .46, .56), uniform_mat)
    mesh_component(instructor_bp, 'Head', (0, 0, 166), (.27, .27, .3), head_mat, '/Engine/BasicShapes/Sphere')
    for side in [-1, 1]:
        mesh_component(instructor_bp, 'Arm' + str(side), (0, side * 32, 118), (.13, .13, .6), uniform_mat)
        mesh_component(instructor_bp, 'Leg' + str(side), (0, side * 13, 43), (.17, .17, .86), dark_mat)
    # Small face plate on local +X makes the facing direction unambiguous.
    mesh_component(instructor_bp, 'FacePlate', (13, 0, 167), (.02, .18, .08), dark_mat, collision=False)
compile_save(instructor_bp)

cue_bp, fresh = make_bp(ROOT + '/BP_TrainingHighlight', unreal.Actor.static_class())
if fresh:
    for name, pos, scale in [('EdgeLeft', (0, -20, 0), (.42, .012, .012)), ('EdgeRight', (0, 20, 0), (.42, .012, .012)), ('EdgeBack', (20, 0, 0), (.012, .4, .012)), ('EdgeFront', (-20, 0, 0), (.012, .4, .012))]:
        mesh_component(cue_bp, name, pos, scale, cue_mat, collision=False)
compile_save(cue_bp)

director_bp, fresh = make_bp(ROOT + '/BP_TrainingDirector', native('TrainingDirector'))
if fresh:
    d = B.get_default_object(director_bp)
    d.get_editor_property('DialoguePanel').set_editor_property('widget_class', dialogue_widget.generated_class())
    d.get_editor_property('ObjectivePanel').set_editor_property('widget_class', objective_widget.generated_class())
    # Give the full grip instruction enough height without reducing its font size.
    panel = d.get_editor_property('ObjectivePanel')
    panel.set_editor_property('draw_size', unreal.IntPoint(850, 540))
    panel.set_editor_property('relative_location', unreal.Vector(195, 115, 183))
    d.set_editor_property('RayMaterial', unreal.load_asset(MENU + '/M_MenuRay'))
    for hand in ['Left', 'Right']:
        d.set_editor_property(hand + 'TriggerAction', unreal.load_asset('/Game/XRFramework/Input/Actions/Hands/IA_Hand_IndexCurl_' + hand))
compile_save(director_bp)

selection_pawn, fresh = make_bp(ROOT + '/BP_LevelSelectionPawn', native('VRMainMenuPawn'))
if fresh:
    d = B.get_default_object(selection_pawn)
    menu_d = B.get_default_object(unreal.load_asset(MENU + '/BP_MainMenuPawn'))
    for key in ['MenuInputContext', 'LeftTriggerAction', 'RightTriggerAction', 'RayMaterial']:
        d.set_editor_property(key, menu_d.get_editor_property(key))
    d.get_editor_property('MenuPanel').set_editor_property('widget_class', selection_widget.generated_class())
compile_save(selection_pawn)
selection_mode, fresh = make_bp(ROOT + '/BP_LevelSelectionGameMode', native('VRMainMenuGameMode'))
if fresh:
    B.get_default_object(selection_mode).set_editor_property('default_pawn_class', selection_pawn.generated_class())
compile_save(selection_mode)

# Create the worlds before assigning soft World references through Python's reflected properties.
selection_new = False
tutorial_new = not assets.does_asset_exist(MAP_TUTORIAL)
if tutorial_new:
    assert level.new_level(MAP_TUTORIAL)

list_path = ROOT + '/DA_LevelList'
if not assets.does_asset_exist(list_path):
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', native('CTLLevelList'))
    catalog = tools.create_asset('DA_LevelList', ROOT, native('CTLLevelList'), factory)
    entries = []
    for key, label, map_path in [('Tutorial', 'Tutorial \u2014 Training Facility', MAP_TUTORIAL), ('Level1', 'Level 1 \u2014 Catch The Lizard', MAP_LEVEL1)]:
        entry = unreal.CTLLevelEntry()
        entry.set_editor_property('id', key)
        entry.set_editor_property('display_name', unreal.Text(label))
        entry.set_editor_property('map', unreal.load_asset(map_path))
        entries.append(entry)
    catalog.set_editor_property('levels', entries)
    catalog.set_editor_property('main_menu_map', unreal.load_asset(MAP_MAIN))
    catalog.set_editor_property('selection_map', unreal.load_asset(MAP_SELECTION))
    assert assets.save_loaded_asset(catalog)
else:
    catalog = unreal.load_asset(list_path)

for bp in [unreal.load_asset(MENU + '/BP_MainMenuPawn'), selection_pawn, director_bp]:
    B.get_default_object(bp).set_editor_property('LevelList', catalog)
    compile_save(bp)

selection_marker = Path(unreal.Paths.project_saved_dir()) / 'TutorialCheckpoint/SelectionLayout.complete'
if not selection_marker.exists() and phase == 'selection':
    if editor.get_editor_world().get_path_name() != MAP_SELECTION + '.Map_LevelSelection':
        raise RuntimeError('Open Map_LevelSelection before running selection authoring.')
    editor.get_editor_world().get_world_settings().set_editor_property('default_game_mode', selection_mode.generated_class())
    if len(actors.get_all_level_actors()) == 0 or not any(isinstance(a, unreal.PlayerStart) for a in actors.get_all_level_actors()):
        start = actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 0))
        start.set_actor_label('Selection Player Start')
        for label, mesh_path, mat_path, pos, scale in [
            ('Selection Backdrop', '/Engine/BasicShapes/Sphere', MENU + '/M_MenuBackdrop', (0, 0, 0), (60, 60, 60)),
            ('Selection Floor', '/Engine/BasicShapes/Cube', MENU + '/M_MenuFloor', (0, 0, -3), (30, 30, .05))]:
            a = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*pos))
            a.set_actor_label(label)
            a.static_mesh_component.set_static_mesh(unreal.load_asset(mesh_path))
            a.static_mesh_component.set_material(0, unreal.load_asset(mat_path))
            a.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
            a.static_mesh_component.set_cast_shadow(False)
            a.set_actor_scale3d(unreal.Vector(*scale))
    assert level.save_current_level()
    selection_marker.write_text('Layout saved; preserve designer edits on reruns.')

def spawn(bp, label, pos, yaw=0):
    actor = actors.spawn_actor_from_class(bp.generated_class(), unreal.Vector(*pos), unreal.Rotator(pitch=0, yaw=yaw, roll=0))
    actor.set_actor_label(label)
    return actor

def prop(label, pos, scale, mat, collision=True):
    a = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*pos))
    a.set_actor_label(label)
    c = a.static_mesh_component
    c.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
    c.set_material(0, mat)
    c.set_collision_profile_name('BlockAll' if collision else 'NoCollision')
    c.set_cast_shadow(False)
    a.set_actor_scale3d(unreal.Vector(*scale))
    return a

def sign(label, value, pos, yaw=180, size=7):
    a = actors.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(*pos), unreal.Rotator(pitch=0, yaw=yaw, roll=0))
    a.set_actor_label(label)
    c = a.get_component_by_class(unreal.TextRenderComponent)
    c.set_text(unreal.Text(value))
    c.set_world_size(size)
    c.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
    return a

room_marker = Path(unreal.Paths.project_saved_dir()) / 'TutorialCheckpoint/RoomLayout.complete'
if not room_marker.exists() and phase == 'room':
    if editor.get_editor_world().get_path_name() != MAP_TUTORIAL + '.Map_TrainingFacility':
        raise RuntimeError('Open Map_TrainingFacility before running room authoring.')
    world = editor.get_editor_world()
    world.get_world_settings().set_editor_property('default_game_mode', unreal.load_asset('/Game/XRFramework/Blueprints/BP_XRGameMode').generated_class())
    world.get_world_settings().set_editor_property('enable_world_bounds_checks', False)
    start = actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 0))
    start.set_actor_label('Recruit Start - face the training table')
    prop('Facility Floor', (25, 0, -5), (8.5, 8, .1), floor_mat)
    for label, pos, scale in [('Back Wall', (450, 0, 160), (.1, 8, 3.2)), ('Entry Wall', (-400, 0, 160), (.1, 8, 3.2)), ('Left Wall', (25, -400, 160), (8.5, .1, 3.2)), ('Right Wall', (25, 400, 160), (8.5, .1, 3.2))]:
        prop(label, pos, scale, wall_mat)
    for y in [-395, 395]:
        prop('Facility Accent Stripe', (25, y, 120), (8.4, .02, .08), uniform_mat, False)
    table = spawn(unreal.load_asset('/Game/CatchTheLizard/BP_Table'), 'Training Table', (90, 0, 0), 90)
    doll = spawn(doll_bp, 'Training Lizard - inert toy', (66, -28, 100))
    doll_body = next(c for c in doll.get_components_by_class(unreal.StaticMeshComponent) if c.get_name() == 'LizardBody')
    doll_body.set_material(0, doll_body.static_mesh.get_material(0))
    container = spawn(container_bp, 'Training Capture Container', (78, 30, 102))
    instructor = spawn(instructor_bp, 'Instructor - stationary mannequin', (270, -45, 0), 171)
    prop('Facility Ceiling', (25, 0, 325), (8.5, 8, .1), dark_mat)
    prop('Broom Stand Base', (70, -130, 3), (.6, .6, .06), dark_mat)
    prop('Broom Stand Column', (70, -130, 40), (.1, .1, .8), uniform_mat)
    prop('Broom Stand Tray', (70, -130, 79), (.65, .55, .04), dark_mat)
    broom = spawn(unreal.load_asset('/Game/CatchTheLizard/BP_Broom'), 'Training Broom', (70, -130, 104))
    doll_cue = spawn(cue_bp, 'Doll Highlight', (66, -28, 92))
    doll_cue.set_actor_scale3d(unreal.Vector(.7, .7, 1))
    doll_cue.attach_to_actor(doll, '', unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, False)
    container_cue = spawn(cue_bp, 'Container Highlight', (78, 30, 91))
    broom_cue = spawn(cue_bp, 'Broom Highlight', (70, -130, 82))
    broom_cue.set_actor_scale3d(unreal.Vector(1.5, 1.5, 1))
    director = spawn(director_bp, 'Tutorial Director - edit references and dialogue', (0, 0, 0))
    for name, value in [('TrainingDoll', doll), ('CaptureContainer', container), ('TrainingBroom', broom), ('DollHighlight', doll_cue), ('ContainerHighlight', container_cue), ('BroomHighlight', broom_cue)]:
        director.set_editor_property(name, value)
    sign('Facility Sign', 'TRAINING FACILITY', (435, 0, 255), size=17)
    sign('Doll Sign', 'INERT TRAINING DOLL', (102, -32, 125), 180, 4)
    sign('Container Sign', 'CAPTURE CONTAINER', (110, 38, 135), 180, 4)
    sign('Broom Sign', 'BROOM STATION', (100, -130, 164), 180, 5)
    light = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 280), unreal.Rotator(pitch=-55, yaw=-30, roll=0))
    light.set_actor_label('Simple prop lighting - no shadows')
    lc = light.get_component_by_class(unreal.DirectionalLightComponent)
    lc.set_editor_property('mobility', unreal.ComponentMobility.MOVABLE)
    lc.set_cast_shadows(False)
    lc.set_intensity(3)
    assert level.save_current_level()
    room_marker.write_text('Layout saved; preserve designer edits on reruns.')

for p in [MENU + '/WBP_MainMenu', '/Game/CatchTheLizard/BP_Lizard', '/Game/XRFramework/Blueprints/BP_XRPawn', '/Game/XRFramework/Blueprints/BP_XRGameMode']:
    compile_save(unreal.load_asset(p))
assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
assets.sync_browser_to_objects([ROOT + '/BP_TrainingDirector', ROOT + '/DA_LevelList'])
report = {'tutorial_map': MAP_TUTORIAL, 'selection_map': MAP_SELECTION, 'level_1': MAP_LEVEL1, 'level_list': list_path, 'blueprints_compiled': True, 'assets_saved': True}
(Path(unreal.Paths.project_saved_dir()) / 'TutorialCheckpoint/AssetVerification.json').write_text(json.dumps(report, indent=2))
print('TRAINING_FACILITY_CREATED', json.dumps(report))
