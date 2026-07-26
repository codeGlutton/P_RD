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


def top_band(bp, root, turn_token=88.0, names=False):
    """Round counter left, turn order centred, objective right.

    Nine of the ten mock-ups share this band unchanged, so it lives in one
    place -- otherwise a tweak to the round panel has to be made nine times and
    the layouts stop being comparable the moment one of them is missed.
    """
    kit.round_panel(bp, root, 24, 20, 240, 68, "tl", 22)
    kit.turn_row(bp, root, centred(6, turn_token, 8), 18, turn_token, 8, "tc",
                 names=names)
    kit.objective_panel(bp, root, W - 384, 20, 360, 68, "tr", 17)


# ─── 1안 클래식 CRPG ───────────────────────────────────────────────────────────

def layout_01(bp, root):
    """Three layers: top band, battlefield, bottom band. The safe, dense one.

    The allies stack as three wide rows down the bottom-left rather than
    sitting side by side -- stacked they eat less width and the command rail
    gets the room, which is what the mock-up is really trading for.
    """
    top_band(bp, root, 104.0)

    bottom = H - 24
    # 시안 실측: 행 534x140, 행 사이는 거의 붙어 있다(2px). 500x116 에 12px
    # 간격이면 같은 정보가 더 좁은 자리에 눌려 카드가 비어 보인다.
    row_w, row_h, row_gap = 534.0, 140.0, 4.0
    party_top = bottom - 3 * row_h - 2 * row_gap
    for i in range(3):
        kit.party_card(bp, root, i, M, party_top + i * (row_h + row_gap),
                       row_w, row_h, "bl", "strip")

    enemy_w, enemy_h = 330.0, 200.0
    enemy_x = W - M - enemy_w
    end_h = 80.0

    cmd_w, cmd_h, cmd_gap = 156.0, 330.0, 10.0
    start = centred(6, cmd_w, cmd_gap, M + row_w + 24, enemy_x - 24)
    for i in range(6):
        kit.command_card(bp, root, i, start + i * (cmd_w + cmd_gap),
                         bottom - cmd_h, cmd_w, cmd_h, "bc")

    kit.enemy_panel(bp, root, enemy_x, bottom - end_h - 12 - enemy_h,
                    enemy_w, enemy_h, "br")
    kit.end_turn(bp, root, enemy_x, bottom - end_h, enemy_w, end_h, "br", 24)


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

    bottom = H - 24
    hero_w, hero_h = 440.0, 300.0
    kit.party_card(bp, root, 0, M, bottom - hero_h, hero_w, hero_h, "bl",
                   "hero")

    strip_w, strip_h, gap = 360.0, 96.0, 12.0
    strips_bottom = bottom - hero_h - 24
    for i in (1, 2):
        kit.party_card(bp, root, i, M,
                       strips_bottom - (3 - i) * (strip_h + gap) + gap,
                       strip_w, strip_h, "bl", "strip")

    enemy_w, enemy_h = 340.0, 240.0
    enemy_x = W - M - enemy_w
    end_h = 88.0
    cmd_w, cmd_h, cmd_gap = 152.0, 300.0, 10.0
    start = centred(6, cmd_w, cmd_gap, M + hero_w + 24, enemy_x - 24)
    for i in range(6):
        kit.command_card(bp, root, i, start + i * (cmd_w + cmd_gap),
                         bottom - cmd_h, cmd_w, cmd_h, "bc")

    kit.enemy_panel(bp, root, enemy_x, bottom - end_h - 12 - enemy_h,
                    enemy_w, enemy_h, "br")
    kit.end_turn(bp, root, enemy_x, bottom - end_h, enemy_w, end_h, "br", 26)


# ─── 4안 방사형 컨텍스트 메뉴 ──────────────────────────────────────────────────

def layout_04(bp, root):
    """Command cards orbit the unit on the battlefield.

    The mock-up keeps them as cards rather than bare icons, so the name and
    the damage line stay readable while the ring still frees the screen edges.
    The ring is an ellipse, not a circle: the screen is wider than it is tall,
    and a circle would push the top and bottom cards into the bands.
    """
    top_band(bp, root)

    cx, cy = W * 0.5, H * 0.48
    cmd_w, cmd_h = 168.0, 152.0
    radius_x, radius_y = 290.0, 215.0
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

    kit.enemy_panel(bp, root, W - M - 340, 120, 340, 210, "tr")
    kit.end_turn(bp, root, W - M - 260, bottom - 88, 260, 88, "br", 26)


# ─── 5안 하단 통합 바 ──────────────────────────────────────────────────────────

def layout_05(bp, root):
    """Everything you press lives in one thick bar across the bottom."""
    top_band(bp, root)

    bar_w, bar_h = W - 2 * M, 268.0
    body, size = kit.card(bp, "CommandBar", root, M, H - 24 - bar_h,
                          bar_w, bar_h, "bc", role="party")

    strip_w, strip_h, gap = 352.0, 72.0, 8.0
    for i in range(3):
        kit.party_card(bp, body, i, 24, 24 + i * (strip_h + gap),
                       strip_w, strip_h, "tl", "strip")

    end_w = 200.0
    cmd_w, cmd_h, cmd_gap = 168.0, 214.0, 10.0
    start = centred(6, cmd_w, cmd_gap, 24 + strip_w + 24, bar_w - end_w - 44)
    for i in range(6):
        kit.command_card(bp, body, i, start + i * (cmd_w + cmd_gap),
                         (bar_h - cmd_h) / 2.0, cmd_w, cmd_h, "tl", "card")

    kit.end_turn(bp, body, bar_w - end_w - 24, (bar_h - 120) / 2.0,
                 end_w, 120, "tl", 26)
    # 바 하나가 통째로 한 덩어리라 프레임도 바깥에 한 번만 두른다.
    kit.frame(bp, "CommandBar", body, bar_w, bar_h)

    kit.enemy_panel(bp, root, W - M - 340, 300, 340, 210, "mr")


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

    bottom = H - 24
    cmd_w, cmd_h = 176.0, 288.0
    spread, lift = 132.0, 20.0
    # 부채는 아래쪽 여백만 화면 밖으로 나가야 한다. 첫 시도는 바깥쪽 카드의
    # 이름과 피해량까지 데리고 나갔다.
    base_y = bottom - cmd_h - 8
    for i in range(6):
        offset = i - 2.5
        kit.command_card(bp, root, i,
                         W * 0.5 - cmd_w / 2.0 + offset * spread,
                         base_y + abs(offset) * lift,
                         cmd_w, cmd_h, "bc", "card", angle=offset * 4.0)

    row_w, row_h, gap = 440.0, 112.0, 12.0
    for i in range(3):
        kit.party_card(bp, root, i, M,
                       bottom - (3 - i) * (row_h + gap) + gap,
                       row_w, row_h, "bl", "strip")

    enemy_w, enemy_h = 340.0, 210.0
    kit.enemy_panel(bp, root, W - M - enemy_w, bottom - 96 - 12 - enemy_h,
                    enemy_w, enemy_h, "br")
    kit.end_turn(bp, root, W - M - enemy_w, bottom - 96, enemy_w, 96, "br", 26)


# ─── 8안 미니멀 ────────────────────────────────────────────────────────────────

def layout_08(bp, root):
    """Only the unit on turn is read out; the rest lives on the battlefield.

    Slots 1 and 2 and the enemy panel are dropped on purpose. The runtime
    tolerates the gap, so the omission is the design point rather than a hole.
    """
    kit.round_panel(bp, root, 24, 20, 240, 60, "tl", 20)
    kit.objective_panel(bp, root, 24, 88, 340, 52, "tl", 16, icon=False)
    kit.turn_row(bp, root, centred(6, 76, 8), 18, 76, 8, "tc", names=False)

    bottom = H - 24
    hero_w, hero_h = 400.0, 240.0
    kit.party_card(bp, root, 0, M, bottom - hero_h, hero_w, hero_h, "bl",
                   "hero")

    end_w = 240.0
    cmd_w, cmd_h, cmd_gap = 168.0, 208.0, 10.0
    start = centred(6, cmd_w, cmd_gap, M + hero_w + 24, W - M - end_w - 20)
    for i in range(6):
        kit.command_card(bp, root, i, start + i * (cmd_w + cmd_gap),
                         bottom - cmd_h, cmd_w, cmd_h, "bc")
    kit.end_turn(bp, root, W - M - end_w, bottom - 104, end_w, 104, "br", 26)


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
    round_w, strip_w, token = 170.0, 250.0, 60.0
    objective_w, gap = 220.0, 12.0

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

    bottom = H - 24
    end_w = 220.0
    row_w = 440.0
    cmd_w, cmd_h, cmd_gap = 176.0, 196.0, 12.0
    start = centred(6, cmd_w, cmd_gap, M + row_w + 24, W - M - end_w - 20)
    for i in range(6):
        kit.command_card(bp, root, i, start + i * (cmd_w + cmd_gap),
                         bottom - cmd_h, cmd_w, cmd_h, "bc")

    # 조준 정보는 레일 바로 위에 레일과 같은 폭으로. 눈이 카드에서 대상까지
    # 화면을 가로지르지 않는다.
    rail_w = 6 * cmd_w + 5 * cmd_gap
    kit.enemy_panel(bp, root, start, bottom - cmd_h - 14 - 130, rail_w, 130,
                    "bc")

    row_h, gap = 106.0, 12.0
    for i in range(3):
        kit.party_card(bp, root, i, M,
                       bottom - (3 - i) * (row_h + gap) + gap,
                       row_w, row_h, "bl", "strip")

    kit.end_turn(bp, root, W - M - end_w, bottom - 104, end_w, 104, "br", 26)


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


def build_one(asset_name, title, compose):
    blueprint = kit.create_asset(asset_name)
    kit.add(blueprint, "CanvasPanel", "RootCanvas", "")
    compose(blueprint, "RootCanvas")
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)
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

unreal.log("[Layouts] built {}/{}".format(
    len(LAYOUTS) - len(failures), len(LAYOUTS)))
if failures:
    raise RuntimeError("layouts failed:\n  " + "\n  ".join(failures))
unreal.log("[Layouts] done")
