"""글자 크기의 **원본 값**을 뽑는다. git 원본 판을 물려 놓고 돌리는 용도.

"줄임 275칸" 패스가 잘라낸 크기를 되돌리려면 원래 값이 있어야 하는데,
기록(text_audit)에는 일부만 남아 있다. 판 자체가 기록이다 -- Content/UI 를
git 원본으로 잠깐 되돌린 상태에서 이걸 돌리면 전부 나온다.

    git stash push -- Content/UI
    UnrealEditor-Cmd.exe <project> -run=pythonscript -script=".../export_orig_sizes.py"
    git stash pop
"""

import json
import sys
from pathlib import Path

import unreal

ROOT = Path("D:/UnrealProjects/P_RD_develop_20260803")
sys.path.insert(0, str(ROOT / "Tools/UI"))

import wbp_names  # noqa: E402

OUT = ROOT / "Tools/UI/mockups/orig_sizes.json"


def main():
    found = {}
    for asset, names in wbp_names.text_widgets().items():
        blueprint = unreal.EditorAssetLibrary.load_asset(asset)
        if blueprint is None:
            continue
        tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")
        if tree is None:
            continue
        for name in sorted(names):
            widget = unreal.find_object(None, f"{tree.get_path_name()}.{name}")
            if isinstance(widget, unreal.TextBlock):
                font = widget.get_editor_property("font")
                found[f"{asset}/{name}"] = float(font.get_editor_property("size"))
    OUT.write_text(json.dumps(found, ensure_ascii=False, indent=1),
                   encoding="utf-8")
    unreal.log(f"원본 크기 {len(found)}개 -> {OUT}")


main()
