"""Shared parts for building the combat HUD layout variants.

Ten layouts have to be comparable, so they share one set of components and
one palette; only the arrangement differs. Anything a layout does not want it
simply does not create -- UCombatLayoutHUDWidget finds widgets by name and
skips the ones that are missing.

Art lives in SVN, not git: Content/SVN is a junction to the SVN working copy,
so the textures are addressed as /Game/SVN/OutSideAsset/UI/CombatHUD/... .
"""
import unreal

PACKAGE_PATH = "/Game/UI/CombatLayouts"
ART = "/Game/SVN/OutSideAsset/UI/CombatHUD"
C04 = ART + "/Concept04"

CANVAS_W, CANVAS_H = 1920.0, 1080.0
MARGIN = 20.0

# ─── tokens ───────────────────────────────────────────────────────────────────
#
# Lifted from Config/DefaultCombatHUD.ini so the layouts match the art line the
# frames were painted for.

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
SELECT_CORNER = 16.0             # how large that corner draws, in design px
SELECT_SIZE = SELECT_CORNER / SELECT_MARGIN
SELECT_BLEED = 6.0               # how far the marker sits outside its card

TEXT_COLOR = unreal.LinearColor(0.90, 0.88, 0.82, 1.0)
TEXT_DIM = unreal.LinearColor(0.62, 0.60, 0.56, 1.0)
GOLD = unreal.LinearColor(*FRAME_GOLD)
TEAL = unreal.LinearColor(0.32, 0.76, 0.72, 0.85)
HP_GREEN = unreal.LinearColor(0.32, 0.60, 0.28, 1.0)
HP_RED = unreal.LinearColor(0.68, 0.22, 0.20, 1.0)
AP_ON = unreal.LinearColor(1.0, 1.0, 1.0, 1.0)
AP_OFF = unreal.LinearColor(0.22, 0.22, 0.24, 0.85)
EMPTY_SOCKET = unreal.LinearColor(0.07, 0.07, 0.08, 1.0)

#: Portraits we actually have art for. A slot with none keeps an empty socket
#: rather than borrowing another unit's face -- the runtime overwrites it as
#: soon as the model supplies one.
PARTY_PORTRAITS = (ART + "/Portraits/T_Portrait_Knight_Cutout_v2", None, None)

#: Which painted glyph each command slot shows. Slot 0 is 이동; the rest follow
#: the mock skill order.
COMMAND_ICONS = (
    C04 + "/T_C04_Icon_Move",
    C04 + "/T_C04_Icon_BasicAttack",
    ART + "/Icons/Skills/T_Skill_ShieldBash_UI",
    ART + "/Icons/Skills/T_Skill_BindingSlash_UI",
    ART + "/Icons/Skills/T_Skill_BreakthroughSlash_UI",
    ART + "/Icons/Skills/T_Skill_ReflectStance_UI",
)

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

def create_asset(asset_name):
    """Make (or remake) a widget blueprint parented to UCombatLayoutHUDWidget."""
    full = "{}/{}".format(PACKAGE_PATH, asset_name)
    if unreal.EditorAssetLibrary.does_asset_exist(full):
        unreal.EditorAssetLibrary.delete_asset(full)

    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", unreal.CombatLayoutHUDWidget)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name, PACKAGE_PATH, unreal.WidgetBlueprint, factory)
    if asset is None:
        raise RuntimeError("could not create {}".format(full))
    # UWidgetBlueprint does not expose ParentClass to Python, so confirm the
    # parenting through the generated class instead of trusting the factory.
    generated = asset.generated_class()
    if generated is None or not unreal.MathLibrary.class_is_child_of(
            generated, unreal.CombatLayoutHUDWidget):
        raise RuntimeError("{} is not parented to CombatLayoutHUDWidget".format(
            full))
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
    "ml": (0.0, 0.5), "mc": (0.5, 0.5), "mr": (1.0, 0.5),
    "bl": (0.0, 1.0), "bc": (0.5, 1.0), "br": (1.0, 1.0),
}


def place(blueprint, name, x, y, w, h, anchor="tl", parent_size=None):
    """Position a canvas child given absolute coordinates in its parent.

    `anchor` decides which parent edge the widget stays glued to when the
    screen is not 16:9 -- left blocks hug the left, centred rails stay centred,
    right blocks hug the right.
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


def ghost_button(blueprint, name, parent, x, y, w, h, parent_size=None):
    """A hit area with no chrome of its own, so the plate under it shows."""
    button = add(blueprint, "Button", name, parent)
    style = button.get_editor_property("widget_style")
    for state in ("normal", "hovered", "pressed", "disabled"):
        brush = style.get_editor_property(state)
        brush.set_editor_property("resource_object", None)
        brush.set_editor_property("tint_color", unreal.SlateColor(
            unreal.LinearColor(1.0, 0.95, 0.85,
                               0.10 if state == "hovered" else 0.0)))
        style.set_editor_property(state, brush)
    button.set_editor_property("widget_style", style)
    place(blueprint, name, x, y, w, h, "tl", parent_size)
    return button


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

    for piece, y, tint in (("T", 0.0, top), ("B", h - band_h, bottom)):
        image(blueprint, "{}_Frame_Rail_{}".format(prefix, piece), parent,
              top_w, y, w - 2 * top_w, band_h, size,
              texture="{}/T_C04_Frame_Rail_{}".format(C04, piece), tint=tint,
              size=(RAIL_H_TILE[0] * scale, band_h),
              tiling=unreal.SlateBrushTileType.HORIZONTAL)

    for piece, x in (("L", 0.0), ("R", w - band_v)):
        image(blueprint, "{}_Frame_Rail_{}".format(prefix, piece), parent,
              x, top_h, band_v, h - top_h - bot_h, size,
              texture="{}/T_C04_Frame_Rail_{}".format(C04, piece), tint=side,
              size=(band_v, RAIL_V_TILE[1] * scale),
              tiling=unreal.SlateBrushTileType.VERTICAL)


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


def marker(blueprint, name, parent, w, h, parent_size, tint=TEAL):
    """The selection bracket, riding just outside the panel it highlights.

    Inset, it covered the content -- on the command card it cut the cooldown
    line off.
    """
    image(blueprint, name, parent, -SELECT_BLEED, -SELECT_BLEED,
          w + 2 * SELECT_BLEED, h + 2 * SELECT_BLEED, parent_size,
          texture=C04 + "/T_C04_Select_Frame", tint=tint,
          size=(SELECT_SIZE, SELECT_SIZE), margin=SELECT_MARGIN)


# ─── components ───────────────────────────────────────────────────────────────

def round_panel(blueprint, root, x, y, w=200.0, h=60.0, anchor="tl", size=18):
    body, extent = card(blueprint, "RoundPanel", root, x, y, w, h, anchor)
    label(blueprint, "RoundText", body, 0, (h - size - 10) / 2.0, w, size + 10,
          "ROUND 1", size, GOLD, "center", extent, bold=True)
    frame(blueprint, "RoundPanel", body, w, h)


def objective_panel(blueprint, root, x, y, w=340.0, h=60.0, anchor="tr",
                    size=16, icon=True):
    body, extent = card(blueprint, "ObjectivePanel", root, x, y, w, h, anchor)
    text_x = 8.0
    if icon:
        glyph = min(32.0, h - 20.0)
        image(blueprint, "ObjectiveIcon", body, 14, (h - glyph) / 2.0,
              glyph, glyph, extent,
              texture=C04 + "/T_C04_Icon_Objective", tint=GOLD)
        text_x = 14 + glyph + 8
    label(blueprint, "ObjectiveText", body, text_x, (h - size - 10) / 2.0,
          w - text_x - 12, size + 10, "목표", size, TEXT_COLOR, "center", extent)
    frame(blueprint, "ObjectivePanel", body, w, h)


def turn_row(blueprint, root, x, y, token=96.0, gap=12.0, anchor="tc",
             count=6, names=True, framed=True):
    """The turn order, as a row of framed portrait tokens."""
    for index in range(count):
        name = "TurnToken_{}".format(index)
        body, size = card(blueprint, name, root, x + index * (token + gap), y,
                          token, token, anchor)
        portrait = token * (0.72 if names else 0.82)
        px, py, pe = ring(blueprint, "TurnPortrait_{}".format(index), body,
                          (token - portrait) / 2.0, token * 0.06, portrait,
                          GOLD, size)
        image(blueprint, "TurnPortrait_{}".format(index), body,
              px, py, pe, pe, size, tint=EMPTY_SOCKET)
        if names:
            label(blueprint, "TurnName_{}".format(index), body,
                  4, token - 22, token - 8, 18, "이름", 12, TEXT_DIM,
                  "center", size)
        if framed:
            frame(blueprint, name, body, token, token)
        marker(blueprint, "TurnCurrent_{}".format(index), body, token, token,
               size)


def party_card(blueprint, root, index, x, y, w, h, anchor="tl", style="card",
               chip_gems=False):
    """One ally read-out.

    `style` picks how much of it is drawn:
      card  -- portrait, name, HP bar and text, four AP gems, status
      strip -- the same on one horizontal line
      chip  -- portrait and an HP bar only, for layouts that keep the HUD
               small; `chip_gems` adds the AP row back for the one layout that
               shows a single unit and nothing else
      hero  -- a large panel for the one unit whose turn it is
    """
    name = "PartyCard_{}".format(index)
    body, size = card(blueprint, name, root, x, y, w, h, anchor)
    portrait_art = PARTY_PORTRAITS[index]
    portrait_tint = (unreal.LinearColor(1, 1, 1, 1) if portrait_art
                     else EMPTY_SOCKET)

    def gems(gx, gy, extent, pitch):
        # Each gem gets a dim backing that is never hidden: the widget
        # collapses spent gems, and a gem that just disappears reads as "this
        # unit has fewer slots" instead of "spent".
        for pip in range(4):
            image(blueprint, "PartyAPPipBg_{}_{}".format(index, pip), body,
                  gx + pip * pitch, gy, extent, extent, size,
                  texture=ART + "/Elements/T_Badge_Gem_AP", tint=AP_OFF)
        for pip in range(4):
            image(blueprint, "PartyAPPip_{}_{}".format(index, pip), body,
                  gx + pip * pitch, gy, extent, extent, size,
                  texture=ART + "/Elements/T_Badge_Gem_AP", tint=AP_ON)

    if style == "chip":
        px, py, pe = ring(blueprint, "PartyPortrait_{}".format(index), body,
                          6, (h - (h - 12)) / 2.0 + 6, h - 12, GOLD, size)
        image(blueprint, "PartyPortrait_{}".format(index), body,
              px, py, pe, pe, size, texture=portrait_art, tint=portrait_tint)
        left = h + 4
        gem_run = 4 * 22.0 if chip_gems else 0.0
        label(blueprint, "PartyName_{}".format(index), body,
              left, 6, w - left - 8 - gem_run, 20, "이름", 13, TEXT_COLOR,
              "left", size)
        bar(blueprint, "PartyHPBar_{}".format(index), body,
            left, h - 26, w - left - 8, 12, HP_GREEN, size)
        if chip_gems:
            gems(w - gem_run - 8, 6, 20, 22)

    elif style == "strip":
        portrait = h - 20
        px, py, pe = ring(blueprint, "PartyPortrait_{}".format(index), body,
                          10, 10, portrait, GOLD, size)
        image(blueprint, "PartyPortrait_{}".format(index), body,
              px, py, pe, pe, size, texture=portrait_art, tint=portrait_tint)
        left = portrait + 22
        run = w - left - 12
        hp_text = 68.0
        label(blueprint, "PartyName_{}".format(index), body,
              left, 8, run, 24, "이름", 16, TEXT_COLOR, "left", size, bold=True)
        bar(blueprint, "PartyHPBar_{}".format(index), body,
            left, 36, run - hp_text - 6, 14, HP_GREEN, size)
        label(blueprint, "PartyHPText_{}".format(index), body,
              left + run - hp_text, 34, hp_text, 18, "0/0", 12, TEXT_DIM,
              "right", size)
        gems(left, 56, 18, 21)
        label(blueprint, "PartyStatus_{}".format(index), body,
              left + 96, 56, run - 96, 18, "", 12, GOLD, "right", size)

    elif style == "hero":
        portrait = h * 0.56
        px, py, pe = ring(blueprint, "PartyPortrait_{}".format(index), body,
                          18, 18, portrait, GOLD, size)
        image(blueprint, "PartyPortrait_{}".format(index), body,
              px, py, pe, pe, size, texture=portrait_art, tint=portrait_tint)
        left = portrait + 34
        label(blueprint, "PartyName_{}".format(index), body,
              left, 22, w - left - 20, 32, "이름", 24, TEXT_COLOR, "left",
              size, bold=True)
        bar(blueprint, "PartyHPBar_{}".format(index), body,
            left, 62, w - left - 20, 20, HP_GREEN, size)
        label(blueprint, "PartyHPText_{}".format(index), body,
              left, 86, w - left - 20, 22, "0/0", 15, TEXT_DIM, "left", size)
        gems(left, 116, 26, 30)
        label(blueprint, "PartyAPText_{}".format(index), body,
              left + 124, 118, 60, 22, "0/0", 14, TEXT_DIM, "left", size)
        label(blueprint, "PartyStatus_{}".format(index), body,
              18, h - 42, w - 36, 24, "", 15, GOLD, "left", size)

    else:  # card
        portrait = min(84.0, h * 0.5)
        px, py, pe = ring(blueprint, "PartyPortrait_{}".format(index), body,
                          14, 14, portrait, GOLD, size)
        image(blueprint, "PartyPortrait_{}".format(index), body,
              px, py, pe, pe, size, texture=portrait_art, tint=portrait_tint)
        left = portrait + 22
        label(blueprint, "PartyName_{}".format(index), body,
              left, 16, w - left - 14, 24, "이름", 16, TEXT_COLOR, "left",
              size, bold=True)
        bar(blueprint, "PartyHPBar_{}".format(index), body,
            left, 46, w - left - 14, 14, HP_GREEN, size)
        label(blueprint, "PartyHPText_{}".format(index), body,
              left, 62, w - left - 14, 18, "0/0", 12, TEXT_DIM, "left", size)
        gems(left, 84, 20, 23)
        label(blueprint, "PartyAPText_{}".format(index), body,
              14, portrait + 20, portrait, 16, "0/0", 11, TEXT_DIM, "center",
              size)
        label(blueprint, "PartyStatus_{}".format(index), body,
              14, h - 40, w - 28, 20, "", 12, GOLD, "left", size)

    frame(blueprint, name, body, w, h)
    marker(blueprint, "PartySelected_{}".format(index), body, w, h, size)


def command_card(blueprint, root, index, x, y, w, h, anchor="tl", style="card",
                 angle=None):
    """One command slot. Slot 0 is 이동, 1..5 are skills.

    `style`:
      card -- painted icon, name, damage, cooldown, cost gem
      icon -- the glyph and the cost only, for minimal layouts
    """
    name = "CommandCard_{}".format(index)
    body, size = card(blueprint, name, root, x, y, w, h, anchor)

    if angle is not None:
        # A hand of cards needs the fan; canvas slots cannot rotate, so the
        # widget's own render transform does it. The pivot is the bottom edge
        # so the cards splay from a common point rather than spinning in place.
        widget = helper.umg_find_widget(blueprint, name)
        widget.set_render_transform_angle(angle)
        pivot = unreal.Vector2D(0.5, 1.4)
        widget.set_editor_property("render_transform_pivot", pivot)

    if style == "icon":
        glyph = min(w, h) * 0.78
        image(blueprint, "CommandIcon_{}".format(index), body,
              (w - glyph) / 2.0, (h - glyph) / 2.0, glyph, glyph, size,
              texture=COMMAND_ICONS[index], tint=unreal.LinearColor(1, 1, 1, 1))
        gem = 26.0
        image(blueprint, "CommandCostGem_{}".format(index), body,
              w - gem - 10, 10, gem, gem, size,
              texture=ART + "/Elements/T_Badge_Gem_AP",
              tint=unreal.LinearColor(1, 1, 1, 1))
        label(blueprint, "CommandCost_{}".format(index), body,
              w - gem - 10, 14, gem, 20, "0", 13,
              unreal.LinearColor(1.0, 0.97, 0.90, 1.0), "center", size,
              bold=True)
        label(blueprint, "CommandCooldown_{}".format(index), body,
              0, h - 30, w, 20, "", 12, GOLD, "center", size)
    else:
        glyph = min(w - 40, h * 0.46)
        image(blueprint, "CommandIcon_{}".format(index), body,
              (w - glyph) / 2.0, 16, glyph, glyph, size,
              texture=COMMAND_ICONS[index], tint=unreal.LinearColor(1, 1, 1, 1))
        text_top = 16 + glyph + 8
        label(blueprint, "CommandName_{}".format(index), body,
              8, text_top, w - 16, 24, "이름", 15, TEXT_COLOR, "center", size)
        label(blueprint, "CommandDamage_{}".format(index), body,
              8, text_top + 26, w - 16, 20, "0~0", 13, TEXT_DIM, "center", size)
        label(blueprint, "CommandCooldown_{}".format(index), body,
              8, text_top + 46, w - 16, 20, "", 12, GOLD, "center", size)
        gem = 32.0
        image(blueprint, "CommandCostGem_{}".format(index), body,
              w - gem - 16, 14, gem, gem, size,
              texture=ART + "/Elements/T_Badge_Gem_AP",
              tint=unreal.LinearColor(1, 1, 1, 1))
        label(blueprint, "CommandCost_{}".format(index), body,
              w - gem - 16, 19, gem, 22, "0", 15,
              unreal.LinearColor(1.0, 0.97, 0.90, 1.0), "center", size,
              bold=True)

    frame(blueprint, name, body, w, h)

    # The button covers the card and is added after the plate but before the
    # overlays, so the overlays draw on top. Text and images are hit-test
    # invisible, so nothing steals the click.
    ghost_button(blueprint, "CommandButton_{}".format(index), body,
                 0, 0, w, h, size)

    # Unusable skills are pressed down by ink with the material over it.
    #
    # The ink has to be *inside* the widget the runtime hides, because that is
    # the only thing the runtime knows how to toggle. Two earlier shapes both
    # failed quietly: a separate ink image the runtime never knew the name of
    # (so every card stayed equally dark), and ink painted onto the Border's
    # own background (a Border keeps the engine's outline brush, which draws a
    # rule rather than a fill, so the card barely changed).
    disabled_name = "CommandDisabled_{}".format(index)
    disabled = add(blueprint, "Border", disabled_name, body)
    disabled.set_editor_property("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
    paint(disabled, tint=unreal.LinearColor(1.0, 1.0, 1.0, 0.0))
    place(blueprint, disabled_name, 0, 0, w, h, "tl", size)

    stack = "CommandDisabledStack_{}".format(index)
    add(blueprint, "CanvasPanel", stack, disabled_name)
    image(blueprint, "CommandDisabledInk_{}".format(index), stack, 0, 0, w, h,
          size, tint=unreal.LinearColor(0.0, 0.0, 0.0, 0.66))
    image(blueprint, "CommandDisabledMat_{}".format(index), stack, 0, 0, w, h,
          size, texture=ART + "/Materials/M_UI_CRPG_DisabledOverlay",
          tint=unreal.LinearColor(1, 1, 1, 1))

    marker(blueprint, "CommandSelected_{}".format(index), body, w, h, size)


def enemy_panel(blueprint, root, x, y, w, h, anchor="br", style="wide"):
    """The selected enemy read-out."""
    body, size = card(blueprint, "EnemyPanel", root, x, y, w, h, anchor)
    enemy_ring = unreal.LinearColor(0.62, 0.24, 0.20, 1.0)

    if style == "tall":
        portrait = min(w - 40, 110.0)
        px, py, pe = ring(blueprint, "EnemyPortrait", body,
                          (w - portrait) / 2.0, 18, portrait, enemy_ring, size)
        image(blueprint, "EnemyPortrait", body, px, py, pe, pe, size,
              texture=ART + "/Portraits/T_Portrait_Spider",
              tint=unreal.LinearColor(1, 1, 1, 1))
        top = 18 + portrait + 10
        label(blueprint, "EnemyName", body, 12, top, w - 24, 26, "적", 17,
              TEXT_COLOR, "center", size, bold=True)
        bar(blueprint, "EnemyHPBar", body, 16, top + 32, w - 32, 14, HP_RED, size)
        label(blueprint, "EnemyHPText", body, 16, top + 48, w - 32, 18, "0/0",
              12, TEXT_DIM, "center", size)
        label(blueprint, "EnemyDefense", body, 16, top + 70, w - 32, 18, "",
              12, TEXT_DIM, "center", size)
        label(blueprint, "EnemyStatus", body, 12, top + 96, w - 24, 40, "", 12,
              GOLD, "center", size)
        label(blueprint, "EnemyForecast", body, 12, top + 140, w - 24, 20, "",
              12, TEXT_DIM, "center", size)
    else:
        portrait = min(h - 44, 76.0)
        px, py, pe = ring(blueprint, "EnemyPortrait", body, 14, 14, portrait,
                          enemy_ring, size)
        image(blueprint, "EnemyPortrait", body, px, py, pe, pe, size,
              texture=ART + "/Portraits/T_Portrait_Spider",
              tint=unreal.LinearColor(1, 1, 1, 1))
        left = portrait + 24
        label(blueprint, "EnemyName", body, left, 14, w - left - 14, 26, "적",
              16, TEXT_COLOR, "left", size, bold=True)
        bar(blueprint, "EnemyHPBar", body, left, 46, w - left - 14, 14, HP_RED,
            size)
        label(blueprint, "EnemyHPText", body, left, 62, 96, 18, "0/0", 12,
              TEXT_DIM, "left", size)
        label(blueprint, "EnemyDefense", body, left + 100, 62,
              w - left - 114, 18, "", 12, TEXT_DIM, "right", size)
        label(blueprint, "EnemyStatus", body, 14, h - 30, (w - 28) * 0.5, 18,
              "", 12, GOLD, "left", size)
        label(blueprint, "EnemyForecast", body, 14 + (w - 28) * 0.5, h - 30,
              (w - 28) * 0.5, 18, "", 12, TEXT_DIM, "right", size)

    frame(blueprint, "EnemyPanel", body, w, h)


def end_turn(blueprint, root, x, y, w=300.0, h=64.0, anchor="br", size=19):
    """The end-turn button: a hit area with its own framed plate inside."""
    ghost_button(blueprint, "EndTurnButton", root, x, y, w, h)
    place(blueprint, "EndTurnButton", x, y, w, h, anchor)

    add(blueprint, "CanvasPanel", "EndTurnCanvas", "EndTurnButton")
    plate, extent = "EndTurnCanvas", (w, h)
    image(blueprint, "EndTurnInk", plate, 0, 0, w, h, extent, tint=CARD_INK)
    image(blueprint, "EndTurnSurface", plate, 0, 0, w, h, extent,
          texture=C04 + "/T_C04_Surface", tint=SURFACE_TINT,
          size=(SURFACE_TILE, SURFACE_TILE),
          tiling=unreal.SlateBrushTileType.BOTH)
    glyph = min(40.0, h - 20)
    text_w = size * 4.0
    block = glyph + 10 + text_w
    image(blueprint, "EndTurnIcon", plate, (w - block) / 2.0, (h - glyph) / 2.0,
          glyph, glyph, extent, texture=C04 + "/T_C04_Icon_EndTurn", tint=GOLD)
    label(blueprint, "EndTurnLabel", plate, (w - block) / 2.0 + glyph + 10,
          (h - size - 10) / 2.0, text_w, size + 10, "턴 종료", size, GOLD,
          "left", extent, bold=True)
    frame(blueprint, "EndTurn", plate, w, h)


# ─── verification ─────────────────────────────────────────────────────────────

#: Names the runtime drives that every layout must carry. The rest are
#: optional on purpose -- a layout that drops the enemy panel or the names is
#: making a design point, and the runtime tolerates the gap.
REQUIRED = (
    ["RoundText", "EndTurnButton"]
    + ["PartyCard_{}".format(i) for i in range(1)]
    + ["CommandCard_{}".format(i) for i in range(6)]
    + ["CommandButton_{}".format(i) for i in range(6)]
    + ["CommandCost_{}".format(i) for i in range(6)]
    + ["CommandSelected_{}".format(i) for i in range(6)]
    + ["CommandDisabled_{}".format(i) for i in range(6)]
)

#: Everything the runtime will write to if it is there. Counted, not required,
#: so each layout's report says how much of the model it actually shows.
OPTIONAL = (
    ["ObjectiveText", "EnemyPanel", "EnemyPortrait", "EnemyName", "EnemyHPBar",
     "EnemyHPText", "EnemyDefense", "EnemyStatus", "EnemyForecast"]
    + ["PartyCard_{}".format(i) for i in range(1, 3)]
    + ["PartySelected_{}".format(i) for i in range(3)]
    + ["PartyPortrait_{}".format(i) for i in range(3)]
    + ["PartyName_{}".format(i) for i in range(3)]
    + ["PartyHPBar_{}".format(i) for i in range(3)]
    + ["PartyHPText_{}".format(i) for i in range(3)]
    + ["PartyAPText_{}".format(i) for i in range(3)]
    + ["PartyStatus_{}".format(i) for i in range(3)]
    + ["PartyAPPip_{}_{}".format(i, p) for i in range(3) for p in range(4)]
    + ["CommandIcon_{}".format(i) for i in range(6)]
    + ["CommandName_{}".format(i) for i in range(6)]
    + ["CommandCooldown_{}".format(i) for i in range(6)]
    + ["CommandDamage_{}".format(i) for i in range(6)]
    + ["TurnToken_{}".format(i) for i in range(6)]
    + ["TurnPortrait_{}".format(i) for i in range(6)]
    + ["TurnName_{}".format(i) for i in range(6)]
    + ["TurnCurrent_{}".format(i) for i in range(6)]
)


def verify(asset_name):
    """Re-load from disk and confirm the contract names survived the save.

    Missing names are tolerated at runtime, which is exactly why they have to
    be checked here -- a typo produces a blank field, not an error.
    """
    full = "{}/{}".format(PACKAGE_PATH, asset_name)
    blueprint = unreal.EditorAssetLibrary.load_asset(full)
    if blueprint is None:
        raise RuntimeError("{} did not save".format(full))
    missing = [n for n in REQUIRED
               if helper.umg_find_widget(blueprint, n) is None]
    if missing:
        raise RuntimeError("{}: missing required widgets:\n  {}".format(
            asset_name, "\n  ".join(missing)))
    shown = sum(1 for n in OPTIONAL
                if helper.umg_find_widget(blueprint, n) is not None)
    return shown, len(OPTIONAL)
