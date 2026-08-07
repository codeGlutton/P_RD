"""턴 토큰 0의 속도 줄 배치를 나머지 토큰(1~9)에 복제한다.

사람이 에디터에서 토큰 0 (TurnSpeedIcon_0 · TurnSpeed_0_Center) 을 손으로
맞춰 놓았다. 그 자리·크기·z·글꼴·글자 여백을 그대로 읽어 아홉 개에 옮긴다.

토큰 0 이 도구가 마지막으로 놓은 값 그대로면(= 에디터에서 저장을 안 했으면)
복제할 것이 없으므로 멈추고 알린다.

Run headless:
    UnrealEditor-Cmd.exe <project> -run=pythonscript
        -script="Tools/UI/mirror_turn_speed.py"
"""

import unreal
from pathlib import Path

RESULT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/mirror_speed.txt")
ASSET = "/Game/UI/CombatLayouts/WBP_CombatHUD04"

# place_turn_speed.py 가 마지막으로 놓은 값. 이것과 똑같으면 사람 손이 안 닿은 것.
TOOL_PLACED = {"icon": (17.0, 102.0, 23.0, 23.0), "text": (40.0, 102.0, 51.0, 23.0)}

LINES = []


def say(text):
    LINES.append(text)
    unreal.log(text)


def find(tree, name):
    return unreal.find_object(None, f"{tree.get_path_name()}.{name}")


def slot_of(widget):
    slot = widget.get_editor_property("slot") if widget else None
    return slot if isinstance(slot, unreal.CanvasPanelSlot) else None


def read_box(slot):
    offsets = slot.get_offsets()
    return (round(float(offsets.left), 1), round(float(offsets.top), 1),
            round(float(offsets.right), 1), round(float(offsets.bottom), 1))


def main():
    blueprint = unreal.EditorAssetLibrary.load_asset(ASSET)
    tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")

    icon0 = find(tree, "TurnSpeedIcon_0")
    holder0 = find(tree, "TurnSpeed_0_Center")
    text0 = find(tree, "TurnSpeed_0")
    icon_slot0, holder_slot0 = slot_of(icon0), slot_of(holder0)
    if icon_slot0 is None or holder_slot0 is None or text0 is None:
        say("토큰 0 부품을 못 찾음 -- 중단")
        RESULT.write_text("\n".join(LINES) + "\n", encoding="utf-8")
        return

    icon_box = read_box(icon_slot0)
    text_box = read_box(holder_slot0)
    if icon_box == TOOL_PLACED["icon"] and text_box == TOOL_PLACED["text"]:
        say("토큰 0 이 도구가 놓은 값 그대로다 -- 에디터에서 저장했는지 확인할 것. 중단.")
        RESULT.write_text("\n".join(LINES) + "\n", encoding="utf-8")
        return

    icon_z = icon_slot0.get_z_order()
    holder_z = holder_slot0.get_z_order()
    font0 = text0.get_editor_property("font")
    size0 = float(font0.get_editor_property("size"))
    just0 = text0.get_editor_property("justification")
    inner0 = text0.get_editor_property("slot")
    inner_pad = inner0.get_editor_property("padding") \
        if isinstance(inner0, unreal.OverlaySlot) else None
    inner_v = inner0.get_editor_property("vertical_alignment") \
        if isinstance(inner0, unreal.OverlaySlot) else None

    say(f"토큰 0 실측: 아이콘 {icon_box} z{icon_z} · 글자칸 {text_box} z{holder_z} "
        f"· {size0:.0f}pt")

    copied = 0
    for index in range(1, 12):
        icon_slot = slot_of(find(tree, f"TurnSpeedIcon_{index}"))
        holder_slot = slot_of(find(tree, f"TurnSpeed_{index}_Center"))
        text = find(tree, f"TurnSpeed_{index}")
        if icon_slot is None or holder_slot is None or text is None:
            continue
        blueprint.modify()
        icon_slot.set_offsets(unreal.Margin(*icon_box))
        icon_slot.set_z_order(icon_z)
        holder_slot.set_offsets(unreal.Margin(*text_box))
        holder_slot.set_z_order(holder_z)
        text.modify()
        font = text.get_editor_property("font")
        font.set_editor_property("size", size0)
        text.set_editor_property("font", font)
        text.set_editor_property("justification", just0)
        inner = text.get_editor_property("slot")
        if isinstance(inner, unreal.OverlaySlot) and inner_pad is not None:
            inner.set_vertical_alignment(inner_v)
            inner.set_padding(inner_pad)
        copied += 1
    say(f"토큰 {copied}개에 복제")

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    say(f"저장={unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)}")
    RESULT.parent.mkdir(parents=True, exist_ok=True)
    RESULT.write_text("\n".join(LINES) + "\n", encoding="utf-8")


main()
