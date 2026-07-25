"""Author layout 1 (classic CRPG) of the combat HUD as a real Widget Blueprint.

The widget class finds everything by name, so this script's job is to place
widgets under the agreed names and dress them in the Concept04 art. Nothing
here knows about combat rules -- the names are the whole contract.

Everything sits on canvas panels with absolute 1920x1080 coordinates, anchored
to the screen edge the block belongs to, so the layout holds at 16:9 (the
logical canvas the project's ShortestSide scaling produces at that ratio).

Art lives in SVN, not git: Content/SVN is a junction to the SVN working copy,
so the textures are addressed as /Game/SVN/OutSideAsset/UI/CombatHUD/... .

Run through Tools/RunEditorPython.ps1 -- a bare -ExecutePythonScript exits 0
even when this raises.
"""
import unreal

PACKAGE_PATH = "/Game/UI/CombatLayouts"
ASSET_NAME = "WBP_CombatLayout_01_ClassicCRPG"
FULL_PATH = "{}/{}".format(PACKAGE_PATH, ASSET_NAME)

ART = "/Game/SVN/OutSideAsset/UI/CombatHUD"
C04 = ART + "/Concept04"

CANVAS_W, CANVAS_H = 1920.0, 1080.0

# ─── tokens ───────────────────────────────────────────────────────────────────
#
# Lifted from Config/DefaultCombatHUD.ini so this layout matches the art line
# the frames were painted for.

FRAME_GOLD = (0.730, 0.490, 0.239, 1.0)
SURFACE_TINT = unreal.LinearColor(0.180, 0.188, 0.194, 1.0)
CARD_INK = unreal.LinearColor(0.0012, 0.0018, 0.0016, 0.99)

#: The frame is lit from overhead. All four edges are the same texture at the
#: same strength, so without this the bottom moulding renders as brightly as
#: the top and the card reads flat.
SHADE_TOP, SHADE_SIDE, SHADE_BOTTOM = 1.00, 0.58, 0.18

#: Corner art draws at a fraction of the frame's own short side, floored so it
#: cannot vanish on a chip. A fixed size made the same ornament 14% of a hero
#: card and 42% of a companion.
CORNER_RATIO, CORNER_MIN, CORNER_MAX = 0.16, 16.0, 52.0

#: Rails cannot go below this. Coupled all the way down to the corner's arm, a
#: small card gave a 2.6px rail: a 132px painted moulding drawn 51x smaller.
#: Its bead-dark-bead profile cannot exist in three pixels.
BAND_MIN = 7.0

# Measured off the Concept04 art (SourceArt/.../Concept04Metrics.json).
CORNER_TOP = (460.0, 451.0)
CORNER_BOTTOM = (461.0, 438.0)
CORNER_BAND_H, CORNER_BAND_V = 132.0, 113.0
RAIL_H_TILE = (64.0, 132.0)      # T/B rails, tile horizontally
RAIL_V_TILE = (113.0, 64.0)      # L/R rails, tile vertically
SURFACE_TILE = 1254.0
SHADE_SIZE = (8.0, 256.0)
RING_HOLE_RATIO = 0.7705         # how much of the ring the portrait may fill
SELECT_MARGIN = 0.312253         # nine-slice margin on T_C04_Select_Frame
SELECT_CORNER = 16.0            # 그려질 모서리 크기(디자인 픽셀)
SELECT_BLEED = 6.0              # 카드 바깥으로 물리는 양
SELECT_SIZE = SELECT_CORNER / SELECT_MARGIN

TEXT_COLOR = unreal.LinearColor(0.90, 0.88, 0.82, 1.0)
TEXT_DIM = unreal.LinearColor(0.62, 0.60, 0.56, 1.0)
GOLD = unreal.LinearColor(*FRAME_GOLD)
TEAL = unreal.LinearColor(0.32, 0.76, 0.72, 0.85)
HP_GREEN = unreal.LinearColor(0.32, 0.60, 0.28, 1.0)
HP_RED = unreal.LinearColor(0.68, 0.22, 0.20, 1.0)
AP_ON = unreal.LinearColor(1.0, 1.0, 1.0, 1.0)
AP_OFF = unreal.LinearColor(0.22, 0.22, 0.24, 0.85)
EMPTY_SOCKET = unreal.LinearColor(0.07, 0.07, 0.08, 1.0)

# Bottom band geometry. The three blocks share one row, so their widths are
# declared together and checked against the canvas before anything is built.
PARTY_X, PARTY_W, PARTY_H, PARTY_GAP = 20.0, 210.0, 168.0, 8.0
COMMAND_W, COMMAND_H, COMMAND_GAP = 140.0, 188.0, 8.0
ENEMY_W, ENEMY_H = 300.0, 120.0
ENEMY_X = CANVAS_W - 20.0 - ENEMY_W

helper = unreal.MCPythonHelper
_LOADED = {}


def art(path):
    """Load a piece of art once, and fail loudly if it is not there.

    A missing brush resource does not error at runtime -- the widget just draws
    a white box, which is easy to mistake for a placeholder left in on purpose.
    """
    if path not in _LOADED:
        asset = unreal.EditorAssetLibrary.load_asset(path)
        if asset is None:
            raise RuntimeError("art missing: {}".format(path))
        _LOADED[path] = asset
    return _LOADED[path]


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


def brush_of(widget):
    """The brush property, which is named differently per widget type."""
    for field in ("brush", "background"):
        try:
            return field, widget.get_editor_property(field)
        except Exception:
            continue
    raise RuntimeError("no brush on {}".format(type(widget).__name__))


def paint(widget, texture=None, tint=None, size=None, tiling=None, margin=None):
    """Point a widget's brush at art.

    `tiling` repeats the source at its own size instead of scaling it, which is
    the difference between a painted grain that stays the thickness it was
    drawn and one that smears. `margin` switches the brush to nine-slice so the
    corners keep their shape while the middle stretches.
    """
    field, brush = brush_of(widget)
    if texture is not None:
        brush.set_editor_property("resource_object", art(texture))
    if size is not None:
        # ImageSize is FDeprecateSlateVector2D, not FVector2D. Python exposes
        # the type but not a constructor that takes values, so it is built
        # empty and filled through its own fields.
        extent = unreal.DeprecateSlateVector2D()
        extent.set_editor_property("x", float(size[0]))
        extent.set_editor_property("y", float(size[1]))
        brush.set_editor_property("image_size", extent)
    if margin is not None:
        brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.BOX)
        brush.set_editor_property("margin", unreal.Margin(*[margin] * 4))
    elif tiling is not None:
        brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
        brush.set_editor_property("tiling", tiling)
    if tint is not None:
        brush.set_editor_property("tint_color", unreal.SlateColor(tint))
    widget.set_editor_property(field, brush)
    return widget


def image(blueprint, name, parent, x, y, w, h, parent_size=None, **paint_args):
    widget = add(blueprint, "Image", name, parent)
    if paint_args:
        paint(widget, **paint_args)
    place(blueprint, name, x, y, w, h, "tl", parent_size)
    return widget


def hud_font(size, bold=False):
    info = unreal.SlateFontInfo()
    info.set_editor_property("font_object", art(ART + "/Fonts/F_HUD_NotoSerifKR"))
    info.set_editor_property("typeface_font_name",
                             unreal.Name("Bold" if bold else "Regular"))
    info.set_editor_property("size", int(size))
    return info


def label(blueprint, name, parent, x, y, w, h, text, size=14,
          color=TEXT_COLOR, align="left", parent_size=None, bold=False):
    block = add(blueprint, "TextBlock", name, parent)
    block.set_editor_property("text", unreal.Text(text))
    block.set_editor_property("font", hud_font(size, bold))
    block.set_editor_property("color_and_opacity", unreal.SlateColor(color))
    block.set_editor_property("justification", {
        "left": unreal.TextJustify.LEFT,
        "center": unreal.TextJustify.CENTER,
        "right": unreal.TextJustify.RIGHT,
    }[align])
    place(blueprint, name, x, y, w, h, "tl", parent_size)
    return block


def bar(blueprint, name, parent, x, y, w, h, fill, parent_size=None):
    """A progress bar drawn as a recessed slot with a flat fill.

    The default Slate bar carries its own rounded style, which reads as a
    different art line next to the painted frames, so both halves are replaced.
    """
    progress = add(blueprint, "ProgressBar", name, parent)
    style = progress.get_editor_property("widget_style")
    background = style.get_editor_property("background_image")
    background.set_editor_property("resource_object", None)
    background.set_editor_property("tint_color", unreal.SlateColor(
        unreal.LinearColor(0.03, 0.03, 0.035, 0.95)))
    style.set_editor_property("background_image", background)
    filled = style.get_editor_property("fill_image")
    filled.set_editor_property("resource_object", None)
    filled.set_editor_property("tint_color", unreal.SlateColor(fill))
    style.set_editor_property("fill_image", filled)
    progress.set_editor_property("widget_style", style)
    progress.set_editor_property("fill_color_and_opacity",
                                 unreal.LinearColor(1, 1, 1, 1))
    progress.set_editor_property("percent", 0.75)
    place(blueprint, name, x, y, w, h, "tl", parent_size)
    return progress


# ─── Concept04 frame ──────────────────────────────────────────────────────────

def corner_extent(w, h):
    return max(CORNER_MIN, min(CORNER_MAX, min(w, h) * CORNER_RATIO))


def frame(blueprint, prefix, parent, w, h):
    """Lay the Concept04 frame around a w x h panel.

    Four corners plus four rails, all derived from one painted corner and one
    painted rail mirrored into place. Generating separate art per corner would
    not be symmetric, because each generation draws its own ornament.

    The corner draws at a fixed size and never scales within the frame; the
    rails repeat to reach it. Repeating rather than stretching keeps the bead
    at the thickness the corner hands it.
    """
    size = (w, h)
    extent = corner_extent(w, h)
    scale = extent / max(CORNER_TOP)
    top_w, top_h = CORNER_TOP[0] * scale, CORNER_TOP[1] * scale
    bot_w, bot_h = CORNER_BOTTOM[0] * scale, CORNER_BOTTOM[1] * scale
    band_h = max(BAND_MIN, CORNER_BAND_H * scale)
    band_v = max(BAND_MIN, CORNER_BAND_V * scale)

    def shade(factor):
        return unreal.LinearColor(FRAME_GOLD[0] * factor,
                                  FRAME_GOLD[1] * factor,
                                  FRAME_GOLD[2] * factor, 1.0)

    top, side, bottom = shade(SHADE_TOP), shade(SHADE_SIDE), shade(SHADE_BOTTOM)

    for piece, x, y, pw, ph, tint in (
            ("Corner_TL", 0.0, 0.0, top_w, top_h, top),
            ("Corner_TR", w - top_w, 0.0, top_w, top_h, top),
            ("Corner_BL", 0.0, h - bot_h, bot_w, bot_h, bottom),
            ("Corner_BR", w - bot_w, h - bot_h, bot_w, bot_h, bottom)):
        image(blueprint, "{}_Frame_{}".format(prefix, piece), parent,
              x, y, pw, ph, size,
              texture="{}/T_C04_Frame_{}".format(C04, piece), tint=tint)

    rail_run_h = w - 2 * top_w
    for piece, y, tint in (("T", 0.0, top), ("B", h - band_h, bottom)):
        image(blueprint, "{}_Frame_Rail_{}".format(prefix, piece), parent,
              top_w, y, rail_run_h, band_h, size,
              texture="{}/T_C04_Frame_Rail_{}".format(C04, piece), tint=tint,
              size=(RAIL_H_TILE[0] * scale, band_h),
              tiling=unreal.SlateBrushTileType.HORIZONTAL)

    rail_run_v = h - top_h - bot_h
    for piece, x in (("L", 0.0), ("R", w - band_v)):
        image(blueprint, "{}_Frame_Rail_{}".format(prefix, piece), parent,
              x, top_h, band_v, rail_run_v, size,
              texture="{}/T_C04_Frame_Rail_{}".format(C04, piece), tint=side,
              size=(band_v, RAIL_V_TILE[1] * scale),
              tiling=unreal.SlateBrushTileType.VERTICAL)

    return max(band_h, band_v)


def card(blueprint, name, parent, x, y, w, h, anchor="tl", parent_size=None):
    """A panel: ink, painted surface, overhead shade, then an inner canvas.

    The inner canvas matters: hiding the card has to hide its contents, and a
    widget only hides its own subtree. Laying the contents on the root canvas
    instead would leave orphaned text floating over an empty slot whenever a
    party member is missing.

    The frame goes on last, by the caller, so it sits above the content.
    """
    border = add(blueprint, "Border", name, parent)
    border.set_editor_property("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
    paint(border, tint=CARD_INK)
    place(blueprint, name, x, y, w, h, anchor, parent_size)

    inner = "{}_Canvas".format(name)
    add(blueprint, "CanvasPanel", inner, name)
    size = (w, h)

    # Tiled at its own 1254px size rather than scaled to the card: shrinking it
    # to fit is what made the grain disappear and the panel read as flat black.
    image(blueprint, "{}_Surface".format(name), inner, 0, 0, w, h, size,
          texture=C04 + "/T_C04_Surface", tint=SURFACE_TINT,
          size=(SURFACE_TILE, SURFACE_TILE),
          tiling=unreal.SlateBrushTileType.BOTH)
    # A vertical ramp, 8px wide -- it stretches sideways losslessly.
    image(blueprint, "{}_Shade".format(name), inner, 0, 0, w, h, size,
          texture=C04 + "/T_C04_Shade",
          tint=unreal.LinearColor(1, 1, 1, 1), size=SHADE_SIZE)
    return inner, size


def ring(blueprint, name, parent, x, y, extent, tint, parent_size):
    """A portrait bezel, returning where the portrait goes inside it.

    The hole ratio is how much of the ring the portrait may fill before the
    bezel starts covering it, so the portrait is inset rather than matched.
    """
    inner = extent * RING_HOLE_RATIO
    offset = (extent - inner) / 2.0
    image(blueprint, name + "_Ring", parent, x, y, extent, extent, parent_size,
          texture=C04 + "/T_C04_Ring", tint=tint)
    return x + offset, y + offset, inner


# ─── layout ───────────────────────────────────────────────────────────────────

def build_top_band(blueprint, root):
    """Round counter on the left, turn order across the middle, objective right."""
    panel, size = card(blueprint, "RoundPanel", root, 24, 20, 200, 60, "tl")
    label(blueprint, "RoundText", panel, 0, 16, 200, 28,
          "ROUND 1", 20, GOLD, "center", size, bold=True)
    frame(blueprint, "RoundPanel", panel, 200, 60)

    # Turn order: six tokens centred as a row, so the row stays put at 4:3.
    token, gap = 96.0, 12.0
    row_w = 6 * token + 5 * gap
    start_x = (CANVAS_W - row_w) / 2.0
    for index in range(6):
        name = "TurnToken_{}".format(index)
        x = start_x + index * (token + gap)
        body, size = card(blueprint, name, root, x, 16, token, token, "tc")
        px, py, pe = ring(blueprint, "TurnPortrait_{}".format(index), body,
                          (token - 68) / 2.0, 6, 68, GOLD, size)
        image(blueprint, "TurnPortrait_{}".format(index), body,
              px, py, pe, pe, size, tint=EMPTY_SOCKET)
        label(blueprint, "TurnName_{}".format(index), body, 4, 74, token - 8, 18,
              "이름", 12, TEXT_DIM, "center", size)
        frame(blueprint, name, body, token, token)
        # Drawn last so the active marker sits above the frame.
        image(blueprint, "TurnCurrent_{}".format(index), body,
              -SELECT_BLEED, -SELECT_BLEED,
              token + 2 * SELECT_BLEED, token + 2 * SELECT_BLEED, size,
              texture=C04 + "/T_C04_Select_Frame", tint=TEAL,
              size=(SELECT_SIZE, SELECT_SIZE), margin=SELECT_MARGIN)

    panel, size = card(blueprint, "ObjectivePanel", root, CANVAS_W - 364, 20,
                       340, 60, "tr")
    image(blueprint, "ObjectiveIcon", panel, 16, 14, 32, 32, size,
          texture=C04 + "/T_C04_Icon_Objective", tint=GOLD)
    label(blueprint, "ObjectiveText", panel, 56, 16, 268, 28,
          "목표", 16, TEXT_COLOR, "center", size)
    frame(blueprint, "ObjectivePanel", panel, 340, 60)


def build_party(blueprint, root):
    """Three ally cards along the bottom-left: portrait, HP, AP gems, status."""
    top = CANVAS_H - 24 - PARTY_H
    for index in range(3):
        name = "PartyCard_{}".format(index)
        x = PARTY_X + index * (PARTY_W + PARTY_GAP)
        body, size = card(blueprint, name, root, x, top, PARTY_W, PARTY_H, "bl")

        px, py, pe = ring(blueprint, "PartyPortrait_{}".format(index), body,
                          14, 14, 84, GOLD, size)
        portrait = PARTY_PORTRAITS[index]
        image(blueprint, "PartyPortrait_{}".format(index), body,
              px, py, pe, pe, size, texture=portrait,
              tint=unreal.LinearColor(1, 1, 1, 1) if portrait
              else EMPTY_SOCKET)

        label(blueprint, "PartyName_{}".format(index), body,
              106, 16, 90, 24, "이름", 16, TEXT_COLOR, "left", size, bold=True)
        bar(blueprint, "PartyHPBar_{}".format(index), body,
            106, 46, 90, 14, HP_GREEN, size)
        label(blueprint, "PartyHPText_{}".format(index), body,
              106, 62, 90, 18, "0/0", 12, TEXT_DIM, "left", size)

        # AP as four gems plus a number. Each gem gets a dim backing that is
        # never hidden: the widget collapses spent gems, and a gem that just
        # disappears reads as "this unit has fewer slots" instead of "spent".
        for pip in range(4):
            image(blueprint, "PartyAPPipBg_{}_{}".format(index, pip), body,
                  106 + pip * 23, 84, 20, 20, size,
                  texture=ART + "/Elements/T_Badge_Gem_AP", tint=AP_OFF)
        for pip in range(4):
            image(blueprint, "PartyAPPip_{}_{}".format(index, pip), body,
                  106 + pip * 23, 84, 20, 20, size,
                  texture=ART + "/Elements/T_Badge_Gem_AP", tint=AP_ON)
        label(blueprint, "PartyAPText_{}".format(index), body,
              14, 104, 84, 16, "0/0", 11, TEXT_DIM, "center", size)

        label(blueprint, "PartyStatus_{}".format(index), body,
              14, 128, PARTY_W - 28, 20, "", 12, GOLD, "left", size)

        frame(blueprint, name, body, PARTY_W, PARTY_H)
        # Selection last so it reads above the frame it highlights.
        image(blueprint, "PartySelected_{}".format(index), body,
              -SELECT_BLEED, -SELECT_BLEED,
              PARTY_W + 2 * SELECT_BLEED, PARTY_H + 2 * SELECT_BLEED, size,
              texture=C04 + "/T_C04_Select_Frame", tint=TEAL,
              size=(SELECT_SIZE, SELECT_SIZE), margin=SELECT_MARGIN)


#: Portraits we actually have art for. A slot with none keeps an empty socket
#: rather than borrowing another unit's face -- the runtime overwrites it as
#: soon as the model supplies one.
PARTY_PORTRAITS = (
    ART + "/Portraits/T_Portrait_Knight_Cutout_v2",
    None,
    None,
)


#: Which painted glyph each command slot shows. Slot 0 is 이동; the rest follow
#: the mock skill order.
COMMAND_ICONS = (
    C04 + "/T_C04_Icon_Move",
    ART + "/Icons/Skills/T_Skill_ShieldBash_UI",
    ART + "/Icons/Skills/T_Skill_BindingSlash_UI",
    ART + "/Icons/Skills/T_Skill_BreakthroughSlash_UI",
    ART + "/Icons/Skills/T_Skill_ReflectStance_UI",
    C04 + "/T_C04_Icon_BasicAttack",
)


def build_command_rail(blueprint, root):
    """Six command cards centred at the bottom. Slot 0 is 이동, 1..5 are skills."""
    row_w = 6 * COMMAND_W + 5 * COMMAND_GAP
    # Centre the rail in the gap the other two blocks leave, not on the screen
    # centre -- the party block reaches past screen centre at this card size.
    free_left = PARTY_X + 3 * PARTY_W + 2 * PARTY_GAP + 24
    free_right = ENEMY_X - 24
    start_x = free_left + (free_right - free_left - row_w) / 2.0
    top = CANVAS_H - 24 - COMMAND_H

    for index in range(6):
        name = "CommandCard_{}".format(index)
        x = start_x + index * (COMMAND_W + COMMAND_GAP)
        body, size = card(blueprint, name, root, x, top, COMMAND_W, COMMAND_H,
                          "bc")

        image(blueprint, "CommandIcon_{}".format(index), body,
              (COMMAND_W - 84) / 2.0, 18, 84, 84, size,
              texture=COMMAND_ICONS[index],
              tint=unreal.LinearColor(1, 1, 1, 1))
        label(blueprint, "CommandName_{}".format(index), body,
              8, 112, COMMAND_W - 16, 24, "이름", 15, TEXT_COLOR, "center", size)
        label(blueprint, "CommandDamage_{}".format(index), body,
              8, 138, COMMAND_W - 16, 20, "0~0", 13, TEXT_DIM, "center", size)
        label(blueprint, "CommandCooldown_{}".format(index), body,
              8, 158, COMMAND_W - 16, 20, "", 12, GOLD, "center", size)

        # Cost badge on a gem in the top-right corner, matching the concept.
        image(blueprint, "CommandCostGem_{}".format(index), body,
              COMMAND_W - 48, 14, 32, 32, size,
              texture=ART + "/Elements/T_Badge_Gem_AP",
              tint=unreal.LinearColor(1, 1, 1, 1))
        label(blueprint, "CommandCost_{}".format(index), body,
              COMMAND_W - 48, 19, 32, 22, "0", 15,
              unreal.LinearColor(1.0, 0.97, 0.90, 1.0), "center", size,
              bold=True)

        frame(blueprint, name, body, COMMAND_W, COMMAND_H)

        # The button covers the card and is added after the plate but before
        # the overlays, so the overlays draw on top. Text and images are
        # hit-test invisible, so nothing steals the click.
        button = add(blueprint, "Button", "CommandButton_{}".format(index), body)
        style = button.get_editor_property("widget_style")
        for state in ("normal", "hovered", "pressed", "disabled"):
            state_brush = style.get_editor_property(state)
            state_brush.set_editor_property("resource_object", None)
            state_brush.set_editor_property("tint_color", unreal.SlateColor(
                unreal.LinearColor(1.0, 0.95, 0.85,
                                   0.10 if state == "hovered" else 0.0)))
            style.set_editor_property(state, state_brush)
        button.set_editor_property("widget_style", style)
        place(blueprint, "CommandButton_{}".format(index),
              0, 0, COMMAND_W, COMMAND_H, "tl", size)

        # 못 쓰는 스킬은 먹판으로 눌러 두고 그 위에 재질을 덮는다. 재질만
        # 얹었을 때는 카드가 눈에 띄게 어두워지지 않아서 쓸 수 있는 스킬과
        # 구분이 안 됐다.
        #
        # 먹판이 위젯이 감추는 대상 그 자체여야 한다. 먹판을 따로 둔 첫
        # 시도는 C++이 그 이름을 모르니 한 번도 안 감춰졌고, 결국 모든
        # 카드가 똑같이 어두워졌다.
        disabled_name = "CommandDisabled_{}".format(index)
        disabled = add(blueprint, "Border", disabled_name, body)
        disabled.set_editor_property("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
        paint(disabled, tint=unreal.LinearColor(0.0, 0.0, 0.0, 0.66))
        place(blueprint, disabled_name, 0, 0, COMMAND_W, COMMAND_H, "tl", size)
        paint(add(blueprint, "Image", "CommandDisabledMat_{}".format(index),
                  disabled_name),
              texture=ART + "/Materials/M_UI_CRPG_DisabledOverlay",
              tint=unreal.LinearColor(1, 1, 1, 1))
        image(blueprint, "CommandSelected_{}".format(index), body,
              -SELECT_BLEED, -SELECT_BLEED,
              COMMAND_W + 2 * SELECT_BLEED, COMMAND_H + 2 * SELECT_BLEED, size,
              texture=C04 + "/T_C04_Select_Frame", tint=TEAL,
              size=(SELECT_SIZE, SELECT_SIZE), margin=SELECT_MARGIN)


def build_enemy_and_turn_end(blueprint, root):
    """Enemy read-out bottom-right, with the end-turn button under it."""
    button_h = 64.0
    top = CANVAS_H - 24 - button_h - 12 - ENEMY_H
    body, size = card(blueprint, "EnemyPanel", root, ENEMY_X, top,
                      ENEMY_W, ENEMY_H, "br")

    px, py, pe = ring(blueprint, "EnemyPortrait", body, 14, 14, 76,
                      unreal.LinearColor(0.62, 0.24, 0.20, 1.0), size)
    image(blueprint, "EnemyPortrait", body, px, py, pe, pe, size,
          texture=ART + "/Portraits/T_Portrait_Spider",
          tint=unreal.LinearColor(1, 1, 1, 1))

    label(blueprint, "EnemyName", body, 100, 14, 186, 26, "적", 16,
          TEXT_COLOR, "left", size, bold=True)
    bar(blueprint, "EnemyHPBar", body, 100, 46, 186, 14, HP_RED, size)
    label(blueprint, "EnemyHPText", body, 100, 62, 96, 18, "0/0", 12,
          TEXT_DIM, "left", size)
    label(blueprint, "EnemyDefense", body, 196, 62, 90, 18, "", 12,
          TEXT_DIM, "right", size)
    label(blueprint, "EnemyStatus", body, 14, 88, 140, 18, "", 12,
          GOLD, "left", size)
    label(blueprint, "EnemyForecast", body, 158, 88, 128, 18, "", 12,
          TEXT_DIM, "right", size)
    frame(blueprint, "EnemyPanel", body, ENEMY_W, ENEMY_H)

    button_top = CANVAS_H - 24 - button_h
    button = add(blueprint, "Button", "EndTurnButton", root)
    style = button.get_editor_property("widget_style")
    for state, alpha in (("normal", 0.0), ("hovered", 0.12),
                         ("pressed", 0.0), ("disabled", 0.0)):
        state_brush = style.get_editor_property(state)
        state_brush.set_editor_property("resource_object", None)
        state_brush.set_editor_property("tint_color", unreal.SlateColor(
            unreal.LinearColor(1.0, 0.95, 0.85, alpha)))
        style.set_editor_property(state, state_brush)
    button.set_editor_property("widget_style", style)
    place(blueprint, "EndTurnButton", ENEMY_X, button_top, ENEMY_W, button_h,
          "br")

    # The button holds one child, so the plate lives inside it and the frame
    # and label sit on the plate.
    add(blueprint, "CanvasPanel", "EndTurnCanvas", "EndTurnButton")
    plate = "EndTurnCanvas"
    plate_size = (ENEMY_W, button_h)
    image(blueprint, "EndTurnInk", plate, 0, 0, ENEMY_W, button_h, plate_size,
          tint=CARD_INK)
    image(blueprint, "EndTurnSurface", plate, 0, 0, ENEMY_W, button_h,
          plate_size, texture=C04 + "/T_C04_Surface", tint=SURFACE_TINT,
          size=(SURFACE_TILE, SURFACE_TILE),
          tiling=unreal.SlateBrushTileType.BOTH)
    image(blueprint, "EndTurnIcon", plate, 78, 12, 40, 40, plate_size,
          texture=C04 + "/T_C04_Icon_EndTurn", tint=GOLD)
    label(blueprint, "EndTurnLabel", plate, 126, 18, 120, 28, "턴 종료", 19,
          GOLD, "left", plate_size, bold=True)
    frame(blueprint, "EndTurn", plate, ENEMY_W, button_h)


def check_bottom_band():
    """Stop before building if the three bottom blocks would overlap.

    They did on the first pass -- the party block ran under the command rail
    because the rail was centred on the screen instead of on the space left
    over. A rendered capture hides that: the cards just draw on top of each
    other and still look deliberate.
    """
    party_right = PARTY_X + 3 * PARTY_W + 2 * PARTY_GAP
    rail_w = 6 * COMMAND_W + 5 * COMMAND_GAP
    free = ENEMY_X - 24 - (party_right + 24)
    if free < rail_w:
        raise RuntimeError(
            "bottom band does not fit: rail needs {:.0f}px, {:.0f}px free "
            "between party (ends {:.0f}) and enemy (starts {:.0f})".format(
                rail_w, free, party_right, ENEMY_X))


def check_materials():
    """UI brushes can only draw materials whose domain is User Interface.

    A surface-domain material assigned to a brush renders as flat white, which
    reads as a missing texture rather than as a wrong setting.
    """
    for name in ("M_UI_CRPG_DisabledOverlay",):
        material = art("{}/Materials/{}".format(ART, name))
        domain = material.get_editor_property("material_domain")
        if domain != unreal.MaterialDomain.MD_UI:
            raise RuntimeError("{} domain is {}, needs MD_UI".format(
                name, domain))
    unreal.log("[L01] materials are UI domain")


def build():
    check_bottom_band()
    check_materials()
    blueprint = create_asset()

    add(blueprint, "CanvasPanel", "RootCanvas", "")
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
    unreal.log("[L01] art referenced: {} assets".format(len(_LOADED)))


build()
verify()
unreal.log("[L01] done")
