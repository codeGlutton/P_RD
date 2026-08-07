""""줄임 275칸" 패스가 잘라낸 글자 크기를 원본으로 되돌린다.

왜 되돌리나
-----------
그 패스는 "칸을 넘치면 잘못" 이라고 봤는데, 전투 UI 는 **작은 칸에 큰
글자를 일부러 넘치게** 넣은 디자인이었다 -- 진짜 기준은 칸이 아니라 뒤의
판 그림이다. TurnAPText 가 20 -> 10pt 로 반토막 나 AP 숫자가 안 보이게 된
것이 대표 피해다.

규칙: **지금 값이 원본보다 작으면 원본으로.** 키운 것(타이틀 37pt 등)은
의도한 것이므로 안 건드린다.

원본 값은 export_orig_sizes.py 가 git 원본 판에서 뽑아 둔 것이다.

Run headless:
    UnrealEditor-Cmd.exe <project> -run=pythonscript
        -script="Tools/UI/restore_shrunk_text.py"
"""

import json
from pathlib import Path

import unreal

ROOT = Path("D:/UnrealProjects/P_RD_develop_20260803")
ORIG = json.loads((ROOT / "Tools/UI/mockups/orig_sizes.json").read_text(
    encoding="utf-8"))
RESULT = ROOT / "Saved/LegacyAudit/restore_shrunk.txt"

LINES = []


def say(text):
    LINES.append(text)
    unreal.log(text)


def main():
    by_asset = {}
    for key, size in ORIG.items():
        asset, _sep, name = key.rpartition("/")
        by_asset.setdefault(asset, []).append((name, float(size)))

    restored = kept = 0
    for asset in sorted(by_asset):
        blueprint = unreal.EditorAssetLibrary.load_asset(asset)
        if blueprint is None:
            continue
        tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")
        rows = []
        for name, orig in sorted(by_asset[asset]):
            widget = unreal.find_object(None, f"{tree.get_path_name()}.{name}")
            if not isinstance(widget, unreal.TextBlock):
                continue
            font = widget.get_editor_property("font")
            now = float(font.get_editor_property("size"))
            if now >= orig - 0.5:
                kept += 1
                continue
            blueprint.modify()
            widget.modify()
            font.set_editor_property("size", orig)
            widget.set_editor_property("font", font)
            restored += 1
            rows.append(f"    {name:44} {now:.0f} -> {orig:.0f}pt")
        if rows:
            say(asset)
            for row in rows:
                say(row)
            unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
            saved = unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)
            say(f"{asset.rsplit('/', 1)[-1]} 저장={saved}")

    say(f"\n되돌림 {restored}개 · 그대로 {kept}개")
    RESULT.parent.mkdir(parents=True, exist_ok=True)
    RESULT.write_text("\n".join(LINES) + "\n", encoding="utf-8")


main()
