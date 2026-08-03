"""Verify the structural contract of WBP_MonsterTab_Marchbound."""

import unreal


ASSET = "/Game/UI/MonsterTab/WBP_MonsterTab_Marchbound"
OBJECT_PREFIX = ASSET + ".WBP_MonsterTab_Marchbound:WidgetTree."
REQUIRED = [
    "MonsterTabViewportRoot",
    "MonsterTabScale",
    "MonsterTabCanvas",
    "MonsterTabBaseFrame",
    "MonsterTabTitleText",
    "MonsterDetailPortrait",
    "MonsterDetailNameText",
    "MonsterDetailHPBar",
    "MonsterDetailHPText",
    "MonsterDetailAPText",
    "MonsterDetailSpeedText",
]
for index in range(3):
    REQUIRED.extend(
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
    REQUIRED.extend([f"MonsterSkillBox_{index}", f"MonsterSkillName_{index}"])

asset = unreal.EditorAssetLibrary.load_asset(ASSET)
if asset is None:
    raise RuntimeError(f"Missing monster-tab WBP: {ASSET}")

missing = [name for name in REQUIRED if unreal.load_object(None, OBJECT_PREFIX + name) is None]
if missing:
    raise RuntimeError(f"Monster-tab WBP missing widgets: {missing}")

unreal.log(f"RD_MONSTER_TAB_VERIFY success widgets={len(REQUIRED)} asset={ASSET}")
