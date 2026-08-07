"""Does anything sit on the wood?

무엇을 재나
-----------
판마다 "받침 그림"과 "그 안에 놓인 것들"을 짝지어, 놓인 것이 받침의 **구멍**
안에 있는지 본다. 구멍은 사람이 목록 페이지에서 맞춘 자리이고, 9-slice 는
테두리를 원본 픽셀 크기로 그리므로 늘려 놓아도 두께가 안 변한다.

전에는 눈으로만 봤다. "글자가 나무를 밟았다" 를 사람이 스크린샷에서 찾아
알려 주는 동안, 숫자는 처음부터 파일 안에 있었다.

Run headless:
    UnrealEditor-Cmd.exe <project> -run=pythonscript
        -script="Tools/UI/verify_inside_plates.py"
"""

import sys
from pathlib import Path

import unreal

sys.path.insert(0, str(Path(__file__).resolve().parent))

import json  # noqa: E402

HERE = Path(__file__).resolve().parent
_USER = json.loads((HERE / "mockups/rects_user.json").read_text(encoding="utf-8"))
_MEASURED = {a["name"]: a for a in json.loads(
    (HERE / "mockups/assets.json").read_text(encoding="utf-8"))}


def part_rect(part):
    """그림 한 장의 (칸 비율, 원본 크기). 사람이 맞춘 것이 있으면 그것을 쓴다."""
    entry = _MEASURED.get(part)
    if entry is None:
        return None, None
    holes = _USER.get(part) or entry.get("holes") or []
    if not holes:
        return None, None
    hole = holes[0]
    return (hole.get("rect") or hole.get("inner") or hole.get("box")), entry["size"]


def opening(part, width, height, sliced):
    """그 그림을 (width,height) 로 놓았을 때의 구멍.

    9-slice 로 그리면 테두리가 원본 픽셀 그대로라 픽셀로 환산해야 하고,
    통짜로 그리면 전체가 같은 배율이라 비율이 맞다. **그리는 방식은 그림이
    아니라 쓰는 자리가 정한다** -- 같은 그림을 상세창은 9-slice 로, 용병탭은
    통짜로 쓴다.
    """
    ratio, source = part_rect(part)
    if ratio is None:
        return None
    if not sliced:
        return (ratio[0] * width, ratio[1] * height,
                (ratio[2] - ratio[0]) * width, (ratio[3] - ratio[1]) * height)
    near = [ratio[0] * source[0], ratio[1] * source[1]]
    far = [(1.0 - ratio[2]) * source[0], (1.0 - ratio[3]) * source[1]]
    for axis, span in ((0, width), (1, height)):
        total = near[axis] + far[axis]
        if total > span * 0.8 and total > 0.0:
            shrink = span * 0.8 / total
            near[axis] *= shrink
            far[axis] *= shrink
    return (near[0], near[1], width - near[0] - far[0], height - near[1] - far[1])

RESULT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/inside_plates.txt")

# 판 -> [(받침 위젯 이름, 받침 부품, 그 안에 있어야 하는 것들의 부모 캔버스)]
# 부모 캔버스를 적는 이유는 "받침과 형제인 것" 과 "받침 위에 놓인 것" 을
# 갈라야 하기 때문이다. 같은 캔버스에 있으면 좌표가 같은 기준이다.
CHECKS = [
    ("/Game/UI/CombatLayouts/WBP_MercenaryHire_Marchbound", [
        # 카드 · 파티 자리 · 스킬 칸. 그림은 통짜로 그리므로 비율이 맞다.
        ("HireCard_0_Art", "T_MB_HireRowNormal", "HireCard_0", False),
        ("HireCard_1_Art", "T_MB_HireRowNormal", "HireCard_1", False),
        ("PartySlotArt_0", "T_MB_HirePartyRowEmpty", "PartySlot_0", False),
        ("PartySlotArt_1", "T_MB_HirePartyRowEmpty", "PartySlot_1", False),
        ("HireDetailStatsArt", "T_MB_HireStatsStrip", "HireDetailStatsPanel", False),
        ("HireDetailNameArt", "T_MB_HireNamePlate", "HireDetailNamePanel", False),
    ]),
    ("/Game/UI/MonsterTab/WBP_MonsterTab_Marchbound", [
        ("MonsterRowNormal_0", "T_MT_RowNormal", "MonsterRow_0", False),
        ("MonsterRowNormal_1", "T_MT_RowNormal", "MonsterRow_1", False),
    ]),
    ("/Game/UI/CombatDetail/WBP_CombatDetailOverlay", [
        ("DetailIdentityPlate", "T_MB_HireRowNormal", "DetailIdentityColumn", True),
        ("DetailStatPlate", "T_MB_HireRowNormal", "DetailStatColumn", True),
        ("DetailRightPlate", "T_MB_HireRowNormal", "DetailRightColumn", True),
    ]),
]

LINES = []


def slot_rect(widget):
    """캔버스 슬롯의 자리. 앵커로 놓인 것은 1920x1080 기준으로 편다."""
    slot = widget.get_editor_property("slot")
    if not isinstance(slot, unreal.CanvasPanelSlot):
        return None
    anchors = slot.get_anchors()
    minimum, maximum = anchors.minimum, anchors.maximum
    offsets = slot.get_offsets()
    if minimum.x == maximum.x and minimum.y == maximum.y:
        position = slot.get_position()
        size = slot.get_size()
        return (position.x, position.y, size.x, size.y)
    # 늘려 놓은 것. 부모 크기를 모르므로 비율만 돌려주고 부르는 쪽이 곱한다.
    return ("stretch", minimum, maximum, offsets)


def main():
    for asset_path, entries in CHECKS:
        blueprint = unreal.EditorAssetLibrary.load_asset(asset_path)
        if blueprint is None:
            LINES.append(f"{asset_path} 없음")
            continue
        tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")
        LINES.append(f"[{asset_path.rsplit('/', 1)[-1]}]")

        for plate_name, part, column_name, sliced in entries:
            column = unreal.find_object(None, f"{tree.get_path_name()}.{column_name}")
            plate = unreal.find_object(None, f"{tree.get_path_name()}.{plate_name}")
            if column is None or plate is None:
                LINES.append(f"  {column_name}: 받침이나 열을 못 찾음")
                continue

            # 열의 실제 크기. 앵커 비율로 놓았으므로 1920x1080 에서 편다.
            slot = column.get_editor_property("slot")
            anchors = slot.get_anchors()
            if anchors.minimum.x == anchors.maximum.x:
                size = slot.get_size()
                width, height = size.x, size.y
            else:
                width = (anchors.maximum.x - anchors.minimum.x) * 1920.0
                height = (anchors.maximum.y - anchors.minimum.y) * 1080.0
            hole = opening(part, width, height, sliced)
            if hole is None:
                LINES.append(f"  {column_name}: {part} 의 칸을 모름")
                continue
            hx, hy, hw, hh = hole
            LINES.append(f"  {column_name}  {width:.0f}x{height:.0f}"
                         f"  구멍 x{hx:.0f} y{hy:.0f} {hw:.0f}x{hh:.0f}")

            # 열의 직계 자식은 받침과 콘텐츠 칸 둘뿐이다. 정작 봐야 할 것은
            # **콘텐츠 칸 안**이므로, 있으면 그 안을 0,0 기준으로 본다.
            content = unreal.find_object(
                None, f"{tree.get_path_name()}.{column_name}Content")
            if content is not None:
                inside, hx, hy = content, 0.0, 0.0
            else:
                inside = column

            bad, seen, folded = [], [], []

            def walk(panel, dx, dy):
                """자리를 가진 것만 본다.

                덩어리(DetailStatBlock 같은 것)는 칸을 통째로 덮는 빈 캔버스라
                자기 자리가 없다. 그 안의 글자·그림이 진짜 놓인 것이므로
                덮개는 그냥 지나쳐 안으로 들어간다.
                """
                for child in panel.get_all_children():
                    if child.get_name() == plate_name:
                        continue
                    # 접힌 것은 자리를 안 먹는다. 옛 배치가 남긴 이름들이라
                    # 좌표가 카드 밖에 있는 게 정상이다.
                    if child.get_visibility() == unreal.SlateVisibility.COLLAPSED:
                        folded.append(child.get_name())
                        continue
                    rect = slot_rect(child)
                    if rect is None:
                        continue
                    if rect[0] == "stretch":
                        if isinstance(child, unreal.PanelWidget):
                            walk(child, dx, dy)
                        continue
                    x, y, width_, height_ = rect[0] + dx, rect[1] + dy, rect[2], rect[3]
                    over = []
                    if x < hx - 0.5:
                        over.append(f"왼쪽 {hx - x:.0f}px")
                    if y < hy - 0.5:
                        over.append(f"위 {hy - y:.0f}px")
                    if x + width_ > hx + hw + 0.5:
                        over.append(f"오른쪽 {(x + width_) - (hx + hw):.0f}px")
                    if y + height_ > hy + hh + 0.5:
                        over.append(f"아래 {(y + height_) - (hy + hh):.0f}px")
                    # 칸을 통째로 덮는 것은 그러라고 있는 것이다 --
                    # 고름 표시 그림과 눌림을 받는 투명 단추.
                    if (abs(x - dx) < 0.5 and abs(y - dy) < 0.5
                            and width_ >= hw and height_ >= hh):
                        continue
                    seen.append(child.get_name())
                    if over:
                        bad.append(f"    나무를 밟음: {child.get_name()}  " + " · ".join(over))
                    if isinstance(child, unreal.PanelWidget):
                        walk(child, x, y)

            walk(inside, 0.0, 0.0)
            LINES.extend(bad)
            if folded:
                LINES.append(f"    접혀 있어 안 잼: {len(folded)}개 "
                             f"({', '.join(folded[:4])}{' …' if len(folded) > 4 else ''})")
            LINES.append(f"    잰 것 {len(seen)}개 · "
                         f"{'나무를 밟은 것 없음' if not bad else f'{len(bad)}개 삐져나옴'}")

    RESULT.parent.mkdir(parents=True, exist_ok=True)
    RESULT.write_text("\n".join(LINES) + "\n", encoding="utf-8")
    for line in LINES:
        unreal.log(line)


main()
