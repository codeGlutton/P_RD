"""Re-lay the live settings panel — two columns, one button bar, no dead rows.

왜 다시 짜는가 (0804 검수)
--------------------------
1. **뒤로만 혼자 반대편에 있었다.** Back 은 왼쪽 아래, Reset/저장/포기는 오른쪽
   아래에 흩어져 있어 어느 것이 한 묶음인지 안 보였다. 넷을 한 줄에 모은다.
2. **컨트롤 없는 줄이 자리를 먹고 있었다.** 밝기·빠른 모드·연출 생략·자동 턴
   종료·크레딧·라이선스는 이름표만 있고 값을 바꿀 물건이 없다. C++ 도 이름표에
   글자만 넣고 읽는 곳이 없다. 접어 둔다 -- 지우면 변수 GUID 가 끊긴다.
3. **아트톤을 새로 맞출 예정이라 뼈대만 정확히 잡는다.** 그림은 Settings 폴더의
   기존 판/단추/슬라이더를 쓰고, 자리는 틀 그림의 창(setScrim.window) 안으로만
   잡는다. 새 그림이 오면 텍스처만 갈아 끼우면 된다.

살아 있는 줄만 놓는다
---------------------
    소리   전체 · 배경음 · 효과음 · UI      (슬라이더 4)
    화면   화면 흔들림 · 진동 · 연출         (체크 3)
           품질 · 프레임                     (단추 무리 2)
    조작   언어                              (단추 무리 1)

이름은 하나도 바꾸지 않는다. ``USettingsPanelWidget`` 이 54개를
BindWidgetOptional 로 잡는데, 이름이 틀리면 에러 한 줄 없이 기능만 빠진다.

Run headless:
    UnrealEditor-Cmd.exe <project> -run=pythonscript
        -script="Tools/UI/build_settings_panel.py"
"""

import sys
from pathlib import Path

import unreal

sys.path.insert(0, str(Path(__file__).resolve().parent))

from frame_registry import FRAMES  # noqa: E402

# 판 바탕. 속이 차 있고 테두리가 얇은 양피지 줄판을 늘려 쓴다.
PANEL = "T_MB_HireRowNormal"
# 모바일 배율. 손가락으로 누르는 화면이라 글자와 누를 자리를 키운다.
MOBILE = 1.35
# 이번 배치에서 안 쓰게 된 한 줄 받침들. 떼면 GUID 만 남으므로 접어 둔다.
ROW_PLATES = ["Set_scrim"]
from kit_brush import (  # noqa: E402
    apply_kit, inner_rect, kit_source_size, kit_texture,
    project_font, stretchable)
from ui_layout import fit_aspect  # noqa: E402
from kit_brush import _set_size as set_brush_size  # noqa: E402

RESULT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/settings_build.txt")
ASSET = "/Game/UI/WBP_SettingsPanel"
SET = "/Game/SVN/OutSideAsset/AICreation/UI/Settings"
ORPHANS = Path(__file__).resolve().parent / "settings_orphans.txt"
LINES = []

# 바탕이 양피지가 되면서 글자색을 뒤집었다. 크림색 글씨는 양피지 위에서
# 안 읽힌다. 어두운 스크림 위에 놓이는 것(제목 명패 글씨)만 크림색이다.
CREAM = unreal.LinearColor(0.10, 0.055, 0.022, 1.0)
PARCHMENT = unreal.LinearColor(0.24, 0.15, 0.07, 1.0)
GOLD = unreal.LinearColor(0.55, 0.29, 0.07, 1.0)
OUTLINE = unreal.LinearColor(0.02, 0.01, 0.0, 0.95)

# 값을 바꿀 물건이 없는 이름표들. 만들지도 지우지도 않고 접기만 한다.
DEAD_ROWS = [
    "BrightnessRow_Label", "FastModeRow_Label", "SkipAnimationRow_Label",
    "AutoEndTurnRow_Label", "CreditsRow_Label", "LicenseRow_Label",
    "CreditsOpenButtonText", "LicenseOpenButtonText", "InfoSectionHeader",
]


def find(tree, name):
    return unreal.find_object(None, tree.get_path_name() + "." + name)


def texture(leaf):
    return unreal.load_object(None, f"{SET}/{leaf}.{leaf}")


def place(canvas, child, pos, box, z):
    slot = canvas.add_child_to_canvas(child)
    slot.set_anchors(unreal.Anchors(unreal.Vector2D(0.0, 0.0), unreal.Vector2D(0.0, 0.0)))
    slot.set_alignment(unreal.Vector2D(0.0, 0.0))
    slot.set_editor_property("auto_size", False)
    slot.set_position(unreal.Vector2D(pos[0], pos[1]))
    slot.set_size(unreal.Vector2D(box[0], box[1]))
    slot.set_z_order(z)


def reuse(tree, name, kind):
    """이름이 이미 있으면 그것을 쓴다. 이름을 바꾸거나 지우면 GUID 가 끊긴다."""
    existing = find(tree, name)
    if existing is not None:
        if not isinstance(existing, kind):
            raise RuntimeError(f"{name} 이 {type(existing).__name__} 로 이미 있다")
        existing.modify()
        return existing
    return unreal.new_object(kind, outer=tree, name=name)


def style(text, size, color, align="left"):
    font = text.get_editor_property("font")
    project_font(font, int(size * MOBILE))
    outline = font.get_editor_property("outline_settings")
    outline.set_editor_property("outline_size", 2 if size >= 28 else 1)
    outline.set_editor_property("outline_color", OUTLINE)
    font.set_editor_property("outline_settings", outline)
    text.set_editor_property("font", font)
    text.set_color_and_opacity(unreal.SlateColor(color))
    text.set_editor_property("justification", {
        "left": unreal.TextJustify.LEFT, "right": unreal.TextJustify.RIGHT,
    }.get(align, unreal.TextJustify.CENTER))
    text.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)


def place_middle(canvas, child, pos, box, z):
    """칸 가운데에 놓는다. 캔버스 슬롯에 크기를 주면 글자는 왼쪽 위에 붙는다."""
    slot = canvas.add_child_to_canvas(child)
    slot.set_anchors(unreal.Anchors(unreal.Vector2D(0.0, 0.0), unreal.Vector2D(0.0, 0.0)))
    slot.set_alignment(unreal.Vector2D(0.5, 0.5))
    slot.set_editor_property("auto_size", True)
    slot.set_position(unreal.Vector2D(pos[0] + box[0] * 0.5, pos[1] + box[1] * 0.5))
    slot.set_z_order(z)


def add_text(tree, canvas, name, value, size, pos, box, z, color, align="left",
             middle=True):
    """글자 하나. 기본으로 **칸 가운데**에 놓는다.

    align 은 여러 줄일 때 줄끼리의 정렬이고, 칸 안 어디에 앉느냐는 슬롯이
    정한다. 둘을 헷갈려서 왼쪽 정렬 글자가 칸 위쪽에 붙어 있었다.
    """
    text = reuse(tree, name, unreal.TextBlock)
    if value is not None:
        text.set_text(unreal.Text(value))
    style(text, size, color, align)
    if middle:
        place_middle(canvas, text, pos, box, z)
    else:
        place(canvas, text, pos, box, z)
    return text


def add_image(tree, canvas, name, leaf, pos, box, z):
    """leaf 가 KitA 부품 이름이면 9-slice 까지 잡아 입힌다.

    아니면 옛 Settings 폴더 그림으로 떨어진다 -- 아직 KitA 에 없는 조각(패널 틀
    같은 것)이 있어서, 한 번에 다 갈아 끼우려다 화면을 비우지 않으려는 것이다.
    """
    image = reuse(tree, name, unreal.Image)
    if leaf and apply_kit(image, leaf, target=box):
        # 늘리지 말라고 적힌 그림은 칸에 그대로 맞추면 비율이 망가진다.
        # 슬라이더 홈(596x88)을 235x16 에 욱여넣어 실 한 가닥이 돼 있었다.
        if not stretchable(leaf):
            source = kit_source_size(leaf)
            if source is not None:
                fitted = fit_aspect((pos[0], pos[1], box[0], box[1]), source)
                pos, box = (fitted[0], fitted[1]), (fitted[2], fitted[3])
    else:
        source = texture(leaf) if leaf else None
        if source is not None:
            image.set_brush_from_texture(source, False)
    image.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
    place(canvas, image, pos, box, z)
    return image


def style_check(check):
    """체크박스는 Image 가 아니라 위젯 스타일에 그림을 넣는다.

    켬/끔 두 장뿐이라 눌림·호버도 같은 그림을 쓴다. 상태마다 다른 그림이 오면
    그때 나눈다 -- 지금 없는 상태를 미리 만들어 두면 어느 게 진짜인지 흐려진다.
    """
    off = kit_texture("T_KitA_Checkbox_Off")
    on = kit_texture("T_KitA_Checkbox_On")
    if off is None or on is None:
        return
    style = check.get_editor_property("widget_style")

    def brush(texture):
        made = unreal.SlateBrush()
        made.set_editor_property("resource_object", texture)
        made.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
        set_brush_size(made, float(texture.blueprint_get_size_x()),
                       float(texture.blueprint_get_size_y()))
        return made

    for state in ("unchecked_image", "unchecked_hovered_image", "unchecked_pressed_image"):
        style.set_editor_property(state, brush(off))
    for state in ("checked_image", "checked_hovered_image", "checked_pressed_image"):
        style.set_editor_property(state, brush(on))
    check.set_editor_property("widget_style", style)


def add_button(tree, canvas, name, pos, box, z, leaf="T_KitA_Button_Wide_Normal"):
    """그림은 뒤에 따로 깔고 단추 자체는 투명하게 둔다.

    단추 스타일에 그림을 박으면 눌림/비활성 상태마다 다시 넣어야 하고, 새 아트가
    오면 파이썬을 또 고쳐야 한다. 판은 Image 로 분리해 둔다.
    """
    if leaf:
        add_image(tree, canvas, name + "Plate", leaf, pos, box, z)
    button = reuse(tree, name, unreal.Button)
    button_style = button.get_editor_property("widget_style")
    empty = unreal.SlateBrush()
    empty.set_editor_property("draw_as", unreal.SlateBrushDrawType.NO_DRAW_TYPE)
    for state in ("normal", "hovered", "pressed", "disabled"):
        button_style.set_editor_property(state, empty)
    button.set_editor_property("widget_style", button_style)
    place(canvas, button, pos, box, z + 1)
    return button


def build():
    blueprint = unreal.EditorAssetLibrary.load_asset(ASSET)
    if blueprint is None:
        raise RuntimeError(f"missing {ASSET}")
    blueprint.modify()
    tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")
    tree.modify()

    root = find(tree, "SettingsPanelRoot")
    if root is None:
        # 뿌리 캔버스 이름을 모르면 화면 전체를 덮는 캔버스를 찾는다.
        for name in ("RootCanvas", "CanvasPanel_0", "Set_root"):
            root = find(tree, name)
            if isinstance(root, unreal.CanvasPanel):
                break
    if not isinstance(root, unreal.CanvasPanel):
        raise RuntimeError("설정 패널의 뿌리 캔버스를 못 찾음")
    root.modify()
    root.clear_children()
    root_slot = root.get_editor_property("slot")
    if isinstance(root_slot, unreal.CanvasPanelSlot):
        root_slot.modify()
        root_slot.set_anchors(unreal.Anchors(unreal.Vector2D(0.0, 0.0), unreal.Vector2D(1.0, 1.0)))
        root_slot.set_offsets(unreal.Margin(0.0, 0.0, 0.0, 0.0))

    frame = FRAMES["setScrim"]
    wx, wy, ww, wh = frame["window"]
    # 판의 나무 두께는 짐작하지 않는다. 28 로 두었더니 제목과 줄이 나무 위로
    # 40px 넘게 올라와 있었다 -- 이 그림의 나무는 좌우 66px 이다.
    # 9-slice 는 테두리를 원본 픽셀 크기로 그리므로 늘려도 그 두께 그대로다.
    body_x, body_y, body_w, body_h = inner_rect(PANEL, wx, wy, ww, wh)
    pad = 16.0
    cx, cy, cw = body_x + pad, body_y + pad, body_w - pad * 2

    # 바탕. 옛 그림(T_set_scrim)은 남색·금세공이라 A안과 결이 다르다. 전체 화면
    # 스크림은 아직 KitA 에 없으므로, 그림 없이 어두운 나무색으로만 깔아 둔다.
    # 새 스크림이 오면 여기 이름만 넣으면 된다.
    # 그림 없는 Image 는 **아무것도 안 그린다**. 색만 넣어 두었더니 설정창
    # 뒤로 전장이 그대로 보였다. 색을 칠하려면 Border 여야 한다.
    # 이름을 새로 쓴다. 있는 Set_scrim 은 Image 라 종류를 못 바꾸고,
    # 바꾸려 들면 이름 충돌로 엔진이 죽는다. 옛것은 아래에서 접는다.
    scrim = reuse(tree, "Set_scrimBg", unreal.Border)
    scrim.set_brush_color(unreal.LinearColor(0.055, 0.038, 0.026, 0.88))
    scrim.set_editor_property("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
    scrim.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
    place(root, scrim, (0.0, 0.0), (1920.0, 1080.0), 0)
    # 내용이 앉을 판. 한 줄 받침을 9-slice 로 늘려 큰 틀로 쓴다 -- 큰 판 그림이
    # 아직 없어서다. 늘려도 모서리가 안 뭉개지는 게 9-slice 를 잰 이유다.
    # 속이 빈 나무 받침을 늘려 쓰고 있었다. 뒤가 비치고 나무만 두꺼웠다.
    # 속이 찬 양피지 판으로 바꾸고, 테두리는 이 크기에 맞게 얇게 그린다.
    add_image(tree, root, "Set_panel_body", PANEL, (wx, wy), (ww, wh), 1)


    add_text(tree, root, "SettingsTitleText", "설정", 52,
             (cx, cy + 6.0), (cw, 66.0), 10, CREAM, "center")
    add_text(tree, root, "StatusText", "", 24,
             (cx, cy + 78.0), (cw, 36.0), 10, PARCHMENT, "center")

    top = cy + 124.0
    col_w = (cw - 40.0) / 2.0
    columns = (cx, cx + col_w + 40.0)
    # 한 줄 받침 원본은 140px 짜리고 위아래 나무가 33px 씩이다. 44 로 두면
    # 나무끼리 겹쳐 뭉개진 덩어리가 된다. 속이 남을 만큼은 줘야 한다.
    row_h, pitch, head_h = 66.0, 74.0, 40.0

    def header(name, label, column, y):
        add_text(tree, root, name, label, 30,
                 (columns[column], y), (col_w, head_h), 10, GOLD)
        return y + head_h + 6.0

    def row_plate(key, column, y):
        """한 줄 받침을 깔고, **그 안쪽** 글자 자리와 조작 자리를 돌려준다.

        20px 로 짐작했었다. 이 그림은 한 줄용이라 좌우 나무가 66px 이고,
        글자가 나무 절반을 밟고 있었다.
        """
        # 줄마다 테를 두르지 않는다. 판이 이미 양피지라 줄 테두리까지 있으면
        # 한 화면에 테가 열두 겹이 된다. 66px 짜리 줄에 140px 짜리 받침을
        # 욱여넣던 자리이기도 하다 -- 위아래 나무가 서로 겹쳐 뭉개졌다.
        ROW_PLATES.append(f"Set_row_{key}_plate")
        return columns[column] + 10.0, columns[column] + col_w * 0.42

    def slider_row(key, label_name, label, column, y, slider_name):
        left, control = row_plate(key, column, y)
        add_text(tree, root, label_name, label, 25,
                 (left, y), (col_w * 0.40, row_h), 10, CREAM)
        # 홈 그림은 596x88 이다. 높이를 16 으로 주면 세로로 5배 눌려 실
        # 한 가닥처럼 보였다. 폭에 맞는 높이를 그림에서 얻어 쓴다.
        track_w = col_w * 0.50
        track_h = track_w * 88.0 / 596.0
        track_y = y + (row_h - track_h) * 0.5
        add_image(tree, root, f"Set_slider_track_{key}", "T_KitA_Slider_Track",
                  (control, track_y), (track_w, track_h), 8)
        slider = reuse(tree, slider_name, unreal.Slider)
        place(root, slider, (control, track_y), (track_w, track_h), 10)
        return y + pitch

    def check_row(key, label_name, label, column, y, box_name):
        left, control = row_plate(key, column, y)
        add_text(tree, root, label_name, label, 25,
                 (left, y), (col_w * 0.40, row_h), 10, CREAM)
        check = reuse(tree, box_name, unreal.CheckBox)
        style_check(check)
        # 켬 그림은 122x116 이다. 정사각에 넣으면 가로로 눌린다.
        check_h = row_h * 0.72
        check_w = check_h * 122.0 / 116.0
        place(root, check, (columns[column] + col_w - check_w - 24.0,
                            y + (row_h - check_h) * 0.5), (check_w, check_h), 10)
        return y + pitch

    def button_row(key, label_name, label, column, y, entries):
        left, control = row_plate(key, column, y)
        add_text(tree, root, label_name, label, 25,
                 (left, y), (col_w * 0.36, row_h), 10, CREAM)
        span = col_w - (control - columns[column]) - 20.0
        width = (span - 8.0 * (len(entries) - 1)) / len(entries)
        for index, (button_name, text_name, caption) in enumerate(entries):
            bx = control + (width + 8.0) * index
            # 칸 그림(186x176)은 늘리지 말라고 적혀 있다. 가로로 늘려 쓰면
            # 모서리가 뭉개지므로, 늘려도 되는 작은 단추 그림을 쓴다.
            button_h = row_h * 0.78
            button_y = y + (row_h - button_h) * 0.5
            add_button(tree, root, button_name, (bx, button_y), (width, button_h), 8,
                       "T_KitA_Button_Small_Normal")
            hole = inner_rect("T_KitA_Button_Small_Normal", bx, button_y,
                              width, button_h)
            add_text(tree, root, text_name, caption, 22,
                     (hole[0], hole[1]), (hole[2], hole[3]), 12, CREAM, "center")
        return y + pitch

    # ── 왼쪽: 소리 · 조작 ────────────────────────────────────────────────
    y = header("AudioSectionHeader", "소리", 0, top)
    y = slider_row("master", "MasterVolumeRow_Label", "전체", 0, y, "MasterVolumeSlider")
    y = slider_row("bgm", "BGMVolumeRow_Label", "배경음", 0, y, "BgmVolumeSlider")
    y = slider_row("sfx", "SFXVolumeRow_Label", "효과음", 0, y, "SfxVolumeSlider")
    y = slider_row("ui", "UIVolumeRow_Label", "조작음", 0, y, "UiVolumeSlider")

    y = header("GameplaySectionHeader", "조작", 0, y + 22.0)
    y = button_row("language", "LanguageRow_Label", "언어", 0, y,
                   [("LanguageKoreanButton", "LanguageKoreanButtonText", "한국어"),
                    ("LanguageEnglishButton", "LanguageEnglishButtonText", "English")])

    # ── 오른쪽: 화면 ─────────────────────────────────────────────────────
    y = header("DisplaySectionHeader", "화면", 1, top)
    y = check_row("shake", "ScreenShakeRow_Label", "화면 흔들림", 1, y, "ScreenShakeCheckBox")
    y = check_row("vibration", "VibrationRow_Label", "진동", 1, y, "VibrationCheckBox")
    y = check_row("effects", "EffectsRow_Label", "연출", 1, y, "EffectsCheckBox")
    y = button_row("quality", "QualityRow_Label", "품질", 1, y,
                   [("LowQualityButton", "LowQualityButtonText", "낮음"),
                    ("MediumQualityButton", "MediumQualityButtonText", "보통"),
                    ("HighQualityButton", "HighQualityButtonText", "높음")])
    y = button_row("fps", "FpsRow_Label", "프레임", 1, y,
                   [("FpsThirtyButton", "FpsThirtyButtonText", "30"),
                    ("FpsSixtyButton", "FpsSixtyButtonText", "60")])

    # ── 아래 단추 한 줄. 넷이 한 묶음이라는 걸 자리로 보여 준다. ────────
    #
    # 전에는 뒤로만 왼쪽 아래, 나머지 셋은 오른쪽 아래로 갈라져 있었다.
    # 런 액션 둘(저장·포기)은 타이틀에서 통째로 접혀야 하므로 RunActionsPanel
    # 안에 넣는다 -- C++ 이 그 하나만 보고 접는다. 접히면 가로 묶음이라
    # 자리를 안 먹고, 뒤로·되돌리기가 자연스럽게 왼쪽에 남는다.
    bar_y = wy + wh - pad - 74.0
    bar_h = 70.0
    plain_w = cw * 0.21
    for index, (button_name, text_name, caption) in enumerate(
            (("BackButton", "BackButtonText", "뒤로"),
             ("ResetButton", "ResetButtonText", "되돌리기"))):
        bx = cx + (plain_w + 12.0) * index
        add_button(tree, root, button_name, (bx, bar_y), (plain_w, bar_h), 10,
                   "T_KitA_Button_Small_Normal")
        hole = inner_rect("T_KitA_Button_Small_Normal", bx, bar_y, plain_w, bar_h)
        add_text(tree, root, text_name, caption, 27,
                 (hole[0], hole[1]), (hole[2], hole[3]), 14, CREAM, "center")

    run_actions = reuse(tree, "RunActionsPanel", unreal.HorizontalBox)
    run_actions.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
    run_x = cx + (plain_w + 12.0) * 2 + 24.0
    place(root, run_actions, (run_x, bar_y), (cx + cw - run_x, bar_h), 10)
    for button_name, text_name, caption in (
            ("SaveAndExitButton", "SaveAndExitButtonText", "저장하고 나가기"),
            ("AbandonRunButton", "AbandonRunButtonText", "탐험 포기")):
        cell = reuse(tree, f"Set_run_{button_name}", unreal.Overlay)
        cell.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
        cell_slot = run_actions.add_child_to_horizontal_box(cell)
        cell_slot.set_size(unreal.SlateChildSize(1.0, unreal.SlateSizeRule.FILL))
        cell_slot.set_padding(unreal.Margin(6.0, 0.0, 6.0, 0.0))
        plate = reuse(tree, button_name + "Plate", unreal.Image)
        # 가로 묶음이 폭을 정하므로 정확한 크기는 런타임에 난다. 높이는
        # 아래 단추 줄 높이(bar_h)로 고정이라 그걸로 두께를 정한다.
        apply_kit(plate, "T_KitA_Button_Wide_Normal", target=(bar_h * 3.0, bar_h))
        plate.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
        cell.add_child(plate)
        button = reuse(tree, button_name, unreal.Button)
        button_style = button.get_editor_property("widget_style")
        empty = unreal.SlateBrush()
        empty.set_editor_property("draw_as", unreal.SlateBrushDrawType.NO_DRAW_TYPE)
        for state in ("normal", "hovered", "pressed", "disabled"):
            button_style.set_editor_property(state, empty)
        button.set_editor_property("widget_style", button_style)
        cell.add_child(button)
        label = reuse(tree, text_name, unreal.TextBlock)
        label.set_text(unreal.Text(caption))
        style(label, 27, CREAM, "center")
        label_slot = cell.add_child(label)
        label_slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_CENTER)
        label_slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_CENTER)
        # 양끝 쇠장식 폭만큼 비워 둔다. 글자가 길면 장식 위로 올라갔다.
        cap = inner_rect("T_KitA_Button_Wide_Normal", 0.0, 0.0, bar_h * 3.0, bar_h)
        label_slot.set_padding(unreal.Margin(cap[0], 0.0, cap[0], 0.0))

    # ── 포기 확인. 화면 가운데를 덮는다. AbandonConfirmPanel 은 Overlay 라
    #    절대 좌표를 못 받는다. 안에 캔버스를 하나 깔고 그 위에 놓는다.
    outer = reuse(tree, "AbandonConfirmPanel", unreal.Overlay)
    outer.set_visibility(unreal.SlateVisibility.COLLAPSED)
    place(root, outer, (0.0, 0.0), (1920.0, 1080.0), 40)
    confirm = reuse(tree, "Set_confirm_canvas", unreal.CanvasPanel)
    confirm.clear_children()
    outer.add_child(confirm)
    dim = reuse(tree, "Set_confirm_dim", unreal.Border)
    dim.set_brush_color(unreal.LinearColor(0.02, 0.015, 0.01, 0.72))
    dim.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
    place(confirm, dim, (0.0, 0.0), (1920.0, 1080.0), 0)
    box = (610.0, 380.0, 700.0, 320.0)
    add_image(tree, confirm, "Set_confirm_plate", "T_KitA_Row_Plate",
              (box[0], box[1]), (box[2], box[3]), 1)
    add_text(tree, confirm, "AbandonConfirmTitleText", "탐험을 포기할까?", 36,
             (box[0] + 30.0, box[1] + 40.0), (box[2] - 60.0, 52.0), 4, CREAM, "center")
    add_text(tree, confirm, "AbandonConfirmBodyText",
             "지금까지 모은 것은 사라진다.", 24,
             (box[0] + 30.0, box[1] + 104.0), (box[2] - 60.0, 70.0), 4, PARCHMENT, "center")
    for index, (button_name, text_name, caption) in enumerate(
            (("CancelAbandonButton", "CancelAbandonButtonText", "돌아가기"),
             ("ConfirmAbandonButton", "ConfirmAbandonButtonText", "포기하기"))):
        bx = box[0] + 40.0 + (box[2] / 2.0 - 20.0) * index
        add_button(tree, confirm, button_name, (bx, box[1] + 210.0),
                   (box[2] / 2.0 - 60.0, 66.0), 4)
        add_text(tree, confirm, text_name, caption, 26,
                 (bx, box[1] + 226.0), (box[2] / 2.0 - 60.0, 40.0), 8, CREAM, "center")

    # 새 배치가 쓰지 않는 옛 위젯을 접힌 캔버스에 도로 붙인다.
    #
    # clear_children() 는 떼기만 하고 지우지 않는다. 떼인 채로 두면 변수 GUID 만
    # 남아 컴파일마다 "Variable [X] was deleted but still has a GUID" ensure 가
    # 뜬다. 파이썬에는 그 GUID 를 지우는 길이 없으므로 붙여 두는 게 답이다.
    parking = reuse(tree, "Set_parked", unreal.CanvasPanel)
    parking.clear_children()
    parking.set_visibility(unreal.SlateVisibility.COLLAPSED)
    place(root, parking, (0.0, 0.0), (1.0, 1.0), -100)
    parked = 0
    for line in ORPHANS.read_text(encoding="utf-8").splitlines():
        name = line.strip()
        if not name or name.startswith("#"):
            continue
        widget = find(tree, name)
        if widget is None or widget.get_parent() is not None:
            continue
        widget.modify()
        place(parking, widget, (0.0, 0.0), (1.0, 1.0), 0)
        widget.set_visibility(unreal.SlateVisibility.COLLAPSED)
        parked += 1
    LINES.append(f"parked={parked}")

    # 값을 바꿀 물건이 없는 줄들과, 이번에 안 쓰게 된 줄 받침들. 접는다.
    for name in DEAD_ROWS + ROW_PLATES:
        widget = find(tree, name)
        if widget is not None:
            widget.modify()
            place(root, widget, (0.0, 0.0), (1.0, 1.0), -50)
            widget.set_visibility(unreal.SlateVisibility.COLLAPSED)

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    saved = unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)
    LINES.append(f"saved={saved}")
    for name in ("BackButton", "SaveAndExitButton", "MasterVolumeSlider",
                 "LanguageKoreanButton", "AbandonConfirmPanel", "RunActionsPanel"):
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
    RESULT.write_text("\n".join(LINES), encoding="utf-8")
