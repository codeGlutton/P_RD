"""Run the editor-only structural combat HUD builder."""

import unreal


unreal.SystemLibrary.execute_console_command(
    None, "RD.Editor.BuildCombatHUDAdditions"
)
unreal.log("RD_COMBAT_HUD_BUILD_COMMAND dispatched")
