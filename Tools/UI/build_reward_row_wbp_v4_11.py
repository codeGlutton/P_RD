import json
import os
import re
import unreal

ASSET_PATH = "/Game/BP/UI/WBP_RewardRow"
ASSET_NAME = "WBP_RewardRow"
ASSET_DIR = "/Game/BP/UI"
PARENT_CLASS_PATH = "/Script/P_RD.RewardRowWidgetBase"
TEXTURE_DIR = "/Game/UI/RewardV4_11/Tex"

TEXTURE_NAMES = {
    "D:/UnrealProjects/P_RD_develop_20260701_216/output/combat_ui_kit/uploads/nav_button_frame_normal_20260706_015417.png": "T_reward_v4_row_icon_frame",
    "D:/UnrealProjects/P_RD_CombatUI_Assets_/P_RD_CombatUI_Assets_nobg/01_TopBar/02_GOLD_Pill/gold_icon.png": "T_reward_v4_gold_icon",
}

ROW_FRAME_SRC = "D:/UnrealProjects/P_RD_develop_20260701_216/output/combat_ui_kit/uploads/nav_button_frame_normal_20260706_015417.png"
GOLD_ICON_SRC = "D:/UnrealProjects/P_RD_CombatUI_Assets_/P_RD_CombatUI_Assets_nobg/01_TopBar/02_GOLD_Pill/gold_icon.png"

REQUIRED_WIDGETS = [
    "RewardRowRootSizeBox",
    "RewardRowCanvas",
    "mRowIconFrame",
    "mRewardIcon",
    "RewardSingleTextBox",
    "RewardSingleTextOverlay",
    "mRewardSingleText",
    "mRewardMainText",
    "mRewardSubText",
]


def lin(r, g, b, a=1.0):
    return unreal.LinearColor(float(r), float(g), float(b), float(a))


def norm_path(path):
    return (path or "").replace("\\", "/")


def safe_name(value):
    value = re.sub(r"[^A-Za-z0-9_]+", "_", value or "widget")
    value = re.sub(r"_+", "_", value).strip("_")
    return value or "widget"


def texture_asset_path(src):
    src = norm_path(src)
    name = TEXTURE_NAMES.get(src)
    if not name:
        name = "T_reward_v4_" + safe_name(os.path.splitext(os.path.basename(src))[0])
    return "%s/%s" % (TEXTURE_DIR, name)


def import_texture(src, result):
    src = norm_path(src)
    if not src:
        return None
    if not os.path.exists(src):
        result["missing_sources"].append(src)
        return None

    asset_path = texture_asset_path(src)
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        result["texture_existing"].append(asset_path)
        return asset_path

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", src)
    task.set_editor_property("destination_path", TEXTURE_DIR)
    task.set_editor_property("destination_name", asset_path.rsplit("/", 1)[-1])
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        result["texture_imported"].append(asset_path)
        return asset_path
    result["errors"].append("texture import failed: %s -> %s" % (src, asset_path))
    return None


def load_texture(path):
    return unreal.EditorAssetLibrary.load_asset(path) if path else None


def find(bp, name):
    try:
        return unreal.EditorUtilityLibrary.find_source_widget_by_name(bp, name)
    except Exception:
        return None


def add_widget(bp, cls, name, parent=None, result=None):
    existing = find(bp, name)
    if existing is not None:
        if result is not None:
            result["updated"].append(name)
        return existing

    if parent is None:
        attempts = [
            lambda: unreal.EditorUtilityLibrary.add_source_widget(bp, cls, name),
            lambda: unreal.EditorUtilityLibrary.add_source_widget(bp, cls, name, None),
            lambda: unreal.EditorUtilityLibrary.add_source_widget(bp, cls, name, ""),
        ]
    else:
        attempts = [
            lambda: unreal.EditorUtilityLibrary.add_source_widget(bp, cls, name, parent),
        ]

    last_error = None
    for attempt in attempts:
        try:
            widget = attempt()
            if widget is not None:
                if result is not None:
                    result["created"].append(name)
                return widget
        except Exception as exc:
            last_error = exc

    if result is not None:
        result["errors"].append("%s: add failed: %s" % (name, str(last_error)[:180]))
    return None


def set_canvas_slot(widget, x, y, w, h, z=0):
    if widget is None:
        return
    try:
        widget.slot.set_editor_property(
            "layout_data",
            unreal.AnchorData(
                offsets=unreal.Margin(float(x), float(y), float(w), float(h)),
                anchors=unreal.Anchors(
                    minimum=unreal.Vector2D(0.0, 0.0),
                    maximum=unreal.Vector2D(0.0, 0.0),
                ),
                alignment=unreal.Vector2D(0.0, 0.0),
            ),
        )
        widget.slot.set_editor_property("z_order", int(z))
    except Exception:
        pass


def set_fill_slot(widget):
    if widget is None or widget.slot is None:
        return
    for prop, value in [
        ("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL),
        ("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_FILL),
    ]:
        try:
            widget.slot.set_editor_property(prop, value)
        except Exception:
            pass


def set_vertical_center_fill_slot(widget):
    if widget is None or widget.slot is None:
        return
    for prop, value in [
        ("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL),
        ("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_CENTER),
    ]:
        try:
            widget.slot.set_editor_property(prop, value)
        except Exception:
            pass


def set_image(widget, texture_path):
    if widget is None:
        return
    tex = load_texture(texture_path)
    if tex is not None:
        try:
            widget.set_brush_from_texture(tex)
        except Exception:
            pass


def set_text(widget, text, size, color, justify="left"):
    if widget is None:
        return
    try:
        widget.set_text(text)
    except Exception:
        try:
            widget.set_editor_property("text", text)
        except Exception:
            pass
    try:
        font = widget.get_editor_property("font")
        font.set_editor_property("size", int(size))
        widget.set_editor_property("font", font)
    except Exception:
        pass
    try:
        widget.set_color_and_opacity(unreal.SlateColor(color))
    except Exception:
        pass
    try:
        widget.set_editor_property(
            "justification",
            unreal.TextJustify.LEFT if justify == "left" else unreal.TextJustify.CENTER,
        )
    except Exception:
        pass
    try:
        widget.set_auto_wrap_text(False)
    except Exception:
        pass


def create_blueprint(result):
    parent_class = unreal.load_class(None, PARENT_CLASS_PATH)
    if parent_class is None:
        result["errors"].append("parent class load failed: %s" % PARENT_CLASS_PATH)
        return None

    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset(ASSET_NAME, ASSET_DIR, unreal.WidgetBlueprint, factory)
    result["created_asset"] = bp is not None
    return bp


def has_required_widgets(bp):
    if bp is None:
        return False
    return all(find(bp, name) is not None for name in REQUIRED_WIDGETS)


def build():
    result = {
        "asset": ASSET_PATH,
        "created_asset": False,
        "loaded_existing": False,
        "created": [],
        "updated": [],
        "texture_existing": [],
        "texture_imported": [],
        "missing_sources": [],
        "errors": [],
    }

    if unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH):
        existing = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
        if has_required_widgets(existing):
            result["loaded_existing"] = True
            print("REWARD_ROW_WBP_BUILD|" + json.dumps(result, ensure_ascii=False))
            return
        result["deleted_incomplete_existing"] = unreal.EditorAssetLibrary.delete_asset(ASSET_PATH)

    frame_texture = import_texture(ROW_FRAME_SRC, result)
    gold_texture = import_texture(GOLD_ICON_SRC, result)
    bp = create_blueprint(result)
    if bp is None:
        print("REWARD_ROW_WBP_BUILD|" + json.dumps(result, ensure_ascii=False))
        return

    root = add_widget(bp, unreal.SizeBox, "RewardRowRootSizeBox", None, result)
    if root is not None:
        try:
            root.set_width_override(490.0)
            root.set_height_override(82.0)
        except Exception:
            pass

    canvas = add_widget(bp, unreal.CanvasPanel, "RewardRowCanvas", root.get_name() if root else None, result)
    set_fill_slot(canvas)
    canvas_name = canvas.get_name() if canvas else None

    frame = add_widget(bp, unreal.Image, "mRowIconFrame", canvas_name, result)
    set_canvas_slot(frame, 4.0, 1.0, 151.0, 79.0, 1)
    set_image(frame, frame_texture)

    icon = add_widget(bp, unreal.Image, "mRewardIcon", canvas_name, result)
    set_canvas_slot(icon, 39.0, 19.0, 43.0, 43.0, 2)
    set_image(icon, gold_texture)

    single_box = add_widget(bp, unreal.SizeBox, "RewardSingleTextBox", canvas_name, result)
    set_canvas_slot(single_box, 158.0, 0.0, 328.0, 82.0, 3)

    single_overlay = add_widget(bp, unreal.Overlay, "RewardSingleTextOverlay", single_box.get_name() if single_box else None, result)
    set_fill_slot(single_overlay)

    single = add_widget(bp, unreal.TextBlock, "mRewardSingleText", single_overlay.get_name() if single_overlay else None, result)
    set_vertical_center_fill_slot(single)
    set_text(single, "골드 +0", 30, lin(0.95, 0.90, 0.70, 1.0), "left")

    main = add_widget(bp, unreal.TextBlock, "mRewardMainText", canvas_name, result)
    set_canvas_slot(main, 158.0, 9.0, 328.0, 32.0, 3)
    set_text(main, "보상 이름", 25, lin(0.95, 0.90, 0.70, 1.0), "left")
    try:
        main.set_visibility(unreal.SlateVisibility.COLLAPSED)
    except Exception:
        pass

    sub = add_widget(bp, unreal.TextBlock, "mRewardSubText", canvas_name, result)
    set_canvas_slot(sub, 158.0, 44.0, 328.0, 30.0, 3)
    set_text(sub, "보상 설명", 19, lin(0.82, 0.80, 0.72, 1.0), "left")
    try:
        sub.set_visibility(unreal.SlateVisibility.COLLAPSED)
    except Exception:
        pass

    try:
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        result["compiled"] = True
    except Exception as exc:
        result["compiled"] = False
        result["errors"].append("compile failed: %s" % str(exc)[:180])

    try:
        result["saved"] = unreal.EditorAssetLibrary.save_asset(ASSET_PATH)
    except Exception as exc:
        result["saved"] = False
        result["errors"].append("save failed: %s" % str(exc)[:180])

    print("REWARD_ROW_WBP_BUILD|" + json.dumps(result, ensure_ascii=False))


build()
