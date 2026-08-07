"""AllyName_Center 의 부모 사슬과 슬롯 종류를 알아본다."""
from pathlib import Path

import unreal

RESULT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/name_center.txt")
LINES = []

blueprint = unreal.EditorAssetLibrary.load_asset("/Game/UI/CombatLayouts/WBP_CombatHUD04")
tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")

for name in ("AllyName_Center", "AllyName", "EnemyName_Center"):
    widget = unreal.find_object(None, f"{tree.get_path_name()}.{name}")
    if widget is None:
        LINES.append(f"{name}: 없음")
        continue
    chain = []
    node = widget
    while node is not None:
        slot = node.get_editor_property("slot")
        chain.append(f"{node.get_name()}({type(node).__name__}, "
                     f"slot={type(slot).__name__ if slot else None})")
        node = node.get_parent()
    LINES.append(f"{name}: " + " <- ".join(chain))

RESULT.write_text("\n".join(LINES) + "\n", encoding="utf-8")
unreal.log("\n".join(LINES))
