"""Print the committed mercenary-hire widget tree and authored layout."""

import unreal


ASSET_PATH = "/Game/UI/CombatLayouts/WBP_MercenaryHire_Marchbound"
OBJECT_PREFIX = f"{ASSET_PATH}.WBP_MercenaryHire_Marchbound:WidgetTree."


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
            f"auto={slot.get_editor_property('auto_size')} "
            f"z={slot.get_z_order()}"
        )
    if slot is None:
        return "slot=None"
    return f"slot={slot.get_class().get_name()}"


asset = unreal.load_asset(ASSET_PATH)
if asset is None:
    raise RuntimeError(f"Missing asset {ASSET_PATH}")

names = [
    "RootCanvas",
    "Backdrop",
    "Backdrop_Art",
    "HireBottomBar",
    "HireBottomBar_Art",
    "PartyCountText",
    "NoticeText",
    "DepartHolder",
    "DepartButton",
    "DepartLabel",
]
for index in range(6):
    names.extend(
        [
            f"HireCard_{index}",
            f"HireCard_{index}_Art",
            f"HireSelected_{index}",
            f"HirePortrait_{index}",
            f"HireName_{index}",
            f"HireRole_{index}",
            f"HireHP_{index}",
            f"HireSkill_{index}_0",
            f"HireSkill_{index}_1",
            f"HireBadge_{index}",
            f"HireSeal_{index}",
            f"HireTrait_{index}",
            f"HireButton_{index}",
        ]
    )
for index in range(3):
    names.extend(
        [
            f"PartySlot_{index}",
            f"PartySlotFace_{index}",
            f"PartySlotName_{index}",
        ]
    )

widgets = []
for name in names:
    target = unreal.load_object(None, OBJECT_PREFIX + name)
    if target is None:
        unreal.log_warning(f"RD_HIRE_LAYOUT_MISSING name={name}")
        continue
    widgets.append(target)
for target in widgets:
    parent = target.get_parent()
    details = ""
    if isinstance(target, unreal.TextBlock):
        details = f" text={target.get_text()} font={target.get_editor_property('font').get_editor_property('size')}"
    elif isinstance(target, unreal.Image):
        resource = target.get_editor_property("brush").get_editor_property("resource_object")
        details = f" texture={resource.get_path_name() if resource is not None else 'None'}"
    unreal.log(
        "RD_HIRE_LAYOUT "
        f"name={target.get_name()} "
        f"type={target.get_class().get_name()} "
        f"parent={parent.get_name() if parent is not None else 'None'} "
        f"visibility={target.get_visibility()} "
        f"{slot_text(target.slot)}"
        f"{details}"
    )
