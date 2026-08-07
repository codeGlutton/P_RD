"""확정된 요약판 시안(summary_layout.json)을 실제 WBP 에 옮긴다.

웹 편집기의 판(600x430)과 WBP 의 AllyPanel/EnemyPanel 이 같은 크기라
좌표를 1:1 로 옮기면 된다. 상세 오버레이(WBP_CombatDetailOverlay)는
기능 격자를 유지한 채 판·소켓·제목판만 시안 문법으로 간다.

Run headless (에디터·게임을 먼저 닫을 것 — 안 닫으면 저장이 조용히 실패한다):
    UnrealEditor-Cmd.exe <project> -run=pythonscript
        -script="Tools/UI/apply_summary_wbp.py"
"""

import json
from pathlib import Path

import unreal

ROOT = Path("D:/UnrealProjects/P_RD_develop_20260803")
RESULT = ROOT / "Saved/LegacyAudit/apply_summary.txt"
LAYOUT = json.loads((ROOT / "Tools/UI/mockups/summary_layout.json")
                    .read_text(encoding="utf-8"))
# 편집기에서 고른 에셋 이름 -> 실제 경로 (카탈로그 그대로)
ASSET_PATHS = {entry["name"]: entry["asset"] for entry in json.loads(
    (ROOT / "Tools/UI/mockups/assets.json").read_text(encoding="utf-8"))}


def texture_by_name(name):
    """카탈로그 이름으로 텍스처를 불러온다. 없어진 에셋이면 알리고 None.

    카탈로그(assets.json)는 뽑던 날 기준이라, 그 뒤 지워진 텍스처가 남아 있다.
    실제로 편집기에서 고른 판이 조용히 기본값으로 떨어진 일이 있었다(0806).
    """
    if not name:
        return None
    path = ASSET_PATHS.get(name)
    tex = unreal.EditorAssetLibrary.load_asset(f"{path}.{name}") if path else None
    if tex is None:
        say(f"    !! 고른 에셋이 프로젝트에 없음: {name} -- 기본 에셋으로 대체")
    return tex

HUD = "/Game/UI/CombatLayouts/WBP_CombatHUD04"
OVERLAY = "/Game/UI/CombatDetail/WBP_CombatDetailOverlay"

AI = "/Game/SVN/OutSideAsset/AICreation/UI"
TEX = {
    "portrait_frame": f"{AI}/Marchbound/RewardSettlement/T_RS_PortraitFrame.T_RS_PortraitFrame",
    "row_empty": f"{AI}/Marchbound/Hire/T_MB_HirePartyRowEmpty.T_MB_HirePartyRowEmpty",
    "socket": f"{AI}/Hire/T_Hire_DetailSkillSocket_V08.T_Hire_DetailSkillSocket_V08",
    "hp_frame": f"{AI}/CombatHUD/UnitHpBar/T_CombatHUD_UnitHpBar_Backplate_FrameOnly"
                ".T_CombatHUD_UnitHpBar_Backplate_FrameOnly",
    "panel": f"{AI}/Marchbound/Common/T_MB_GenericDetailPanel.T_MB_GenericDetailPanel",
    "kita_portrait": f"{AI}/Marchbound/KitA/T_KitA_Portrait_Frame.T_KitA_Portrait_Frame",
    "btn_wide": f"{AI}/Marchbound/KitA/T_KitA_Button_Wide_Normal.T_KitA_Button_Wide_Normal",
    "name_plate": f"{AI}/Marchbound/Hire/T_MB_HireNamePlate.T_MB_HireNamePlate",
    "stats_strip": f"{AI}/Marchbound/Hire/T_MB_HireStatsStrip.T_MB_HireStatsStrip",
    "divider": f"{AI}/Hire/T_Hire_Divider_V11.T_Hire_Divider_V11",
    "hp_icon": f"{AI}/HUD04/KK_HUD04_hp_icon.KK_HUD04_hp_icon",
    "ap_gem": f"{AI}/HUD04/KK_HUD04_zone_cost_badge.KK_HUD04_zone_cost_badge",
    "speed_icon": f"{AI}/Marchbound/Combat/T_MB_Icon_Speed.T_MB_Icon_Speed",
    "defense_icon": f"{AI}/CombatHUD/UnitHpBar/T_UnitHpBar_Defense_Icon"
                    ".T_UnitHpBar_Defense_Icon",
    "crit_gem": f"{AI}/HUD04/KK_HUD04_ap_pip_lit.KK_HUD04_ap_pip_lit",
    "info_panel": f"{AI}/Hire/T_Hire_InfoPanel_V11.T_Hire_InfoPanel_V11",
    "btn_small": f"{AI}/Marchbound/KitA/T_KitA_Button_Small_Normal"
                 ".T_KitA_Button_Small_Normal",
}

# 웹 편집기 DRAW 와 같은 글자 크기 규칙 (px = 칸 높이 x 비율, pt = px x 0.75)
FONT_FACTOR = {"name": 0.8, "hp": 0.44, "ap": 0.46, "speed": 0.46}
# 글꼴별 줄상자 비율(em). 세로 가운데 = 위정렬 + (칸높이 - px*비율)/2 여백.
LINE_RATIO = {"Oswald": 1.49, "LINESeed": 1.45}

LINES = []


def say(text):
    LINES.append(text)
    unreal.log(text)


def srgb_to_linear(c):
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def color(hex_str, alpha=1.0):
    r, g, b = (int(hex_str[i:i + 2], 16) / 255.0 for i in (0, 2, 4))
    return unreal.LinearColor(srgb_to_linear(r), srgb_to_linear(g),
                              srgb_to_linear(b), alpha)


COL_NAME = color("F2E0B4")
COL_DARK = color("3A2416")
COL_LIGHT = color("E8D7B4")

FONTS = {
    "Oswald": "/Game/SVN/OutSideAsset/Fonts/F_HUD_Oswald.F_HUD_Oswald",
    "LINESeed": "/Game/SVN/OutSideAsset/Fonts/F_HUD_LINESeedKR.F_HUD_LINESeedKR",
}


def find(tree, name):
    return unreal.find_object(None, f"{tree.get_path_name()}.{name}")


def canvas_slot(widget):
    slot = widget.get_editor_property("slot") if widget else None
    return slot if isinstance(slot, unreal.CanvasPanelSlot) else None


PANEL_W, PANEL_H = 600.0, 430.0


def set_box(widget, x, y, w, h, z=None):
    slot = canvas_slot(widget)
    if slot is None:
        # 오버레이 자식에 캔버스 좌표를 쓰면 음수 패딩이 되어 부모 밖으로 밀린다.
        # 그러면 그림은 보여도 눌림이 안 닿는다 (0806: 몬스터 탭 닫기가 안 먹음).
        # 자리는 부모 마운트가 잡게 두고, 여기서는 알리고 손 뗀다.
        say(f"    !! 캔버스 슬롯 아님(자리 안 건드림): "
            f"{widget.get_name() if widget else '?'}")
        return
    # 비율 앵커가 남아 있으면 절대좌표가 엉뚱하게 해석된다(상세 오버레이 기둥이
    # 그랬다). 좌상단 점 앵커로 통일하고 나서 자리를 준다.
    slot.set_anchors(unreal.Anchors(minimum=unreal.Vector2D(0.0, 0.0),
                                    maximum=unreal.Vector2D(0.0, 0.0)))
    slot.set_alignment(unreal.Vector2D(0.0, 0.0))
    slot.set_auto_size(False)
    slot.set_offsets(unreal.Margin(float(x), float(y), float(w), float(h)))
    if z is not None:
        slot.set_z_order(z)


def overlay_pad(widget, left, top, right, bottom, h="fill", v="fill"):
    slot = widget.get_editor_property("slot") if widget else None
    if not isinstance(slot, unreal.OverlaySlot):
        say(f"    !! 오버레이 슬롯 아님: {widget.get_name() if widget else '?'}")
        return
    align_h = {"fill": unreal.HorizontalAlignment.H_ALIGN_FILL,
               "left": unreal.HorizontalAlignment.H_ALIGN_LEFT}[h]
    align_v = {"fill": unreal.VerticalAlignment.V_ALIGN_FILL,
               "top": unreal.VerticalAlignment.V_ALIGN_TOP}[v]
    slot.set_horizontal_alignment(align_h)
    slot.set_vertical_alignment(align_v)
    slot.set_padding(unreal.Margin(float(left), float(top),
                                   float(right), float(bottom)))


def collapse(tree, name):
    widget = find(tree, name)
    if widget is not None:
        widget.set_visibility(unreal.SlateVisibility.COLLAPSED)
        return True
    return False


def set_brush(widget, tex_key):
    """Image/Border 의 그림을 통짜(Image)로 바꾸고, 남아 있던 색물(틴트)을 씻는다."""
    tex = unreal.EditorAssetLibrary.load_asset(TEX[tex_key])
    if widget is None or tex is None:
        say(f"    !! 그림 못 바꿈: {tex_key}")
        return
    white = unreal.LinearColor(1.0, 1.0, 1.0, 1.0)
    prop = "brush" if isinstance(widget, unreal.Image) else "background"
    brush = widget.get_editor_property(prop)
    brush.set_editor_property("resource_object", tex)
    brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
    brush.set_editor_property("tint_color", unreal.SlateColor(white))
    widget.set_editor_property(prop, brush)
    if isinstance(widget, unreal.Border):
        # 옛 판의 초록/갈색은 Border 의 brush_color 가 물들인 것이다.
        widget.set_editor_property("brush_color", white)
    elif isinstance(widget, unreal.Image):
        widget.set_editor_property("color_and_opacity", white)


def line_ratio(text_widget):
    font = text_widget.get_editor_property("font")
    face = font.get_editor_property("font_object")
    name = face.get_name() if face else ""
    for key, ratio in LINE_RATIO.items():
        if key.lower() in name.lower():
            return ratio
    return 1.45


def outline_text(text_widget, size=2, col=None):
    """글자에 검은 테두리를 두른다. 양피지 위 흰 글자가 뭉개지지 않게."""
    if text_widget is None:
        return
    font = text_widget.get_editor_property("font")
    outline = font.get_editor_property("outline_settings")
    outline.set_editor_property("outline_size", int(size))
    outline.set_editor_property(
        "outline_color", col if col is not None
        else unreal.LinearColor(0.0, 0.0, 0.0, 1.0))
    font.set_editor_property("outline_settings", outline)
    text_widget.set_editor_property("font", font)


def style_text(text_widget, box_h, factor, col=None, dark=False, fs=0, face=None):
    """글자 크기/색/글꼴을 웹 규칙대로 잡고, _Center 안에서 세로 가운데 여백을 계산한다."""
    if text_widget is None:
        return
    px = fs if fs > 0 else round(box_h * factor)
    pt = round(px * 0.75)
    font = text_widget.get_editor_property("font")
    font.set_editor_property("size", pt)
    if face in FONTS:
        face_obj = unreal.load_object(None, FONTS[face])
        if face_obj is not None:
            font.set_editor_property("font_object", face_obj)
            font.set_editor_property("typeface_font_name", "Bold")
    text_widget.set_editor_property("font", font)
    if col is not None:
        text_widget.set_editor_property(
            "color_and_opacity", unreal.SlateColor(col))
    if dark:
        # 양피지 판 위 짙은 글자는 그림자를 빼야 판에 붙어 보인다.
        text_widget.set_editor_property(
            "shadow_color_and_opacity", unreal.LinearColor(0, 0, 0, 0))
    pad = (box_h - px * line_ratio(text_widget)) / 2.0
    overlay_pad(text_widget, 0, pad, 0, pad)
    return pt


def place_summary_panel(tree, prefix):
    """요약판을 화면 모서리로 물리고 줄인다.

    카드 여섯 장이 화면 가운데를 쓰는데 요약판이 600x430 그대로면 왼쪽
    카드 줄과 겹친다(0806 검수). 판 안의 자리는 그대로 두고 **판째로**
    줄여(0.72) 모서리에 붙인다 -- 안쪽 좌표를 다시 잡을 필요가 없다.
    """
    panel = find(tree, f"{prefix}Panel")
    if canvas_slot(panel) is None:
        return
    scale = 0.72
    width, height = 600.0, 430.0
    margin_x, top = 16.0, 168.0
    left = margin_x if prefix == "Ally" else 1920.0 - margin_x - width * scale
    # set_box 로 둬야 앵커까지 좌상단으로 맞춘다. 오른쪽 앵커가 남아 있으면
    # 같은 숫자가 "오른쪽 끝에서 1472px" 로 읽혀 판이 화면 밖으로 나간다.
    set_box(panel, left, top, width, height)
    panel.set_render_transform_pivot(unreal.Vector2D(0.0, 0.0))
    transform = unreal.WidgetTransform()
    transform.set_editor_property("scale", unreal.Vector2D(scale, scale))
    panel.set_render_transform(transform)
    say(f"    판 자리 {left:.0f},{top:.0f} x{scale} (카드와 안 겹치게)")


def apply_summary_panel(tree, prefix):
    """AllyPanel/EnemyPanel 하나에 확정 배치를 적용한다."""
    say(f"  [{prefix}Panel]")
    place_summary_panel(tree, prefix)
    lay = LAYOUT

    # 초상: 틀을 시안 에셋으로, 얼굴은 틀 안 72% 칸으로
    p = lay["portrait"]
    frame = find(tree, f"{prefix}PortraitFrame")
    set_box(frame, p["x"], p["y"], p["w"], p["h"])
    set_brush(frame, "portrait_frame")
    set_box(find(tree, f"{prefix}Portrait"),
            p["x"] + p["w"] * 0.14, p["y"] + p["h"] * 0.14,
            p["w"] * 0.72, p["h"] * 0.72)

    # 이름: 위 나무띠 안 (한글 이름이라 LINESeed).
    # 이름 칸은 판 전체 오버레이(PlateMount) 안이라 패딩으로 자리를 만든다.
    n = lay["name"]
    place(find(tree, f"{prefix}Name_Center"), (0, 0, PANEL_W, PANEL_H),
          n["x"], n["y"], n["w"], n["h"])
    style_text(find(tree, f"{prefix}Name"), n["h"], FONT_FACTOR["name"],
               col=COL_NAME, fs=int(n.get("fs") or 0), face="LINESeed")

    # HP바: 틀 + 채움(그어둔 칸 비율) + 숫자
    h = lay["hp"]
    set_box(find(tree, f"{prefix}HPBackMount"), h["x"], h["y"], h["w"], h["h"])
    back = find(tree, f"{prefix}HPBack")
    if back is not None:
        overlay_pad(back, 0, 0, 0, 0)
        set_brush(back, "hp_frame")
    overlay_pad(find(tree, f"{prefix}HPBar"),
                h["w"] * 0.088, h["h"] * 0.222, h["w"] * 0.089, h["h"] * 0.222)
    center = find(tree, f"{prefix}HPText_Center")
    if center is not None:
        overlay_pad(center, 0, 0, 0, 0)
    style_text(find(tree, f"{prefix}HPText"), h["h"], FONT_FACTOR["hp"],
               fs=int(h.get("fs") or 0), face="Oswald")

    # AP 칸
    a = lay["ap"]
    set_box(find(tree, f"{prefix}APPlateMount"), a["x"], a["y"], a["w"], a["h"])
    set_brush(find(tree, f"{prefix}APPlate"), "row_empty")
    center = find(tree, f"{prefix}APText_Center")
    if center is not None:
        overlay_pad(center, 0, 0, 0, 0)
    # 판(RowEmpty)이 중간 갈색이라 짙은 글자는 안 읽힌다(0806 검수). 밝은 글자+그림자.
    style_text(find(tree, f"{prefix}APText"), a["h"], FONT_FACTOR["ap"],
               col=COL_NAME, fs=int(a.get("fs") or 0), face="Oswald")

    # 속도 칸: 판 + 아이콘 + 숫자(왼정렬)
    s = lay["speed"]
    set_box(find(tree, f"{prefix}SpeedPlateMount"), s["x"], s["y"], s["w"], s["h"])
    set_brush(find(tree, f"{prefix}SpeedPlate"), "row_empty")
    cell = s["h"] * 0.56
    set_box(find(tree, f"{prefix}SpeedIcon"),
            s["x"] + s["w"] * 0.22, s["y"] + (s["h"] - cell) / 2, cell, cell)
    center = find(tree, f"{prefix}SpeedText_Center")
    if center is not None:
        overlay_pad(center, s["w"] * 0.22 + cell + 10, 0, 0, 0)
    text = find(tree, f"{prefix}SpeedText")
    style_text(text, s["h"], FONT_FACTOR["speed"], col=COL_NAME,
               fs=int(s.get("fs") or 0), face="Oswald")
    if text is not None:
        text.set_editor_property("justification", unreal.TextJustify.LEFT)

    # 치명타 칸 (확정 시안 0806). 확률 값이 게임 데이터에 아직 없어 "-" 로 둔다.
    panel_canvas = find(tree, f"{prefix}Panel")
    if "crit" in lay and isinstance(panel_canvas, unreal.CanvasPanel):
        k = lay["crit"]
        plate = ensure_image(tree, panel_canvas, f"{prefix}CritPlate",
                             k["x"], k["y"], k["w"], k["h"], z=12)
        plate_tex = texture_by_name((k.get("art") or {}).get("plate") or "")
        if plate_tex is not None:
            brush = plate.get_editor_property("brush")
            brush.set_editor_property("resource_object", plate_tex)
            brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
            plate.set_editor_property("brush", brush)
        else:
            set_brush(plate, "stats_strip")
        cell = k["h"] * 0.5
        ensure_image(tree, panel_canvas, f"{prefix}CritIcon",
                     k["x"] + k["w"] * 0.12, k["y"] + (k["h"] - cell) / 2,
                     cell, cell, "crit_gem", z=13)
        ensure_text(tree, panel_canvas, f"{prefix}CritText",
                    k["x"] + k["w"] * 0.12 + cell + 10, k["y"],
                    k["w"] * 0.86 - cell - 10, k["h"], "-",
                    round(k["h"] * 0.46 * 0.75), COL_NAME, face="Oswald",
                    z=14, justify="left")

    # 상태 아이콘 3칸 (+ 클릭 → 상태 상세용 투명 단추)
    r = lay["statusRow"]
    cell = min(r["h"], (r["w"] - 24) / 3.0)
    for i in range(3):
        cx = r["x"] + (cell + 12) * i
        cy = r["y"] + (r["h"] - cell) / 2
        set_box(find(tree, f"{prefix}StatusFrame_{i}Mount"), cx, cy, cell, cell)
        set_box(find(tree, f"{prefix}StatusIcon_{i}"),
                cx + cell * 0.15, cy + cell * 0.15, cell * 0.7, cell * 0.7)
        if isinstance(panel_canvas, unreal.CanvasPanel):
            ensure_button(tree, panel_canvas, f"{prefix}StatusButton_{i}",
                          cx, cy, cell, cell)

    # 명령 카드의 AP 젬 숫자가 배지(z50) 뒤로 가려졌다(0806 검수) --
    # 글자 칸이 판 마운트(z0) 안에 있어서다. 카드 캔버스로 꺼내 배지 위(z60)에.
    if prefix == "Ally":
        lifted = 0
        for i in range(6):
            card = find(tree, f"CommandCard_{i}")
            center = find(tree, f"CommandCost_{i}_Center")
            if center is None or not isinstance(card, unreal.CanvasPanel):
                continue
            if center.get_parent() != card:
                center.modify()
                center.get_parent().remove_child(center)
                card.add_child(center)
            set_box(center, 158.4, 61.3, 49.7, 54.8, z=60)
            lifted += 1
        say(f"    카드 AP 숫자 {lifted}칸 배지 위로")

    # 시안에서 뺀 부품들은 접는다 (배지·상태 라벨/글·아래 배너·옛 아이콘들)
    gone = [f"{prefix}BadgePlateMount", f"{prefix}StatusLabel_Center",
            f"{prefix}Status_Center"]
    gone += (["AllySummaryHint_Center"] if prefix == "Ally" else
             ["EnemyForecast_Center", "EnemyHPIcon", "EnemyDefenseIcon",
              "EnemyDamageIcon", "EnemyDefense_Center"])
    folded = [name for name in gone if collapse(tree, name)]
    say(f"    접음: {', '.join(folded)}")


def ensure_next_skill(tree, blueprint):
    """적 판에만 있는 다음 스킬 소켓. C++ 가 아이콘을 걸고 켜고 끈다."""
    box = LAYOUT["nextSkill"]
    panel = find(tree, "EnemyPanel")
    if not isinstance(panel, unreal.CanvasPanel):
        say("    !! EnemyPanel 캔버스 못 찾음")
        return
    for name, (x, y, w, h), tex_key in (
            ("EnemyNextSkillFrame",
             (box["x"], box["y"], box["w"], box["h"]), "socket"),
            ("EnemyNextSkillIcon",
             (box["x"] + box["w"] * 0.16, box["y"] + box["h"] * 0.16,
              box["w"] * 0.68, box["h"] * 0.68), None)):
        widget = find(tree, name)
        if widget is None:
            widget = unreal.new_object(unreal.Image, outer=tree, name=name)
            panel.add_child(widget)
        widget.modify()
        set_box(widget, x, y, w, h, z=30)
        if tex_key is not None:
            set_brush(widget, tex_key)
        # 기본은 접어 둔다 — 아이콘이 오면 C++ 가 켠다.
        widget.set_visibility(unreal.SlateVisibility.COLLAPSED)
    # 소켓 클릭 → 스킬 상세 (C++ 가 이름으로 배선). C++ 가 같이 켜고 끈다.
    button = ensure_button(tree, panel, "EnemyNextSkillButton",
                           box["x"], box["y"], box["w"], box["h"], z=35)
    button.set_visibility(unreal.SlateVisibility.COLLAPSED)
    say(f"    다음 스킬 소켓: {box['x']},{box['y']} {box['w']}x{box['h']}")


def apply_hud():
    blueprint = unreal.EditorAssetLibrary.load_asset(HUD)
    tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")
    say(HUD.rsplit("/", 1)[-1])
    blueprint.modify()
    for prefix in ("Ally", "Enemy"):
        apply_summary_panel(tree, prefix)
    ensure_next_skill(tree, blueprint)

    # 턴 칸을 누르면 그 유닛을 보게 한다(0806). 칸마다 투명 단추를 깐다 --
    # 카드를 펴는 손이 여기 하나로 모였으므로 눌리는 자리가 있어야 한다.
    made = 0
    for index in range(10):
        token = find(tree, f"TurnToken_{index}")
        token_slot = canvas_slot(token)
        parent = token.get_parent() if token is not None else None
        if token_slot is None or not isinstance(parent, unreal.CanvasPanel):
            continue
        offsets = token_slot.get_offsets()
        ensure_button(tree, parent, f"TurnTokenButton_{index}",
                      offsets.left, offsets.top, offsets.right, offsets.bottom,
                      z=40)
        made += 1
    say(f"    턴 칸 단추 {made}개")
    unreal.SystemLibrary.execute_console_command(
        None, f"RD.Editor.CleanWidgetVariables {HUD}")
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    say(f"  저장={unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)}")


def set_font(text_widget, pt, col=None, face=None):
    """여러 줄 글 상자용: 크기·색·글꼴만 잡고 여백은 안 건드린다."""
    if text_widget is None:
        return
    font = text_widget.get_editor_property("font")
    font.set_editor_property("size", pt)
    if face in FONTS:
        face_obj = unreal.load_object(None, FONTS[face])
        if face_obj is not None:
            font.set_editor_property("font_object", face_obj)
            font.set_editor_property("typeface_font_name", "Bold")
    text_widget.set_editor_property("font", font)
    if col is not None:
        text_widget.set_editor_property("color_and_opacity", unreal.SlateColor(col))


def walk_widgets(root):
    """위젯 트리를 전부 돈다 (패널류의 자식을 재귀로)."""
    stack = [root]
    while stack:
        node = stack.pop()
        if node is None:
            continue
        yield node
        if isinstance(node, unreal.PanelWidget):
            stack.extend(list(node.get_all_children()))


def apply_overlay():
    """상세 오버레이를 확정 시안 문법으로: 판은 GenericDetailPanel(짙은 슬레이트),
    양피지 기둥판은 걷고 내용을 슬레이트 위에 바로 얹는다. 글자는 밝은 색으로.
    타격범위 격자·차단 알약판 같은 기능 부품은 그대로 둔다."""
    blueprint = unreal.EditorAssetLibrary.load_asset(OVERLAY)
    tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")
    say(OVERLAY.rsplit("/", 1)[-1])
    blueprint.modify()
    set_brush(find(tree, "DetailFrameImage"), "panel")
    if collapse(tree, "DetailTitlePlateMount"):
        say("    제목판 접음 (띠가 판 그림에 있음)")
    set_brush(find(tree, "DetailIconFrame"), "socket")

    # 양피지 기둥판을 걷는다 — 시안은 짙은 슬레이트 바탕에 내용을 바로 얹는다.
    plates = ["DetailIdentityPlate", "DetailStatPlate", "DetailRightPlate",
              "DetailWidePlate"]
    folded = [name for name in plates if collapse(tree, name)]
    say(f"    양피지 판 접음: {', '.join(folded)}")

    # ── 확정 시안 4판: 편집기 스킬 상세 보드(800x520)를 2배 그대로 옮긴다.
    # 좌표는 summary_layout.json 의 sk* (사용자 조정 반영), 없으면 편집기 기본값.
    SK_DEFAULT = {
        "skPanel": {"x": 0, "y": 0, "w": 800, "h": 520},
        "skName": {"x": 260, "y": 34, "w": 280, "h": 56},
        "skIcon": {"x": 55, "y": 105, "w": 150, "h": 150},
        "skStats": {"x": 235, "y": 110, "w": 505, "h": 54},
        "skPlate": {"x": 225, "y": 172, "w": 525, "h": 106, "on": True},
        "skDesc": {"x": 235, "y": 180, "w": 505, "h": 92},
        "skDivider": {"x": 70, "y": 286, "w": 660, "h": 8},
        "skSelName": {"x": 70, "y": 302, "w": 200, "h": 26},
        "skGridSelect": {"x": 70, "y": 332, "w": 200, "h": 100},
        "skSelCaption": {"x": 70, "y": 436, "w": 200, "h": 24},
        "skHitName": {"x": 290, "y": 302, "w": 200, "h": 26},
        "skGridHit": {"x": 290, "y": 332, "w": 200, "h": 100},
        "skHitCaption": {"x": 290, "y": 436, "w": 200, "h": 24},
        "skBlockTitle": {"x": 510, "y": 302, "w": 230, "h": 26},
        "skBlockAim": {"x": 510, "y": 342, "w": 230, "h": 32},
        "skBlockEffect": {"x": 510, "y": 384, "w": 230, "h": 32},
        "skClose": {"x": 324, "y": 466, "w": 152, "h": 46},
    }
    sk = {key: LAYOUT.get(key, SK_DEFAULT[key]) for key in SK_DEFAULT}
    PX = (1920.0 - sk["skPanel"]["w"] * 2) / 2
    PY = (1080.0 - sk["skPanel"]["h"] * 2) / 2

    def bx2(box):
        return (PX + box["x"] * 2, PY + box["y"] * 2,
                box["w"] * 2, box["h"] * 2)

    def pt2(box, factor):
        px = int(box.get("fs") or 0) or round(box["h"] * factor)
        return round(px * 2 * 0.75)

    root_canvas = find(tree, "DetailPanelRoot")
    set_box(find(tree, "DetailFrameImage"), *bx2(sk["skPanel"]))

    # 제목: 접힌 제목판 마운트에서 꺼내 판 띠 자리에 직접
    title_center = find(tree, "DetailTitleText_Center")
    if title_center is not None and isinstance(root_canvas, unreal.CanvasPanel):
        if title_center.get_parent() != root_canvas:
            title_center.modify()
            title_center.get_parent().remove_child(title_center)
            root_canvas.add_child(title_center)
        set_box(title_center, *bx2(sk["skName"]), z=60)
        title_center.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    # 세로 가운데는 style_text 가 잡는다 (set_font 만 쓰면 위에 붙는다 - 0806 검수)
    style_text(find(tree, "DetailTitleText"), sk["skName"]["h"] * 2, 0.8,
               col=COL_NAME, fs=int(sk["skName"].get("fs") or 0) * 2,
               face="LINESeed")

    # 정체성 기둥을 판 전체로 펴고 내용(원점 37,25) 안에 절대좌표로
    set_box(find(tree, "DetailIdentityColumn"), 0, 0, 1920, 1080)
    content = find(tree, "DetailIdentityColumnContent")
    if content is not None:
        set_box(content, 37, 25, 1846, 1030)

    def in_content(x, y, w, h):
        return (x - 37, y - 25, w, h)

    # 아이콘 소켓 + 그림 (청록 틴트 제거)
    ix, iy, iw, ih = bx2(sk["skIcon"])
    set_box(find(tree, "DetailIconFrame"), *in_content(ix, iy, iw, ih))
    icon = find(tree, "DetailIconImage")
    set_box(icon, *in_content(ix + iw * 0.16, iy + ih * 0.16,
                              iw * 0.68, ih * 0.68))
    if icon is not None:
        icon.set_editor_property(
            "color_and_opacity", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))

    # 스탯 띠 + 요약줄 (짙은 글자)
    sx, sy, sw, shh = bx2(sk["skStats"])
    if isinstance(content, unreal.CanvasPanel):
        ensure_image(tree, content, "DetailStatsPlate",
                     *in_content(sx, sy, sw, shh), tex_key="stats_strip", z=-1)
    set_box(find(tree, "DetailSubtitleText_Center"), *in_content(sx, sy, sw, shh))
    subtitle = find(tree, "DetailSubtitleText")
    # 쿨타임까지 낀 긴 줄도 한 줄에 들어가는 크기 (편집기와 같은 0.34).
    set_font(subtitle, pt2(sk["skStats"], 0.34), col=COL_DARK, face="LINESeed")
    if subtitle is not None:
        subtitle.set_editor_property("justification", unreal.TextJustify.CENTER)
        subtitle.set_editor_property(
            "shadow_color_and_opacity", unreal.LinearColor(0, 0, 0, 0))
        # 한 줄 고정. 접히면 둘째 줄이 들여쓴 것처럼 보인다(0806).
        # auto_wrap_text 만 꺼도 wrap_text_at 이 0 보다 크면 거기서 접힌다.
        subtitle.set_editor_property("auto_wrap_text", False)
        subtitle.set_editor_property("wrap_text_at", 0.0)

    # 자유 판때기: 편집기에서 자리·에셋을 고른 받침판 (기본은 설명 뒤)
    if isinstance(root_canvas, unreal.CanvasPanel):
        plate_box = sk["skPlate"]
        free_plate = ensure_image(tree, root_canvas, "DetailFreePlate",
                                  *bx2(plate_box), z=3)
        plate_tex = texture_by_name(
            (plate_box.get("art") or {}).get("plate") or "")
        if plate_tex is not None:
            brush = free_plate.get_editor_property("brush")
            brush.set_editor_property("resource_object", plate_tex)
            brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
            free_plate.set_editor_property("brush", brush)
        else:
            set_brush(free_plate, "info_panel")
        if plate_box.get("on", True) is False:
            free_plate.set_visibility(unreal.SlateVisibility.COLLAPSED)
        else:
            free_plate.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)

    # 설명 (판 없이 밝은 글). 왼정렬 + 칸 밖으로 안 넘치게 자른다
    # (0806 검수: 가운데 정렬 긴 글이 격자 제목까지 흘렀다).
    collapse(tree, "DetailBodyHeading_Center")
    body_center = find(tree, "DetailBodyText_Center")
    set_box(body_center, *in_content(*bx2(sk["skDesc"])))
    if body_center is not None:
        body_center.set_editor_property(
            "clipping", unreal.WidgetClipping.CLIP_TO_BOUNDS)
    body_text = find(tree, "DetailBodyText")
    # 편집기 19px -> 게임 38px -> 28pt (0806: 글이 너무 작다는 검수)
    set_font(body_text,
             round((int(sk["skDesc"].get("fs") or 0) or 19) * 1.5),
             col=COL_LIGHT, face="LINESeed")
    if body_text is not None:
        body_text.set_editor_property("justification", unreal.TextJustify.LEFT)

    # 구분선 (C++ 가 켜는 위젯이라 이름 유지, 두 개 같은 자리)
    for name in ("DetailDivider_0", "DetailDivider_1"):
        divider = find(tree, name)
        set_box(divider, *bx2(sk["skDivider"]), z=55)
        set_brush(divider, "divider")

    # 격자(타일): 편집기 그리기 규칙 그대로 -- 제목 위, 5x5 칸, 밑에 설명 글
    right = find(tree, "DetailRightColumn")
    if right is not None:
        right.set_editor_property("render_opacity", 1.0)
        set_box(right, 0, 0, 1920, 1080)
    set_box(find(tree, "DetailRightColumnContent"), 37, 25, 1846, 1030)
    # 제목·타일·설명이 각각 따로 옮기는 부품이다(0806 요청).
    for kind, grid_key, name_key, caption_key in (
            ("Select", "skGridSelect", "skSelName", "skSelCaption"),
            ("Hit", "skGridHit", "skHitName", "skHitCaption")):
        gx, gy, _, _ = bx2(sk[grid_key])
        cs = min((sk[grid_key]["w"] - 4) / 5.0,
                 (sk[grid_key]["h"] - 4) / 5.0) * 2
        for i in range(5):
            for j in range(5):
                for suffix in ("Bg", ""):
                    cell = find(tree, f"Detail{kind}Cell_R{i}C{j}{suffix}")
                    if cell is None:
                        continue
                    set_box(cell, *in_content(gx + j * (cs + 2),
                                              gy + i * (cs + 2), cs, cs))
                    cell.set_visibility(
                        unreal.SlateVisibility.HIT_TEST_INVISIBLE)
        heading_box = sk[name_key]
        set_box(find(tree, f"Detail{kind}Heading_Center"),
                *in_content(*bx2(heading_box)))
        set_font(find(tree, f"Detail{kind}Heading"), pt2(heading_box, 0.78),
                 col=color("F0C479"), face="LINESeed")
        caption_box = sk[caption_key]
        set_box(find(tree, f"Detail{kind}CaptionText_Center"),
                *in_content(*bx2(caption_box)))
        set_font(find(tree, f"Detail{kind}CaptionText"), pt2(caption_box, 0.72),
                 col=COL_LIGHT, face="LINESeed")

    # 차단 규칙: 판 없이 글만 (0806 컨펌) -- 알약판 그림은 안 그린다.
    # 제목·두 줄이 각각 따로 옮기는 부품이다.
    set_box(find(tree, "DetailBlockerHeading_Center"),
            *in_content(*bx2(sk["skBlockTitle"])))
    set_font(find(tree, "DetailBlockerHeading"), pt2(sk["skBlockTitle"], 0.78),
             col=color("F0C479"), face="LINESeed")
    set_box(find(tree, "DetailAimBlockerPlateMount"),
            *in_content(*bx2(sk["skBlockAim"])))
    set_box(find(tree, "DetailEffectBlockerPlateMount"),
            *in_content(*bx2(sk["skBlockEffect"])))
    for name in ("DetailAimBlockerPlate", "DetailEffectBlockerPlate"):
        pill = find(tree, name)
        if pill is None:
            continue
        prop = "brush" if isinstance(pill, unreal.Image) else "background"
        brush = pill.get_editor_property(prop)
        brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.NO_DRAW_TYPE)
        pill.set_editor_property(prop, brush)
    for name in ("DetailAimBlockerLabel", "DetailAimBlockerText",
                 "DetailEffectBlockerLabel", "DetailEffectBlockerText"):
        text = find(tree, name)
        if text is not None:
            set_font(text, pt2(sk["skBlockAim"], 0.55), col=COL_LIGHT,
                     face="LINESeed")

    # 유닛 상세의 스킬 줄은 격자 자리에 (둘은 같이 안 나온다)
    gsx, gsy, _, _ = bx2(sk["skGridSelect"])
    set_box(find(tree, "DetailSkillRowHost"), *in_content(gsx, gsy, 1400, 100))

    # 수치 칩 기둥만 투명 (요약줄이 대신한다)
    stat_column = find(tree, "DetailStatColumn")
    if stat_column is not None:
        stat_column.set_editor_property("render_opacity", 0.0)

    # ── 아티팩트 상세: 편집기 ar* 배치 (효과 제목·설명·희귀도 보석)
    AR_DEFAULT = {
        "arName": {"x": 205, "y": 32, "w": 392, "h": 72},
        "arSlot": {"x": 97, "y": 122, "w": 160, "h": 170},
        "arRarity": {"x": 275, "y": 126, "w": 190, "h": 42},
        "arHeader": {"x": 276, "y": 175, "w": 130, "h": 36},
        "arDesc": {"x": 275, "y": 221, "w": 417, "h": 164},
    }
    ar = {key: LAYOUT.get(key, AR_DEFAULT[key]) for key in AR_DEFAULT}

    wide = find(tree, "DetailWideColumn")
    if wide is not None:
        wide.set_editor_property("render_opacity", 1.0)
        set_box(wide, 0, 0, 1920, 1080)
    set_box(find(tree, "DetailWideColumnContent"), 37, 25, 1846, 1030)
    set_box(find(tree, "DetailExtraBlock"), 0, 0, 1920, 1080)
    set_box(find(tree, "DetailExtraHeading_Center"), *in_content(*bx2(ar["arHeader"])))
    set_font(find(tree, "DetailExtraHeading"), pt2(ar["arHeader"], 0.6),
             col=color("F0C479"), face="LINESeed")
    extra_center = find(tree, "DetailExtraText_Center")
    set_box(extra_center, *in_content(*bx2(ar["arDesc"])))
    if extra_center is not None:
        extra_center.set_editor_property(
            "clipping", unreal.WidgetClipping.CLIP_TO_BOUNDS)
    extra_text = find(tree, "DetailExtraText")
    set_font(extra_text, 22, col=COL_LIGHT, face="LINESeed")
    if extra_text is not None:
        extra_text.set_editor_property("justification", unreal.TextJustify.LEFT)

    # 희귀도 보석 5칸 (C++ 가 등급만큼 켜고 나머지는 어둡게)
    extra_block = find(tree, "DetailExtraBlock")
    if isinstance(extra_block, unreal.CanvasPanel):
        gx, gy, gw, gh = bx2(ar["arRarity"])
        gem = min(gh, (gw - 4 * 12) / 5.0)
        for index in range(5):
            widget = ensure_image(tree, extra_block, f"DetailRarityGem_{index}",
                                  *in_content(gx + (gem + 12) * index, gy,
                                              gem, gem),
                                  tex_key="crit_gem", z=20)
            widget.set_visibility(unreal.SlateVisibility.COLLAPSED)

    # 닫기 단추 (C++ 가 DetailCloseButton 이름으로 배선) + 어디든 눌러 닫는 받이
    if isinstance(root_canvas, unreal.CanvasPanel):
        cx, cy, cw, chh = bx2(sk["skClose"])
        ensure_image(tree, root_canvas, "DetailCloseArt",
                     cx, cy, cw, chh, "btn_wide", z=56)
        ensure_button(tree, root_canvas, "DetailCloseButton",
                      cx, cy, cw, chh, z=57)
        ensure_text(tree, root_canvas, "DetailCloseText", cx, cy, cw, chh,
                    "닫기", pt2(sk["skClose"], 0.42), COL_NAME, z=58)
        ensure_button(tree, root_canvas, "DetailCloseCatch", 0, 0, 1920, 1080, z=-1)
    say("    확정 시안 4판 (편집기 sk 배치 x2 · 타일/차단 글만 · 닫기 단추)")

    # 짙은 갈색 글자는 양피지용이었다. 슬레이트 위에서는 밝은 글자여야 읽힌다.
    # 차단 알약판(금색 판) 위 글자는 그대로 짙게 둔다.
    # (WidgetTree 는 파이썬에 root_widget 을 안 내놓아 이름으로 뿌리를 잡는다.)
    root = find(tree, "RootCanvas") or find(tree, "DetailPanelRoot")
    recolored = 0
    for widget in walk_widgets(root):
        if not isinstance(widget, unreal.TextBlock):
            continue
        # 양피지 띠 위 요약줄(짙은 글)과 금색 소제목들은 그대로 둔다.
        if widget.get_name() in ("DetailSubtitleText", "DetailSelectHeading",
                                 "DetailHitHeading", "DetailBlockerHeading"):
            continue
        widget.set_editor_property("color_and_opacity", unreal.SlateColor(COL_LIGHT))
        widget.set_editor_property(
            "shadow_color_and_opacity", unreal.LinearColor(0, 0, 0, 0.6))
        recolored += 1
    say(f"    글자 밝게: {recolored}개")

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    say(f"  저장={unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)}")


def place(widget, mount, x, y, w, h):
    """부모(mount 상자) 안 부품을 절대좌표 (x,y,w,h) 자리로.

    오버레이 자식이면 mount 기준 패딩으로, 캔버스 자식이면 절대좌표로 놓는다.
    set_box 의 600x430 폴백은 요약판 전용이라 여기서는 쓰면 안 된다.
    """
    if widget is None:
        return
    mx, my, mw, mh = mount
    slot = widget.get_editor_property("slot")
    if isinstance(slot, unreal.OverlaySlot):
        overlay_pad(widget, x - mx, y - my, (mx + mw) - (x + w), (my + mh) - (y + h))
    else:
        set_box(widget, x, y, w, h)


def ensure_image(tree, panel, name, x, y, w, h, tex_key=None, z=0):
    """캔버스에 이름 붙은 Image 를 (없으면 만들어) 자리에 놓는다."""
    widget = find(tree, name)
    if widget is None:
        widget = unreal.new_object(unreal.Image, outer=tree, name=name)
        panel.add_child(widget)
    widget.modify()
    set_box(widget, x, y, w, h, z=z)
    if tex_key is not None:
        set_brush(widget, tex_key)
    return widget


def ensure_text(tree, panel, name, x, y, w, h, text, pt, col, face="LINESeed",
                z=20, justify="center", shadow=True):
    """캔버스에 이름 붙은 TextBlock 을 (없으면 만들어) 놓고 꾸민다.

    글은 칸 위에 붙어 그려지므로(0806 검수: 닫기 글자가 떠 보임) 칸 높이와
    줄상자 차이만큼 y 를 내려 세로 가운데를 맞춘다.
    """
    widget = find(tree, name)
    if widget is None:
        widget = unreal.new_object(unreal.TextBlock, outer=tree, name=name)
        panel.add_child(widget)
    widget.modify()
    line_h = pt * (96.0 / 72.0) * 1.45
    pad = max(0.0, (h - line_h) / 2.0)
    set_box(widget, x, y + pad, w, h - pad, z=z)
    widget.set_editor_property("text", text)
    set_font(widget, pt, col=col, face=face)
    widget.set_editor_property("justification", {
        "center": unreal.TextJustify.CENTER,
        "left": unreal.TextJustify.LEFT,
        "right": unreal.TextJustify.RIGHT}[justify])
    if not shadow:
        widget.set_editor_property(
            "shadow_color_and_opacity", unreal.LinearColor(0, 0, 0, 0))
    widget.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    return widget


def ensure_button(tree, panel, name, x, y, w, h, z=40):
    """소켓 위에 까는 투명 클릭 단추. 그림은 안 그리고 눌림만 받는다."""
    widget = find(tree, name)
    if widget is None:
        widget = unreal.new_object(unreal.Button, outer=tree, name=name)
        panel.add_child(widget)
    widget.modify()
    set_box(widget, x, y, w, h, z=z)
    style = widget.get_editor_property("widget_style")
    for state in ("normal", "hovered", "pressed", "disabled"):
        brush = style.get_editor_property(state)
        brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.NO_DRAW_TYPE)
        style.set_editor_property(state, brush)
    widget.set_editor_property("widget_style", style)
    return widget


def apply_merc_tab():
    """보유 용병 상세탭(HUD04 MercenaryPanel)을 확정 시안(mc*)으로 재배치한다.

    편집기 판(1920x1080)과 보드 좌표계가 같아 1:1 로 옮긴다. 로스터 카드
    내부와 스킬 배선(C++)은 이름을 안 바꾸므로 그대로 산다. 용병별 EXP 는
    아직 배선할 값이 없어 이번 판에는 안 넣는다.
    """
    # 편집기 기본 배치 (저장본에 없는 키는 이걸로 -- 부품을 갈아 끼우면
    # 옛 키가 사라지므로 여기서 받쳐 준다).
    MC_DEFAULT = {
        "mcPanel": {"x": 0, "y": 0, "w": 1920, "h": 1080},
        "mcTitle": {"x": 660, "y": 22, "w": 600, "h": 64},
        # 판 테두리 안쪽 기준 (0806: 85,115 는 제목띠 위로 튀어나갔다)
        "mcRoster": {"x": 300, "y": 270, "w": 330, "h": 380},
        "mcPortrait": {"x": 570, "y": 118, "w": 335, "h": 360},
        "mcName": {"x": 935, "y": 140, "w": 565, "h": 92},
        "mcStats": {"x": 935, "y": 296, "w": 530, "h": 214},
        "mcSkillHeader": {"x": 570, "y": 540, "w": 930, "h": 32},
        "mcSkills": {"x": 570, "y": 580, "w": 930, "h": 118},
        "mcClose": {"x": 810, "y": 900, "w": 300, "h": 92},
    }
    lay = {key: LAYOUT.get(key, MC_DEFAULT[key]) for key in MC_DEFAULT}
    blueprint = unreal.EditorAssetLibrary.load_asset(HUD)
    tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")
    say("MercenaryPanel (보유 용병 상세)")
    blueprint.modify()

    # 판 바탕: 바깥 틀을 시안 판으로
    set_brush(find(tree, "RuntimeMercenaryRosterShell"), "panel")

    # 제목 -- 위 나무띠 안 (헤더 섹션 40,30 좌표계)
    t = lay["mcTitle"]
    set_box(find(tree, "MercenaryTitleText_Center"),
            t["x"] - 40, t["y"] - 30, t["w"], t["h"])
    style_text(find(tree, "MercenaryTitleText"), t["h"], 0.72,
               col=COL_NAME, fs=int(t.get("fs") or 0), face="LINESeed")

    # 닫기 -- 파티 편성 대신, 아래 가운데로 (기존 닫기 단추를 옮겨 쓴다).
    # 헤더 구역(높이 130) 안에 둔 채 좌표만 내리면 구역 밖이라 클릭이 잘린다
    # (0806 검수: 닫기가 안 눌림). 보드 캔버스로 꺼내 절대좌표에 앉힌다.
    c = lay["mcClose"]
    board_canvas = find(tree, "MercenaryBoard")
    close_box = (c["x"], c["y"], c["w"], c["h"])
    for name in ("MercenaryBackArtMount", "MercenaryCloseButton",
                 "MercenaryCloseText_Center"):
        widget = find(tree, name)
        if widget is None or not isinstance(board_canvas, unreal.CanvasPanel):
            continue
        if isinstance(widget.get_parent(), unreal.Button):
            continue   # 단추 내용물이면 단추와 같이 움직인다
        if widget.get_parent() != board_canvas:
            widget.modify()
            widget.get_parent().remove_child(widget)
            board_canvas.add_child(widget)
        set_box(widget, *close_box, z=60)
    # 글 칸이 단추 위에 형제로 얹히면 클릭을 먹는다. 보기만 하게 한다.
    text_center = find(tree, "MercenaryCloseText_Center")
    if text_center is not None and not isinstance(
            text_center.get_parent(), unreal.Button):
        text_center.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    set_brush(find(tree, "MercenaryBackArt"), "btn_wide")
    close_text = find(tree, "MercenaryCloseText")
    style_text(close_text, c["h"], 0.42, col=COL_NAME, face="LINESeed")
    if close_text is not None:
        close_text.set_editor_property("text", "닫기")

    # 목록 -- 3장 세로. 편집기 mcRoster 그리기와 같게: 초상 · 이름 · Lv 배지.
    r = lay["mcRoster"]
    set_box(find(tree, "MercRosterSection"), r["x"], r["y"], r["w"], r["h"])
    card_h = (r["h"] - 28) / 3.0
    for i in range(3):
        scale_box = find(tree, f"MercenaryCardScale_{i}")
        set_box(scale_box, 0, i * (card_h + 14), r["w"], card_h)
        # 카드 속 좌표를 줄 크기 그대로 쓰려면 늘려 채우기여야 한다.
        # 맞춰 넣기면 옛 카드 비율(350x190)로 줄어들어 글자만 작아진다.
        if scale_box is not None:
            scale_box.set_editor_property("stretch", unreal.Stretch.FILL)
        card = find(tree, f"PartyCard_{i}")
        content = find(tree, f"PartyContent_{i}")
        if not isinstance(card, unreal.CanvasPanel):
            continue
        set_box(card, 0, i * (card_h + 14), r["w"], card_h)
        set_box(find(tree, f"PartyPlate_{i}Mount"), 0, 0, r["w"], card_h)
        set_box(find(tree, f"PartyButton_{i}"), 0, 0, r["w"], card_h)
        # 판보다 내용이 위로 오게 (안 그러면 고른 줄 판이 이름을 덮는다 - 0806)
        set_box(find(tree, f"PartyPlate_{i}Mount"), 0, 0, r["w"], card_h, z=0)
        set_box(find(tree, f"PartyPlate_{i}"), 0, 0, r["w"], card_h, z=0)
        if isinstance(content, unreal.CanvasPanel):
            set_box(content, 0, 0, r["w"], card_h, z=10)
            cell = card_h * 0.76
            set_box(find(tree, f"PartyPortrait_{i}"),
                    16, (card_h - cell) / 2, cell, cell)
            # 이름 칸은 Lv 배지 앞에서 끊는다 (겹치면 글자가 배지를 파고든다)
            place(find(tree, f"PartyName_{i}_Center"), (0, 0, r["w"], card_h),
                  cell + 26, 0, r["w"] - cell - 26 - 84, card_h)
            name_text = find(tree, f"PartyName_{i}")
            # 양피지 줄판 위라 흰 글자 + 검은 테두리 (0806 요청)
            style_text(name_text, card_h, 0.26, col=color("FFFFFF"),
                       face="LINESeed")
            outline_text(name_text, 2)
            if name_text is not None:
                name_text.set_editor_property(
                    "justification", unreal.TextJustify.LEFT)
            # Lv 배지: 내용 묶음 안에 둬야 빈 칸에서 같이 접힌다.
            ensure_image(tree, content, f"PartyLevelPlate_{i}",
                         r["w"] - 92, card_h / 2 - 17, 76, 34, "btn_small", z=20)
            level_text = ensure_text(tree, content, f"PartyLevel_{i}",
                                     r["w"] - 92, card_h / 2 - 17, 76, 34,
                                     "Lv 1", 14, color("FFFFFF"),
                                     face="Oswald", z=21)
            outline_text(level_text, 2)

    # 상세 섹션을 판 전체 좌표계로 펴고, 부품을 시안 자리로
    set_box(find(tree, "MercDetailSection"), 0, 0, 1920, 1080)
    board = find(tree, "MercenaryBoard")
    detail = find(tree, "MercDetailSection")

    # 초상: 틀을 새로 깔고 큰 히어로 그림을 틀 안에
    p = lay["mcPortrait"]
    ensure_image(tree, board, "MercenaryPortraitFrame",
                 p["x"], p["y"], p["w"], p["h"], "kita_portrait", z=5)
    set_box(find(tree, "MercenaryHeroPortrait"),
            p["x"] + p["w"] * 0.13, p["y"] + p["h"] * 0.13,
            p["w"] * 0.74, p["h"] * 0.74, z=6)

    # 이름판
    n = lay["mcName"]
    ensure_image(tree, detail, "MercenaryNamePlate",
                 n["x"], n["y"], n["w"], n["h"], "name_plate", z=2)
    set_box(find(tree, "MercenaryDetailName_Center"), n["x"], n["y"], n["w"], n["h"])
    name_text = find(tree, "MercenaryDetailName")
    style_text(name_text, n["h"], 0.5, col=color("FFFFFF"), face="LINESeed")
    outline_text(name_text, 2)
    slot = canvas_slot(find(tree, "MercenaryDetailName_Center"))
    if slot is not None:
        slot.set_z_order(3)

    # 스탯 4줄 (HP 하트 · AP 파란 젬 · 속도 날개 · 치명타 젬 "-")
    st = lay["mcStats"]
    row_h = (st["h"] - 24) / 4.0
    stat_rows = [("MercenaryChip0Frame", "MercenaryChip0Label_Center",
                  "MercenaryChip0Label", "MercenaryDetailHP_Center",
                  "MercenaryDetailHP", "hp_icon"),
                 ("MercenaryChip1Frame", "MercenaryChip1Label_Center",
                  "MercenaryChip1Label", "MercenaryDetailAP_Center",
                  "MercenaryDetailAP", "ap_gem"),
                 ("MercenaryChip2Frame", "MercenaryChip2Label_Center",
                  "MercenaryChip2Label", "MercenaryDetailSpeed_Center",
                  "MercenaryDetailSpeed", "speed_icon")]
    for i, (frame, label_c, label, value_c, value, icon) in enumerate(stat_rows):
        y = st["y"] + i * (row_h + 8)
        row_box = (st["x"], y, st["w"], row_h)
        frame_widget = find(tree, frame)
        set_box(find(tree, f"{frame}Mount"), st["x"], y, st["w"], row_h)
        set_brush(frame_widget, "stats_strip")
        ensure_image(tree, detail, f"MercenaryStatIcon_{i}",
                     st["x"] + 20, y + row_h * 0.18,
                     row_h * 0.64, row_h * 0.64, icon, z=20)
        place(find(tree, label_c), row_box,
              st["x"] + 20 + row_h * 0.64 + 16, y, 120, row_h)
        style_text(find(tree, label), row_h, 0.44, col=color("FFFFFF"),
                   face="LINESeed")
        outline_text(find(tree, label), 2)
        place(find(tree, value_c), row_box,
              st["x"] + st["w"] * 0.42, y, st["w"] * 0.3, row_h)
        style_text(find(tree, value), row_h, 0.44, col=color("FFFFFF"),
                   face="Oswald")
        outline_text(find(tree, value), 2)

    # 4번째 줄: 치명타 (확률 데이터가 아직 없어 "-" 고정, 배선은 추후)
    crit_y = st["y"] + 3 * (row_h + 8)
    crit_plate = ensure_image(tree, detail, "MercenaryCritPlate",
                              st["x"], crit_y, st["w"], row_h, z=3)
    set_brush(crit_plate, "stats_strip")
    ensure_image(tree, detail, "MercenaryStatIcon_3",
                 st["x"] + 20, crit_y + row_h * 0.18,
                 row_h * 0.64, row_h * 0.64, "crit_gem", z=20)
    outline_text(ensure_text(tree, detail, "MercenaryCritLabel",
                 st["x"] + 20 + row_h * 0.64 + 16, crit_y, 140, row_h,
                 "치명타", round(row_h * 0.44 * 0.75), color("FFFFFF"),
                 z=20, justify="left"), 2)
    outline_text(ensure_text(tree, detail, "MercenaryCritValue",
                 st["x"] + st["w"] * 0.42, crit_y, st["w"] * 0.3, row_h,
                 "-", round(row_h * 0.44 * 0.75), color("FFFFFF"),
                 face="Oswald", z=20, justify="left"), 2)

    # "스킬" 구분선 + 제목
    sh = lay["mcSkillHeader"]
    ensure_image(tree, detail, "MercenarySkillDivider",
                 sh["x"], sh["y"] + sh["h"] / 2 - 4, sh["w"], 8, "divider", z=2)
    set_box(find(tree, "MercenarySkillHeading_Center"),
            sh["x"], sh["y"], sh["w"], sh["h"])
    style_text(find(tree, "MercenarySkillHeading"), sh["h"], 0.6,
               col=color("F0C479"), face="LINESeed")
    slot = canvas_slot(find(tree, "MercenarySkillHeading_Center"))
    if slot is not None:
        slot.set_z_order(3)

    # 스킬 6칸 -- 두 줄(3x2)에서 한 줄로, 원형 소켓
    sk = lay["mcSkills"]
    cell = min(sk["h"], (sk["w"] - 5 * 24) / 6.0)
    step = (sk["w"] - cell) / 5.0
    for i in range(6):
        x = sk["x"] + step * i
        y = sk["y"] + (sk["h"] - cell) / 2.0
        cell_box = (x, y, cell, cell)
        set_box(find(tree, f"MercenarySkillFrame_{i}Mount"), x, y, cell, cell)
        set_brush(find(tree, f"MercenarySkillFrame_{i}"), "socket")
        set_box(find(tree, f"MercenarySkillIcon_{i}Mount"),
                x + cell * 0.16, y + cell * 0.16, cell * 0.68, cell * 0.68)
        place(find(tree, f"MercenarySkillCost_{i}_Center"), cell_box,
              x + cell * 0.7, y + cell * 0.04, cell * 0.26, cell * 0.26)
        # 작은 소켓에는 이름 글이 안 들어간다 -- 시안대로 아이콘만.
        collapse(tree, f"MercenarySkillName_{i}_Center")
        # 소켓 클릭 → 스킬 상세 (C++ 가 이름으로 찾아 배선한다)
        ensure_button(tree, detail, f"MercenarySkillButton_{i}", x, y, cell, cell)

    unreal.SystemLibrary.execute_console_command(
        None, f"RD.Editor.CleanWidgetVariables {HUD}")
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    say(f"  저장={unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)}")


UNIT_HP_BAR = "/Game/UI/CombatHUD/UnitHpBar/WBP_CombatUnitHpBar"
MONSTER = "/Game/UI/MonsterTab/WBP_MonsterTab_Marchbound"


def apply_unit_hp_bar():
    """머리 위 HP바 밑에 상태 띠 두 칸을 깐다 (왼쪽 이로움 · 오른쪽 해로움).

    자리·크기·색은 C++ 가 매 틱 잡는다. 판은 칸만 만들어 둔다 --
    없으면 C++ 가 찾지 못해 아무것도 안 그린다.
    """
    blueprint = unreal.EditorAssetLibrary.load_asset(UNIT_HP_BAR)
    if blueprint is None:
        say("UnitHpBar WBP 못 찾음 -- 건너뜀")
        return
    tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")
    say(UNIT_HP_BAR.rsplit("/", 1)[-1])
    blueprint.modify()
    canvas = find(tree, "RootCanvas")
    if not isinstance(canvas, unreal.CanvasPanel):
        say("    !! 뿌리 캔버스 못 찾음")
        return
    for index, name in enumerate(("HpStatusRailBuff", "HpStatusRailDebuff")):
        rail = ensure_image(tree, canvas, name,
                            20 + index * 160, 66, 160, 9, z=30)
        # 단색 띠 -- 그림 없이 상자로 그린다. 색은 C++ 가 물들인다.
        brush = rail.get_editor_property("brush")
        brush.set_editor_property("resource_object", None)
        brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.BOX)
        rail.set_editor_property("brush", brush)
        rail.set_visibility(unreal.SlateVisibility.COLLAPSED)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    say(f"  저장={unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)}")

# 편집기 기본 배치 (summary_layout.json 에 mt* 가 아직 없으면 이걸 쓴다)
MT_DEFAULT = {
    "mtPanel":   {"x": 0,   "y": 0,   "w": 1920, "h": 1080},
    "mtTitle":   {"x": 660, "y": 22,  "w": 600,  "h": 64},
    "mtRoster":  {"x": 300, "y": 270, "w": 330,  "h": 380},
    "mtPortrait": {"x": 570, "y": 118, "w": 335, "h": 360},
    "mtName":    {"x": 935, "y": 140, "w": 565,  "h": 92},
    "mtStats":   {"x": 935, "y": 296, "w": 530,  "h": 214},
    "mtSkillHeader": {"x": 570, "y": 540, "w": 930, "h": 32},
    "mtSkills":  {"x": 570, "y": 580, "w": 930,  "h": 118},
    "mtClose":   {"x": 810, "y": 900, "w": 300,  "h": 92},
}


def apply_monster_tab():
    """몬스터 탭을 용병탭과 같은 문법으로 재배치한다 (mt* 배치)."""
    lay = {key: LAYOUT.get(key, MT_DEFAULT[key]) for key in MT_DEFAULT}
    blueprint = unreal.EditorAssetLibrary.load_asset(MONSTER)
    if blueprint is None:
        say("MonsterTab WBP 못 찾음 -- 건너뜀")
        return
    tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")
    say(MONSTER.rsplit("/", 1)[-1])
    blueprint.modify()
    canvas = find(tree, "MonsterTabCanvas")

    # 판 바탕 + 옛 세로 기둥 정리
    set_brush(find(tree, "MonsterTabBaseFrame"), "panel")
    for name in ("MonsterTabDivider_0", "MonsterTabDivider_1",
                 "MonsterListHeading_Center", "MonsterCenterNameText_Center",
                 "MonsterStatusHeading_Center", "MonsterDetailHPBar"):
        collapse(tree, name)

    # 제목: 위 나무띠 안
    t = lay["mtTitle"]
    set_box(find(tree, "MonsterTabTitleText_Center"),
            t["x"], t["y"], t["w"], t["h"], z=60)
    style_text(find(tree, "MonsterTabTitleText"), t["h"], 0.72,
               col=COL_NAME, face="LINESeed")

    # 목록 3줄 + 줄마다 Lv 배지 (용병 목록과 같은 문법, 값은 C++ 가 적는다)
    r = lay["mtRoster"]
    row_h = (r["h"] - 28) / 3.0
    for i in range(3):
        set_box(find(tree, f"MonsterRow_{i}"),
                r["x"], r["y"] + i * (row_h + 14), r["w"], row_h)
        row = find(tree, f"MonsterRow_{i}")
        if isinstance(row, unreal.CanvasPanel):
            # 줄 안의 부품들: 초상 왼쪽 · 이름 가운데 · Lv 배지 오른쪽
            # (편집기 mtRoster 그리기와 같은 비율)
            cell = row_h * 0.76
            set_box(find(tree, f"MonsterRowNormal_{i}Mount"), 0, 0, r["w"], row_h,
                    z=0)
            set_box(find(tree, f"MonsterRowSelected_{i}"), 0, 0, r["w"], row_h,
                    z=1)
            # 초상·이름은 판(고른 줄 포함)보다 위로 (0806: 글이 판 뒤로 갔다)
            set_box(find(tree, f"MonsterRowPortrait_{i}"),
                    16, (row_h - cell) / 2, cell, cell, z=10)
            # 이름 칸은 줄 마운트(Overlay) 안이라 판 뒤로 간다. 줄 캔버스로 꺼낸다.
            name_center = find(tree, f"MonsterRowName_{i}_Center")
            if name_center is not None and name_center.get_parent() != row:
                name_center.modify()
                name_center.get_parent().remove_child(name_center)
                row.add_child(name_center)
            set_box(name_center, cell + 26, 0, r["w"] - cell - 26 - 84, row_h,
                    z=11)
            name_text = find(tree, f"MonsterRowName_{i}")
            style_text(name_text, row_h, 0.26, col=color("FFFFFF"),
                       face="LINESeed")
            outline_text(name_text, 2)
            if name_text is not None:
                name_text.set_editor_property(
                    "justification", unreal.TextJustify.LEFT)
            ensure_image(tree, row, f"MonsterRowLevelPlate_{i}",
                         r["w"] - 92, row_h / 2 - 17, 76, 34, "btn_small", z=20)
            level_text = ensure_text(tree, row, f"MonsterRowLevel_{i}",
                                     r["w"] - 92, row_h / 2 - 17, 76, 34,
                                     "Lv 1", 14, color("FFFFFF"),
                                     face="Oswald", z=21)
            outline_text(level_text, 2)

    # 초상: 틀 + 그림 (틀 안 74%)
    p = lay["mtPortrait"]
    if isinstance(canvas, unreal.CanvasPanel):
        ensure_image(tree, canvas, "MonsterPortraitFrame",
                     p["x"], p["y"], p["w"], p["h"], "kita_portrait", z=5)
    for name in ("MonsterDetailPortraitScale",):
        set_box(find(tree, name), p["x"] + p["w"] * 0.13, p["y"] + p["h"] * 0.13,
                p["w"] * 0.74, p["h"] * 0.74, z=6)

    # 이름판 + 레벨 줄 (레벨은 C++ 가 TypeText 에 적는다)
    n = lay["mtName"]
    if isinstance(canvas, unreal.CanvasPanel):
        ensure_image(tree, canvas, "MonsterNamePlate",
                     n["x"], n["y"], n["w"], n["h"], "name_plate", z=2)
    set_box(find(tree, "MonsterDetailNameText_Center"),
            n["x"], n["y"], n["w"], n["h"], z=3)
    style_text(find(tree, "MonsterDetailNameText"), n["h"], 0.5,
               col=color("FFFFFF"), face="LINESeed")
    outline_text(find(tree, "MonsterDetailNameText"), 2)
    set_box(find(tree, "MonsterDetailTypeText_Center"),
            n["x"], n["y"] + n["h"] + 8, n["w"], 40, z=3)
    type_text = find(tree, "MonsterDetailTypeText")
    style_text(type_text, 40, 0.6, col=COL_LIGHT, face="LINESeed")
    if type_text is not None:
        type_text.set_editor_property("justification", unreal.TextJustify.LEFT)

    # 스탯 4줄 (HP 하트 · AP 젬 · 속도 날개 · 치명타 젬). 방어 줄은 뺐다(0806).
    # 치명타는 옛 방어 칩(2번)을 재활용한다 -- 값은 데이터가 생길 때까지 "-".
    st = lay["mtStats"]
    row = (st["h"] - 24) / 4.0
    stat_rows = [("MonsterChip0Frame", "MonsterChip0Label_Center", "MonsterChip0Label",
                  "MonsterDetailHPText_Center", "MonsterDetailHPText", "hp_icon"),
                 ("MonsterChip1Frame", "MonsterChip1Label_Center", "MonsterChip1Label",
                  "MonsterDetailAPText_Center", "MonsterDetailAPText", "ap_gem"),
                 ("MonsterChip3Frame", "MonsterChip3Label_Center", "MonsterChip3Label",
                  "MonsterDetailSpeedText_Center", "MonsterDetailSpeedText",
                  "speed_icon"),
                 ("MonsterChip2Frame", "MonsterChip2Label_Center", "MonsterChip2Label",
                  "MonsterDetailDefenseText_Center", "MonsterDetailDefenseText",
                  "crit_gem")]
    for i, (frame, label_c, label, value_c, value, icon) in enumerate(stat_rows):
        y = st["y"] + i * (row + 8)
        row_box = (st["x"], y, st["w"], row)
        set_box(find(tree, f"{frame}Mount"), st["x"], y, st["w"], row)
        set_brush(find(tree, frame), "stats_strip")
        if isinstance(canvas, unreal.CanvasPanel):
            ensure_image(tree, canvas, f"MonsterStatIcon_{i}",
                         st["x"] + 18, y + row * 0.18, row * 0.64, row * 0.64,
                         icon, z=20)
        place(find(tree, label_c), row_box,
              st["x"] + 18 + row * 0.64 + 14, y, 120, row)
        style_text(find(tree, label), row, 0.5, col=color("FFFFFF"),
                   face="LINESeed")
        outline_text(find(tree, label), 2)
        place(find(tree, value_c), row_box, st["x"] + st["w"] * 0.45, y,
              st["w"] * 0.3, row)
        style_text(find(tree, value), row, 0.5, col=color("FFFFFF"),
                   face="Oswald")
        outline_text(find(tree, value), 2)

    # 옛 방어 칩을 치명타로: 라벨과 값을 판에서 고쳐 둔다 (C++ 는 안 건드림)
    crit_label = find(tree, "MonsterChip2Label")
    if crit_label is not None:
        crit_label.set_editor_property("text", "치명타")
    crit_value = find(tree, "MonsterDetailDefenseText")
    if crit_value is not None:
        crit_value.set_editor_property("text", "-")

    # 상태 두 칸: 스탯 밑에 작게 (C++ 가 켜고 끈다). 스킬 구분선과 안 붙게 띄운다.
    set_box(find(tree, "MonsterStatusText_0_Center"),
            st["x"], st["y"] + st["h"] + 14, 260, 36)
    set_box(find(tree, "MonsterStatusText_1_Center"),
            st["x"] + 270, st["y"] + st["h"] + 14, 260, 36)
    for i in range(2):
        style_text(find(tree, f"MonsterStatusText_{i}"), 36, 0.55,
                   col=COL_LIGHT, face="LINESeed")

    # "스킬" 구분선 + 소켓 4칸.
    # 제목 글은 선 위에 겹치므로, 편집기처럼 글 뒤에 판 바탕색 조각을 깐다.
    sh = lay["mtSkillHeader"]
    if isinstance(canvas, unreal.CanvasPanel):
        ensure_image(tree, canvas, "MonsterSkillDivider",
                     sh["x"], sh["y"] + sh["h"] / 2 - 4, sh["w"], 8, "divider", z=2)
        gap = ensure_image(tree, canvas, "MonsterSkillHeadingGap",
                           sh["x"] + sh["w"] / 2 - 70, sh["y"],
                           140, sh["h"], z=3)
        gap_brush = gap.get_editor_property("brush")
        gap_brush.set_editor_property("resource_object", None)
        gap_brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.BOX)
        gap_brush.set_editor_property("tint_color", unreal.SlateColor(color("1C150E")))
        gap.set_editor_property("brush", gap_brush)
        gap.set_editor_property("color_and_opacity", color("1C150E"))
    set_box(find(tree, "MonsterSkillHeading_Center"),
            sh["x"], sh["y"], sh["w"], sh["h"], z=4)
    style_text(find(tree, "MonsterSkillHeading"), sh["h"], 0.6,
               col=color("F0C479"), face="LINESeed")
    sk = lay["mtSkills"]
    cell = min(sk["h"], (sk["w"] - 3 * 24) / 4.0)
    step = (sk["w"] - cell) / 3.0
    for i in range(4):
        x = sk["x"] + step * i
        y = sk["y"] + (sk["h"] - cell) / 2.0
        set_box(find(tree, f"MonsterSkillSlot_{i}"), x, y, cell, cell)
        set_brush(find(tree, f"MonsterSkillSlot_{i}"), "socket")
        set_box(find(tree, f"MonsterSkillIcon_{i}Mount"),
                x + cell * 0.16, y + cell * 0.16, cell * 0.68, cell * 0.68)
        collapse(tree, f"MonsterSkillName_{i}_Center")

    # 닫기: 아래 가운데, 넓은 단추 + "닫기".
    # 단추·그림·글자는 모두 마운트(Overlay) 안 형제다. 마운트만 옮기고
    # 안쪽은 패딩 0 으로 채워야 눌림 자리가 그림과 맞는다.
    c = lay["mtClose"]
    set_box(find(tree, "MonsterBackArtMount"), c["x"], c["y"], c["w"], c["h"], z=60)
    for name in ("MonsterBackButton", "MonsterBackArt", "MonsterBackText_Center"):
        overlay_pad(find(tree, name), 0, 0, 0, 0)
    set_brush(find(tree, "MonsterBackArt"), "btn_wide")
    back_text = find(tree, "MonsterBackText")
    style_text(back_text, c["h"], 0.42, col=COL_NAME, face="LINESeed")
    if back_text is not None:
        back_text.set_editor_property("text", "닫기")
    # 글·그림은 눌림을 삼키지 않게. 단추만 받는다.
    for name in ("MonsterBackArt", "MonsterBackText_Center"):
        widget = find(tree, name)
        if widget is not None:
            widget.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    say(f"  저장={unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)}")


apply_hud()
apply_merc_tab()
apply_monster_tab()
apply_unit_hp_bar()
apply_overlay()
RESULT.parent.mkdir(parents=True, exist_ok=True)
RESULT.write_text("\n".join(LINES) + "\n", encoding="utf-8")
