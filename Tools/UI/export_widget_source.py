"""위젯마다 **지금 물려 있는 그림**을 뽑는다.

왜 따로 뽑나
------------
갤러리는 그림 이름을 WBP 편집기 추출본(workspace.json)에서 읽는데, 그건
추출한 그 때의 값이다. 그 뒤에 그림을 물리면 갤러리는 여전히 "붙어 있는
그림 없음" 이라고 말한다 -- 타이틀 단추에 판을 물려 놓고도 그랬다.

여기서 뽑은 값은 갤러리가 추출본 위에 덮어 쓴다. 추출을 다시 돌릴 필요가
없고, 물린 직후 바로 맞는다.

Button 은 스타일에 브러시가 있다
--------------------------------
Image 는 brush 하나지만 Button 은 normal/hovered/pressed/disabled 넷이다.
평소 모습인 normal 을 본다. 타이틀 단추처럼 칠을 지운 것은 그림이 없다고
나오는데, 그게 맞다 -- 그림은 밑의 FrameImage 가 들고 있다.

Run headless:
    UnrealEditor-Cmd.exe <project> -run=pythonscript
        -script="Tools/UI/export_widget_source.py"
"""

import json
from pathlib import Path

import unreal

ROOT = Path("D:/UnrealProjects/P_RD_develop_20260803")
WORKSPACE = Path("D:/UnrealProjects_WBP_Editor/data/workspace.json")
OUT = ROOT / "Tools/UI/mockups/widget_src.json"


def brush_of(widget):
    """이 위젯이 평소에 그리는 브러시. 없으면 None."""
    if isinstance(widget, unreal.Image):
        return widget.get_editor_property("brush")
    if isinstance(widget, unreal.Border):
        return widget.get_editor_property("background")
    if isinstance(widget, unreal.Button):
        return widget.get_editor_property("widget_style").get_editor_property(
            "normal")
    return None


def main():
    data = json.loads(WORKSPACE.read_text(encoding="utf-8"))
    by_asset = {}
    for document in data.get("documents", []):
        if document.get("sourceKind") != "current-develop-wbp":
            continue
        for widget in document.get("widgets", []):
            by_asset.setdefault(document.get("assetPath", "?"), set()).add(
                widget["name"])

    found = {}
    for asset in sorted(by_asset):
        blueprint = unreal.EditorAssetLibrary.load_asset(asset)
        if blueprint is None:
            continue
        tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")
        if tree is None:
            continue
        for name in sorted(by_asset[asset]):
            widget = unreal.find_object(None, f"{tree.get_path_name()}.{name}")
            if widget is None:
                continue
            brush = brush_of(widget)
            if brush is None:
                continue
            texture = brush.get_editor_property("resource_object")
            draw = str(brush.get_editor_property("draw_as")).split(
                ".")[-1].split(":")[0].strip()
            entry = {"draw": draw}
            if texture is not None:
                # 경로에서 ".이름" 꼬리를 뗀다. 갤러리가 그대로 보여 준다.
                entry["src"] = texture.get_path_name().split(".")[0]
                # 9-slice 를 실제처럼 그리려면 **원본 크기와 여백 비율**이
                # 둘 다 있어야 한다. 테두리 두께 = 여백비율 x 원본크기 이고,
                # 이 값은 칸이 커져도 안 변한다(ElementBatcher.cpp:857).
                entry["size"] = [int(texture.blueprint_get_size_x()),
                                 int(texture.blueprint_get_size_y())]
                margin = brush.get_editor_property("margin")
                entry["margin"] = [round(float(margin.left), 5),
                                   round(float(margin.top), 5),
                                   round(float(margin.right), 5),
                                   round(float(margin.bottom), 5)]
            found[f"{asset}/{name}"] = entry

    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(found, ensure_ascii=False, indent=1),
                   encoding="utf-8")
    with_art = sum(1 for e in found.values() if e.get("src"))
    unreal.log(f"위젯 {len(found)}개 (그림 물린 것 {with_art}개) -> {OUT}")


main()
