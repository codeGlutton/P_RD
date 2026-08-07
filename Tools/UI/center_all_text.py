"""모든 글자칸을 가운데(좌우·세로)로 맞추고, 칸에 들어가는 만큼 키운다.

세로 가운데는 왜 감싸야 하나
----------------------------
TextBlock 에는 세로 정렬이 없다. Justification 은 **좌우만** 정한다. 그래서
캔버스에 정해진 높이로 깔면 글자가 늘 칸 위쪽에 붙는다 -- 339칸 중 대부분이
그랬다.

칸을 auto size 로 바꾸는 손쉬운 길이 있지만 두 군데서 깨진다.

    줄바꿈 켜진 문단   폭이 사라져 한 줄로 늘어난다
    앵커를 늘린 칸     offsets 가 크기가 아니라 여백이라 자리가 틀어진다

그래서 **Overlay 로 감싼다.** 칸은 Overlay 가 그대로 물려받고, 글자는 그
안에서 HAlign_Fill · VAlign_Center 로 놓인다. 폭이 유지되니 줄바꿈도 산다.
타이틀에서 C++ 이 런타임에 하던 것과 같은 방법이다(TitleMenuWidget.cpp:293)
-- 다만 이제 판에 박아 두므로 런타임 보정이 필요 없다.

크기는 fit_text.py 가 재 둔 값을 쓴다. **줄이지는 않는다.**

Run headless:
    python Tools/UI/fit_text.py            # 먼저 재고
    UnrealEditor-Cmd.exe <project> -run=pythonscript
        -script="Tools/UI/center_all_text.py"
"""

import json
import sys
from pathlib import Path

import unreal

ROOT = Path("D:/UnrealProjects/P_RD_develop_20260803")
sys.path.insert(0, str(ROOT / "Tools/UI"))

import wbp_names  # noqa: E402
FIT = ROOT / "Tools/UI/mockups/text_fit.json"
APART = ROOT / "Tools/UI/mockups/title_fit.json"
PADDED = ROOT / "Tools/UI/mockups/text_pad.json"
WORKSPACE = Path("D:/UnrealProjects_WBP_Editor/data/workspace.json")
RESULT = ROOT / "Saved/LegacyAudit/center_text.txt"

LINES = []


def say(text):
    LINES.append(text)
    unreal.log(text)


def wrap_in_overlay(tree, widget):
    """글자칸을 같은 자리의 Overlay 로 감싼다. 이미 감싸져 있으면 그대로.

    @return (감쌌나, 왜)
    """
    slot = widget.get_editor_property("slot")
    if isinstance(slot, unreal.OverlaySlot):
        slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
        slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_CENTER)
        return False, "이미 Overlay 안"

    parent = widget.get_parent()
    if parent is None:
        return False, "부모가 없음"

    # 캔버스가 아니면 슬롯이 세로 정렬을 스스로 들고 있다. 감쌀 것 없다.
    if not isinstance(slot, unreal.CanvasPanelSlot):
        touched = False
        for name, value in (("horizontal_alignment",
                             unreal.HorizontalAlignment.H_ALIGN_FILL),
                            ("vertical_alignment",
                             unreal.VerticalAlignment.V_ALIGN_CENTER)):
            try:
                slot.set_editor_property(name, value)
                touched = True
            except Exception:  # noqa: BLE001
                pass
        return touched, f"{type(slot).__name__} 에 정렬을 바로 넣음"

    name = f"{widget.get_name()}_Center"
    overlay = unreal.find_object(None, f"{tree.get_path_name()}.{name}")
    if overlay is None:
        overlay = unreal.new_object(unreal.Overlay, outer=tree, name=name)

    anchors = slot.get_anchors()
    offsets = slot.get_offsets()
    align = slot.get_alignment()
    z = slot.get_z_order()
    auto = slot.get_auto_size()

    parent.remove_child(widget)
    if overlay.get_parent() is None:
        parent.add_child(overlay)
    overlay_slot = overlay.get_editor_property("slot")
    if isinstance(overlay_slot, unreal.CanvasPanelSlot):
        overlay_slot.set_anchors(anchors)
        overlay_slot.set_offsets(offsets)
        overlay_slot.set_alignment(align)
        overlay_slot.set_auto_size(auto)
        overlay_slot.set_z_order(z)
    overlay.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)

    overlay.add_child(widget)
    text_slot = widget.get_editor_property("slot")
    if isinstance(text_slot, unreal.OverlaySlot):
        text_slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
        text_slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_CENTER)
    return True, "Overlay 로 감쌈"


def main():
    fits = json.loads(FIT.read_text(encoding="utf-8")) if FIT.is_file() else {}
    # 따로 잡아 둔 것은 건드리지 않는다.
    #
    # 타이틀 단추는 fit_title_buttons.py 가 **대문자를 속에, 줄 상자를 틀에**
    # 맞춰 37pt 로 정했다. 여기 규칙(글자칸에 줄 상자를 맞춤)으로 다시 재면
    # 25pt 로 되돌아간다 -- 규칙이 다른 것이지 어느 하나가 틀린 게 아니다.
    apart = set(json.loads(APART.read_text(encoding="utf-8"))) if APART.is_file() else set()
    # 잉크 가운데 + 실측 보정으로 **위쪽 정렬**을 잡아 둔 칸들. 여기서
    # 가운데 정렬로 되돌리면 그 계산이 통째로 날아간다.
    padded = (set(json.loads(PADDED.read_text(encoding="utf-8")))
              if PADDED.is_file() else set())
    # 감싼 글자는 추출본에서 빠져 있다. wbp_names 가 Overlay 이름에서 되찾는다.
    by_asset = wbp_names.text_widgets()

    touched, wrapped, resized, missed = {}, 0, 0, 0
    for asset in sorted(by_asset):
        blueprint = unreal.EditorAssetLibrary.load_asset(asset)
        if blueprint is None:
            continue
        tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")
        if tree is None:
            continue

        rows = []
        for name in sorted(by_asset[asset]):
            widget = unreal.find_object(None, f"{tree.get_path_name()}.{name}")
            if not isinstance(widget, unreal.TextBlock):
                missed += 1
                continue

            blueprint.modify()
            widget.modify()
            widget.set_editor_property("justification", unreal.TextJustify.CENTER)
            if f"{asset}/{name}" in padded:
                # 위쪽 정렬 + 계산 여백이 이미 들어 있다. 안 건드린다.
                touched[asset] = blueprint
                rows.append(f"    {name:44} 잉크 가운데로 잡아 둠(text_pad)")
                continue
            did, why = wrap_in_overlay(tree, widget)
            if did:
                wrapped += 1

            note = ""
            fit = None if name in apart else fits.get(f"{asset}/{name}")
            if name in apart:
                note = "  크기는 따로 잡아 둠(title_fit)"
            if fit is not None:
                font = widget.get_editor_property("font")
                now = float(font.get_editor_property("size"))
                want = float(fit["size"])
                if abs(want - now) >= 1.0:
                    font.set_editor_property("size", want)
                    widget.set_editor_property("font", font)
                    resized += 1
                    note = f"  {now:.0f} -> {want:.0f}pt  ({fit['why']})"
            touched[asset] = blueprint
            rows.append(f"    {name:44} {why}{note}")

        if rows:
            say(asset)
            for row in rows:
                say(row)

    world = None
    for asset, blueprint in touched.items():
        # 새로 만든 Overlay 는 변수 GUID 가 없다. 컴파일 전에 맞춰 둔다.
        unreal.SystemLibrary.execute_console_command(
            world, f"RD.Editor.CleanWidgetVariables {asset}")
        unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
        saved = unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)
        say(f"{asset.rsplit('/', 1)[-1]} 저장={saved}")

    say(f"\n감싼 칸 {wrapped}개 · 키운 칸 {resized}개 · 못 찾은 칸 {missed}개")
    RESULT.parent.mkdir(parents=True, exist_ok=True)
    RESULT.write_text("\n".join(LINES) + "\n", encoding="utf-8")


main()
