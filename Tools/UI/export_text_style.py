"""글자칸의 지금 모습을 뽑는다. 갤러리가 진짜대로 그리게.

왜 따로 뽑나
------------
WBP 편집기 추출본(workspace.json)에는 자리와 글만 있고 **글꼴·크기·색·정렬이
없다.** 그래서 갤러리가 글자를 다 같은 모양으로 그리고 있었다 -- 그 화면에서
"이 크기면 되겠다" 를 판단할 수가 없다.

여기서 뽑은 값은 두 군데 쓴다.

    갤러리   지금 모습 그대로 그리고, 바꿔 보는 자리의 시작값이 된다
    되돌리기 사람이 바꾼 뒤 원래대로 돌릴 때 견줄 값

Run headless:
    UnrealEditor-Cmd.exe <project> -run=pythonscript
        -script="Tools/UI/export_text_style.py"
"""

import json
import sys
from pathlib import Path

import unreal

ROOT = Path("D:/UnrealProjects/P_RD_develop_20260803")
sys.path.insert(0, str(ROOT / "Tools/UI"))

import wbp_names  # noqa: E402

OUT = ROOT / "Tools/UI/mockups/text_style.json"

# 고를 수 있는 글꼴. 갤러리 고르개도 이 목록을 쓴다.
FONTS = {
    "F_HUD_Oswald": "/Game/SVN/OutSideAsset/Fonts/F_HUD_Oswald.F_HUD_Oswald",
    "F_HUD_LINESeedKR":
        "/Game/SVN/OutSideAsset/Fonts/F_HUD_LINESeedKR.F_HUD_LINESeedKR",
    "F_HUD_NotoSansKR":
        "/Game/SVN/OutSideAsset/Fonts/F_HUD_NotoSansKR.F_HUD_NotoSansKR",
    "Roboto": "/Engine/EngineFonts/Roboto.Roboto",
}


def enum_name(value):
    return str(value).split(".")[-1].split(":")[0].strip()


def hexed(colour):
    def byte(v):
        return max(0, min(255, int(round(float(v) * 255.0))))
    return f"#{byte(colour.r):02x}{byte(colour.g):02x}{byte(colour.b):02x}"


def rgba(colour):
    def byte(v):
        return max(0, min(255, int(round(float(v) * 255.0))))
    return [byte(colour.r), byte(colour.g), byte(colour.b),
            round(float(colour.a), 3)]


def shadow_of(widget):
    """글자 그림자. 슬레이트 그림자는 번짐이 없는 **한 번 밀어 그린 것**이다."""
    offset = widget.get_editor_property("shadow_offset")
    colour = widget.get_editor_property("shadow_color_and_opacity")
    return {"x": round(float(offset.x), 2), "y": round(float(offset.y), 2),
            "color": rgba(colour)}


def outline_of(widget):
    """글자 테두리. 0 이면 없는 것이다."""
    settings = widget.get_editor_property("font").get_editor_property(
        "outline_settings")
    return {"size": int(settings.get_editor_property("outline_size")),
            "color": rgba(settings.get_editor_property("outline_color")),
            # 테두리를 그림자에도 입혔나. 은은한 그림자를 만드는 유일한 손잡이다.
            "drop": bool(settings.get_editor_property(
                "apply_outline_to_drop_shadows"))}


def main():
    # 이름은 wbp_names 가 되찾아 준다. 감싼 글자는 추출본에서 빠져 있어
    # TextBlock 만 훑으면 하나도 안 나온다.
    by_asset = wbp_names.text_widgets()

    styles = {}
    for asset in sorted(by_asset):
        blueprint = unreal.EditorAssetLibrary.load_asset(asset)
        if blueprint is None:
            continue
        tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")
        if tree is None:
            continue
        for name in sorted(by_asset[asset]):
            widget = unreal.find_object(None, f"{tree.get_path_name()}.{name}")
            if not isinstance(widget, unreal.TextBlock):
                continue

            font = widget.get_editor_property("font")
            face = font.get_editor_property("font_object")
            colour = widget.get_editor_property("color_and_opacity")

            try:
                shown = str(widget.get_text())
            except Exception:  # noqa: BLE001
                shown = ""
            entry = {
                # 글도 같이 들고 나온다. Overlay 로 감싼 뒤로 추출본에서
                # 글자칸이 통째로 빠져(캔버스 직계가 아니다) 갤러리가 무엇을
                # 쓸지 알 길이 없어졌다.
                "text": shown,
                "font": face.get_name() if face else "Roboto",
                "typeface": str(font.get_editor_property("typeface_font_name")),
                "size": float(font.get_editor_property("size")),
                "color": hexed(colour.get_editor_property("specified_color")),
                "just": enum_name(widget.get_editor_property("justification")),
                # 그림자와 테두리도 같이. 갤러리가 이걸 모르면 제멋대로
                # 그림자를 넣어 "웹에는 있는데 게임에는 없는" 차이가 생긴다.
                "shadow": shadow_of(widget),
                "outline": outline_of(widget),
                # 줄바꿈이 켜져 있으면 문단이다. 크기를 키우면 안 되고, 칸을
                # auto 로 바꾸면 접히지 않아 한 줄로 늘어난다.
                "wrap": bool(widget.get_editor_property("auto_wrap_text")),
            }

            slot = widget.get_editor_property("slot")
            # 가운데 맞추려고 Overlay 로 감싼 것들이 있다. 자리를 쥔 것은
            # 그 Overlay 이므로 한 겹 올라가서 읽는다 -- 안 그러면 칸 크기가
            # 없어서 "칸 ?x?" 로 나오고, 크기 재기도 못 한다.
            if isinstance(slot, unreal.OverlaySlot):
                # **정렬과 여백을 그대로 들고 나온다.**
                #
                # 갤러리가 "감쌌으면 가운데" 로 넘겨짚고 있었는데, 지금은
                # 위쪽 정렬 + 계산된 여백으로 놓는 자리가 있어 그리는 모습이
                # 게임과 달라졌다. 넘겨짚지 말고 적힌 대로 그려야 한다.
                pad = slot.get_editor_property("padding")
                entry["valign"] = enum_name(
                    slot.get_editor_property("vertical_alignment"))
                entry["halign"] = enum_name(
                    slot.get_editor_property("horizontal_alignment"))
                entry["pad"] = [round(float(pad.left), 1), round(float(pad.top), 1),
                                round(float(pad.right), 1), round(float(pad.bottom), 1)]
                holder = widget.get_parent()
                if holder is not None:
                    slot = holder.get_editor_property("slot")
                    entry["wrapped"] = True
            if isinstance(slot, unreal.CanvasPanelSlot):
                align = slot.get_alignment()
                offsets = slot.get_offsets()
                anchors = slot.get_anchors()
                entry["auto"] = bool(slot.get_auto_size())
                entry["align"] = [round(align.x, 3), round(align.y, 3)]
                entry["box"] = [round(offsets.right, 1), round(offsets.bottom, 1)]
                # 앵커를 늘려 놓았으면 offsets 는 크기가 아니라 안쪽 여백이다.
                # 그런 칸의 box 값은 크기로 쓰면 안 된다.
                entry["stretched"] = (anchors.minimum.x != anchors.maximum.x
                                      or anchors.minimum.y != anchors.maximum.y)
            styles[f"{asset}/{name}"] = entry

    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps({"fonts": sorted(FONTS), "styles": styles},
                              ensure_ascii=False, indent=1), encoding="utf-8")
    unreal.log(f"글자칸 {len(styles)}개를 {OUT} 에 적음")


main()
