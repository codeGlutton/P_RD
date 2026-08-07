"""전투 확인 2차분.

1. 용병 상세 스킬 칸에 아이콘 자리(HireDetailSkillIcon_N)를 만든다.
   판·글자 사이에 끼우고 기본은 NoDraw -- 그림은 C++ 이 넣는다.

3. 턴 토큰 속도 줄을 원래 자리(y103)로 되돌린다.
   y77 은 토큰 틀 그림의 속 구조를 모른 채 옮긴 눈대중이었고 틀 왼쪽
   테두리에 걸렸다. 제대로 된 자리는 사람이 그림에 그어 주는 칸으로 잡는다
   (place_turn_speed.py).

Run headless:
    UnrealEditor-Cmd.exe <project> -run=pythonscript
        -script="Tools/UI/fix_combat_ui2.py"
"""

import unreal
from pathlib import Path

RESULT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/fix_combat_ui2.txt")
LINES = []


def say(text):
    LINES.append(text)
    unreal.log(text)


def find(tree, name):
    return unreal.find_object(None, f"{tree.get_path_name()}.{name}")


def main():
    # 1. 스킬 칸 아이콘 자리
    hire = unreal.EditorAssetLibrary.load_asset(
        "/Game/UI/CombatLayouts/WBP_MercenaryHire_Marchbound")
    tree = unreal.find_object(None, hire.get_path_name() + ":WidgetTree")
    say("WBP_MercenaryHire_Marchbound")
    for index in range(6):
        mount = find(tree, f"HireDetailSkillArt_{index}Mount")
        if mount is None:
            say(f"    칸 {index}: 묶음 없음")
            continue
        name = f"HireDetailSkillIcon_{index}"
        icon = find(tree, name)
        if icon is None:
            icon = unreal.new_object(unreal.Image, outer=tree, name=name)
        hire.modify()
        mount.modify()
        icon.modify()

        # 판(첫 아이) 바로 뒤에 끼우려면 순서를 다시 짠다.
        children = list(mount.get_all_children())
        kept = []
        for child in children:
            if child is icon:
                continue
            slot = child.get_editor_property("slot")
            if isinstance(slot, unreal.OverlaySlot):
                kept.append((child, slot.get_editor_property("horizontal_alignment"),
                             slot.get_editor_property("vertical_alignment"),
                             slot.get_editor_property("padding")))
            else:
                kept.append((child, None, None, None))
        for child, _h, _v, _p in kept:
            mount.remove_child(child)
        if icon.get_parent() is not None:
            icon.get_parent().remove_child(icon)

        def put(child, halign=None, valign=None, padding=None):
            mount.add_child(child)
            slot = child.get_editor_property("slot")
            if isinstance(slot, unreal.OverlaySlot) and halign is not None:
                slot.set_horizontal_alignment(halign)
                slot.set_vertical_alignment(valign)
                slot.set_padding(padding)

        put(*kept[0])                     # 판
        # 아이콘 -- 116px 팔각판 속에 맞춰 사방 22px 들인다.
        put(icon, unreal.HorizontalAlignment.H_ALIGN_FILL,
            unreal.VerticalAlignment.V_ALIGN_FILL,
            unreal.Margin(22.0, 22.0, 22.0, 22.0))
        for item in kept[1:]:
            put(*item)

        # 기본은 안 그림. 빈 칸이 흰 사각으로 뜨지 않게 하고, 그림과 표시는
        # C++(BuildDetail)이 켠다.
        brush = icon.get_editor_property("brush")
        brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.NO_DRAW_TYPE)
        icon.set_editor_property("brush", brush)
        icon.set_visibility(unreal.SlateVisibility.COLLAPSED)
        say(f"    HireDetailSkillIcon_{index} 만듦 (판 바로 뒤)")

    unreal.SystemLibrary.execute_console_command(
        None, "RD.Editor.CleanWidgetVariables /Game/UI/CombatLayouts/WBP_MercenaryHire_Marchbound")
    unreal.BlueprintEditorLibrary.compile_blueprint(hire)
    say(f"저장={unreal.EditorAssetLibrary.save_loaded_asset(hire, False)}")

    # 3. 속도 줄 원위치
    hud = unreal.EditorAssetLibrary.load_asset("/Game/UI/CombatLayouts/WBP_CombatHUD04")
    tree = unreal.find_object(None, hud.get_path_name() + ":WidgetTree")
    say("WBP_CombatHUD04")
    for index in range(12):
        for name in (f"TurnSpeedIcon_{index}", f"TurnSpeed_{index}_Center"):
            widget = find(tree, name)
            slot = widget.get_editor_property("slot") if widget else None
            if not isinstance(slot, unreal.CanvasPanelSlot):
                continue
            offsets = slot.get_offsets()
            if abs(offsets.top - 77.0) < 0.5:
                hud.modify()
                widget.modify()
                slot.set_offsets(unreal.Margin(offsets.left, 103.0,
                                               offsets.right, offsets.bottom))
    say("    속도 줄 77 -> 103 (원위치)")
    unreal.BlueprintEditorLibrary.compile_blueprint(hud)
    say(f"저장={unreal.EditorAssetLibrary.save_loaded_asset(hud, False)}")

    RESULT.parent.mkdir(parents=True, exist_ok=True)
    RESULT.write_text("\n".join(LINES) + "\n", encoding="utf-8")


main()
