"""Dump the live world-map WBP so a re-layout does not break wiring.

``UFrontendMapWidget`` 은 아래 이름들을 BindWidgetOptional 로 잡는다. 없어도
컴파일은 되지만 조용히 nullptr 이 되어 기능만 빠진다. 다시 짜기 전에 지금
무엇이 있고 어디에 어떤 크기로 앉아 있는지부터 적어 둔다.

WidgetTree.RootWidget 은 파이썬에 안 열려 있고 트리를 훑는 길도 없다.
그래서 헤더에서 뽑은 이름을 하나씩 찍어 본다.
"""

from pathlib import Path

import unreal

OUT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/frontend_map_tree.txt")
ASSET = "/Game/UI/WBP_FrontendMap"
LINES = []

NAMES = [
    "CloseButton", "EnterRoomButton", "CloseButtonText", "EnterButtonText",
    "MapStatusText", "MapTitleText", "MapPreviewTitleText",
    "MapPreviewDescriptionText", "MapPreviewStateText", "MapPreviewPanel",
    "MapGraphSize", "MapGraphCanvas", "MapScrollBox", "MapLegendScroll",
    "MapLegendList", "MapLegendTitle", "MapDimBackground", "MapPaperPanel",
    "MapPaperShadow", "Map_Scrim", "Map_ParchmentBody", "Map_ScrollRodTop",
    "Map_ScrollRodBottom", "Map_NodeArea", "Map_NodeMetrics", "Map_ColPitch",
    "Map_CurrentMarker", "Map_SelectGlow", "Map_LegendGroup",
]


def describe(widget):
    slot = widget.get_editor_property("slot")
    if isinstance(slot, unreal.CanvasPanelSlot):
        data = slot.get_editor_property("layout_data")
        off = data.get_editor_property("offsets")
        anc = data.get_editor_property("anchors")
        return (f"canvas pos=({off.left:.0f},{off.top:.0f}) size=({off.right:.0f},{off.bottom:.0f})"
                f" anchor=({anc.minimum.x:.2f},{anc.minimum.y:.2f})"
                f"-({anc.maximum.x:.2f},{anc.maximum.y:.2f})")
    return type(slot).__name__ if slot is not None else "no slot"


blueprint = unreal.EditorAssetLibrary.load_asset(ASSET)
tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")
LINES.append(f"=== {ASSET} ===")
for name in NAMES:
    widget = unreal.find_object(None, tree.get_path_name() + "." + name)
    if widget is None:
        LINES.append(f"  {name:26s} 없음")
        continue
    parent = widget.get_parent()
    LINES.append(f"  {name:26s} <{type(widget).__name__}> parent={parent.get_name() if parent else '-'}"
                 f"  {describe(widget)}")

OUT.write_text("\n".join(LINES), encoding="utf-8")
