"""틀·글자·버튼을 **한 판(Mount)에 묶는다.**

무엇이 문제였나
---------------
지금은 셋이 캔버스에 따로따로 절대 좌표로 놓여 있다. 서로 아무 끈이 없어서

    틀에 다른 그림을 물리면      글자칸은 안 따라간다
    그어 둔 칸을 다시 그으면     글자칸은 안 따라간다
    틀을 옮기거나 크기를 바꾸면  글자칸은 안 따라간다

매번 도구를 돌려 다시 맞춰야 했다. 그 대가를 C++ 도 치르고 있다 --
CONTINUE 를 숨기면 리플로우가 안 돼서 프레임·버튼·글자 **셋을 따로** 옮긴다
(TitleMenuWidget_Flow.cpp:208).

어떻게 묶나
-----------
    캔버스
    └ XxxMount (Overlay)            <- 자리를 여기 하나가 쥔다
       ├ XxxFrameImage (Image)      Fill / Fill
       ├ XxxText (TextBlock)        Fill / Center · 좌우 여백 = 그어 둔 칸
       └ XxxButton (Button)         Fill / Fill      <- 맨 위라 누름을 받는다

이러면 Mount 하나만 옮기면 셋이 같이 가고, 하나만 숨기면 셋이 같이 숨는다.
이름은 그대로라 C++ 의 이름 조회(GetWidgetFromName·FindDeep)는 안 깨진다.

세로 여백은 안 준다
-------------------
글자 줄 상자가 칸보다 커지면 슬레이트가 세로 가운데를 버리고 위에 붙인다.
그래서 세로는 틀 높이를 다 쓰고, **눈에 보이는 크기만** 그어 둔 칸에 맞춘다
(fit_title_buttons.py 가 그 크기를 잰다).

Run headless:
    UnrealEditor-Cmd.exe <project> -run=pythonscript
        -script="Tools/UI/mount_text_to_frame.py"
"""

import json
from pathlib import Path

import unreal

ROOT = Path("D:/UnrealProjects/P_RD_develop_20260803")
MOCKUPS = ROOT / "Tools/UI/mockups"
RESULT = ROOT / "Saved/LegacyAudit/mount_text.txt"

ASSET = "/Game/UI/WBP_TitleMenu"
# (묶음 이름, 글자칸, 틀칸, 누름칸)
GROUPS = [
    ("StartButtonMount__base_16_9", "StartButtonText__base_16_9",
     "StartButtonFrameImage__base_16_9", "StartButton__base_16_9"),
    ("ContinueButtonMount__base_16_9", "ContinueButtonText__base_16_9",
     "ContinueButtonFrameImage__base_16_9", "ContinueButton__base_16_9"),
    ("SettingsButtonMount__base_16_9", "SettingsButtonText__base_16_9",
     "SettingsButtonFrameImage__base_16_9", "SettingsButton__base_16_9"),
    ("ExitButtonMount__base_16_9", "ExitButtonText__base_16_9",
     "ExitButtonFrameImage__base_16_9", None),
    ("VersionMount__base_16_9", "VersionText__base_16_9",
     "VersionPlateImage__base_16_9", None),
]

FILL = unreal.HorizontalAlignment.H_ALIGN_FILL
VFILL = unreal.VerticalAlignment.V_ALIGN_FILL
VCENTER = unreal.VerticalAlignment.V_ALIGN_CENTER

LINES = []


def say(text):
    LINES.append(text)
    unreal.log(text)


def find(tree, name):
    return unreal.find_object(None, f"{tree.get_path_name()}.{name}") if name else None


def drawn_region(texture):
    """그림에 그어 둔 칸 (좌, 상, 우, 하) 비율. 없으면 None."""
    if texture is None:
        return None
    book = json.loads((MOCKUPS / "rects_user.json").read_text(encoding="utf-8"))
    for mark in book.get(texture.get_name(), []):
        rect = mark.get("inner") or mark.get("rect") or mark.get("box")
        if rect and len(rect) == 4:
            return [float(v) for v in rect]
    return None


def slot_box(widget):
    """캔버스 슬롯의 (앵커, offsets, alignment, z). 아니면 None."""
    slot = widget.get_editor_property("slot") if widget else None
    if not isinstance(slot, unreal.CanvasPanelSlot):
        return None
    return (slot.get_anchors(), slot.get_offsets(), slot.get_alignment(),
            slot.get_z_order())


def put(overlay, widget, halign, valign, padding=None):
    parent = widget.get_parent()
    if parent is not None and parent != overlay:
        parent.remove_child(widget)
    if widget.get_parent() != overlay:
        overlay.add_child(widget)
    slot = widget.get_editor_property("slot")
    if isinstance(slot, unreal.OverlaySlot):
        slot.set_horizontal_alignment(halign)
        slot.set_vertical_alignment(valign)
        if padding is not None:
            slot.set_padding(padding)


def main():
    blueprint = unreal.EditorAssetLibrary.load_asset(ASSET)
    tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")
    made = 0

    for mount_name, text_name, frame_name, button_name in GROUPS:
        text = find(tree, text_name)
        frame = find(tree, frame_name)
        button = find(tree, button_name)
        if text is None or frame is None:
            say(f"{mount_name}: {'글자' if text is None else '틀'} 없음 -- 건너뜀")
            continue

        # 자리는 **틀**이 쥐고 있던 것을 그대로 물려받는다.
        box = slot_box(frame)
        if box is None:
            say(f"{mount_name}: 틀이 캔버스에 없음 -- 건너뜀")
            continue
        anchors, offsets, align, z = box
        canvas = frame.get_parent()

        mount = find(tree, mount_name)
        if mount is None:
            mount = unreal.new_object(unreal.Overlay, outer=tree, name=mount_name)
        blueprint.modify()
        mount.modify()

        if mount.get_parent() is None:
            canvas.add_child(mount)
        mount_slot = mount.get_editor_property("slot")
        if isinstance(mount_slot, unreal.CanvasPanelSlot):
            mount_slot.set_anchors(anchors)
            mount_slot.set_offsets(offsets)
            mount_slot.set_alignment(align)
            mount_slot.set_auto_size(False)
            mount_slot.set_z_order(z)
        # 묶음 자체는 누름을 안 먹는다. 안의 버튼이 받는다.
        mount.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)

        # 그어 둔 칸만큼 **좌우로만** 들인다. 세로는 틀 높이를 다 쓴다.
        brush = frame.get_editor_property("brush")
        region = drawn_region(brush.get_editor_property("resource_object"))
        pad = unreal.Margin(0.0, 0.0, 0.0, 0.0)
        note = "좌우 여백 없음"
        if region:
            width = float(offsets.right)
            left = region[0] * width
            right = (1.0 - region[2]) * width
            if left + right < width * 0.8:
                pad = unreal.Margin(left, 0.0, right, 0.0)
                note = f"좌우 여백 {left:.0f}/{right:.0f} (그어 둔 칸)"

        # 넣는 차례가 곧 위아래다. 버튼이 마지막이라 맨 위에서 누름을 받는다.
        frame.modify(); text.modify()
        put(mount, frame, FILL, VFILL)
        put(mount, text, FILL, VCENTER, pad)
        if button is not None:
            button.modify()
            put(mount, button, FILL, VFILL)

        # 세로 가운데를 주려고 씌웠던 판은 이제 필요 없다. Mount 가 한다.
        old = find(tree, f"{text_name}_Center")
        if old is not None and old.get_children_count() == 0:
            old_parent = old.get_parent()
            if old_parent is not None:
                old_parent.remove_child(old)
            say(f"    {text_name}_Center 걷어냄")

        made += 1
        say(f"{mount_name}  <- 틀·글자{'·버튼' if button else ''} 묶음")
        say(f"    자리 {offsets.right:.0f}x{offsets.bottom:.0f} · {note}")

    unreal.SystemLibrary.execute_console_command(
        None, f"RD.Editor.CleanWidgetVariables {ASSET}")
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    say(f"\n묶음 {made}개 · 저장="
        f"{unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)}")
    RESULT.parent.mkdir(parents=True, exist_ok=True)
    RESULT.write_text("\n".join(LINES) + "\n", encoding="utf-8")


main()
