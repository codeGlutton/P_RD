"""상세 오버레이 핵심 위젯의 슬롯·z·가시성·글꼴을 본다."""
from pathlib import Path

import unreal

RESULT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/overlay_state.txt")
LINES = []
blueprint = unreal.EditorAssetLibrary.load_asset(
    "/Game/UI/CombatDetail/WBP_CombatDetailOverlay")
tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")

for name in ("DetailFrameImage", "DetailScrimBg", "DetailTint",
             "DetailTitleText_Center", "DetailTitleText",
             "DetailIconImage", "DetailIconFrame", "DetailIdentityColumn"):
    widget = unreal.find_object(None, f"{tree.get_path_name()}.{name}")
    if widget is None:
        LINES.append(f"{name}: 없음")
        continue
    slot = widget.get_editor_property("slot")
    info = [f"vis={widget.get_editor_property('visibility')}",
            f"slot={type(slot).__name__ if slot else '-'}"]
    if isinstance(slot, unreal.CanvasPanelSlot):
        offsets = slot.get_offsets()
        anchors = slot.get_anchors()
        info.append(f"z={slot.get_z_order()}")
        info.append(f"box=({offsets.left:.0f},{offsets.top:.0f},"
                    f"{offsets.right:.0f},{offsets.bottom:.0f})")
        info.append(f"anchor=({anchors.minimum.x:.2f},{anchors.minimum.y:.2f})"
                    f"~({anchors.maximum.x:.2f},{anchors.maximum.y:.2f})")
    parent = widget.get_parent()
    info.append(f"parent={parent.get_name() if parent else '-'}")
    if isinstance(widget, unreal.TextBlock):
        info.append(f"text='{widget.get_editor_property('text')}'")
        font = widget.get_editor_property("font")
        info.append(f"size={font.get_editor_property('size')}")
        col = widget.get_editor_property("color_and_opacity")
        info.append(f"color={col.get_editor_property('specified_color')}")
    if isinstance(widget, unreal.Image):
        col = widget.get_editor_property("color_and_opacity")
        info.append(f"tint={col}")
        info.append(f"opacity={widget.get_editor_property('render_opacity')}")
    LINES.append(f"{name}: " + " | ".join(str(part) for part in info))

RESULT.write_text("\n".join(LINES) + "\n", encoding="utf-8")
