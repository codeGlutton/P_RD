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

## 묶음

카드 한 장의 부품은 전부 그 카드 밑에 들어간다. CommandCard_i 안에 판과
아이콘과 글자와 버튼이 있고, 아군은 PartyCard_i 안에, 적은 EnemyPanel 안에
있다.

런타임이 카드를 접을 때 그 묶음 하나만 끄기 때문이다. 처음에는 전부 뿌리
캔버스에 평평하게 붙였는데 -- 좌표계를 하나로 두려고 -- 그러면 묶음이 빈
껍데기가 되어 접어도 아무 일이 안 일어난다. 실제로 카드가 안 접혔고, 한참
입력 쪽을 뒤졌다.

묶음 안 좌표는 묶음의 왼쪽 위에서 잰다. 화면 기준으로 적힌 자리표에서 묶음의
자리를 빼면 된다 -- local() 이 그 일을 한다.

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

try:
    from hud04_zone_art import ZONE_ART  # noqa: E402
except ImportError:
    ZONE_ART = {}

ASSET = "WBP_CombatHUD04"
ART = "/Game/SVN/OutSideAsset/UI/KayKit/HUD04"

def art_path(name):
    """HUD04 폴더의 그림 이름을 엔진 경로로. 없으면 None."""
    return "{}/{}".format(ART, name) if name else None


#: 명령 카드 여섯의 본. prepare_hud04.py 가 자리를 이 판으로 통일해 둔다.
CARD_TEMPLATE = "action_top"

#: 아군 칸 셋의 본. 카드와 같은 까닭으로 그림도 한 벌만 쓴다.
PARTY_TEMPLATE = "bottom_status_right"

#: 명령 카드 여섯이 함께 쓰는 판 그림.
#:
#: 시안은 카드마다 따로 그려 줬지만 자리만 다른 같은 카드다. 여섯 벌을 두면
#: 몰딩이 조금씩 달라서 한 줄에 놓았을 때 눈에 걸린다.
CARD_PLATE = "KK_HUD04_action_top"

#: 비용 배지. 시안1 부터 쓰던 둥근 배지를 그대로 쓴다.
COST_BADGE = "/Game/SVN/OutSideAsset/UI/KayKit/KK_Badge_Round"

#: 시안 좌표 -> 설계 캔버스.
K = kit.CHROME_SCALE

#: 층. 판과 내용과 표시를 확실히 갈라 둔다.
Z_PLATE, Z_CONTENT, Z_TEXT, Z_MARK = 0, 10, 15, 40

# 글자색을 미색 하나로 모았다.
#
# 검정 테두리를 두르기로 하면서 바탕이 밝든 어둡든 미색이 읽힌다. 전에는
# 양피지 위에 진갈색, 어두운 판 위에 미색을 따로 썼는데, 판을 한 장으로 줄이고
# 글자를 옮기다 보니 어느 글자가 어느 바탕에 얹히는지가 자꾸 바뀌었다.
TEXT_PALE = unreal.LinearColor(0.973, 0.973, 0.953, 1.0)
TEXT_DARK = TEXT_PALE
TEXT_DIM = unreal.LinearColor(0.80, 0.78, 0.74, 1.0)

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


def zone_art(plate, element):
    """구역 조정 쪽에서 얹은 그림 이름. 없으면 None.

    쪽에서 고른 것이 시안에서 뗀 조각보다 앞선다 -- 시안 것은 처음 자리를 잡을
    때 쓴 밑그림이고, 쪽에서 고른 것은 실제로 넣기로 정한 그림이다.
    """
    got = ZONE_ART.get(plate, {}).get(element)
    return got["texture"] if got else None


def local(rect, origin):
    """화면 기준 자리를 묶음 안 자리로. 묶음의 왼쪽 위를 뺀다."""
    if rect is None:
        return None
    return [rect[0] - origin[0], rect[1] - origin[1], rect[2], rect[3]]


def group(blueprint, root, name, plate_name, z=Z_PLATE):
    """묶음 한 칸을 만든다. 부품은 전부 이 안에 들어간다.

    런타임이 접을 때 이 하나만 끈다. 부품을 밖에 두면 접어도 아무 일이 없다.
    """
    x, y, w, h = at(PLACE[plate_name])
    kit.add(blueprint, "CanvasPanel", name, root)
    kit.place(blueprint, name, x, y, w, h, "tl", None, z)
    return name, (x, y), (w, h)


def plate(blueprint, parent, origin, plate_name, widget_name,
          texture_name=None):
    """판 껍데기 한 장. 늘리지 않고 오려 낸 크기 그대로 놓는다."""
    x, y, w, h = local(at(PLACE[plate_name]), origin)
    kit.image(blueprint, widget_name, parent, x, y, w, h, None, z_order=Z_PLATE,
              texture="{}/{}".format(ART, texture_name or TEXTURE[plate_name]),
              tint=kit.WHITE)


def piece(blueprint, parent, origin, widget_name, plate_name, element,
          z=Z_CONTENT):
    """뗀 그림 한 장을 제자리에. 없으면 아무것도 안 만든다.

    자리는 **자리표(DETAIL)** 를 먼저 본다. 뗀 그림에도 자기가 잘려 나온
    자리가 붙어 있지만, 그것은 시안을 자를 때의 자리라 손으로 맞춘 값을
    모른다 -- 구역을 옮겨도 그림은 안 따라와서, 쪽에서는 맞았는데 게임에서는
    그대로인 일이 생긴다. 실제로 턴 초상이 그랬다.
    """
    chosen = zone_art(plate_name, element)
    name, sprite_rect = sprite(plate_name, element)
    if chosen:
        name = chosen
    if name is None:
        return False
    rect = spot(plate_name, element) or sprite_rect
    x, y, w, h = local(rect, origin)
    kit.image(blueprint, widget_name, parent, x, y, w, h, None, z_order=z,
              texture="{}/{}".format(ART, name), tint=kit.WHITE)
    return True


#: 글자를 칸 안 어디에 붙이나. 요소 이름 -> (가로, 세로).
#:
#: 적어 두지 않은 것은 **가로세로 모두 가운데**다. 칸은 시안에서 잰 상자이고
#: 글자는 런타임이 넣는 것이라 길이를 모른다 -- 가운데가 어느 길이에서도
#: 덜 어긋난다.
#:
#: 여기 적는 것은 예외뿐이다. 왼쪽부터 채워야 읽히는 줄 같은 것.
TEXT_ALIGN = {
}


def text(blueprint, name, parent, origin, rect, value, points, colour,
         align=None, bold=False):
    placed = local(rect, origin)
    if placed is None:
        return
    x, y, w, h = placed
    halign, valign = TEXT_ALIGN.get(name.rsplit("_", 1)[0],
                                    TEXT_ALIGN.get(name, ("center", "middle")))
    kit.label(blueprint, name, parent, x, y, w, h, value, points, colour,
              align or halign, None, bold=bold)
    # 글자를 아이콘 위층으로 올린다. kit.label 은 내용 층에 놓는데, 같은 층이면
    # 만든 차례가 위아래를 정해서 아이콘이 큰 칸에서는 글자가 묻힌다.
    kit.place(blueprint, name, x, y, w, h, "tl", None, Z_TEXT)
    # 층을 정한 **뒤에** 붙인다. kit.place 가 칸 크기를 다시 정하므로 먼저
    # 붙이면 그 값이 덮인다.
    kit.align_in(blueprint, name, x, y, w, h, align or halign, valign)


def outline(blueprint, parent, origin, widget_name, rect):
    """선택 테두리. 가운데가 빈 그림이라 밑그림이 비친다."""
    x, y, w, h = local(at(rect), origin)
    kit.image(blueprint, widget_name, parent, x, y, w, h, None, z_order=Z_MARK,
              texture="{}/KK_HUD04_selected_outline".format(ART),
              tint=kit.WHITE)
    # 테두리는 늘려도 무늬가 안 뭉갠다. 9슬라이스로 모서리만 지킨다.
    kit.paint(kit.helper.umg_find_widget(blueprint, widget_name),
              margin=(0.25, 0.25))
    kit.fold(blueprint, widget_name)


def top_row(blueprint, root):
    name, origin, _ = group(blueprint, root, "RoundPanel", "top_left_parchment")
    plate(blueprint, name, origin, "top_left_parchment", "RoundPlate")
    text(blueprint, "RoundText", name, origin,
         spot("top_left_parchment", "round_label"), "ROUND 1", 24, TEXT_DARK,
         bold=True)

    turn, turn_origin, _ = group(blueprint, root, "TurnPanel",
                                 "top_center_turn_order")
    plate(blueprint, turn, turn_origin, "top_center_turn_order", "TurnPlate")
    for index in range(5):
        element = "turn_portrait_%02d" % (index + 1)
        rect = spot("top_center_turn_order", element)
        if rect is None:
            continue
        # 칸 하나. 런타임이 자리 수만큼 켜고 끄므로 그 안에 초상과 표시를 둔다.
        x, y, w, h = local(rect, turn_origin)
        token = "TurnToken_%d" % index
        kit.add(blueprint, "CanvasPanel", token, turn)
        kit.place(blueprint, token, x, y, w, h, "tl", None, Z_CONTENT)
        token_origin = (rect[0], rect[1])
        piece(blueprint, token, token_origin, "TurnPortrait_%d" % index,
              "top_center_turn_order", element)
        # 두르는 자리는 따로 잰 것이 있으면 그것을, 없으면 초상 자리를 쓴다.
        outline(blueprint, token, token_origin, "TurnCurrent_%d" % index,
                DETAIL["top_center_turn_order"].get("selected_outline")
                or DETAIL["top_center_turn_order"][element])

    obj, obj_origin, _ = group(blueprint, root, "ObjectivePanel",
                               "top_right_parchment")
    plate(blueprint, obj, obj_origin, "top_right_parchment", "ObjectivePlate")
    text(blueprint, "ObjectiveText", obj, obj_origin,
         spot("top_right_parchment", "objective_text"),
         "모든 적 처치 — 남은 적 2", 20, TEXT_DARK)


def enemy_panel(blueprint, root):
    key = "upper_right_enemy_panel"
    name, origin, _ = group(blueprint, root, "EnemyPanel", key)
    plate(blueprint, name, origin, key, "EnemyPlate")

    piece(blueprint, name, origin, "EnemyPortrait", key, "enemy_portrait")
    piece(blueprint, name, origin, "EnemyHPIcon", key, "hp_icon")
    piece(blueprint, name, origin, "EnemyDefenseIcon", key, "defense_icon")
    piece(blueprint, name, origin, "EnemyDamageIcon", key, "damage_icon")

    text(blueprint, "EnemyName", name, origin, spot(key, "enemy_name"), "독수리",
         22, TEXT_PALE, bold=True)
    text(blueprint, "EnemyHPText", name, origin, spot(key, "hp_value"), "50/50",
         18, TEXT_PALE)
    text(blueprint, "EnemyDefense", name, origin, spot(key, "defense_text"),
         "방어 0", 17, TEXT_PALE, align="left")
    text(blueprint, "EnemyForecast", name, origin, spot(key, "damage_text"),
         "예상 피해 8~14", 17, TEXT_PALE, align="left")

    hp = local(spot(key, "hp_bar"), origin)
    if hp:
        bx, by, bw, bh = hp
        kit.bar(blueprint, "EnemyHPBar", name, bx, by, bw, bh,
                unreal.LinearColor(0.86, 0.24, 0.20, 1.0))


def commands(blueprint, root):
    for index, (plate_name, name, damage, cooldown, cost) in enumerate(COMMANDS):
        card, origin, size = group(blueprint, root, "CommandCard_%d" % index,
                                   plate_name)
        # 여섯 장이 같은 판 그림을 쓴다. 시안은 카드마다 따로 그려 줬지만
        # 자리만 다른 같은 카드라, 그림도 한 장이면 된다 -- 여섯 벌을 두면
        # 몰딩이 조금씩 달라서 한 줄에 놓았을 때 눈에 걸린다.
        plate(blueprint, card, origin, plate_name, "CommandPlate_%d" % index,
              CARD_PLATE)

        # 아이콘도 한 벌만 쓴다. 런타임이 스킬마다 다른 그림으로 갈아 끼우니
        # 여기 있는 것은 자리를 잡아 두는 밑그림일 뿐이다. 카드마다 따로 두면
        # 판을 한 장으로 줄인 뜻이 없다.
        #
        # 자리는 이 카드의 것을, 그림은 본 카드의 것을 쓴다.
        icon_name, icon_rect = sprite(CARD_TEMPLATE, "action_icon")
        here = spot(plate_name, "action_icon") or icon_rect
        if icon_name and here:
            ix, iy, iw, ih = local(here, origin)
            kit.image(blueprint, "CommandIcon_%d" % index, card,
                      ix, iy, iw, ih, None, z_order=Z_CONTENT,
                      texture="{}/{}".format(ART, icon_name), tint=kit.WHITE)
        text(blueprint, "CommandName_%d" % index, card, origin,
             spot(plate_name, "action_name"), name, 18, TEXT_PALE, bold=True)
        text(blueprint, "CommandDamage_%d" % index, card, origin,
             spot(plate_name, "damage_text") or spot(plate_name, "stance_text"),
             damage, 15, TEXT_PALE)
        # 쿨타임도 배지 위에 숫자를 얹는다. 비용 배지 아래에 같은 모양으로
        # 놓아, 둘이 한 쌍으로 읽히게 한다.
        cool_badge = spot(plate_name, "cooldown_badge")
        if cool_badge:
            cx, cy, cw, ch = local(cool_badge, origin)
            kit.image(blueprint, "CommandCooldownBadge_%d" % index, card,
                      cx, cy, cw, ch, None, z_order=Z_CONTENT,
                      texture=art_path(zone_art(plate_name, "cooldown_badge"))
                      or COST_BADGE, tint=kit.WHITE)
        text(blueprint, "CommandCooldown_%d" % index, card, origin,
             cool_badge or spot(plate_name, "cooldown_text"),
             cooldown, 19, TEXT_PALE, bold=True)
        # 비용은 배지 그림 위에 숫자를 얹는다. 시안은 판에 배지를 그려
        # 넣었는데, 판을 한 장으로 줄이면서 그 배지도 한 벌만 남았다.
        # 숫자만 얹으면 판마다 배지가 있는 자리와 없는 자리가 갈린다.
        badge = spot(plate_name, "cost_badge")
        if badge:
            bx, by, bw, bh = local(badge, origin)
            kit.image(blueprint, "CommandCostBadge_%d" % index, card,
                      bx, by, bw, bh, None, z_order=Z_CONTENT,
                      texture=art_path(zone_art(plate_name, "cost_badge"))
                      or COST_BADGE, tint=kit.WHITE)
        text(blueprint, "CommandCost_%d" % index, card, origin,
             badge, cost, 19, TEXT_PALE, bold=True)

        # 골라진 표시는 안 만든다. 스킬을 고르는 순간 조준에 들고, 조준 중에는
        # 카드가 통째로 비킨다 -- 금테가 켜지자마자 카드와 같이 사라져서
        # 한 프레임 반짝이는 것이 전부였다.
        #
        # 가림막은 판 그림을 한 장 더 깔고 어둡게 물들인다. 검은 네모를 덮으면
        # 판 밖까지 사각형으로 덮여 시안과 모양이 달라진다.
        kit.image(blueprint, "CommandDisabled_%d" % index, card, 0, 0,
                  size[0], size[1], None, z_order=Z_MARK,
                  texture="{}/{}".format(ART, CARD_PLATE),
                  tint=unreal.LinearColor(0.0, 0.0, 0.0, 0.62))
        kit.fold(blueprint, "CommandDisabled_%d" % index)

        # 버튼도 카드 안이다. 밖에 두면 접힌 카드가 계속 눌린다.
        kit.ghost_button(blueprint, "CommandButton_%d" % index, card, 0, 0,
                         size[0], size[1])


def party(blueprint, root):
    for index, (plate_name, name, hp, status, lit) in enumerate(PARTY):
        card, origin, size = group(blueprint, root, "PartyCard_%d" % index,
                                   plate_name)
        # 셋이 같은 판 그림을 쓴다. 자리만 다른 같은 칸이다.
        plate(blueprint, card, origin, plate_name, "PartyPlate_%d" % index,
              TEXTURE[PARTY_TEMPLATE])

        for widget_name, element in (
                ("PartyPortrait_%d" % index, "party_portrait"),
                ("PartyHPIcon_%d" % index, "hp_icon")):
            # 자리는 이 칸의 것을, 그림은 본 칸의 것을 쓴다. 초상은 런타임이
            # 용병마다 갈아 끼우니 여기 있는 것은 밑그림이다.
            art_name, _ = sprite(PARTY_TEMPLATE, element)
            here = spot(plate_name, element)
            if art_name and here:
                ax, ay, aw, ah = local(here, origin)
                kit.image(blueprint, widget_name, card, ax, ay, aw, ah, None,
                          z_order=Z_CONTENT,
                          texture="{}/{}".format(ART, art_name),
                          tint=kit.WHITE)
        if not piece(blueprint, card, origin, "PartyStatusIcon_%d" % index,
                     plate_name, "status_icon"):
            # 상태 아이콘은 시안이 기사 줄에만 그려 뒀다. 셋 다 있어야 런타임이
            # 누가 걸리든 켤 수 있으므로 기사 것을 빌려 자리만 잡아 둔다.
            borrowed, _ = sprite("bottom_status_left", "status_icon")
            hp_icon = local(spot(plate_name, "hp_icon"), origin)
            if borrowed and hp_icon:
                ix, iy, iw, ih = hp_icon
                kit.image(blueprint, "PartyStatusIcon_%d" % index, card,
                          ix, iy + ih * 1.25, iw, ih, None, z_order=Z_CONTENT,
                          texture="{}/{}".format(ART, borrowed), tint=kit.WHITE)
                kit.fold(blueprint, "PartyStatusIcon_%d" % index)

        text(blueprint, "PartyName_%d" % index, card, origin,
             spot(plate_name, "character_name"), name, 17, TEXT_PALE,
             align="left", bold=True)
        text(blueprint, "PartyHPText_%d" % index, card, origin,
             spot(plate_name, "hp_value"), hp, 15, TEXT_PALE, align="left")
        text(blueprint, "PartyStatus_%d" % index, card, origin,
             spot(plate_name, "status_text"), status, 14, TEXT_PALE,
             align="left")

        bar = local(spot(plate_name, "hp_bar"), origin)
        if bar:
            bx, by, bw, bh = bar
            kit.bar(blueprint, "PartyHPBar_%d" % index, card, bx, by, bw, bh,
                    unreal.LinearColor(0.44, 0.74, 0.24, 1.0))

        # AP 는 낱개로 놓는다. 런타임이 개수를 세어 하나씩 켠다 -- 묶음 그림
        # 한 장이면 3/4 를 못 그린다.
        gems = local(spot(plate_name, "ap_gems"), origin)
        if gems:
            gx, gy, gw, gh = gems
            step = gw / float(AP_PIPS)
            for pip in range(AP_PIPS):
                kit.image(blueprint, "PartyAPPip_%d_%d" % (index, pip), card,
                          gx + step * pip, gy, step * 0.86, gh, None,
                          z_order=Z_CONTENT,
                          texture="{}/KK_HUD04_ap_pip_lit".format(ART),
                          tint=kit.WHITE)
                if pip >= lit:
                    kit.fold(blueprint, "PartyAPPip_%d_%d" % (index, pip))

            # 여덟을 넘으면 낱개로는 못 보여준다. 그때만 이 글자가 켜진다.
            kit.image(blueprint, "PartyAPIcon_%d" % index, card, gx, gy,
                      step * 0.86, gh, None, z_order=Z_CONTENT,
                      texture="{}/KK_HUD04_ap_pip_lit".format(ART),
                      tint=kit.WHITE)
            kit.fold(blueprint, "PartyAPIcon_%d" % index)
            kit.label(blueprint, "PartyAPText_%d" % index, card,
                      gx + step, gy, gw - step, gh, "", 15, TEXT_PALE, "left",
                      None, bold=True)
            kit.place(blueprint, "PartyAPText_%d" % index, gx + step, gy,
                      gw - step, gh, "tl", None, Z_TEXT)
            kit.fold(blueprint, "PartyAPText_%d" % index)

        outline(blueprint, card, origin, "PartySelected_%d" % index,
                PLACE[plate_name])
        kit.ghost_button(blueprint, "PartyButton_%d" % index, card, 0, 0,
                         size[0], size[1])


def end_turn(blueprint, root):
    name, origin, size = group(blueprint, root, "EndTurnPanel",
                               "bottom_right_button")
    plate(blueprint, name, origin, "bottom_right_button", "EndTurnPlate")
    text(blueprint, "EndTurnLabel", name, origin,
         spot("bottom_right_button", "button_label"), "턴 종료", 24, TEXT_PALE,
         bold=True)
    kit.ghost_button(blueprint, "EndTurnButton", name, 0, 0, size[0], size[1])


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
