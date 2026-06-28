# concept_02 HUD v10 - unified designer coordinate space.
#
# The JSON editor works in a 1920x1080 design space. Older v8 builds put the
# top bar into one ScaleBox and the rest of the HUD into viewport-stretched
# anchors, which distorted square assets on non-16:9 devices. This build keeps
# every designer asset/marker in one 1920x1080 DesignCanvas, then scales that
# canvas once with a ScaleBox.
#
# Important: create the generated WidgetBlueprint from a clean factory. The old
# duplicate-source-and-delete-root path leaves deleted designer widget GUIDs in
# the blueprint and can cook into an Android asset that fails with
# ObjectSerializationError / Bad export index.
import json
import os
import re

import unreal
import UnrealMCPython.asset_actions as A
import UnrealMCPython.umg_actions as U

DIR = "/Game/UI/Concept02"
TEXDIR = DIR + "/Tex"
HUD = DIR + "/WBP_Concept02_HUD"
NAME = "WBP_Concept02_HUD"
PARENT_CLASS = "/Script/P_RD.CombatTileMapHUDWidget"
CONCEPT = "D:/UnrealProjects/P_RD_develop_20260622/Start_CombatUIRectEditor/concept_02_left-rail.json"

EXCLUDE = {"tilemap_safe_area"}
GROUP_ANCHORS = {
    "HUD_SkillRail": "skill_rail",
    "HUD_DiceTray": "dice_tray",
    "HUD_Move": "btn_move",
    "HUD_EndTurn": "btn_end_turn",
    "HUD_TurnOrder": "turn_order",
    # 탑바 내비 버튼 앵커 — C++가 이 자리에 투명 UButton을 얹어 클릭을 받고 TopMenuBar 패널 토글로 위임한다.
    "HUD_Map": "btn_map",
    "HUD_Dice": "btn_dice",
    "HUD_Skill": "btn_skill",
    "HUD_Settings": "btn_settings",
}
DW, DH = 1920.0, 1080.0

eal = unreal.EditorAssetLibrary
res = {"img": 0, "anchors": 0, "err": [], "imp": 0, "markers": 0}


def tname(fp):
    return "T_" + re.sub(r"[^A-Za-z0-9_]", "_", os.path.splitext(os.path.basename(fp))[0])[:86]


def brush(dest, w, h):
    obj = dest + "." + dest.split("/")[-1]
    return "(ResourceObject=/Script/Engine.Texture2D'\"%s\"',ImageSize=(X=%.1f,Y=%.1f),DrawAs=Image)" % (obj, w, h)


def set_canvas_layout(widget_name, x, y, w, h, z=None):
    U.ue_set_slot_layout(
        asset_path=HUD,
        widget_name=widget_name,
        anchor_min_x=0.0,
        anchor_min_y=0.0,
        anchor_max_x=0.0,
        anchor_max_y=0.0,
        offset_x=float(x),
        offset_y=float(y),
        size_x=float(w),
        size_y=float(h),
    )
    if z is not None:
        try:
            widget = unreal.load_object(None, "%s.%s:WidgetTree.%s" % (HUD, NAME, widget_name))
            widget.slot.set_editor_property("z_order", int(z))
        except Exception:
            pass


def create_clean_hud_asset():
    if eal.does_asset_exist(HUD):
        eal.delete_asset(HUD)

    parent_class = unreal.load_class(None, PARENT_CLASS)
    if parent_class is None:
        raise RuntimeError("parent class load failed: %s" % PARENT_CLASS)

    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    widget_bp = asset_tools.create_asset(NAME, DIR, unreal.WidgetBlueprint, factory)
    if widget_bp is None:
        raise RuntimeError("failed to create WidgetBlueprint: %s" % HUD)

    try:
        unreal.BlueprintEditorLibrary.compile_blueprint(widget_bp)
    except Exception as ex:
        res["err"].append("InitialCompile: " + str(ex)[:80])
    unreal.EditorAssetLibrary.save_loaded_asset(widget_bp)
    return widget_bp


def fill_canvas(widget_name):
    widget = unreal.load_object(None, "%s.%s:WidgetTree.%s" % (HUD, NAME, widget_name))
    slot = widget.slot
    anchors = unreal.Anchors()
    anchors.set_editor_property("minimum", unreal.Vector2D(0.0, 0.0))
    anchors.set_editor_property("maximum", unreal.Vector2D(1.0, 1.0))
    margin = unreal.Margin()
    for key in ("left", "top", "right", "bottom"):
        margin.set_editor_property(key, 0.0)
    data = unreal.AnchorData()
    data.set_editor_property("anchors", anchors)
    data.set_editor_property("offsets", margin)
    data.set_editor_property("alignment", unreal.Vector2D(0.0, 0.0))
    slot.set_editor_property("layout_data", data)


def _cell_main(cell, horiz):
    rect = cell.get("screenRect") or cell.get("rect") or [0, 0, 1, 1]
    return float(rect[2] if horiz else rect[3])


def _cell_cross(cell, horiz):
    rect = cell.get("screenRect") or cell.get("rect") or [0, 0, 1, 1]
    return float(rect[3] if horiz else rect[2])


def flow_layout(elem, cw, ch):
    horiz = elem.get("layout") != "flow-v"
    cells = elem.get("children") or []
    count = len(cells)
    gap = float(elem.get("gap", 0))
    pad = float(elem.get("pad", 0))
    fixed = 0.0
    grow = 0.0
    for cell in cells:
        if cell.get("grow", 0) > 0:
            grow += cell["grow"]
        else:
            fixed += _cell_main(cell, horiz)

    bar_main = cw if horiz else ch
    natural = 2 * pad + gap * max(0, count - 1) + fixed
    shrink = (bar_main / natural) if (elem.get("shrink") is not False and natural > bar_main and bar_main > 0) else 1.0
    gap *= shrink
    pad *= shrink
    avail = bar_main - 2 * pad - gap * max(0, count - 1)
    free = max(0.0, avail - fixed * shrink)

    out = []
    cur = pad
    for cell in cells:
        if cell.get("grow", 0) > 0:
            main = (free * (cell["grow"] / grow)) if grow > 0 else 0.0
        else:
            main = _cell_main(cell, horiz) * shrink
        cross = _cell_cross(cell, horiz) * shrink
        if horiz:
            height = min(cross, ch - 2 * pad)
            out.append((cur, pad + ((ch - 2 * pad) - height) / 2, main, height))
        else:
            width = min(cross, cw - 2 * pad)
            out.append((pad + ((cw - 2 * pad) - width) / 2, cur, width, main))
        cur += main + gap
    return out


def flatten(elem, box):
    ax, ay, w, h = box
    items = []
    if elem.get("asset"):
        items.append({"img": True, "name": elem.get("name", ""), "asset": elem["asset"], "rect": (ax, ay, w, h), "z": elem.get("zOrder", 0)})

    children = elem.get("children") or []
    if elem.get("layout") in ("flow-h", "flow-v") and children:
        for child, (cx, cy, cw, ch) in zip(children, flow_layout(elem, w, h)):
            items += flatten(child, (ax + cx, ay + cy, cw, ch))

    for region in elem.get("regions", []):
        rx, ry, rw, rh = region["rect"]
        items.append({
            "img": bool(region.get("imageAsset")),
            "name": region.get("name", ""),
            "asset": region.get("imageAsset"),
            "rect": (ax + rx, ay + ry, rw, rh),
            "z": elem.get("zOrder", 0),
        })
    return items


doc = json.load(open(CONCEPT, encoding="utf-8"))
all_elems = doc["screens"][0]["elements"]
elems = [elem for elem in all_elems if elem["name"] not in EXCLUDE]
elem_by_name = {elem["name"]: elem for elem in all_elems}
flat = {elem["name"]: flatten(elem, tuple(elem["screenRect"])) for elem in elems}

tex = {}
for elem in elems:
    for item in flat[elem["name"]]:
        if item["img"] and item["asset"]:
            tex[item["asset"]] = TEXDIR + "/" + tname(item["asset"])

for fp, dest in tex.items():
    if eal.does_asset_exist(dest):
        continue
    if os.path.exists(fp):
        try:
            A.ue_import_texture(file_path=os.path.normpath(fp), destination_path=TEXDIR, destination_name=tname(fp))
            res["imp"] += 1
        except Exception as ex:
            res["err"].append("TEX " + tname(fp) + ": " + str(ex)[:80])

create_clean_hud_asset()

U.ue_add_widget(asset_path=HUD, widget_type="CanvasPanel", widget_name="RootCanvas", parent_name="")
U.ue_add_widget(asset_path=HUD, widget_type="ScaleBox", widget_name="DesignScaleBox", parent_name="RootCanvas")
U.ue_add_widget(asset_path=HUD, widget_type="SizeBox", widget_name="DesignSizeBox", parent_name="DesignScaleBox")
U.ue_add_widget(asset_path=HUD, widget_type="CanvasPanel", widget_name="DesignCanvas", parent_name="DesignSizeBox")

try:
    fill_canvas("DesignScaleBox")
    scale_box = unreal.load_object(None, "%s.%s:WidgetTree.DesignScaleBox" % (HUD, NAME))
    scale_box.set_editor_property("stretch", unreal.Stretch.SCALE_TO_FIT)
    try:
        scale_box.set_editor_property("stretch_direction", unreal.StretchDirection.BOTH)
    except Exception:
        pass
except Exception as ex:
    res["err"].append("DesignScaleBox: " + str(ex)[:80])

try:
    size_box = unreal.load_object(None, "%s.%s:WidgetTree.DesignSizeBox" % (HUD, NAME))
    size_box.set_width_override(DW)
    size_box.set_height_override(DH)
    try:
        size_box.slot.set_editor_property("horizontal_alignment", unreal.HorizontalAlignment.HALIGN_LEFT)
        size_box.slot.set_editor_property("vertical_alignment", unreal.VerticalAlignment.VALIGN_TOP)
    except Exception:
        pass
except Exception as ex:
    res["err"].append("DesignSizeBox: " + str(ex)[:80])

idx = 0
for elem in elems:
    elem_name = re.sub(r"[^A-Za-z0-9_]", "_", elem["name"])
    for item in flat[elem["name"]]:
        if not item["img"]:
            continue
        fp = item["asset"]
        if not fp or fp not in tex:
            continue
        x, y, w, h = item["rect"]
        leaf_name = re.sub(r"[^A-Za-z0-9_]", "_", item["name"] or "")
        idx += 1
        widget_name = "R_%s_%s_%d" % (elem_name, leaf_name, idx)
        try:
            U.ue_add_widget(asset_path=HUD, widget_type="Image", widget_name=widget_name, parent_name="DesignCanvas")
            U.ue_set_widget_property(asset_path=HUD, widget_name=widget_name, property_name="Brush", value=brush(tex[fp], w, h))
            set_canvas_layout(widget_name, x, y, w, h, item.get("z", 0))
            res["img"] += 1
        except Exception as ex:
            res["err"].append(widget_name + ": " + str(ex)[:80])

for anchor_name, elem_name in GROUP_ANCHORS.items():
    elem = elem_by_name.get(elem_name)
    if elem is None:
        continue
    x, y, w, h = elem["screenRect"]
    try:
        U.ue_add_widget(asset_path=HUD, widget_type="Image", widget_name=anchor_name, parent_name="DesignCanvas")
        U.ue_set_widget_property(asset_path=HUD, widget_name=anchor_name, property_name="Brush", value="(DrawAs=NoDrawType)")
        set_canvas_layout(anchor_name, x, y, w, h, elem.get("zOrder", 0))
        res["anchors"] += 1
    except Exception as ex:
        res["err"].append(anchor_name + ": " + str(ex)[:80])

# 텍스트 마커(값 + 고정 라벨)는 WBP가 실제 TextBlock으로 소유한다: 마커 자리(중심)에 심고 폰트/색/정렬을
# WBP가 선언적으로 들고, C++는 SetText만 한다(런타임 정렬 핵 제거). 그 외 마커는 NoDrawType 앵커 유지.
#   { 마커이름: (기본텍스트, 줄수) } — 줄수로 폰트 높이 분배.
TEXT_MARKERS = {
    "lv_value": ("0", 1),
    "gold_value": ("0", 1),
    "hp_value": ("0/0", 1),
    "btn_move_label": ("MOVE\n0/0", 2),
    "btn_end_turn_label": ("END\nTURN", 2),
    "room_name_text": ("", 1),
}


def place_text_marker(widget_name, x, y, w, h, z, text, lines):
    # 검증된 안전 구조: color/font/slot만 CDO에 둔다(Android unversioned 직렬화에서 text(FText)/justify/visibility를
    # CDO에 넣으면 Bad export index로 로드 실패). text/정렬/가시성은 C++가 런타임에 SetText/SetJustification으로 채운다.
    tb = unreal.load_object(None, "%s.%s:WidgetTree.%s" % (HUD, NAME, widget_name))
    color = unreal.SlateColor()
    color.set_editor_property("specified_color", unreal.LinearColor(0.98, 1.0, 0.92, 1.0))
    tb.set_editor_property("color_and_opacity", color)
    font = tb.get_editor_property("font")
    font.set_editor_property("size", max(8, min(80, int(round((float(h) / max(1, lines)) * 0.62)))))
    tb.set_editor_property("font", font)
    # autosize 블록을 점앵커+alignment(0.5,0.5)+position(마커 중심)으로 칸 중앙 정렬.
    anchors = unreal.Anchors()
    anchors.set_editor_property("minimum", unreal.Vector2D(0.0, 0.0))
    anchors.set_editor_property("maximum", unreal.Vector2D(0.0, 0.0))
    margin = unreal.Margin()
    margin.set_editor_property("left", float(x + w * 0.5))
    margin.set_editor_property("top", float(y + h * 0.5))
    margin.set_editor_property("right", 0.0)
    margin.set_editor_property("bottom", 0.0)
    data = unreal.AnchorData()
    data.set_editor_property("anchors", anchors)
    data.set_editor_property("offsets", margin)
    data.set_editor_property("alignment", unreal.Vector2D(0.5, 0.5))
    slot = tb.slot
    slot.set_editor_property("layout_data", data)
    slot.set_editor_property("auto_size", True)
    slot.set_editor_property("z_order", int(z))


seen_markers = set()
for elem in elems:
    for item in flat[elem["name"]]:
        if item["img"]:
            continue
        marker = re.sub(r"[^A-Za-z0-9_]", "_", item["name"] or "")
        if not marker or marker in seen_markers:
            continue
        seen_markers.add(marker)
        x, y, w, h = item["rect"]
        widget_name = "HUD_M_" + marker
        try:
            if marker in TEXT_MARKERS:
                default_text, lines = TEXT_MARKERS[marker]
                U.ue_add_widget(asset_path=HUD, widget_type="TextBlock", widget_name=widget_name, parent_name="DesignCanvas")
                place_text_marker(widget_name, x, y, w, h, item.get("z", 0), default_text, lines)
            else:
                U.ue_add_widget(asset_path=HUD, widget_type="Image", widget_name=widget_name, parent_name="DesignCanvas")
                U.ue_set_widget_property(asset_path=HUD, widget_name=widget_name, property_name="Brush", value="(DrawAs=NoDrawType)")
                set_canvas_layout(widget_name, x, y, w, h, item.get("z", 0))
            res["markers"] += 1
        except Exception as ex:
            res["err"].append(widget_name + ": " + str(ex)[:80])

U.ue_compile_widget_blueprint(asset_path=HUD)
A.ue_save_asset(asset_path=HUD)

if unreal.load_class(None, "%s.%s_C" % (HUD, NAME)) is None:
    res["err"].append("GeneratedClassLoad: failed")

unreal.log_warning("CONCEPT02V10 RESULT " + json.dumps(res, ensure_ascii=False))
