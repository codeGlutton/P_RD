"""Run the editor-only Marchbound mercenary-hire WBP builder."""

import unreal


unreal.SystemLibrary.execute_console_command(
    None, "RD.Editor.BuildMercenaryHire"
)
unreal.log("RD_MB_HIRE_BUILD_COMMAND dispatched")
