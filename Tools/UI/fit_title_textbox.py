"""묶음 안의 글자를 **그어 둔 칸 한가운데**에 놓는다.

왜 VAlign_Center 를 안 쓰나
---------------------------
슬레이트는 내용이 칸보다 크면 세로 가운데를 버리고 위에 붙인다. 글자 줄
상자(ascent+descent)는 눈에 보이는 대문자의 1.8배라, 대문자를 칸에 꽉 채우면
줄 상자는 늘 칸을 넘는다. 그래서 가운데 정렬에 기대면 큰 글자를 못 쓴다.

한 번은 칸을 틀 전체 높이로 넓혀서 피했는데, 그러면 **그어 둔 칸의 위아래
값이 죽는다.** 사람이 15.3% / 84.0% 를 적어 뒀는데 안 쓰이는 것이다.

    글자를 **위쪽 정렬**로 두고, 대문자가 칸 한가운데 오도록 여백을 계산한다.

        줄 시작 = 칸위 + (칸높이 - 대문자높이) / 2 - 윗공백

    이러면 넘치든 안 넘치든 자리가 딱 정해진다. 그어 둔 칸도 그대로 쓴다.

Run headless:
    python Tools/UI/fit_title_buttons.py      # 크기를 먼저 재고
    UnrealEditor-Cmd.exe <project> -run=pythonscript
        -script="Tools/UI/fit_title_textbox.py"
"""

import json
from pathlib import Path

import unreal

ROOT = Path("D:/UnrealProjects/P_RD_develop_20260803")
MOCKUPS = ROOT / "Tools/UI/mockups"
RESULT = ROOT / "Saved/LegacyAudit/title_box.txt"
ASSET = "/Game/UI/WBP_TitleMenu"

PLAN_PATH = MOCKUPS / "title_fit.json"
PLAN = (json.loads(PLAN_PATH.read_text(encoding="utf-8"))
        if PLAN_PATH.is_file() else {})

# 실측 보정.
#
# 글꼴 파일이 말하는 위치대로 놓았는데 게임 화면을 재 보니 대문자가 칸
# 가운데보다 2~3px 아래에 그려졌다 -- 언리얼(FreeType 힌팅)이 글자를 앉히는
# 자리가 글꼴의 약속과 그만큼 다르다. 계산으로는 못 없애서, 게임을 찍어 잰
# 오차를 그대로 빼 준다. 글꼴이나 크기를 바꾸면 다시 재야 한다.
CALIB_PATH = MOCKUPS / "title_calib.json"
CALIB = (json.loads(CALIB_PATH.read_text(encoding="utf-8"))
         if CALIB_PATH.is_file() else {})

# (글자칸, 틀칸)
PAIRS = [
    ("StartButtonText__base_16_9", "StartButtonFrameImage__base_16_9"),
    ("ContinueButtonText__base_16_9", "ContinueButtonFrameImage__base_16_9"),
    ("SettingsButtonText__base_16_9", "SettingsButtonFrameImage__base_16_9"),
    ("ExitButtonText__base_16_9", "ExitButtonFrameImage__base_16_9"),
    ("VersionText__base_16_9", "VersionPlateImage__base_16_9"),
]

LINES = []


def say(text):
    LINES.append(text)
    unreal.log(text)


def find(name):
    tree = unreal.find_object(None, BLUEPRINT.get_path_name() + ":WidgetTree")
    return unreal.find_object(None, f"{tree.get_path_name()}.{name}")


def drawn_region(texture):
    if texture is None:
        return None
    book = json.loads((MOCKUPS / "rects_user.json").read_text(encoding="utf-8"))
    for mark in book.get(texture.get_name(), []):
        rect = mark.get("inner") or mark.get("rect") or mark.get("box")
        if rect and len(rect) == 4:
            return [float(v) for v in rect]
    return None


BLUEPRINT = unreal.EditorAssetLibrary.load_asset(ASSET)


def main():
    for text_name, frame_name in PAIRS:
        text = find(text_name)
        frame = find(frame_name)
        plan = PLAN.get(text_name)
        if text is None or frame is None or plan is None:
            say(f"{text_name}: 글자/틀/잰 값 중 없는 것이 있어 건너뜀")
            continue

        mount = text.get_parent()
        mount_slot = mount.get_editor_property("slot") if mount else None
        if not isinstance(mount_slot, unreal.CanvasPanelSlot):
            say(f"{text_name}: 묶음이 캔버스에 없음 -- 건너뜀")
            continue
        offsets = mount_slot.get_offsets()
        width, height = float(offsets.right), float(offsets.bottom)

        brush = frame.get_editor_property("brush")
        region = drawn_region(brush.get_editor_property("resource_object"))
        if region is None:
            say(f"{text_name}: 그어 둔 칸이 없음 -- 건너뜀")
            continue

        left = region[0] * width
        right = (1.0 - region[2]) * width
        top = region[1] * height
        bottom = (1.0 - region[3]) * height
        open_h = max(1.0, height - top - bottom)

        # 잰 값에는 글꼴 비율이 들어 있다(fit_title_buttons.py).
        size = float(plan["size"])
        cap = float(plan.get("capEm", 0.84)) * size * (96.0 / 72.0)
        head = float(plan.get("headEm", 0.37)) * size * (96.0 / 72.0)
        # 대문자가 칸 한가운데 오도록 줄 시작을 잡고, 실측 오차만큼 올린다.
        error = float(CALIB.get(text_name, 0.0))
        pad_top = top + (open_h - cap) / 2.0 - head - error

        BLUEPRINT.modify()
        text.modify()
        slot = text.get_editor_property("slot")
        if isinstance(slot, unreal.OverlaySlot):
            slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
            # **위쪽 정렬.** 가운데 정렬은 넘칠 때 조용히 위로 붙어 버린다.
            slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_TOP)
            slot.set_padding(unreal.Margin(left, pad_top, right, 0.0))

        font = text.get_editor_property("font")
        font.set_editor_property("size", size)
        text.set_editor_property("font", font)

        say(f"{text_name}")
        say(f"    그어 둔 칸 {left:.0f}/{top:.0f}/{right:.0f}/{bottom:.0f} "
            f"-> 속 {width - left - right:.0f}x{open_h:.0f}")
        say(f"    {size:.0f}pt · 대문자 {cap:.0f}px · 줄 시작 {pad_top:.1f} "
            f"(실측 보정 {error:+.1f}px) "
            f"-> 대문자 {top + (open_h - cap) / 2.0:.1f}.."
            f"{top + (open_h + cap) / 2.0:.1f} (칸 {top:.0f}..{height - bottom:.0f})")

    unreal.BlueprintEditorLibrary.compile_blueprint(BLUEPRINT)
    say(f"\n저장={unreal.EditorAssetLibrary.save_loaded_asset(BLUEPRINT, False)}")
    RESULT.parent.mkdir(parents=True, exist_ok=True)
    RESULT.write_text("\n".join(LINES) + "\n", encoding="utf-8")


main()
