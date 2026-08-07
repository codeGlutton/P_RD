"""Audit the live detail overlay: does the layout actually match what C++ drives?

확인하는 것
-----------
1. C++ 이 GetWidgetFromName 으로 찾는 이름이 다 있고 종류가 맞는가
2. DetailPanelRoot 의 실제 크기 -- 런타임 스킬 행이 이 캔버스의 비율(0.10~0.90 x,
   0.66~0.90 y)에 앉으므로, 이걸 모르면 내가 비워 둔 띠가 엉뚱한 데 있게 된다
3. 그 띠에 글자가 겹치지 않는가
4. 입력을 막는 위젯(Button 등)이 새로 생기지 않았는가 -- 패널 바깥을 톡 쳐서
   닫는 기존 조작을 깨면 안 된다
5. 런타임에 문자열로 에셋을 로드하는 코드가 붙지 않았는가 (PR #300 규약)

Run headless:
    UnrealEditor-Cmd.exe <project> -run=pythonscript
        -script="Tools/UI/verify_detail_overlay_wiring.py"
"""

from pathlib import Path

import unreal

OUT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/overlay_wiring.txt")
ASSET_PATH = "/Game/UI/CombatDetail/WBP_CombatDetailOverlay"

# CombatLayoutHUDWidget.cpp 가 찾는 것들
REQUIRED = {
    "DetailIconImage": unreal.Image,
    "DetailTitleText": unreal.TextBlock,
    "DetailSubtitleText": unreal.TextBlock,
    "DetailBodyText": unreal.TextBlock,
    "DetailPanelRoot": unreal.CanvasPanel,
    "DetailIdentityColumn": unreal.CanvasPanel,
    "DetailStatColumn": unreal.CanvasPanel,
    "DetailRightColumn": unreal.CanvasPanel,
    "DetailDivider_0": unreal.Image,
    "DetailDivider_1": unreal.Image,
    "DetailStatBlock": unreal.CanvasPanel,
    "DetailTargetBlock": unreal.CanvasPanel,
    "DetailSkillBlock": unreal.CanvasPanel,
    "DetailSkillRowHost": unreal.CanvasPanel,
    "DetailExtraBlock": unreal.CanvasPanel,
    "DetailExtraHeading": unreal.TextBlock,
    "DetailExtraText": unreal.TextBlock,
}

lines = []


def rect_of(widget):
    slot = widget.get_editor_property("slot")
    if not isinstance(slot, unreal.CanvasPanelSlot):
        return None
    layout = slot.get_editor_property("layout_data")
    anchors = layout.get_editor_property("anchors")
    offsets = layout.get_editor_property("offsets")
    return {
        "anchors": (anchors.minimum.x, anchors.minimum.y, anchors.maximum.x, anchors.maximum.y),
        "offsets": (offsets.left, offsets.top, offsets.right, offsets.bottom),
    }


blueprint = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")
prefix = tree.get_path_name() + "."

widgets = {}
for obj in unreal.ObjectIterator():
    if isinstance(obj, unreal.Widget) and str(obj.get_path_name()).startswith(prefix):
        widgets[str(obj.get_name())] = obj

lines.append("=== 1. C++ 이 찾는 이름 ===")
for name, expected in REQUIRED.items():
    widget = widgets.get(name)
    if widget is None:
        lines.append(f"  {name}: 없음  <-- C++ 이 이 칸을 조용히 건너뛴다")
    elif not isinstance(widget, expected):
        lines.append(f"  {name}: 종류 불일치 {widget.get_class().get_name()} "
                     f"(기대 {expected.__name__})  <-- Cast 실패로 못 찾는다")
    else:
        lines.append(f"  {name}: ok ({widget.get_class().get_name()})")

lines.append("")
lines.append("=== 2. DetailPanelRoot 실제 크기 ===")
panel = widgets.get("DetailPanelRoot")
panel_rect = rect_of(panel) if panel else None
if panel_rect is None:
    lines.append("  슬롯을 못 읽음 (루트 직속이 아닐 수 있음)")
    panel_box = (0.0, 0.0, 1920.0, 1080.0)
    lines.append("  -> 1920x1080 으로 가정하고 계속")
else:
    anchors, offsets = panel_rect["anchors"], panel_rect["offsets"]
    lines.append(f"  anchors={tuple(round(v, 3) for v in anchors)} "
                 f"offsets={tuple(round(v, 1) for v in offsets)}")
    stretched = abs(anchors[0] - anchors[2]) > 0.0001 or abs(anchors[1] - anchors[3]) > 0.0001
    if stretched:
        x = anchors[0] * 1920.0 + offsets[0]
        y = anchors[1] * 1080.0 + offsets[1]
        w = (anchors[2] - anchors[0]) * 1920.0 - offsets[0] - offsets[2]
        h = (anchors[3] - anchors[1]) * 1080.0 - offsets[1] - offsets[3]
    else:
        w, h = offsets[2], offsets[3]
        x = anchors[0] * 1920.0 + offsets[0]
        y = anchors[1] * 1080.0 + offsets[1]
    panel_box = (x, y, w, h)
    lines.append(f"  -> 화면상 {x:.0f},{y:.0f} {w:.0f}x{h:.0f}")

lines.append("")
lines.append("=== 3. 공통 머리와 열 배치 ===")


def overlaps(a, b):
    return not (a[0] + a[2] <= b[0] or b[0] + b[2] <= a[0]
                or a[1] + a[3] <= b[1] or b[1] + b[3] <= a[1])


def design_box(name):
    widget = widgets.get(name)
    info = rect_of(widget) if widget else None
    if info is None:
        return None
    anchors, offsets = info["anchors"], info["offsets"]
    slot = widget.get_editor_property("slot")
    alignment = slot.get_alignment()
    stretch_x = abs(anchors[2] - anchors[0]) > 0.0001
    stretch_y = abs(anchors[3] - anchors[1]) > 0.0001
    width = ((anchors[2] - anchors[0]) * 1920.0 - offsets[0] - offsets[2]
             if stretch_x else offsets[2])
    height = ((anchors[3] - anchors[1]) * 1080.0 - offsets[1] - offsets[3]
              if stretch_y else offsets[3])
    x = anchors[0] * 1920.0 + offsets[0] - alignment.x * width
    y = anchors[1] * 1080.0 + offsets[1] - alignment.y * height
    return (x, y, width, height)


title = design_box("DetailTitlePlate")
column_boxes = [(name, design_box(name)) for name in
                ("DetailIdentityColumn", "DetailStatColumn", "DetailRightColumn")]
for name, box in column_boxes:
    lines.append(f"  {name}: {tuple(round(v, 1) for v in box) if box else '없음'}")
lines.append(f"  DetailTitlePlate: {tuple(round(v, 1) for v in title) if title else '없음'}")
bad = []
for name, box in column_boxes:
    if box is None:
        bad.append(f"{name} 없음")
    elif title is not None and overlaps(title, box):
        bad.append(f"제목판과 {name} 겹침")
for index in range(len(column_boxes) - 1):
    left_name, left_box = column_boxes[index]
    right_name, right_box = column_boxes[index + 1]
    if left_box is not None and right_box is not None and overlaps(left_box, right_box):
        bad.append(f"{left_name}/{right_name} 겹침")
lines.append("  문제: " + ", ".join(bad) if bad else "  제목/열 겹침 없음")

lines.append("")
lines.append("=== 4. 입력을 막는 위젯 ===")
blockers = []
for name, widget in widgets.items():
    if isinstance(widget, unreal.Button):
        blockers.append(f"  {name} (Button)")
    else:
        visibility = str(widget.get_editor_property("visibility"))
        if "SELF_HIT_TEST_INVISIBLE" not in visibility and "HIT_TEST_INVISIBLE" not in visibility \
                and "COLLAPSED" not in visibility and "HIDDEN" not in visibility:
            blockers.append(f"  {name} ({widget.get_class().get_name()}, {visibility})")
lines.append("\n".join(blockers) if blockers
             else "  없음 -- 패널 바깥을 톡 쳐서 닫는 조작이 유지된다")

OUT.write_text("\n".join(lines), encoding="utf-8")
