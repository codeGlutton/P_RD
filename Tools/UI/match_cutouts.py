# -*- coding: utf-8 -*-
"""시안에서 오려 낸 조각이 그 시안 어디에 앉는지 찾아 표로 남긴다.

## 왜

조각은 시안에서 잘라낸 것이므로 원래 자리가 반드시 있다. 그 자리를 손으로
적으면 틀린다 -- 이 작업에서 좌표를 눈대중으로 옮기다 여러 번 그랬다.
조각을 시안 위에서 밀어 보며 가장 잘 맞는 곳을 찾으면 사람이 개입할 일이
없다.

## 어떻게

알파가 있는 픽셀만 비교한다. 조각은 배경이 지워져 있고 시안에는 배경이
남아 있으므로, 알파 밖을 견주면 잔디와 지붕이 점수를 흐린다.

성기게 훑어 후보를 잡고 그 둘레만 촘촘히 다시 본다. 1672x941 을 1px 씩
전부 보면 조각 하나에 몇 분이 걸린다.

## 무엇이 걸러지나

같은 시안에 여러 번 생성한 조각 묶음이 섞여 있다. 겹치는 자리를 차지하는
것끼리는 더 잘 맞는 쪽만 남긴다 -- 안 그러면 같은 판이 두 번 놓인다.

    python match_cutouts.py --mockup 01
"""
import argparse
import glob
import io
import json
import os
import sys

import numpy as np
from PIL import Image

CUTOUTS = (r"C:/Users/2009e/.codex/generated_images"
           r"/019fa031-cbf8-7d41-944b-2727570617e9")
MOCKUPS = r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitHUDMockups/Raw"
OUT = r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitUIKit/Chrome"

MIN_SIDE = 60      # 이보다 작은 조각은 부품이 아니라 아이콘이다
COARSE = 6         # 성긴 훑기 간격

#: 이보다 안 맞으면 그 시안의 조각이 아니다.
#:
#: 같은 폴더에 여러 번 생성한 묶음이 섞여 있어, 다른 판에서 나온 조각이
#: 빈 자리에 억지로 붙는다. 잘 맞은 것은 차이가 5~10 이고 억지로 붙은 것은
#: 20 을 넘는다 -- 시안1 에서 실측한 값이다.
MAX_SCORE = 15.0


def match(mock_grey, part):
    """조각이 가장 잘 맞는 자리와 그때의 평균차."""
    arr = np.asarray(part.convert("RGBA"), dtype=float)
    grey = arr[:, :, :3].mean(axis=2)
    alpha = arr[:, :, 3] > 200
    th, tw = grey.shape
    mh, mw = mock_grey.shape
    if th >= mh or tw >= mw or alpha.sum() < 400:
        return None

    best, pos = None, (0, 0)
    for step in (COARSE, 1):
        if step == COARSE:
            ys = range(0, mh - th, step)
            xs = range(0, mw - tw, step)
        else:
            by, bx = pos
            ys = range(max(0, by - COARSE), min(mh - th, by + COARSE + 1))
            xs = range(max(0, bx - COARSE), min(mw - tw, bx + COARSE + 1))
        for y in ys:
            for x in xs:
                win = mock_grey[y:y + th, x:x + tw]
                score = float(np.abs(win[alpha] - grey[alpha]).mean())
                if best is None or score < best:
                    best, pos = score, (y, x)
    return pos[1], pos[0], tw, th, best


def overlap(a, b):
    ix = max(0, min(a[0] + a[2], b[0] + b[2]) - max(a[0], b[0]))
    iy = max(0, min(a[1] + a[3], b[1] + b[3]) - max(a[1], b[1]))
    small = min(a[2] * a[3], b[2] * b[3])
    return (ix * iy) / float(max(small, 1))


def run(number):
    folder = os.path.join(CUTOUTS, "시안%d" % int(number))
    mock_path = os.path.join(MOCKUPS, "KK_HUD_Polish_%02d.png" % int(number))
    if not (os.path.isdir(folder) and os.path.exists(mock_path)):
        print("  없음: %s" % folder)
        return None
    mock = np.asarray(Image.open(mock_path).convert("RGB"), dtype=float)
    mock_grey = mock.mean(axis=2)

    found = []
    for path in sorted(glob.glob(os.path.join(folder, "*.png"))):
        name = os.path.basename(path)
        part = Image.open(path)
        if part.mode != "RGBA" or min(part.size) < MIN_SIDE:
            continue
        if part.size[0] > 1660 and part.size[1] > 930:
            continue  # 시안 전체가 통째로 든 것
        got = match(mock_grey, part)
        if got is None:
            continue
        x, y, w, h, score = got
        found.append({"file": name, "x": x, "y": y, "w": w, "h": h,
                      "score": round(score, 2)})

    # 같은 자리를 다투는 것끼리는 더 잘 맞는 쪽만 남긴다.
    found.sort(key=lambda e: e["score"])
    kept = []
    for item in found:
        box = (item["x"], item["y"], item["w"], item["h"])
        if any(overlap(box, (k["x"], k["y"], k["w"], k["h"])) > 0.5
               for k in kept):
            continue
        if item["score"] > MAX_SCORE:
            continue
        kept.append(item)
    kept.sort(key=lambda e: (e["y"], e["x"]))

    print("=== 시안%s: 조각 %d개 (후보 %d개에서 추림) ===" % (
        number, len(kept), len(found)))
    for item in kept:
        print("  %-46s x=%4d y=%3d %4dx%-4d  차 %5.1f" % (
            item["file"], item["x"], item["y"], item["w"], item["h"],
            item["score"]))
    return kept


def main():
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8")
    ap = argparse.ArgumentParser()
    ap.add_argument("--mockup", default=None, help="비우면 1~7 전부")
    args = ap.parse_args()

    numbers = ([args.mockup] if args.mockup
               else [str(i) for i in range(1, 8)])
    table = {}
    for number in numbers:
        got = run(number)
        if got:
            table["%02d" % int(number)] = got
        print()

    os.makedirs(OUT, exist_ok=True)
    path = os.path.join(OUT, "cutout_places.json")
    with io.open(path, "w", encoding="utf-8") as handle:
        json.dump(table, handle, ensure_ascii=False, indent=1)
    print("자리표: %s" % path)


if __name__ == "__main__":
    main()
