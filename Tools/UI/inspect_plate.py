"""DetailFreePlate 상태와 판때기 텍스처 로드를 점검한다."""
from pathlib import Path

import unreal

RESULT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/plate_state.txt")
LINES = []

tex = unreal.EditorAssetLibrary.load_asset(
    "/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Defeat/"
    "T_MB_Defeat_BattleSummary.T_MB_Defeat_BattleSummary")
LINES.append(f"직접 로드: {tex}")

blueprint = unreal.EditorAssetLibrary.load_asset(
    "/Game/UI/CombatDetail/WBP_CombatDetailOverlay")
tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")
plate = unreal.find_object(None, f"{tree.get_path_name()}.DetailFreePlate")
if plate is None:
    LINES.append("DetailFreePlate 없음")
else:
    brush = plate.get_editor_property("brush")
    slot = plate.get_editor_property("slot")
    offsets = slot.get_offsets() if isinstance(slot, unreal.CanvasPanelSlot) else None
    LINES.append(f"brush={brush.get_editor_property('resource_object')}")
    LINES.append(f"vis={plate.get_editor_property('visibility')}")
    if offsets:
        LINES.append(f"box=({offsets.left:.0f},{offsets.top:.0f},"
                     f"{offsets.right:.0f},{offsets.bottom:.0f})"
                     f" z={slot.get_z_order()}")

RESULT.write_text("\n".join(LINES) + "\n", encoding="utf-8")
