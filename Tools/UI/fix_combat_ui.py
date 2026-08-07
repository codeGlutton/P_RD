"""전투·용병 화면에서 게임 확인 때 잡힌 것들을 고친다.

1. 용병 카드: 고르면 글자가 사라짐
   묶을 때 HireSelected(고름 표시 판)가 캔버스에 남아, 묶음(z 낮음) 안의
   글자를 z5 로 덮었다. 원래 층은 판 < 고름표시 < 글자였다. 고름 표시를
   묶음 **안**, 판 바로 뒤·글자 앞에 끼워 원래 층을 되살린다.

2. 용병 화면 안내 문구(NoticeText): 필요 없다 -> 접는다.

4. AP 숫자 위 알약(TurnAPNumberPlate): 뺀다.

5. 아티팩트 뒤 판(ArtifactStripPlate·Art): 뺀다.

3. 턴 토큰 속도 줄: 토큰 틀 아래 테두리에 걸려 밖으로 보였다.
   위로 올려 초상 아래 모서리 배지처럼 앉힌다. (웹 편집기에서 더 다듬을 수
   있게 캔버스 자리만 옮긴다.)

Run headless:
    UnrealEditor-Cmd.exe <project> -run=pythonscript
        -script="Tools/UI/fix_combat_ui.py"
"""

import unreal
from pathlib import Path

RESULT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/fix_combat_ui.txt")
LINES = []


def say(text):
    LINES.append(text)
    unreal.log(text)


def find(tree, name):
    return unreal.find_object(None, f"{tree.get_path_name()}.{name}")


def collapse(tree, blueprint, name):
    widget = find(tree, name)
    if widget is None:
        say(f"    {name}: 없음")
        return
    blueprint.modify()
    widget.modify()
    widget.set_visibility(unreal.SlateVisibility.COLLAPSED)
    say(f"    {name}: 접음")


def canvas_box(widget):
    slot = widget.get_editor_property("slot") if widget else None
    if not isinstance(slot, unreal.CanvasPanelSlot):
        return None
    offsets = slot.get_offsets()
    align = slot.get_alignment()
    return (offsets.left - align.x * offsets.right,
            offsets.top - align.y * offsets.bottom,
            offsets.right, offsets.bottom)


def hire_cards(blueprint, tree):
    """고름 표시를 묶음 안으로. 층: 판 -> 고름표시 -> 글자 -> 버튼."""
    for index in range(8):
        mount = find(tree, f"HireCard_{index}_ArtMount")
        selected = find(tree, f"HireSelected_{index}")
        if mount is None or selected is None:
            continue
        mount_box = canvas_box(mount)
        sel_box = canvas_box(selected)
        if mount_box is None or sel_box is None:
            say(f"    HireCard_{index}: 자리 못 읽음 -- 건너뜀")
            continue

        blueprint.modify()
        mount.modify()
        selected.modify()

        # 아이들의 정렬·여백을 떠 두고 순서를 다시 짠다.
        children = list(mount.get_all_children())
        kept = []
        for child in children:
            slot = child.get_editor_property("slot")
            if isinstance(slot, unreal.OverlaySlot):
                kept.append((child, slot.get_editor_property("horizontal_alignment"),
                             slot.get_editor_property("vertical_alignment"),
                             slot.get_editor_property("padding")))
            else:
                kept.append((child, None, None, None))
        for child, _h, _v, _p in kept:
            mount.remove_child(child)
        sel_parent = selected.get_parent()
        if sel_parent is not None:
            sel_parent.remove_child(selected)

        def put_back(child, halign, valign, padding):
            mount.add_child(child)
            slot = child.get_editor_property("slot")
            if isinstance(slot, unreal.OverlaySlot) and halign is not None:
                slot.set_horizontal_alignment(halign)
                slot.set_vertical_alignment(valign)
                slot.set_padding(padding)

        # 판(첫 아이) 먼저.
        head, rest = kept[0], kept[1:]
        put_back(*head)
        # 고름 표시 -- 원래 캔버스 자리를 여백으로 그대로 재현한다.
        mount.add_child(selected)
        sel_slot = selected.get_editor_property("slot")
        if isinstance(sel_slot, unreal.OverlaySlot):
            sel_slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
            sel_slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_FILL)
            sel_slot.set_padding(unreal.Margin(
                sel_box[0] - mount_box[0], sel_box[1] - mount_box[1],
                (mount_box[0] + mount_box[2]) - (sel_box[0] + sel_box[2]),
                (mount_box[1] + mount_box[3]) - (sel_box[1] + sel_box[3])))
        # 글자들 -- 버튼은 맨 위(누름 받이)로.
        button = [item for item in rest if isinstance(item[0], unreal.Button)]
        others = [item for item in rest if not isinstance(item[0], unreal.Button)]
        for item in others:
            put_back(*item)
        for item in button:
            put_back(*item)
        say(f"    HireCard_{index}: 고름 표시를 판과 글자 사이로 ({len(others)}글자)")


def turn_speed(blueprint, tree):
    """속도 줄을 위로 -- 초상 아래 모서리 배지."""
    for index in range(12):
        icon = find(tree, f"TurnSpeedIcon_{index}")
        holder = find(tree, f"TurnSpeed_{index}_Center")
        moved = False
        for widget in (icon, holder):
            slot = widget.get_editor_property("slot") if widget else None
            if not isinstance(slot, unreal.CanvasPanelSlot):
                continue
            blueprint.modify()
            widget.modify()
            offsets = slot.get_offsets()
            if offsets.top > 90.0:
                slot.set_offsets(unreal.Margin(offsets.left, 77.0,
                                               offsets.right, offsets.bottom))
                moved = True
            slot.set_z_order(max(slot.get_z_order(), 22))
        if moved:
            say(f"    턴 토큰 {index}: 속도 줄 103 -> 77 (z>=22)")


def main():
    hire = unreal.EditorAssetLibrary.load_asset(
        "/Game/UI/CombatLayouts/WBP_MercenaryHire_Marchbound")
    tree = unreal.find_object(None, hire.get_path_name() + ":WidgetTree")
    say("WBP_MercenaryHire_Marchbound")
    hire_cards(hire, tree)
    collapse(tree, hire, "NoticeText")
    collapse(tree, hire, "NoticeText_Center")
    unreal.SystemLibrary.execute_console_command(
        None, "RD.Editor.CleanWidgetVariables /Game/UI/CombatLayouts/WBP_MercenaryHire_Marchbound")
    unreal.BlueprintEditorLibrary.compile_blueprint(hire)
    say(f"저장={unreal.EditorAssetLibrary.save_loaded_asset(hire, False)}")

    hud = unreal.EditorAssetLibrary.load_asset("/Game/UI/CombatLayouts/WBP_CombatHUD04")
    tree = unreal.find_object(None, hud.get_path_name() + ":WidgetTree")
    say("WBP_CombatHUD04")
    collapse(tree, hud, "TurnAPNumberPlate")
    collapse(tree, hud, "ArtifactStripPlate")
    collapse(tree, hud, "ArtifactStripPlateArt")
    turn_speed(hud, tree)
    unreal.SystemLibrary.execute_console_command(
        None, "RD.Editor.CleanWidgetVariables /Game/UI/CombatLayouts/WBP_CombatHUD04")
    unreal.BlueprintEditorLibrary.compile_blueprint(hud)
    say(f"저장={unreal.EditorAssetLibrary.save_loaded_asset(hud, False)}")

    # 6. 스킬 아이콘 -- 판 배선은 정상. 데이터(mIcon)가 비었는지 조사만 한다.
    say("\n스킬 데이터 mIcon 조사")
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    empty = 0
    for data in registry.get_assets_by_path("/Game", recursive=True):
        if str(data.asset_class_path.asset_name) != "StaticSkillData":
            continue
        skill = data.get_asset()
        if skill is None:
            continue
        icon = skill.get_editor_property("mIcon")
        path = str(icon.export_text()) if icon else ""
        if not path or path == "None":
            empty += 1
            say(f"    아이콘 없음: {data.package_name}")
    say(f"아이콘 빈 스킬 {empty}개")

    RESULT.parent.mkdir(parents=True, exist_ok=True)
    RESULT.write_text("\n".join(LINES) + "\n", encoding="utf-8")


main()
