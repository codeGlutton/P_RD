"""Rebuild only the authored mercenary inventory tab after editor startup."""

import unreal


unreal.SystemLibrary.execute_console_command(
    None, "RD.Editor.BuildCombatHUDInventoryTab"
)
unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
