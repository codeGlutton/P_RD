"""타이틀 단추 글자를 **틀 속에 꽉 차게** 잰다.

무엇이 문제였나
---------------
글자칸을 그어 둔 영역(357x53)에 딱 맞추고 크기를 재니 25pt 가 나왔다. 줄
상자로 보면 칸의 94% 라 꽉 찬 값인데, **눈에 보이는 대문자는 줄 상자의 56%
뿐**이라(Oswald 는 악센트 자리로 위쪽이 넓다) 작아 보인다.

    줄 상자 = ascent + descent = 1.49em      <- 칸에 들어가야 세로 가운데가 산다
    대문자   = 0.84em                          <- 사람 눈에 보이는 것

그래서 둘을 **다른 칸에 맞춘다.**

    대문자   그어 둔 영역(속)에 맞춘다 -- 나무테를 안 밟는 선
    줄 상자   틀 전체에 맞춘다        -- 넘치면 슬레이트가 위로 붙여 버린다

글자칸도 영역이 아니라 **틀 높이**로 넓힌다. 칸은 자르지 않으므로 위아래
여유는 그냥 여백이고, 그 덕에 대문자를 훨씬 크게 넣을 수 있다.

    python Tools/UI/fit_title_buttons.py
"""

import json
import math
from pathlib import Path

from PIL import ImageFont

ROOT = Path("D:/UnrealProjects/P_RD_develop_20260803")
MOCKUPS = ROOT / "Tools/UI/mockups"
WORKSPACE = Path("D:/UnrealProjects_WBP_Editor/data/workspace.json")
OUT = MOCKUPS / "title_fit.json"

ASSET = "/Game/UI/WBP_TitleMenu"
# (글자칸, 틀칸, 한 벌 이름)
#
# 한 벌은 **가장 작은 값으로 맞춘다.** 네 단추가 37·37·37·39 로 제각각이면
# 글자 하나가 튀어 보인다. 메뉴 줄은 같은 크기여야 한 벌로 읽힌다.
PAIRS = [
    ("StartButtonText__base_16_9", "StartButtonFrameImage__base_16_9", "메뉴"),
    ("ContinueButtonText__base_16_9", "ContinueButtonFrameImage__base_16_9", "메뉴"),
    ("SettingsButtonText__base_16_9", "SettingsButtonFrameImage__base_16_9", "메뉴"),
    ("ExitButtonText__base_16_9", "ExitButtonFrameImage__base_16_9", "메뉴"),
    ("VersionText__base_16_9", "VersionPlateImage__base_16_9", "버전"),
]

FACES = {
    ("F_HUD_Oswald", True): MOCKUPS / "fonts/Oswald-Bold.ttf",
    ("F_HUD_Oswald", False): MOCKUPS / "fonts/Oswald-Regular.ttf",
    ("F_HUD_LINESeedKR", True): MOCKUPS / "fonts/LINESeedKR-Bold.ttf",
    ("F_HUD_LINESeedKR", False): MOCKUPS / "fonts/LINESeedKR-Regular.ttf",
    ("Roboto", True): Path("C:/Windows/Fonts/arialbd.ttf"),
    ("Roboto", False): Path("C:/Windows/Fonts/arial.ttf"),
}

PROBE = 100
PT_TO_PX = 96.0 / 72.0     # 슬레이트가 pt 를 픽셀로 바꾸는 배율
CAP_FILL = 0.80            # 대문자가 속 높이의 이만큼까지
WIDE_FILL = 0.90           # 글줄이 속 너비의 이만큼까지
SMALLEST, BIGGEST = 10, 72


def rects():
    """이름 -> 자리(x, y, w, h). 추출본이 잰 값."""
    data = json.loads(WORKSPACE.read_text(encoding="utf-8"))
    found = {}
    for document in data.get("documents", []):
        if document.get("sourceKind") != "current-develop-wbp":
            continue
        if document.get("assetPath") != ASSET:
            continue
        for widget in document.get("widgets", []):
            rect = widget.get("rect") or {}
            key = widget.get("holds") or widget.get("name")
            found[key] = (float(rect.get("x", 0)), float(rect.get("y", 0)),
                          float(rect.get("w", 0)), float(rect.get("h", 0)))
    return found


def main():
    styles = json.loads((MOCKUPS / "text_style.json").read_text(
        encoding="utf-8"))["styles"]
    drawn = json.loads((MOCKUPS / "rects_user.json").read_text(encoding="utf-8"))
    sources = json.loads((MOCKUPS / "widget_src.json").read_text(encoding="utf-8"))
    catalogue = {a["name"]: a for a in json.loads(
        (MOCKUPS / "assets.json").read_text(encoding="utf-8"))}
    where = rects()

    plan, groups = {}, {}
    for text_name, frame_name, group in PAIRS:
        look = styles.get(f"{ASSET}/{text_name}")
        frame = where.get(frame_name)
        held = sources.get(f"{ASSET}/{frame_name}") or {}
        art = (held.get("src") or "").rsplit("/", 1)[-1]
        region = None
        for mark in drawn.get(art, []):
            region = mark.get("inner") or mark.get("rect") or mark.get("box")
            if region:
                break
        if look is None or frame is None or region is None:
            print(f"  {text_name}: 건너뜀 (글자 {look is not None} · "
                  f"틀 {frame is not None} · 영역 {region is not None})")
            continue

        # 속(그어 둔 영역)이 틀 안에서 차지하는 자리.
        #
        # 9-slice 는 테두리를 원본 픽셀로 그리므로 여백이 원본 기준이고,
        # 통짜로 늘리는 것은 자리에 비례한다.
        entry = catalogue.get(art) or {}
        size = entry.get("size") or [frame[2], frame[3]]
        if held.get("draw") == "BOX":
            base_w, base_h = float(size[0]), float(size[1])
        else:
            base_w, base_h = frame[2], frame[3]
        left, top = region[0] * base_w, region[1] * base_h
        right, bottom = (1.0 - region[2]) * base_w, (1.0 - region[3]) * base_h
        open_w = max(10.0, frame[2] - left - right)
        open_h = max(10.0, frame[3] - top - bottom)
        open_cx = frame[0] + left + open_w / 2.0
        open_cy = frame[1] + top + open_h / 2.0

        face = FACES.get((look["font"], str(look.get("typeface", "")).lower()
                          .startswith("bold")))
        if face is None or not face.is_file():
            print(f"  {text_name}: 글꼴 {look['font']} 을 못 찾음")
            continue
        probe = ImageFont.truetype(str(face), PROBE)
        ascent, descent = probe.getmetrics()
        line_em = (ascent + descent) / PROBE
        box = probe.getbbox(look.get("text") or "AGY")
        cap_em = (box[3] - box[1]) / PROBE
        wide_em = (box[2] - box[0]) / PROBE

        by_cap = open_h * CAP_FILL / (cap_em * PT_TO_PX)
        by_wide = open_w * WIDE_FILL / (wide_em * PT_TO_PX) if wide_em else by_cap
        # 줄 상자 제한은 없다.
        #
        # 전에는 "줄 상자가 칸을 넘으면 세로 가운데를 잃는다" 며 틀 높이로
        # 묶었다. 이제 글자를 **위쪽 정렬**로 두고 여백을 계산해 놓으므로
        # (fit_title_textbox.py) 넘쳐도 자리가 안 흔들린다. 그러니 눈에
        # 보이는 대문자만 칸에 맞추면 된다.
        by_line = frame[3] / (line_em * PT_TO_PX)
        want = int(math.floor(min(by_cap, by_wide)))
        want = max(SMALLEST, min(BIGGEST, want))

        # 줄 상자 가운데와 대문자 가운데가 어긋난 만큼 칸을 밀어 준다.
        # Oswald 는 위쪽 여유가 커서 그냥 두면 글자가 조금 내려앉는다.
        nudge = (box[1] + box[3]) / 2.0 / PROBE - line_em / 2.0

        plan[text_name] = {
            "size": want, "group": group, "nudge": nudge,
            # 자리를 잡는 쪽(fit_title_textbox.py)이 글꼴 비율을 알아야 한다.
            "capEm": round(cap_em, 4), "headEm": round(box[1] / PROBE, 4),
            "lineEm": round(line_em, 4),
            "was": float(look["size"]),
            # **자리는 틀 왼쪽위에서 얼마나 떨어졌는지로 적는다.**
            #
            # 추출본의 x·y 는 화면 절대 좌표인데 캔버스 슬롯의 offsets 는
            # 앵커 기준이다. 절대값을 그대로 넣었더니 앵커(화면 아래)에
            # 650 을 또 더해 글자가 화면 밖으로 나갔다.
            "left": open_cx - open_w / 2.0 - frame[0],
            "middle": open_cy - frame[3] / 2.0 - frame[1],
            "width": open_w, "height": frame[3],
            "why": (f"속 {open_w:.0f}x{open_h:.0f} · 대문자 {by_cap:.0f} · "
                    f"폭 {by_wide:.0f} · 줄상자 {by_line:.0f}"),
        }
        groups.setdefault(group, []).append(want)

    # 한 벌은 가장 작은 값으로. 자리 보정도 그 크기로 다시 잡는다.
    for name, item in plan.items():
        smallest = min(groups[item["group"]])
        if smallest != item["size"]:
            item["why"] += f" · 한 벌({item['group']})에 맞춰 {item['size']}->{smallest}"
            item["size"] = smallest
        shift = item["nudge"] * item["size"] * PT_TO_PX
        item["box"] = [round(item.pop("left"), 1),
                       round(item.pop("middle") - shift, 1),
                       round(item.pop("width"), 1), round(item.pop("height"), 1)]
        item["why"] += f" · 내림 보정 {shift:.1f}px"
        item.pop("nudge", None)
        print(f"  {name:34} {item['was']:.0f} -> {item['size']}pt   {item['why']}")

    OUT.write_text(json.dumps(plan, ensure_ascii=False, indent=1),
                   encoding="utf-8")
    print(f"\n{len(plan)}개를 {OUT} 에 적음")


if __name__ == "__main__":
    main()
