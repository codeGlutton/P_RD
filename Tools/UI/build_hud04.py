# -*- coding: utf-8 -*-
"""시안4 전투 HUD 를 WBP 로 굽는다.

## 어떻게 만드나

시안에서 뜯은 판 열넷을 제자리에 놓고, 그 안 잰 자리에 글자와 그림을 얹는다.
판은 늘리지 않는다 -- 시안에서 오려 낸 것이라 늘리면 몰딩이 뭉갠다.

자리는 hud04_slots.py, 그림 조각은 hud04_sprites.py 에서 온다. 둘 다
기계가 만든다. 시안이 바뀌면 다시 돌린다.

## 이름은 런타임이 정한다

UCombatLayoutHUDWidget 이 이름으로 위젯을 찾는다. 그 이름을 여기서 새로
지으면 글자는 보이는데 게임이 값을 못 넣는다 -- 처음에 Action* 으로 지었다가
그렇게 됐다. 찾는 쪽 이름을 그대로 쓴다.

    RoundText / ObjectiveText
    TurnToken_i / TurnPortrait_i / TurnCurrent_i
    CommandCard_i / CommandButton_i / CommandIcon_i / CommandName_i
    CommandCost_i / CommandDamage_i / CommandCooldown_i
    CommandSelected_i / CommandDisabled_i
    PartyCard_i / PartyPortrait_i / PartyName_i / PartyHPBar_i
    PartyHPText_i / PartyStatus_i / PartyStatusIcon_i
    PartyAPPip_i_j / PartyAPIcon_i / PartyAPText_i
    PartySelected_i
    EnemyPanel / EnemyPortrait / EnemyName / EnemyHPBar / EnemyHPText
    EnemyDefense / EnemyForecast
    EndTurnButton / EndTurnLabel

## 층

    판     Z=0   맨 아래. 오려 낸 껍데기다
    내용   Z=10  초상, 아이콘, 막대
    글자   Z=15  아이콘 위에 온다
    표시   Z=40  선택 테두리와 쿨타임 가림막

판과 내용을 같은 층에 두었더니 아군 초상이 판 뒤로 숨었다. 같은 층이면 어느
쪽이 위인지 만든 차례가 정하는데, 그건 코드를 조금만 옮겨도 바뀐다.

    python (RunEditorPython) build_hud04.py
"""
import os
import sys

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import combat_layout_kit as kit  # noqa: E402
from hud04_slots import DETAIL, PLACE, TEXTURE  # noqa: E402
from hud04_sprites import SPRITE  # noqa: E402

ASSET = "WBP_CombatHUD04"
ART = "/Game/SVN/OutSideAsset/UI/KayKit/HUD04"

#: 시안 좌표 -> 설계 캔버스.
K = kit.CHROME_SCALE

#: 층. 판과 내용과 표시를 확실히 갈라 둔다.
Z_PLATE, Z_CONTENT, Z_TEXT, Z_MARK = 0, 10, 15, 40

TEXT_DARK = unreal.LinearColor(0.16, 0.11, 0.07, 1.0)
TEXT_PALE = unreal.LinearColor(0.98, 0.95, 0.88, 1.0)
TEXT_DIM = unreal.LinearColor(0.62, 0.58, 0.52, 1.0)

#: 명령 여섯. 판 이름과 시안에 적힌 값. 차례는 시안 그림 차례다.
COMMANDS = (
    ("action_top", "이동", "", "", "1"),
    ("action_left_upper", "평타", "피해 4~7", "", "1"),
    ("action_right_upper", "방패 강타", "피해 8~14", "쿨 2턴", "2"),
    ("action_left_lower", "고정 참격", "피해 5~9", "", "1"),
    ("action_right_lower", "돌파 베기", "피해 12~18", "남은 3턴", "2"),
    ("action_bottom", "반격 태세", "방어 태세", "쿨 1턴", "1"),
)

#: 아군 세 줄.
PARTY = (
    ("bottom_status_left", "기사", "90/100", "중독 2턴", 3),
    ("bottom_status_center", "궁수", "100/100", "", 4),
    ("bottom_status_right", "마법사", "75/100", "", 4),
)

#: 화면에 낱개로 그릴 수 있는 AP 최대 개수.
#:
#: AP 에는 상한이 없다. 그런데 낱개 아이콘은 칸 너비가 정해져 있어 무한히 늘 수
#: 없으므로, 여기까지는 낱개로 그리고 넘으면 런타임이 "아이콘 x N" 으로 바꾼다.
#: 여덟은 런타임(UCombatLayoutHUDWidget)이 찾는 개수와 맞춘 값이다.
AP_PIPS = 8


def at(rect):
    """시안 픽셀로 적힌 자리를 설계 캔버스로."""
    return [v * K for v in rect]


def spot(plate, element):
    found = DETAIL.get(plate, {}).get(element)
    return at(found) if found else None


def sprite(plate, element):
    """뗀 그림의 이름과 자리. 없으면 (None, None)."""
    name = "KK_HUD04_%s__%s" % (plate, element)
    return (name, at(SPRITE[name])) if name in SPRITE else (None, None)


def plate(blueprint, root, plate_name, widget_name):
    x, y, w, h = at(PLACE[plate_name])
    kit.image(blueprint, widget_name, root, x, y, w, h, None, z_order=Z_PLATE,
              texture="{}/{}".format(ART, TEXTURE[plate_name]), tint=kit.WHITE)


def piece(blueprint, root, widget_name, plate_name, element, z=Z_CONTENT):
    """뗀 그림 한 장을 제자리에. 없으면 아무것도 안 만든다."""
    name, rect = sprite(plate_name, element)
    if name is None:
        return False
    x, y, w, h = rect
    kit.image(blueprint, widget_name, root, x, y, w, h, None, z_order=z,
              texture="{}/{}".format(ART, name), tint=kit.WHITE)
    return True


def text(blueprint, name, root, rect, value, points, colour, align="center",
         bold=False):
    if rect is None:
        return
    x, y, w, h = rect
    kit.label(blueprint, name, root, x, y, w, h, value, points, colour, align,
              None, bold=bold)
    # 글자를 아이콘 위층으로 올린다. kit.label 은 내용 층에 놓는데, 같은 층이면
    # 만든 차례가 위아래를 정해서 아이콘이 큰 칸에서는 글자가 묻힌다 --
    # 반격 태세의 "방어 태세" 줄이 그렇게 사라졌다.
    kit.place(blueprint, name, x, y, w, h, "tl", None, Z_TEXT)


def outline(blueprint, root, widget_name, rect):
    """선택 테두리. 가운데가 빈 그림이라 밑그림이 비친다."""
    x, y, w, h = at(rect)
    kit.image(blueprint, widget_name, root, x, y, w, h, None, z_order=Z_MARK,
              texture="{}/KK_HUD04_selected_outline".format(ART),
              tint=kit.WHITE)
    # 테두리는 늘려도 무늬가 안 뭉갠다. 9슬라이스로 모서리만 지킨다.
    kit.paint(kit.helper.umg_find_widget(blueprint, widget_name),
              margin=(0.25, 0.25))
    kit.fold(blueprint, widget_name)


def top_row(blueprint, root):
    plate(blueprint, root, "top_left_parchment", "RoundPlate")
    text(blueprint, "RoundText", root, spot("top_left_parchment", "round_label"),
         "ROUND 1", 24, TEXT_DARK, bold=True)

    plate(blueprint, root, "top_center_turn_order", "TurnPlate")
    for index in range(5):
        element = "turn_portrait_%02d" % (index + 1)
        rect = spot("top_center_turn_order", element)
        if rect is None:
            continue
        # 칸 자체. 런타임이 자리 수만큼 켜고 끈다.
        x, y, w, h = rect
        kit.add(blueprint, "CanvasPanel", "TurnToken_%d" % index, root)
        kit.place(blueprint, "TurnToken_%d" % index, x, y, w, h, "tl", None,
                  Z_CONTENT)
        piece(blueprint, root, "TurnPortrait_%d" % index,
              "top_center_turn_order", element)
        outline(blueprint, root, "TurnCurrent_%d" % index,
                DETAIL["top_center_turn_order"][element])

    plate(blueprint, root, "top_right_parchment", "ObjectivePlate")
    text(blueprint, "ObjectiveText", root,
         spot("top_right_parchment", "objective_text"),
         "모든 적 처치 — 남은 적 2", 20, TEXT_DARK)


def enemy_panel(blueprint, root):
    key = "upper_right_enemy_panel"
    x, y, w, h = at(PLACE[key])
    kit.add(blueprint, "CanvasPanel", "EnemyPanel", root)
    kit.place(blueprint, "EnemyPanel", x, y, w, h, "tl", None, Z_PLATE)
    plate(blueprint, root, key, "EnemyPlate")

    piece(blueprint, root, "EnemyPortrait", key, "enemy_portrait")
    piece(blueprint, root, "EnemyHPIcon", key, "hp_icon")
    piece(blueprint, root, "EnemyDefenseIcon", key, "defense_icon")
    piece(blueprint, root, "EnemyDamageIcon", key, "damage_icon")

    text(blueprint, "EnemyName", root, spot(key, "enemy_name"), "독수리", 22,
         TEXT_PALE, bold=True)
    text(blueprint, "EnemyHPText", root, spot(key, "hp_value"), "50/50", 18,
         TEXT_PALE)
    text(blueprint, "EnemyDefense", root, spot(key, "defense_text"), "방어 0",
         17, TEXT_PALE, align="left")
    text(blueprint, "EnemyForecast", root, spot(key, "damage_text"),
         "예상 피해 8~14", 17, TEXT_PALE, align="left")

    hp = spot(key, "hp_bar")
    if hp:
        bx, by, bw, bh = hp
        kit.bar(blueprint, "EnemyHPBar", root, bx, by, bw, bh,
                unreal.LinearColor(0.86, 0.24, 0.20, 1.0))


def commands(blueprint, root):
    for index, (plate_name, name, damage, cooldown, cost) in enumerate(COMMANDS):
        x, y, w, h = at(PLACE[plate_name])
        kit.add(blueprint, "CanvasPanel", "CommandCard_%d" % index, root)
        kit.place(blueprint, "CommandCard_%d" % index, x, y, w, h, "tl", None,
                  Z_PLATE)
        plate(blueprint, root, plate_name, "CommandPlate_%d" % index)

        piece(blueprint, root, "CommandIcon_%d" % index, plate_name,
              "action_icon")
        text(blueprint, "CommandName_%d" % index, root,
             spot(plate_name, "action_name"), name, 18, TEXT_PALE, bold=True)
        text(blueprint, "CommandDamage_%d" % index, root,
             spot(plate_name, "damage_text") or spot(plate_name, "stance_text"),
             damage, 15, TEXT_PALE)
        text(blueprint, "CommandCooldown_%d" % index, root,
             spot(plate_name, "cooldown_text"), cooldown, 15, TEXT_DIM)
        text(blueprint, "CommandCost_%d" % index, root,
             spot(plate_name, "cost_badge"), cost, 19, TEXT_DARK, bold=True)

        # 골라진 표시와 못 쓰는 표시. 여섯 장 다 있어야 런타임이 어느 칸이든
        # 켤 수 있다 -- 시안이 한 칸에만 그려 준 것이라도 그렇다.
        outline(blueprint, root, "CommandSelected_%d" % index, PLACE[plate_name])
        # 가림막은 판 그림을 한 장 더 깔고 어둡게 물들인다. 검은 네모를 덮으면
        # 판 밖까지 사각형으로 덮여 시안과 모양이 달라진다.
        kit.image(blueprint, "CommandDisabled_%d" % index, root, x, y, w, h,
                  None, z_order=Z_MARK,
                  texture="{}/{}".format(ART, TEXTURE[plate_name]),
                  tint=unreal.LinearColor(0.0, 0.0, 0.0, 0.62))
        kit.fold(blueprint, "CommandDisabled_%d" % index)

        kit.ghost_button(blueprint, "CommandButton_%d" % index, root, x, y, w, h)


def party(blueprint, root):
    for index, (plate_name, name, hp, status, lit) in enumerate(PARTY):
        x, y, w, h = at(PLACE[plate_name])
        kit.add(blueprint, "CanvasPanel", "PartyCard_%d" % index, root)
        kit.place(blueprint, "PartyCard_%d" % index, x, y, w, h, "tl", None,
                  Z_PLATE)
        plate(blueprint, root, plate_name, "PartyPlate_%d" % index)

        piece(blueprint, root, "PartyPortrait_%d" % index, plate_name,
              "party_portrait")
        piece(blueprint, root, "PartyHPIcon_%d" % index, plate_name, "hp_icon")
        if not piece(blueprint, root, "PartyStatusIcon_%d" % index, plate_name,
                     "status_icon"):
            # 상태 아이콘은 시안이 기사 줄에만 그려 뒀다. 셋 다 있어야 런타임이
            # 누가 걸리든 켤 수 있으므로 기사 것을 빌려 자리만 잡아 둔다.
            borrowed, rect = sprite("bottom_status_left", "status_icon")
            if borrowed and spot(plate_name, "hp_icon"):
                ix, iy, iw, ih = spot(plate_name, "hp_icon")
                kit.image(blueprint, "PartyStatusIcon_%d" % index, root,
                          ix, iy + ih * 1.25, iw, ih, None, z_order=Z_CONTENT,
                          texture="{}/{}".format(ART, borrowed), tint=kit.WHITE)
                kit.fold(blueprint, "PartyStatusIcon_%d" % index)

        text(blueprint, "PartyName_%d" % index, root,
             spot(plate_name, "character_name"), name, 17, TEXT_PALE,
             align="left", bold=True)
        text(blueprint, "PartyHPText_%d" % index, root,
             spot(plate_name, "hp_value"), hp, 15, TEXT_PALE, align="left")
        text(blueprint, "PartyStatus_%d" % index, root,
             spot(plate_name, "status_text"), status, 14, TEXT_PALE,
             align="left")

        bar = spot(plate_name, "hp_bar")
        if bar:
            bx, by, bw, bh = bar
            kit.bar(blueprint, "PartyHPBar_%d" % index, root, bx, by, bw, bh,
                    unreal.LinearColor(0.44, 0.74, 0.24, 1.0))

        # AP 는 낱개로 놓는다. 런타임이 개수를 세어 하나씩 켠다 -- 묶음 그림
        # 한 장이면 3/4 를 못 그린다.
        gems = spot(plate_name, "ap_gems")
        if gems:
            gx, gy, gw, gh = gems
            # 낱개 아이콘을 왼쪽부터 채운다. 런타임이 남은 AP 만큼만 켠다.
            # 시안은 네 개로 그려져 있지만 AP 에 상한이 없으므로 자리는 여덟까지
            # 잡아 둔다 -- 없는 위젯은 런타임이 그냥 못 찾고 넘어간다.
            step = gw / float(AP_PIPS)
            for pip in range(AP_PIPS):
                kit.image(blueprint, "PartyAPPip_%d_%d" % (index, pip), root,
                          gx + step * pip, gy, step * 0.86, gh, None,
                          z_order=Z_CONTENT,
                          texture="{}/KK_HUD04_ap_pip_lit".format(ART),
                          tint=kit.WHITE)
                if pip >= lit:
                    kit.fold(blueprint, "PartyAPPip_%d_%d" % (index, pip))

            # 여덟을 넘으면 낱개로는 못 보여준다. 그때만 이 글자가 켜진다.
            kit.image(blueprint, "PartyAPIcon_%d" % index, root, gx, gy,
                      step * 0.86, gh, None, z_order=Z_CONTENT,
                      texture="{}/KK_HUD04_ap_pip_lit".format(ART),
                      tint=kit.WHITE)
            kit.fold(blueprint, "PartyAPIcon_%d" % index)
            kit.label(blueprint, "PartyAPText_%d" % index, root,
                      gx + step, gy, gw - step, gh, "", 15, TEXT_PALE, "left",
                      None, bold=True)
            kit.place(blueprint, "PartyAPText_%d" % index, gx + step, gy,
                      gw - step, gh, "tl", None, Z_TEXT)
            kit.fold(blueprint, "PartyAPText_%d" % index)

        outline(blueprint, root, "PartySelected_%d" % index, PLACE[plate_name])
        kit.ghost_button(blueprint, "PartyButton_%d" % index, root, x, y, w, h)


def end_turn(blueprint, root):
    plate(blueprint, root, "bottom_right_button", "EndTurnPlate")
    text(blueprint, "EndTurnLabel", root,
         spot("bottom_right_button", "button_label"), "턴 종료", 24, TEXT_PALE,
         bold=True)
    kit.ghost_button(blueprint, "EndTurnButton", root,
                     *at(PLACE["bottom_right_button"]))


kit.reset_ledger()
bp = kit.create_asset(ASSET)
kit.add(bp, "CanvasPanel", "RootCanvas", "")
top_row(bp, "RootCanvas")
enemy_panel(bp, "RootCanvas")
commands(bp, "RootCanvas")
party(bp, "RootCanvas")
end_turn(bp, "RootCanvas")
unreal.EditorAssetLibrary.save_loaded_asset(bp, False)
kit.commit_asset(ASSET)
unreal.log("[HUD04] {} 구움".format(ASSET))
