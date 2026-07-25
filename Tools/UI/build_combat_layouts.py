"""Build all ten combat HUD layout variants as real Widget Blueprints.

One layout = one WBP. They share the components in combat_layout_kit so that
only the arrangement differs and the comparison is about placement, not about
one of them happening to have nicer cards.

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


# ─── 1안 클래식 CRPG ───────────────────────────────────────────────────────────

def layout_01(bp, root):
    """Three layers: top band, battlefield, bottom band. The safe, dense one."""
    kit.round_panel(bp, root, 24, 20, 200, 60, "tl", 20)
    kit.turn_row(bp, root, centred(6, 96, 12), 16, 96, 12, "tc")
    kit.objective_panel(bp, root, W - 364, 20, 340, 60, "tr")

    party_w, party_h, gap = 210.0, 168.0, 8.0
    for i in range(3):
        kit.party_card(bp, root, i, M + i * (party_w + gap),
                       H - 24 - party_h, party_w, party_h, "bl", "card")

    enemy_w = 300.0
    enemy_x = W - M - enemy_w
    cmd_w, cmd_h, cmd_gap = 140.0, 188.0, 8.0
    start = centred(6, cmd_w, cmd_gap,
                    M + 3 * party_w + 2 * gap + 24, enemy_x - 24)
    for i in range(6):
        kit.command_card(bp, root, i, start + i * (cmd_w + cmd_gap),
                         H - 24 - cmd_h, cmd_w, cmd_h, "bc")

    kit.enemy_panel(bp, root, enemy_x, H - 24 - 64 - 12 - 120, enemy_w, 120,
                    "br")
    kit.end_turn(bp, root, enemy_x, H - 24 - 64, enemy_w, 64, "br")


# ─── 2안 좌측 세로 파티 ────────────────────────────────────────────────────────

def layout_02(bp, root):
    """Allies stacked down the left edge; a big skill rail owns the bottom."""
    kit.round_panel(bp, root, 24, 18, 180, 52, "tl", 18)
    kit.turn_row(bp, root, centred(6, 78, 10), 14, 78, 10, "tc", names=False)
    kit.objective_panel(bp, root, W - 324, 18, 300, 52, "tr", 15)

    strip_w, strip_h, gap = 320.0, 92.0, 10.0
    top = 300.0
    for i in range(3):
        # The unit whose turn it is reaches further right, so the selection
        # reads from shape as well as from the marker colour.
        width = strip_w + (40.0 if i == 0 else 0.0)
        kit.party_card(bp, root, i, M, top + i * (strip_h + gap),
                       width, strip_h, "ml", "strip")

    cmd_w, cmd_h, cmd_gap = 170.0, 210.0, 10.0
    start = centred(6, cmd_w, cmd_gap)
    for i in range(6):
        kit.command_card(bp, root, i, start + i * (cmd_w + cmd_gap),
                         H - 24 - cmd_h, cmd_w, cmd_h, "bc")

    kit.enemy_panel(bp, root, W - M - 220, 260, 220, 420, "mr", "tall")
    kit.end_turn(bp, root, W - M - 220, H - 24 - 64, 220, 64, "br", 17)


# ─── 3안 활성 유닛 집중 ────────────────────────────────────────────────────────

def layout_03(bp, root):
    """The unit on turn gets a hero panel; the other two shrink to chips."""
    kit.round_panel(bp, root, 24, 18, 180, 52, "tl", 18)
    kit.turn_row(bp, root, centred(6, 86, 10), 14, 86, 10, "tc")
    kit.objective_panel(bp, root, W - 324, 18, 300, 52, "tr", 15)

    hero_w, hero_h = 400.0, 260.0
    kit.party_card(bp, root, 0, M, H - 24 - hero_h, hero_w, hero_h, "bl",
                   "hero")
    chip_w, chip_h = 196.0, 64.0
    for i in (1, 2):
        kit.party_card(bp, root, i, M + (i - 1) * (chip_w + 8),
                       H - 24 - hero_h - 10 - chip_h, chip_w, chip_h, "bl",
                       "chip")

    enemy_w = 300.0
    enemy_x = W - M - enemy_w
    cmd_w, cmd_h, cmd_gap = 150.0, 190.0, 10.0
    start = centred(6, cmd_w, cmd_gap, M + hero_w + 30, enemy_x - 24)
    for i in range(6):
        kit.command_card(bp, root, i, start + i * (cmd_w + cmd_gap),
                         H - 24 - cmd_h, cmd_w, cmd_h, "bc")

    kit.enemy_panel(bp, root, enemy_x, H - 24 - 64 - 12 - 120, enemy_w, 120,
                    "br")
    kit.end_turn(bp, root, enemy_x, H - 24 - 64, enemy_w, 64, "br")


# ─── 4안 방사형 컨텍스트 메뉴 ──────────────────────────────────────────────────

def layout_04(bp, root):
    """Skills orbit the unit on the battlefield; the edges stay nearly empty."""
    kit.round_panel(bp, root, 24, 18, 180, 52, "tl", 18)
    kit.turn_row(bp, root, centred(6, 78, 10), 14, 78, 10, "tc", names=False)
    kit.objective_panel(bp, root, W - 300, 18, 276, 52, "tr", 14)

    # The ring sits a little above centre: the lower half of the screen is
    # where the thumbs are, and the chips already live down there.
    cx, cy, radius, glyph = W * 0.5, H * 0.46, 200.0, 112.0
    for i in range(6):
        angle = math.radians(-90 + i * 60)
        kit.command_card(bp, root, i,
                         cx + radius * math.cos(angle) - glyph / 2.0,
                         cy + radius * math.sin(angle) - glyph / 2.0,
                         glyph, glyph, "mc", "icon")

    # The centre of the ring carries the remaining AP of the unit on turn.
    kit.label(bp, "PartyAPText_0", root, cx - 60, cy - 26, 120, 48, "0/0", 34,
              kit.GOLD, "center", None, True)

    chip_w, chip_h = 200.0, 62.0
    for i in range(3):
        kit.party_card(bp, root, i, M + i * (chip_w + 8), H - 24 - chip_h,
                       chip_w, chip_h, "bl", "chip")

    kit.enemy_panel(bp, root, W - M - 300, 300, 300, 120, "tr")
    kit.end_turn(bp, root, W - M - 260, H - 24 - 68, 260, 68, "br")


# ─── 5안 하단 통합 바 ──────────────────────────────────────────────────────────

def layout_05(bp, root):
    """One thick bottom bar carries everything; the top two thirds stay clear."""
    kit.round_panel(bp, root, 24, 16, 168, 46, "tl", 16)
    kit.turn_row(bp, root, centred(6, 68, 8), 12, 68, 8, "tc", names=False)
    kit.objective_panel(bp, root, W - 300, 16, 276, 46, "tr", 14)

    bar_w, bar_h = W - 2 * M, 236.0
    bar_top = H - 24 - bar_h
    body, size = kit.card(bp, "CommandBar", root, M, bar_top, bar_w, bar_h, "bc")

    strip_w, strip_h, gap = 296.0, 62.0, 8.0
    for i in range(3):
        kit.party_card(bp, body, i, 34, 22 + i * (strip_h + gap),
                       strip_w, strip_h, "tl", "chip")

    cmd_w, cmd_h, cmd_gap = 148.0, 196.0, 8.0
    start = centred(6, cmd_w, cmd_gap, 34 + strip_w + 24, bar_w - 300)
    for i in range(6):
        kit.command_card(bp, body, i, start + i * (cmd_w + cmd_gap),
                         (bar_h - cmd_h) / 2.0, cmd_w, cmd_h, "tl", "card")

    kit.end_turn(bp, body, bar_w - 268, (bar_h - 72) / 2.0, 250, 72, "tl", 20)
    # The bar is one piece of furniture, so the frame goes round the whole
    # thing rather than round each block inside it.
    kit.frame(bp, "CommandBar", body, bar_w, bar_h)

    kit.enemy_panel(bp, root, W - M - 320, 420, 320, 130, "mr")


# ─── 6안 좌우 대칭 ─────────────────────────────────────────────────────────────

def layout_06(bp, root):
    """Allies down the left, the enemy down the right, facing each other."""
    kit.round_panel(bp, root, 24, 18, 180, 52, "tl", 18)
    kit.turn_row(bp, root, centred(6, 86, 10), 14, 86, 10, "tc")
    kit.objective_panel(bp, root, W - 324, 18, 300, 52, "tr", 15)

    col_w, gap = 300.0, 10.0
    heights = (196.0, 150.0, 150.0)      # the unit on turn takes more room
    top = 150.0
    y = top
    for i in range(3):
        kit.party_card(bp, root, i, M, y, col_w, heights[i], "ml", "card")
        y += heights[i] + gap

    enemy_w = 300.0
    enemy_x = W - M - enemy_w
    kit.enemy_panel(bp, root, enemy_x, top, enemy_w, 380, "mr", "tall")

    cmd_w, cmd_h, cmd_gap = 170.0, 200.0, 10.0
    start = centred(6, cmd_w, cmd_gap, M + col_w + 24, enemy_x - 24)
    for i in range(6):
        kit.command_card(bp, root, i, start + i * (cmd_w + cmd_gap),
                         H - 24 - cmd_h, cmd_w, cmd_h, "bc")

    kit.end_turn(bp, root, enemy_x, H - 24 - 64, enemy_w, 64, "br")


# ─── 7안 카드 핸드 ─────────────────────────────────────────────────────────────

def layout_07(bp, root):
    """Skills as a hand of cards, fanned and overlapping."""
    kit.round_panel(bp, root, 24, 18, 180, 52, "tl", 18)
    kit.turn_row(bp, root, centred(6, 78, 10), 14, 78, 10, "tc", names=False)
    kit.objective_panel(bp, root, W - 324, 18, 300, 52, "tr", 15)

    cmd_w, cmd_h = 156.0, 232.0
    spread, lift = 108.0, 18.0
    # The hand is meant to run off the bottom edge, but only its blank lower
    # margin -- at the first pass the outer cards took their names and damage
    # numbers off screen with them.
    base_y = H - 24 - cmd_h - 6
    for i in range(6):
        offset = i - 2.5
        # Cards rise towards the middle of the fan and tilt away from it, so
        # the hand reads as held rather than as a row that happens to overlap.
        kit.command_card(bp, root, i,
                         W * 0.5 - cmd_w / 2.0 + offset * spread,
                         base_y + abs(offset) * lift,
                         cmd_w, cmd_h, "bc", "card", angle=offset * 5.0)

    card_w, card_h, gap = 190.0, 150.0, 8.0
    for i in range(3):
        kit.party_card(bp, root, i, M, H - 24 - (3 - i) * (card_h + gap) + gap,
                       card_w, card_h, "bl", "card")

    kit.enemy_panel(bp, root, W - M - 300, H - 24 - 64 - 12 - 120, 300, 120,
                    "br")
    kit.end_turn(bp, root, W - M - 300, H - 24 - 64, 300, 64, "br")


# ─── 8안 미니멀 ────────────────────────────────────────────────────────────────

def layout_08(bp, root):
    """Almost nothing on screen. Slots 1 and 2 and the enemy panel are dropped."""
    kit.round_panel(bp, root, 20, 14, 130, 36, "tl", 13)
    kit.objective_panel(bp, root, 20, 56, 260, 34, "tl", 12, icon=False)
    kit.turn_row(bp, root, centred(6, 54, 8), 12, 54, 8, "tc", names=False,
                 framed=False)

    # Only the unit on turn is read out, and only its HP and AP -- the gems
    # come back here because they are the whole party read-out in this layout.
    kit.party_card(bp, root, 0, M, H - 24 - 74, 320, 74, "bl", "chip",
                   chip_gems=True)

    glyph, gap = 96.0, 14.0
    start = centred(6, glyph, gap)
    for i in range(6):
        kit.command_card(bp, root, i, start + i * (glyph + gap),
                         H - 24 - glyph, glyph, glyph, "bc", "icon")

    kit.end_turn(bp, root, W - M - 220, H - 24 - 68, 220, 68, "br", 17)


# ─── 9안 상단 정보 / 하단 조작 ─────────────────────────────────────────────────

def layout_09(bp, root):
    """Everything you read is up top, everything you press is down below."""
    bar_w, bar_h = W - 2 * M, 132.0
    body, size = kit.card(bp, "InfoBar", root, M, 16, bar_w, bar_h, "tc")

    # The blocks are laid out left to right by running width rather than by
    # fixed offsets. Stacked by hand, the enemy panel ended up 62px wide and
    # its numbers spilled out of the bar.
    def middle(height):
        return (bar_h - height) / 2.0

    strip_w, token, gap = 244.0, 62.0, 12.0
    round_w, objective_w = 128.0, 232.0
    turn_w = 6 * token + 5 * 6.0

    x = 14.0
    kit.round_panel(bp, body, x, middle(48), round_w, 48, "tl", 15)
    x += round_w + gap
    for i in range(3):
        kit.party_card(bp, body, i, x + i * (strip_w + 6), middle(96),
                       strip_w, 96, "tl", "strip")
    x += 3 * strip_w + 2 * 6 + gap
    kit.turn_row(bp, body, x, middle(token), token, 6.0, "tl", names=False)
    x += turn_w + gap

    objective_x = bar_w - 14 - objective_w
    kit.enemy_panel(bp, body, x, middle(104), objective_x - gap - x, 104, "tl")
    kit.objective_panel(bp, body, objective_x, middle(48), objective_w, 48,
                        "tl", 13)
    kit.frame(bp, "InfoBar", body, bar_w, bar_h)

    cmd_w, cmd_h, cmd_gap = 176.0, 226.0, 10.0
    end_w = 240.0
    start = centred(6, cmd_w, cmd_gap, M, W - M - end_w - 24)
    for i in range(6):
        kit.command_card(bp, root, i, start + i * (cmd_w + cmd_gap),
                         H - 24 - cmd_h, cmd_w, cmd_h, "bc")
    kit.end_turn(bp, root, W - M - end_w, H - 24 - cmd_h / 2.0 - 40, end_w, 80,
                 "br", 22)


# ─── 10안 상황 전환형 ──────────────────────────────────────────────────────────

def layout_10(bp, root):
    """The moment a skill is aimed: the bottom grows a targeting read-out.

    The read-out is the enemy panel itself rather than a second copy of the
    same numbers -- widget names are unique, so a duplicate would be a static
    label that never updates, which is worse than not showing it.
    """
    kit.round_panel(bp, root, 24, 18, 180, 52, "tl", 18)
    kit.turn_row(bp, root, centred(6, 78, 10), 14, 78, 10, "tc", names=False)

    cmd_w, cmd_h, cmd_gap = 156.0, 196.0, 10.0
    start = centred(6, cmd_w, cmd_gap)
    for i in range(6):
        kit.command_card(bp, root, i, start + i * (cmd_w + cmd_gap),
                         H - 24 - cmd_h, cmd_w, cmd_h, "bc")

    # The aiming panel sits directly above the rail, the width of the rail, so
    # the eye goes card -> target without crossing the screen.
    rail_w = 6 * cmd_w + 5 * cmd_gap
    kit.enemy_panel(bp, root, start, H - 24 - cmd_h - 14 - 116, rail_w, 116,
                    "bc")

    chip_w, chip_h = 196.0, 60.0
    for i in range(3):
        kit.party_card(bp, root, i, M, H - 24 - (3 - i) * (chip_h + 8) + 8,
                       chip_w, chip_h, "bl", "chip")

    kit.end_turn(bp, root, W - M - 240, H - 24 - 68, 240, 68, "br", 17)


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
