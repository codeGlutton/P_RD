"""Run the editor-only Marchbound monster-tab WBP builder."""

import unreal


unreal.SystemLibrary.execute_console_command(None, "RD.Editor.BuildMonsterTab")
unreal.log("RD_MONSTER_TAB_BUILD_COMMAND dispatched")
