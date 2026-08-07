"""Rebuild the Marchbound monster tab from the editor C++ source of truth."""

import unreal


ASSET_DIRECTORY = "/Game/UI/MonsterTab"

unreal.SystemLibrary.execute_console_command(None, "RD.Editor.BuildMonsterTab")
unreal.EditorAssetLibrary.save_directory(
    ASSET_DIRECTORY, only_if_is_dirty=False, recursive=True
)
unreal.log("RD_MONSTER_TAB_BUILD_COMMAND dispatched and saved")

