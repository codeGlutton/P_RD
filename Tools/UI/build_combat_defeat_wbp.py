"""Import defeat textures, build the responsive WBP, and verify the asset."""

import runpy
from pathlib import Path

import unreal


PROJECT_ROOT = Path(r"D:\UnrealProjects\P_RD_develop_20260803")
IMPORT_SCRIPT = PROJECT_ROOT / "Tools" / "UI" / "import_defeat_parts.py"
WBP_PATH = "/Game/UI/CombatResult/WBP_CombatDefeat"


def main() -> None:
    runpy.run_path(str(IMPORT_SCRIPT), run_name="__main__")
    # Recreate the generated WBP so every designer widget keeps its intended
    # stable name after iterative rebuilds.
    if unreal.EditorAssetLibrary.does_asset_exist(WBP_PATH):
        if not unreal.EditorAssetLibrary.delete_asset(WBP_PATH):
            raise RuntimeError(f"Could not replace generated WBP: {WBP_PATH}")
    unreal.SystemLibrary.execute_console_command(None, "RD.Editor.BuildCombatDefeat")

    widget_blueprint = unreal.load_asset(WBP_PATH)
    if widget_blueprint is None:
        raise RuntimeError(f"Defeat WBP was not created: {WBP_PATH}")
    if not unreal.EditorAssetLibrary.save_asset(WBP_PATH, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save defeat WBP: {WBP_PATH}")

    unreal.log(f"RD_COMBAT_DEFEAT_WBP_VERIFY success={WBP_PATH}")


main()
