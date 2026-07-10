import json
import os
import re
import unreal

ASSET_PATH = "/Game/BP/UI/WBP_Reward"
ASSET_NAME = "WBP_Reward"
ASSET_DIR = "/Game/BP/UI"
PARENT_CLASS_PATH = "/Script/P_RD.RewardUIWidgetBase"

CONCEPT_PATH = "D:/UnrealProjects/P_RD_develop_20260701_216/Start_CombatUIRectEditor/concept_reward_v4_11_center-list-scroll.json"
TEXTURE_DIR = "/Game/UI/RewardV4_11/Tex"

TEXTURE_NAMES = {
    "D:/UnrealProjects/P_RD_CombatUI_Assets_/P_RD_CombatUI_Assets_nobg/15_Reward/00_Background/reward_bg.png": "T_reward_v4_bg",
    "D:/UnrealProjects/P_RD_develop_20260701_216/output/combat_ui_kit/uploads/class_card_frame_selected_20260706_013700_20260706_022206.png": "T_reward_v4_panel_frame",
    "D:/UnrealProjects/P_RD_CombatUI_Assets_/P_RD_CombatUI_Assets_nobg/00_Common/Frames_Shared/nav_button_frame_normal.png": "T_reward_v4_scrollbar_track",
    "D:/UnrealProjects/P_RD_CombatUI_Assets_/P_RD_CombatUI_Assets_nobg/01_TopBar/03_HP_Pill/hp_fill_bar.png": "T_reward_v4_scrollbar_thumb",
    "D:/UnrealProjects/P_RD_develop_20260701_216/output/combat_ui_kit/uploads/nav_button_frame_normal_20260706_015417.png": "T_reward_v4_row_icon_frame",
    "D:/UnrealProjects/P_RD_CombatUI_Assets_/P_RD_CombatUI_Assets_nobg/01_TopBar/02_GOLD_Pill/gold_icon.png": "T_reward_v4_gold_icon",
    "D:/UnrealProjects/P_RD_CombatUI_Assets_/P_RD_CombatUI_Assets_nobg/17_Frontend_Common/00_Buttons/btn_frame_normal.png": "T_reward_v4_btn_frame_normal",
}

ROW_WIDTH = 490.0
ROW_HEIGHT = 82.0
ROW_GAP = 24.0
ROW_TEXT_X = 158.0
ROW_TEXT_Y = 17.0
ROW_TEXT_W = 328.0
ROW_TEXT_H = 48.0


def norm_path(path):
    return (path or "").replace("\\", "/")


def lin(r, g, b, a=1.0):
    return unreal.LinearColor(float(r), float(g), float(b), float(a))


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
    if not path:
        return None
    return unreal.EditorAssetLibrary.load_asset(path)


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

    attempts = []
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


def remove_if_exists(bp, name, result):
    widget = find(bp, name)
    if widget is None:
        return
    if hasattr(unreal.EditorUtilityLibrary, "remove_source_widget"):
        try:
            unreal.EditorUtilityLibrary.remove_source_widget(bp, name)
            result["removed"].append(name)
            return
        except Exception as exc:
            result["errors"].append("%s: remove failed: %s" % (name, str(exc)[:120]))
    try:
        widget.set_visibility(unreal.SlateVisibility.COLLAPSED)
        result["collapsed"].append(name)
    except Exception as exc:
        result["errors"].append("%s: collapse failed: %s" % (name, str(exc)[:120]))


def set_canvas_slot(widget, x, y, w, h, z=0, fill=False):
    if widget is None:
        return
    slot = widget.slot
    if fill:
        anchors = unreal.Anchors(
            minimum=unreal.Vector2D(0.0, 0.0),
            maximum=unreal.Vector2D(1.0, 1.0),
        )
        offsets = unreal.Margin(0.0, 0.0, 0.0, 0.0)
    else:
        anchors = unreal.Anchors(
            minimum=unreal.Vector2D(0.0, 0.0),
            maximum=unreal.Vector2D(0.0, 0.0),
        )
        offsets = unreal.Margin(float(x), float(y), float(w), float(h))
    layout_data = unreal.AnchorData(
        offsets=offsets,
        anchors=anchors,
        alignment=unreal.Vector2D(0.0, 0.0),
    )
    slot.set_editor_property("layout_data", layout_data)
    try:
        slot.set_editor_property("z_order", int(z))
    except Exception:
        pass


def set_fill_slot(widget):
    if widget is None:
        return
    for prop, value in [
        ("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL),
        ("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_FILL),
    ]:
        try:
            widget.slot.set_editor_property(prop, value)
        except Exception:
            pass


def set_center_slot(widget):
    if widget is None:
        return
    for prop, value in [
        ("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_CENTER),
        ("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_CENTER),
    ]:
        try:
            widget.slot.set_editor_property(prop, value)
        except Exception:
            pass


def set_vertical_row_slot(widget, top, bottom):
    if widget is None:
        return
    try:
        widget.slot.set_editor_property("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_CENTER)
        widget.slot.set_editor_property("padding", unreal.Margin(0.0, float(top), 0.0, float(bottom)))
    except Exception:
        pass


def set_text(widget, text, size, color, justify="center", auto_wrap=False):
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
        widget.set_auto_wrap_text(bool(auto_wrap))
    except Exception:
        pass


def set_image(widget, texture_path, color=None):
    if widget is None:
        return
    tex = load_texture(texture_path)
    if tex is not None:
        try:
            widget.set_brush_from_texture(tex)
        except Exception:
            pass
    if color is not None:
        try:
            widget.set_color_and_opacity(color)
        except Exception:
            pass


def set_border(widget, color, padding=None):
    if widget is None:
        return
    try:
        widget.set_brush_color(color)
    except Exception:
        pass
    if padding is not None:
        try:
            widget.set_padding(padding)
        except Exception:
            pass


def set_visibility(widget, visibility):
    if widget is None:
        return
    try:
        widget.set_visibility(visibility)
    except Exception:
        pass


def set_transparent_button_style(button, result):
    if button is None:
        return
    try:
        style = button.get_editor_property("widget_style")
        draw_as = None
        try:
            draw_as = unreal.SlateBrushDrawType.NO_DRAW_TYPE
        except Exception:
            try:
                draw_as = unreal.SlateBrushDrawType.NO_DRAW
            except Exception:
                draw_as = None

        transparent_tint = None
        try:
            transparent_tint = unreal.SlateColor()
            transparent_tint.set_editor_property("specified_color", lin(1.0, 1.0, 1.0, 0.0))
        except Exception:
            transparent_tint = None

        for prop in ["normal", "hovered", "pressed", "disabled"]:
            brush = style.get_editor_property(prop)
            for brush_prop, value in [
                ("resource_object", None),
                ("draw_as", draw_as),
                ("tint_color", transparent_tint),
            ]:
                if value is None and brush_prop != "resource_object":
                    continue
                try:
                    brush.set_editor_property(brush_prop, value)
                except Exception:
                    pass
            style.set_editor_property(prop, brush)
        style.set_editor_property("normal_padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
        style.set_editor_property("pressed_padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
        button.set_editor_property("widget_style", style)
    except Exception as exc:
        result["errors"].append("transparent button style failed: %s" % str(exc)[:160])


def inner_transform(element, out_w, out_h):
    base = element.get("innerDesignSize") or [element.get("screenRect", [0, 0, out_w, out_h])[2], element.get("screenRect", [0, 0, out_w, out_h])[3]]
    base_w = max(1.0, float(base[0]))
    base_h = max(1.0, float(base[1]))
    mode = element.get("innerScaleMode") or "stretch"
    sx = float(out_w) / base_w
    sy = float(out_h) / base_h
    if mode == "contain":
        sx = sy = min(sx, sy)
    elif mode == "cover":
        sx = sy = max(sx, sy)
    elif mode == "width":
        sx = sy = float(out_w) / base_w
    elif mode == "height":
        sx = sy = float(out_h) / base_h
    content_w = base_w * sx
    content_h = base_h * sy
    align_h = element.get("innerAlignH") or "center"
    align_v = element.get("innerAlignV") or "center"
    ox = 0.0 if align_h == "left" else (float(out_w) - content_w if align_h == "right" else (float(out_w) - content_w) * 0.5)
    oy = 0.0 if align_v == "top" else (float(out_h) - content_h if align_v == "bottom" else (float(out_h) - content_h) * 0.5)
    return ox, oy, sx, sy


def region_rect(element, region, out_w, out_h):
    ox, oy, sx, sy = inner_transform(element, out_w, out_h)
    r = region.get("rect") or [0, 0, out_w, out_h]
    return ox + float(r[0]) * sx, oy + float(r[1]) * sy, float(r[2]) * sx, float(r[3]) * sy


def make_text_for_region(region, fallback):
    # concept가 지정한 표시 텍스트만 사용한다(하드코딩 주입 금지). 없으면 fallback(보통 빈 문자열).
    txt = region.get("text")
    return txt if isinstance(txt, str) and txt != "" else (fallback or "")


def add_region_widgets(bp, parent_canvas_name, element, prefix, base_z, result, text_fallbacks=None):
    text_fallbacks = text_fallbacks or {}
    rect = element.get("screenRect") or [0, 0, 1, 1]
    out_w = max(1.0, float(rect[2]))
    out_h = max(1.0, float(rect[3]))
    for idx, region in enumerate(element.get("regions", [])):
        rtype = region.get("type") or "content"
        x, y, w, h = region_rect(element, region, out_w, out_h)
        name = "%s_%02d_%s" % (prefix, idx, safe_name(region.get("name") or rtype))
        if rtype in ["text", "value"]:
            text = add_widget(bp, unreal.TextBlock, name, parent_canvas_name, result)
            set_canvas_slot(text, x, y, w, h, base_z + idx + 1)
            align = "left" if region.get("textAlignH") == "left" else "center"
            set_text(text, make_text_for_region(region, text_fallbacks.get(idx, "")), 24, lin(0.94, 0.90, 0.76, 1.0), align)
            continue

        media_src = norm_path(region.get("imageAsset") or "")
        if not media_src:
            continue
        texture_path = import_texture(media_src, result)
        image = add_widget(bp, unreal.Image, name, parent_canvas_name, result)
        set_canvas_slot(image, x, y, w, h, base_z + idx + 1)
        set_image(image, texture_path, lin(1.0, 1.0, 1.0, 1.0))


def create_or_load_blueprint(result):
    parent_class = unreal.load_class(None, PARENT_CLASS_PATH)
    if parent_class is None:
        result["errors"].append("parent class load failed: %s" % PARENT_CLASS_PATH)
        return None

    if unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH):
        result["deleted_existing"] = unreal.EditorAssetLibrary.delete_asset(ASSET_PATH)

    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    created = tools.create_asset(ASSET_NAME, ASSET_DIR, unreal.WidgetBlueprint, factory)
    result["created_asset"] = created is not None
    return created


def collect_generated_names(elements, names, path_prefix=""):
    for idx, element in enumerate(elements):
        element_name = "%s%02d_%s" % (path_prefix, idx, safe_name(element.get("name")))
        names.extend([
            "RewardElem_%s" % element_name,
            "RewardElem_%s_Canvas" % element_name,
        ])
        for ridx, region in enumerate(element.get("regions", [])):
            names.append("RewardElem_%s_%02d_%s" % (element_name, ridx, safe_name(region.get("name") or region.get("type"))))
        collect_generated_names(element.get("children", []), names, element_name + "_")


def add_reward_row_art(bp, row_canvas_name, child, row_name, result):
    rect = child.get("screenRect") or [0, 0, ROW_WIDTH, ROW_HEIGHT]
    out_w = max(1.0, float(rect[2]))
    out_h = max(1.0, float(rect[3]))
    z = int(child.get("zOrder", 0))

    for ridx, region in enumerate(child.get("regions", [])):
        if region.get("type") in ["text", "value"]:
            continue

        media_src = norm_path(region.get("imageAsset") or "")
        if not media_src:
            continue

        x, y, w, h = region_rect(child, region, out_w, out_h)
        image_name = "%s_%02d_%s" % (row_name, ridx, safe_name(region.get("name") or region.get("type")))
        image = add_widget(bp, unreal.Image, image_name, row_canvas_name, result)
        set_canvas_slot(image, x, y, w, h, z + ridx + 1)
        set_image(image, import_texture(media_src, result), lin(1.0, 1.0, 1.0, 1.0))


def build():
    result = {
        "asset": ASSET_PATH,
        "created_asset": False,
        "loaded_existing": False,
        "created": [],
        "updated": [],
        "removed": [],
        "collapsed": [],
        "texture_imported": [],
        "texture_existing": [],
        "missing_sources": [],
        "errors": [],
    }

    with open(CONCEPT_PATH, encoding="utf-8") as fp:
        concept = json.load(fp)
    screen = concept["screens"][0]
    result["concept_title"] = concept.get("title")

    # Import all imageAsset paths from the JSON before assigning brushes.
    def import_from_elements(elements):
        for element in elements:
            if element.get("asset"):
                import_texture(element["asset"], result)
            for region in element.get("regions", []):
                if region.get("imageAsset"):
                    import_texture(region["imageAsset"], result)
            import_from_elements(element.get("children", []))
    import_from_elements(screen.get("elements", []))

    bp = create_or_load_blueprint(result)
    if bp is None:
        print("REWARD_WBP_BUILD|" + json.dumps(result, ensure_ascii=False))
        return

    remove_names = [
        "RewardAspectScaleBox",
        "RewardDesignSizeBox",
        "RewardDesignCanvas",
        "mTitleText",
        "RewardRowsScrollBox",
        "mRewardRowsBox",
        "RewardScrollbarTrack",
        "RewardScrollbarThumb",
        "mCloseButton",
        "mCloseButtonCanvas",
        "mCloseButtonFrame",
        "mCloseButtonText",
    ]
    for i in range(1, 4):
        remove_names.extend([
            "RewardPreviewRow%d_Size" % i,
            "RewardPreviewRow%d_Canvas" % i,
        ])
    collect_generated_names(screen.get("elements", []), remove_names)
    # Remove children before parents in case remove_source_widget does not cascade consistently.
    for name in sorted(set(remove_names), key=len, reverse=True):
        remove_if_exists(bp, name, result)

    root = add_widget(bp, unreal.CanvasPanel, "RewardRootCanvas", None, result)

    scale = add_widget(bp, unreal.ScaleBox, "RewardAspectScaleBox", root.get_name() if root else None, result)
    set_canvas_slot(scale, 0, 0, 0, 0, 0, True)
    if scale is not None:
        try:
            scale.set_stretch(unreal.Stretch.SCALE_TO_FIT)
        except Exception:
            pass

    size = add_widget(bp, unreal.SizeBox, "RewardDesignSizeBox", scale.get_name() if scale else None, result)
    set_center_slot(size)
    if size is not None:
        try:
            size.set_width_override(float(concept.get("designSize", [1920, 1080])[0]))
            size.set_height_override(float(concept.get("designSize", [1920, 1080])[1]))
        except Exception:
            pass

    canvas = add_widget(bp, unreal.CanvasPanel, "RewardDesignCanvas", size.get_name() if size else None, result)
    set_fill_slot(canvas)
    canvas_name = canvas.get_name() if canvas else None

    for element_index, element in enumerate(screen.get("elements", [])):
        element_name = "%02d_%s" % (element_index, safe_name(element.get("name")))
        rect = element.get("screenRect") or [0, 0, 1, 1]
        z = int(element.get("zOrder", 0))

        if element.get("name") == "보상 목록 스크롤 뷰포트":
            scroll = add_widget(bp, unreal.ScrollBox, "RewardRowsScrollBox", canvas_name, result)
            set_canvas_slot(scroll, rect[0], rect[1], rect[2], rect[3], z)
            try:
                scroll.set_clipping(unreal.WidgetClipping.CLIP_TO_BOUNDS)
            except Exception:
                pass
            # 실제 ScrollBox 자체 바만 쓴다. 내용이 넘칠 때(4줄 이상)만 뜨도록 상시표시를 끄고 슬림하게 둔다.
            for prop, value in [("always_show_scrollbar", False), ("scroll_bar_thickness", 6.0)]:
                try:
                    scroll.set_editor_property(prop, value)
                except Exception:
                    pass
            rows = add_widget(bp, unreal.VerticalBox, "mRewardRowsBox", scroll.get_name() if scroll else None, result)
            # 실제 보상 행은 URewardUIWidgetBase가 mRewardRowsBox에 런타임 생성한다.
            # 여기에는 스크롤/패널 레이아웃 컨테이너만 둔다.

            # JSON의 스크롤바는 정적 이미지 오버레이라 실제 스크롤 위치를 반영하지 못한다.
            # 위젯은 남겨(레이아웃 계약 유지) 두되 기본 접힘 처리해, 실제 ScrollBox 바만 보이게 한다.
            for ridx, region in enumerate(element.get("regions", [])):
                media_src = norm_path(region.get("imageAsset") or "")
                if not media_src:
                    continue
                x, y, w, h = region_rect(element, region, rect[2], rect[3])
                image_name = "RewardScrollbarTrack" if "트랙" in region.get("name", "") else "RewardScrollbarThumb"
                image = add_widget(bp, unreal.Image, image_name, canvas_name, result)
                set_canvas_slot(image, rect[0] + x, rect[1] + y, w, h, z + 20 + ridx)
                set_image(image, import_texture(media_src, result), lin(1.0, 1.0, 1.0, 1.0))
                try:
                    image.set_visibility(unreal.SlateVisibility.COLLAPSED)
                except Exception:
                    pass
            continue

        if element.get("name") == "element_5":
            button = add_widget(bp, unreal.Button, "mCloseButton", canvas_name, result)
            set_canvas_slot(button, rect[0], rect[1], rect[2], rect[3], z)
            set_transparent_button_style(button, result)
            button_tex = import_texture(element.get("asset"), result)

            button_canvas = add_widget(bp, unreal.CanvasPanel, "mCloseButtonCanvas", button.get_name() if button else None, result)
            set_fill_slot(button_canvas)
            button_frame = add_widget(bp, unreal.Image, "mCloseButtonFrame", button_canvas.get_name() if button_canvas else None, result)
            set_canvas_slot(button_frame, 0.0, 0.0, rect[2], rect[3], 0)
            set_image(button_frame, button_tex, lin(1.0, 1.0, 1.0, 1.0))
            text = add_widget(bp, unreal.TextBlock, "mCloseButtonText", button_canvas.get_name() if button_canvas else None, result)
            set_canvas_slot(text, 0.0, 42.0, rect[2], 44.0, z + 1)
            set_text(text, "닫기", 30, lin(0.23, 0.23, 0.23, 1.0), "center", False)
            continue

        if element.get("name") == "배경":
            continue

        elem_canvas = add_widget(bp, unreal.CanvasPanel, "RewardElem_%s_Canvas" % element_name, canvas_name, result)
        set_canvas_slot(elem_canvas, rect[0], rect[1], rect[2], rect[3], z)
        add_region_widgets(bp, elem_canvas.get_name() if elem_canvas else None, element, "RewardElem_%s" % element_name, z, result)

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

    print("REWARD_WBP_BUILD|" + json.dumps(result, ensure_ascii=False))


build()
