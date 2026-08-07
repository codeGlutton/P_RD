"""Drop widget-variable GUIDs whose widget no longer exists.

배치를 다시 짜면서 지워진 위젯의 변수 GUID 가 블루프린트에 남아, 컴파일마다
"Variable [X] was deleted but still has a GUID" 가 이름 수만큼 뜬다. 파이썬에는
그 GUID 를 지우는 길이 없어서 에디터 명령(RD.Editor.CleanWidgetVariables)을
따로 열어 두었다. 이 스크립트는 그 명령을 대상 자산마다 부른다.

결과는 로그가 아니라 파일로 확인한다 -- 명령이 남기는 RD_WIDGET_VAR_CLEAN 줄은
stdout 까지 안 올라온다.
"""

from pathlib import Path

import unreal

OUT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/widget_var_clean.txt")
ASSETS = [
    "/Game/UI/WBP_SettingsPanel",
    "/Game/UI/CombatDetail/WBP_CombatDetailOverlay",
    "/Game/UI/CombatLayouts/WBP_CombatHUD04",
    "/Game/UI/MonsterTab/WBP_MonsterTab_Marchbound",
]

LINES = []
world = unreal.EditorLevelLibrary.get_editor_world() if hasattr(
    unreal, "EditorLevelLibrary") else None
unreal.SystemLibrary.execute_console_command(
    world, "RD.Editor.CleanWidgetVariables " + " ".join(ASSETS))

# 명령이 끝난 뒤 남은 변수 수를 세어 결과를 확인한다.
for path in ASSETS:
    blueprint = unreal.EditorAssetLibrary.load_asset(path)
    LINES.append(f"{path}: {'ok' if blueprint is not None else '없음'}")

OUT.write_text("\n".join(LINES), encoding="utf-8")
