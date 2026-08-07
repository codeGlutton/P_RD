"""compute_text_centering 이 계산한 여백을 판에 넣는다.

글자를 감싼 X_Center 판 안에서 **위쪽 정렬 + 여백**으로 놓는다. 가운데
정렬은 줄 상자 기준이라 잉크가 내려앉고, 넘칠 때는 조용히 위로 붙어
버린다 -- 위쪽 정렬은 넘치든 말든 자리가 딱 정해진다.

Run headless:
    python Tools/UI/compute_text_centering.py     # 먼저 계산
    UnrealEditor-Cmd.exe <project> -run=pythonscript
        -script="Tools/UI/apply_text_centering.py"
"""

import json
from pathlib import Path

import unreal

ROOT = Path("D:/UnrealProjects/P_RD_develop_20260803")
PLAN = json.loads((ROOT / "Tools/UI/mockups/text_pad.json").read_text(
    encoding="utf-8"))
RESULT = ROOT / "Saved/LegacyAudit/text_pad.txt"

LINES = []


def say(text):
    LINES.append(text)
    unreal.log(text)


def main():
    by_asset = {}
    for key, item in PLAN.items():
        asset, _sep, name = key.rpartition("/")
        by_asset.setdefault(asset, []).append((name, item))

    touched = 0
    for asset in sorted(by_asset):
        blueprint = unreal.EditorAssetLibrary.load_asset(asset)
        if blueprint is None:
            say(f"{asset}: 판 없음")
            continue
        tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")
        done = 0
        for name, item in sorted(by_asset[asset]):
            widget = unreal.find_object(None, f"{tree.get_path_name()}.{name}")
            if not isinstance(widget, unreal.TextBlock):
                say(f"    {name}: 글자칸 아님")
                continue
            slot = widget.get_editor_property("slot")
            if not isinstance(slot, unreal.OverlaySlot):
                say(f"    {name}: Overlay 안이 아님 -- 건너뜀")
                continue
            blueprint.modify()
            widget.modify()
            pad = slot.get_editor_property("padding")
            slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
            slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_TOP)
            slot.set_padding(unreal.Margin(float(pad.left),
                                           float(item["padTop"]),
                                           float(pad.right), 0.0))
            done += 1
        if done:
            unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
            saved = unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)
            say(f"{asset.rsplit('/', 1)[-1]}  {done}칸 · 저장={saved}")
            touched += done

    say(f"\n모두 {touched}칸")
    RESULT.parent.mkdir(parents=True, exist_ok=True)
    RESULT.write_text("\n".join(LINES) + "\n", encoding="utf-8")


main()
