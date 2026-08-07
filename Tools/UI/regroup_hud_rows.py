"""전투 HUD 의 핍(AP 눈금)·상태 줄을 하위 캔버스로 묶는다.

뿌리 캔버스는 이미 구역(TurnPanel·AllyPanel…)으로 나뉘어 있다. 진짜 뭉친
곳은 그 안이다 -- 파티 칸 하나에 위젯 33개 중 20개가 AP 핍이고, 턴 AP 바도
22개 중 20개가 핍이다. 에디터에서 칸을 집으려면 핍 스무 개를 헤집어야 했다.

    PartyAPPipRow_N    파티 칸의 AP 핍 20개
    PartyStatusRow_N   파티 칸의 상태 틀·아이콘·글
    TurnAPPipRow       턴 바의 AP 핍 20개

C++ 는 핍을 이름으로 찾아 켜고 끄기만 하므로(자리 안 옮김) 묶어도 배선이
안 깨진다.

Run headless:
    UnrealEditor-Cmd.exe <project> -run=pythonscript
        -script="Tools/UI/regroup_hud_rows.py"
"""

import unreal
from pathlib import Path

RESULT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/regroup_hud.txt")
ASSET = "/Game/UI/CombatLayouts/WBP_CombatHUD04"

LINES = []


def say(text):
    LINES.append(text)
    unreal.log(text)


def find(tree, name):
    return unreal.find_object(None, f"{tree.get_path_name()}.{name}")


def canvas_box(widget):
    slot = widget.get_editor_property("slot") if widget else None
    if not isinstance(slot, unreal.CanvasPanelSlot):
        return None
    anchors = slot.get_anchors()
    if (anchors.minimum.x != anchors.maximum.x
            or anchors.minimum.y != anchors.maximum.y):
        return None
    offsets = slot.get_offsets()
    align = slot.get_alignment()
    return (offsets.left - align.x * offsets.right,
            offsets.top - align.y * offsets.bottom,
            offsets.right, offsets.bottom, slot.get_z_order())


def gather(tree, blueprint, parent_name, section_name, z, rect, members):
    parent = find(tree, parent_name)
    if not isinstance(parent, unreal.CanvasPanel):
        say(f"    {parent_name}: 캔버스 아님 -- 건너뜀")
        return
    section = find(tree, section_name)
    if section is None:
        section = unreal.new_object(unreal.CanvasPanel, outer=tree,
                                    name=section_name)
    blueprint.modify()
    section.modify()
    if section.get_parent() is None:
        parent.add_child(section)
    slot = section.get_editor_property("slot")
    if isinstance(slot, unreal.CanvasPanelSlot):
        slot.set_alignment(unreal.Vector2D(0.0, 0.0))
        slot.set_auto_size(False)
        slot.set_offsets(unreal.Margin(*[float(v) for v in rect]))
        slot.set_z_order(z)

    moved = 0
    for name in members:
        widget = find(tree, name)
        if widget is None or widget.get_parent() == section:
            continue
        box = canvas_box(widget)
        if box is None:
            continue
        widget.modify()
        widget.get_parent().remove_child(widget)
        section.add_child(widget)
        child = widget.get_editor_property("slot")
        if isinstance(child, unreal.CanvasPanelSlot):
            child.set_alignment(unreal.Vector2D(0.0, 0.0))
            child.set_auto_size(False)
            child.set_offsets(unreal.Margin(box[0] - rect[0], box[1] - rect[1],
                                            box[2], box[3]))
            child.set_z_order(box[4])
        moved += 1
    say(f"    {section_name}: {moved}개 들임")


def main():
    blueprint = unreal.EditorAssetLibrary.load_asset(ASSET)
    tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")
    say(ASSET.rsplit("/", 1)[-1])

    for index in range(3):
        pips = [f"PartyAPPip_{index}_{p}" for p in range(10)] \
             + [f"PartyAPPipUsed_{index}_{p}" for p in range(10)]
        gather(tree, blueprint, f"PartyContent_{index}",
               f"PartyAPPipRow_{index}", 16, (171, 118, 153, 34), pips)
        status = [f"PartyStatusFrame_{index}_{s}" for s in range(3)] \
               + [f"PartyStatusIcon_{index}_{s}" for s in range(3)] \
               + [f"PartyStatusIcon_{index}", f"PartyStatus_{index}_Center"]
        gather(tree, blueprint, f"PartyContent_{index}",
               f"PartyStatusRow_{index}", 18, (171, 146, 163, 44), status)

    turn_pips = [f"TurnAPPip_{p}" for p in range(10)] \
              + [f"TurnAPPipUsed_{p}" for p in range(10)]
    gather(tree, blueprint, "TurnAPPanel", "TurnAPPipRow", 10,
           (70, 16, 320, 36), turn_pips)

    unreal.SystemLibrary.execute_console_command(
        None, f"RD.Editor.CleanWidgetVariables {ASSET}")
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    say(f"저장={unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)}")
    RESULT.parent.mkdir(parents=True, exist_ok=True)
    RESULT.write_text("\n".join(LINES) + "\n", encoding="utf-8")


main()
