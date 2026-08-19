import unreal


unreal.SystemLibrary.execute_console_command(
    None, "RD.Editor.BuildSkillTacticalDiagram"
)
unreal.log("RD_SKILL_TACTICAL_PYTHON_BUILD_DONE")
unreal.SystemLibrary.quit_editor()
