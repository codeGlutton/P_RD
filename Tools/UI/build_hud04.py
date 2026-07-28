# -*- coding: utf-8 -*-
"""시안4 전투 HUD 를 WBP 로 굽는다.

## 어떻게 만드나

시안에서 뜯은 판 열넷을 제자리에 놓고, 그 안 잰 자리에 글자와 그림을 얹는다.
판은 늘리지 않는다 -- 시안에서 오려 낸 것이라 늘리면 몰딩이 뭉갠다.

자리는 hud04_slots.py 에서 온다. 그 파일은 prepare_hud04.py 가 만들고, 그
값은 시안을 만든 쪽이 준 좌표 그대로다. 사람 손을 한 번도 안 거친다.

## 좌표

판 자리도 판 안 자리도 전부 **화면 기준**이다. 판을 캔버스에 놓고 그 안에
다시 얹으면 두 좌표계를 오가야 하고, 한 번 어긋나면 어느 쪽이 틀렸는지 안
보인다. 그래서 판도 내용도 같은 뿌리 캔버스에 나란히 놓는다.

## 이름 규칙

C++ 이 이름으로 위젯을 찾는다. 못 찾은 것은 건너뛴다.

    RoundText / ObjectiveText
    TurnPortrait_0~4 / TurnSelected
    EnemyPortrait / EnemyName / EnemyHPValue / EnemyHPBar
    EnemyDefenseText / EnemyDamageText
    ActionName_0~5 / ActionIcon_0~5 / ActionDamage_0~5
    ActionCooldown_0~5 / ActionCost_0~5 / ActionButton_0~5
    ActionSelected_0~5 / ActionCooldownOverlay_0~5
    PartyPortrait_0~2 / PartyName_0~2 / PartyHPValue_0~2 / PartyHPBar_0~2
    PartyStatus_0~2 / PartyAPGems_0~2 / PartySelected_0~2
    EndTurnButton / EndTurnLabel

    python (RunEditorPython) build_hud04.py
"""
import os
import sys

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import combat_layout_kit as kit  # noqa: E402
from hud04_slots import DETAIL, PLACE, TEXTURE  # noqa: E402

ASSET = "WBP_CombatHUD04"
ART = "/Game/SVN/OutSideAsset/UI/KayKit/HUD04"
HEADS = "/Game/SVN/OutSideAsset/UI/KayKit/Heads"

#: 시안 좌표 -> 설계 캔버스.
K = kit.CHROME_SCALE

TEXT_DARK = unreal.LinearColor(0.16, 0.11, 0.07, 1.0)
TEXT_PALE = unreal.LinearColor(0.98, 0.95, 0.88, 1.0)
TEXT_DIM = unreal.LinearColor(0.62, 0.58, 0.52, 1.0)

#: 행동 여섯 장. 판 이름과 시안에 적힌 값.
#: 자리 순서는 시안 그림 순서다 -- 위, 왼쪽 위, 오른쪽 위, 왼쪽 아래,
#: 오른쪽 아래, 아래.
ACTIONS = (
    ("action_top", "이동", "", "", "1"),
    ("action_left_upper", "평타", "피해 4~7", "", "1"),
    ("action_right_upper", "방패 강타", "피해 8~14", "쿨 2턴", "2"),
    ("action_left_lower", "고정 참격", "피해 5~9", "", "1"),
    ("action_right_lower", "돌파 베기", "피해 12~18", "남은 3턴", "2"),
    ("action_bottom", "반격 태세", "방어 태세", "쿨 1턴", "1"),
)

#: 아군 세 줄. 판 이름과 시안 값.
PARTY = (
    ("bottom_status_left", "기사", "90/100", "중독 2턴", "KK_Face_Knight_HeadV2"),
    ("bottom_status_center", "궁수", "100/100", "", "KK_Face_Ranger_HeadV2"),
    ("bottom_status_right", "마법사", "75/100", "", "KK_Face_Mage_HeadV2"),
)

#: 턴 줄에 걸리는 다섯. 시안에 그려진 차례.
TURN_FACES = ("KK_Face_Knight_HeadV2", "KK_Face_Enemy_Eagle_HeadV2",
              "KK_Face_Ranger_HeadV2", "KK_Face_Enemy_Eagle_HeadV2",
              "KK_Face_Mage_HeadV2")


def at(rect):
    """시안 픽셀로 적힌 자리를 설계 캔버스로."""
    return [v * K for v in rect]


def spot(plate, element):
    """판 안 요소의 화면 자리. 없으면 None."""
    found = DETAIL.get(plate, {}).get(element)
    return at(found) if found else None


def plate(blueprint, root, plate_name, widget_name, z=kit.Z_FILL):
    """판 한 장을 제자리에. 늘리지 않는다."""
    x, y, w, h = at(PLACE[plate_name])
    kit.image(blueprint, widget_name, root, x, y, w, h, None, z_order=z,
              texture="{}/{}".format(ART, TEXTURE[plate_name]), tint=kit.WHITE)


def text(blueprint, name, root, rect, value, points, colour, align="center",
         bold=False):
    if rect is None:
        return
    x, y, w, h = rect
    kit.label(blueprint, name, root, x, y, w, h, value, points, colour, align,
              None, bold=bold)


def face(blueprint, name, root, rect, texture, z=kit.Z_CONTENT):
    if rect is None:
        return
    x, y, w, h = rect
    kit.image(blueprint, name, root, x, y, w, h, None, z_order=z,
              texture="{}/{}".format(HEADS, texture), tint=kit.WHITE)


def top_row(blueprint, root):
    plate(blueprint, root, "top_left_parchment", "RoundPlate")
    text(blueprint, "RoundText", root, spot("top_left_parchment", "round_label"),
         "ROUND 1", 24, TEXT_DARK, bold=True)

    plate(blueprint, root, "top_center_turn_order", "TurnPlate")
    for index in range(5):
        face(blueprint, "TurnPortrait_%d" % index, root,
             spot("top_center_turn_order", "turn_portrait_%02d" % (index + 1)),
             TURN_FACES[index])
    # 지금 차례 표시. 판에 없어 낱장으로 얹고 접어 둔다 -- 판에 그려 넣으면
    # 그 칸만 게임에서 영영 지금 차례가 된다.
    marker = spot("top_center_turn_order", "selected_outline")
    if marker:
        x, y, w, h = marker
        kit.image(blueprint, "TurnSelected", root, x, y, w, h, None,
                  z_order=kit.Z_MARKER, tint=unreal.LinearColor(1.0, 0.82, 0.32, 1.0))
        kit.fold(blueprint, "TurnSelected")

    plate(blueprint, root, "top_right_parchment", "ObjectivePlate")
    text(blueprint, "ObjectiveText", root,
         spot("top_right_parchment", "objective_text"),
         "모든 적 처치 — 남은 적 2", 20, TEXT_DARK)


def enemy_panel(blueprint, root):
    plate(blueprint, root, "upper_right_enemy_panel", "EnemyPlate")
    key = "upper_right_enemy_panel"
    face(blueprint, "EnemyPortrait", root, spot(key, "enemy_portrait"),
         "KK_Face_Enemy_Eagle_HeadV2")
    text(blueprint, "EnemyName", root, spot(key, "enemy_name"), "독수리", 22,
         TEXT_PALE, bold=True)
    text(blueprint, "EnemyHPValue", root, spot(key, "hp_value"), "50/50", 18,
         TEXT_PALE)
    text(blueprint, "EnemyDefenseText", root, spot(key, "defense_text"), "방어 0",
         17, TEXT_PALE)
    text(blueprint, "EnemyDamageText", root, spot(key, "damage_text"),
         "예상 피해 8~14", 17, TEXT_PALE, align="left")

    hp = spot(key, "hp_bar")
    if hp:
        x, y, w, h = hp
        kit.bar(blueprint, "EnemyHPBar", root, x, y, w, h,
                unreal.LinearColor(0.78, 0.18, 0.16, 1.0))


def actions(blueprint, root):
    for index, (plate_name, name, damage, cooldown, cost) in enumerate(ACTIONS):
        plate(blueprint, root, plate_name, "ActionPlate_%d" % index,
              kit.Z_CONTENT)
        text(blueprint, "ActionName_%d" % index, root,
             spot(plate_name, "action_name"), name, 18, TEXT_PALE, bold=True)
        text(blueprint, "ActionDamage_%d" % index, root,
             spot(plate_name, "damage_text") or spot(plate_name, "stance_text"),
             damage, 15, TEXT_PALE)
        text(blueprint, "ActionCooldown_%d" % index, root,
             spot(plate_name, "cooldown_text"), cooldown, 15, TEXT_DIM)
        text(blueprint, "ActionCost_%d" % index, root,
             spot(plate_name, "cost_badge"), cost, 19, TEXT_DARK, bold=True)

        icon = spot(plate_name, "action_icon")
        if icon:
            x, y, w, h = icon
            kit.image(blueprint, "ActionIcon_%d" % index, root, x, y, w, h,
                      None, z_order=kit.Z_CONTENT, tint=kit.WHITE)

        # 고른 표시와 쿨타임 가림막. 둘 다 판에 없다.
        for element, widget, tint in (
                ("selected_outline", "ActionSelected_%d" % index,
                 unreal.LinearColor(1.0, 0.82, 0.32, 1.0)),
                ("cooldown_overlay", "ActionCooldownOverlay_%d" % index,
                 unreal.LinearColor(0.0, 0.0, 0.0, 0.55))):
            rect = spot(plate_name, element)
            if rect is None:
                # 시안이 이 칸에만 그려 준 것이라도 여섯 장 다 있어야 한다.
                # 없으면 런타임이 그 칸만 표시를 못 켠다.
                rect = at(PLACE[plate_name])
            x, y, w, h = rect
            kit.image(blueprint, widget, root, x, y, w, h, None,
                      z_order=kit.Z_MARKER, tint=tint)
            kit.fold(blueprint, widget)

        x, y, w, h = at(PLACE[plate_name])
        kit.ghost_button(blueprint, "ActionButton_%d" % index, root, x, y, w, h)


def party(blueprint, root):
    for index, (plate_name, name, hp, status, portrait) in enumerate(PARTY):
        plate(blueprint, root, plate_name, "PartyPlate_%d" % index,
              kit.Z_CONTENT)
        face(blueprint, "PartyPortrait_%d" % index, root,
             spot(plate_name, "party_portrait"), portrait)
        text(blueprint, "PartyName_%d" % index, root,
             spot(plate_name, "character_name"), name, 17, TEXT_PALE,
             align="left", bold=True)
        text(blueprint, "PartyHPValue_%d" % index, root,
             spot(plate_name, "hp_value"), hp, 15, TEXT_PALE, align="left")
        text(blueprint, "PartyStatus_%d" % index, root,
             spot(plate_name, "status_text"), status, 14, TEXT_PALE,
             align="left")

        bar = spot(plate_name, "hp_bar")
        if bar:
            x, y, w, h = bar
            kit.bar(blueprint, "PartyHPBar_%d" % index, root, x, y, w, h,
                    unreal.LinearColor(0.44, 0.74, 0.24, 1.0))

        gems = spot(plate_name, "ap_gems")
        if gems:
            x, y, w, h = gems
            kit.image(blueprint, "PartyAPGems_%d" % index, root, x, y, w, h,
                      None, z_order=kit.Z_CONTENT, tint=kit.WHITE)

        # 지금 차례인 아군 테두리. 왼쪽 칸에만 그려져 있지만 셋 다 있어야
        # 런타임이 차례를 옮길 수 있다.
        rect = spot(plate_name, "selected_outline") or at(PLACE[plate_name])
        x, y, w, h = rect
        kit.image(blueprint, "PartySelected_%d" % index, root, x, y, w, h,
                  None, z_order=kit.Z_MARKER,
                  tint=unreal.LinearColor(1.0, 0.82, 0.32, 1.0))
        kit.fold(blueprint, "PartySelected_%d" % index)

        kit.ghost_button(blueprint, "PartyButton_%d" % index, root,
                         *at(PLACE[plate_name]))


def end_turn(blueprint, root):
    plate(blueprint, root, "bottom_right_button", "EndTurnPlate", kit.Z_CONTENT)
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
actions(bp, "RootCanvas")
party(bp, "RootCanvas")
end_turn(bp, "RootCanvas")
unreal.EditorAssetLibrary.save_loaded_asset(bp, False)
kit.commit_asset(ASSET)
unreal.log("[HUD04] {} 구움".format(ASSET))
