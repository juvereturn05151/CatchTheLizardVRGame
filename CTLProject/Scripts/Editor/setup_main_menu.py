"""Run inside the rebuilt Unreal Editor. Creates editable assets; reruns retain designer work."""
import json
from pathlib import Path
import unreal
from editor_toolset.toolsets.blueprint import BlueprintTools as B

ROOT = '/Game/CatchTheLizard/Menu'
MENU_MAP = '/Game/Maps/Map_MainMenu'
GAME_MAP = '/Game/Maps/Map_CatchTheLizard'
assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

def native(name):
    cls = unreal.load_class(None, '/Script/CTLProject.' + name)
    assert cls, 'Build the Editor target and restart Unreal before running this script'
    return cls

def blueprint(name, parent):
    path = ROOT + '/' + name
    if not assets.does_asset_exist(path):
        B.create(ROOT, name, parent)
    return unreal.load_asset(path)

def compile_save(bp):
    B.compile_blueprint(bp, warnings_as_errors=True)
    assert bp.get_editor_property('status') == unreal.BlueprintStatus.BS_UP_TO_DATE, bp.get_path_name()
    assert assets.save_loaded_asset(bp, only_if_is_dirty=False)

def unlit(name, color, parameter=False, two_sided=False):
    path = ROOT + '/' + name
    if assets.does_asset_exist(path):
        return unreal.load_asset(path)
    m = tools.create_asset(name, ROOT, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property('two_sided', two_sided)
    me = unreal.MaterialEditingLibrary
    n = me.create_material_expression(m, unreal.MaterialExpressionVectorParameter if parameter else unreal.MaterialExpressionConstant3Vector, -250, 0)
    if parameter:
        n.set_editor_property('parameter_name', 'RayColor')
        n.set_editor_property('default_value', unreal.LinearColor(*color))
    else:
        n.set_editor_property('constant', unreal.LinearColor(*color))
    assert me.connect_material_property(n, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    me.recompile_material(m)
    assert assets.save_loaded_asset(m)
    return m

ui = unreal.get_default_object(unreal.load_class(None, '/Script/UMGToolSet.UMGToolSet'))
widget_path = ROOT + '/WBP_MainMenu'
if not assets.does_asset_exist(widget_path):
    widget = ui.call_method('CreateWidgetBlueprint', args=(ROOT, 'WBP_MainMenu', native('VRMainMenuWidget')))

    def add(cls, name, parent=None):
        return ui.call_method('AddWidget', args=(widget, cls.static_class(), name, parent, -1))

    root = add(unreal.Border, 'MenuBackground')
    root.widget.set_brush_color(unreal.LinearColor(.012, .027, .033, 1))
    root.widget.set_padding(unreal.Margin(80, 46, 80, 40))
    content = add(unreal.VerticalBox, 'MenuContent', root.widget)
    content.slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
    content.slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_CENTER)

    def text(name, value, size, color, padding):
        item = add(unreal.TextBlock, name, content.widget)
        item.widget.set_text(unreal.Text(value))
        font = item.widget.get_editor_property('font')
        font.size = size
        font.typeface_font_name = 'Bold' if size >= 45 else 'Regular'
        item.widget.set_font(font)
        item.widget.set_color_and_opacity(unreal.SlateColor(specified_color=unreal.LinearColor(*color)))
        item.widget.set_editor_property('justification', unreal.TextJustify.CENTER)
        item.widget.set_editor_property('auto_wrap_text', True)
        item.slot.set_padding(padding)
        # Text labels must not intercept pointer hit testing intended for a parent button.
        item.widget.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
        return item

    text('MenuEyebrow', 'A LITTLE HUNT. A BIG ADVENTURE.', 21, (.32, .91, .63, 1), unreal.Margin(0, 0, 0, 14))
    text('MenuTitle', 'Catch The Lizard', 58, (.95, .99, .97, 1), unreal.Margin(0, 0, 0, 12))
    text('InteractionHint', 'Point either controller and press the trigger', 25, (.64, .76, .75, 1), unreal.Margin(0, 0, 0, 34))

    def button(name, label, colors):
        box = add(unreal.SizeBox, name + 'Size', content.widget)
        box.widget.set_height_override(120)
        box.slot.set_padding(unreal.Margin(65, 0, 65, 20))
        item = add(unreal.Button, name, box.widget)
        item.widget.set_editor_property('is_focusable', False)
        # Standard down-and-up click method gives visible press and release-to-activate behavior.
        item.widget.set_click_method(unreal.ButtonClickMethod.DOWN_AND_UP)
        style = item.widget.get_editor_property('widget_style')
        for prop, color in zip(['normal', 'hovered', 'pressed', 'disabled'], colors):
            brush = getattr(style, prop)
            brush.tint_color = unreal.SlateColor(specified_color=unreal.LinearColor(*color))
            brush.draw_as = unreal.SlateBrushDrawType.IMAGE
            setattr(style, prop, brush)
        style.normal_padding = unreal.Margin(16, 12, 16, 12)
        style.pressed_padding = unreal.Margin(16, 18, 16, 6)
        item.widget.set_style(style)
        label_item = add(unreal.TextBlock, name + 'Label', item.widget)
        label_item.widget.set_text(unreal.Text(label))
        font = label_item.widget.get_editor_property('font'); font.size = 42; font.typeface_font_name = 'Bold'
        label_item.widget.set_font(font)
        label_item.widget.set_editor_property('justification', unreal.TextJustify.CENTER)
        label_item.widget.set_color_and_opacity(unreal.SlateColor(specified_color=unreal.LinearColor(.98, 1, .98, 1)))
        label_item.widget.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
        label_item.slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_CENTER)
        label_item.slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_CENTER)
        ui.call_method('ToggleWidgetAsVariable', args=(widget, item.widget, True))
        return item

    button('StartButton', 'Start', [(.035, .32, .19, 1), (.06, .56, .30, 1), (.025, .20, .12, 1), (.08, .13, .12, 1)])
    button('ExitButton', 'Exit', [(.06, .12, .15, 1), (.13, .25, .30, 1), (.025, .055, .07, 1), (.07, .10, .11, 1)])
    status = text('StatusText', '', 23, (.92, .77, .38, 1), unreal.Margin(0, 4, 0, 0))
    ui.call_method('ToggleWidgetAsVariable', args=(widget, status.widget, True))
else:
    widget = unreal.load_asset(widget_path)
compile_save(widget)

ray = unlit('M_MenuRay', (.1, .65, 1, 1), parameter=True)
backdrop = unlit('M_MenuBackdrop', (.008, .018, .025, 1), two_sided=True)
floor = unlit('M_MenuFloor', (.022, .041, .046, 1))

# Keep the exact trigger action assets the gameplay pawn already uses, with a menu-only context.
context_path = ROOT + '/IMC_MainMenu'
if not assets.does_asset_exist(context_path):
    context = assets.duplicate_asset('/Game/XRFramework/Input/IMC_Hands', context_path)
    assert context
    hand_context = unreal.load_asset('/Game/XRFramework/Input/IMC_Hands')
    mappings = hand_context.get_editor_property('default_key_mappings').mappings
    triggers = [m for m in mappings if m.action.get_name() in ['IA_Hand_IndexCurl_Left', 'IA_Hand_IndexCurl_Right']]
    assert triggers, 'No existing VR trigger mappings found'
    context.set_editor_property('default_key_mappings', unreal.InputMappingContextMappingData(mappings=triggers))
    context.set_editor_property('context_description', unreal.Text('Main menu triggers only; removed before gameplay'))
    assert assets.save_loaded_asset(context)
else:
    context = unreal.load_asset(context_path)

pawn_is_new = not assets.does_asset_exist(ROOT + '/BP_MainMenuPawn')
pawn = blueprint('BP_MainMenuPawn', native('VRMainMenuPawn'))
defaults = B.get_default_object(pawn)
if pawn_is_new:
    defaults.set_editor_property('gameplay_map', unreal.load_asset(GAME_MAP))
    defaults.set_editor_property('menu_input_context', context)
    for hand in ['Left', 'Right']:
        defaults.set_editor_property(hand.lower() + '_trigger_action', unreal.load_asset('/Game/XRFramework/Input/Actions/Hands/IA_Hand_IndexCurl_' + hand))
    defaults.set_editor_property('ray_material', ray)
    panel = defaults.get_editor_property('menu_panel')
    panel.set_editor_property('widget_class', widget.generated_class())
compile_save(pawn)

mode_is_new = not assets.does_asset_exist(ROOT + '/BP_MainMenuGameMode')
mode = blueprint('BP_MainMenuGameMode', native('VRMainMenuGameMode'))
if mode_is_new:
    B.get_default_object(mode).set_editor_property('default_pawn_class', pawn.generated_class())
compile_save(mode)

level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not assets.does_asset_exist(MENU_MAP):
    assert level.new_level(MENU_MAP)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property('default_game_mode', mode.generated_class())
    world.get_world_settings().set_editor_property('enable_world_bounds_checks', False)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    start = actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    start.set_actor_label('Menu Player Start')

    def static(name, mesh, material, location, scale):
        actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location))
        actor.set_actor_label(name)
        comp = actor.static_mesh_component
        comp.set_static_mesh(unreal.load_asset(mesh))
        comp.set_material(0, material)
        comp.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        comp.set_cast_shadow(False)
        actor.set_actor_scale3d(unreal.Vector(*scale))
        return actor

    static('Unlit Menu Backdrop', '/Engine/BasicShapes/Sphere', backdrop, (0, 0, 0), (60, 60, 60))
    static('Unlit Menu Floor', '/Engine/BasicShapes/Cube', floor, (0, 0, -3), (30, 30, .05))
    # No realtime lights, post-process effects, navigation, physics props or gameplay actors.
    assert level.save_current_level()
else:
    assert level.load_level(MENU_MAP)

for path in ['/Game/XRFramework/Blueprints/BP_XRPawn', '/Game/XRFramework/Blueprints/BP_XRGameMode']:
    bp = unreal.load_asset(path)
    B.compile_blueprint(bp, warnings_as_errors=True)
    assert bp.get_editor_property('status') == unreal.BlueprintStatus.BS_UP_TO_DATE, path
    assets.save_loaded_asset(bp, only_if_is_dirty=True)

assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
unreal.EditorLevelLibrary.set_level_viewport_camera_info(unreal.Vector(-40, 0, 160), unreal.Rotator(0, 0, 0))
result = {
    'menu_map': MENU_MAP,
    'gameplay_map': str(B.get_default_object(pawn).get_editor_property('gameplay_map')),
    'widget': widget.get_path_name(),
    'pawn': pawn.get_path_name(),
    'game_mode': mode.get_path_name(),
    'trigger_mapping_count': len(context.get_editor_property('default_key_mappings').mappings),
    'blueprints_compiled': True,
    'all_assets_saved': True,
}
report = Path(unreal.Paths.project_saved_dir()) / 'MainMenu-AssetVerification.json'
report.write_text(json.dumps(result, indent=2))
print('MAIN_MENU_ASSETS_READY', json.dumps(result))
