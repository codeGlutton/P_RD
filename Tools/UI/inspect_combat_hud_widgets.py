"""Dump the committed combat HUD widget tree and paint-relevant properties."""

import unreal


ASSET_PATH = "/Game/UI/CombatLayouts/WBP_CombatHUD04"


def vec(value):
    return f"({value.x:.1f},{value.y:.1f})"


asset = unreal.load_asset(ASSET_PATH)
if asset is None:
    raise RuntimeError(f"Missing {ASSET_PATH}")

tree = unreal.load_object(
    None, f"{ASSET_PATH}.WBP_CombatHUD04:WidgetTree"
)
if tree is None:
    raise RuntimeError("Missing authored WidgetTree")

names = [
    "RootCanvas",
    "RoundPanel",
    "RoundPlate",
    "RoundText",
    "ObjectivePanel",
    "ObjectivePlate",
    "CommandLayer",
    "CommandLayerScale",
    "ConfirmPanel",
    "ConfirmPlate",
    "ConfirmButton",
    "EndTurnPanel",
    "TurnPanel",
    "TurnPlate",
    "MenuEmptyMask",
    "MenuEmptyMaskLabel",
    "MenuMercenaryMask",
    "MenuMercenaryMaskLabel",
]
for index in range(4):
    names.append(f"MenuButton_{index}")
for index in range(10):
    names.extend(
        [
            f"TurnToken_{index}",
            f"TurnPortraitCrop_{index}",
            f"TurnPortrait_{index}",
            f"TurnName_{index}",
            f"TurnCurrent_{index}",
            f"TurnRoundDivider_{index}",
            f"TurnRoundLabel_{index}",
        ]
    )
for index in range(3):
    names.extend(
        [
            f"PartyHPBar_{index}",
            f"PartyHPText_{index}",
            f"PartyStatus_{index}",
            f"PartyStatusIcon_{index}",
        ]
    )

for name in names:
    target = unreal.load_object(
        None, f"{ASSET_PATH}.WBP_CombatHUD04:WidgetTree.{name}"
    )
    if target is None:
        unreal.log_warning(f"RD_COMBAT_WIDGET missing={name}")
        continue
    parent = target.get_parent()
    slot = target.slot
    layout = ""
    if isinstance(slot, unreal.CanvasPanelSlot):
        layout = (
            f" pos={vec(slot.get_position())} size={vec(slot.get_size())}"
            f" align={vec(slot.get_alignment())} z={slot.get_z_order()}"
        )

    extra = ""
    if isinstance(target, unreal.Border):
        color = target.get_editor_property("brush_color")
        extra += (
            f" brush=({color.r:.3f},{color.g:.3f},{color.b:.3f},{color.a:.3f})"
        )
    if isinstance(target, unreal.Image):
        brush = target.get_editor_property("brush")
        resource = brush.get_editor_property("resource_object")
        tint = brush.get_editor_property("tint_color").get_editor_property("specified_color")
        extra += (
            f" resource={resource.get_path_name() if resource else 'None'}"
            f" tint=({tint.r:.3f},{tint.g:.3f},{tint.b:.3f},{tint.a:.3f})"
        )
    if isinstance(target, unreal.TextBlock):
        font = target.get_editor_property("font")
        extra += (
            f" font={font.get_editor_property('size')}"
            f" text={str(target.get_text())!r}"
        )
    unreal.log(
        "RD_COMBAT_WIDGET"
        f" name={target.get_name()}"
        f" type={target.get_class().get_name()}"
        f" parent={parent.get_name() if parent else 'None'}"
        f" visibility={target.get_visibility()}"
        f" enabled={target.get_is_enabled()}"
        f"{layout}{extra}"
    )

unreal.log("RD_COMBAT_WIDGET_DUMP success")
