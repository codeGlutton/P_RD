"""Author layout 1 (classic CRPG) of the combat HUD as a real Widget Blueprint.

The widget class finds everything by name, so this script's job is to place
widgets under the agreed names and give them a readable look. Nothing here
knows about combat rules -- the names are the whole contract.

Everything sits on canvas panels with absolute 1920x1080 coordinates, anchored
to the screen edge the block belongs to, so the layout holds at 16:9 (the
logical canvas the project's ShortestSide scaling produces at that ratio).

Run through Tools/RunEditorPython.ps1 -- a bare -ExecutePythonScript exits 0
even when this raises.
"""
import unreal

PACKAGE_PATH = "/Game/UI/CombatLayouts"
ASSET_NAME = "WBP_CombatLayout_01_ClassicCRPG"
FULL_PATH = "{}/{}".format(PACKAGE_PATH, ASSET_NAME)

CANVAS_W, CANVAS_H = 1920.0, 1080.0

# Palette. Dark slate panels, warm gold trim, teal for "this is yours / active".
INK = unreal.LinearColor(0.04, 0.05, 0.07, 0.92)
PANEL = unreal.LinearColor(0.07, 0.08, 0.11, 0.90)
PANEL_LIGHT = unreal.LinearColor(0.11, 0.13, 0.17, 0.94)
GOLD = unreal.LinearColor(0.85, 0.70, 0.36, 1.0)
TEAL = unreal.LinearColor(0.30, 0.85, 0.80, 1.0)
TEXT = unreal.LinearColor(0.90, 0.90, 0.88, 1.0)
TEXT_DIM = unreal.LinearColor(0.62, 0.63, 0.66, 1.0)
HP_GREEN = unreal.LinearColor(0.35, 0.72, 0.34, 1.0)
HP_RED = unreal.LinearColor(0.76, 0.26, 0.24, 1.0)
AP_ON = unreal.LinearColor(0.36, 0.78, 0.92, 1.0)
DISABLED = unreal.LinearColor(0.0, 0.0, 0.0, 0.62)

AP_OFF = unreal.LinearColor(0.18, 0.22, 0.26, 1.0)

# Bottom band geometry. The three blocks share one row, so their widths are
# declared together and checked against the canvas before anything is built.
PARTY_X, PARTY_W, PARTY_H, PARTY_GAP = 20.0, 210.0, 140.0, 8.0
ENEMY_W = 300.0
ENEMY_X = CANVAS_W - 20.0 - ENEMY_W

helper = unreal.MCPythonHelper


# ─── asset ────────────────────────────────────────────────────────────────────

def create_asset():
    """Make (or remake) the widget blueprint parented to UCombatLayoutHUDWidget."""
    if unreal.EditorAssetLibrary.does_asset_exist(FULL_PATH):
        unreal.EditorAssetLibrary.delete_asset(FULL_PATH)

    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", unreal.CombatLayoutHUDWidget)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        ASSET_NAME, PACKAGE_PATH, unreal.WidgetBlueprint, factory)
    if asset is None:
        raise RuntimeError("could not create {}".format(FULL_PATH))
    # UWidgetBlueprint does not expose ParentClass to Python, so confirm the
    # parenting through the generated class instead of trusting the factory.
    generated = asset.generated_class()
    if generated is None or not unreal.MathLibrary.class_is_child_of(
            generated, unreal.CombatLayoutHUDWidget):
        raise RuntimeError("{} is not parented to CombatLayoutHUDWidget".format(
            FULL_PATH))
    unreal.log("[L01] created {} parent=CombatLayoutHUDWidget".format(FULL_PATH))
    return asset


# ─── tree building ────────────────────────────────────────────────────────────

def add(blueprint, widget_type, name, parent):
    result = helper.umg_add_widget(blueprint, widget_type, name, parent)
    if '"success":true' not in result.replace(" ", ""):
        raise RuntimeError("add {} {} under {} -> {}".format(
            widget_type, name, parent, result))
    return helper.umg_find_widget(blueprint, name)


ANCHORS = {
    "tl": (0.0, 0.0), "tc": (0.5, 0.0), "tr": (1.0, 0.0),
    "bl": (0.0, 1.0), "bc": (0.5, 1.0), "br": (1.0, 1.0),
}


def place(blueprint, name, x, y, w, h, anchor="tl", parent_size=None):
    """Position a canvas child given absolute coordinates in its parent.

    `anchor` decides which parent edge the widget stays glued to when the
    screen is not 16:9 -- left blocks hug the left, the command rail stays
    centred, the enemy panel hugs the right.
    """
    ax, ay = ANCHORS[anchor]
    pw, ph = parent_size if parent_size else (CANVAS_W, CANVAS_H)
    result = helper.umg_set_slot_layout(
        blueprint, name, ax, ay, ax, ay, x - ax * pw, y - ay * ph, w, h)
    if '"success":true' not in result.replace(" ", ""):
        raise RuntimeError("place {} -> {}".format(name, result))


def panel(blueprint, name, parent, x, y, w, h, color, anchor="tl",
          parent_size=None):
    """A filled rectangle that can also hold one child (used as a card body)."""
    border = add(blueprint, "Border", name, parent)
    border.set_editor_property("brush_color", color)
    border.set_editor_property("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
    place(blueprint, name, x, y, w, h, anchor, parent_size)
    return border


def card(blueprint, name, parent, x, y, w, h, color, anchor="tl",
         parent_size=None):
    """A panel plus an inner canvas, so children can be placed freely.

    The inner canvas matters: hiding the card has to hide its contents, and a
    widget only hides its own subtree. Laying the contents on the root canvas
    instead would leave orphaned text floating over an empty slot whenever a
    party member is missing.
    """
    panel(blueprint, name, parent, x, y, w, h, color, anchor, parent_size)
    inner = "{}_Canvas".format(name)
    add(blueprint, "CanvasPanel", inner, name)
    return inner, (w, h)


def label(blueprint, name, parent, x, y, w, h, text, size=14,
          color=TEXT, align="left", parent_size=None):
    block = add(blueprint, "TextBlock", name, parent)
    block.set_editor_property("text", unreal.Text(text))
    font = block.get_editor_property("font")
    font.size = size
    block.set_editor_property("font", font)
    block.set_editor_property("color_and_opacity", unreal.SlateColor(color))
    block.set_editor_property("justification", {
        "left": unreal.TextJustify.LEFT,
        "center": unreal.TextJustify.CENTER,
        "right": unreal.TextJustify.RIGHT,
    }[align])
    place(blueprint, name, x, y, w, h, "tl", parent_size)
    return block


def swatch(blueprint, name, parent, x, y, w, h, color, parent_size=None):
    """A plain tinted rectangle. Stands in for art until real brushes land."""
    image = add(blueprint, "Image", name, parent)
    image.set_editor_property("color_and_opacity", color)
    place(blueprint, name, x, y, w, h, "tl", parent_size)
    return image


def bar(blueprint, name, parent, x, y, w, h, fill, parent_size=None):
    progress = add(blueprint, "ProgressBar", name, parent)
    progress.set_editor_property("fill_color_and_opacity", fill)
    progress.set_editor_property("percent", 0.75)
    place(blueprint, name, x, y, w, h, "tl", parent_size)
    return progress


# ─── layout ───────────────────────────────────────────────────────────────────

def build_top_band(blueprint, root):
    """Round counter on the left, turn order across the middle, objective right."""
    round_panel = card(blueprint, "RoundPanel", root, 24, 20, 190, 56, PANEL, "tl")
    label(blueprint, "RoundText", round_panel[0], 0, 14, 190, 28,
          "ROUND 1", 20, GOLD, "center", round_panel[1])

    # Turn order: six tokens centred as a row, so the row stays put at 4:3.
    token_w, token_h, gap = 92.0, 92.0, 14.0
    row_w = 6 * token_w + 5 * gap
    start_x = (CANVAS_W - row_w) / 2.0
    for index in range(6):
        name = "TurnToken_{}".format(index)
        x = start_x + index * (token_w + gap)
        token, size = card(blueprint, name, root, x, 18, token_w, token_h,
                           PANEL_LIGHT, "tc")
        swatch(blueprint, "TurnPortrait_{}".format(index), token,
               8, 6, 76, 56, unreal.LinearColor(0.30, 0.32, 0.38, 1.0), size)
        label(blueprint, "TurnName_{}".format(index), token, 4, 66, 84, 20,
              "이름", 12, TEXT_DIM, "center", size)
        # Drawn last so the active outline sits above the portrait.
        swatch(blueprint, "TurnCurrent_{}".format(index), token,
               0, 86, token_w, 6, TEAL, size)

    objective = card(blueprint, "ObjectivePanel", root, CANVAS_W - 344, 20, 320, 56,
                     PANEL, "tr")
    label(blueprint, "ObjectiveText", objective[0], 12, 14, 296, 28,
          "적 전멸", 18, TEXT, "center", objective[1])


def build_party(blueprint, root):
    """Three ally cards along the bottom-left: portrait, HP, AP pips, status."""
    top = CANVAS_H - 24 - PARTY_H
    for index in range(3):
        name = "PartyCard_{}".format(index)
        x = PARTY_X + index * (PARTY_W + PARTY_GAP)
        body, size = card(blueprint, name, root, x, top, PARTY_W, PARTY_H,
                          PANEL, "bl")

        swatch(blueprint, "PartyPortrait_{}".format(index), body,
               10, 10, 76, 76, unreal.LinearColor(0.28, 0.30, 0.36, 1.0), size)
        label(blueprint, "PartyName_{}".format(index), body,
              94, 8, 106, 24, "이름", 16, TEXT, "left", size)
        bar(blueprint, "PartyHPBar_{}".format(index), body,
            94, 40, 106, 14, HP_GREEN, size)
        label(blueprint, "PartyHPText_{}".format(index), body,
              94, 56, 106, 18, "0/0", 12, TEXT_DIM, "left", size)

        # AP as four pips plus a number. Each pip gets a dim backing square that
        # is never hidden: the widget collapses spent pips, and a pip that just
        # disappears reads as "this unit has fewer slots" instead of "spent".
        for pip in range(4):
            swatch(blueprint, "PartyAPPipBg_{}_{}".format(index, pip), body,
                   94 + pip * 22, 80, 18, 18, AP_OFF, size)
        for pip in range(4):
            swatch(blueprint, "PartyAPPip_{}_{}".format(index, pip), body,
                   94 + pip * 22, 80, 18, 18, AP_ON, size)
        label(blueprint, "PartyAPText_{}".format(index), body,
              10, 88, 76, 16, "0/0", 11, TEXT_DIM, "center", size)

        label(blueprint, "PartyStatus_{}".format(index), body,
              10, 108, PARTY_W - 20, 20, "", 12, GOLD, "left", size)

        # Selection outline last so it reads above the card contents.
        swatch(blueprint, "PartySelected_{}".format(index), body,
               0, PARTY_H - 6, PARTY_W, 6, TEAL, size)


def build_command_rail(blueprint, root):
    """Six command cards centred at the bottom. Slot 0 is 이동, 1..5 are skills."""
    card_w, card_h, gap = 140.0, 168.0, 8.0
    row_w = 6 * card_w + 5 * gap
    # Centre the rail in the gap the other two blocks leave, not on the screen
    # centre -- the party block reaches past screen centre at this card size.
    free_left = PARTY_X + 3 * PARTY_W + 2 * PARTY_GAP + 24
    free_right = ENEMY_X - 24
    start_x = free_left + (free_right - free_left - row_w) / 2.0
    top = CANVAS_H - 24 - card_h
    for index in range(6):
        name = "CommandCard_{}".format(index)
        x = start_x + index * (card_w + gap)
        body, size = card(blueprint, name, root, x, top, card_w, card_h,
                          PANEL_LIGHT, "bc")

        swatch(blueprint, "CommandIcon_{}".format(index), body,
               (card_w - 84) / 2.0, 12, 84, 84,
               unreal.LinearColor(0.34, 0.36, 0.42, 1.0), size)
        label(blueprint, "CommandName_{}".format(index), body,
              4, 100, card_w - 8, 22, "이름", 15, TEXT, "center", size)
        label(blueprint, "CommandDamage_{}".format(index), body,
              4, 122, card_w - 8, 20, "0~0", 13, TEXT_DIM, "center", size)
        label(blueprint, "CommandCooldown_{}".format(index), body,
              4, 142, card_w - 8, 20, "", 12, GOLD, "center", size)
        # Cost badge in the top-right corner, matching the mock-up.
        label(blueprint, "CommandCost_{}".format(index), body,
              card_w - 32, 6, 26, 24, "0", 16, AP_ON, "center", size)

        # The button covers the card and is added before the overlays so the
        # overlays draw on top; text and images are hit-test invisible, so
        # nothing steals the click.
        add(blueprint, "Button", "CommandButton_{}".format(index), body)
        button = helper.umg_find_widget(
            blueprint, "CommandButton_{}".format(index))
        style = button.get_editor_property("widget_style")
        for state in ("normal", "hovered", "pressed", "disabled"):
            brush = style.get_editor_property(state)
            brush.set_editor_property("tint_color", unreal.SlateColor(
                unreal.LinearColor(1.0, 1.0, 1.0, 0.0)))
            style.set_editor_property(state, brush)
        button.set_editor_property("widget_style", style)
        place(blueprint, "CommandButton_{}".format(index),
              0, 0, card_w, card_h, "tl", size)

        swatch(blueprint, "CommandDisabled_{}".format(index), body,
               0, 0, card_w, card_h, DISABLED, size)
        swatch(blueprint, "CommandSelected_{}".format(index), body,
               0, card_h - 6, card_w, 6, TEAL, size)


def build_enemy_and_turn_end(blueprint, root):
    """Enemy read-out bottom-right, with the end-turn button under it."""
    panel_w, panel_h = ENEMY_W, 104.0
    x = ENEMY_X
    top = CANVAS_H - 24 - 64 - 12 - panel_h
    body, size = card(blueprint, "EnemyPanel", root, x, top, panel_w, panel_h,
                      PANEL, "br")

    swatch(blueprint, "EnemyPortrait", body, 10, 10, 64, 64,
           unreal.LinearColor(0.34, 0.24, 0.26, 1.0), size)
    label(blueprint, "EnemyName", body, 84, 10, 206, 24, "적", 17, TEXT,
          "left", size)
    bar(blueprint, "EnemyHPBar", body, 84, 40, 206, 14, HP_RED, size)
    label(blueprint, "EnemyHPText", body, 84, 56, 100, 20, "0/0", 13,
          TEXT_DIM, "left", size)
    label(blueprint, "EnemyDefense", body, 190, 56, 100, 20, "", 13,
          TEXT_DIM, "right", size)
    label(blueprint, "EnemyStatus", body, 10, 78, 140, 18, "", 12,
          GOLD, "left", size)
    label(blueprint, "EnemyForecast", body, 154, 78, panel_w - 164, 18, "", 12,
          TEXT_DIM, "right", size)

    button_top = CANVAS_H - 24 - 64
    button = add(blueprint, "Button", "EndTurnButton", root)
    style = button.get_editor_property("widget_style")
    for state, tint in (("normal", unreal.LinearColor(0.16, 0.18, 0.22, 0.95)),
                        ("hovered", unreal.LinearColor(0.22, 0.26, 0.31, 0.98)),
                        ("pressed", unreal.LinearColor(0.10, 0.12, 0.15, 1.0)),
                        ("disabled", unreal.LinearColor(0.10, 0.11, 0.13, 0.7))):
        brush = style.get_editor_property(state)
        brush.set_editor_property("tint_color", unreal.SlateColor(tint))
        style.set_editor_property(state, brush)
    button.set_editor_property("widget_style", style)
    place(blueprint, "EndTurnButton", x, button_top, panel_w, 64, "br")

    end_label = add(blueprint, "TextBlock", "EndTurnLabel", "EndTurnButton")
    end_label.set_editor_property("text", unreal.Text("턴 종료"))
    font = end_label.get_editor_property("font")
    font.size = 20
    end_label.set_editor_property("font", font)
    end_label.set_editor_property("color_and_opacity", unreal.SlateColor(GOLD))


def check_bottom_band():
    """Stop before building if the three bottom blocks would overlap.

    They did on the first pass -- the party block ran under the command rail
    because the rail was centred on the screen instead of on the space left
    over. A rendered capture hides that: the cards just draw on top of each
    other and still look deliberate.
    """
    party_right = PARTY_X + 3 * PARTY_W + 2 * PARTY_GAP
    rail_w = 6 * 140.0 + 5 * 8.0
    free = ENEMY_X - 24 - (party_right + 24)
    if free < rail_w:
        raise RuntimeError(
            "bottom band does not fit: rail needs {:.0f}px, {:.0f}px free "
            "between party (ends {:.0f}) and enemy (starts {:.0f})".format(
                rail_w, free, party_right, ENEMY_X))


def build():
    check_bottom_band()
    blueprint = create_asset()

    add(blueprint, "CanvasPanel", "RootCanvas", "")
    backdrop = add(blueprint, "Image", "Backdrop", "RootCanvas")
    backdrop.set_editor_property("color_and_opacity",
                                 unreal.LinearColor(0.0, 0.0, 0.0, 0.0))
    helper.umg_set_slot_layout(blueprint, "Backdrop", 0, 0, 1, 1, 0, 0, 0, 0)

    build_top_band(blueprint, "RootCanvas")
    build_party(blueprint, "RootCanvas")
    build_command_rail(blueprint, "RootCanvas")
    build_enemy_and_turn_end(blueprint, "RootCanvas")

    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)
    return blueprint


# ─── verification ─────────────────────────────────────────────────────────────

# Every name the widget class looks for. Missing ones are tolerated at runtime,
# which is exactly why they have to be checked here -- a typo would silently
# produce a blank field instead of an error.
EXPECTED = (
    ["RoundText", "ObjectiveText", "EnemyPanel", "EnemyPortrait", "EnemyName",
     "EnemyHPBar", "EnemyHPText", "EnemyDefense", "EnemyStatus",
     "EnemyForecast", "EndTurnButton"]
    + ["PartyCard_{}".format(i) for i in range(3)]
    + ["PartySelected_{}".format(i) for i in range(3)]
    + ["PartyPortrait_{}".format(i) for i in range(3)]
    + ["PartyName_{}".format(i) for i in range(3)]
    + ["PartyHPBar_{}".format(i) for i in range(3)]
    + ["PartyHPText_{}".format(i) for i in range(3)]
    + ["PartyAPText_{}".format(i) for i in range(3)]
    + ["PartyStatus_{}".format(i) for i in range(3)]
    + ["PartyAPPip_{}_{}".format(i, p) for i in range(3) for p in range(4)]
    + ["CommandCard_{}".format(i) for i in range(6)]
    + ["CommandButton_{}".format(i) for i in range(6)]
    + ["CommandIcon_{}".format(i) for i in range(6)]
    + ["CommandName_{}".format(i) for i in range(6)]
    + ["CommandCost_{}".format(i) for i in range(6)]
    + ["CommandCooldown_{}".format(i) for i in range(6)]
    + ["CommandDamage_{}".format(i) for i in range(6)]
    + ["CommandDisabled_{}".format(i) for i in range(6)]
    + ["CommandSelected_{}".format(i) for i in range(6)]
    + ["TurnToken_{}".format(i) for i in range(6)]
    + ["TurnPortrait_{}".format(i) for i in range(6)]
    + ["TurnName_{}".format(i) for i in range(6)]
    + ["TurnCurrent_{}".format(i) for i in range(6)]
)


def verify():
    """Re-load from disk and confirm every contract name survived the save."""
    blueprint = unreal.EditorAssetLibrary.load_asset(FULL_PATH)
    if blueprint is None:
        raise RuntimeError("{} did not save".format(FULL_PATH))
    missing = [n for n in EXPECTED
               if helper.umg_find_widget(blueprint, n) is None]
    unreal.log("[L01] verify: {}/{} names present".format(
        len(EXPECTED) - len(missing), len(EXPECTED)))
    if missing:
        raise RuntimeError("missing widgets:\n  " + "\n  ".join(missing))


build()
verify()
unreal.log("[L01] done")
