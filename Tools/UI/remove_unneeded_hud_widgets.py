"""Take back the three widget families the design doesn't want.

되돌리는 것
-----------
    TurnName_%d             턴바에 이름은 안 쓰기로 함
    CommandCostLine_%d      AP 는 카드 오른쪽 배지에 이미 적혀 있음
    CommandCooldownIcon_%d  쿨타임도 오른쪽 배지에 이미 있음

코드 쪽 Find 는 그대로 둔다. 못 찾으면 조용히 건너뛰므로 자산에서 빼기만 하면
되고, 나중에 배치가 바뀌어 다시 필요해지면 이름만 만들면 붙는다.

지우는 대신 캔버스에서 떼고 이름을 바꾸면 블루프린트 변수 GUID 가 끊겨
ensure 가 뜬다. 그래서 **부모에서 떼기만 하고 이름은 건드리지 않는다.**

Run headless:
    UnrealEditor-Cmd.exe <project> -run=pythonscript
        -script="Tools/UI/remove_unneeded_hud_widgets.py"
"""

from pathlib import Path

import unreal

RESULT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/hud_widgets_remove.txt")
ASSET = "/Game/UI/CombatLayouts/WBP_CombatHUD04"
LINES = []

DROP_PREFIXES = ("TurnName_", "CommandCostLine_", "CommandCooldownIcon_")


def build():
    blueprint = unreal.EditorAssetLibrary.load_asset(ASSET)
    if blueprint is None:
        raise RuntimeError(f"missing {ASSET}")
    blueprint.modify()
    tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")
    tree.modify()
    prefix = tree.get_path_name() + "."

    removed = 0
    for obj in list(unreal.ObjectIterator()):
        if not isinstance(obj, unreal.Widget):
            continue
        if not str(obj.get_path_name()).startswith(prefix):
            continue
        name = str(obj.get_name())
        if not name.startswith(DROP_PREFIXES):
            continue
        parent = obj.get_parent()
        if parent is None:
            continue
        parent.modify()
        obj.modify()
        if parent.remove_child(obj):
            removed += 1

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    saved = unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)
    LINES.append(f"removed={removed} saved={saved}")
    if not saved:
        LINES.append("SAVE BLOCKED -- 에디터/게임에서 이 WBP 를 닫고 다시 실행")


try:
    build()
except Exception as error:  # noqa: BLE001
    import traceback
    LINES.append("FAILED: %s" % error)
    LINES.append(traceback.format_exc())
finally:
    RESULT.write_text("\n".join(LINES), encoding="utf-8")
