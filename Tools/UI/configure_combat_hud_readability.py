"""Retune the committed WBP_CombatHUD04 readability layout.

The crop wrappers and round-marker widgets are part of the widget blueprint.
This script intentionally does not create WidgetTree objects: UE 5.7 requires
editor-only GUID bookkeeping when adding them. It only reapplies safe,
idempotent layout/style settings to the committed structure.
"""

import unreal


ASSET_PATH = "/Game/UI/CombatLayouts/WBP_CombatHUD04"
OBJECT_PREFIX = f"{ASSET_PATH}.WBP_CombatHUD04:WidgetTree."
FRAME_ASSET_PATH = "/Game/UI/Art/Combat/T_SkillCard_Frame_Combat"
COMMAND_CARD_SIZE = unreal.Vector2D(200.0, 228.0)
DESIGN_SIZE = unreal.Vector2D(1920.0, 1080.0)
FRAME_SOURCE_SIZE = (1217.0, 1292.0)
FRAME_ALPHA_BOUNDS = (162.0, 97.0, 1045.0, 1200.0)


def widget(name, expected_type=None):
    result = unreal.load_object(None, OBJECT_PREFIX + name)
    if result is None:
        raise RuntimeError(
            f"Missing committed widget {name}; add structural widgets through "
            "the UMG editor/helper before running this retune script."
        )
    if expected_type is not None and not isinstance(result, expected_type):
        raise RuntimeError(
            f"{name} is {result.get_class().get_name()}, "
            f"expected {expected_type.__name__}"
        )
    result.modify()
    return result


def optional_widget(name, expected_type=None):
    result = unreal.load_object(None, OBJECT_PREFIX + name)
    if result is None:
        return None
    if expected_type is not None and not isinstance(result, expected_type):
        raise RuntimeError(
            f"{name} is {result.get_class().get_name()}, "
            f"expected {expected_type.__name__}"
        )
    result.modify()
    return result


def canvas_layout(target, position, size, z_order):
    slot = target.slot
    if not isinstance(slot, unreal.CanvasPanelSlot):
        raise RuntimeError(
            f"{target.get_name()} must use CanvasPanelSlot, "
            f"got {slot.get_class().get_name()}"
        )
    slot.modify()
    slot.set_position(unreal.Vector2D(*position))
    slot.set_size(unreal.Vector2D(*size))
    slot.set_z_order(z_order)


def canvas_bounds(target, position, size, z_order):
    """Apply literal top-left bounds, including widgets authored with pivots."""

    slot = target.slot
    if not isinstance(slot, unreal.CanvasPanelSlot):
        raise RuntimeError(
            f"{target.get_name()} must use CanvasPanelSlot, "
            f"got {slot.get_class().get_name()}"
        )
    slot.modify()
    slot.set_alignment(unreal.Vector2D(0.0, 0.0))
    slot.set_editor_property("auto_size", False)
    slot.set_position(unreal.Vector2D(*position))
    slot.set_size(unreal.Vector2D(*size))
    slot.set_z_order(z_order)


def set_font_size(target, size):
    font = target.get_editor_property("font")
    font.set_editor_property("size", size)
    target.set_editor_property("font", font)


def command_plate_bounds():
    """Overscan the source canvas so its non-transparent bbox fills the card."""

    source_width, source_height = FRAME_SOURCE_SIZE
    alpha_left, alpha_top, alpha_right, alpha_bottom = FRAME_ALPHA_BOUNDS
    alpha_width = alpha_right - alpha_left
    alpha_height = alpha_bottom - alpha_top
    plate_width = COMMAND_CARD_SIZE.x * source_width / alpha_width
    plate_height = COMMAND_CARD_SIZE.y * source_height / alpha_height
    plate_left = -COMMAND_CARD_SIZE.x * alpha_left / alpha_width
    plate_top = -COMMAND_CARD_SIZE.y * alpha_top / alpha_height
    return (
        unreal.Vector2D(plate_left, plate_top),
        unreal.Vector2D(plate_width, plate_height),
    )


def retune_mercenary_panel():
    """Align live content to the simplified 1672x941 mercenary-tab base."""

    # The source base is 1672x941, which is the same 16:9 design as the WBP.
    # Work in source-image coordinates and convert to the 1920x1080 authored
    # canvas with one shared factor.
    design_scale = 1920.0 / 1672.0

    def authored(rect):
        return tuple(float(value) * design_scale for value in rect)

    board = widget("MercenaryBoard", unreal.CanvasPanel)
    canvas_bounds(board, (0.0, 0.0), (1920.0, 1080.0), 1)

    # The old modal scrim is a full-screen translucent black Border.  The new
    # roster artwork is also a full-screen child of MercenaryPanel, so leaving
    # the scrim visible paints black over the generated shell while the live
    # card contents (which sit in MercenaryBoard) remain bright.  The shell has
    # its own opaque board and transparent exterior; it no longer needs this
    # legacy dim layer.
    widget("MercenaryScrim", unreal.Border).set_visibility(
        unreal.SlateVisibility.COLLAPSED
    )

    title = widget("MercenaryTitleText", unreal.TextBlock)
    title_rect = authored((600.0, 48.0, 472.0, 76.0))
    canvas_bounds(title, title_rect[:2], title_rect[2:], 4)
    title.set_editor_property("justification", unreal.TextJustify.CENTER)
    set_font_size(title, 46)

    widget("MercenarySubtitleText", unreal.TextBlock).set_visibility(
        unreal.SlateVisibility.COLLAPSED
    )
    close_text = widget("MercenaryCloseText", unreal.TextBlock)
    close_text.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
    close_text_rect = authored((1580.0, 48.0, 58.0, 58.0))
    canvas_bounds(close_text, close_text_rect[:2], close_text_rect[2:], 5)
    close_text.set_editor_property("justification", unreal.TextJustify.CENTER)
    set_font_size(close_text, 42)

    gold_label = widget("MercenaryGoldLabel", unreal.TextBlock)
    label_rect = authored((36.0, 50.0, 90.0, 52.0))
    canvas_bounds(gold_label, label_rect[:2], label_rect[2:], 4)
    gold_label.set_editor_property("justification", unreal.TextJustify.CENTER)
    set_font_size(gold_label, 24)

    gold_text = widget("MercenaryGoldText", unreal.TextBlock)
    gold_rect = authored((128.0, 46.0, 150.0, 60.0))
    canvas_bounds(gold_text, gold_rect[:2], gold_rect[2:], 4)
    gold_text.set_editor_property("justification", unreal.TextJustify.CENTER)
    set_font_size(gold_text, 36)

    close_button = widget("MercenaryCloseButton", unreal.Button)
    close_rect = authored((1568.0, 36.0, 82.0, 82.0))
    canvas_bounds(close_button, close_rect[:2], close_rect[2:], 6)

    normal_card = unreal.load_asset(
        "/Game/UI/Art/Combat/T_MB_MercenaryCard_Normal"
    )
    selected_card = unreal.load_asset(
        "/Game/UI/Art/Combat/T_MB_MercenaryCard_Selected"
    )
    if not isinstance(normal_card, unreal.Texture2D):
        raise RuntimeError("Missing T_MB_MercenaryCard_Normal")
    if not isinstance(selected_card, unreal.Texture2D):
        raise RuntimeError("Missing T_MB_MercenaryCard_Selected")

    card_screen_y = (202.0, 428.0, 654.0)
    card_size = (350.0, 190.0)
    for index, screen_y in enumerate(card_screen_y):
        scale_box = widget(f"MercenaryCardScale_{index}", unreal.ScaleBox)
        scale_box.set_editor_property("stretch", unreal.Stretch.SCALE_TO_FIT)
        scale_box.set_editor_property(
            "stretch_direction", unreal.StretchDirection.BOTH
        )
        wrapper_rect = authored((24.0, screen_y, card_size[0], card_size[1]))
        canvas_bounds(
            scale_box, wrapper_rect[:2], wrapper_rect[2:], 3
        )

        card = widget(f"PartyCard_{index}", unreal.CanvasPanel)
        card.set_clipping(unreal.WidgetClipping.CLIP_TO_BOUNDS_ALWAYS)
        plate = widget(f"PartyPlate_{index}", unreal.Image)
        canvas_bounds(plate, (0.0, 0.0), card_size, 1)
        plate.set_brush_from_texture(
            selected_card if index == 0 else normal_card, False
        )
        plate.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)

        content = widget(f"PartyContent_{index}", unreal.CanvasPanel)
        canvas_bounds(content, (0.0, 0.0), card_size, 2)

        portrait = widget(f"PartyPortrait_{index}", unreal.Image)
        canvas_bounds(portrait, (24.0, 20.0), (112.0, 150.0), 10)

        name = widget(f"PartyName_{index}", unreal.TextBlock)
        canvas_bounds(name, (145.0, 24.0), (176.0, 42.0), 15)
        name.set_editor_property("justification", unreal.TextJustify.CENTER)
        set_font_size(name, 24)

        hp_bar = widget(f"PartyHPBar_{index}", unreal.ProgressBar)
        canvas_bounds(hp_bar, (145.0, 77.0), (176.0, 30.0), 10)
        hp_text = widget(f"PartyHPText_{index}", unreal.TextBlock)
        canvas_bounds(hp_text, (145.0, 76.0), (176.0, 31.0), 15)
        hp_text.set_editor_property("justification", unreal.TextJustify.CENTER)
        set_font_size(hp_text, 18)

        ap_plate = widget(f"PartyAPPlate_{index}", unreal.Image)
        canvas_bounds(ap_plate, (145.0, 119.0), (176.0, 32.0), 10)
        ap_text = widget(f"PartyAPText_{index}", unreal.TextBlock)
        canvas_bounds(ap_text, (145.0, 119.0), (176.0, 32.0), 15)
        ap_text.set_editor_property("justification", unreal.TextJustify.CENTER)
        set_font_size(ap_text, 19)

        button = widget(f"PartyButton_{index}", unreal.Button)
        canvas_bounds(button, (0.0, 0.0), card_size, 29)

        # Slot zero is the first status at the portrait's upper-right. Extra
        # effects continue leftward and remain inside the portrait bounds.
        for status_index in range(3):
            frame_x = 296.0 - 45.0 * status_index
            frame = widget(
                f"PartyStatusFrame_{index}_{status_index}", unreal.Image
            )
            icon = widget(
                f"PartyStatusIcon_{index}_{status_index}", unreal.Image
            )
            canvas_bounds(frame, (frame_x, 146.0), (40.0, 40.0), 18)
            canvas_bounds(icon, (frame_x + 5.0, 151.0), (30.0, 30.0), 19)

    for legacy_name in (
        "MercenaryHeaderPlate",
        "MercenaryBoardPlate",
        "MercenaryBoardShadow",
        "MercenaryBoardInner",
        "MercenaryClosePlate",
    ):
        widget(legacy_name).set_visibility(unreal.SlateVisibility.COLLAPSED)

    unreal.log(
        "RD_MERCENARY_LAYOUT "
        "title_screen=(600,48,472,76) "
        "cards_screen=[(24,202),(24,428),(24,654)] "
        "card_size_screen=(350,190) "
        "legacy_scrim=collapsed"
    )


def retune_command_card(index, frame_texture):
    """Make one command card taller than wide while keeping its radial centre.

    The committed cards were authored as nested canvas groups. Resizing only
    the group leaves its fixed-size children hanging outside it, so map every
    direct child into the new local coordinate space as part of the same pass.
    """

    card = widget(f"CommandCard_{index}", unreal.CanvasPanel)
    card_slot = card.slot
    if not isinstance(card_slot, unreal.CanvasPanelSlot):
        raise RuntimeError(
            f"{card.get_name()} must use CanvasPanelSlot, "
            f"got {card_slot.get_class().get_name()}"
        )

    old_position = card_slot.get_position()
    old_size = card_slot.get_size()
    alignment = card_slot.get_alignment()
    if old_size.x <= 0.0 or old_size.y <= 0.0:
        raise RuntimeError(
            f"{card.get_name()} has invalid size "
            f"({old_size.x:.1f}, {old_size.y:.1f})"
        )

    # Canvas position is the alignment pivot. Keep the visual centre fixed so
    # the six-card radial composition does not drift as the bounds change.
    new_position = unreal.Vector2D(
        old_position.x
        + (0.5 - alignment.x) * (old_size.x - COMMAND_CARD_SIZE.x),
        old_position.y
        + (0.5 - alignment.y) * (old_size.y - COMMAND_CARD_SIZE.y),
    )
    card_slot.modify()
    card_slot.set_position(new_position)
    card_slot.set_size(COMMAND_CARD_SIZE)

    scale_x = COMMAND_CARD_SIZE.x / old_size.x
    scale_y = COMMAND_CARD_SIZE.y / old_size.y
    uniform_scale = min(scale_x, scale_y)
    fill_names = {
        f"CommandDisabled_{index}",
        f"CommandButton_{index}",
    }
    preserve_aspect_names = {
        f"CommandIcon_{index}",
        f"CommandCooldownBadge_{index}",
        f"CommandCostBadge_{index}",
    }

    for child_index in range(card.get_children_count()):
        child = card.get_child_at(child_index)
        child_slot = child.slot
        if not isinstance(child_slot, unreal.CanvasPanelSlot):
            raise RuntimeError(
                f"{child.get_name()} must use CanvasPanelSlot, "
                f"got {child_slot.get_class().get_name()}"
            )
        child_slot.modify()

        child_name = child.get_name()
        if child_name == f"CommandPlate_{index}":
            if not isinstance(child, unreal.Image):
                raise RuntimeError(
                    f"{child_name} is {child.get_class().get_name()}, "
                    "expected Image"
                )
            plate_position, plate_size = command_plate_bounds()
            child_slot.set_alignment(unreal.Vector2D(0.0, 0.0))
            child_slot.set_position(plate_position)
            child_slot.set_size(plate_size)
            child.set_brush_from_texture(frame_texture, False)
            child.set_editor_property(
                "color_and_opacity", unreal.LinearColor(1.0, 1.0, 1.0, 1.0)
            )
            continue

        if child_name in fill_names:
            child_slot.set_position(unreal.Vector2D(0.0, 0.0))
            child_slot.set_size(COMMAND_CARD_SIZE)
            continue

        child_position = child_slot.get_position()
        child_size = child_slot.get_size()
        child_alignment = child_slot.get_alignment()
        mapped_center = unreal.Vector2D(
            (
                child_position.x
                + (0.5 - child_alignment.x) * child_size.x
            )
            * scale_x,
            (
                child_position.y
                + (0.5 - child_alignment.y) * child_size.y
            )
            * scale_y,
        )
        if child_name in preserve_aspect_names:
            new_child_size = unreal.Vector2D(
                child_size.x * uniform_scale,
                child_size.y * uniform_scale,
            )
        else:
            new_child_size = unreal.Vector2D(
                child_size.x * scale_x,
                child_size.y * scale_y,
            )
        child_slot.set_position(
            unreal.Vector2D(
                mapped_center.x
                - (0.5 - child_alignment.x) * new_child_size.x,
                mapped_center.y
                - (0.5 - child_alignment.y) * new_child_size.y,
            )
        )
        child_slot.set_size(new_child_size)
        if (
            isinstance(child, unreal.TextBlock)
            and abs(uniform_scale - 1.0) > 0.0001
        ):
            old_font_size = child.get_editor_property("font").get_editor_property(
                "size"
            )
            set_font_size(
                child, max(1, int(round(old_font_size * uniform_scale)))
            )

    unreal.log(
        "RD_SKILL_CARD_RETUNE "
        f"index={index} "
        f"old_position=({old_position.x:.1f},{old_position.y:.1f}) "
        f"old_size=({old_size.x:.1f},{old_size.y:.1f}) "
        f"new_position=({new_position.x:.1f},{new_position.y:.1f}) "
        f"new_size=({COMMAND_CARD_SIZE.x:.1f},{COMMAND_CARD_SIZE.y:.1f})"
    )


def assert_command_cards_do_not_overlap():
    cards = []
    for index in range(6):
        card = widget(f"CommandCard_{index}", unreal.CanvasPanel)
        slot = card.slot
        anchors = slot.get_anchors()
        if (
            abs(anchors.minimum.x - anchors.maximum.x) > 0.0001
            or abs(anchors.minimum.y - anchors.maximum.y) > 0.0001
        ):
            raise RuntimeError(
                f"{card.get_name()} uses stretch anchors; "
                "radial overlap cannot be checked in anchor-local coordinates"
            )
        position = slot.get_position()
        size = slot.get_size()
        alignment = slot.get_alignment()
        cards.append(
            (
                index,
                anchors.minimum,
                position.x - alignment.x * size.x,
                position.y - alignment.y * size.y,
                size.x,
                size.y,
            )
        )
        absolute_left = anchors.minimum.x * DESIGN_SIZE.x + cards[-1][2]
        absolute_top = anchors.minimum.y * DESIGN_SIZE.y + cards[-1][3]
        absolute_right = absolute_left + size.x
        absolute_bottom = absolute_top + size.y
        if (
            absolute_left < -0.01
            or absolute_top < -0.01
            or absolute_right > DESIGN_SIZE.x + 0.01
            or absolute_bottom > DESIGN_SIZE.y + 0.01
        ):
            raise RuntimeError(
                f"CommandCard_{index} leaves the 1600x900 viewport "
                f"(design bounds {absolute_left:.1f},{absolute_top:.1f},"
                f"{absolute_right:.1f},{absolute_bottom:.1f})"
            )

    checked_pairs = 0
    for left_index in range(len(cards)):
        left = cards[left_index]
        for right_index in range(left_index + 1, len(cards)):
            right = cards[right_index]
            if (
                abs(left[1].x - right[1].x) > 0.0001
                or abs(left[1].y - right[1].y) > 0.0001
            ):
                raise RuntimeError(
                    f"CommandCard_{left[0]} and CommandCard_{right[0]} "
                    "use different anchors; radial overlap is ambiguous"
                )
            overlap_x = min(left[2] + left[4], right[2] + right[4]) - max(
                left[2], right[2]
            )
            overlap_y = min(left[3] + left[5], right[3] + right[5]) - max(
                left[3], right[3]
            )
            if overlap_x > 0.01 and overlap_y > 0.01:
                raise RuntimeError(
                    f"CommandCard_{left[0]} overlaps CommandCard_{right[0]} "
                    f"by ({overlap_x:.1f}, {overlap_y:.1f})"
                )
            checked_pairs += 1
    unreal.log(
        "RD_SKILL_CARD_OVERLAP_CHECK "
        f"pairs={checked_pairs} overlaps=0 inside_1600x900=6"
    )


asset = unreal.load_asset(ASSET_PATH)
if asset is None:
    raise RuntimeError(f"Missing asset {ASSET_PATH}")
asset.modify()
frame_texture = unreal.load_asset(FRAME_ASSET_PATH)
if not isinstance(frame_texture, unreal.Texture2D):
    raise RuntimeError(f"Missing combat skill-card frame {FRAME_ASSET_PATH}")

for index in range(6):
    retune_command_card(index, frame_texture)
    # Mobile readability: the action name is the primary tap target label.
    # Keep costs/damage secondary but still comfortably legible.
    set_font_size(widget(f"CommandName_{index}", unreal.TextBlock), 30)
    set_font_size(widget(f"CommandCost_{index}", unreal.TextBlock), 23)
    cost_line = optional_widget(f"CommandCostLine_{index}", unreal.TextBlock)
    if cost_line is not None:
        set_font_size(cost_line, 20)
    damage = optional_widget(f"CommandDamage_{index}", unreal.TextBlock)
    if damage is not None:
        set_font_size(damage, 19)
assert_command_cards_do_not_overlap()
if optional_widget("PartyContent_0", unreal.CanvasPanel) is not None:
    retune_mercenary_panel()
else:
    unreal.log_warning(
        "RD_COMBAT_HUD_READABILITY mercenary panel widgets absent; "
        "skipping unrelated panel retune"
    )

# The round belongs to the turn-order rail.  Hide the detached legacy plaque;
# the boundary labels below render only `R1`, `R2`, ... above the first token
# of each round.
round_panel = widget("RoundPanel", unreal.CanvasPanel)
round_panel.set_visibility(unreal.SlateVisibility.COLLAPSED)

end_turn_label = widget("EndTurnLabel", unreal.TextBlock)
set_font_size(end_turn_label, 34)

artifact_strip = widget("ArtifactStrip", unreal.CanvasPanel)
canvas_layout(artifact_strip, (18.0, -244.0), (500.0, 84.0), 2)
widget("ArtifactTrayFrame", unreal.Image).set_visibility(
    unreal.SlateVisibility.COLLAPSED
)
widget("ArtifactStripPlate", unreal.Border).set_visibility(
    unreal.SlateVisibility.COLLAPSED
)
widget("ArtifactStripLabel", unreal.TextBlock).set_visibility(
    unreal.SlateVisibility.COLLAPSED
)
for index in range(6):
    frame = widget(f"ArtifactFrame_{index}", unreal.Image)
    icon = widget(f"ArtifactIcon_{index}", unreal.Image)
    frame.set_visibility(unreal.SlateVisibility.COLLAPSED)
    canvas_layout(frame, (4.0 + 80.0 * index, 4.0), (76.0, 76.0), 1)
    canvas_layout(icon, (14.0 + 80.0 * index, 14.0), (56.0, 56.0), 2)

ap_scale = widget("TurnAPScale", unreal.ScaleBox)
canvas_layout(ap_scale, (18.0, -150.0), (720.0, 124.0), 2)

# The turn rail is assembled from one reusable token frame.  The old long
# background plate would visually merge all entries back into one bar.
widget("TurnPlate", unreal.Image).set_visibility(unreal.SlateVisibility.COLLAPSED)

# Replace the baked four-cell menu artwork with a separate frame and four
# icon-only buttons.  The button widgets stay in place so runtime bindings are
# unchanged; only the old visual plate/masks/labels are suppressed.
widget("ObjectivePlate", unreal.Image).set_visibility(
    unreal.SlateVisibility.COLLAPSED
)
canvas_bounds(widget("OptionsRailFrame", unreal.Image), (0.0, 0.0), (470.0, 173.0), 1)
for name, position, icon_size in (
    ("MenuMapIcon", (47.0, 47.0), (74.0, 74.0)),
    ("MenuMercenaryIcon", (153.5, 38.0), (63.0, 96.0)),
    ("MenuMonsterIcon", (244.0, 38.0), (90.0, 96.0)),
    ("MenuSettingsIcon", (350.0, 47.0), (74.0, 74.0)),
):
    canvas_bounds(widget(name, unreal.Image), position, icon_size, 31)
for index, position in enumerate((37.0, 138.0, 242.0, 343.0)):
    canvas_bounds(
        widget(f"MenuButton_{index}", unreal.Button),
        (position, 31.0),
        (94.0, 112.0),
        40,
    )
for name in (
    "MenuMercenaryMask",
    "MenuEmptyMask",
    "MenuMercenaryMaskLabel",
    "MenuEmptyMaskLabel",
):
    widget(name).set_visibility(unreal.SlateVisibility.COLLAPSED)

turn_panel = widget("TurnPanel", unreal.CanvasPanel)
canvas_layout(turn_panel, (-580.0, 8.0), (1090.0, 174.0), 0)

for index in range(10):
    token = widget(f"TurnToken_{index}", unreal.CanvasPanel)
    crop = widget(f"TurnPortraitCrop_{index}", unreal.ScaleBox)
    portrait = widget(f"TurnPortrait_{index}", unreal.Image)
    if portrait.get_parent() != crop or crop.get_parent() != token:
        raise RuntimeError(
            f"Turn portrait {index} must be nested Token > Crop > Portrait"
        )
    crop.set_editor_property("stretch", unreal.Stretch.SCALE_TO_FILL)
    crop.set_editor_property("stretch_direction", unreal.StretchDirection.BOTH)
    crop.set_clipping(unreal.WidgetClipping.CLIP_TO_BOUNDS_ALWAYS)
    token.set_clipping(unreal.WidgetClipping.CLIP_TO_BOUNDS_ALWAYS)

    token_x = 5.0 + 109.0 * index
    canvas_bounds(token, (token_x, 30.0), (108.0, 144.0), 10)
    canvas_bounds(
        widget(f"TurnFrame_{index}", unreal.Image),
        (0.0, 0.0),
        (108.0, 144.0),
        5,
    )
    canvas_bounds(crop, (14.0, 17.0), (80.0, 86.0), 10)
    canvas_bounds(
        widget(f"TurnCurrent_{index}", unreal.Image),
        (9.0, 11.0),
        (90.0, 97.0),
        40,
    )
    widget(f"TurnSpeedPlate_{index}", unreal.Border).set_visibility(
        unreal.SlateVisibility.COLLAPSED
    )
    canvas_bounds(
        widget(f"TurnSpeedIcon_{index}", unreal.Image),
        (13.0, 103.0),
        (36.0, 36.0),
        21,
    )
    speed = widget(f"TurnSpeed_{index}", unreal.TextBlock)
    canvas_bounds(speed, (52.0, 103.0), (42.0, 34.0), 22)
    speed.set_editor_property("justification", unreal.TextJustify.CENTER)
    set_font_size(speed, 22)

    divider = widget(f"TurnRoundDivider_{index}", unreal.Border)
    divider.set_editor_property(
        "brush_color", unreal.LinearColor(1.0, 1.0, 1.0, 1.0)
    )
    canvas_layout(divider, (token_x, 0.0), (108.0, 34.0), 30)

    label = widget(f"TurnRoundLabel_{index}", unreal.TextBlock)
    label.set_editor_property(
        "color_and_opacity",
        unreal.SlateColor(
            specified_color=unreal.LinearColor(0.96, 0.78, 0.40, 1.0),
            color_use_rule=unreal.SlateColorStylingMode.USE_COLOR_SPECIFIED,
        ),
    )
    label.set_editor_property("justification", unreal.TextJustify.CENTER)
    set_font_size(label, 19)
    canvas_layout(label, (token_x, 2.0), (108.0, 30.0), 31)

unreal.BlueprintEditorLibrary.compile_blueprint(asset)
if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
    raise RuntimeError(f"Could not save {ASSET_PATH}")

unreal.log(
    "RD_COMBAT_HUD_READABILITY "
    "skill_cards=200x228 frame=v03_combat visible_ratio=0.877193 "
    "button_fonts=30 end_turn_font=34 round_labels=integrated "
    "artifacts=icons_only ap=720x124 "
    "portrait_crops=10 reusable_turn_frames=10 token_size=108x144 "
    "turn_speeds=10 "
    "round_markers=10 option_icons=4"
)
