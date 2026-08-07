"""턴 토큰 속도 줄(아이콘+숫자)을 **그림에 그어 둔 칸**에 앉힌다.

쓰는 법
-------
1. assets.html 에서 ``T_MB_TurnToken_Frame`` 을 열어 속도 줄이 앉을 칸을
   **하나** 그리고 `디스크에 저장`.
2. 이 스크립트를 돌린다. 토큰 10개의 아이콘·숫자가 그 칸에 맞춰 앉는다.

칸 안 배치: 아이콘은 왼쪽에 정사각(칸 높이만큼), 숫자는 나머지를 채운다.

토큰 틀은 108x144 자리에 IMAGE(통짜 늘림)로 그려지므로 칸 비율을 자리
크기에 그대로 곱하면 된다.

Run headless:
    UnrealEditor-Cmd.exe <project> -run=pythonscript
        -script="Tools/UI/place_turn_speed.py"
"""

import json
from pathlib import Path

import unreal

ROOT = Path("D:/UnrealProjects/P_RD_develop_20260803")
ART = "T_MB_TurnToken_Frame"
ASSET = "/Game/UI/CombatLayouts/WBP_CombatHUD04"
TOKEN_W, TOKEN_H = 108.0, 144.0

RESULT = ROOT / "Saved/LegacyAudit/place_turn_speed.txt"
LINES = []


def say(text):
    LINES.append(text)
    unreal.log(text)


def main():
    book = json.loads((ROOT / "Tools/UI/mockups/rects_user.json").read_text(
        encoding="utf-8"))
    # 칸이 여럿이면 **가장 아래 것**이 속도 줄이다. 위 칸은 초상 창이다.
    region = None
    for mark in book.get(ART, []):
        rect = mark.get("inner") or mark.get("rect") or mark.get("box")
        if rect and len(rect) == 4:
            if region is None or (rect[1] + rect[3]) > (region[1] + region[3]):
                region = rect
    if not region:
        say(f"{ART} 에 그어 둔 칸이 없다. assets.html 에서 먼저 긋고 저장할 것.")
        RESULT.write_text("\n".join(LINES) + "\n", encoding="utf-8")
        return

    left = region[0] * TOKEN_W
    top = region[1] * TOKEN_H
    width = (region[2] - region[0]) * TOKEN_W
    height = (region[3] - region[1]) * TOKEN_H
    say(f"그어 둔 칸 -> 토큰 자리 {left:.0f},{top:.0f} {width:.0f}x{height:.0f}")

    blueprint = unreal.EditorAssetLibrary.load_asset(ASSET)
    tree = unreal.find_object(None, blueprint.get_path_name() + ":WidgetTree")
    done = 0
    for index in range(12):
        icon = unreal.find_object(None, f"{tree.get_path_name()}.TurnSpeedIcon_{index}")
        holder = unreal.find_object(None, f"{tree.get_path_name()}.TurnSpeed_{index}_Center")
        if icon is None or holder is None:
            continue
        icon_slot = icon.get_editor_property("slot")
        text_slot = holder.get_editor_property("slot")
        if not isinstance(icon_slot, unreal.CanvasPanelSlot) \
                or not isinstance(text_slot, unreal.CanvasPanelSlot):
            continue
        blueprint.modify()
        icon.modify()
        holder.modify()
        # 아이콘: 칸 왼쪽 정사각.
        icon_slot.set_offsets(unreal.Margin(left, top, height, height))
        icon_slot.set_z_order(22)
        # 숫자: 나머지.
        text_slot.set_offsets(unreal.Margin(left + height, top,
                                            max(10.0, width - height), height))
        text_slot.set_z_order(22)

        # 글자 크기도 칸에 맞춘다.
        #
        # 원복 패스가 이 숫자를 원래 크기(20pt)로 되돌렸는데, 그건 옛 34px
        # 자리 기준이다. 칸이 23px 로 얇아졌으니 그대로 두면 숫자만 칸 아래로
        # 삐져나온다 -- "그래도 삐져나옴" 의 원인. 잉크(숫자 0.73em)가 칸을
        # 채우는 크기로 내리고, 위 정렬 + 여백으로 잉크를 칸 가운데 놓는다.
        text = unreal.find_object(None, f"{tree.get_path_name()}.TurnSpeed_{index}")
        if isinstance(text, unreal.TextBlock):
            text.modify()
            size_pt = max(8, int(height * 0.72 / (0.73 * (96.0 / 72.0))))
            font = text.get_editor_property("font")
            font.set_editor_property("size", float(size_pt))
            text.set_editor_property("font", font)
            inner = text.get_editor_property("slot")
            if isinstance(inner, unreal.OverlaySlot):
                em = size_pt * 96.0 / 72.0
                ink, head = em * 0.73, em * 0.19
                pad_top = (height - ink) / 2.0 - head - 0.055 * em
                inner.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
                inner.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_TOP)
                inner.set_padding(unreal.Margin(0.0, pad_top, 0.0, 0.0))
        done += 1
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    say(f"토큰 {done}개 · 저장="
        f"{unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)}")
    RESULT.parent.mkdir(parents=True, exist_ok=True)
    RESULT.write_text("\n".join(LINES) + "\n", encoding="utf-8")


main()
