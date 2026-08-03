"""Read-only dump of the authored combat mercenary-tab widget hierarchy."""

import unreal


ASSET = "/Game/UI/CombatLayouts/WBP_CombatHUD04"
PREFIX = f"{ASSET}.WBP_CombatHUD04:WidgetTree."
NAMES = [
    "MercenaryPanel",
    "RuntimeMercenaryRosterShell",
    "MercenaryBoard",
    "MercenaryHeroPortrait",
    "MercenaryDetailName",
    "MercenaryDetailHP",
    "MercenaryDetailAP",
    "MercenaryDetailSpeed",
    "TurnPanel",
    "TurnRoundLabel_0",
    "TurnRoundLabel_1",
]
for index in range(3):
    NAMES.extend(
        [
            f"MercenaryCardScale_{index}",
            f"PartyCard_{index}",
            f"PartyPlate_{index}",
            f"PartyPortrait_{index}",
            f"PartyButton_{index}",
        ]
    )


for name in NAMES:
    target = unreal.load_object(None, PREFIX + name)
    if target is None:
        unreal.log_warning(f"RD_MERCENARY_TAB missing={name}")
        continue
    parent = target.get_parent()
    slot = target.slot
    layout = ""
    if isinstance(slot, unreal.CanvasPanelSlot):
        position = slot.get_position()
        size = slot.get_size()
        layout = (
            f" position=({position.x:.1f},{position.y:.1f})"
            f" size=({size.x:.1f},{size.y:.1f})"
            f" z={slot.get_z_order()}"
        )
    resource = ""
    if isinstance(target, unreal.Image):
        brush = target.get_editor_property("brush")
        obj = brush.get_editor_property("resource_object")
        resource = f" resource={obj.get_path_name() if obj else 'None'}"
    unreal.log(
        "RD_MERCENARY_TAB"
        f" name={name} type={target.get_class().get_name()}"
        f" parent={parent.get_name() if parent else 'None'}"
        f" visibility={target.get_visibility()}"
        f"{layout}{resource}"
    )
