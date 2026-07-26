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
#: `rail`은 직선 구간을 화면에 그릴 두께다. 시안 실측: 레일은 ~10px 얇은
#: 선이고 모서리 브래킷만 덩어리다. 원판의 띠를 그대로 그리면 레일이
#: 모서리만큼 두꺼워져 테두리가 화면의 주인공이 된다 -- 4차 반영본이
#: 정확히 그랬다.
#: 레일 두께는 시안 실측에서 나온다. 예전 값(18/10)은 내 격자에서 고른
#: 것이라 화면에 7px로 그려졌고, 시안은 12px였다 -- 테두리가 얇아 카드가
#: 큼직하게 안 읽히는 주된 원인이었다. 아트가 위젯 폭의 약 70%를 그리므로
#: 12 / 0.7 = 17 로 역산한다.
HEAVY = {"corner": 2 * UNIT, "link": UNIT, "rail": 26.0,
         "band": UNIT * 85.0 / 128.0, "prefix": "KK_HFrame"}
LIGHT = {"corner": UNIT, "link": UNIT / 2.0, "rail": 17.0,
         "band": UNIT * 34.0 / 128.0, "prefix": "KK_LFrame"}

#: 금속 프레임. 나무와 단면 두께가 같게 그려져서 치수는 그대로 쓰고 접두사만
#: 바뀐다 -- 잘라낸 조각의 이음면 두께를 재서 확인했다(둘 다 128 / 64).
METAL = dict(HEAVY, prefix="KK_MFrame")
METAL_LIGHT = dict(LIGHT, prefix="KK_MLFrame")

#: 어느 역할이 어느 프레임을 쓰는가.
#:
#: 지금까지는 전 패널이 같은 나무 테두리였고, 역할 구분이 면 색에만 있었다.
#: 목업은 스킬 카드를 금속으로, 아군 카드를 나무로 그린다 -- 테두리가 먼저
#: 읽히므로 여기서 갈라야 형태만 봐도 무엇인지 안다.
FRAME_FAMILY = {
    "party": ("wood", "wood"),
    "party_lead": ("wood", "wood"),
    "info": ("wood", "wood"),
    "command": ("metal", "metal"),
    "enemy": ("metal", "metal"),
    "turn": ("metal", "metal"),
    "action": ("button", "button"),
}
FRAME_SETS = {
    "wood": (HEAVY, LIGHT),
    "metal": (METAL, METAL_LIGHT),
}

#: 시안 파일을 픽셀로 재서 잡은 프레임 보정색.
#:
#: 시안의 파티 프레임은 (109,67,28) 어두운 호두색인데 4차 나무 원판은 밝은
#: 주황으로 나왔다 -- 색상은 같고 밝기·채도만 높아 곱색으로 내려앉힌다.
#: 금속은 시안 (118,120,123)보다 어두워 1을 넘는 값으로 올린다. 임시 보정이고
#: 다음 원판 발주에 이 실측값이 목표로 들어간다.
#: 흰색은 아래 팔레트에서 정의된다. 여기서 이름으로 부르면 정의 전 참조라
#: 모듈이 통째로 안 읽힌다 -- 실제로 그렇게 죽었다.
FRAME_TINTS = {
    "wood": unreal.LinearColor(1.0, 1.0, 1.0, 1.0),
    "metal": unreal.LinearColor(1.0, 1.0, 1.0, 1.0),
}

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

#: 수치마다 색을 준다. 전부 흰 계열이면 위계가 글자 크기로만 갈리고, 훑을 때
#: 무엇이 HP이고 무엇이 행동력인지 형태를 읽어야 알 수 있다.
HP_TEXT = unreal.LinearColor(1.00, 0.86, 0.84, 1.0)
AP_TEXT = unreal.LinearColor(1.00, 0.88, 0.52, 1.0)
DAMAGE_TEXT = unreal.LinearColor(0.98, 0.95, 0.88, 1.0)
COOLDOWN_TEXT = unreal.LinearColor(1.00, 0.80, 0.45, 1.0)

#: 판 안쪽 위에 얹는 밝은 선과 아래에 까는 어두운 선. 이 두 줄이 "깎아 만든
#: 판"과 "색칠한 사각형"을 가른다. 목업의 면은 전부 이 층을 갖고 있다.
RIM_LIGHT = unreal.LinearColor(1.0, 0.97, 0.90, 0.35)
RIM_DARK = unreal.LinearColor(0.0, 0.0, 0.0, 0.50)
CARD_SHADOW = unreal.LinearColor(0.0, 0.0, 0.0, 0.45)

#: 숫자를 얹는 배지. 밝은 원에 진한 숫자여야 작은 크기에서 숫자가 산다.
BADGE_FACE = unreal.LinearColor(1.00, 0.96, 0.90, 1.0)
#: 시안 배지는 어두운 갈색 원에 흰 숫자다. 진한 글씨로 칠했더니 원판도
#: 어두워 숫자가 통째로 묻혔다 -- 실측에서 아주밝은 픽셀 0개였다.
BADGE_TEXT = unreal.LinearColor(1.0, 0.98, 0.94, 1.0)

#: Portraits we actually have art for. A slot with none keeps an empty socket
#: rather than borrowing another unit's face -- the runtime overwrites it as
#: soon as the model supplies one.
PARTY_PORTRAITS = (KK + "/KK_Face_Knight", KK + "/KK_Face_Archer",
                   KK + "/KK_Face_Mage")

#: 턴 순서 칸의 기본 얼굴. 게임플레이가 유닛 초상화를 주면 덮어쓴다.
#: 지금은 미리보기 장면(기사 - 독수리 - 궁수 - 독수리 - 마법사)에 맞춘다.
TURN_PORTRAITS = (KK + "/KK_Face_Knight", KK + "/KK_Face_Eagle",
                  KK + "/KK_Face_Archer", KK + "/KK_Face_Eagle",
                  KK + "/KK_Face_Mage", None)

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
    "party":      ("Wood", WHITE),
    "party_lead": ("Wood_Active", WHITE),
    "command":    ("Stone_Skill", WHITE),
    "enemy":      ("Stone_Enemy", WHITE),
    "turn":       ("Wood", WHITE),
    "info":       ("Parchment", WHITE),
    "action":     ("Wood_Active", WHITE),
}


#: 그리기 층. 캔버스 자식 순서에만 기대면 부품이 많은 카드에서 프레임이
#: 사라진다 -- 아군 카드와 커맨드 카드가 정확히 그렇게 됐다.
Z_SHADOW, Z_FILL, Z_CONTENT, Z_FRAME, Z_OVERLAY, Z_MARKER = (
    -10, 0, 10, 20, 30, 40)


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
    info.set_editor_property("font_object", art(KK + "/Fonts/F_HUD_NotoSansKR"))
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
    # 새 바 아트는 여백 없이 캔버스를 꽉 채운다. 옛 아트의 상하 여백을
    # 가정하고 2배 높이로 그리면 뚱뚱한 초록 덩어리가 된다.
    place(blueprint, name, x, y, w, h, "tl", parent_size, Z_CONTENT)
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


def frame(blueprint, prefix, parent, w, h, weight=None, family="wood"):
    """Lay a frame around a w x h panel by butting pieces together.

    Two corners and three links per weight, mirrored into all four sides. The
    pieces were cut from one drawn panel, so the corner's arm and the link's
    cross-section are the same pixels -- joins cannot show a step, which is
    what the previous set could not manage across separate generations.

    Nothing is stretched. The run between corners is filled with whole links;
    `card()` snaps panel sizes so the division comes out even.
    """
    if weight is None:
        heavy, light = FRAME_SETS.get(family, FRAME_SETS["wood"])
        weight = heavy if min(w, h) >= 8 * UNIT else light
    size = (w, h)
    corner, link = weight["corner"], weight["link"]
    rail = weight.get("rail", link)
    art_prefix = weight["prefix"]
    tint = FRAME_TINTS.get(family, WHITE)

    def piece(name, source, x, y, pw, ph, tiling=None, **flips):
        image(blueprint, name, parent, x, y, pw, ph, size, z_order=Z_FRAME,
              texture="{}/{}_{}".format(KK, art_prefix, source), tint=tint,
              tiling=tiling,
              size=(link, rail) if tiling is not None else None)
        if flips:
            flip(blueprint, name, **flips)

    # 직선 구간은 타일 브러시 한 장이다. 조각마다 위젯을 놓으면 폭이 넓을수록
    # 개수가 폭발한다 -- 화면 폭 바의 윗변 하나가 위젯 114개까지 갔고 UMG
    # 컴파일러가 위젯마다 스택을 덤프해 빌드가 8GB에서 멈췄다. image_size를
    # 조각 크기로 두면 늘리는 게 아니라 그 크기로 반복이라 같은 픽셀이다.
    run_h, run_v = w - 2 * corner, h - 2 * corner
    piece("{}_FT".format(prefix), "Link_T", corner, 0, run_h, rail,
          tiling=unreal.SlateBrushTileType.HORIZONTAL)
    piece("{}_FB".format(prefix), "Link_B", corner, h - rail, run_h, rail,
          tiling=unreal.SlateBrushTileType.HORIZONTAL)
    piece("{}_FL".format(prefix), "Link_S", 0, corner, rail, run_v,
          tiling=unreal.SlateBrushTileType.VERTICAL)
    piece("{}_FR".format(prefix), "Link_S", w - rail, corner, rail, run_v,
          tiling=unreal.SlateBrushTileType.VERTICAL, horizontal=True)

    piece("{}_FCTL".format(prefix), "Corner_T", 0, 0, corner, corner)
    piece("{}_FCTR".format(prefix), "Corner_T", w - corner, 0, corner, corner,
          horizontal=True)
    piece("{}_FCBL".format(prefix), "Corner_B", 0, h - corner, corner, corner)
    piece("{}_FCBR".format(prefix), "Corner_B", w - corner, h - corner,
          corner, corner, horizontal=True)
    return weight["band"]


def button_frame(blueprint, prefix, parent, w, h, pressed=False):
    """The raised button plate, assembled from one corner and two links.

    A button needs a different silhouette from an information panel -- the
    end-turn control used the panel frame and so nothing about its shape said
    it could be pressed. This set has a lit top edge and a darker skirt, and a
    pressed variant whose outline matches to the pixel.
    """
    state = "Down" if pressed else "Up"
    size = (w, h)
    corner, link = UNIT, UNIT / 2.0

    def piece(tag, source, x, y, pw, ph, tiling=None, **flips):
        nm = "{}_B{}".format(prefix, tag)
        image(blueprint, nm, parent, x, y, pw, ph, size, z_order=Z_FRAME,
              texture="{}/KK_Button_{}_{}".format(KK, source, state),
              tint=WHITE, tiling=tiling,
              size=(link, link) if tiling is not None else None)
        if flips:
            flip(blueprint, nm, **flips)

    # 직선 구간은 타일 브러시 한 장 (frame()과 같은 이유).
    run_h, run_v = w - 2 * corner, h - 2 * corner
    piece("T", "Link_H", corner, 0, run_h, link,
          tiling=unreal.SlateBrushTileType.HORIZONTAL)
    piece("B", "Link_H", corner, h - link, run_h, link,
          tiling=unreal.SlateBrushTileType.HORIZONTAL, vertical=True)
    piece("L", "Link_V", 0, corner, link, run_v,
          tiling=unreal.SlateBrushTileType.VERTICAL)
    piece("R", "Link_V", w - link, corner, link, run_v,
          tiling=unreal.SlateBrushTileType.VERTICAL, horizontal=True)
    piece("CTL", "Corner", 0, 0, corner, corner)
    piece("CTR", "Corner", w - corner, 0, corner, corner, horizontal=True)
    piece("CBL", "Corner", 0, h - corner, corner, corner, vertical=True)
    piece("CBR", "Corner", w - corner, h - corner, corner, corner,
          horizontal=True, vertical=True)


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

    # 계약 이름은 래퍼가 갖는다.
    #
    # 그림자를 판의 형제로 깔았더니, 런타임이 판을 감출 때 그림자만 남아
    # 화면에 회색 유령 사각형이 떴다. 턴 순서 빈 칸 자리에서 정확히 그랬다.
    # 감춰야 할 것들을 한 껍데기에 넣어야 그런 잔재가 안 생긴다.
    add(blueprint, "CanvasPanel", name, parent)
    place(blueprint, name, x, y, w, h, anchor, parent_size)
    size = (w, h)

    shadow = "{}_Shadow".format(name)
    paint(add(blueprint, "Image", shadow, name), tint=CARD_SHADOW)
    place(blueprint, shadow, -3, 5, w + 6, h + 4, "tl", size, Z_SHADOW)

    plate = "{}_Plate".format(name)
    border = add(blueprint, "Border", plate, name)
    border.set_editor_property("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
    paint(border, tint=INK)
    place(blueprint, plate, 0, 0, w, h, "tl", size, Z_FILL)

    inner = "{}_Canvas".format(name)
    add(blueprint, "CanvasPanel", inner, plate)
    # Tiled at its own size rather than scaled to the card: shrinking a surface
    # to fit is what made the previous art line read as flat black.
    fill, tint = SURFACES.get(role, SURFACES["command"])
    image(blueprint, "{}_Fill".format(name), inner, 0, 0, w, h, size,
          z_order=Z_FILL, texture="{}/KK_Fill_{}".format(KK, fill), tint=tint,
          size=(FILL_TILE, FILL_TILE),
          tiling=unreal.SlateBrushTileType.BOTH)
    # 타일 원단은 그 자체로는 평평하다. 판 전체에 걸리는 조명은 코드가 얹는다.
    # 다만 밝은 판에서는 같은 램프가 훨씬 세게 먹는다 -- 양피지가 시안 157
    # 대비 111까지 눌렸다. 밝은 역할에는 램프를 절반만 건다.
    shade_tint = (unreal.LinearColor(1.0, 1.0, 1.0, 0.35)
                  if role == "info" else WHITE)
    image(blueprint, "{}_Shade".format(name), inner, 0, 0, w, h, size,
          z_order=Z_FILL + 1, texture=SHADE, tint=shade_tint, size=SHADE_SIZE)

    # 프레임이 면을 무는 자리에 두 줄. 위는 빛을 받아 밝고 아래는 그늘진다.
    # 램프 한 장만으로는 면이 평평하게 읽힌다.
    band = weight["band"]
    image(blueprint, "{}_Rim".format(name), inner, band, band,
          w - 2 * band, 2.0, size, z_order=Z_FILL + 2, tint=RIM_LIGHT)
    image(blueprint, "{}_Seat".format(name), inner, band, h - band - 2.0,
          w - 2 * band, 2.0, size, z_order=Z_FILL + 2, tint=RIM_DARK)
    return inner, size


def ring(blueprint, name, parent, x, y, extent, tint, parent_size,
         enemy=False):
    """A portrait bezel, returning where the portrait goes inside it."""
    # 베젤을 초상화 위에 올린다.
    #
    # 게임플레이가 주는 초상화는 사각 스프라이트라 원형 구멍에 그대로 넣으면
    # 네 귀퉁이가 삐져나온다. 구멍에 내접한 사각형의 귀퉁이는 중심에서 124px,
    # 베젤은 88px부터 128px까지 불투명하니 위에 얹기만 하면 가려진다.
    inner = extent * RING_HOLE_RATIO
    offset = (extent - inner) / 2.0
    image(blueprint, name + "_Ring", parent, x, y, extent, extent, parent_size,
          z_order=Z_CONTENT + 2,
          texture="{}/KK_Ring_{}".format(KK, "Enemy" if enemy else "Portrait"),
          tint=tint)
    return x + offset, y + offset, inner


def tag(blueprint, name, parent, x, y, extent, glyph, parent_size):
    """A small pictogram that sits beside a number.

    Drawn at 2U in the art and used at 16~24px here, so the shapes were kept to
    one solid silhouette -- anything finer turns to mush at this size.
    """
    return image(blueprint, name, parent, x, y, extent, extent, parent_size,
                 texture="{}/KK_Tag_{}".format(KK, glyph), tint=WHITE)


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

    def piece(tag, source, x, y, pw, ph, tiling=None, **flips):
        nm = "{}_{}".format(name, tag)
        image(blueprint, nm, root, x, y, pw, ph, size,
              texture="{}/KK_Select_{}".format(KK, source), tint=WHITE,
              tiling=tiling, size=(l, l) if tiling is not None else None)
        if flips:
            flip(blueprint, nm, **flips)

    # 직선 구간은 타일 브러시 한 장. 조각마다 위젯을 놓으면 폭이 넓을수록
    # 개수가 폭발한다 -- 화면 폭 바 하나가 위젯 114개까지 갔다.
    piece("T", "Link_H", c, 0, gw - 2 * c, l,
          tiling=unreal.SlateBrushTileType.HORIZONTAL)
    piece("B", "Link_H", c, gh - l, gw - 2 * c, l,
          tiling=unreal.SlateBrushTileType.HORIZONTAL, vertical=True)
    piece("L", "Link_V", 0, c, l, gh - 2 * c,
          tiling=unreal.SlateBrushTileType.VERTICAL)
    piece("R", "Link_V", gw - l, c, l, gh - 2 * c,
          tiling=unreal.SlateBrushTileType.VERTICAL, horizontal=True)
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
    frame(blueprint, "RoundPanel", body, *extent, family="wood")


def objective_panel(blueprint, root, x, y, w=340.0, h=60.0, anchor="tr",
                    size=16, icon=True):
    body, extent = card(blueprint, "ObjectivePanel", root, x, y, w, h, anchor,
                        role="info")
    text_x = 8.0
    if icon:
        # 깃발 원화는 캔버스의 절반만 그림이다. 시안의 46px 깃발을 맞추려면
        # 위젯을 그만큼 키워야 한다.
        glyph = min(64.0, h - 6.0)
        image(blueprint, "ObjectiveIcon", body, 14, (h - glyph) / 2.0,
              glyph, glyph, extent,
              texture=KK + "/KK_Icon_Objective", tint=WHITE)
        text_x = 14 + glyph + 8
    label(blueprint, "ObjectiveText", body, text_x, (h - size - 10) / 2.0,
          w - text_x - 12, size + 10, "목표", size, TEXT_ON_LIGHT, "center",
          extent)
    frame(blueprint, "ObjectivePanel", body, *extent, family="wood")


def turn_row(blueprint, root, x, y, token=96.0, gap=12.0, anchor="tc",
             count=6, names=True, framed=True):
    """The turn order, as a row of framed portrait tokens."""
    for index in range(count):
        name = "TurnToken_{}".format(index)
        body, size = card(blueprint, name, root, x + index * (token + gap), y,
                          token, token, anchor, role="turn")
        # 이름표가 있으면 링을 줄여 프레임 안쪽에 이름 자리를 만든다. 프레임은
        # 내용보다 위 층이라, 자리를 안 비우면 이름 아래쪽이 잘린다.
        portrait = token * (0.62 if names else 0.90)
        px, py, pe = ring(blueprint, "TurnPortrait_{}".format(index), body,
                          (token - portrait) / 2.0, token * 0.05, portrait,
                          WHITE, size)
        face = TURN_PORTRAITS[index] if index < len(TURN_PORTRAITS) else None
        image(blueprint, "TurnPortrait_{}".format(index), body,
              px, py, pe, pe, size, texture=face,
              tint=WHITE if face else EMPTY_SOCKET)
        if names:
            label(blueprint, "TurnName_{}".format(index), body,
                  4, token * 0.05 + portrait + 2, token - 8, 16, "이름", 11,
                  TEXT_DIM, "center", size)
        if framed:
            frame(blueprint, name, body, *size, family="metal")
        marker(blueprint, "TurnCurrent_{}".format(index), body,
               size[0], size[1], size)

        # 칸 사이에 진행 방향 화살표. 줄이 늘어서 있다는 것만으로는 왼쪽에서
        # 오른쪽인지 반대인지 읽히지 않는다. 토큰과 같은 앵커로 놓아야 4:3에서
        # 줄과 화살표가 따로 놀지 않는다.
        if index + 1 < count:
            arrow = min(gap * 1.7, token * 0.30)
            arrow_name = "TurnArrow_{}".format(index)
            paint(add(blueprint, "Image", arrow_name, root),
                  texture=KK + "/KK_Turn_Arrow", tint=GOLD)
            place(blueprint, arrow_name,
                  x + index * (token + gap) + token + (gap - arrow) / 2.0,
                  y + (token - arrow) / 2.0, arrow, arrow, anchor, None,
                  Z_CONTENT)


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
    plate, size = card(blueprint, name, root, x, y, w, h, anchor,
                       role="party_lead" if index == 0 else "party")

    # 내용은 전부 이 캔버스 안에 넣는다. 빈 칸일 때 런타임이 이 하나만
    # 감추면 장식까지 같이 사라진다 -- 위젯을 하나씩 감추면 계약에 없는
    # 초상화 테와 꺼진 보석 바탕이 남아 빈 칸에 유령 고리와 유령 보석이 뜬다.
    body = "PartyContent_{}".format(index)
    add(blueprint, "CanvasPanel", body, plate)
    place(blueprint, body, 0, 0, size[0], size[1], "tl", size, Z_CONTENT)
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
                  texture=KK + "/KK_Gem_Blue_Off", tint=WHITE)
        for pip in range(4):
            image(blueprint, "PartyAPPip_{}_{}".format(index, pip), body,
                  gx + pip * pitch, gy, extent, extent, size,
                  texture=KK + "/KK_Gem_Blue_On", tint=WHITE)

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
        # 상태이상 아이콘은 글자와 짝이라 런타임이 같이 켜고 끈다. 늘 켜 두면
        # 아무 상태도 없는 카드에 해골이 상시로 붙는다.
        tag(blueprint, "PartyStatusIcon_{}".format(index), body,
            left + run - status_w, h * 0.10, max(14.0, min(22.0, h * 0.15)),
            "Poison", size)
        label(blueprint, "PartyStatus_{}".format(index), body,
              left + run - status_w + max(14.0, min(22.0, h * 0.15)) + 4,
              h * 0.09, status_w - 20, line, "", info_size, GOLD, "left", size)

        # 하트 - 바 - 숫자를 붙여서 한 덩어리로 읽히게 한다. 숫자를 카드
        # 오른쪽 끝에 붙여 두면 바와 사이가 텅 비어 둘이 딴 정보로 보인다.
        # 하트 표는 뺐다. 시안 아군 카드에는 없다(붉은 픽셀 22개 = 사실상
        # 0). 초록 바 자체가 이미 HP로 읽히는데 앞에 붉은 점을 붙이면
        # 색이 하나 늘고 바가 그만큼 짧아진다.
        hp_text = min(run * 0.26, 88.0)
        bar_w = run - hp_text - 14
        bar(blueprint, "PartyHPBar_{}".format(index), body,
            left, h * 0.45, bar_w, max(14.0, h * 0.13), HP_GREEN, size)
        label(blueprint, "PartyHPText_{}".format(index), body,
              left + bar_w + 12, h * 0.40, hp_text, line,
              "0/0", info_size, HP_TEXT, "left", size)

        # 보석 줄도 같은 이유로 숫자를 바로 옆에 붙인다.
        gem = max(22.0, min(30.0, h * 0.24))
        # 젬 사이를 더 벌린다. 붙여 놓으면 넷이 한 덩어리로 뭉쳐 개수가
        # 안 읽힌다 -- 시안은 젬 하나 폭의 3분의 1을 띄운다.
        # 시안은 보석 줄 윗변이 행 높이의 61%에 온다. 0.70 이면 8%p 낮게
        # 앉아 이름·바와의 간격이 벌어지고 카드 아래가 비어 보인다.
        gems(left, h * 0.61, gem, gem * 1.35)
        label(blueprint, "PartyAPText_{}".format(index), body,
              left + gem * 1.35 * 4 + 10, h * 0.61, 64, line, "0/0", info_size,
              AP_TEXT, "left", size)

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

    # 프레임과 선택 표시는 판에 직접 얹는다. 내용 캔버스에 넣으면 빈 칸에서
    # 테두리까지 같이 사라져 카드가 통째로 없어진 것처럼 보인다.
    frame(blueprint, name, plate, *size, family="wood")
    marker(blueprint, "PartySelected_{}".format(index), plate,
           size[0], size[1], size)


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
        # 아이콘이 카드의 중심이다. 위에 붙여 두고 글자를 아래에 흩으면
        # 가운데가 비어 카드가 세 토막으로 읽힌다.
        #
        # 글자 블록 높이를 먼저 재고, 남는 세로를 아이콘이 갖는다.
        text_block = 24 + 20 + 20 + 26
        glyph = min(w - 14, h - text_block - 28)
        # 아이콘 원화는 캔버스의 위아래 12.5%가 여백이다. 시안의 "카드 위에서
        # 28px" 을 맞추려면 위젯을 그만큼 더 올려 놓아야 한다.
        icon_top = max(6.0, 28.0 - glyph * 0.125)
        image(blueprint, "CommandIcon_{}".format(index), body,
              (w - glyph) / 2.0, icon_top, glyph, glyph, size,
              texture=COMMAND_ICONS[index], tint=WHITE)
        text_top = h - text_block - 8
        label(blueprint, "CommandName_{}".format(index), body,
              8, text_top, w - 16, 24, "이름", 15, TEXT_COLOR, "center", size)
        label(blueprint, "CommandDamage_{}".format(index), body,
              8, text_top + 26, w - 16, 20, "0~0", 13, DAMAGE_TEXT, "center",
              size)
        tag(blueprint, "CommandCooldownIcon_{}".format(index), body,
            (w - 96) / 2.0, text_top + 46, 18, "Cooldown", size)
        label(blueprint, "CommandCooldown_{}".format(index), body,
              (w - 96) / 2.0 + 22, text_top + 46, 78, 20, "", 12,
              COOLDOWN_TEXT, "left", size)
        # 배지는 밝은 원에 진한 숫자다. 반대로 하면 밝은 카드 위에서 숫자가
        # 배지에 먹힌다 -- 대비 방향이 뒤집혀 있었다.
        gem = 40.0
        image(blueprint, "CommandCostGem_{}".format(index), body,
              w - gem - 12, 12, gem, gem, size,
              texture=KK + "/KK_Badge_Round", tint=BADGE_FACE)
        label(blueprint, "CommandCost_{}".format(index), body,
              w - gem - 12, 12 + (gem - 26) / 2.0, gem, 26, "0", 19,
              BADGE_TEXT, "center", size, bold=True)
        # 배지 숫자는 아이콘에 붙어 있어 훑을 때 안 걸린다. 카드 아래에
        # "AP n"을 한 번 더 적는다. 고를 때 보는 건 이쪽이다.
        label(blueprint, "CommandCostLine_{}".format(index), body,
              8, text_top + 68, w - 16, 24, "", 15, AP_TEXT, "center", size,
              bold=True)

    frame(blueprint, name, body, *size, family="metal")

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
          size, tint=unreal.LinearColor(0.0, 0.0, 0.0, 0.22))
    image(blueprint, "CommandDisabledMat_{}".format(index), stack, 0, 0, w, h,
          size, texture=KK + "/KK_Veil_Disabled",
          tint=unreal.LinearColor(1.0, 1.0, 1.0, 0.35),
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
              texture=KK + "/KK_Face_Eagle",
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
        # 자리를 높이의 비율로 잡는다. 고정 오프셋으로 박아 두고 패널만
        # 키웠더니 위쪽 1/3만 쓰고 아래는 빈 벌판이 됐다. 아군 띠에서 이미
        # 겪은 것과 같은 실수다.
        pad = max(12.0, h * 0.09)
        portrait = min(h * 0.46, w * 0.28)
        px, py, pe = ring(blueprint, "EnemyPortrait", body, pad, pad,
                          portrait, enemy_ring, size, enemy=True)
        image(blueprint, "EnemyPortrait", body, px, py, pe, pe, size,
              texture=KK + "/KK_Face_Eagle",
              tint=unreal.LinearColor(1, 1, 1, 1))

        left = pad + portrait + pad * 0.8
        run = w - left - pad
        name_size = int(max(13, min(20, h * 0.13)))
        info_size = int(max(11, min(16, h * 0.10)))
        line = h * 0.16

        label(blueprint, "EnemyName", body, left, h * 0.09, run, line, "적",
              name_size, TEXT_COLOR, "left", size, bold=True)

        hp_text = min(run * 0.30, 92.0)
        bar(blueprint, "EnemyHPBar", body, left, h * 0.32,
            run - hp_text - 10, max(12.0, h * 0.09), HP_RED, size)
        label(blueprint, "EnemyHPText", body, left + run - hp_text,
              h * 0.26, hp_text, line, "0/0", info_size, TEXT_COLOR, "right",
              size)

        icon = max(16.0, min(22.0, h * 0.11))
        tag(blueprint, "EnemyDefenseIcon", body, left, h * 0.50, icon,
            "Defense", size)
        label(blueprint, "EnemyDefense", body, left + icon + 6, h * 0.49,
              run - icon - 6, line, "", info_size, TEXT_DIM, "left", size)

        tag(blueprint, "EnemyForecastIcon", body, left, h * 0.68, icon,
            "Damage", size)
        label(blueprint, "EnemyForecast", body, left + icon + 6, h * 0.67,
              run - icon - 6, line, "", info_size, DAMAGE_TEXT, "left", size)

        label(blueprint, "EnemyStatus", body, pad, h - line - pad * 0.6,
              w - 2 * pad, line, "", info_size, GOLD, "left", size)

    frame(blueprint, "EnemyPanel", body, *size, family="metal")


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
    # 정보 판이 아니라 누르는 것이다. 볼록한 판을 둘러야 형태만 보고
    # 눌러도 되는 것인지 알 수 있다.
    button_frame(blueprint, "EndTurn", plate, *extent)


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
