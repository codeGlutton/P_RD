"""Dump a faithful render spec (rect/text/font/colour) for the detail WBPs.

Paired with make_detail_preview.py, which turns this into a self-contained HTML
preview. Kept separate because only this half needs the editor.
"""

import json
from pathlib import Path

import unreal

OUT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/detail_render_spec.json")

TARGETS = [
    "/Game/UI/CombatDetail/WBP_SkillDetail_Marchbound",
    "/Game/UI/CombatDetail/WBP_EnemyDetail_Marchbound",
]


def prop(obj, name):
    try:
        return obj.get_editor_property(name)
    except Exception:  # noqa: BLE001
        return None


def to_hex(color):
    if color is None:
        return None
    try:
        red, green, blue, alpha = color.r, color.g, color.b, color.a
    except AttributeError:
        return None

    def channel(value):
        # sRGB 근사. 미리보기용이라 감마만 맞으면 충분하다.
        clamped = max(0.0, min(1.0, float(value))) ** (1.0 / 2.2)
        return int(round(clamped * 255))

    return "rgba(%d,%d,%d,%.3f)" % (channel(red), channel(green), channel(blue), float(alpha))


def slate_color_hex(value):
    if value is None:
        return None
    specified = prop(value, "specified_color")
    return to_hex(specified if specified is not None else value)


def texture_path(brush_owner, *names):
    for name in names:
        brush = prop(brush_owner, name)
        if brush is None:
            continue
        resource = prop(brush, "resource_object")
        if resource is not None:
            try:
                return str(resource.get_path_name())
            except Exception:  # noqa: BLE001
                continue
    return None


result = {}
for asset_path in TARGETS:
    blueprint = unreal.EditorAssetLibrary.load_asset(asset_path)
    if blueprint is None:
        continue
    tree_prefix = blueprint.get_path_name() + ":WidgetTree."
    widgets = []
    for obj in unreal.ObjectIterator():
        if not isinstance(obj, unreal.Widget):
            continue
        if not str(obj.get_path_name()).startswith(tree_prefix):
            continue
        slot = prop(obj, "slot")
        if not isinstance(slot, unreal.CanvasPanelSlot):
            continue
        layout = prop(slot, "layout_data")
        offsets = prop(layout, "offsets")
        entry = {
            "name": str(obj.get_name()),
            "class": str(obj.get_class().get_name()),
            "rect": [offsets.left, offsets.top, offsets.right, offsets.bottom],
            "z": int(prop(slot, "z_order") or 0),
        }

        if isinstance(obj, unreal.TextBlock):
            entry["text"] = str(prop(obj, "text") or "")
            font = prop(obj, "font")
            entry["fontSize"] = int(prop(font, "size") or 24) if font is not None else 24
            entry["color"] = slate_color_hex(prop(obj, "color_and_opacity"))
            justify = prop(obj, "justification")
            entry["justify"] = str(justify).rsplit(".", 1)[-1].lower() if justify is not None else "center"
            entry["wrap"] = bool(prop(obj, "auto_wrap_text"))
        elif isinstance(obj, unreal.Image):
            entry["texture"] = texture_path(obj, "brush")
        elif isinstance(obj, unreal.Border):
            entry["color"] = to_hex(prop(obj, "brush_color"))
        elif isinstance(obj, unreal.ProgressBar):
            entry["percent"] = float(prop(obj, "percent") or 0.0)
            entry["color"] = to_hex(prop(obj, "fill_color_and_opacity"))

        widgets.append(entry)

    widgets.sort(key=lambda item: item["z"])
    result[asset_path.rsplit("/", 1)[-1]] = widgets

OUT.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
