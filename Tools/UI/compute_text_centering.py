"""감싼 글자 전부에 대해 **잉크가 칸 가운데 오는 여백**을 계산한다.

타이틀에서 확인한 것의 일반화
-----------------------------
VAlign_Center 는 글자의 **줄 상자**를 가운데 놓는다. 눈에 보이는 잉크는 줄
상자 안에서 아래쪽에 치우쳐 있어(Oswald 는 0.045em), 가운데 정렬만 믿으면
글자가 2~3px 내려앉아 보인다. 게다가 언리얼은 글꼴 파일의 약속보다 몇 px
더 아래에 그린다(실측 -- font_calib.json).

그래서 타이틀처럼 **위쪽 정렬 + 계산한 여백**으로 놓는다.

    여백위 = (칸높이 - 잉크높이) / 2 - 잉크윗공백 - 실측보정(글꼴) x 글자크기

칸은 글자를 감싼 X_Center 판의 자리다. 그 판의 자리는 우리가 글자 여백을
바꿔도 안 변하므로, 이 계산은 몇 번을 다시 돌려도 같은 답이 나온다.

건너뛰는 것
-----------
    줄바꿈 문단     잉크 계산이 한 줄짜리다. 문단은 가운데 정렬이 맞다.
    auto 칸        칸이 글자를 딱 감싸 가운데를 잴 대상이 없다.
    타이틀 다섯 칸  fit_title_textbox 가 칸별 실측값으로 따로 잡는다.

    python Tools/UI/compute_text_centering.py     -> mockups/text_pad.json
"""

import json
import re
from pathlib import Path

from PIL import ImageFont

ROOT = Path("D:/UnrealProjects/P_RD_develop_20260803")
MOCKUPS = ROOT / "Tools/UI/mockups"
WORKSPACE = Path("D:/UnrealProjects_WBP_Editor/data/workspace.json")
OUT = MOCKUPS / "text_pad.json"

PT_TO_PX = 96.0 / 72.0
PROBE = 100

FACES = {
    ("F_HUD_Oswald", True): MOCKUPS / "fonts/Oswald-Bold.ttf",
    ("F_HUD_Oswald", False): MOCKUPS / "fonts/Oswald-Regular.ttf",
    ("F_HUD_LINESeedKR", True): MOCKUPS / "fonts/LINESeedKR-Bold.ttf",
    ("F_HUD_LINESeedKR", False): MOCKUPS / "fonts/LINESeedKR-Regular.ttf",
    ("Roboto", True): Path("C:/Windows/Fonts/arialbd.ttf"),
    ("Roboto", False): Path("C:/Windows/Fonts/malgunbd.ttf"),
    ("F_HUD_NotoSansKR", True): Path("C:/Windows/Fonts/malgunbd.ttf"),
    ("F_HUD_NotoSansKR", False): Path("C:/Windows/Fonts/malgun.ttf"),
}

HANGUL = re.compile(r"[\uac00-\ud7a3]")


def sample_of(text, font_name):
    """잉크를 잴 글. 판의 글이 못 미더우면 대표 글자로 잰다.

    빈 글·기호 한둘('-')은 런타임에 딴 글이 들어온다. 그때 잉크가 달라지면
    자리가 흔들리므로, 그 글꼴이 실제로 그릴 만한 대표 글자로 잰다.
    Oswald 에는 한글이 없다 -- 한글이 적혀 있으면 대표 글자로 바꾼다.
    """
    text = (text or "").strip()
    korean_face = font_name in ("F_HUD_LINESeedKR", "F_HUD_NotoSansKR")
    if font_name == "F_HUD_Oswald" and HANGUL.search(text):
        return "AGY0"
    if len(text) >= 2 and any(ch.isalnum() or HANGUL.match(ch) for ch in text):
        return text
    return "가늠A0" if korean_face else "AGY0"


def opening_of(rows, holder, sources, regions, asset):
    """글자가 탄 틀의 **그어 둔 영역** 세로 구간 (위, 아래). 없으면 None.

    글자 -> X_Center -> Mount 로 올라가, Mount 이름에서 틀(그림) 위젯을
    찾는다(mount_all 이 `틀이름 + Mount` 로 지었다). 그 그림에 사람이 그어 둔
    칸(rects_user)이 있으면, 그것이 위젯 칸보다 진짜 기준이다 -- 전투 UI 는
    글자 칸이 판 그림과 안 맞는 데가 많다.

    9-slice(BOX)는 테두리가 원본 픽셀 그대로라 여백도 원본 기준이고,
    통짜는 자리에 비례한다 -- 타이틀에서 확인한 규칙 그대로.
    """
    center_row = rows.get(holder)
    if center_row is None:
        return None
    mount = center_row.get("holder") or ""
    if not mount.endswith("Mount"):
        return None
    frame_name = mount[:-len("Mount")]
    frame_row = rows.get(frame_name)
    if frame_row is None:
        return None
    held = sources.get(f"{asset}/{frame_name}") or {}
    art = (held.get("src") or "").rsplit("/", 1)[-1]
    region = None
    for mark in regions.get(art, []):
        region = mark.get("inner") or mark.get("rect") or mark.get("box")
        if region and len(region) == 4:
            break
    if not region:
        return None
    rect = frame_row.get("rect") or {}
    top, height = float(rect.get("y", 0.0)), float(rect.get("h", 0.0))
    if height <= 4.0:
        return None
    size = held.get("size")
    if held.get("draw") == "BOX" and size:
        art_h = float(size[1])
        lo = top + region[1] * art_h
        hi = top + height - (1.0 - region[3]) * art_h
    else:
        lo = top + region[1] * height
        hi = top + region[3] * height
    return (lo, hi) if hi - lo > 4.0 else None


def main():
    styles = json.loads((MOCKUPS / "text_style.json").read_text(
        encoding="utf-8"))["styles"]
    calib = json.loads((MOCKUPS / "font_calib.json").read_text(encoding="utf-8"))
    regions = json.loads((MOCKUPS / "rects_user.json").read_text(
        encoding="utf-8")) if (MOCKUPS / "rects_user.json").is_file() else {}
    sources = json.loads((MOCKUPS / "widget_src.json").read_text(
        encoding="utf-8")) if (MOCKUPS / "widget_src.json").is_file() else {}
    title_own = set(json.loads((MOCKUPS / "title_fit.json").read_text(
        encoding="utf-8"))) if (MOCKUPS / "title_fit.json").is_file() else set()
    data = json.loads(WORKSPACE.read_text(encoding="utf-8"))

    plan, skipped = {}, {"문단": 0, "auto": 0, "칸없음": 0, "글꼴없음": 0}
    faces = {}
    for document in data.get("documents", []):
        if document.get("sourceKind") != "current-develop-wbp":
            continue
        asset = document.get("assetPath", "?")
        rows = {w["name"]: w for w in document.get("widgets", [])}
        # 묶음마다 글자가 몇 개 타나. 영역 기준을 쓸지 정하는 데 쓴다.
        riders = {}
        for w in document.get("widgets", []):
            if w.get("className") in ("TextBlock", "RichTextBlock"):
                mid = rows.get(w.get("holder") or "", {}).get("holder") or ""
                if mid.endswith("Mount"):
                    riders[mid] = riders.get(mid, 0) + 1
        for row in document.get("widgets", []):
            if row.get("className") not in ("TextBlock",):
                continue
            name = row["name"]
            holder = row.get("holder") or ""
            if not holder.endswith("_Center") or name in title_own:
                continue
            key = f"{asset}/{name}"
            look = styles.get(key)
            if look is None:
                continue
            if look.get("wrap"):
                skipped["문단"] += 1
                continue
            if look.get("auto"):
                skipped["auto"] += 1
                continue
            box_row = rows.get(holder)
            box_h = float((box_row or {}).get("rect", {}).get("h", 0.0))
            if box_h < 8.0:
                skipped["칸없음"] += 1
                continue

            font_name = look.get("font", "Roboto")
            bold = str(look.get("typeface", "")).lower().startswith("bold")
            face_key = (font_name, bold)
            if face_key not in faces:
                path = FACES.get(face_key) or FACES.get((font_name, not bold))
                faces[face_key] = (ImageFont.truetype(str(path), PROBE)
                                   if path and path.is_file() else None)
            probe = faces[face_key]
            if probe is None:
                skipped["글꼴없음"] += 1
                continue

            size_px = float(look["size"]) * PT_TO_PX
            ink = probe.getbbox(sample_of(look.get("text"), font_name))
            ink_top = ink[1] / PROBE * size_px
            ink_h = (ink[3] - ink[1]) / PROBE * size_px
            k = float(calib.get(f"{font_name}|{'Bold' if bold else 'Regular'}", 0.0))

            # 그어 둔 영역이 있으면 **그 세로 구간** 가운데에 놓는다.
            # 없으면 글자칸 가운데 -- 칸이 판과 맞는 화면에서는 그걸로 충분하다.
            #
            # 단, **한 틀에 글자가 하나일 때만.** 판 하나에 이름·상태·설명이
            # 줄줄이 타는 경우(AllyPanel)는 영역 가운데로 몰면 전부 한 줄에
            # 겹친다 -- 그런 배치는 글자칸이 곧 각자의 줄이다.
            opening = None
            if riders.get(rows.get(holder, {}).get("holder", ""), 0) == 1:
                opening = opening_of(rows, holder, sources, regions, asset)
            if opening is not None:
                # 영역이 글자와 **같은 급의 자리**일 때만 쓴다. 스킬 카드의
                # 코스트 젬 글자는 카드 판 전체 영역(184px)에 붙잡혀 젬 밖
                # 한가운데로 밀려났다 -- 젬(55px)과 판은 급이 다르다.
                # 영역이 글자칸의 2.5배를 넘거나 글자칸 가운데를 안 품으면
                # 남의 영역이다.
                box_row_ = rows.get(holder) or {}
                box_y = float(box_row_.get("rect", {}).get("y", 0.0))
                box_mid = box_y + box_h / 2.0
                if (opening[1] - opening[0] > box_h * 2.5
                        or not (opening[0] - 20.0 <= box_mid <= opening[1] + 20.0)):
                    opening = None
            if opening is not None:
                box_row = rows.get(holder) or {}
                center_top = float(box_row.get("rect", {}).get("y", 0.0))
                target = (opening[0] + opening[1]) / 2.0
                pad_top = target - ink_h / 2.0 - ink_top - k * size_px - center_top
                base = f"영역 {opening[0]:.0f}..{opening[1]:.0f} 기준"
            else:
                pad_top = (box_h - ink_h) / 2.0 - ink_top - k * size_px
                base = "글자칸 기준"
            plan[key] = {"padTop": round(pad_top, 1), "boxH": round(box_h, 1),
                         "size": look["size"],
                         "why": f"{base} · 잉크 {ink_h:.0f}px · "
                                f"윗공백 {ink_top:.0f} · 보정 {k * size_px:.1f}"}

    OUT.write_text(json.dumps(plan, ensure_ascii=False, indent=1),
                   encoding="utf-8")
    print(f"{len(plan)}칸 계산 · 건너뜀 {skipped}")
    for key, item in list(plan.items())[:5]:
        print(f"  {key.rsplit('/', 1)[-1]:40} 칸 {item['boxH']:.0f} · "
              f"여백위 {item['padTop']:.1f}  ({item['why']})")


if __name__ == "__main__":
    main()
