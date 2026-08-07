"""추출본에서 **글자칸 이름**을 되찾는다.

왜 따로 두나
------------
글자를 세로 가운데로 놓으려고 Overlay 로 감쌌더니, WBP 편집기 추출기가
캔버스 직계만 담는 탓에 TextBlock 339개가 추출본에서 통째로 빠졌다. 남은
것은 이름이 ``X_Center`` 인 빈 Overlay 뿐이다.

이름을 추출본의 TextBlock 에서만 읽던 도구들이 한꺼번에 아무것도 못 찾게
됐다 -- 글꼴 뽑기 · 크기 재기 · 가운데 맞추기가 전부 0개로 돌았다. 되찾는
규칙을 한 곳에만 둔다.
"""

import json
from pathlib import Path

WORKSPACE = Path("D:/UnrealProjects_WBP_Editor/data/workspace.json")
WRAP_SUFFIX = "_Center"
TEXT_CLASSES = ("TextBlock", "RichTextBlock")


def real_name(widget):
    """이 줄이 가리키는 **진짜 글자칸 이름**. 글자칸이 아니면 빈 글.

    추출기가 감싼 알맹이를 겉으로 올려 주면서 줄의 이름은 감싼 판이고
    종류·글은 알맹이 것이 된다. 그래서 판에서 찾을 이름은 ``holds`` 다.
    그 값이 없는 옛 추출본을 위해 이름 꼬리로도 되찾는다.
    """
    if widget.get("className") not in TEXT_CLASSES + ("Overlay",):
        return ""
    holds = widget.get("holds")
    if holds:
        return holds if widget.get("className") in TEXT_CLASSES else ""
    name = widget.get("name", "")
    if widget.get("className") in TEXT_CLASSES:
        return name
    return name[:-len(WRAP_SUFFIX)] if name.endswith(WRAP_SUFFIX) else ""


def text_widgets(workspace=None):
    """에셋 -> 글자칸 이름 모음.

    추출본에 TextBlock 이 그대로 있으면 그것을, 감싸여 사라졌으면 감싼
    Overlay 이름에서 꼬리를 떼어 되찾는다. 둘 다 모은다 -- 어떤 화면은
    감싸여 있고 어떤 화면은 아직 아닐 수 있다.
    """
    path = Path(workspace) if workspace else WORKSPACE
    data = json.loads(path.read_text(encoding="utf-8"))
    found = {}
    for document in data.get("documents", []):
        if document.get("sourceKind") != "current-develop-wbp":
            continue
        asset = document.get("assetPath", "?")
        for widget in document.get("widgets", []):
            real = real_name(widget)
            if real:
                found.setdefault(asset, set()).add(real)
    return found


def boxes(workspace=None):
    """에셋/위젯 -> 그려질 자리 (w, h).

    감싼 글자는 자기 자리가 없다. 감싼 Overlay 의 자리가 곧 글자칸이다.
    """
    path = Path(workspace) if workspace else WORKSPACE
    data = json.loads(path.read_text(encoding="utf-8"))
    found = {}
    for document in data.get("documents", []):
        if document.get("sourceKind") != "current-develop-wbp":
            continue
        asset = document.get("assetPath", "?")
        for widget in document.get("widgets", []):
            real = real_name(widget)
            if not real:
                continue
            rect = widget.get("rect") or {}
            found[f"{asset}/{real}"] = (float(rect.get("w", 0.0)),
                                        float(rect.get("h", 0.0)))
    return found
