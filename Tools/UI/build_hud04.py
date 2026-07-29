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
    TurnPageLeft / TurnPageLeftText / TurnPageRight / TurnPageRightText
    CommandCard_i / CommandButton_i / CommandIcon_i / CommandName_i
    CommandCost_i / CommandDamage_i / CommandCooldown_i
    CommandSelected_i / CommandDisabled_i
    PartyCard_i / PartyPortrait_i / PartyName_i / PartyHPBar_i
    PartyHPText_i / PartyStatus_i / PartyStatusIcon_i / PartyStatusFrame_i_j
    PartyAPPip_i_j / PartyAPPipUsed_i_j / PartyAPPlate_i / PartyAPText_i
    MenuButton_i
    EnemyPanel / EnemyPortrait / EnemyName / EnemyHPBar / EnemyHPText
    EnemyDefense / EnemyForecast
    EndTurnButton / EndTurnLabel / ConfirmButton / ConfirmLabel
    TurnAPText / TurnAPPip_j / TurnAPPipUsed_j

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

try:
    from hud04_plate_art import PLATE_ART  # noqa: E402
except ImportError:
    PLATE_ART = {}

try:
    from hud04_zorder import Z_ORDER  # noqa: E402
except ImportError:
    Z_ORDER = {}

ASSET = "WBP_CombatHUD04"
ART = "/Game/SVN/OutSideAsset/AICreation/UI/HUD04"

def z_of(plate, element, fallback):
    """이 구역이 몇 층인가. 쪽에서 정한 것이 있으면 그것.

    층을 코드에 못 박아 두면 겹치는 차례를 바꿀 때마다 사람을 불러야 한다.
    쪽에서 눈으로 보며 정하는 편이 빠르다.

    이 판에 없으면 묶음의 본에서 물려받는다. 여섯 장이 한 카드이므로 층만
    판마다 따로 놀면 어느 카드에서는 글자가 배지 밑으로 들어간다.
    """
    for where in (plate, TEMPLATE_OF.get(plate)):
        got = Z_ORDER.get(where, {}).get(element) if where else None
        if isinstance(got, int):
            return got
    return fallback


def draw_badge(blueprint, parent, origin, widget_name, plate_name, element,
               rect):
    """배지 한 장. 얹은 그림이 있으면 그것을, 없으면 둥근 배지를.

    맞춤을 여기서도 지킨다 -- 배지 그림이 세로로 긴 것들이라(778x938 같은)
    구역에 늘여 넣으면 눌린 보석이 된다.
    """
    entry = zone_art_entry(plate_name, element)
    placed = local(rect, origin)
    if entry:
        placed = fit_rect(placed, entry.get("size"),
                          entry.get("fit") or "contain")
    x, y, w, h = placed
    kit.image(blueprint, widget_name, parent, x, y, w, h, None,
              z_order=z_of(plate_name, element, Z_CONTENT),
              texture=art_path(entry["texture"]) if entry else COST_BADGE,
              tint=kit.WHITE)


def art_path(name):
    """HUD04 폴더의 그림 이름을 엔진 경로로. 없으면 None."""
    return "{}/{}".format(ART, name) if name else None


#: 명령 카드 여섯의 본. prepare_hud04.py 가 자리를 이 판으로 통일해 둔다.
CARD_TEMPLATE = "action_top"

#: 아군 칸 셋의 본. 카드와 같은 까닭으로 그림도 한 벌만 쓴다.
PARTY_TEMPLATE = "bottom_status_left"

#: 명령 카드 여섯이 함께 쓰는 판 그림.
#:
#: 시안은 카드마다 따로 그려 줬지만 자리만 다른 같은 카드다. 여섯 벌을 두면
#: 몰딩이 조금씩 달라서 한 줄에 놓았을 때 눈에 걸린다.
CARD_PLATE = "KK_HUD04_action_top"

#: 비용 배지. 시안1 부터 쓰던 둥근 배지를 그대로 쓴다.
COST_BADGE = "/Game/SVN/OutSideAsset/AICreation/UI/Common/KK_Badge_Round"

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
#: AP 낱개를 몇 개까지 그리나. 넘으면 낱개는 안 그리고 숫자만 남는다.
AP_PIPS = 10

#: 메뉴 넷의 누를 자리. 왼쪽부터 지도 · 스킬 · 가방 · 설정.
MENU_ZONES = ("menu_map", "menu_skill", "menu_bag", "menu_settings")

#: 턴 순서 초상 칸 수. 판 그림에 그려진 칸과 같아야 한다.
TURN_SLOTS = 6

#: 양끝 넘김칸. 그쪽에 가려진 수를 적는다. (구역, 위젯 이름)
TURN_PAGES = (("end_left", "TurnPageLeft"), ("end_right", "TurnPageRight"))

#: AP 숫자판 그림의 원래 크기. 비율을 지켜 앉히는 데 쓴다.
AP_NUMBER_SIZE = (1689, 584)

#: AP 낱개 그림의 원래 크기. 비율을 지켜 앉히는 데 쓴다.
AP_PIP_SIZE = (778, 938)

#: 아군 칸 위에 서는 상태 홈 수. 구역이 있는 만큼만 그린다.
STATUS_SLOTS = 5

#: 가운데 AP 막대의 판 이름.
AP_BAR = "bottom_center_ap_bar"

#: 확정 단추와 턴 종료 사이 틈.
CONFIRM_GAP = 8


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


#: 묶인 판 -> 그 묶음의 본. 그림을 본에서 물려받는다.
#:
#: 한 판에만 그림을 얹어도 묶음 전체가 같아야 한다 -- 여섯 장이 한 카드이므로
#: 카드 0번에만 보석이 뜨고 나머지 다섯에 안 뜨면 통일한 뜻이 없다.
TEMPLATE_OF = {}
for _group, _template in ((COMMANDS, CARD_TEMPLATE), (PARTY, PARTY_TEMPLATE)):
    for _row in _group:
        TEMPLATE_OF[_row[0]] = _template


def fit_rect(rect, size, fit):
    """그림을 구역 안에 어떻게 앉힐지 계산한다.

    쪽에서 고른 맞춤이 게임에서는 아무 일도 안 하고 있었다 -- 쪽은 CSS 로
    비율을 지켜 그리는데 굽는 쪽은 구역에 늘여 넣었다. 눈으로 맞춘 것과 나온
    것이 달라지는 자리다.

        채우기  구역에 꽉 늘린다. 비율이 깨진다
        맞추기  비율을 지켜 안에 들어간다. 남는 쪽에 여백
        덮기    비율을 지켜 꽉 채운다. 넘치는 쪽은 삐져나온다
        가운데  원래 크기 그대로 가운데
    """
    x, y, w, h = rect
    if not size or fit == "fill":
        return rect
    art_w, art_h = size
    if fit == "none":
        return (x + (w - art_w) / 2.0, y + (h - art_h) / 2.0, art_w, art_h)
    ratio = min(w / art_w, h / art_h) if fit == "contain"         else max(w / art_w, h / art_h)
    got_w, got_h = art_w * ratio, art_h * ratio
    return (x + (w - got_w) / 2.0, y + (h - got_h) / 2.0, got_w, got_h)


def zone_art_entry(plate, element):
    """얹은 그림 한 줄. 이 판에 없으면 묶음의 본에서 물려받는다."""
    for where in (plate, TEMPLATE_OF.get(plate)):
        got = ZONE_ART.get(where, {}).get(element) if where else None
        if got:
            return got
    return None


def zone_art(plate, element):
    """구역 조정 쪽에서 얹은 그림 이름. 없으면 None.

    쪽에서 고른 것이 시안에서 뗀 조각보다 앞선다 -- 시안 것은 처음 자리를 잡을
    때 쓴 밑그림이고, 쪽에서 고른 것은 실제로 넣기로 정한 그림이다.

    이 판에 없으면 묶음의 본에서 물려받는다.
    """
    got = zone_art_entry(plate, element)
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
          texture_name=None, art_size=None):
    # 갈아 끼운 것이 가장 앞선다. 그다음이 부르는 쪽이 짚어 준 것, 마지막이
    # 시안에서 오려 낸 그 판의 것이다.
    texture_name = PLATE_ART.get(plate_name) or texture_name
    """판 껍데기 한 장. 늘리지 않고 오려 낸 크기 그대로 놓는다."""
    x, y, w, h = local(at(PLACE[plate_name]), origin)
    # 그림 크기를 알려 주면 비율을 지켜 안에 앉힌다. 갈아 끼운 그림이 판과
    # 비율이 다를 때 늘리면 몰딩이 뭉갠다.
    if art_size:
        x, y, w, h = fit_rect((x, y, w, h), art_size, "contain")
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
    entry = zone_art_entry(plate_name, element)
    name, sprite_rect = sprite(plate_name, element)
    if name is None:
        # 이 판에서 안 뗐으면 묶음의 본 것을 쓴다. 여섯이 한 카드이므로
        # 밑그림도 한 벌이어야 한다.
        name, sprite_rect = sprite(TEMPLATE_OF.get(plate_name) or "", element)
    if entry:
        name = entry["texture"]
    if name is None:
        return False
    rect = spot(plate_name, element) or sprite_rect
    x, y, w, h = local(rect, origin)
    if entry:
        # 쪽에서 고른 맞춤을 지킨다. 안 지키면 눈으로 맞춘 것과 나온 것이 다르다.
        x, y, w, h = fit_rect((x, y, w, h), entry.get("size"),
                              entry.get("fit") or "contain")
    kit.image(blueprint, widget_name, parent, x, y, w, h, None,
              z_order=z_of(plate_name, element, z),
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
         align=None, bold=False, zone=None):
    """글자 한 줄. zone 은 (판, 요소) -- 층을 쪽에서 정할 수 있게 짚어 준다."""
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
    kit.place(blueprint, name, x, y, w, h, "tl", None,
              z_of(zone[0], zone[1], Z_TEXT) if zone else Z_TEXT)
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
    for index in range(TURN_SLOTS):
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
        # 초상은 밑그림 없이 빈 칸으로 굽는다.
        #
        # 시안에서 뗀 조각을 깔아 봤는데, 그 조각은 옛 판의 자리에서 잘린
        # 것이라 얼굴 옆에 칸 사이 화살표까지 붙어 있다. 새 판의 액자에
        # 넣으면 얼굴이 한쪽으로 밀리고 화살표가 옆 칸을 침범한다.
        #
        # 액자는 판 그림에 이미 있고 얼굴은 런타임이 넣는다. 밑그림이 할
        # 일이 없다.
        kit.image(blueprint, "TurnPortrait_%d" % index, token,
                  0, 0, w, h, None, z_order=Z_CONTENT)
        # 접어 둔다. 브러시가 없는 이미지는 **흰 네모**로 그려져서, 펴 둔 채로
        # 구우면 액자마다 흰 판이 박힌다. 런타임이 얼굴을 넣을 때 편다.
        kit.fold(blueprint, "TurnPortrait_%d" % index)
        # 두르는 자리는 **칸마다 따로** 잰다.
        #
        # 전에는 한 벌을 여섯이 같이 썼다. 칸 간격이 고르면 그래도 되는데,
        # 구역 쪽에서 칸 하나를 옮기면 그 칸만 테가 어긋난다 -- 옮길 수 있게
        # 만들어 놓고 테는 못 옮기게 둔 셈이었다.
        #
        # 못 찾으면 초상 자리를 그대로 두른다.
        outline(blueprint, token, token_origin, "TurnCurrent_%d" % index,
                DETAIL["top_center_turn_order"].get("selected_outline_%02d" % (index + 1))
                or DETAIL["top_center_turn_order"][element])

    # 양끝 넘김칸. 여섯 칸에 안 들어가는 수를 적고, 누르면 창이 그쪽으로
    # 옮겨 간다. 대부분의 판에서는 비어 있다 -- 런타임이 접어 둔다.
    for element, page_name in TURN_PAGES:
        rect = local(spot("top_center_turn_order", element), turn_origin)
        if rect is None:
            continue
        px, py, pw, ph = rect
        kit.label(blueprint, page_name + "Text", turn, px, py, pw, ph,
                  "", 16, TEXT_PALE, "center", None, bold=True)
        kit.place(blueprint, page_name + "Text", px, py, pw, ph, "tl", None,
                  Z_TEXT)
        kit.fold(blueprint, page_name + "Text")
        kit.ghost_button(blueprint, page_name, turn, px, py, pw, ph)
        kit.fold(blueprint, page_name)

    # 목표 글자가 있던 자리다. 그 칸을 메뉴 넷으로 쓰기로 했다.
    #
    # 아이콘은 막대 그림 안에 이미 그려져 있으므로 여기서는 누를 자리만 그
    # 위에 얹는다 -- 아이콘을 따로 오려 놓으면 두 벌을 맞춰야 한다.
    obj, obj_origin, _ = group(blueprint, root, "ObjectivePanel",
                               "top_right_parchment")
    plate(blueprint, obj, obj_origin, "top_right_parchment", "ObjectivePlate")
    for index, element in enumerate(MENU_ZONES):
        rect = local(spot("top_right_parchment", element), obj_origin)
        if rect is None:
            continue
        kit.ghost_button(blueprint, "MenuButton_%d" % index, obj,
                         rect[0], rect[1], rect[2], rect[3])


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
        # 여섯이 한 판을 나눠 쓴다. 갈아 끼운 것이 있으면 그것으로.
        plate(blueprint, card, origin, plate_name, "CommandPlate_%d" % index,
              PLATE_ART.get(CARD_TEMPLATE) or CARD_PLATE)

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
             spot(plate_name, "action_name"), name, 18, TEXT_PALE, bold=True,
             zone=(plate_name, "action_name"))
        text(blueprint, "CommandDamage_%d" % index, card, origin,
             spot(plate_name, "damage_text") or spot(plate_name, "stance_text"),
             damage, 15, TEXT_PALE, zone=(plate_name, "damage_text"))
        # 쿨타임도 배지 위에 숫자를 얹는다. 비용 배지 아래에 같은 모양으로
        # 놓아, 둘이 한 쌍으로 읽히게 한다.
        cool_badge = spot(plate_name, "cooldown_badge")
        if cool_badge:
            draw_badge(blueprint, card, origin,
                       "CommandCooldownBadge_%d" % index, plate_name,
                       "cooldown_badge", cool_badge)
        text(blueprint, "CommandCooldown_%d" % index, card, origin,
             cool_badge or spot(plate_name, "cooldown_text"),
             cooldown, 19, TEXT_PALE, bold=True,
             zone=(plate_name, "cooldown_badge"))
        # 비용은 배지 그림 위에 숫자를 얹는다. 시안은 판에 배지를 그려
        # 넣었는데, 판을 한 장으로 줄이면서 그 배지도 한 벌만 남았다.
        # 숫자만 얹으면 판마다 배지가 있는 자리와 없는 자리가 갈린다.
        badge = spot(plate_name, "cost_badge")
        if badge:
            draw_badge(blueprint, card, origin, "CommandCostBadge_%d" % index,
                       plate_name, "cost_badge", badge)
        text(blueprint, "CommandCost_%d" % index, card, origin,
             badge, cost, 19, TEXT_PALE, bold=True,
             zone=(plate_name, "cost_badge"))

        # 골라진 표시는 안 만든다. 스킬을 고르는 순간 조준에 들고, 조준 중에는
        # 카드가 통째로 비킨다 -- 금테가 켜지자마자 카드와 같이 사라져서
        # 한 프레임 반짝이는 것이 전부였다.
        #
        # 가림막은 판 그림을 한 장 더 깔고 어둡게 물들인다. 검은 네모를 덮으면
        # 판 밖까지 사각형으로 덮여 시안과 모양이 달라진다.
        kit.image(blueprint, "CommandDisabled_%d" % index, card, 0, 0,
                  size[0], size[1], None, z_order=Z_MARK,
                  texture="{}/{}".format(
                      ART, PLATE_ART.get(CARD_TEMPLATE) or CARD_PLATE),
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
              PLATE_ART.get(PARTY_TEMPLATE) or TEXTURE[PARTY_TEMPLATE])

        # 초상과 하트도 piece() 로 간다. 그래야 쪽에서 얹은 그림과 맞춤을
        # 탄다 -- 따로 그리고 있었더니 넣어 준 하트가 게임에 안 들어갔다.
        piece(blueprint, card, origin, "PartyPortrait_%d" % index, plate_name,
              "party_portrait")
        # 구역을 지웠으면 안 그린다. piece() 는 구역이 없으면 시안에서 잘려
        # 나온 자리로 떨어지는데, 그것은 옛 판의 자리라 엉뚱한 곳에 뜬다.
        if spot(plate_name, "hp_icon") is not None:
            piece(blueprint, card, origin, "PartyHPIcon_%d" % index,
                  plate_name, "hp_icon")
        # 상태 칸은 **홈과 아이콘이 따로**다.
        #
        # 홈(status_icon…)은 늘 켜 두는 빈 액자고, 아이콘(status_icon_img…)은
        # 런타임이 무엇에 걸렸는지에 따라 갈아 끼우는 그림이다. 한 칸으로
        # 묶어 두면 상태가 없을 때 액자까지 사라져 위쪽이 뻥 뚫린다.
        #
        # 런타임은 아직 카드마다 아이콘 **하나**만 안다. 나머지 홈은 빈 액자로
        # 서 있다 -- 여럿을 켜려면 C++ 이 이름을 두 겹으로 찾아야 한다.
        for slot in range(STATUS_SLOTS):
            suffix = "" if slot == 0 else "_%d" % slot
            # 구역이 있는 만큼만 그린다. 그림은 묶음의 본에서 물려받으므로
            # 구역을 지워도 그림은 남아 있다 -- 그것만 보고 그리면 자리가
            # 없는 홈을 그리려다 죽는다.
            if spot(plate_name, "status_icon" + suffix) is None:
                continue
            piece(blueprint, card, origin,
                  "PartyStatusFrame_%d_%d" % (index, slot),
                  plate_name, "status_icon" + suffix)
        # 아이콘은 홈마다 하나씩. 런타임이 걸린 순서대로 앞에서부터 켠다.
        #
        # 밑그림을 깔아 둔다. 브러시가 없는 이미지는 흰 네모로 그려져서, 접힌
        # 것을 런타임이 펴는 순간 하얀 판이 튀어나온다 -- 턴 초상에서 겪었다.
        placeholder, _ = sprite("bottom_status_left", "status_icon")
        for slot in range(STATUS_SLOTS):
            suffix = "" if slot == 0 else "_%d" % slot
            icon_zone = spot(plate_name, "status_icon_img" + suffix)
            if icon_zone is None:
                continue
            ix, iy, iw, ih = local(icon_zone, origin)
            icon_name = "PartyStatusIcon_%d_%d" % (index, slot)
            kit.image(blueprint, icon_name, card, ix, iy, iw, ih, None,
                      z_order=Z_CONTENT,
                      texture="{}/{}".format(ART, placeholder) if placeholder
                      else None, tint=kit.WHITE)
            kit.fold(blueprint, icon_name)

        text(blueprint, "PartyName_%d" % index, card, origin,
             spot(plate_name, "character_name"), name, 17, TEXT_PALE,
             align="left", bold=True)
        text(blueprint, "PartyHPText_%d" % index, card, origin,
             spot(plate_name, "hp_value"), hp, 15, TEXT_PALE, align="left")
        text(blueprint, "PartyStatus_%d" % index, card, origin,
             spot(plate_name, "status_text"), status, 14, TEXT_PALE,
             align="left")

        # HP 막대는 그림 틀 안에 든다. 막대만 두면 카드 안에서 홀로 떠
        # 보이고, 글자를 밖에 두면 좁은 세로 카드에서 놓을 자리가 없다.
        bar = local(spot(plate_name, "hp_bar"), origin)
        if bar:
            bx, by, bw, bh = bar
            kit.image(blueprint, "PartyHPPlate_%d" % index, card,
                      bx, by, bw, bh, None, z_order=Z_PLATE + 1,
                      texture="{}/KK_HUD04_hp_bar_plate".format(ART),
                      tint=kit.WHITE)
            inset = bh * 0.18
            kit.bar(blueprint, "PartyHPBar_%d" % index, card,
                    bx + inset, by + inset, bw - inset * 2, bh - inset * 2,
                    unreal.LinearColor(0.44, 0.74, 0.24, 1.0))

        # AP 줄은 숫자판 하나와 낱개 열이다.
        #
        # 자리를 여기서 계산하지 않고 **구역에서 읽는다.** 전에는 ap_gems 한
        # 칸을 받아 열로 나눴는데, 그러면 구역 쪽에서 낱개 하나를 못 옮긴다 --
        # 열 개를 따로 놓고 싶어도 손댈 자리가 없었다.
        #
        # 숫자판은 늘 켜 둔다. 낱개만 있으면 여덟인지 아홉인지 세어야 한다.
        #
        # 쓴 칸과 남은 칸은 그림이 다르다. 런타임이 브러시를 갈아 끼우는 대신
        # 두 장을 겹쳐 놓고 하나만 켠다 -- 그래야 텍스처 경로가 굽는 쪽에만
        # 있고 C++ 은 이름만 알면 된다.
        number = local(spot(plate_name, "ap_number"), origin)
        if number:
            nx, ny, nw, nh = fit_rect(number, AP_NUMBER_SIZE, "contain")
            kit.image(blueprint, "PartyAPPlate_%d" % index, card,
                      nx, ny, nw, nh, None, z_order=Z_CONTENT,
                      texture="{}/KK_HUD04_ap_number_plate".format(ART),
                      tint=kit.WHITE)
            # 글자는 제 칸이 있으면 그 칸에 넣는다. 숫자판 그림은 비율을
            # 지키느라 칸보다 작아지는데, 글자까지 따라 줄면 판 밖으로 삐져
            # 나가거나 가운데가 안 맞는다.
            words = local(spot(plate_name, "ap_number_txt"), origin) \
                or (nx, ny, nw, nh)
            kit.label(blueprint, "PartyAPText_%d" % index, card,
                      words[0], words[1], words[2], words[3], "4/4", 13,
                      TEXT_PALE, "center", None, bold=True)
            # 세로도 가운데로 붙인다. place() 는 칸의 왼쪽 위에 글자를 놓고
            # justification 은 가로만 옮겨서, 칸이 높으면 글자가 위로 뜬다 --
            # AP 숫자가 그렇게 떠 있었다.
            kit.place(blueprint, "PartyAPText_%d" % index,
                      words[0], words[1], words[2], words[3],
                      "tl", None, Z_TEXT)
            kit.align_in(blueprint, "PartyAPText_%d" % index,
                         words[0], words[1], words[2], words[3],
                         "center", "middle")

        for pip in range(AP_PIPS):
            cell = local(spot(plate_name, "ap_pip_%02d" % (pip + 1)), origin)
            if cell is None:
                continue
            for suffix, art in (("", "KK_HUD04_ap_pip"),
                                ("Used", "KK_HUD04_ap_pip_spent")):
                pip_name = "PartyAPPip%s_%d_%d" % (suffix, index, pip)
                # 칸에 늘려 넣지 않는다. 보석이 납작해진다 -- 자리가 좁으면
                # 작아질 뿐, 비율은 지킨다.
                px, py, pw, ph = fit_rect(cell, AP_PIP_SIZE, "contain")
                kit.image(blueprint, pip_name, card, px, py, pw, ph, None,
                          z_order=Z_CONTENT,
                          texture="{}/{}".format(ART, art), tint=kit.WHITE)
                if suffix or pip >= lit:
                    kit.fold(blueprint, pip_name)

        # 차례 표시 테두리는 안 그린다. 지금 차례인 유닛은 위쪽 턴 순서 줄이
        # 이미 보여 주므로, 아군 칸까지 노란 테를 두르면 같은 말이 두 번이다.
        kit.ghost_button(blueprint, "PartyButton_%d" % index, card, 0, 0,
                         size[0], size[1])


def ap_bar(blueprint, root):
    """지금 차례인 유닛의 행동력을 크게 보여 주는 막대.

    카드 안 숫자는 셋을 견주는 값이고, 이 막대는 **지금 쓸 수 있는 것**이다.
    같은 값을 두 번 그리는 것처럼 보이지만 보는 목적이 다르다 -- 카드는
    누구를 움직일지, 막대는 무엇을 할 수 있을지를 답한다.
    """
    if AP_BAR not in PLACE:
        return
    name, origin, _ = group(blueprint, root, "TurnAPPanel", AP_BAR)
    plate(blueprint, name, origin, AP_BAR, "TurnAPPlate")

    number = local(spot(AP_BAR, "ap_number"), origin)
    if number:
        nx, ny, nw, nh = fit_rect(number, AP_NUMBER_SIZE, "contain")
        kit.image(blueprint, "TurnAPNumberPlate", name, nx, ny, nw, nh, None,
                  z_order=Z_CONTENT,
                  texture="{}/KK_HUD04_ap_number_plate".format(ART),
                  tint=kit.WHITE)
        words = local(spot(AP_BAR, "ap_number_txt"), origin) or (nx, ny, nw, nh)
        kit.label(blueprint, "TurnAPText", name, words[0], words[1], words[2],
                  words[3], "10/10", 20, TEXT_PALE, "center", None, bold=True)
        kit.place(blueprint, "TurnAPText", words[0], words[1], words[2],
                  words[3], "tl", None, Z_TEXT)
        kit.align_in(blueprint, "TurnAPText", words[0], words[1], words[2],
                     words[3], "center", "middle")

    for pip in range(AP_PIPS):
        cell = local(spot(AP_BAR, "ap_pip_%02d" % (pip + 1)), origin)
        if cell is None:
            continue
        for suffix, art in (("", "KK_HUD04_ap_pip"),
                            ("Used", "KK_HUD04_ap_pip_spent")):
            pip_name = "TurnAPPip%s_%d" % (suffix, pip)
            px, py, pw, ph = fit_rect(cell, AP_PIP_SIZE, "contain")
            kit.image(blueprint, pip_name, name, px, py, pw, ph, None,
                      z_order=Z_CONTENT,
                      texture="{}/{}".format(ART, art), tint=kit.WHITE)
            if suffix:
                kit.fold(blueprint, pip_name)


def end_turn(blueprint, root):
    name, origin, size = group(blueprint, root, "EndTurnPanel",
                               "bottom_right_button")
    plate(blueprint, name, origin, "bottom_right_button", "EndTurnPlate")
    text(blueprint, "EndTurnLabel", name, origin,
         spot("bottom_right_button", "button_label"), "턴 종료", 24, TEXT_PALE,
         bold=True)
    kit.ghost_button(blueprint, "EndTurnButton", name, 0, 0, size[0], size[1])

    # 확정 단추는 턴 종료 바로 위에 같은 크기로 선다.
    #
    # 평소에는 접혀 있다. 스킬을 고르고 칸을 짚어 공격 범위가 뜬 그때만
    # 펴진다 -- 늘 떠 있으면 무엇을 확정하는 단추인지 읽히지 않는다.
    px, py, pw, ph = at(PLACE["bottom_right_button"])
    py -= ph + CONFIRM_GAP
    kit.add(blueprint, "CanvasPanel", "ConfirmPanel", root)
    kit.place(blueprint, "ConfirmPanel", px, py, pw, ph, "tl", None, Z_PLATE)
    kit.image(blueprint, "ConfirmPlate", "ConfirmPanel", 0, 0, pw, ph, None,
              z_order=Z_PLATE,
              texture="{}/KK_HUD04_confirm_button".format(ART), tint=kit.WHITE)
    label = local(spot("bottom_right_button", "button_label"),
                  at(PLACE["bottom_right_button"])[:2])
    if label:
        kit.label(blueprint, "ConfirmLabel", "ConfirmPanel", label[0], label[1],
                  label[2], label[3], "확정", 24, TEXT_PALE, "center", None,
                  bold=True)
        kit.place(blueprint, "ConfirmLabel", label[0], label[1], label[2],
                  label[3], "tl", None, Z_TEXT)
        kit.align_in(blueprint, "ConfirmLabel", label[0], label[1], label[2],
                     label[3], "center", "middle")
    kit.ghost_button(blueprint, "ConfirmButton", "ConfirmPanel", 0, 0, pw, ph)
    kit.fold(blueprint, "ConfirmPanel")


kit.reset_ledger()
bp = kit.create_asset(ASSET)
kit.add(bp, "CanvasPanel", "RootCanvas", "")
top_row(bp, "RootCanvas")
enemy_panel(bp, "RootCanvas")
commands(bp, "RootCanvas")
party(bp, "RootCanvas")
ap_bar(bp, "RootCanvas")
end_turn(bp, "RootCanvas")
# 저장 전에 컴파일한다.
#
# 위젯을 트리에 붙이는 것만으로는 **생성 클래스**가 안 바뀐다. CreateWidget 은
# 그 클래스를 쓰므로, 컴파일을 빠뜨리면 새로 붙인 위젯이 트리에는 있는데
# 화면에는 없다 -- AP 낱개 스무 장이 그래서 안 보였다. 자리도 그림도 표시
# 상태도 다 맞는데 안 그려져서 한참 헤맸다.
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
unreal.EditorAssetLibrary.save_loaded_asset(bp, False)
kit.commit_asset(ASSET)
unreal.log("[HUD04] {} 구움".format(ASSET))
