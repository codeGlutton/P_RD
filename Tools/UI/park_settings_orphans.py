"""Re-attach the settings widgets the re-layout left dangling, and say what happened.

``clear_children()`` 는 자식을 떼기만 하고 지우지 않는다. 떼인 위젯은 트리에서
안 보이는데 블루프린트 변수 목록에는 GUID 가 남아, 컴파일할 때마다
"Variable [X] was deleted but still has a GUID" ensure 가 뜬다(실측 98개).
파이썬에는 그 GUID 를 지우는 길이 없으므로, 접힌 캔버스에 도로 붙이는 게 답이다.

빌더 안에서 한 번 시도했으나 하나도 못 붙였다. 목록 98개 중 대부분은 여전히
제 부모(떼인 컨테이너)를 가진 **자손**이고, 부모가 없는 것은 떼인 컨테이너
자신뿐이기 때문이다. 컨테이너를 붙이면 그 아래가 통째로 따라온다.

이 스크립트는 그 사실을 눈으로 확인하려고 분리했다. 붙인 것과 못 붙인 것을
따로 적는다 -- 0 이면 조용히 지나가는 대신 왜 0 인지 알 수 있어야 한다.
"""

from pathlib import Path

import unreal

RESULT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/settings_park.txt")
ASSET = "/Game/UI/WBP_SettingsPanel"
ORPHANS = Path(__file__).resolve().parent / "settings_orphans.txt"
LINES = []


def find(tree, name):
    return unreal.find_object(None, tree.get_path_name() + "." + name)


def run():
    blueprint = unreal.EditorAssetLibrary.load_asset(ASSET)
    blueprint.modify()
    tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")
    tree.modify()

    root = find(tree, "SettingsPanelRoot")
    parking = find(tree, "Set_parked")
    if not isinstance(parking, unreal.CanvasPanel):
        parking = unreal.new_object(unreal.CanvasPanel, outer=tree, name="Set_parked")
        slot = root.add_child_to_canvas(parking)
        slot.set_anchors(unreal.Anchors(unreal.Vector2D(0.0, 0.0), unreal.Vector2D(0.0, 0.0)))
        slot.set_size(unreal.Vector2D(1.0, 1.0))
        slot.set_z_order(-100)
    parking.modify()
    parking.set_visibility(unreal.SlateVisibility.COLLAPSED)

    parked, had_parent, missing = [], [], []
    for line in ORPHANS.read_text(encoding="utf-8").splitlines():
        name = line.strip()
        if not name or name.startswith("#"):
            continue
        widget = find(tree, name)
        if widget is None:
            missing.append(name)
            continue
        if widget.get_parent() is not None:
            had_parent.append(name)
            continue
        widget.modify()
        slot = parking.add_child_to_canvas(widget)
        slot.set_anchors(unreal.Anchors(unreal.Vector2D(0.0, 0.0), unreal.Vector2D(0.0, 0.0)))
        slot.set_size(unreal.Vector2D(1.0, 1.0))
        widget.set_visibility(unreal.SlateVisibility.COLLAPSED)
        parked.append(name)

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    saved = unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)
    LINES.append(f"saved={saved}")
    LINES.append(f"붙임 {len(parked)}: {', '.join(parked)}")
    LINES.append(f"이미 부모 있음 {len(had_parent)}  (붙인 컨테이너 아래로 딸려 온다)")
    LINES.append(f"아예 없음 {len(missing)}: {', '.join(missing)}")


try:
    run()
except Exception as error:  # noqa: BLE001
    import traceback
    LINES.append("FAILED: %s" % error)
    LINES.append(traceback.format_exc())
finally:
    RESULT.write_text("\n".join(LINES), encoding="utf-8")
