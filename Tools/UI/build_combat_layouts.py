"""Build all ten combat HUD layout variants as real Widget Blueprints.

One layout = one WBP. They share the components in combat_layout_kit so that
only the arrangement differs and the comparison is about placement, not about
one of them happening to have nicer cards.

Proportions are read off the ten KayKit mock-ups. The mock-ups are the finish
target, not the source of layout -- the arrangements were already chosen and
running; what the mock-ups settle is how much room each block gets.

Each layout is a function that composes the kit's parts on a root canvas. A
layout omits whatever it does not want; UCombatLayoutHUDWidget finds widgets by
name and skips the missing ones, so dropping the enemy panel or the skill names
is a design choice rather than a broken build.

Run through Tools/RunEditorPython.ps1 -- a bare -ExecutePythonScript exits 0
even when this raises.
"""
import math
import os
import sys

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import combat_layout_kit as kit  # noqa: E402

W, H = kit.CANVAS_W, kit.CANVAS_H
M = kit.MARGIN


def centred(count, item, gap, left=0.0, right=W):
    """Where a row of `count` items starts so the row is centred in a span."""
    row = count * item + (count - 1) * gap
    return left + (right - left - row) / 2.0


def top_band(bp, root, turn_token=145.0, names=False):
    """Round counter left, turn order centred, objective right.

    Nine of the ten mock-ups share this band unchanged, so it lives in one
    place -- otherwise a tweak to the round panel has to be made nine times and
    the layouts stop being comparable the moment one of them is missed.
    """
    # 시안 실측(1672)을 1920 으로 환산해 넣는다. 이전 값은 내가 고른 것이라
    # 양피지 판이 시안의 절반 높이(42 대 87)로 나왔다.
    #   라운드 판  양피지 206x87  -> 판 268x100
    #   목표 현판  양피지 323x88  -> 판 396x100, 시안보다 41px 왼쪽에서 시작
    #   턴 칸      시안 다섯 칸이 554 를 채운다 -> 칸 127
    # 1672 캡처에서 시안 외곽은 245x120. card()가 정보판에 더하는 외곽을
    # 역산하면 본체 270x128이 정확히 그 크기가 된다.
    kit.round_panel(bp, root, 17, 14, 270, 128, "tl", 30)
    # 시안 턴 칸은 가로 138px, 세로 120px인 직사각형이다. 정사각형 145를
    # 넣으면 폭은 가까워져도 높이가 27px 넘친다. 프레임 외곽과 선택 테두리
    # 여유까지 역산한 본체가 148x114다.
    turn_x = centred(5, turn_token, 8) - 7.5
    kit.turn_row(bp, root, turn_x, 18, turn_token, 8, "tc",
                 count=5, names=names, token_h=114.0)
    kit.objective_panel(bp, root, 1495, 14, 398, 123, "tr", 20)


# ─── 1안 클래식 CRPG ───────────────────────────────────────────────────────────

def layout_01(bp, root):
    """Three layers: top band, battlefield, bottom band. The safe, dense one.

    The allies stack as three wide rows down the bottom-left rather than
    sitting side by side -- stacked they eat less width and the command rail
    gets the room, which is what the mock-up is really trading for.
    """
    top_band(bp, root, 148.0)

    # 아래 수치는 전부 시안(KK_HUD_Polish_01)을 1920x1080 에서 실측한 값이다.
    # 이전 값들은 여백 상수와 centred() 로 스스로 계산한 것이라 시안과 맞을
    # 이유가 없었다 -- 실제로 아군 판이 44px 위, 스킬 줄이 50px 오른쪽,
    # 적 패널이 72px 아래에 있었다.
    #
    #   아군 판  x  24  y  668   497 x 384 (3행)
    #   스킬 줄  x 520  y  709   960 x 349 (6장)
    #   적 패널  x1560  y  709   349 x 223
    #   턴 종료  x1564  y  948   332 x 100
    # 1672 실측: 시안 행의 나무 면이 107px 이고 행 사이 틈이 5px 인데,
    # 우리는 95px 에 틈 17px 이었다. 전체 높이는 비슷한데 행이 얇고 틈이 넓어
    # 안이 비어 보인다. 프레임이 위아래로 6px 씩 먹으므로 카드는 그만큼 크게
    # 잡고 틈은 없앤다.
    # 시안 실측(1672): 세 줄이 591~917 을 채우고 아래 24px 이 비어 있다.
    # 1920 으로 환산하면 678 에서 시작해 줄 하나가 125 다.
    row_w, row_h, row_gap = 487.0, 124.0, 0.0
    party_top = 672.0
    for i in range(3):
        kit.party_card(bp, root, i, M, party_top + i * (row_h + row_gap),
                       row_w, row_h, "bl", "strip")

    # 판 상자가 아니라 '보이는 면'이 시안과 같아야 한다. 프레임이 안쪽으로
    # 17px 을 먹으므로 상자를 그만큼 키워 면이 1560,709 에서 349x223 으로
    # 나오게 한다.
    # 스킬 줄 마지막 카드가 1564 에서 끝나므로 1560 에서 시작하면 4px 겹친다.
    enemy_x, enemy_y = 1572.0, 709.0
    enemy_w, enemy_h = 316.0, 219.0
    end_x, end_y = 1543.0, 956.5
    end_w, end_h = 350.0, 100.0

    # 같은 이유로 12px 씩 키운다. 시안의 석재 면은 520~1500 을 채운다.
    cmd_w, cmd_h, cmd_gap = 161.5, 351.0, 6.0
    start, cmd_top = 550.0, 707.0
    for i in range(6):
        kit.command_card(bp, root, i, start + i * (cmd_w + cmd_gap),
                         cmd_top, cmd_w, cmd_h, "bc")

    kit.enemy_panel(bp, root, enemy_x, enemy_y, enemy_w, enemy_h, "br")
    kit.end_turn(bp, root, end_x, end_y, end_w, end_h, "br", 26)


# ─── 2안 좌측 세로 파티 ────────────────────────────────────────────────────────

def layout_02(bp, root):
    """Allies down the left edge, enemy down the right, one wide rail below.

    The ally cards sit high on the left rather than at the bottom, which frees
    the whole bottom edge for a rail of six large cards.
    """
    top_band(bp, root)

    card_w, card_h, gap = 420.0, 176.0, 14.0
    for i in range(3):
        kit.party_card(bp, root, i, M, 140 + i * (card_h + gap),
                       card_w, card_h, "tl", "strip")

    enemy_w = 340.0
    kit.enemy_panel(bp, root, W - M - enemy_w, 140, enemy_w, 470, "tr", "tall")

    bottom = H - 24
    end_w = 240.0
    cmd_w, cmd_h, cmd_gap = 196.0, 200.0, 12.0
    start = centred(6, cmd_w, cmd_gap, M, W - M - end_w - 20)
    for i in range(6):
        kit.command_card(bp, root, i, start + i * (cmd_w + cmd_gap),
                         bottom - cmd_h, cmd_w, cmd_h, "bc")
    kit.end_turn(bp, root, W - M - end_w, bottom - 96, end_w, 96, "br", 26)


# ─── 3안 활성 유닛 집중 ────────────────────────────────────────────────────────

def layout_03(bp, root):
    """The unit on turn gets a hero panel; the other two shrink to strips."""
    top_band(bp, root)

    # 아래 수치는 시안(KK_HUD_Polish_03)을 1672 화면에서 재고 1920 설계로
    # 환산한 것이다. 바닥에서 역산하던 이전 값은 왼쪽 열을 180px 아래로
    # 밀어 놓았고 활성 카드를 120px 크게 만들었다.
    #
    #   칩 두 장   x  21  y 313   365 x 230 (둘 합쳐)
    #   활성 카드  x  11  y 556   451 x 499
    #   스킬 줄    x 467  y 737  1109 x 315
    #   적 패널    x1543  y 660   369 x 243
    #   턴 종료    x1550  y 918   335 x 124
    hero_w, hero_h = 451.0, 499.0
    kit.party_card(bp, root, 0, 22, 556, hero_w, hero_h, "bl", "hero")

    strip_w, strip_h, gap = 365.0, 108.0, 14.0
    for i in (1, 2):
        kit.party_card(bp, root, i, 22, 313 + (i - 1) * (strip_h + gap),
                       strip_w, strip_h, "bl", "strip")

    enemy_x, enemy_y, enemy_w, enemy_h = 1543.0, 660.0, 369.0, 243.0
    end_x, end_y, end_w, end_h = 1550.0, 918.0, 335.0, 124.0

    cmd_gap = 10.0
    cmd_w = (1109.0 - 5 * cmd_gap) / 6.0
    cmd_h, cmd_top, start = 315.0, 737.0, 467.0
    for i in range(6):
        kit.command_card(bp, root, i, start + i * (cmd_w + cmd_gap),
                         cmd_top, cmd_w, cmd_h, "bc")

    kit.enemy_panel(bp, root, enemy_x, enemy_y, enemy_w, enemy_h, "br")
    kit.end_turn(bp, root, end_x, end_y, end_w, end_h, "br", 26)


# ─── 4안 방사형 컨텍스트 메뉴 ──────────────────────────────────────────────────

def layout_04(bp, root):
    """Command cards orbit the unit on the battlefield.

    The mock-up keeps them as cards rather than bare icons, so the name and
    the damage line stay readable while the ring still frees the screen edges.
    The ring is an ellipse, not a circle: the screen is wider than it is tall,
    and a circle would push the top and bottom cards into the bands.
    """
    top_band(bp, root)

    # 시안 실측(1672): 카드 152x172, 고리 중심 (831,469), 반경 가로 186 세로 248.
    # 1920 으로 환산하면 카드 174x197, 반경 214/285 다.
    #
    # 우리 것은 카드가 34px 납작했고 고리가 가로로 퍼지고 세로로 눌려 있었다.
    # 타원을 화면 비율에 맞춘다고 가로를 늘렸는데, 시안은 오히려 세로로 긴
    # 타원이다 -- 위아래 띠를 피하는 것보다 카드가 전장을 덜 가리는 쪽을
    # 택한 것으로 보인다.
    cx, cy = W * 0.5, H * 0.498
    cmd_w, cmd_h = 174.0, 197.0
    radius_x, radius_y = 214.0, 285.0
    for i in range(6):
        angle = math.radians(-90 + i * 60)
        kit.command_card(bp, root, i,
                         cx + radius_x * math.cos(angle) - cmd_w / 2.0,
                         cy + radius_y * math.sin(angle) - cmd_h / 2.0,
                         cmd_w, cmd_h, "mc", "compact")

    # 남은 행동력을 고리 한가운데에. 고를 때 제일 먼저 보는 숫자다.
    kit.label(bp, "PartyAPText_0", root, cx - 90, cy - 34, 180, 60, "0/0", 42,
              kit.GOLD, "center", None, True)

    chip_w, chip_h = 236.0, 92.0
    bottom = H - 24
    for i in range(3):
        kit.party_card(bp, root, i, M + i * (chip_w + 10), bottom - chip_h,
                       chip_w, chip_h, "bl", "chip")

    kit.enemy_panel(bp, root, W - M - 340, 148, 340, 210, "tr")
    kit.end_turn(bp, root, W - M - 260, bottom - 88, 260, 88, "br", 26)


# ─── 5안 하단 통합 바 ──────────────────────────────────────────────────────────

def layout_05(bp, root):
    """Everything you press lives in one thick bar across the bottom."""
    top_band(bp, root)

    # 시안(KK_HUD_Polish_05)을 1672 화면에서 재고 1920 설계로 환산했다.
    # 바닥에서 역산하던 값은 바를 85px 낮게, 적 패널을 절반 크기로 만들었다.
    #
    #   하단 바   x  57  y 691  1807 x 356
    #   스킬 줄   x 454  y 727  1141 x 297   (바 안쪽 좌표로는 397, 36)
    #   턴 종료   x1615  y 730   247 x 269   (바 안쪽 1558, 39)
    #   적 패널   x1424  y 332   472 x 321
    bar_x, bar_y = 57.0, 691.0
    bar_w, bar_h = 1807.0, 356.0
    body, size = kit.card(bp, "CommandBar", root, bar_x, bar_y,
                          bar_w, bar_h, "bc", role="party")

    strip_w, strip_h, gap = 372.0, 96.0, 10.0
    for i in range(3):
        kit.party_card(bp, body, i, 26, 30 + i * (strip_h + gap),
                       strip_w, strip_h, "tl", "strip")

    cmd_gap = 10.0
    cmd_w = (1141.0 - 5 * cmd_gap) / 6.0
    cmd_h = 297.0
    for i in range(6):
        kit.command_card(bp, body, i, 397 + i * (cmd_w + cmd_gap), 36,
                         cmd_w, cmd_h, "tl", "card")

    kit.end_turn(bp, body, 1558, 39, 247, 269, "tl", 30)
    # 바 하나가 통째로 한 덩어리라 프레임도 바깥에 한 번만 두른다.
    kit.frame(bp, "CommandBar", body, bar_w, bar_h)

    kit.enemy_panel(bp, root, 1424, 332, 472, 321, "mr")


# ─── 6안 좌우 대칭 ─────────────────────────────────────────────────────────────

def layout_06(bp, root):
    """Allies down the left, the enemy down the right, facing each other."""
    top_band(bp, root)

    col_w, card_h, gap = 400.0, 196.0, 16.0
    for i in range(3):
        kit.party_card(bp, root, i, M, 130 + i * (card_h + gap),
                       col_w, card_h, "tl", "strip")

    enemy_w = 380.0
    kit.enemy_panel(bp, root, W - M - enemy_w, 130, enemy_w, 560, "tr", "tall")

    bottom = H - 24
    end_w = 220.0
    cmd_w, cmd_h, cmd_gap = 190.0, 190.0, 12.0
    start = centred(6, cmd_w, cmd_gap, M, W - M - end_w - 20)
    for i in range(6):
        kit.command_card(bp, root, i, start + i * (cmd_w + cmd_gap),
                         bottom - cmd_h, cmd_w, cmd_h, "bc")
    kit.end_turn(bp, root, W - M - end_w, bottom - 96, end_w, 96, "br", 26)


# ─── 7안 카드 핸드 ─────────────────────────────────────────────────────────────

def layout_07(bp, root):
    """Skills as a hand of cards, fanned and overlapping."""
    top_band(bp, root)

    # 기울이지 않는다.
    #
    # "손패"라는 이름 때문에 부채꼴로 펼쳤는데 시안은 카드를 똑바로 세워
    # 나란히 둔다. 기울이면 바깥쪽 카드의 이름과 AP 가 화면 아래로 나가고,
    # 실제로 그렇게 잘려 있었다. 손패다움은 카드 자체의 모양이 맡는다.
    #
    # 시안 실측(1672): 카드 줄 x 250~965 y 400~620, 파티 x 18~235 y 340~600,
    # 적 패널 x 975~1130 y 365~575, 턴 종료 x 975~1130 y 585~630.
    cmd_gap = 10.0
    cmd_w = (821.0 - 5 * cmd_gap) / 6.0
    cmd_h, cmd_top, start = 253.0, 459.0, 287.0
    for i in range(6):
        kit.command_card(bp, root, i, start + i * (cmd_w + cmd_gap),
                         cmd_top, cmd_w, cmd_h, "bc", "card")

    row_w, row_h, gap = 249.0, 99.0, 6.0
    for i in range(3):
        kit.party_card(bp, root, i, 21, 390 + i * (row_h + gap),
                       row_w, row_h, "bl", "strip")

    kit.enemy_panel(bp, root, 1120, 419, 178, 241, "br")
    kit.end_turn(bp, root, 1120, 672, 178, 52, "br", 22)


# ─── 8안 미니멀 ────────────────────────────────────────────────────────────────

def layout_08(bp, root):
    """Only the unit on turn is read out; the rest lives on the battlefield.

    Slots 1 and 2 and the enemy panel are dropped on purpose. The runtime
    tolerates the gap, so the omission is the design point rather than a hole.
    """
    # 시안(KK_HUD_Polish_08)을 1672 화면에서 재고 1920 설계로 환산했다.
    #
    #   라운드+목표  x  25  y  17   313 x 111
    #   턴 띠        x 563  y   4   517 x 155  -> 칸 112 (다섯 칸)
    #   파티 카드    x  11  y 714   389 x 196
    #   스킬 줄      x 444  y 704   914 x 213
    #   턴 종료      x1420  y 775   236 x 128
    kit.round_panel(bp, root, 26, 16, 290, 78, "tl", 22)
    kit.objective_panel(bp, root, 26, 104, 350, 62, "tl", 17, icon=False)
    kit.turn_row(bp, root, centred(5, 112, 8), 8, 112, 8, "tc", names=False)

    hero_w, hero_h = 447.0, 225.0
    kit.party_card(bp, root, 0, 13, 820, hero_w, hero_h, "bl", "hero")

    cmd_gap = 10.0
    cmd_w = (1050.0 - 5 * cmd_gap) / 6.0
    cmd_h, cmd_top, start = 245.0, 808.0, 510.0
    for i in range(6):
        kit.command_card(bp, root, i, start + i * (cmd_w + cmd_gap),
                         cmd_top, cmd_w, cmd_h, "bc")
    kit.end_turn(bp, root, 1630, 890, 271, 147, "br", 28)


# ─── 9안 상단 정보 / 하단 조작 ─────────────────────────────────────────────────

def layout_09(bp, root):
    """Everything you read is up top, everything you press is down below."""
    bar_w, info_h = W - 2 * M, 208.0
    body, size = kit.card(bp, "InfoBar", root, M, 16, bar_w, info_h, "tc",
                          role="party")

    def middle(height):
        return (info_h - height) / 2.0

    # 폭을 순서대로 배분한다. 고정 오프셋으로 쌓았더니 적 패널이 62px로
    # 짜부라져 수치가 바 밖으로 나간 적이 있다.
    # 파티 250px 에 숫자까지 넣으려니 "90/" 에서 잘렸고, 목표 현판 220px
    # 에서도 문장이 잘렸다. 라운드 판과 턴 칸을 줄여 그 폭을 넘긴다.
    round_w, strip_w, token = 140.0, 286.0, 54.0
    objective_w, gap = 250.0, 10.0

    x = 18.0
    kit.round_panel(bp, body, x, middle(72), round_w, 72, "tl", 22)
    x += round_w + gap
    for i in range(3):
        kit.party_card(bp, body, i, x + i * (strip_w + 8), middle(150),
                       strip_w, 150, "tl", "strip")
    x += 3 * strip_w + 2 * 8 + gap
    kit.turn_row(bp, body, x, middle(token), token, 5.0, "tl", names=False)
    x += 6 * token + 5 * 5 + gap

    objective_x = bar_w - 18 - objective_w
    enemy_w = objective_x - gap - x
    if enemy_w < 200.0:
        raise RuntimeError(
            "9안 상단 바가 모자란다: 적 패널 자리 {:.0f}px".format(enemy_w))
    kit.enemy_panel(bp, body, x, middle(170), enemy_w, 170, "tl")
    kit.objective_panel(bp, body, objective_x, middle(72), objective_w, 72,
                        "tl", 17)
    kit.frame(bp, "InfoBar", body, bar_w, info_h)

    bottom = H - 24
    action_h = 250.0
    action, action_size = kit.card(bp, "ActionBar", root, M,
                                   bottom - action_h, bar_w, action_h, "bc",
                                   role="party")
    end_w = 220.0
    cmd_w, cmd_h, cmd_gap = 188.0, 190.0, 12.0
    start = centred(6, cmd_w, cmd_gap, 24, bar_w - end_w - 44)
    for i in range(6):
        kit.command_card(bp, action, i, start + i * (cmd_w + cmd_gap),
                         (action_h - cmd_h) / 2.0, cmd_w, cmd_h, "tl", "card")
    kit.end_turn(bp, action, bar_w - end_w - 24, (action_h - 130) / 2.0,
                 end_w, 130, "tl", 28)
    kit.frame(bp, "ActionBar", action, bar_w, action_h)


# ─── 10안 상황 전환형 ──────────────────────────────────────────────────────────

def layout_10(bp, root):
    """The moment a skill is aimed: the bottom grows a targeting read-out.

    The read-out is the enemy panel itself rather than a second copy of the
    same numbers -- widget names are unique, so a duplicate would be a static
    label that never updates, which is worse than not showing it.
    """
    top_band(bp, root, names=True)

    # 시안(KK_HUD_Polish_10)을 1672 화면에서 재고 1920 설계로 환산했다.
    #
    #   조준 정보 띠  x 401  y 593  1223 x 134  -> 460, 681, 1404 x 154
    #   스킬 줄       x 394  y 738  1050 x 183  -> 452, 847, 1206 x 210
    #   턴 종료       x1471  y 747   174 x 154  -> 1689, 858, 200 x 177
    #   파티 세 줄    좌측, 카드 높이는 1안과 같은 계열
    cmd_gap = 12.0
    cmd_w = (1206.0 - 5 * cmd_gap) / 6.0
    cmd_h, cmd_top, start = 210.0, 847.0, 452.0
    for i in range(6):
        kit.command_card(bp, root, i, start + i * (cmd_w + cmd_gap),
                         cmd_top, cmd_w, cmd_h, "bc")

    # 조준 정보는 레일 바로 위에 레일과 같은 폭으로. 눈이 카드에서 대상까지
    # 화면을 가로지르지 않는다.
    kit.enemy_panel(bp, root, 460, 681, 1404, 154, "bc")

    row_w, row_h, gap = 440.0, 124.0, 0.0
    for i in range(3):
        kit.party_card(bp, root, i, 22, 672 + i * (row_h + gap),
                       row_w, row_h, "bl", "strip")

    kit.end_turn(bp, root, 1689, 858, 200, 177, "br", 26)


# ─── run ──────────────────────────────────────────────────────────────────────

LAYOUTS = (
    ("WBP_CombatLayout_01_ClassicCRPG", "1안 클래식 CRPG", layout_01),
    ("WBP_CombatLayout_02_LeftParty", "2안 좌측 세로 파티", layout_02),
    ("WBP_CombatLayout_03_ActiveUnit", "3안 활성 유닛 집중", layout_03),
    ("WBP_CombatLayout_04_Radial", "4안 방사형", layout_04),
    ("WBP_CombatLayout_05_BottomBar", "5안 하단 통합 바", layout_05),
    ("WBP_CombatLayout_06_Mirrored", "6안 좌우 대칭", layout_06),
    ("WBP_CombatLayout_07_CardHand", "7안 카드 핸드", layout_07),
    ("WBP_CombatLayout_08_Minimal", "8안 미니멀", layout_08),
    ("WBP_CombatLayout_09_SplitBands", "9안 정보·조작 분리", layout_09),
    ("WBP_CombatLayout_10_Targeting", "10안 상황 전환형", layout_10),
)


RECTS = {}


def build_one(asset_name, title, compose):
    kit.reset_ledger()
    blueprint = kit.create_asset(asset_name)
    kit.add(blueprint, "CanvasPanel", "RootCanvas", "")
    compose(blueprint, "RootCanvas")
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)
    # 다 짓고 저장한 뒤에야 제자리로 옮긴다. 여기 전에 터지면 기존 안이 남는다.
    kit.commit_asset(asset_name)
    RECTS[asset_name] = kit.absolute_rects()
    shown, total = kit.verify(asset_name)
    unreal.log("[Layouts] {:38s} {}  bound {}/{}".format(
        asset_name, title, shown, total))


only = None
failures = []
for asset_name, title, compose in LAYOUTS:
    if only and asset_name not in only:
        continue
    try:
        build_one(asset_name, title, compose)
    except Exception as error:
        failures.append("{}: {}".format(asset_name, error))
        unreal.log_error("[Layouts] {} FAILED: {}".format(asset_name, error))

# 위젯별 정밀 비교가 쓸 좌표 장부.
import json as _json
_out = r"D:/UnrealProjects/P_RD_develop_20260726/Saved/UI/CombatLayouts/rects.json"
try:
    os.makedirs(os.path.dirname(_out), exist_ok=True)
except NameError:
    import os
    os.makedirs(os.path.dirname(_out), exist_ok=True)
with open(_out, "w", encoding="utf-8") as _h:
    _json.dump(RECTS, _h, ensure_ascii=False, indent=1)
unreal.log("[Layouts] 좌표 장부 {} ({}개 배치안)".format(_out, len(RECTS)))

# 임시 이름이 남으면 다음 실행에서 헷갈리고 콘텐츠 브라우저도 지저분해진다.
for stray in unreal.EditorAssetLibrary.list_assets(kit.PACKAGE_PATH, False):
    if kit.BUILDING_SUFFIX in stray:
        unreal.EditorAssetLibrary.delete_asset(stray.split(".")[0])
        unreal.log("[Layouts] 임시 정리 {}".format(stray))

unreal.log("[Layouts] built {}/{}".format(
    len(LAYOUTS) - len(failures), len(LAYOUTS)))
if failures:
    raise RuntimeError("layouts failed:\n  " + "\n  ".join(failures))
unreal.log("[Layouts] done")
