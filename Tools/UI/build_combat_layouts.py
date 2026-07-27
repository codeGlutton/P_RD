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
import cutout_layout  # noqa: E402

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
    """시안에서 오려 낸 껍데기를 놓고 그 구멍에 내용을 채운다.

    조각을 만들어 이어 붙이거나 9슬라이스로 늘리는 방식으로는 손그림 품질에
    닿지 못했다. 시안 판을 통째로 오려 원래 크기 그대로 놓으면 몰딩도 비례도
    시안 그 자체이고, 우리가 맞출 것은 위치뿐이다.

    아래 좌표는 전부 조각 기준이다. 조각에 초상 자리와 카드 칸이 구멍으로
    뚫려 있어 그걸 읽었고, 글자와 바 자리는 시안에서 재어 조각 원점을 뺐다.
    내가 눈대중으로 고른 숫자가 없다 -- 이 작업에서 틀린 것의 대부분이
    그렇게 고른 값이었다.
    """
    K = kit.CHROME_SCALE
    holes = kit.CHROME_HOLES

    # ── 상단: 라운드 판 / 목표 현판 / 턴 순서 ─────────────────────────────
    holder, size, _ = kit.chrome(bp, root, "round")
    kit.label(bp, "RoundText", holder, 30 * K, 34 * K, 190 * K, 46 * K,
              "ROUND 1", 26, kit.TEXT_ON_LIGHT, "center", size, bold=True)

    holder, size, _ = kit.chrome(bp, root, "objective")
    kit.image(bp, "ObjectiveIcon", holder, 20 * K, 34 * K, 46 * K, 52 * K,
              size, texture=kit.KK + "/KK_Icon_Objective", tint=kit.WHITE)
    kit.label(bp, "ObjectiveText", holder, 78 * K, 40 * K, 262 * K, 44 * K,
              "목표", 19, kit.TEXT_ON_LIGHT, "left", size)

    holder, size, _ = kit.chrome(bp, root, "turn")
    pw, ph = holes["turn_portrait_size"]
    for i, (hx, hy) in enumerate(holes["turn_portrait"]):
        slot = "TurnToken_{}".format(i)
        kit.add(bp, "CanvasPanel", slot, holder)
        kit.place(bp, slot, hx * K, hy * K, pw * K, ph * K, "tl", size,
                  kit.Z_CONTENT)
        cell = (pw * K, ph * K)
        face = (kit.TURN_PORTRAITS[i]
                if i < len(kit.TURN_PORTRAITS) else None)
        kit.image(bp, "TurnPortrait_{}".format(i), slot, 0, 0,
                  cell[0], cell[1], cell, texture=face, tint=kit.WHITE)
        kit.chrome_select(bp, "TurnCurrent_{}".format(i), slot, "turn",
                          cell[0], cell[1], cell)

    # ── 아군 세 줄 ────────────────────────────────────────────────────────
    holder, size, _ = kit.chrome(bp, root, "party")
    pw, ph = holes["party_portrait_size"]
    for i in range(3):
        hx, hy = holes["party_portrait"][i]
        row_y = holes["party_row_y"][i]
        card = "PartyCard_{}".format(i)
        kit.add(bp, "CanvasPanel", card, holder)
        row = (435 * K, holes["party_row_h"] * K)
        kit.place(bp, card, 4 * K, row_y * K, row[0], row[1], "tl", size,
                  kit.Z_CONTENT)
        body = "PartyContent_{}".format(i)
        kit.add(bp, "CanvasPanel", body, card)
        kit.place(bp, body, 0, 0, row[0], row[1], "tl", row, kit.Z_CONTENT)

        # 초상은 구멍 자리에. 구멍은 조각 좌표라 행 원점을 뺀다.
        kit.image(bp, "PartyPortrait_{}".format(i), body,
                  (hx - 4) * K, (hy - row_y) * K, pw * K, ph * K, row,
                  texture=kit.PARTY_PORTRAITS[i], tint=kit.WHITE)
        kit.label(bp, "PartyName_{}".format(i), body, 104 * K, 6 * K,
                  180 * K, 34 * K, "이름", 19, kit.TEXT_COLOR, "left", row,
                  bold=True)
        kit.tag(bp, "PartyStatusIcon_{}".format(i), body, 317 * K, 9 * K,
                22 * K, "Poison", row)
        kit.label(bp, "PartyStatus_{}".format(i), body, 343 * K, 9 * K,
                  90 * K, 26 * K, "", 14, kit.GOLD, "left", row)
        kit.bar(bp, "PartyHPBar_{}".format(i), body, 124 * K, 45 * K,
                92 * K, 14 * K, kit.HP_GREEN, row)
        kit.label(bp, "PartyHPText_{}".format(i), body, 224 * K, 38 * K,
                  92 * K, 28 * K, "0/0", 15, kit.HP_TEXT, "left", row,
                  bold=True)
        for pip in range(4):
            kit.image(bp, "PartyAPPipBg_{}_{}".format(i, pip), body,
                      (124 + pip * 34) * K, 68 * K, 26 * K, 26 * K, row,
                      texture=kit.KK + "/KK_Gem_Blue_Off", tint=kit.WHITE)
        for pip in range(4):
            kit.image(bp, "PartyAPPip_{}_{}".format(i, pip), body,
                      (124 + pip * 34) * K, 68 * K, 26 * K, 26 * K, row,
                      texture=kit.KK + "/KK_Gem_Blue_On", tint=kit.WHITE)
        kit.label(bp, "PartyAPText_{}".format(i), body, 268 * K, 66 * K,
                  70 * K, 28 * K, "0/0", 14, kit.AP_TEXT, "left", row)
        kit.chrome_select(bp, "PartySelected_{}".format(i), card, "party",
                          row[0], row[1], row)

    # ── 스킬 카드 여섯 칸 ─────────────────────────────────────────────────
    holder, size, _ = kit.chrome(bp, root, "skills")
    cw, ch = holes["skill_card_size"]
    cy = holes["skill_card_y"]
    bw, bh = holes["skill_badge_size"]
    for i, cx in enumerate(holes["skill_card_x"]):
        card = "CommandCard_{}".format(i)
        kit.add(bp, "CanvasPanel", card, holder)
        cell = (cw * K, ch * K)
        kit.place(bp, card, cx * K, cy * K, cell[0], cell[1], "tl", size,
                  kit.Z_CONTENT)
        body = "CommandBody_{}".format(i)
        kit.add(bp, "CanvasPanel", body, card)
        kit.place(bp, body, 0, 0, cell[0], cell[1], "tl", cell, kit.Z_CONTENT)

        kit.image(bp, "CommandIcon_{}".format(i), body, 22 * K, 6 * K,
                  85 * K, 92 * K, cell, texture=kit.COMMAND_ICONS[i],
                  tint=kit.WHITE)
        # 배지 구멍은 조각 좌표라 카드 원점을 뺀다.
        kit.label(bp, "CommandCost_{}".format(i), body,
                  (cw - bw - 6) * K, (holes["skill_badge_y"] - cy) * K,
                  bw * K, bh * K, "0", 16, kit.BADGE_TEXT, "center", cell,
                  bold=True)
        kit.label(bp, "CommandName_{}".format(i), body, 6 * K, 128 * K,
                  (cw - 12) * K, 30 * K, "이름", 16, kit.TEXT_COLOR,
                  "center", cell, bold=True)
        kit.tag(bp, "CommandCooldownIcon_{}".format(i), body,
                (cw / 2.0 - 44) * K, 164 * K, 18 * K, "Cooldown", cell)
        kit.label(bp, "CommandCooldown_{}".format(i), body,
                  (cw / 2.0 - 20) * K, 162 * K, 80 * K, 24 * K, "", 13,
                  kit.COOLDOWN_TEXT, "left", cell)
        kit.label(bp, "CommandDamage_{}".format(i), body, 6 * K, 192 * K,
                  (cw - 12) * K, 24 * K, "0~0", 13, kit.DAMAGE_TEXT,
                  "center", cell)
        kit.label(bp, "CommandCostLine_{}".format(i), body, 6 * K, 232 * K,
                  (cw - 12) * K, 28 * K, "", 15, kit.AP_TEXT, "center", cell,
                  bold=True)

        kit.ghost_button(bp, "CommandButton_{}".format(i), card,
                         0, 0, cell[0], cell[1], cell)
        kit.image(bp, "CommandDisabled_{}".format(i), card, 0, 0,
                  cell[0], cell[1], cell, z_order=kit.Z_OVERLAY,
                  tint=kit.DISABLED_VEIL)
        kit.chrome_select(bp, "CommandSelected_{}".format(i), card, "skill",
                          cell[0], cell[1], cell)

    # ── 적 패널 ───────────────────────────────────────────────────────────
    holder, size, _ = kit.chrome(bp, root, "enemy", "EnemyPanel")
    ex, ey = holes["enemy_portrait"]
    ew, eh = holes["enemy_portrait_size"]
    kit.image(bp, "EnemyPortrait", holder, ex * K, ey * K, ew * K, eh * K,
              size, texture=kit.KK + "/KK_Face_Eagle", tint=kit.WHITE)
    kit.label(bp, "EnemyName", holder, 126 * K, 22 * K, 150 * K, 34 * K,
              "적", 19, kit.TEXT_COLOR, "left", size, bold=True)
    kit.bar(bp, "EnemyHPBar", holder, 126 * K, 62 * K, 108 * K, 16 * K,
            kit.HP_RED, size)
    kit.label(bp, "EnemyHPText", holder, 240 * K, 55 * K, 66 * K, 28 * K,
              "0/0", 15, kit.TEXT_COLOR, "left", size, bold=True)
    kit.tag(bp, "EnemyDefenseIcon", holder, 126 * K, 104 * K, 20 * K,
            "Defense", size)
    kit.label(bp, "EnemyDefense", holder, 150 * K, 102 * K, 140 * K, 26 * K,
              "", 14, kit.TEXT_DIM, "left", size)
    kit.tag(bp, "EnemyForecastIcon", holder, 126 * K, 146 * K, 20 * K,
            "Damage", size)
    kit.label(bp, "EnemyForecast", holder, 150 * K, 144 * K, 150 * K, 26 * K,
              "", 14, kit.DAMAGE_TEXT, "left", size)
    kit.label(bp, "EnemyStatus", holder, 20 * K, 172 * K, 268 * K, 24 * K,
              "", 13, kit.GOLD, "center", size)

    # ── 턴 종료 ───────────────────────────────────────────────────────────
    holder, size, _ = kit.chrome(bp, root, "endturn")
    kit.ghost_button(bp, "EndTurnButton", holder, 0, 0, size[0], size[1], size)
    kit.label(bp, "EndTurnLabel", holder, 20 * K, 28 * K, 268 * K, 44 * K,
              "턴 종료", 26, kit.TEXT_COLOR, "center", size, bold=True)


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
    round_w, strip_w, token = 140.0, 268.0, 54.0
    objective_w, gap = 244.0, 10.0

    x = 18.0
    kit.round_panel(bp, body, x, middle(72), round_w, 72, "tl", 22)
    x += round_w + gap
    for i in range(3):
        kit.party_card(bp, body, i, x + i * (strip_w + 8), middle(150),
                       strip_w, 150, "tl", "strip")
    x += 3 * strip_w + 2 * 8 + gap
    kit.turn_row(bp, body, x, middle(token), token, 5.0, "tl", names=False)
    # 여섯 칸 자리를 잡아 두었는데 실제로 보이는 건 다섯 칸이다. 남는 한 칸이
    # 적 패널 자리를 먹어 191px 까지 내려가 안전장치가 걸렸다.
    x += 5 * token + 4 * 5 + gap

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

#: 2안부터는 시안에서 오려 낸 조각을 그 자리에 놓아 짓는다.
#:
#: 손으로 적던 배치안 열 개를 지웠다. 좌표를 눈으로 옮기는 동안 4안 타원을
#: 가로로 만들고 7안 카드를 부채처럼 펼쳤는데, 둘 다 시안을 보지 않고 이름에서
#: 짐작한 결과였다. 조각을 통째로 놓으면 짐작할 것이 없다 -- 자리는 매칭이
#: 찾고 역할은 표에 있고 내용 자리는 조각에 뚫린 구멍이 알려 준다.
#:
#: 1안만 손으로 남긴다. 인게임에서 확인을 마친 유일한 안이고, 글자 자리를
#: 시안에서 재어 넣어 두었다 -- 생성기의 비율 계산보다 정확하다.
def _from_cutouts(number):
    def compose(blueprint, root):
        cutout_layout.build(blueprint, root, number)
    compose.required = lambda: cutout_layout.expected(number)
    return compose


LAYOUTS = (
    # 1안도 명세에서 짓는다. 손으로 적어 둔 layout_01 을 오래 남겨 두었는데,
    # 이름과 자리가 적힌 명세가 오면서 손으로 적을 이유가 없어졌다.
    ("WBP_CombatLayout_01_ClassicCRPG", "1안 클래식 CRPG", _from_cutouts("01")),
    ("WBP_CombatLayout_02_LeftParty", "2안 좌측 세로 파티", _from_cutouts("02")),
    ("WBP_CombatLayout_03_ActiveUnit", "3안 활성 유닛 집중", _from_cutouts("03")),
    ("WBP_CombatLayout_04_Radial", "4안 방사형", _from_cutouts("04")),
    ("WBP_CombatLayout_05_BottomBar", "5안 하단 통합 바", _from_cutouts("05")),
    ("WBP_CombatLayout_06_Mirrored", "6안 좌우 대칭", _from_cutouts("06")),
    ("WBP_CombatLayout_07_CardHand", "7안 카드 핸드", _from_cutouts("07")),
    ("WBP_CombatLayout_08_Minimal", "8안 미니멀", _from_cutouts("08")),
    ("WBP_CombatLayout_09_SplitBands", "9안 정보·조작 분리", _from_cutouts("09")),
    ("WBP_CombatLayout_10_Targeting", "10안 상황 전환형", _from_cutouts("10")),
    ("WBP_CombatLayout_11_RightGrid", "11안 우측 스킬 격자", _from_cutouts("11")),
    ("WBP_CombatLayout_12_TurnQueue", "12안 좌측 턴 큐", _from_cutouts("12")),
    ("WBP_CombatLayout_13_RightList", "13안 우측 세로 목록", _from_cutouts("13")),
    ("WBP_CombatLayout_14_FloatingBar", "14안 유닛 위 스킬 바",
     _from_cutouts("14")),
    ("WBP_CombatLayout_15_UnifiedDock", "15안 하단 통합 독",
     _from_cutouts("15")),
    ("WBP_CombatLayout_16_FullFrame", "16안 전체 액자", _from_cutouts("16")),
    ("WBP_CombatLayout_17_RightDock", "17안 우측 통합 독", _from_cutouts("17")),
    ("WBP_CombatLayout_18_RightFan", "18안 우측 부채", _from_cutouts("18")),
    ("WBP_CombatLayout_19_TopRail", "19안 상단 스킬 레일", _from_cutouts("19")),
    ("WBP_CombatLayout_20_CommandMode", "20안 지휘 모드",
     _from_cutouts("20")),
)


RECTS = {}


def build_one(asset_name, title, compose, required=None):
    kit.reset_ledger()
    blueprint = kit.create_asset(asset_name)
    kit.add(blueprint, "CanvasPanel", "RootCanvas", "")
    compose(blueprint, "RootCanvas")
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)
    # 다 짓고 저장한 뒤에야 제자리로 옮긴다. 여기 전에 터지면 기존 안이 남는다.
    kit.commit_asset(asset_name)
    RECTS[asset_name] = kit.absolute_rects()
    shown, total = kit.verify(asset_name, required)
    unreal.log("[Layouts] {:38s} {}  bound {}/{}".format(
        asset_name, title, shown, total))


only = None
failures = []
for asset_name, title, compose in LAYOUTS:
    if only and asset_name not in only:
        continue
    try:
        need = compose.required() if hasattr(compose, "required") else None
        build_one(asset_name, title, compose, need)
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
