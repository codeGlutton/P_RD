"""Verify the imported frame and the six committed combat skill cards."""

import unreal


HUD_ASSET_PATH = "/Game/UI/CombatLayouts/WBP_CombatHUD04"
HUD_OBJECT_PREFIX = f"{HUD_ASSET_PATH}.WBP_CombatHUD04:WidgetTree."
FRAME_ASSET_PATH = "/Game/UI/Art/Combat/T_SkillCard_Frame_Combat"
EXPECTED_TEXTURE_SIZE = (1217, 1292)
EXPECTED_CARD_SIZE = unreal.Vector2D(200.0, 228.0)
DESIGN_SIZE = unreal.Vector2D(1920.0, 1080.0)
VIEWPORT_SIZE = unreal.Vector2D(1600.0, 900.0)
FRAME_ALPHA_BOUNDS = (162.0, 97.0, 1045.0, 1200.0)
EPSILON = 0.1
CONTENT_EPSILON = 0.6
EXPECTED_CONTENT_LAYOUT = {
    "CommandIcon": (57.0, 70.4, 86.1, 86.1, None),
    "CommandName": (100.5, 52.1, 144.2, 29.1, 21),
    "CommandDamage": (99.5, 175.9, 144.2, 29.1, 18),
    "CommandCooldownBadge": (158.4, 120.2, 49.6, 51.2, None),
    "CommandCooldown": (183.2, 145.7, 49.8, 54.9, 23),
    "CommandCostBadge": (161.8, 62.9, 42.9, 51.7, None),
    "CommandCost": (183.2, 88.7, 49.8, 54.9, 23),
}


def widget(name, expected_type=None):
    result = unreal.load_object(None, HUD_OBJECT_PREFIX + name)
    if result is None:
        raise RuntimeError(f"Missing widget {name}")
    if expected_type is not None and not isinstance(result, expected_type):
        raise RuntimeError(
            f"{name} is {result.get_class().get_name()}, "
            f"expected {expected_type.__name__}"
        )
    return result


def rect_for_canvas_slot(slot, parent_size):
    anchors = slot.get_anchors()
    if (
        abs(anchors.minimum.x - anchors.maximum.x) > EPSILON
        or abs(anchors.minimum.y - anchors.maximum.y) > EPSILON
    ):
        raise RuntimeError("Stretch anchors are not supported by this verifier")
    position = slot.get_position()
    size = slot.get_size()
    alignment = slot.get_alignment()
    anchor_position = unreal.Vector2D(
        parent_size.x * anchors.minimum.x,
        parent_size.y * anchors.minimum.y,
    )
    left = anchor_position.x + position.x - alignment.x * size.x
    top = anchor_position.y + position.y - alignment.y * size.y
    return (left, top, left + size.x, top + size.y)


def expected_plate_rect():
    source_width, source_height = EXPECTED_TEXTURE_SIZE
    alpha_left, alpha_top, alpha_right, alpha_bottom = FRAME_ALPHA_BOUNDS
    alpha_width = alpha_right - alpha_left
    alpha_height = alpha_bottom - alpha_top
    plate_width = EXPECTED_CARD_SIZE.x * source_width / alpha_width
    plate_height = EXPECTED_CARD_SIZE.y * source_height / alpha_height
    plate_left = -EXPECTED_CARD_SIZE.x * alpha_left / alpha_width
    plate_top = -EXPECTED_CARD_SIZE.y * alpha_top / alpha_height
    return (plate_left, plate_top, plate_width, plate_height)


texture = unreal.load_asset(FRAME_ASSET_PATH)
if not isinstance(texture, unreal.Texture2D):
    raise RuntimeError(f"Missing frame texture {FRAME_ASSET_PATH}")
texture_size = (
    texture.blueprint_get_size_x(),
    texture.blueprint_get_size_y(),
)
if texture_size != EXPECTED_TEXTURE_SIZE:
    raise RuntimeError(
        f"Frame texture is {texture_size}, expected {EXPECTED_TEXTURE_SIZE}"
    )
if texture.get_editor_property(
    "compression_settings"
) != unreal.TextureCompressionSettings.TC_EDITOR_ICON:
    raise RuntimeError("Frame texture must use TC_EDITOR_ICON compression")
if texture.get_editor_property(
    "lod_group"
) != unreal.TextureGroup.TEXTUREGROUP_UI:
    raise RuntimeError("Frame texture must use TEXTUREGROUP_UI")
if texture.get_editor_property(
    "mip_gen_settings"
) != unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS:
    raise RuntimeError("Frame texture must not generate mipmaps")
if not texture.get_editor_property("srgb"):
    raise RuntimeError("Frame texture must use sRGB")
if not texture.get_editor_property("never_stream"):
    raise RuntimeError("Frame texture must stay resident")

hud = unreal.load_asset(HUD_ASSET_PATH)
if hud is None:
    raise RuntimeError(f"Missing HUD asset {HUD_ASSET_PATH}")

cards = []
expected_plate = expected_plate_rect()
for index in range(6):
    card = widget(f"CommandCard_{index}", unreal.CanvasPanel)
    if not isinstance(card.slot, unreal.CanvasPanelSlot):
        raise RuntimeError(f"{card.get_name()} must use CanvasPanelSlot")
    size = card.slot.get_size()
    if (
        abs(size.x - EXPECTED_CARD_SIZE.x) > EPSILON
        or abs(size.y - EXPECTED_CARD_SIZE.y) > EPSILON
    ):
        raise RuntimeError(
            f"{card.get_name()} is ({size.x:.1f},{size.y:.1f}), "
            f"expected ({EXPECTED_CARD_SIZE.x:.1f},{EXPECTED_CARD_SIZE.y:.1f})"
        )

    card_rect = rect_for_canvas_slot(card.slot, DESIGN_SIZE)
    if (
        card_rect[0] < -EPSILON
        or card_rect[1] < -EPSILON
        or card_rect[2] > DESIGN_SIZE.x + EPSILON
        or card_rect[3] > DESIGN_SIZE.y + EPSILON
    ):
        raise RuntimeError(
            f"{card.get_name()} is outside the 1920x1080 design canvas: "
            f"{card_rect}"
        )
    cards.append((index, card_rect))

    plate = widget(f"CommandPlate_{index}", unreal.Image)
    if not isinstance(plate.slot, unreal.CanvasPanelSlot):
        raise RuntimeError(f"{plate.get_name()} must use CanvasPanelSlot")
    plate_position = plate.slot.get_position()
    plate_size = plate.slot.get_size()
    actual_plate = (
        plate_position.x,
        plate_position.y,
        plate_size.x,
        plate_size.y,
    )
    if any(
        abs(actual - expected) > EPSILON
        for actual, expected in zip(actual_plate, expected_plate)
    ):
        raise RuntimeError(
            f"{plate.get_name()} overscan is {actual_plate}, "
            f"expected {expected_plate}"
        )
    brush = plate.get_editor_property("brush")
    resource = brush.get_editor_property("resource_object")
    if resource != texture:
        actual_path = (
            resource.get_path_name() if resource is not None else "None"
        )
        raise RuntimeError(
            f"{plate.get_name()} uses {actual_path}, "
            f"expected {FRAME_ASSET_PATH}"
        )

    for prefix, expected in EXPECTED_CONTENT_LAYOUT.items():
        content = widget(f"{prefix}_{index}")
        if not isinstance(content.slot, unreal.CanvasPanelSlot):
            raise RuntimeError(f"{content.get_name()} must use CanvasPanelSlot")
        position = content.slot.get_position()
        size = content.slot.get_size()
        actual = (position.x, position.y, size.x, size.y)
        if any(
            abs(actual_value - expected_value) > CONTENT_EPSILON
            for actual_value, expected_value in zip(actual, expected[:4])
        ):
            raise RuntimeError(
                f"{content.get_name()} layout is {actual}, "
                f"expected approximately {expected[:4]}"
            )
        expected_font_size = expected[4]
        if expected_font_size is not None:
            if not isinstance(content, unreal.TextBlock):
                raise RuntimeError(
                    f"{content.get_name()} must be a TextBlock"
                )
            font_size = content.get_editor_property(
                "font"
            ).get_editor_property("size")
            if font_size != expected_font_size:
                raise RuntimeError(
                    f"{content.get_name()} font is {font_size}, "
                    f"expected {expected_font_size}"
                )

    viewport_rect = tuple(
        value * VIEWPORT_SIZE.x / DESIGN_SIZE.x
        for value in card_rect
    )
    if (
        viewport_rect[0] < -EPSILON
        or viewport_rect[1] < -EPSILON
        or viewport_rect[2] > VIEWPORT_SIZE.x + EPSILON
        or viewport_rect[3] > VIEWPORT_SIZE.y + EPSILON
    ):
        raise RuntimeError(
            f"{card.get_name()} is outside 1600x900: {viewport_rect}"
        )

for left_index in range(len(cards)):
    left = cards[left_index]
    for right_index in range(left_index + 1, len(cards)):
        right = cards[right_index]
        overlap_x = min(left[1][2], right[1][2]) - max(
            left[1][0], right[1][0]
        )
        overlap_y = min(left[1][3], right[1][3]) - max(
            left[1][1], right[1][1]
        )
        if overlap_x > EPSILON and overlap_y > EPSILON:
            raise RuntimeError(
                f"CommandCard_{left[0]} overlaps CommandCard_{right[0]} "
                f"by ({overlap_x:.1f},{overlap_y:.1f})"
            )

unreal.log(
    "RD_SKILL_CARD_FRAME_VERIFY "
    "texture=1217x1292 cards=6 card_size=200x228 "
    "visible_ratio=0.877193 overscan_bbox_fitted=6 "
    "inside_1600x900=6 overlaps=0 plates_using_frame=6 "
    "content_layout_scaled=42 fonts_scaled=24"
)
