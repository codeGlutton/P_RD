"""Re-skin the live combat detail overlay — three columns, no dead band.

왜 다시 짜는가 (0804 검수)
--------------------------
지난 배치는 화면 아래 45% 를 ``DetailSkillWell`` 이라는 어두운 띠가 가로로
가로챘다. 그 띠는 **유닛 상세의 스킬 칸 자리**인데, 스킬 상세·이동·아티팩트에는
스킬 칸이 없어서 늘 빈 채로 남아 있었다. 게다가 그 띠 때문에 세 열이 모두
위 절반으로 눌려, 왼쪽 열은 그림 하나만 놓인 빈 판이 됐다.

이번 배치의 규칙
----------------
1. **세 열을 세로로 꽉 쓴다.** 띠를 없애고, 열마다 바닥까지 내용을 채운다.
2. **제목판을 피한다.** 그림에 박힌 제목판은 x335~978 · y67~192 다. 왼쪽·가운데
   열 글자는 y205 아래에서 시작해야 제목과 겹치지 않는다(전에 "베기" 바로 밑에
   "수치" 가 붙어 보이던 원인). 오른쪽 열은 제목판 밖이라 y150 부터 쓴다.
3. **서로 못 만나는 것은 같은 자리에 겹쳐 둔다.** 사거리 칸(스킬 전용)과 스킬
   칸(유닛 전용)은 절대 같이 안 뜬다. 오른쪽 열 한 자리를 나눠 쓰고 C++ 이
   덩어리째 껐다 켠다.

        DetailTargetBlock   사거리·영향 칸 + 차단 규칙   (스킬 상세)
        DetailSkillBlock    스킬 칸                      (유닛 상세)
        DetailExtraBlock    효과 목록 · 조작 안내        (아티팩트 · 이동)

   덩어리는 화면 전체를 덮는 빈 캔버스이고 자식은 절대 좌표로 앉는다. 그래야
   보이기만 토글해도 좌표를 다시 계산할 일이 없다.

4. **있는 위젯은 이름 그대로 다시 쓴다.** 떼고 새로 만들면 블루프린트 변수에
   끊긴 GUID 가 남아 "Variable [X] was deleted but still has a GUID" ensure 가
   매번 뜬다. 이름이 이미 있으면 그 물건을 그대로 옮긴다.

Run headless:
    UnrealEditor-Cmd.exe <project> -run=pythonscript
        -script="Tools/UI/build_combat_detail_overlay.py"
"""

import sys
from pathlib import Path

import unreal

sys.path.insert(0, str(Path(__file__).resolve().parent))

from frame_registry import COMPOSED, SCREEN_COLUMNS, compose  # noqa: E402
from kit_brush import (  # noqa: E402
    apply_kit, apply_kit_button, inner_rect, kit_source_size,
    project_font, stretchable)
from ui_layout import fit_aspect  # noqa: E402

CHIP_FRAME = "T_KitA_StatChip_Ring"

# 열 바탕.
#
# KitA 의 ``T_KitA_Row_Plate`` 를 쓰고 있었다. 526x140 짜리 **한 줄** 그림이라
# 속이 비어 있고(전장이 그대로 비쳤다) 나무가 좌우 66px 이다. 열은 371px 이니
# 폭의 17% 가 나무였다. 용병 화면의 양피지 줄판은 속이 차 있고 테두리가 얇다.
COLUMN_PLATE = "T_MB_HireRowNormal"

# 스킬 칸. C++ 의 DetailSkillSlotCount 와 같아야 한다 -- 판에 여섯을 미리
# 만들어 두고 런타임은 스킬 수만큼만 보인다.
SKILL_SLOTS = 6
SKILL_GAP = 10.0

RESULT_PATH = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/overlay_build.txt")
ASSET_PATH = "/Game/UI/CombatDetail/WBP_CombatDetailOverlay"
LINES = []

CREAM = unreal.LinearColor(1.0, 0.94, 0.82, 1.0)
INK = unreal.LinearColor(0.10, 0.055, 0.022, 1.0)
MUTED_INK = unreal.LinearColor(0.20, 0.12, 0.06, 1.0)
GOLD = unreal.LinearColor(0.68, 0.38, 0.10, 1.0)
OUTLINE = unreal.LinearColor(0.02, 0.01, 0.0, 0.95)

# 제목은 공통 머리칸에 두고 세로 기둥과 콘텐츠는 그 아래에서 시작한다.
# 열 안 여백. 나무 두께는 받침 그림이 먹으므로(plated_column) 여기서는
# 글자가 구멍 선에 딱 붙지 않을 만큼만 준다. 24 로 두면 좁은 열에서 글자
# 폭이 모자라 줄이 갈라졌다.
# 모바일 배율.
#
# 값들은 데스크톱 모니터를 보며 정한 것이다. 폰에서는 화면이 손바닥만 하고
# 손가락으로 누르므로 글자와 누를 자리가 다 커야 한다. 1920x1080 판을 폰에
# 맞춰 줄이면 22px 글자가 실제로는 10px 도 안 된다.
MOBILE = 1.35
GUTTER = 14.0
CONTENT_TOP = 180.0
CONTENT_BOTTOM = 1015.0


def find(tree, name):
    return unreal.find_object(None, tree.get_path_name() + "." + name)


def place(canvas, child, pos, box, z):
    slot = canvas.add_child_to_canvas(child)
    slot.set_anchors(unreal.Anchors(unreal.Vector2D(0.0, 0.0), unreal.Vector2D(0.0, 0.0)))
    slot.set_alignment(unreal.Vector2D(0.0, 0.0))
    slot.set_editor_property("auto_size", False)
    slot.set_position(unreal.Vector2D(pos[0], pos[1]))
    slot.set_size(unreal.Vector2D(box[0], box[1]))
    slot.set_z_order(z)


def place_anchored(canvas, child, anchors, offsets, alignment, z):
    """앵커 기반 배치. 화면 폭이 달라도 열과 판이 같은 비율로 늘어난다."""
    slot = canvas.add_child_to_canvas(child)
    slot.set_anchors(unreal.Anchors(
        unreal.Vector2D(anchors[0], anchors[1]),
        unreal.Vector2D(anchors[2], anchors[3])))
    slot.set_alignment(unreal.Vector2D(alignment[0], alignment[1]))
    slot.set_editor_property("auto_size", False)
    slot.set_offsets(unreal.Margin(offsets[0], offsets[1], offsets[2], offsets[3]))
    slot.set_z_order(z)


def place_fill(canvas, child, z, margin=0.0):
    place_anchored(canvas, child, (0.0, 0.0, 1.0, 1.0),
                   (margin, margin, margin, margin), (0.0, 0.0), z)


def place_fraction(canvas, child, rect, z):
    x, y, width, height = rect
    place_anchored(canvas, child,
                   (x / 1920.0, y / 1080.0,
                    (x + width) / 1920.0, (y + height) / 1080.0),
                   (0.0, 0.0, 0.0, 0.0), (0.0, 0.0), z)


def place_center_fixed(canvas, child, center_x, top, width, height, z):
    place_anchored(canvas, child, (center_x, 0.0, center_x, 0.0),
                   (0.0, top, width, height), (0.5, 0.0), z)


def place_hstretch(canvas, child, top, height, z, left=20.0, right=20.0):
    place_anchored(canvas, child, (0.0, 0.0, 1.0, 0.0),
                   (left, top, right, height), (0.0, 0.0), z)


def place_vstretch(canvas, child, left, top, width, bottom, z):
    place_anchored(canvas, child, (0.0, 0.0, 0.0, 1.0),
                   (left, top, width, bottom), (0.0, 0.0), z)


def style(text, size, color, align="center", wrap=None):
    font = text.get_editor_property("font")
    # 프로젝트 글꼴로. 전투 HUD 는 F_HUD_Oswald 를 쓰는데 여기만 엔진 기본
    # Roboto 로 남아 있었다 -- 같은 값이 화면마다 다른 얼굴로 보였다.
    project_font(font, int(size * MOBILE))
    outline = font.get_editor_property("outline_settings")
    # 밝은 제목에는 어두운 외곽선을, 양피지 위의 진한 잉크에는 외곽선을 쓰지 않는다.
    dark_ink = (color.r + color.g + color.b) < 0.75
    outline.set_editor_property("outline_size", 0 if dark_ink else (2 if size >= 26 else 1))
    outline.set_editor_property("outline_color", OUTLINE)
    font.set_editor_property("outline_settings", outline)
    text.set_editor_property("font", font)
    text.set_color_and_opacity(unreal.SlateColor(color))
    text.set_editor_property("justification", {
        "left": unreal.TextJustify.LEFT, "right": unreal.TextJustify.RIGHT,
    }.get(align, unreal.TextJustify.CENTER))
    text.set_editor_property("shadow_offset", unreal.Vector2D(1.5, 1.5))
    text.set_editor_property("shadow_color_and_opacity", unreal.LinearColor(0.02, 0.01, 0.0, 0.8))
    text.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
    text.set_editor_property("auto_wrap_text", bool(wrap))
    if wrap:
        text.set_editor_property("wrap_text_at", wrap)


def reuse(tree, name, kind):
    """이름이 이미 있으면 그 물건을 돌려준다. 지우거나 이름을 바꾸지 않는다."""
    existing = find(tree, name)
    if existing is not None:
        if isinstance(existing, kind):
            existing.modify()
            return existing
        # 종류가 바뀌었다면 새로 못 만든다. 이름 충돌은 엔진을 죽인다.
        raise RuntimeError(f"{name} 이 {type(existing).__name__} 로 이미 있다")
    return unreal.new_object(kind, outer=tree, name=name)


def new_canvas(tree, canvas, name, z):
    child = reuse(tree, name, unreal.CanvasPanel)
    child.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
    place(canvas, child, (0.0, 0.0), (1920.0, 1080.0), z)
    return child


def new_canvas_fraction(tree, canvas, name, rect, z):
    child = reuse(tree, name, unreal.CanvasPanel)
    child.clear_children()
    child.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
    place_fraction(canvas, child, rect, z)
    return child


def new_canvas_fill(tree, canvas, name, z):
    child = reuse(tree, name, unreal.CanvasPanel)
    child.clear_children()
    child.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
    place_fill(canvas, child, z)
    return child


def new_border(tree, canvas, name, pos, box, z, color):
    border = reuse(tree, name, unreal.Border)
    border.set_brush_color(color)
    border.set_editor_property("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
    border.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
    place(canvas, border, pos, box, z)
    return border


def place_middle(canvas, child, rect, z):
    """칸 **가운데**에 놓는다. 좌우도 상하도.

    캔버스 슬롯에 크기를 주면 글자는 그 칸의 **왼쪽 위**에서 그려진다. 칸을
    키울수록 글자가 위로 떠 보였다. 슬롯을 자동 크기로 두고 기준점을 가운데로
    잡으면 글자 크기가 얼마든 정확히 가운데에 온다.
    """
    slot = canvas.add_child_to_canvas(child)
    slot.set_anchors(unreal.Anchors(unreal.Vector2D(0.0, 0.0), unreal.Vector2D(0.0, 0.0)))
    slot.set_alignment(unreal.Vector2D(0.5, 0.5))
    slot.set_editor_property("auto_size", True)
    slot.set_position(unreal.Vector2D(rect[0] + rect[2] * 0.5, rect[1] + rect[3] * 0.5))
    slot.set_z_order(z)


def new_text(tree, canvas, name, value, size, pos, box, z, color, align="left",
             wrap=None, middle=False):
    text = reuse(tree, name, unreal.TextBlock)
    text.set_text(unreal.Text(value))
    style(text, size, color, align, wrap)
    if middle:
        place_middle(canvas, text, (pos[0], pos[1], box[0], box[1]), z)
    else:
        place(canvas, text, pos, box, z)
    return text


def new_image(tree, canvas, name, path, pos, box, z):
    """부품 이름(T_...)이면 9-slice·테두리·**비율**까지 잡아 입힌다.

    부품 시트가 "늘리지 말라" 고 적어 둔 그림(체크 · 손잡이 · 칩 테두리)은
    칸에 그대로 맞추면 비율이 망가진다. 그런 것은 칸 안에 비율을 지켜 넣는다.
    """
    image = reuse(tree, name, unreal.Image)
    if path and path.startswith("T_"):
        apply_kit(image, path, target=box)
        if not stretchable(path):
            source = kit_source_size(path)
            if source is not None:
                pos_x, pos_y, box_w, box_h = fit_aspect(
                    (pos[0], pos[1], box[0], box[1]), source)
                pos, box = (pos_x, pos_y), (box_w, box_h)
    elif path:
        leaf = path.rsplit("/", 1)[-1]
        source = unreal.load_object(None, f"{path}.{leaf}")
        if source is not None:
            image.set_brush_from_texture(source, False)
    image.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
    place(canvas, image, pos, box, z)
    return image


def build_skill_cells(tree, host, width, side):
    """스킬 칸 여섯을 **판에** 만든다. C++ 은 이름으로 찾아 쓰기만 한다.

    전에는 C++ 이 ``ConstructWidget<UButton>`` 으로 만들었다. 그 버튼은 아무
    스타일도 안 받아 슬레이트 기본 회색 사각으로 그려졌고, 오른쪽 열에 큼직한
    회색 판이 하나 떠 있었다. 런타임에서 부품 그림을 불러다 쓸 수는 없으므로
    (PR#300 -- 런타임 LoadObject 금지) 판에서 미리 입혀 둔다.

    칸은 **정사각**이다. 부품 시트가 ``T_KitA_Cell_Normal`` 을 늘리지 말라고
    적어 두었으므로(늘리면 모서리 장식이 뭉개진다) 가로로 6등분해 납작하게
    만들지 않고, 한 변을 맞춰 가운데로 모은다.
    """
    span = side * SKILL_SLOTS + SKILL_GAP * (SKILL_SLOTS - 1)
    start = max(0.0, (width - span) / 2.0)
    for index in range(SKILL_SLOTS):
        button = reuse(tree, f"DetailSkillButton_{index}", unreal.Button)
        button.set_visibility(unreal.SlateVisibility.COLLAPSED)
        apply_kit_button(button, "T_KitA_Cell_Normal", "T_KitA_Cell_Selected",
                         "T_KitA_Cell_Disabled")
        place(host, button, (start + (side + SKILL_GAP) * index, 0.0), (side, side), 2)

        # 그림과 글자를 겹쳐 둔다. 아이콘이 있으면 그림, 없으면 이름을 보인다.
        content = reuse(tree, f"DetailSkillContent_{index}", unreal.Overlay)
        content.clear_children()
        button.set_content(content)

        icon = reuse(tree, f"DetailSkillIcon_{index}", unreal.Image)
        icon_slot = content.add_child_to_overlay(icon)
        icon_slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
        icon_slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_FILL)
        # 칸 그림의 테두리를 밟지 않도록 실측한 구멍만큼 들인다.
        hole_x, hole_y, hole_w, hole_h = inner_rect("T_KitA_Cell_Normal", 0.0, 0.0, side, side)
        icon_slot.set_padding(unreal.Margin(
            hole_x, hole_y, side - hole_x - hole_w, side - hole_y - hole_h))
        icon.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)

        label = reuse(tree, f"DetailSkillLabel_{index}", unreal.TextBlock)
        label_slot = content.add_child_to_overlay(label)
        label_slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_CENTER)
        label_slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_CENTER)
        style(label, 20, INK, "center", hole_w)
        label.set_visibility(unreal.SlateVisibility.COLLAPSED)


def plated_column(tree, panel, name, plate_name, rect, plate=COLUMN_PLATE):
    """열 하나를 만든다: 받침 그림 + **그 그림의 구멍 안**에 놓이는 콘텐츠 칸.

    @return (콘텐츠 캔버스, 안쪽 너비, 안쪽 높이)

    전에는 열 캔버스에 받침을 깔고 글자를 사방 24px 로 넣었다. 받침의 나무는
    좌우 70px 이라, 글자가 40px 넘게 나무를 밟고 옆으로 삐져나왔다. 나무
    두께는 짐작할 값이 아니라 그림에서 나오는 값이다 -- inner_rect 가 준다.
    """
    column = new_canvas_fraction(tree, panel, name, rect, 5)
    # 받침 이름은 예전 그대로 쓴다. 이름을 바꾸면 블루프린트에 옛 이름의 GUID 만
    # 남아 컴파일마다 "was added but did not get a GUID" ensure 가 뜬다.
    new_image_fill(tree, column, plate_name, plate, 1, target=(rect[2], rect[3]))
    inner_x, inner_y, inner_w, inner_h = inner_rect(plate, 0.0, 0.0, rect[2], rect[3])

    content = reuse(tree, f"{name}Content", unreal.CanvasPanel)
    content.clear_children()
    content.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
    place(column, content, (inner_x, inner_y), (inner_w, inner_h), 5)
    return content, inner_w, inner_h


def new_image_fill(tree, canvas, name, path, z, margin=0.0, target=None):
    image = reuse(tree, name, unreal.Image)
    if path and path.startswith("T_"):
        apply_kit(image, path, target=target)
    image.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
    place_fill(canvas, image, z, margin)
    return image


def build():
    blueprint = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
    if blueprint is None:
        raise RuntimeError(f"missing {ASSET_PATH}")
    blueprint.modify()
    tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")
    tree.modify()

    panel = find(tree, "DetailPanelRoot")
    if not isinstance(panel, unreal.CanvasPanel):
        raise RuntimeError("DetailPanelRoot canvas not found")
    panel.modify()

    reused = {}
    for name in ("DetailFrameImage", "DetailIconImage", "DetailTitleText",
                 "DetailSubtitleText", "DetailBodyText"):
        widget = find(tree, name)
        if widget is None:
            raise RuntimeError(f"expected widget missing: {name}")
        widget.modify()
        reused[name] = widget

    # 자식만 뗀다. 이름을 바꾸거나 지우면 변수 GUID 가 끊긴다.
    panel.clear_children()

    # DetailPanelRoot 자체를 화면 전체로 편다. 원래는 (392,84) 에 1136x912 짜리
    # 가운데 패널이라, 그 안에 1920x1080 좌표로 그리면 전체가 화면 밖으로 밀린다.
    panel_slot = panel.get_editor_property("slot")
    panel_slot.modify()
    panel_slot.set_anchors(unreal.Anchors(unreal.Vector2D(0.0, 0.0), unreal.Vector2D(1.0, 1.0)))
    panel_slot.set_alignment(unreal.Vector2D(0.0, 0.0))
    panel_slot.set_offsets(unreal.Margin(0.0, 0.0, 0.0, 0.0))

    # ── 공통 머리 + 화면비 대응 열 ────────────────────────────────────
    # 기둥을 제목 뒤까지 올리면 명패와 오른쪽 정보가 겹쳤다. 머리칸은 공통으로
    # 비우고 기둥/콘텐츠를 y=160/180 아래에서 시작한다.
    (left, mid, right), dividers, _vertical = compose(SCREEN_COLUMNS["SkillDetail"])
    left = (left[0], CONTENT_TOP, left[2], CONTENT_BOTTOM - CONTENT_TOP)
    mid = (mid[0], CONTENT_TOP, mid[2], CONTENT_BOTTOM - CONTENT_TOP)
    right = (right[0], CONTENT_TOP, right[2], CONTENT_BOTTOM - CONTENT_TOP)

    scrim = reuse(tree, "DetailScrimBg", unreal.Border)
    scrim.set_brush_color(unreal.LinearColor(0.015, 0.012, 0.02, 0.52))
    scrim.set_editor_property("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
    scrim.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
    place_fill(panel, scrim, 0)

    place_fill(panel, reused["DetailFrameImage"], 1)
    apply_kit(reused["DetailFrameImage"], COMPOSED["outer"], target=(1920.0, 1080.0))
    reused["DetailFrameImage"].set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)

    for index, x in enumerate(dividers):
        divider = reuse(tree, f"DetailDivider_{index}", unreal.Image)
        apply_kit(divider, COMPOSED["divider"], target=(58.0, 875.0))
        divider.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
        place_anchored(panel, divider,
                       (x / 1920.0, 160.0 / 1080.0, x / 1920.0, 1035.0 / 1080.0),
                       (-29.0, 0.0, 58.0, 0.0), (0.0, 0.0), 3)

    title_plate = reuse(tree, "DetailTitlePlate", unreal.Image)
    apply_kit(title_plate, "T_KitA_Title_Plate")
    title_plate.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
    place_center_fixed(panel, title_plate, 0.5, 34.0, 620.0, 116.0, 18)
    place_center_fixed(panel, reused["DetailTitleText"], 0.5, 54.0, 570.0, 70.0, 20)
    style(reused["DetailTitleText"], 48, CREAM, "center")

    # 각 열을 하나의 Canvas 로 묶는다. 런타임은 이 세 묶음과 기둥만 움직이므로
    # 아티팩트의 2열 전환에서 빈 가운데 판이 남지 않는다.
    identity, iw, ih = plated_column(
        tree, panel, "DetailIdentityColumn", "DetailIdentityPlate", left)
    stat_column, mw, mh = plated_column(
        tree, panel, "DetailStatColumn", "DetailStatPlate", mid)
    right_column, rw, rh = plated_column(
        tree, panel, "DetailRightColumn", "DetailRightPlate", right)

    # ── 왼쪽 열: 정체성 ───────────────────────────────────────────────
    icon = min(iw - GUTTER * 2, 280.0)
    icon_frame = reuse(tree, "DetailIconFrame", unreal.Image)
    apply_kit(icon_frame, "T_KitA_Portrait_Frame")
    icon_frame.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
    place_center_fixed(identity, icon_frame, 0.5, 20.0, icon, icon, 10)
    # 그림이 앉을 자리는 틀에서 잰 구멍이다. 8.5% 는 짐작이었고, 실제 구멍은
    # 19.8%~80.2% 라 그림이 사방 11% 씩 나무 위로 올라와 있었다.
    _hx, hole_top, hole_w, hole_h = inner_rect(
        "T_KitA_Portrait_Frame", 0.0, 20.0, icon, icon)
    place_center_fixed(identity, reused["DetailIconImage"], 0.5, hole_top,
                       hole_w, hole_h, 12)
    reused["DetailIconImage"].set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)

    sub_y = 20.0 + icon + 14.0
    place_hstretch(identity, reused["DetailSubtitleText"], sub_y, 76.0, 12, GUTTER, GUTTER)
    style(reused["DetailSubtitleText"], 23, MUTED_INK, "center", iw - GUTTER * 2)
    body_y = sub_y + 92.0
    new_text(tree, identity, "DetailBodyHeading", "설명", 27,
             (GUTTER, body_y), (iw - GUTTER * 2, 40.0), 12, GOLD, "left")
    place_anchored(identity, reused["DetailBodyText"], (0.0, 0.0, 1.0, 1.0),
                   (GUTTER, body_y + 48.0, GUTTER, GUTTER), (0.0, 0.0), 12)
    style(reused["DetailBodyText"], 22, INK, "left", iw - GUTTER * 2)

    # ── 가운데 열: 작고 정돈된 수치 칩 ───────────────────────────────
    stats = new_canvas_fill(tree, stat_column, "DetailStatBlock", 5)
    heading_h = 44.0
    new_text(tree, stats, "DetailStatHeading", "수치", 29,
             (GUTTER, 8.0), (mw - GUTTER * 2, heading_h), 2, GOLD, "center", middle=True)
    # 칩 다섯을 **세로 한 줄**로 쌓는다.
    #
    # 2+2+1 로 놓았더니 마지막 하나가 외따로 남았고, 두 줄에 맞추려고 열을
    # 넓게 잡느라 왼쪽 설명 열이 좁아졌다. 세로로 쌓으면 열이 좁아도 되고
    # 다섯이 한 묶음으로 읽힌다.
    chip_top = heading_h + 16.0
    chip_room = (mh - chip_top - GUTTER) / 5.0
    chip = min(chip_room - 10.0, mw - GUTTER * 2, 150.0)
    for slot in range(5):
        cx = (mw - chip) / 2.0
        cy = chip_top + chip_room * slot + (chip_room - chip) * 0.5
        new_image(tree, stats, f"DetailChip{slot}Frame", CHIP_FRAME,
                  (cx, cy), (chip, chip), 1)
        # 글자는 링 안쪽에만 넣는다. 0.18/0.43 은 짐작이었고, 실측한 구멍은
        # 가로세로 27.5%~72.4% 다. 라벨 위와 값 아래가 링을 밟고 있었고
        # 가로도 링보다 2.2배 넓어 "6~10" 이 테두리에 닿았다.
        ring_x, ring_y, ring_w, ring_h = inner_rect(CHIP_FRAME, cx, cy, chip, chip)
        new_text(tree, stats, f"DetailChip{slot}Label", "", int(ring_h * 0.28),
                 (ring_x, ring_y), (ring_w, ring_h * 0.38), 2, GOLD, "center",
                 middle=True)
        new_text(tree, stats, f"DetailChip{slot}Value", "-", int(ring_h * 0.44),
                 (ring_x, ring_y + ring_h * 0.38), (ring_w, ring_h * 0.62), 2, INK,
                 "center", middle=True)

    # ── 오른쪽 열: 사거리 / 유닛 스킬 / 효과 중 하나 ─────────────────
    target = new_canvas_fill(tree, right_column, "DetailTargetBlock", 5)
    skill = new_canvas_fill(tree, right_column, "DetailSkillBlock", 5)

    # 효과 목록(아티팩트 · 이동)은 **가운데와 오른쪽을 이어** 쓴다.
    #
    # 아티팩트에는 수치 칩이 없어서 가운데 열이 빈 양피지 판으로 남아 있었다.
    # 런타임이 그 판과 기둥 하나를 접고, 효과 목록이 두 열을 함께 쓴다.
    wide = (mid[0], mid[1], right[0] + right[2] - mid[0], mid[3])
    wide_column, ew, eh = plated_column(
        tree, panel, "DetailWideColumn", "DetailWidePlate", wide)
    extra = new_canvas_fill(tree, wide_column, "DetailExtraBlock", 5)

    half = (rw - GUTTER * 2 - 28.0) / 2.0
    left_x, right_x = GUTTER, GUTTER + half + 28.0
    new_text(tree, target, "DetailSelectHeading", "사거리", 27,
             (left_x, 26.0), (half, 42.0), 2, GOLD, "center")
    new_text(tree, target, "DetailHitHeading", "영향 범위", 27,
             (right_x, 26.0), (half, 42.0), 2, GOLD, "center")

    extent, cell_gap = 5, 6.0
    cell = (half - cell_gap * (extent - 1)) / extent
    span = cell * extent + cell_gap * (extent - 1)
    grid_y = 82.0
    for prefix, base_x in (("DetailSelectCell", left_x), ("DetailHitCell", right_x)):
        for row in range(extent):
            for col in range(extent):
                pos = (base_x + (cell + cell_gap) * col,
                       grid_y + (cell + cell_gap) * row)
                new_border(tree, target, f"{prefix}_R{row}C{col}Bg", pos, (cell, cell), 1,
                           unreal.LinearColor(0.10, 0.065, 0.03, 0.50))
                img = new_image(tree, target, f"{prefix}_R{row}C{col}", None,
                                pos, (cell, cell), 2)
                brush = img.get_editor_property("brush")
                brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
                img.set_editor_property("brush", brush)
                img.set_color_and_opacity(unreal.LinearColor(0.13, 0.09, 0.045, 0.58))

    caption_y = grid_y + span + 12.0
    new_text(tree, target, "DetailSelectCaptionText", "", 22,
             (left_x, caption_y), (half, 34.0), 2, INK, "center")
    new_text(tree, target, "DetailHitCaptionText", "", 22,
             (right_x, caption_y), (half, 34.0), 2, INK, "center")
    blocker_y = caption_y + 56.0
    new_text(tree, target, "DetailBlockerHeading", "차단 규칙", 27,
             (GUTTER, blocker_y), (rw - GUTTER * 2, 40.0), 2, GOLD, "left")
    value_x = 260.0
    for index, (name, label) in enumerate(
            (("Aim", "조준 차단"), ("Effect", "영향 차단"))):
        row_y = blocker_y + 52.0 + index * 58.0
        new_image(tree, target, f"Detail{name}BlockerPlate", "T_KitA_Row_Plate",
                  (GUTTER, row_y), (rw - GUTTER * 2, 48.0), 1)
        new_text(tree, target, f"Detail{name}BlockerLabel", label, 22,
                 (40.0, row_y + 7.0), (200.0, 34.0), 2, MUTED_INK, "left")
        new_text(tree, target, f"Detail{name}BlockerText", "-", 23,
                 (value_x, row_y + 7.0), (rw - value_x - 42.0, 34.0), 2, INK, "left")

    new_text(tree, skill, "DetailSkillHeading", "스킬", 29,
             (GUTTER, 16.0), (rw - GUTTER * 2, 42.0), 2, GOLD, "left")
    host = reuse(tree, "DetailSkillRowHost", unreal.CanvasPanel)
    host.clear_children()
    # 호스트 판은 입력을 먹지 않고, 스킬 칸만 입력을 받는다.
    host.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
    skill_side = min(150.0, (rw - GUTTER * 2 - SKILL_GAP * (SKILL_SLOTS - 1)) / SKILL_SLOTS)
    place_hstretch(skill, host, 76.0, skill_side, 2, GUTTER, GUTTER)
    build_skill_cells(tree, host, rw - GUTTER * 2, skill_side)

    extra_heading = reuse(tree, "DetailExtraHeading", unreal.TextBlock)
    extra_heading.set_text(unreal.Text(""))
    style(extra_heading, 30, GOLD, "center")
    place_middle(extra, extra_heading, (GUTTER, 12.0, ew - GUTTER * 2, 48.0), 2)
    extra_text = reuse(tree, "DetailExtraText", unreal.TextBlock)
    extra_text.set_text(unreal.Text(""))
    style(extra_text, 26, INK, "center", ew - GUTTER * 2)
    place_anchored(extra, extra_text, (0.0, 0.0, 1.0, 1.0),
                   (GUTTER, 76.0, GUTTER, 20.0), (0.0, 0.0), 2)

    # 넓은 열은 평소 접혀 있다. 칩이 없는 화면에서만 런타임이 편다.
    wide_column.set_visibility(unreal.SlateVisibility.COLLAPSED)

    # 옛 배치가 남긴 것들. 이번 배치에는 자리가 없지만, 떼인 채로 두면 변수
    # GUID 만 남아 컴파일마다 ensure 가 뜬다. 접어서 붙여 둔다.
    for stale in ("DetailSkillWell", "DetailAimBlockerRow", "DetailEffectBlockerRow",
                  "DetailTint", "DetailLeftWell", "DetailMidWell", "DetailRightWell",
                  # 받침 이름을 잠깐 바꿨다가 되돌린 흔적. 트리에서 떼도
                  # WidgetTree.AllWidgets 에 남아 컴파일러가 보므로, 접어서
                  # 붙여 둬야 GUID 가 생기고 ensure 가 멎는다.
                  "DetailIdentityColumnPlate", "DetailStatColumnPlate",
                  "DetailRightColumnPlate",
                  # 열 구멍을 색으로 메우던 판. 이제 바탕 그림이 속이 차 있어
                  # 필요 없다.
                  "DetailIdentityColumnFill", "DetailStatColumnFill",
                  "DetailRightColumnFill"):
        widget = find(tree, stale)
        if widget is None:
            continue
        widget.modify()
        place(panel, widget, (0.0, 0.0), (1.0, 1.0), -100)
        widget.set_visibility(unreal.SlateVisibility.COLLAPSED)

    # Python으로 만든 위젯은 WidgetVariableNameToGuidMap에 자동 등록되지 않는다.
    # 컴파일 전에 실제 트리와 GUID 맵을 동기화해 ensure를 막는다.
    world = unreal.EditorLevelLibrary.get_editor_world() if hasattr(
        unreal, "EditorLevelLibrary") else None
    unreal.SystemLibrary.execute_console_command(
        world, f"RD.Editor.CleanWidgetVariables {ASSET_PATH}")
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    saved = unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)

    LINES.append(f"saved={saved}")
    for name in ("DetailIconImage", "DetailTitleText", "DetailSubtitleText",
                 "DetailBodyText", "DetailPanelRoot", "DetailTargetBlock",
                 "DetailSkillBlock", "DetailSkillRowHost", "DetailExtraBlock",
                 "DetailExtraHeading", "DetailExtraText", "DetailIdentityColumn",
                 "DetailStatColumn", "DetailRightColumn", "DetailDivider_0",
                 "DetailDivider_1"):
        LINES.append(f"  {name}: {'ok' if find(tree, name) is not None else 'MISSING'}")
    if not saved:
        LINES.append("SAVE BLOCKED -- 에디터에서 이 WBP 탭을 닫고 다시 실행")


try:
    build()
except Exception as error:  # noqa: BLE001
    import traceback
    LINES.append("FAILED: %s" % error)
    LINES.append(traceback.format_exc())
finally:
    RESULT_PATH.write_text("\n".join(LINES), encoding="utf-8")
