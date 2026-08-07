"""Build standalone Marchbound skill-detail / enemy-detail WBPs (full-screen).

Why these exist
---------------
구형 ``WBP_CombatDetailOverlay`` 는 아이콘/제목/부제/본문 네 칸짜리 범용 패널
하나를 스킬 상세·유닛 상세·이동 상세가 함께 썼다. 그래서 수치가 전부 부제 한
줄에 ``AP 1 · 피해 6~10`` 처럼 눌려 들어갔고, 담는 정보도 네 칸이 한계였다.

모바일 화면에서는 상세창이 화면을 거의 꽉 채워야 한다. 작은 팝업은 손가락으로
읽기도 누르기도 나쁘고, 남는 공간만큼 정보를 못 담는다. 그래서 두 화면 모두
몬스터탭과 같은 ``T_MT_BaseFrame`` (정확히 16:9) 을 전면에 깔고 1920x1080 을
가득 쓴다.

담는 정보는 팀 합의를 따른다.
* 사거리 / 적용 범위 -> UI 에서 표기 (글자 대신 실제 칸 그림으로 보여 준다)
* 조준 차단자 / 영향 차단자 -> 곡사·관통을 대신하는 새 규칙이라 뜻풀이까지 붙인다
* 타수 -> 2 이상일 때 표기
* description 은 스킬 효과 설명만

인게임 배선은 하지 않는다 -- 에셋만 만들어 눈으로 먼저 본다.

Run headless:
    UnrealEditor-Cmd.exe <project> -run=pythonscript
        -script="Tools/UI/build_combat_detail_panels.py"
"""

import json
from pathlib import Path

import unreal


# unreal.log 는 -run=pythonscript 의 stdout 으로 새지 않는다. 결과는 파일로 남긴다.
RESULT_PATH = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/detail_build.txt")
RESULT_LINES = []

# 배치한 그대로를 기록해 둔다. make_detail_preview.py 가 이걸로 미리보기를 그린다.
# 자산 저장이 막혀도(에디터가 WBP 를 열고 있을 때) 디자인은 볼 수 있어야 한다.
SPEC_PATH = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/detail_render_spec.json")
SPEC = {}
CURRENT_SPEC = []


def css_color(color):
    """LinearColor 를 미리보기용 rgba 문자열로. 감마만 맞추면 충분하다."""
    def channel(value):
        return int(round(max(0.0, min(1.0, float(value))) ** (1.0 / 2.2) * 255))

    return "rgba(%d,%d,%d,%.3f)" % (
        channel(color.r), channel(color.g), channel(color.b), float(color.a))


def record(kind, name, position, size, z_order, **extra):
    entry = {
        "name": name,
        "class": kind,
        "rect": [float(position[0]), float(position[1]), float(size[0]), float(size[1])],
        "z": int(z_order),
    }
    entry.update({key: value for key, value in extra.items() if value is not None})
    CURRENT_SPEC.append(entry)

PACKAGE_PATH = "/Game/UI/CombatDetail"
MARCHBOUND = "/Game/SVN/OutSideAsset/AICreation/UI/Marchbound"

DESIGN_WIDTH = 1920.0
DESIGN_HEIGHT = 1080.0

CREAM = unreal.LinearColor(1.0, 0.94, 0.82, 1.0)
PARCHMENT = unreal.LinearColor(0.86, 0.80, 0.68, 1.0)
GOLD = unreal.LinearColor(0.95, 0.78, 0.42, 1.0)
WELL = unreal.LinearColor(0.05, 0.035, 0.02, 0.62)
OUTLINE = unreal.LinearColor(0.02, 0.01, 0.0, 0.95)

# 판에 칠하는 위협 표시와 같은 색이어야 범례와 그림이 뜻을 갖는다.
MOVE_BAND = unreal.LinearColor(0.28, 0.60, 0.95, 0.95)
ATTACK_FILL = unreal.LinearColor(0.90, 0.32, 0.30, 0.95)
CASTER_MARK = unreal.LinearColor(0.98, 0.80, 0.35, 0.95)
EMPTY_CELL = unreal.LinearColor(0.10, 0.085, 0.065, 0.55)

# Dimmer→ScaleBox→SizeBox→Canvas 셸을 이미 갖춘 자산. prepare_blueprint 참고.
DONOR_ASSET = "/Game/UI/MonsterTab/WBP_MonsterTab_Marchbound"
DONOR_SHELL = (
    ("MonsterTabViewportRoot", "ViewportRoot"),
    ("MonsterTabWorldDimmer", "WorldDimmer"),
    ("MonsterTabScale", "Scale"),
    ("MonsterTabDesignSize", "DesignSize"),
    ("MonsterTabCanvas", "Canvas"),
)


TEXTURE_PATHS = {}


def texture(relative):
    """Marchbound 아래 상대 경로로 텍스처를 읽는다."""
    leaf = relative.rsplit("/", 1)[-1]
    path = f"{MARCHBOUND}/{relative}.{leaf}"
    asset = unreal.load_object(None, path)
    if asset is None:
        raise RuntimeError(f"Missing combat-detail texture: {path}")
    TEXTURE_PATHS[asset] = path
    return asset


def prepare_blueprint(asset_name, prefix):
    """셸을 갖춘 WBP를 복제해 알맹이만 비운 뒤 돌려준다.

    ``WidgetTree.RootWidget`` 은 Python 에 노출돼 있지 않아 빈 자산을 만들면
    루트를 꽂을 방법이 없다(팩토리도 헤드리스에서는 루트를 만들지 않는다).
    그래서 같은 셸이 필요한 몬스터탭을 본으로 삼아 복제하고, 설계 캔버스의
    자식만 걷어낸 뒤 이 화면의 내용을 새로 얹는다. 셸 위젯 이름은 화면 이름에
    맞춰 바꾼다 -- 새 자산을 열었을 때 남의 화면 이름이 보이면 안 된다.
    """
    asset_path = f"{PACKAGE_PATH}/{asset_name}"
    reused = unreal.EditorAssetLibrary.does_asset_exist(asset_path)
    if not reused and not unreal.EditorAssetLibrary.duplicate_asset(DONOR_ASSET, asset_path):
        raise RuntimeError(f"Could not duplicate {DONOR_ASSET} -> {asset_path}")

    blueprint = unreal.EditorAssetLibrary.load_asset(asset_path)
    if blueprint is None:
        raise RuntimeError(f"Could not load {asset_path}")
    blueprint.modify()
    tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")
    if tree is None:
        raise RuntimeError("WidgetTree not found on " + blueprint.get_path_name())
    tree.modify()

    # 이미 만들어 둔 자산이면 셸 이름이 이 화면 이름으로 바뀌어 있다. 지우고 다시
    # 만들지 않고 제자리에서 알맹이만 간다 -- 에디터가 열려 있으면 삭제가 막히고,
    # 재빌드가 잦은 작업이라 이쪽이 빠르기도 하다.
    tree_prefix = tree.get_path_name() + "."
    canvas = None
    for donor_name, suffix in DONOR_SHELL:
        widget = unreal.find_object(None, tree_prefix + prefix + suffix)
        if widget is None:
            widget = unreal.find_object(None, tree_prefix + donor_name)
            if widget is None:
                raise RuntimeError(f"Shell widget missing: {prefix + suffix} / {donor_name}")
            widget.modify()
            widget.rename(prefix + suffix)
        else:
            widget.modify()
        if isinstance(widget, unreal.CanvasPanel):
            canvas = widget
    if canvas is None:
        raise RuntimeError("Design canvas missing")

    canvas.clear_children()
    return blueprint, tree, canvas


def make(tree, widget_class, name):
    return unreal.new_object(widget_class, outer=tree, name=name)


def place(canvas, child, position, size, z_order, fill=False):
    slot = canvas.add_child_to_canvas(child)
    anchor_max = unreal.Vector2D(1.0, 1.0) if fill else unreal.Vector2D(0.0, 0.0)
    slot.set_anchors(unreal.Anchors(unreal.Vector2D(0.0, 0.0), anchor_max))
    slot.set_alignment(unreal.Vector2D(0.0, 0.0))
    slot.set_editor_property("auto_size", False)
    slot.set_position(unreal.Vector2D(position[0], position[1]))
    slot.set_size(unreal.Vector2D(size[0], size[1]))
    slot.set_z_order(z_order)
    return slot


def style_text(text_block, size, color, justify):
    font = text_block.get_editor_property("font")
    font.set_editor_property("size", size)
    outline = font.get_editor_property("outline_settings")
    outline.set_editor_property("outline_size", 2 if size >= 26 else 1)
    outline.set_editor_property("outline_color", OUTLINE)
    font.set_editor_property("outline_settings", outline)
    text_block.set_editor_property("font", font)
    text_block.set_color_and_opacity(unreal.SlateColor(color))
    text_block.set_editor_property("justification", justify)
    text_block.set_editor_property("shadow_offset", unreal.Vector2D(1.5, 1.5))
    text_block.set_editor_property(
        "shadow_color_and_opacity", unreal.LinearColor(0.02, 0.01, 0.0, 0.8))
    text_block.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)


def add_text(tree, canvas, name, value, font_size, position, size, z_order,
             color=CREAM, justify=unreal.TextJustify.CENTER, wrap=False):
    block = make(tree, unreal.TextBlock, name)
    block.set_text(unreal.Text(value))
    style_text(block, font_size, color, justify)
    if wrap:
        block.set_editor_property("auto_wrap_text", True)
        block.set_editor_property("wrap_text_at", size[0])
    place(canvas, block, position, size, z_order)
    record("TextBlock", name, position, size, z_order, text=value, fontSize=font_size,
           color=css_color(color), wrap=wrap,
           justify=str(justify).rsplit(".", 1)[-1].lower())
    return block


def add_image(tree, canvas, name, source, position, size, z_order):
    image = make(tree, unreal.Image, name)
    if source is not None:
        image.set_brush_from_texture(source, False)
    image.set_color_and_opacity(unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    image.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
    place(canvas, image, position, size, z_order)
    record("Image", name, position, size, z_order, texture=TEXTURE_PATHS.get(source))
    return image


def add_well(tree, canvas, name, position, size, z_order, color=WELL):
    border = make(tree, unreal.Border, name)
    border.set_brush_color(color)
    border.set_editor_property("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
    border.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
    place(canvas, border, position, size, z_order)
    record("Border", name, position, size, z_order, color=css_color(color))
    return border


def add_transparent_button(tree, canvas, name, position, size, z_order):
    button = make(tree, unreal.Button, name)
    style = button.get_editor_property("widget_style")
    empty = unreal.SlateBrush()
    empty.set_editor_property("draw_as", unreal.SlateBrushDrawType.NO_DRAW_TYPE)
    for state in ("normal", "hovered", "pressed", "disabled"):
        style.set_editor_property(state, empty)
    button.set_editor_property("widget_style", style)
    place(canvas, button, position, size, z_order)
    record("Button", name, position, size, z_order)
    return button


def add_stat_chip(tree, canvas, base_name, chip_frame, label, value, position, extent, z_order):
    """수치 하나를 라벨+값 두 줄짜리 정사각 칩으로 놓는다.

    구형 패널은 ``AP 1 · 피해 6~10 · 쿨타임 2턴`` 을 한 줄에 이어 붙여 눈이
    구분자를 세어야 했다. 칩으로 끊으면 값 자리가 고정돼 스킬을 바꿔 봐도 같은
    위치에서 같은 수치를 읽는다.
    """
    add_image(tree, canvas, base_name + "Frame", chip_frame, position, (extent, extent), z_order)
    add_text(tree, canvas, base_name + "Label", label, int(extent * 0.17),
             (position[0], position[1] + extent * 0.19), (extent, extent * 0.24),
             z_order + 1, GOLD)
    add_text(tree, canvas, base_name + "Value", value, int(extent * 0.26),
             (position[0], position[1] + extent * 0.45), (extent, extent * 0.36),
             z_order + 1, CREAM)


def add_pattern_grid(tree, canvas, base_name, origin, extent, gap, count, pattern, z_order):
    """사거리/영향 범위를 실제 칸 그림으로 놓는다.

    구형 패널은 ``사거리 1 (십자)`` 처럼 글로만 적어서, 십자가 어느 칸까지인지
    플레이어가 머릿속으로 그려야 했다. 칸으로 보여 주면 한 번에 읽힌다.
    각 칸은 이름이 붙은 개별 위젯이라 런타임에 스킬별로 색만 갈아 주면 된다.

    pattern: {(row, col): "caster" | "range" | "area"} -- 나머지는 빈 칸
    """
    palette = {
        "caster": CASTER_MARK,
        "range": MOVE_BAND,
        "area": ATTACK_FILL,
    }
    step = extent + gap
    for row in range(count):
        for column in range(count):
            kind = pattern.get((row, column))
            add_well(
                tree, canvas, f"{base_name}_R{row}C{column}",
                (origin[0] + step * column, origin[1] + step * row),
                (extent, extent), z_order,
                palette.get(kind, EMPTY_CELL))


def add_header(tree, canvas, prefix, title, title_plate, back_button):
    """화면 이름표와 닫기. 두 상세 화면이 같은 자리에 같은 크기로 둔다."""
    add_image(tree, canvas, prefix + "TitlePlate", title_plate, (650.0, 30.0), (620.0, 154.0), 10)
    add_text(tree, canvas, prefix + "TitleText", title, 56, (650.0, 68.0), (620.0, 78.0), 12, CREAM)
    add_image(tree, canvas, prefix + "CloseArt", back_button, (1630.0, 56.0), (230.0, 91.0), 10)
    add_text(tree, canvas, prefix + "CloseLabel", "닫기", 34, (1630.0, 80.0), (230.0, 46.0), 12, CREAM)
    add_transparent_button(tree, canvas, prefix + "CloseButton", (1630.0, 56.0), (230.0, 91.0), 30)


def begin(asset_name):
    """이 화면의 배치 기록을 새로 연다."""
    CURRENT_SPEC.clear()
    SPEC[asset_name] = CURRENT_SPEC


def finish(blueprint, asset_name, tag, detail):
    SPEC[asset_name] = list(CURRENT_SPEC)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    # 저장 결과를 반드시 본다. 에디터가 같은 자산을 열고 있으면 패키지가 잠겨
    # 저장이 조용히 실패하는데, 그걸 성공으로 보고하면 옛 자산을 새 것으로 착각한다.
    # 저장이 막혀도 배치 기록은 남기므로 미리보기로 디자인은 확인할 수 있다.
    if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False):
        RESULT_LINES.append(
            f"{tag} SAVE BLOCKED asset={blueprint.get_path_name()} {detail} "
            "-- 언리얼 에디터에서 이 WBP 탭을 닫고 다시 실행하세요.")
        return
    RESULT_LINES.append(f"{tag} success asset={blueprint.get_path_name()} {detail}")


def build_skill_detail():
    """스킬 상세: 1920x1080 전면. 정체성 / 차단 규칙 / 범위 그림 / 효과."""
    begin("WBP_SkillDetail_Marchbound")
    blueprint, tree, canvas = prepare_blueprint("WBP_SkillDetail_Marchbound", "SkillDetail")

    base_frame = texture("MonsterTab/T_MT_BaseFrame")
    title_plate = texture("Hire/T_MB_HireTitlePlate")
    back_button = texture("Hire/T_MB_HireBackButton")
    skill_card = texture("Combat/T_SkillCard_Frame_Combat")
    chip_frame = texture("Combat/T_MB_StatusSlot_Frame")
    status_slot = texture("Combat/T_MB_ArtifactSlot_Frame")

    add_image(tree, canvas, "SkillDetailBaseFrame", base_frame,
              (0.0, 0.0), (DESIGN_WIDTH, DESIGN_HEIGHT), 0)
    add_header(tree, canvas, "SkillDetail", "스킬 상세", title_plate, back_button)

    # --- 좌상: 정체성 + 핵심 수치 ---------------------------------------
    add_well(tree, canvas, "SkillIdentityWell", (70.0, 210.0), (870.0, 430.0), 5)
    add_image(tree, canvas, "SkillIconFrame", skill_card, (108.0, 250.0), (226.0, 240.0), 10)
    add_image(tree, canvas, "SkillIconImage", None, (126.0, 268.0), (190.0, 190.0), 11)
    add_text(tree, canvas, "SkillNameText", "베기", 50,
             (370.0, 252.0), (420.0, 76.0), 12, CREAM, unreal.TextJustify.LEFT)
    add_text(tree, canvas, "SkillGradeText", "희귀 등급", 28,
             (370.0, 332.0), (420.0, 40.0), 12, GOLD, unreal.TextJustify.LEFT)
    add_text(tree, canvas, "SkillKindText", "액티브 · 근접 · 단일 대상", 24,
             (370.0, 376.0), (420.0, 38.0), 12, PARCHMENT, unreal.TextJustify.LEFT)

    # 타수는 2 이상일 때만 쓰기로 합의된 값이라 칩을 따로 뒀다. 1 이면 런타임에서 숨긴다.
    chip_extent = 140.0
    chip_gap = 16.0
    chips = [
        ("SkillChipCost", "AP", "1"),
        ("SkillChipDamage", "피해", "6~10"),
        ("SkillChipCooldown", "쿨타임", "-"),
        ("SkillChipHits", "타수", "3"),
        ("SkillChipRange", "사거리", "1"),
    ]
    chip_start = 70.0 + (870.0 - (chip_extent * 5 + chip_gap * 4)) * 0.5
    for index, (name, label, value) in enumerate(chips):
        add_stat_chip(tree, canvas, name, chip_frame, label, value,
                      (chip_start + (chip_extent + chip_gap) * index, 470.0), chip_extent, 10)

    # --- 좌하: 차단 규칙 + 부여 효과 -------------------------------------
    add_well(tree, canvas, "SkillRuleWell", (70.0, 665.0), (870.0, 345.0), 5)
    add_text(tree, canvas, "SkillRuleHeading", "차단 규칙", 30,
             (100.0, 682.0), (300.0, 44.0), 10, GOLD, unreal.TextJustify.LEFT)
    add_text(tree, canvas, "SkillAimBlockerLabel", "조준 차단", 26,
             (100.0, 736.0), (200.0, 40.0), 10, PARCHMENT, unreal.TextJustify.LEFT)
    add_text(tree, canvas, "SkillAimBlockerValue", "장애물", 28,
             (310.0, 736.0), (240.0, 40.0), 10, CREAM, unreal.TextJustify.LEFT)
    add_text(tree, canvas, "SkillEffectBlockerLabel", "영향 차단", 26,
             (100.0, 786.0), (200.0, 40.0), 10, PARCHMENT, unreal.TextJustify.LEFT)
    add_text(tree, canvas, "SkillEffectBlockerValue", "없음", 28,
             (310.0, 786.0), (240.0, 40.0), 10, CREAM, unreal.TextJustify.LEFT)

    add_text(tree, canvas, "SkillGrantHeading", "부여 효과", 30,
             (600.0, 682.0), (300.0, 44.0), 10, GOLD, unreal.TextJustify.LEFT)
    grants = [("SkillGrant_0", "취약 2"), ("SkillGrant_1", "-"), ("SkillGrant_2", "-")]
    for index, (name, label) in enumerate(grants):
        slot_x = 600.0 + 106.0 * index
        add_image(tree, canvas, name + "Frame", status_slot, (slot_x, 736.0), (96.0, 96.0), 10)
        add_image(tree, canvas, name + "Icon", None, (slot_x + 14.0, 750.0), (68.0, 68.0), 11)
        add_text(tree, canvas, name + "Text", label, 20,
                 (slot_x - 6.0, 838.0), (108.0, 32.0), 12, PARCHMENT)

    # 곡사·관통을 대신하는 새 규칙이라 뜻풀이를 붙인다. 값만 보면 무슨 말인지 모른다.
    add_text(tree, canvas, "SkillRuleNoteText",
             "차단자로 지정한 대상만 이 스킬을 막는다. '없음'이면 무엇에도 막히지 않는다.",
             22, (100.0, 890.0), (740.0, 100.0), 10, PARCHMENT, unreal.TextJustify.LEFT, True)

    # --- 우상: 사거리 / 영향 범위 그림 -----------------------------------
    add_well(tree, canvas, "SkillRangeWell", (980.0, 210.0), (870.0, 500.0), 5)
    add_text(tree, canvas, "SkillRangeHeading", "사거리", 30,
             (1010.0, 228.0), (300.0, 44.0), 10, GOLD, unreal.TextJustify.LEFT)
    add_text(tree, canvas, "SkillAreaHeading", "영향 범위", 30,
             (1440.0, 228.0), (300.0, 44.0), 10, GOLD, unreal.TextJustify.LEFT)

    # 5x5 판, 가운데가 시전자. 십자 거리 1 과 단일 타격을 기본값으로 그려 둔다.
    center = (2, 2)
    range_pattern = {center: "caster"}
    for offset in ((-1, 0), (1, 0), (0, -1), (0, 1)):
        range_pattern[(center[0] + offset[0], center[1] + offset[1])] = "range"
    area_pattern = {center: "caster", (1, 2): "area"}

    add_pattern_grid(tree, canvas, "SkillRangeCell", (1010.0, 290.0), 68.0, 6.0, 5,
                     range_pattern, 10)
    add_pattern_grid(tree, canvas, "SkillAreaCell", (1440.0, 290.0), 68.0, 6.0, 5,
                     area_pattern, 10)
    add_text(tree, canvas, "SkillRangeCaption", "십자 · 거리 1", 22,
             (1010.0, 662.0), (364.0, 36.0), 10, PARCHMENT)
    add_text(tree, canvas, "SkillAreaCaption", "단일 대상 1칸", 22,
             (1440.0, 662.0), (364.0, 36.0), 10, PARCHMENT)

    # --- 우하: 효과 설명 --------------------------------------------------
    add_well(tree, canvas, "SkillEffectWell", (980.0, 730.0), (870.0, 280.0), 5)
    add_text(tree, canvas, "SkillEffectHeading", "효과", 30,
             (1010.0, 746.0), (300.0, 44.0), 10, GOLD, unreal.TextJustify.LEFT)
    effects = [
        ("SkillEffectText_0", "· 대상에게 6~10의 피해를 준다."),
        ("SkillEffectText_1", "· 3타로 나누어 공격한다."),
        ("SkillEffectText_2", "· 마지막 타격에 취약 2를 부여한다."),
    ]
    for index, (name, line) in enumerate(effects):
        add_text(tree, canvas, name, line, 26,
                 (1010.0, 800.0 + 50.0 * index), (810.0, 44.0), 10, CREAM,
                 unreal.TextJustify.LEFT)
    add_text(tree, canvas, "SkillFlavorText", "칼끝으로 상대의 균형을 무너뜨린다.", 22,
             (1010.0, 952.0), (810.0, 44.0), 10, PARCHMENT, unreal.TextJustify.LEFT)

    finish(blueprint, "WBP_SkillDetail_Marchbound", "RD_SKILL_DETAIL_BUILD",
           "panel=1920x1080 chips=5 grids=2x5x5")


def build_enemy_detail():
    """적 상세: 1920x1080 전면. 정체성/HP/스탯 · 상태 · 스킬 목록 · 위협 범위."""
    begin("WBP_EnemyDetail_Marchbound")
    blueprint, tree, canvas = prepare_blueprint("WBP_EnemyDetail_Marchbound", "EnemyDetail")

    base_frame = texture("MonsterTab/T_MT_BaseFrame")
    title_plate = texture("Hire/T_MB_HireTitlePlate")
    back_button = texture("Hire/T_MB_HireBackButton")
    portrait_frame = texture("RewardSettlement/T_RS_PortraitFrame")
    chip_frame = texture("Combat/T_MB_StatusSlot_Frame")
    status_slot = texture("Combat/T_MB_ArtifactSlot_Frame")
    skill_slot = texture("Common/T_MB_ActionButtonFrame")

    add_image(tree, canvas, "EnemyDetailBaseFrame", base_frame,
              (0.0, 0.0), (DESIGN_WIDTH, DESIGN_HEIGHT), 0)
    add_header(tree, canvas, "EnemyDetail", "적 정보", title_plate, back_button)

    # --- 좌상: 초상화 + 이름 + HP + 수치 ---------------------------------
    add_well(tree, canvas, "EnemyIdentityWell", (70.0, 210.0), (730.0, 560.0), 5)
    add_image(tree, canvas, "EnemyPortraitFrame", portrait_frame, (110.0, 250.0), (300.0, 300.0), 10)
    add_image(tree, canvas, "EnemyPortraitImage", None, (124.0, 264.0), (272.0, 272.0), 11)
    add_text(tree, canvas, "EnemyNameText", "Mushroom", 50,
             (450.0, 258.0), (320.0, 76.0), 12, CREAM, unreal.TextJustify.LEFT)
    add_text(tree, canvas, "EnemyLevelText", "Lv.1 · 적", 28,
             (450.0, 336.0), (320.0, 44.0), 12, GOLD, unreal.TextJustify.LEFT)

    # HP 는 숫자만 있던 것을 막대로 바꾼다. 남은 양을 눈으로 먼저 읽게 하려는 것이다.
    add_well(tree, canvas, "EnemyHpTrack", (450.0, 396.0), (320.0, 48.0), 10)
    hp_bar = make(tree, unreal.ProgressBar, "EnemyHpBar")
    hp_bar.set_percent(1.0)
    hp_bar.set_fill_color_and_opacity(unreal.LinearColor(0.78, 0.20, 0.18, 1.0))
    hp_bar.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
    place(canvas, hp_bar, (454.0, 400.0), (312.0, 40.0), 11)
    record("ProgressBar", "EnemyHpBar", (454.0, 400.0), (312.0, 40.0), 11,
           percent=1.0, color=css_color(unreal.LinearColor(0.78, 0.20, 0.18, 1.0)))
    add_text(tree, canvas, "EnemyHpText", "50 / 50", 26, (450.0, 404.0), (320.0, 36.0), 12, CREAM)

    chip_extent = 150.0
    chip_gap = 16.0
    chips = [
        ("EnemyChipAp", "AP", "0 / 2"),
        ("EnemyChipSpeed", "속도", "5"),
        ("EnemyChipDefense", "방어", "0"),
        ("EnemyChipThreat", "예상 피해", "6~10"),
    ]
    chip_start = 70.0 + (730.0 - (chip_extent * 4 + chip_gap * 3)) * 0.5
    for index, (name, label, value) in enumerate(chips):
        add_stat_chip(tree, canvas, name, chip_frame, label, value,
                      (chip_start + (chip_extent + chip_gap) * index, 600.0), chip_extent, 10)

    # --- 좌하: 현재 걸린 상태 --------------------------------------------
    add_well(tree, canvas, "EnemyStatusWell", (70.0, 795.0), (730.0, 215.0), 5)
    add_text(tree, canvas, "EnemyStatusHeading", "상태", 30,
             (100.0, 812.0), (250.0, 44.0), 10, GOLD, unreal.TextJustify.LEFT)
    statuses = [("EnemyStatus_0", "취약 2"), ("EnemyStatus_1", "-"),
                ("EnemyStatus_2", "-"), ("EnemyStatus_3", "-")]
    for index, (name, label) in enumerate(statuses):
        slot_x = 110.0 + 116.0 * index
        add_image(tree, canvas, name + "Frame", status_slot, (slot_x, 866.0), (96.0, 96.0), 10)
        add_image(tree, canvas, name + "Icon", None, (slot_x + 14.0, 880.0), (68.0, 68.0), 11)
        add_text(tree, canvas, name + "Text", label, 20,
                 (slot_x - 10.0, 966.0), (116.0, 32.0), 12, PARCHMENT)

    # --- 우상: 스킬 목록 --------------------------------------------------
    # 몬스터탭은 스킬을 민무늬 글상자로만 보여 준다. 여기서는 아이콘·이름·비용·피해를
    # 한 행에 묶어 용병탭과 같은 규칙으로 읽히게 한다.
    add_well(tree, canvas, "EnemySkillWell", (840.0, 210.0), (1010.0, 450.0), 5)
    add_text(tree, canvas, "EnemySkillHeading", "스킬", 30,
             (870.0, 226.0), (300.0, 44.0), 10, GOLD, unreal.TextJustify.LEFT)
    skills = [
        ("야수의 발톱", "AP 1", "피해 6~10"),
        ("도약", "AP 2", "이동 3칸"),
        ("포효", "AP 1", "약화 2턴"),
        ("사냥 본능", "패시브", "체력 50% 이하 시 공격 +2"),
    ]
    for index, (name, cost, effect) in enumerate(skills):
        row_y = 284.0 + 92.0 * index
        add_well(tree, canvas, f"EnemySkillRow_{index}", (870.0, row_y), (940.0, 82.0), 8,
                 unreal.LinearColor(0.09, 0.07, 0.05, 0.55))
        add_image(tree, canvas, f"EnemySkillRowFrame_{index}", skill_slot,
                  (884.0, row_y + 9.0), (64.0, 64.0), 10)
        add_image(tree, canvas, f"EnemySkillRowIcon_{index}", None,
                  (894.0, row_y + 19.0), (44.0, 44.0), 11)
        add_text(tree, canvas, f"EnemySkillRowName_{index}", name, 30,
                 (964.0, row_y + 16.0), (340.0, 50.0), 12, CREAM, unreal.TextJustify.LEFT)
        add_text(tree, canvas, f"EnemySkillRowCost_{index}", cost, 26,
                 (1320.0, row_y + 18.0), (130.0, 46.0), 12, GOLD, unreal.TextJustify.LEFT)
        add_text(tree, canvas, f"EnemySkillRowEffect_{index}", effect, 24,
                 (1460.0, row_y + 18.0), (340.0, 46.0), 12, PARCHMENT, unreal.TextJustify.LEFT)

    # --- 우하: 위협 범위 + 패시브 ----------------------------------------
    add_well(tree, canvas, "EnemyThreatWell", (840.0, 685.0), (1010.0, 325.0), 5)
    add_text(tree, canvas, "EnemyThreatHeading", "위협 범위", 30,
             (870.0, 700.0), (300.0, 44.0), 10, GOLD, unreal.TextJustify.LEFT)

    # 7x7 판. 한 턴에 닿는 곳을 그대로 그린다 -- 글로만 적혀 있던 범례가 그림을 갖는다.
    threat_pattern = {(3, 3): "caster"}
    for row in range(7):
        for column in range(7):
            distance = abs(row - 3) + abs(column - 3)
            if distance == 0:
                continue
            if distance <= 2:
                threat_pattern[(row, column)] = "area"
            elif distance <= 4:
                threat_pattern[(row, column)] = "range"
    add_pattern_grid(tree, canvas, "EnemyThreatCell", (880.0, 754.0), 30.0, 4.0, 7,
                     threat_pattern, 10)

    add_well(tree, canvas, "EnemyLegendMoveSwatch", (1160.0, 776.0), (34.0, 34.0), 10, MOVE_BAND)
    add_text(tree, canvas, "EnemyLegendMoveText", "테두리 = 이동 범위", 24,
             (1206.0, 778.0), (340.0, 34.0), 10, PARCHMENT, unreal.TextJustify.LEFT)
    add_well(tree, canvas, "EnemyLegendAttackSwatch", (1160.0, 826.0), (34.0, 34.0), 10, ATTACK_FILL)
    add_text(tree, canvas, "EnemyLegendAttackText", "채움 = 공격 범위", 24,
             (1206.0, 828.0), (340.0, 34.0), 10, PARCHMENT, unreal.TextJustify.LEFT)

    add_text(tree, canvas, "EnemyPassiveHeading", "패시브", 28,
             (1160.0, 890.0), (250.0, 40.0), 10, GOLD, unreal.TextJustify.LEFT)
    add_text(tree, canvas, "EnemyPassiveText", "패시브 없음", 24,
             (1160.0, 936.0), (660.0, 60.0), 10, PARCHMENT, unreal.TextJustify.LEFT, True)

    finish(blueprint, "WBP_EnemyDetail_Marchbound", "RD_ENEMY_DETAIL_BUILD",
           "panel=1920x1080 chips=4 skills=4 grid=7x7")


try:
    build_skill_detail()
    build_enemy_detail()
    RESULT_LINES.append("RD_COMBAT_DETAIL_PANELS_BUILD complete")
except Exception as error:  # noqa: BLE001
    import traceback
    RESULT_LINES.append("FAILED: %s" % error)
    RESULT_LINES.append(traceback.format_exc())
finally:
    SPEC_PATH.write_text(json.dumps(SPEC, ensure_ascii=False, indent=2), encoding="utf-8")
    RESULT_PATH.write_text("\n".join(RESULT_LINES), encoding="utf-8")
