"""CommandCost/Cooldown 글자 칸의 부모 사슬과 슬롯·z 를 본다."""
from pathlib import Path

import unreal

RESULT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/cost_chain.txt")
LINES = []
blueprint = unreal.EditorAssetLibrary.load_asset("/Game/UI/CombatLayouts/WBP_CombatHUD04")
tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")

for name in ("CommandCost_0_Center", "CommandCostBadge_0",
             "CommandCooldown_0_Center", "CommandCooldownBadge_0Mount"):
    widget = unreal.find_object(None, f"{tree.get_path_name()}.{name}")
    if widget is None:
        LINES.append(f"{name}: 없음")
        continue
    chain = []
    node = widget
    while node is not None:
        slot = node.get_editor_property("slot")
        z = slot.get_z_order() if isinstance(slot, unreal.CanvasPanelSlot) else "-"
        chain.append(f"{node.get_name()}(z{z},{type(slot).__name__ if slot else '-'})")
        node = node.get_parent()
    LINES.append(f"{name}: " + " <- ".join(chain))

RESULT.write_text("\n".join(LINES) + "\n", encoding="utf-8")
