"""요약판(아군·적)을 정돈된 격자로 다시 놓는다.

무엇이 엉망이었나
-----------------
줄마다 왼쪽 끝이 달랐다. HP 는 178, AP 알약은 54(초상 밑), 상태 라벨은 52,
상태 아이콘은 150 -- 그래서 판 전체가 어수선하게 읽혔다.

원칙 하나로 정리한다: **초상 오른쪽(x178)부터 오른끝(x556)까지가 내용 기둥**
이고 모든 줄이 그 기둥에 맞춘다.

    줄1  배지 + 이름           (그대로 -- 이미 기둥 안이다)
    줄2  HP 판+바+숫자         (그대로 -- 기준 줄)
    줄3  AP 알약 | 속도 칩      54,316 -> 178,372 · 폭 184 씩 대칭
    줄4  상태 라벨 ... 상태 글   178 / 오른끝 정렬
    줄5  상태 아이콘 3칸        178부터 84 간격
    줄6  요약/예상피해 배너      (그대로 -- 가운데)

이름은 안 바꾸므로 C++ 배선은 그대로 산다.

Run headless:
    UnrealEditor-Cmd.exe <project> -run=pythonscript
        -script="Tools/UI/redesign_summary_panels.py"
"""

import unreal
from pathlib import Path

RESULT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/redesign_summary.txt")
PANEL_W, PANEL_H = 600.0, 430.0

LINES = []


def say(text):
    LINES.append(text)
    unreal.log(text)


def find(tree, name):
    return unreal.find_object(None, f"{tree.get_path_name()}.{name}")


def place_canvas(tree, blueprint, name, x, y, w, h):
    widget = find(tree, name)
    slot = widget.get_editor_property("slot") if widget else None
    if not isinstance(slot, unreal.CanvasPanelSlot):
        say(f"    {name}: 캔버스 아님/없음")
        return
    blueprint.modify()
    widget.modify()
    slot.set_alignment(unreal.Vector2D(0.0, 0.0))
    slot.set_auto_size(False)
    slot.set_offsets(unreal.Margin(x, y, w, h))


def place_in_plate(tree, blueprint, name, x, y, w, h):
    """판 묶음(600x430) 안 _Center 의 자리를 여백으로 잡는다."""
    widget = find(tree, name)
    slot = widget.get_editor_property("slot") if widget else None
    if not isinstance(slot, unreal.OverlaySlot):
        say(f"    {name}: Overlay 아님/없음")
        return
    blueprint.modify()
    widget.modify()
    slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
    slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_FILL)
    slot.set_padding(unreal.Margin(x, y, PANEL_W - x - w, PANEL_H - y - h))


def pad_inside(tree, blueprint, name, left, top, right, bottom):
    widget = find(tree, name)
    slot = widget.get_editor_property("slot") if widget else None
    if not isinstance(slot, unreal.OverlaySlot):
        say(f"    {name}: Overlay 아님/없음")
        return
    blueprint.modify()
    widget.modify()
    slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
    slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_FILL)
    slot.set_padding(unreal.Margin(left, top, right, bottom))


def text_top(tree, blueprint, name, pad_top, justify=None):
    widget = find(tree, name)
    if not isinstance(widget, unreal.TextBlock):
        return
    blueprint.modify()
    widget.modify()
    if justify is not None:
        widget.set_editor_property("justification", justify)
    slot = widget.get_editor_property("slot")
    if isinstance(slot, unreal.OverlaySlot):
        slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
        slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_TOP)
        slot.set_padding(unreal.Margin(0.0, pad_top, 0.0, 0.0))


def panel(tree, blueprint, p):
    say(f"  == {p} ==")
    # 줄3: AP · 속도 -- HP 와 같은 기둥, 폭 184 대칭.
    place_canvas(tree, blueprint, f"{p}APPlateMount", 178, 180, 184, 58)
    place_canvas(tree, blueprint, f"{p}SpeedPlateMount", 372, 180, 184, 58)
    place_canvas(tree, blueprint, f"{p}SpeedIcon", 380, 187, 44, 44)
    pad_inside(tree, blueprint, f"{p}APText_Center", 12.0, 6.0, 12.0, 6.0)
    # 속도 글자는 아이콘(380..424) 오른쪽부터.
    pad_inside(tree, blueprint, f"{p}SpeedText_Center", 56.0, 6.0, 10.0, 6.0)

    # 줄4: 상태 라벨(왼쪽) / 상태 글(오른끝).
    place_in_plate(tree, blueprint, f"{p}StatusLabel_Center", 178, 244, 110, 40)
    place_in_plate(tree, blueprint, f"{p}Status_Center", 300, 244, 256, 40)
    text_top(tree, blueprint, f"{p}StatusLabel", 2.0, unreal.TextJustify.LEFT)
    text_top(tree, blueprint, f"{p}Status", 2.0, unreal.TextJustify.RIGHT)

    # 줄5: 상태 아이콘 3칸, 84 간격.
    for index in range(3):
        place_canvas(tree, blueprint, f"{p}StatusFrame_{index}Mount",
                     178 + 84 * index, 290, 76, 76)
        place_canvas(tree, blueprint, f"{p}StatusIcon_{index}",
                     186 + 84 * index, 298, 60, 60)
    say("    줄3~5 를 x178 기둥에 정렬")


def main():
    hud = unreal.EditorAssetLibrary.load_asset("/Game/UI/CombatLayouts/WBP_CombatHUD04")
    tree = unreal.find_object(None, hud.get_path_name() + ":WidgetTree")
    say("WBP_CombatHUD04")
    panel(tree, hud, "Ally")
    panel(tree, hud, "Enemy")
    unreal.SystemLibrary.execute_console_command(
        None, "RD.Editor.CleanWidgetVariables /Game/UI/CombatLayouts/WBP_CombatHUD04")
    unreal.BlueprintEditorLibrary.compile_blueprint(hud)
    say(f"저장={unreal.EditorAssetLibrary.save_loaded_asset(hud, False)}")
    RESULT.parent.mkdir(parents=True, exist_ok=True)
    RESULT.write_text("\n".join(LINES) + "\n", encoding="utf-8")


main()
