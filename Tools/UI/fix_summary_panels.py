"""요약판(아군·적) 정리 + 턴 토큰 속도 숫자 정렬.

1. HP 숫자가 안 보이던 것
   ``HP n / n`` 글자는 코드가 매 틱 채우고 있었다. 안 보인 이유는 층 --
   글자는 HPBackMount(z5) 안, ProgressBar 는 캔버스 z6 이라 **바가 글자를
   덮었다.** 바를 묶음 안(판 뒤, 글자 앞)으로 넣어 층을 바로잡는다.
   덤으로 바가 판과 한 몸이 되어 옮길 때 같이 간다.

2. 턴 토큰 속도 숫자
   가운데 정렬이라 아이콘과 숫자 사이가 벌어졌다. 왼쪽 정렬로 아이콘 옆에
   바짝 붙인다.

Run headless:
    UnrealEditor-Cmd.exe <project> -run=pythonscript
        -script="Tools/UI/fix_summary_panels.py"
"""

import unreal
from pathlib import Path

RESULT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/fix_summary.txt")
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
    offsets = slot.get_offsets()
    align = slot.get_alignment()
    return (offsets.left - align.x * offsets.right,
            offsets.top - align.y * offsets.bottom,
            offsets.right, offsets.bottom)


def tuck_bar_into_mount(blueprint, tree, bar_name, mount_name):
    """ProgressBar 를 HP 묶음 안, 판 바로 뒤에 끼운다."""
    bar = find(tree, bar_name)
    mount = find(tree, mount_name)
    if bar is None or mount is None:
        say(f"    {bar_name}: 없음")
        return
    if bar.get_parent() == mount:
        say(f"    {bar_name}: 이미 묶음 안")
        return
    bar_box = canvas_box(bar)
    mount_box = canvas_box(mount)
    if bar_box is None or mount_box is None:
        say(f"    {bar_name}: 자리 못 읽음")
        return

    blueprint.modify()
    mount.modify()
    bar.modify()

    kept = []
    for child in list(mount.get_all_children()):
        slot = child.get_editor_property("slot")
        if isinstance(slot, unreal.OverlaySlot):
            kept.append((child, slot.get_editor_property("horizontal_alignment"),
                         slot.get_editor_property("vertical_alignment"),
                         slot.get_editor_property("padding")))
        else:
            kept.append((child, None, None, None))
    for child, _h, _v, _p in kept:
        mount.remove_child(child)
    bar_parent = bar.get_parent()
    if bar_parent is not None:
        bar_parent.remove_child(bar)

    def put(child, halign=None, valign=None, padding=None):
        mount.add_child(child)
        slot = child.get_editor_property("slot")
        if isinstance(slot, unreal.OverlaySlot) and halign is not None:
            slot.set_horizontal_alignment(halign)
            slot.set_vertical_alignment(valign)
            slot.set_padding(padding)

    put(*kept[0])                      # 판(HPBack)
    # 바 -- 원래 자리를 여백으로 재현.
    mount.add_child(bar)
    slot = bar.get_editor_property("slot")
    if isinstance(slot, unreal.OverlaySlot):
        slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
        slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_FILL)
        slot.set_padding(unreal.Margin(
            bar_box[0] - mount_box[0], bar_box[1] - mount_box[1],
            (mount_box[0] + mount_box[2]) - (bar_box[0] + bar_box[2]),
            (mount_box[1] + mount_box[3]) - (bar_box[1] + bar_box[3])))
    for item in kept[1:]:              # 글자들 -- 바 위로.
        put(*item)
    say(f"    {bar_name} -> {mount_name} 안 (판 뒤 · 글자 앞)")


def main():
    hud = unreal.EditorAssetLibrary.load_asset("/Game/UI/CombatLayouts/WBP_CombatHUD04")
    tree = unreal.find_object(None, hud.get_path_name() + ":WidgetTree")
    say("WBP_CombatHUD04")
    tuck_bar_into_mount(hud, tree, "AllyHPBar", "AllyHPBackMount")
    tuck_bar_into_mount(hud, tree, "EnemyHPBar", "EnemyHPBackMount")

    # 속도 숫자: 아이콘 옆에 붙인다.
    for index in range(12):
        text = find(tree, f"TurnSpeed_{index}")
        if isinstance(text, unreal.TextBlock):
            hud.modify()
            text.modify()
            text.set_editor_property("justification", unreal.TextJustify.LEFT)
        holder = find(tree, f"TurnSpeed_{index}_Center")
        slot = holder.get_editor_property("slot") if holder else None
        if isinstance(slot, unreal.CanvasPanelSlot):
            offsets = slot.get_offsets()
            # 아이콘 오른쪽 6px 뒤부터.
            icon = find(tree, f"TurnSpeedIcon_{index}")
            icon_slot = icon.get_editor_property("slot") if icon else None
            if isinstance(icon_slot, unreal.CanvasPanelSlot):
                io = icon_slot.get_offsets()
                slot.set_offsets(unreal.Margin(io.left + io.right + 6.0,
                                               offsets.top, offsets.right,
                                               offsets.bottom))
    say("    속도 숫자: 왼쪽 정렬 · 아이콘 옆 6px")

    unreal.SystemLibrary.execute_console_command(
        None, "RD.Editor.CleanWidgetVariables /Game/UI/CombatLayouts/WBP_CombatHUD04")
    unreal.BlueprintEditorLibrary.compile_blueprint(hud)
    say(f"저장={unreal.EditorAssetLibrary.save_loaded_asset(hud, False)}")
    RESULT.parent.mkdir(parents=True, exist_ok=True)
    RESULT.write_text("\n".join(LINES) + "\n", encoding="utf-8")


main()
