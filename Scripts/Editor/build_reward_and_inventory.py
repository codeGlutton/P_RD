"""Rebuild the reward settlement WBP and exact-size inventory entry."""

import unreal


for command in (
    "RD.Editor.BuildRewardSettlement",
    "RD.Editor.VerifyRewardSettlement",
    "RD.Editor.BuildCombatHUDInventoryTab",
):
    unreal.log("RD_EDITOR_SCRIPT execute " + command)
    unreal.SystemLibrary.execute_console_command(None, command)

unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
