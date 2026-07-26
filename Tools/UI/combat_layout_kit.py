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
KK = "/Game/SVN/OutSideAsset/UI/KayKit"

CANVAS_W, CANVAS_H = 1920.0, 1080.0
MARGIN = 20.0

# ─── grid and tokens ──────────────────────────────────────────────────────────
#
# One number runs the whole kit: UNIT. Every part's canvas is a multiple of it
# and every joint lands on a multiple of it, so assembling a panel is addition
# rather than measurement. The old art line needed CORNER_RATIO, BAND_MIN and a
# nine-slice margin of 0.312253, all of them read off the drawing after the
# fact -- and a mistake in the last one made the selection frame swallow a card.

#: Design pixels per grid unit. Parts are drawn at 4x this and drawn down.
UNIT = 32.0

#: Frame weights. `corner` and `link` are the sizes the pieces draw at; `band`
#: is how much of the link is moulding, which is what content has to clear.
#: The band ratio is a property of the drawn moulding, read once off the master
#: -- but nothing depends on it lining up, because the pieces were cut from one
#: continuous drawing and provably share a cross-section.
HEAVY = {"corner": 2 * UNIT, "link": UNIT, "band": UNIT * 85.0 / 128.0,
         "prefix": "KK_HFrame"}
LIGHT = {"corner": UNIT, "link": UNIT / 2.0, "band": UNIT * 34.0 / 128.0,
         "prefix": "KK_LFrame"}

SELECT_CORNER = UNIT       # KK_Select_Corner draws at 1U
SELECT_LINK = UNIT / 2.0   # KK_Select_Link_* draw at 0.5U
SELECT_BLEED = 4.0         # how far the marker sits outside its panel

FILL_TILE = 4 * UNIT       # KK_Fill_* are 4U squares
RING_HOLE_RATIO = 11.0 / 16.0
GEM = UNIT                 # KK_Gem_* draw at 1U
ICON = 2 * UNIT            # KK_Icon_* are 2U, glyph inside the middle 3/4
BAR_TILE = UNIT / 2.0      # KK_Bar_* are 0.5U wide, 1U tall
VEIL_TILE = UNIT

# KayKit palette. Bright matte, no oxidised bronze -- the characters are light
# low-poly chibi and the old dark-fantasy HUD read as a different game.
WHITE = unreal.LinearColor(1.0, 1.0, 1.0, 1.0)
TEXT_COLOR = unreal.LinearColor(0.99, 0.97, 0.93, 1.0)
TEXT_DIM = unreal.LinearColor(0.86, 0.84, 0.80, 1.0)
GOLD = unreal.LinearColor(0.91, 0.72, 0.29, 1.0)          # #E8B84B
#: 양피지·주황처럼 밝은 판 위에 얹는 글씨. 흰색은 씻겨서 안 읽힌다.
TEXT_ON_LIGHT = unreal.LinearColor(0.22, 0.13, 0.07, 1.0)
INK = unreal.LinearColor(0.13, 0.14, 0.17, 0.92)
HP_GREEN = unreal.LinearColor(0.24, 0.56, 0.35, 1.0)      # #3E8E5A
HP_RED = unreal.LinearColor(0.82, 0.27, 0.25, 1.0)        # #D2453F
STONE = unreal.LinearColor(0.76, 0.78, 0.81, 1.0)         # #C3C7CE
STONE_DIM = unreal.LinearColor(0.43, 0.45, 0.49, 1.0)     # #6E747E
AP_ON = WHITE
AP_OFF = unreal.LinearColor(0.45, 0.47, 0.51, 0.9)
EMPTY_SOCKET = unreal.LinearColor(0.30, 0.32, 0.36, 1.0)
SELECT_TINT = WHITE        # the marker is already gold in the art

#: Portraits we actually have art for. A slot with none keeps an empty socket
#: rather than borrowing another unit's face -- the runtime overwrites it as
#: soon as the model supplies one.
PARTY_PORTRAITS = (ART + "/Portraits/T_Portrait_Knight_Cutout_v2", None, None)

#: Which painted glyph each command slot shows. Slot 0 is 이동; the rest follow
#: the mock skill order.
COMMAND_ICONS = (
    KK + "/KK_Icon_Move",
    KK + "/KK_Icon_BasicAttack",
    KK + "/KK_Icon_ShieldBash",
    KK + "/KK_Icon_PinSlash",
    KK + "/KK_Icon_Breakthrough",
    KK + "/KK_Icon_Riposte",
)


#: 8x256 세로 그라데이션. 알파 램프라 아트라인과 무관하게 쓰인다.
#: 위가 밝고 아래가 어두운 조명은 KayKit도 같으므로 그대로 가져다 쓴다.
SHADE = C04 + "/T_C04_Shade"
SHADE_SIZE = (8.0, 256.0)

#: 역할마다 다른 면과 색조.
#:
#: 처음엔 모든 패널이 KK_Fill_Stone 하나를 썼고, 그 Stone은 값 범위가
#: 132~148뿐이라 사실상 단색이었다. 화면 전체가 같은 회색 판으로 보였다.
#: 면을 나누고 색조를 주면 어느 판이 무엇인지 형태 전에 색으로 읽힌다.
SURFACES = {
    "party":      ("Wood", unreal.LinearColor(0.78, 0.60, 0.46, 1.0)),
    "party_lead": ("Wood", unreal.LinearColor(0.95, 0.74, 0.55, 1.0)),
    "command":    ("Stone", unreal.LinearColor(0.80, 0.85, 0.94, 1.0)),
    "enemy":      ("Stone", unreal.LinearColor(0.92, 0.58, 0.54, 1.0)),
    "turn":       ("Stone", unreal.LinearColor(0.74, 0.79, 0.88, 1.0)),
    "info":       ("Parchment", unreal.LinearColor(0.96, 0.88, 0.70, 1.0)),
    "action":     ("Wood", unreal.LinearColor(1.00, 0.72, 0.36, 1.0)),
}


#: 그리기 층. 캔버스 자식 순서에만 기대면 부품이 많은 카드에서 프레임이
#: 사라진다 -- 아군 카드와 커맨드 카드가 정확히 그렇게 됐다.
Z_FILL, Z_CONTENT, Z_FRAME, Z_OVERLAY, Z_MARKER = 0, 10, 20, 30, 40


def snap(value, step):
    """Round a length up to the grid so a frame lands on whole pieces."""
    import math
    return max(step, math.ceil(value / step) * step)


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


def place(blueprint, name, x, y, w, h, anchor="tl", parent_size=None,
          z_order=None):
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
    if z_order is not None:
        # 캔버스는 자식 순서대로 그리지만, 순서만 믿으면 부품이 많은 카드에서
        # 뒤로 밀려 사라진다. 프레임과 표시물은 층을 못 박는다.
        widget = helper.umg_find_widget(blueprint, name)
        widget.get_editor_property("slot").set_editor_property("z_order",
                                                              int(z_order))


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


def image(blueprint, name, parent, x, y, w, h, parent_size=None,
          z_order=None, **paint_args):
    z_order = Z_CONTENT if z_order is None else z_order
    widget = add(blueprint, "Image", name, parent)
    if paint_args:
        paint(widget, **paint_args)
    place(blueprint, name, x, y, w, h, "tl", parent_size, z_order)
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
    place(blueprint, name, x, y, w, h, "tl", parent_size, Z_CONTENT)
    return block


def bar(blueprint, name, parent, x, y, w, h, fill, parent_size=None):
    """A progress bar dressed with the KayKit rail pieces.

    Slate draws the bar itself, so this is the one place a brush still gets
    stretched -- but the pieces tile along the bar's axis at their own size,
    so the rounded ends keep their shape instead of smearing.
    """
    progress = add(blueprint, "ProgressBar", name, parent)
    style = progress.get_editor_property("widget_style")
    for slot, texture, tint in (
            ("background_image", KK + "/KK_Bar_Track_Link", WHITE),
            ("fill_image", KK + "/KK_Bar_Link", fill)):
        brush = style.get_editor_property(slot)
        brush.set_editor_property("resource_object", art(texture))
        brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
        brush.set_editor_property("tiling", unreal.SlateBrushTileType.HORIZONTAL)
        extent = unreal.DeprecateSlateVector2D()
        extent.set_editor_property("x", BAR_TILE)
        extent.set_editor_property("y", float(h))
        brush.set_editor_property("image_size", extent)
        brush.set_editor_property("tint_color", unreal.SlateColor(tint))
        style.set_editor_property(slot, brush)
    progress.set_editor_property("widget_style", style)
    progress.set_editor_property("fill_color_and_opacity", WHITE)
    progress.set_editor_property("percent", 0.75)
    # The art keeps a quarter of its height as clear margin above and below the
    # rail, so a widget the height of the wanted bar draws one half that thick.
    # Give the widget the room the art expects and keep it on the same centre
    # line the caller asked for.
    place(blueprint, name, x, y - h / 2.0, w, h * 2.0, "tl", parent_size,
          Z_CONTENT)
    return progress


def ghost_button(blueprint, name, parent, x, y, w, h, parent_size=None,
                 z_order=None):
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
    place(blueprint, name, x, y, w, h, "tl", parent_size,
          Z_OVERLAY - 1 if z_order is None else z_order)
    return button


# ─── KayKit frame ─────────────────────────────────────────────────────────────

def flip(blueprint, name, horizontal=False, vertical=False):
    """Mirror a placed piece. Four corners come from one drawing."""
    widget = helper.umg_find_widget(blueprint, name)
    scale = unreal.Vector2D(-1.0 if horizontal else 1.0,
                            -1.0 if vertical else 1.0)
    transform = widget.get_editor_property("render_transform")
    transform.set_editor_property("scale", scale)
    widget.set_editor_property("render_transform", transform)
    widget.set_editor_property("render_transform_pivot",
                               unreal.Vector2D(0.5, 0.5))


def frame(blueprint, prefix, parent, w, h, weight=None):
    """Lay a frame around a w x h panel by butting pieces together.

    Two corners and three links per weight, mirrored into all four sides. The
    pieces were cut from one drawn panel, so the corner's arm and the link's
    cross-section are the same pixels -- joins cannot show a step, which is
    what the previous set could not manage across separate generations.

    Nothing is stretched. The run between corners is filled with whole links;
    `card()` snaps panel sizes so the division comes out even.
    """
    weight = weight or (HEAVY if min(w, h) >= 8 * UNIT else LIGHT)
    size = (w, h)
    corner, link = weight["corner"], weight["link"]
    art_prefix = weight["prefix"]

    def piece(name, source, x, y, pw, ph, **flips):
        image(blueprint, name, parent, x, y, pw, ph, size, z_order=Z_FRAME,
              texture="{}/{}_{}".format(KK, art_prefix, source), tint=WHITE)
        if flips:
            flip(blueprint, name, **flips)

    runs_h = max(1, int(round((w - 2 * corner) / link)))
    runs_v = max(1, int(round((h - 2 * corner) / link)))
    step_h = (w - 2 * corner) / runs_h
    step_v = (h - 2 * corner) / runs_v

    for i in range(runs_h):
        x = corner + i * step_h
        piece("{}_FT{}".format(prefix, i), "Link_T", x, 0, step_h, link)
        piece("{}_FB{}".format(prefix, i), "Link_B", x, h - link, step_h, link)
    for i in range(runs_v):
        y = corner + i * step_v
        piece("{}_FL{}".format(prefix, i), "Link_S", 0, y, link, step_v)
        piece("{}_FR{}".format(prefix, i), "Link_S", w - link, y, link, step_v,
              horizontal=True)

    piece("{}_FCTL".format(prefix), "Corner_T", 0, 0, corner, corner)
    piece("{}_FCTR".format(prefix), "Corner_T", w - corner, 0, corner, corner,
          horizontal=True)
    piece("{}_FCBL".format(prefix), "Corner_B", 0, h - corner, corner, corner)
    piece("{}_FCBR".format(prefix), "Corner_B", w - corner, h - corner,
          corner, corner, horizontal=True)
    return weight["band"]


def card(blueprint, name, parent, x, y, w, h, anchor="tl", parent_size=None,
         role="command"):
    """A panel: ink, tiled fill, then an inner canvas. The frame goes on last.

    Sizes snap to the grid so the frame divides into whole pieces. The inner
    canvas matters: hiding the card has to hide its contents, and a widget only
    hides its own subtree.
    """
    weight = HEAVY if min(w, h) >= 8 * UNIT else LIGHT
    w = snap(w, weight["link"])
    h = snap(h, weight["link"])

    border = add(blueprint, "Border", name, parent)
    border.set_editor_property("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
    paint(border, tint=INK)
    place(blueprint, name, x, y, w, h, anchor, parent_size)

    inner = "{}_Canvas".format(name)
    add(blueprint, "CanvasPanel", inner, name)
    size = (w, h)
    # Tiled at its own size rather than scaled to the card: shrinking a surface
    # to fit is what made the previous art line read as flat black.
    fill, tint = SURFACES.get(role, SURFACES["command"])
    image(blueprint, "{}_Fill".format(name), inner, 0, 0, w, h, size,
          z_order=Z_FILL, texture="{}/KK_Fill_{}".format(KK, fill), tint=tint,
          size=(FILL_TILE, FILL_TILE),
          tiling=unreal.SlateBrushTileType.BOTH)
    # 위가 밝고 아래가 어두운 조명. 없으면 판이 납작하게 읽힌다.
    image(blueprint, "{}_Shade".format(name), inner, 0, 0, w, h, size,
          z_order=Z_FILL + 1, texture=SHADE, tint=WHITE, size=SHADE_SIZE)
    return inner, size


def ring(blueprint, name, parent, x, y, extent, tint, parent_size,
         enemy=False):
    """A portrait bezel, returning where the portrait goes inside it."""
    inner = extent * RING_HOLE_RATIO
    offset = (extent - inner) / 2.0
    image(blueprint, name + "_Ring", parent, x, y, extent, extent, parent_size,
          texture="{}/KK_Ring_{}".format(KK, "Enemy" if enemy else "Portrait"),
          tint=tint)
    return x + offset, y + offset, inner


def marker(blueprint, name, parent, w, h, parent_size, tint=None):
    """The selection bracket, assembled from a corner and two links.

    A single stretched ring is what swallowed a card last time: its corner was
    a fraction of the source, and the fraction was read off the drawing.
    """
    del tint
    # The runtime shows and hides this by name, so the group carries the
    # contract name and the pieces ride inside it.
    root = name
    add(blueprint, "CanvasPanel", root, parent)
    place(blueprint, root, -SELECT_BLEED, -SELECT_BLEED,
          w + 2 * SELECT_BLEED, h + 2 * SELECT_BLEED, "tl", parent_size,
          Z_MARKER)
    gw, gh = w + 2 * SELECT_BLEED, h + 2 * SELECT_BLEED
    size = (gw, gh)
    c, l = SELECT_CORNER, SELECT_LINK

    runs_h = max(1, int(round((gw - 2 * c) / l)))
    runs_v = max(1, int(round((gh - 2 * c) / l)))
    step_h, step_v = (gw - 2 * c) / runs_h, (gh - 2 * c) / runs_v

    def piece(tag, source, x, y, pw, ph, **flips):
        nm = "{}_{}".format(name, tag)
        image(blueprint, nm, root, x, y, pw, ph, size,
              texture="{}/KK_Select_{}".format(KK, source), tint=WHITE)
        if flips:
            flip(blueprint, nm, **flips)

    for i in range(runs_h):
        x = c + i * step_h
        piece("T%d" % i, "Link_H", x, 0, step_h, l)
        piece("B%d" % i, "Link_H", x, gh - l, step_h, l, vertical=True)
    for i in range(runs_v):
        y = c + i * step_v
        piece("L%d" % i, "Link_V", 0, y, l, step_v)
        piece("R%d" % i, "Link_V", gw - l, y, l, step_v, horizontal=True)
    piece("CTL", "Corner", 0, 0, c, c)
    piece("CTR", "Corner", gw - c, 0, c, c, horizontal=True)
    piece("CBL", "Corner", 0, gh - c, c, c, vertical=True)
    piece("CBR", "Corner", gw - c, gh - c, c, c, horizontal=True, vertical=True)

    helper.umg_set_widget_is_variable(blueprint, root, True)


# ─── components ───────────────────────────────────────────────────────────────

def round_panel(blueprint, root, x, y, w=200.0, h=60.0, anchor="tl", size=18):
    body, extent = card(blueprint, "RoundPanel", root, x, y, w, h, anchor,
                        role="info")
    label(blueprint, "RoundText", body, 0, (h - size - 10) / 2.0, w, size + 10,
          "ROUND 1", size, TEXT_ON_LIGHT, "center", extent, bold=True)
    frame(blueprint, "RoundPanel", body, w, h)


def objective_panel(blueprint, root, x, y, w=340.0, h=60.0, anchor="tr",
                    size=16, icon=True):
    body, extent = card(blueprint, "ObjectivePanel", root, x, y, w, h, anchor,
                        role="info")
    text_x = 8.0
    if icon:
        glyph = min(32.0, h - 20.0)
        image(blueprint, "ObjectiveIcon", body, 14, (h - glyph) / 2.0,
              glyph, glyph, extent,
              texture=KK + "/KK_Icon_Objective", tint=WHITE)
        text_x = 14 + glyph + 8
    label(blueprint, "ObjectiveText", body, text_x, (h - size - 10) / 2.0,
          w - text_x - 12, size + 10, "목표", size, TEXT_ON_LIGHT, "center",
          extent)
    frame(blueprint, "ObjectivePanel", body, w, h)


def turn_row(blueprint, root, x, y, token=96.0, gap=12.0, anchor="tc",
             count=6, names=True, framed=True):
    """The turn order, as a row of framed portrait tokens."""
    for index in range(count):
        name = "TurnToken_{}".format(index)
        body, size = card(blueprint, name, root, x + index * (token + gap), y,
                          token, token, anchor, role="turn")
        # 이름표가 있으면 링을 줄여 프레임 안쪽에 이름 자리를 만든다. 프레임은
        # 내용보다 위 층이라, 자리를 안 비우면 이름 아래쪽이 잘린다.
        portrait = token * (0.60 if names else 0.82)
        px, py, pe = ring(blueprint, "TurnPortrait_{}".format(index), body,
                          (token - portrait) / 2.0, token * 0.05, portrait,
                          WHITE, size)
        image(blueprint, "TurnPortrait_{}".format(index), body,
              px, py, pe, pe, size, tint=EMPTY_SOCKET)
        if names:
            label(blueprint, "TurnName_{}".format(index), body,
                  4, token * 0.05 + portrait + 2, token - 8, 16, "이름", 11,
                  TEXT_DIM, "center", size)
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
    # 차례인 유닛의 카드만 밝게 -- 선택 테두리와 함께 두 겹으로 읽힌다.
    body, size = card(blueprint, name, root, x, y, w, h, anchor,
                      role="party_lead" if index == 0 else "party")
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
                  texture=KK + "/KK_Gem_Off", tint=WHITE)
        for pip in range(4):
            image(blueprint, "PartyAPPip_{}_{}".format(index, pip), body,
                  gx + pip * pitch, gy, extent, extent, size,
                  texture=KK + "/KK_Gem_On", tint=WHITE)

    if style == "chip":
        px, py, pe = ring(blueprint, "PartyPortrait_{}".format(index), body,
                          6, (h - (h - 12)) / 2.0 + 6, h - 12, WHITE, size)
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
        # 아군 행: 왼쪽에 둥근 초상화, 오른쪽 위에 이름, 그 아래 HP 바와
        # 숫자, 맨 아래 AP 보석 줄. 상태이상은 이름 줄 오른쪽 끝.
        #
        # 자리를 높이의 비율로 잡는다. 고정 오프셋으로 짰더니 통합 바(72px)와
        # 상단 정보 바(150px)에서 줄이 서로 올라타 글자가 겹쳤다. 같은 부품이
        # 배치안마다 다른 높이로 들어가는 게 이 키트의 전제다.
        pad = max(8.0, h * 0.09)
        portrait = h - 2 * pad
        px, py, pe = ring(blueprint, "PartyPortrait_{}".format(index), body,
                          pad, pad, portrait, WHITE, size)
        image(blueprint, "PartyPortrait_{}".format(index), body,
              px, py, pe, pe, size, texture=portrait_art, tint=portrait_tint)

        left = portrait + pad * 2.2
        run = w - left - pad * 1.6
        line = h * 0.26
        name_size = int(max(12, min(22, h * 0.17)))
        info_size = int(max(10, min(16, h * 0.12)))

        status_w = min(run * 0.42, 150.0)
        label(blueprint, "PartyName_{}".format(index), body,
              left, h * 0.08, run - status_w, line, "이름", name_size,
              TEXT_COLOR, "left", size, bold=True)
        label(blueprint, "PartyStatus_{}".format(index), body,
              left + run - status_w, h * 0.09, status_w, line, "",
              info_size, GOLD, "right", size)

        hp_text = min(run * 0.30, 96.0)
        bar(blueprint, "PartyHPBar_{}".format(index), body,
            left, h * 0.47, run - hp_text - 8, max(10.0, h * 0.11), HP_GREEN,
            size)
        label(blueprint, "PartyHPText_{}".format(index), body,
              left + run - hp_text, h * 0.40, hp_text, line, "0/0", info_size,
              TEXT_COLOR, "right", size)

        gem = max(14.0, min(24.0, h * 0.16))
        gems(left, h * 0.72, gem, gem * 1.18)
        label(blueprint, "PartyAPText_{}".format(index), body,
              left + run - 64, h * 0.71, 64, line, "0/0", info_size, TEXT_DIM,
              "right", size)

    elif style == "hero":
        portrait = h * 0.56
        px, py, pe = ring(blueprint, "PartyPortrait_{}".format(index), body,
                          18, 18, portrait, WHITE, size)
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
                          14, 14, portrait, WHITE, size)
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
    body, size = card(blueprint, name, root, x, y, w, h, anchor, role="command")

    if angle is not None:
        # A hand of cards needs the fan; canvas slots cannot rotate, so the
        # widget's own render transform does it. The pivot is the bottom edge
        # so the cards splay from a common point rather than spinning in place.
        widget = helper.umg_find_widget(blueprint, name)
        widget.set_render_transform_angle(angle)
        pivot = unreal.Vector2D(0.5, 1.4)
        widget.set_editor_property("render_transform_pivot", pivot)

    if style == "compact":
        # 고리 배치용 납작한 카드. 세로로 긴 카드를 원 둘레에 놓으면 위아래
        # 카드가 상단·하단 띠에 닿는다.
        #
        # 아이콘을 카드 높이만큼 키웠더니 글자 자리가 26px만 남아 이름이 카드
        # 밖으로 흘렀다. 아이콘은 위쪽 절반만 쓰고 글자는 아래에 쌓는다.
        glyph = min(w * 0.34, h * 0.42)
        image(blueprint, "CommandIcon_{}".format(index), body,
              10, 10, glyph, glyph, size,
              texture=COMMAND_ICONS[index], tint=WHITE)

        left = glyph + 18
        label(blueprint, "CommandName_{}".format(index), body,
              left, 12, w - left - 10, 26, "이름", 16, TEXT_COLOR, "left",
              size, bold=True)
        label(blueprint, "CommandCostLine_{}".format(index), body,
              left, 38, w - left - 10, 22, "", 14, TEXT_COLOR, "left", size,
              bold=True)

        label(blueprint, "CommandDamage_{}".format(index), body,
              10, h - 52, w - 20, 22, "0~0", 14, TEXT_DIM, "center", size)
        label(blueprint, "CommandCooldown_{}".format(index), body,
              10, h - 30, w - 20, 22, "", 13, GOLD, "center", size)

        gem = 30.0
        image(blueprint, "CommandCostGem_{}".format(index), body,
              w - gem - 10, 10, gem, gem, size,
              texture=KK + "/KK_Badge_Round", tint=WHITE)
        label(blueprint, "CommandCost_{}".format(index), body,
              w - gem - 10, 15, gem, 22, "0", 15,
              unreal.LinearColor(1.0, 0.97, 0.90, 1.0), "center", size,
              bold=True)

    elif style == "icon":
        glyph = min(w, h) * 0.78
        image(blueprint, "CommandIcon_{}".format(index), body,
              (w - glyph) / 2.0, (h - glyph) / 2.0, glyph, glyph, size,
              texture=COMMAND_ICONS[index], tint=unreal.LinearColor(1, 1, 1, 1))
        gem = 26.0
        image(blueprint, "CommandCostGem_{}".format(index), body,
              w - gem - 10, 10, gem, gem, size,
              texture=KK + "/KK_Badge_Round", tint=WHITE)
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
              texture=KK + "/KK_Badge_Round", tint=WHITE)
        label(blueprint, "CommandCost_{}".format(index), body,
              w - gem - 16, 19, gem, 22, "0", 15,
              unreal.LinearColor(1.0, 0.97, 0.90, 1.0), "center", size,
              bold=True)
        # 배지 숫자는 작고 아이콘에 붙어 있어 훑을 때 안 걸린다. 시안은 카드
        # 맨 아래에 "AP n"을 한 번 더 적는다. 고를 때 보는 건 이쪽이다.
        label(blueprint, "CommandCostLine_{}".format(index), body,
              8, h - 40, w - 16, 24, "", 15, TEXT_COLOR, "center", size,
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
    place(blueprint, disabled_name, 0, 0, w, h, "tl", size, Z_OVERLAY)

    stack = "CommandDisabledStack_{}".format(index)
    add(blueprint, "CanvasPanel", stack, disabled_name)
    image(blueprint, "CommandDisabledInk_{}".format(index), stack, 0, 0, w, h,
          size, tint=unreal.LinearColor(0.0, 0.0, 0.0, 0.66))
    image(blueprint, "CommandDisabledMat_{}".format(index), stack, 0, 0, w, h,
          size, texture=KK + "/KK_Veil_Disabled", tint=WHITE,
          size=(VEIL_TILE, VEIL_TILE),
          tiling=unreal.SlateBrushTileType.BOTH)

    marker(blueprint, "CommandSelected_{}".format(index), body, w, h, size)


def enemy_panel(blueprint, root, x, y, w, h, anchor="br", style="wide"):
    """The selected enemy read-out."""
    body, size = card(blueprint, "EnemyPanel", root, x, y, w, h, anchor,
                      role="enemy")
    enemy_ring = WHITE

    if style == "tall":
        portrait = min(w - 40, 110.0)
        px, py, pe = ring(blueprint, "EnemyPortrait", body,
                          (w - portrait) / 2.0, 18, portrait, enemy_ring, size,
                          enemy=True)
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
                          enemy_ring, size, enemy=True)
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
    image(blueprint, "EndTurnInk", plate, 0, 0, w, h, extent, tint=INK)
    action_fill, action_tint = SURFACES["action"]
    image(blueprint, "EndTurnSurface", plate, 0, 0, w, h, extent,
          texture="{}/KK_Fill_{}".format(KK, action_fill), tint=action_tint,
          size=(FILL_TILE, FILL_TILE),
          tiling=unreal.SlateBrushTileType.BOTH)
    image(blueprint, "EndTurnShade", plate, 0, 0, w, h, extent,
          texture=SHADE, tint=WHITE, size=SHADE_SIZE)
    glyph = min(40.0, h - 20)
    text_w = size * 4.0
    block = glyph + 10 + text_w
    image(blueprint, "EndTurnIcon", plate, (w - block) / 2.0, (h - glyph) / 2.0,
          glyph, glyph, extent, texture=KK + "/KK_Icon_EndTurn", tint=WHITE)
    label(blueprint, "EndTurnLabel", plate, (w - block) / 2.0 + glyph + 10,
          (h - size - 10) / 2.0, text_w, size + 10, "턴 종료", size,
          TEXT_COLOR, "left", extent, bold=True)
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
