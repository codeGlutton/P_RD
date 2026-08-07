"""Dump the enemy and mercenary summary widgets from the combat HUD."""

import unreal


ASSET_PATH = "/Game/UI/CombatLayouts/WBP_CombatHUD04"
OBJECT_PREFIX = f"{ASSET_PATH}.WBP_CombatHUD04:WidgetTree."


def vec(value):
    return f"({value.x:.1f},{value.y:.1f})"


if unreal.load_asset(ASSET_PATH) is None:
    raise RuntimeError(f"Missing {ASSET_PATH}")

names = [
    "EnemyPanel",
    "EnemyPlate",
    "EnemyPortraitFrame",
    "EnemyPortrait",
    "EnemyBadgeText",
    "EnemyName",
    "EnemyHPBack",
    "EnemyHPBar",
    "EnemyHPText",
    "EnemyAPText",
    "EnemySpeedIcon",
    "EnemySpeedText",
    "EnemyStatusLabel",
    "EnemyStatus",
    "EnemyForecast",
]
for index in range(3):
    names.extend(
        [
            f"EnemyStatusFrame_{index}",
            f"EnemyStatusIcon_{index}",
            f"EnemyStatusCount_{index}",
        ]
    )

names.extend(
    [
        "AllyPanel",
        "AllyPlate",
        "AllyPortraitFrame",
        "AllyPortrait",
        "AllyBadgeText",
        "AllyName",
        "AllyHPBack",
        "AllyHPBar",
        "AllyHPText",
        "AllyAPText",
        "AllySpeedIcon",
        "AllySpeedText",
        "AllyStatusLabel",
        "AllyStatus",
        "AllySummaryHint",
    ]
)
for index in range(3):
    names.extend(
        [
            f"AllyStatusFrame_{index}",
            f"AllyStatusIcon_{index}",
            f"AllyStatusCount_{index}",
        ]
    )

for name in names:
    target = unreal.load_object(None, OBJECT_PREFIX + name)
    if target is None:
        unreal.log_warning(f"RD_ENEMY_SUMMARY missing={name}")
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
    if isinstance(target, unreal.Image):
        brush = target.get_editor_property("brush")
        resource = brush.get_editor_property("resource_object")
        extra += f" resource={resource.get_path_name() if resource else 'None'}"
    if isinstance(target, unreal.TextBlock):
        font = target.get_editor_property("font")
        extra += (
            f" font={font.get_editor_property('size')}"
            f" text={str(target.get_text())!r}"
        )

    unreal.log(
        "RD_ENEMY_SUMMARY"
        f" name={target.get_name()}"
        f" type={target.get_class().get_name()}"
        f" parent={parent.get_name() if parent else 'None'}"
        f" visibility={target.get_visibility()}"
        f"{layout}{extra}"
    )

unreal.log("RD_ENEMY_SUMMARY_DUMP success")
