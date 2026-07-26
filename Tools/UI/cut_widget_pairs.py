# -*- coding: utf-8 -*-
"""위젯 하나하나를 시안과 나란히 잘라 저장한다.

dump_widget_rects.py 가 남긴 좌표로, 우리 캡처와 시안에서 **같은 사각형**을
떠서 위아래로 붙인다. 파일 이름이 위젯 이름이므로 어느 부품이 어긋났는지
목록만 훑어도 짚힌다.

같은 사각형을 쓰는 게 핵심이다. 두 그림에서 따로 요소를 찾아 짝지으면
엉뚱한 것끼리 묶여 그럴듯한 거짓 수치가 나온다 -- 이 작업에서 실제로 그랬다.

    python cut_widget_pairs.py [--layout WBP_CombatLayout_01_ClassicCRPG]
"""
import argparse
import io
import json
import os
import sys

import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage

ROOT = r"D:/UnrealProjects/P_RD_develop_20260726/Saved/UI/CombatLayouts"
MOCKUPS = r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitHUDMockups"
OUTDIR = os.path.join(ROOT, "WidgetPairs")
PAD = 6          # 잘라낼 때 둘레 여유. 테두리가 잘리면 어긋남이 안 보인다.
MIN_SIDE = 10    # 이보다 작은 위젯은 건너뛴다
SEARCH = 22      # 자리 어긋남을 찾을 범위 (px)

#: 낱개로 비교해도 뜻이 없는 부품.
#:
#: 우리 프레임은 조각을 이어 붙여 만들지만 시안은 한 장으로 그려져 있다.
#: 17x64 짜리 레일 조각을 시안의 같은 자리와 견주면 그쪽에는 연속된 그림의
#: 아무 부분이 있을 뿐이라, 픽셀차가 크게 나와도 알려 주는 게 없다. 실제로
#: 이런 조각 161장이 순위를 통째로 오염시켰다.
SKIP = ("_Shadow", "_Shade", "_Ink", "_Rim", "_Seat", "_Veil")


def is_frame_piece(name):
    tail = name.rsplit("_", 1)[-1]
    if tail and all(c in "FLRTBC" for c in tail) and len(tail) >= 2:
        return True
    return any(k in name for k in SKIP)


def offset(mock, ours, search=SEARCH):
    """두 조각의 '자리 어긋남'. 그림이 달라도 윤곽이 겹치면 0에 가깝다.

    색이 아니라 윤곽(밝기 기울기)으로 맞춘다. 아이콘 그림이 서로 달라도
    덩어리가 같은 자리에 있으면 어긋남은 작게 나온다 -- 그래야 "자리가 틀린
    것"과 "그림이 다른 것"이 갈린다.
    """
    def edge(a):
        g = np.asarray(a.convert("L"), dtype=float)
        gx = ndimage.sobel(g, axis=1)
        gy = ndimage.sobel(g, axis=0)
        e = np.hypot(gx, gy)
        return (e - e.mean()) / (e.std() + 1e-6)

    m, c = edge(mock), edge(ours)
    h, w = m.shape
    if h <= 2 * search + 4 or w <= 2 * search + 4:
        search = max(1, min(h, w) // 4)
    core = c[search:h - search, search:w - search]
    if core.size == 0:
        return None
    best, bx, by = -1e9, 0, 0
    scores = []
    for dy in range(-search, search + 1, 2):
        for dx in range(-search, search + 1, 2):
            win = m[search + dy:h - search + dy, search + dx:w - search + dx]
            if win.shape != core.shape:
                continue
            score = float((win * core).mean())
            scores.append(score)
            if score > best:
                best, bx, by = score, dx, dy
    if not scores:
        return None
    # 봉우리가 탐색 경계에 붙었으면 맞출 곳을 못 찾은 것이다. 그걸 "경계값만큼
    # 어긋났다"로 읽으면 없는 오차를 만들어 낸다 -- 처음 돌렸을 때 스물다섯
    # 개가 전부 ±22 로 나왔고, 그게 바로 이 경우였다.
    if abs(bx) >= search or abs(by) >= search:
        return None
    # 봉우리가 주변보다 확실히 높지 않으면 우연이다.
    arr = np.asarray(scores)
    if best < arr.mean() + 2.0 * (arr.std() + 1e-6):
        return None
    return bx, by


def load(path):
    if not os.path.exists(path):
        raise SystemExit("파일 없음: " + path)
    return Image.open(path).convert("RGB")


def main():
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8")
    ap = argparse.ArgumentParser()
    ap.add_argument("--layout", default="WBP_CombatLayout_01_ClassicCRPG")
    ap.add_argument("--mockup", default=None,
                    help="기본은 배치안 번호에 맞는 KK_HUD_Polish_NN.png")
    args = ap.parse_args()

    with io.open(os.path.join(ROOT, "rects.json"), encoding="utf-8") as handle:
        rects = json.load(handle)
    if args.layout not in rects:
        raise SystemExit("좌표 없음: %s (가진 것: %s)"
                         % (args.layout, ", ".join(sorted(rects))))

    number = args.layout.split("_")[2]
    mock_path = args.mockup or os.path.join(
        MOCKUPS, "KK_HUD_Polish_%s.png" % number)
    M = load(mock_path)
    C = load(os.path.join(ROOT, args.layout + ".png"))
    if M.size != C.size:
        M = M.resize(C.size, Image.LANCZOS)

    out = os.path.join(OUTDIR, args.layout)
    os.makedirs(out, exist_ok=True)
    for stale in os.listdir(out):
        if stale.endswith(".png"):
            os.remove(os.path.join(out, stale))

    rows = []
    for item in rects[args.layout]:
        w, h = item["w"], item["h"]
        if w < MIN_SIDE or h < MIN_SIDE:
            continue
        x0 = max(0, int(item["x"]) - PAD)
        y0 = max(0, int(item["y"]) - PAD)
        x1 = min(C.width, int(item["x"] + w) + PAD)
        y1 = min(C.height, int(item["y"] + h) + PAD)
        if x1 - x0 < MIN_SIDE or y1 - y0 < MIN_SIDE:
            continue
        if is_frame_piece(item["name"]):
            continue
        box = (x0, y0, x1, y1)
        mc, cc = M.crop(box), C.crop(box)
        off = offset(mc, cc)
        # 위=시안 아래=우리. 가운데 가는 선을 넣어 경계를 분명히 한다.
        pair = Image.new("RGB", (x1 - x0, (y1 - y0) * 2 + 3), (30, 30, 34))
        pair.paste(mc, (0, 0))
        pair.paste(cc, (0, y1 - y0 + 3))
        # 얼마나 다른지 한 줄로 적어 파일 이름에 넣는다.
        diff = float(np.abs(np.asarray(mc, dtype=float)
                            - np.asarray(cc, dtype=float)).mean())
        scale = max(1, min(4, 220 // max(x1 - x0, 1)))
        if scale > 1:
            pair = pair.resize((pair.width * scale, pair.height * scale),
                               Image.NEAREST)
        shift = None if off is None else float(np.hypot(*off))
        name = "%s_d%03d_%s_%s.png" % (
            "xxx" if shift is None else "%03d" % round(shift),
            round(diff), item["type"], item["name"])
        pair.save(os.path.join(out, name))
        rows.append((shift, diff, item["name"], item["type"], w, h, off))

    known = [r for r in rows if r[0] is not None]
    unknown = [r for r in rows if r[0] is None]
    rows = sorted(known, reverse=True)
    print("%d개 저장: %s" % (len(rows), out))
    print("  (프레임 조각은 제외했다 -- 시안은 한 장으로 그려져 낱개 비교가 뜻이 없다)")
    print()
    print("=== 자리 어긋남 큰 순 (앞 25개) ===")
    print("%6s %6s  %-28s %-12s %s" % ("어긋남", "픽셀차", "위젯", "종류", "이동"))
    for shift, d, name, kind, w, h, off in rows[:25]:
        print("%6.1f %6.1f  %-28s %-12s %s"
              % (shift, d, name, kind, "%+d,%+d" % off if off else "-"))
    ok = [r for r in rows if r[0] <= 4]
    total = len(rows) + len(unknown)
    print()
    print("자리를 잴 수 있었던 것 %d/%d" % (len(rows), total))
    print("  그 중 4px 이내 %d개 (%.0f%%)" % (
        len(ok), 100.0 * len(ok) / max(len(rows), 1)))
    print("잴 수 없었던 것 %d개 -- 윤곽이 밋밋하거나 시안 쪽 대응물이 없다."
          % len(unknown))
    print("  (파일 이름이 xxx 로 시작한다. 눈으로 봐야 하는 것들이다.)")


if __name__ == "__main__":
    main()
