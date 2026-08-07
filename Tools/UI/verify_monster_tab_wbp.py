"""Verify the structural and authored-layout contract of the monster tab."""

import unreal


ASSET = "/Game/UI/MonsterTab/WBP_MonsterTab_Marchbound"
OBJECT_PREFIX = ASSET + ".WBP_MonsterTab_Marchbound:WidgetTree."
EXPECTED_CANVAS_BOUNDS = {
    "MonsterTabTitleText": ((340.0, 69.0), (640.0, 104.0)),
    "MonsterBackButton": ((1650.0, 151.0), (190.0, 76.0)),
    "MonsterDetailPortraitScale": ((530.0, 220.0), (530.0, 685.0)),
    "MonsterDetailHPBar": ((1225.0, 305.0), (590.0, 50.0)),
}


def widget(name, expected_type=None):
    result = unreal.load_object(None, OBJECT_PREFIX + name)
    if result is None:
        raise RuntimeError(f"Monster-tab WBP missing widget: {name}")
    if expected_type is not None and not isinstance(result, expected_type):
        raise RuntimeError(
            f"{name} is {result.get_class().get_name()}, expected {expected_type.__name__}"
        )
    return result


def vector_pair(value):
    return (round(value.x, 2), round(value.y, 2))


required = [
    "MonsterTabViewportRoot",
    "MonsterTabScale",
    "MonsterTabDesignSize",
    "MonsterTabCanvas",
    "MonsterTabBaseFrame",
    "MonsterTabTitleText",
    "MonsterBackButton",
    "MonsterDetailPortraitScale",
    "MonsterDetailPortrait",
    "MonsterDetailNameText",
    "MonsterDetailHPBar",
    "MonsterDetailHPText",
    "MonsterDetailAPText",
    "MonsterDetailSpeedText",
]
for index in range(3):
    required.extend(
        [
            f"MonsterRow_{index}",
            f"MonsterRowNormal_{index}",
            f"MonsterRowSelected_{index}",
            f"MonsterRowPortrait_{index}",
            f"MonsterRowName_{index}",
            f"MonsterRowButton_{index}",
        ]
    )
for index in range(4):
    required.extend([f"MonsterSkillBox_{index}", f"MonsterSkillName_{index}"])

asset = unreal.EditorAssetLibrary.load_asset(ASSET)
if asset is None:
    raise RuntimeError(f"Missing monster-tab WBP: {ASSET}")

for name in required:
    widget(name)

for name, (expected_position, expected_size) in EXPECTED_CANVAS_BOUNDS.items():
    slot = widget(name).slot
    if not isinstance(slot, unreal.CanvasPanelSlot):
        raise RuntimeError(f"{name} must use CanvasPanelSlot")
    actual_position = vector_pair(slot.get_position())
    actual_size = vector_pair(slot.get_size())
    if actual_position != expected_position or actual_size != expected_size:
        raise RuntimeError(
            f"{name} bounds={actual_position}/{actual_size}, "
            f"expected={expected_position}/{expected_size}"
        )

scale = widget("MonsterTabScale", unreal.ScaleBox)
if scale.get_editor_property("stretch") != unreal.Stretch.SCALE_TO_FIT:
    raise RuntimeError("MonsterTabScale must preserve the 16:9 composition")

unreal.log(
    f"RD_MONSTER_TAB_VERIFY success widgets={len(required)} "
    f"bounds={len(EXPECTED_CANVAS_BOUNDS)} asset={ASSET}"
)
