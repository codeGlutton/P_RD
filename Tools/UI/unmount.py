"""잘못 묶은 Mount 를 걷어낸다. 안엣것을 캔버스로 되돌린다.

왜 필요한가
-----------
mount_all.py 는 "글자가 어느 그림 안에 있나" 만 보고 묶는다. 그런데 판에
있는 구조를 **코드와 테스트가 붙잡고 있는 자리**가 있다 -- 턴바 토큰은
``TurnFrame_N->GetParent() == Token`` 을 테스트가 확인한다. 거기 판을 하나
끼우면 그 확인이 깨진다.

자리는 Mount 가 쥐고 있으므로, 걷어낼 때 그 자리를 안엣것에 돌려준다.
글자처럼 여백으로 자리를 잡던 것은 여백만큼 안으로 들여서 놓는다.

    UnrealEditor-Cmd.exe <project> -run=pythonscript
        -script="Tools/UI/unmount.py"
"""

import unreal
from pathlib import Path

RESULT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/unmount.txt")

# 걷어낼 것. 이름 앞머리로 고른다.
#
# TurnFrame_* -- 턴바 토큰 안. 테스트가 부모를 붙잡고 있다.
TAKE_APART = ("/Game/UI/CombatLayouts/WBP_CombatHUD04", ("TurnFrame_",))

LINES = []


def say(text):
    LINES.append(text)
    unreal.log(text)


def main():
    asset, prefixes = TAKE_APART
    blueprint = unreal.EditorAssetLibrary.load_asset(asset)
    tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")
    undone = 0

    for index in range(64):
        for prefix in prefixes:
            name = f"{prefix}{index}"
            mount = unreal.find_object(None, f"{tree.get_path_name()}.{name}Mount")
            if mount is None:
                continue
            slot = mount.get_editor_property("slot")
            if not isinstance(slot, unreal.CanvasPanelSlot):
                continue
            anchors = slot.get_anchors()
            offsets = slot.get_offsets()
            align = slot.get_alignment()
            z = slot.get_z_order()
            canvas = mount.get_parent()

            blueprint.modify()
            mount.modify()
            for child in list(mount.get_all_children()):
                pad = unreal.Margin(0.0, 0.0, 0.0, 0.0)
                child_slot = child.get_editor_property("slot")
                if isinstance(child_slot, unreal.OverlaySlot):
                    pad = child_slot.get_editor_property("padding")
                child.modify()
                mount.remove_child(child)
                canvas.add_child(child)
                back = child.get_editor_property("slot")
                if isinstance(back, unreal.CanvasPanelSlot):
                    back.set_anchors(anchors)
                    back.set_alignment(align)
                    back.set_auto_size(False)
                    back.set_z_order(z)
                    # Mount 자리에서 여백만큼 안으로. 보이는 자리는 그대로다.
                    back.set_offsets(unreal.Margin(
                        offsets.left + pad.left, offsets.top + pad.top,
                        offsets.right - pad.left - pad.right,
                        offsets.bottom - pad.top - pad.bottom))
                say(f"    {child.get_name()} <- 캔버스로 되돌림")
            canvas.remove_child(mount)
            undone += 1
            say(f"{name}Mount 걷어냄")

    unreal.SystemLibrary.execute_console_command(
        None, f"RD.Editor.CleanWidgetVariables {asset}")
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    say(f"\n걷어낸 묶음 {undone}개 · 저장="
        f"{unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)}")
    RESULT.parent.mkdir(parents=True, exist_ok=True)
    RESULT.write_text("\n".join(LINES) + "\n", encoding="utf-8")


main()
