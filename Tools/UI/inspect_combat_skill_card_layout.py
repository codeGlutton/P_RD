"""Print the authored combat skill-card canvas layout without modifying it."""

import unreal


ASSET_PATH = "/Game/UI/CombatLayouts/WBP_CombatHUD04"
OBJECT_PREFIX = f"{ASSET_PATH}.WBP_CombatHUD04:WidgetTree."


def load_widget(name):
    result = unreal.load_object(None, OBJECT_PREFIX + name)
    if result is None:
        raise RuntimeError(f"Missing widget {name}")
    return result


def vector_text(vector):
    return f"({vector.x:.1f},{vector.y:.1f})"


def anchors_text(anchors):
    return (
        f"({anchors.minimum.x:.2f},{anchors.minimum.y:.2f})-"
        f"({anchors.maximum.x:.2f},{anchors.maximum.y:.2f})"
    )


def slot_text(slot):
    if isinstance(slot, unreal.CanvasPanelSlot):
        return (
            f"position={vector_text(slot.get_position())} "
            f"size={vector_text(slot.get_size())} "
            f"alignment={vector_text(slot.get_alignment())} "
            f"anchors={anchors_text(slot.get_anchors())} "
            f"z={slot.get_z_order()}"
        )
    return f"slot={slot.get_class().get_name()}"


asset = unreal.load_asset(ASSET_PATH)
if asset is None:
    raise RuntimeError(f"Missing asset {ASSET_PATH}")

for index in range(6):
    card = load_widget(f"CommandCard_{index}")
    slot = card.slot
    if not isinstance(slot, unreal.CanvasPanelSlot):
        raise RuntimeError(
            f"{card.get_name()} must use CanvasPanelSlot, "
            f"got {slot.get_class().get_name()}"
        )
    unreal.log(
        "RD_SKILL_CARD_LAYOUT "
        f"index={index} "
        f"{slot_text(slot)} "
        f"parent={card.get_parent().get_name()}"
    )
    if isinstance(card, unreal.PanelWidget):
        for child_index in range(card.get_children_count()):
            child = card.get_child_at(child_index)
            unreal.log(
                "RD_SKILL_CARD_CHILD "
                f"card={index} "
                f"name={child.get_name()} "
                f"type={child.get_class().get_name()} "
                f"{slot_text(child.slot)}"
            )


mercenary_names = [
    "MercenaryPanel",
    "MercenaryScrim",
    "MercenaryTitleText",
    "MercenarySubtitleText",
    "MercenaryGoldLabel",
    "MercenaryGoldText",
    "MercenaryCloseText",
    "MercenaryCloseButton",
    "MercenaryBoard",
]
for index in range(3):
    mercenary_names.extend(
        [
            f"MercenaryCardScale_{index}",
            f"PartyCard_{index}",
            f"PartyPortrait_{index}",
            f"PartyName_{index}",
            f"PartyHPBar_{index}",
            f"PartyHPText_{index}",
            f"PartyAPPlate_{index}",
            f"PartyAPText_{index}",
            f"PartyButton_{index}",
            f"PartyStatusIcon_{index}_0",
            f"PartyStatusFrame_{index}_0",
        ]
    )

for name in mercenary_names:
    target = load_widget(name)
    parent = target.get_parent()
    unreal.log(
        "RD_MERC_LAYOUT "
        f"name={name} "
        f"type={target.get_class().get_name()} "
        f"{slot_text(target.slot)} "
        f"parent={parent.get_name() if parent is not None else 'None'} "
        f"visibility={target.get_visibility()}"
    )

for index in range(3):
    card = load_widget(f"PartyCard_{index}")
    for child_index in range(card.get_children_count()):
        child = card.get_child_at(child_index)
        unreal.log(
            "RD_PARTY_CARD_CHILD "
            f"card={index} "
            f"name={child.get_name()} "
            f"type={child.get_class().get_name()} "
            f"{slot_text(child.slot)} "
            f"visibility={child.get_visibility()}"
        )
