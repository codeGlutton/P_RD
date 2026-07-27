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

from concurrent.futures import ProcessPoolExecutor

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

#: 글자 없는 조각을 글자 있는 시안에 맞출 때의 문턱.
#:
#: 판은 같은데 시안에만 글자가 있으므로, 제자리에 놓아도 글자 픽셀만큼
#: 차이가 남는다. 3안에서 재 보니 제자리는 20 안쪽이고 엉뚱한 자리는 40을
#: 넘었다.
BLANK_SCORE = 28.0


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


def _one(job):
    """조각 하나를 시안 위에서 맞춘다. 프로세스 풀이 부르므로 최상위에 둔다."""
    mock_path, path = job
    part = Image.open(path)
    if part.mode != "RGBA" or min(part.size) < MIN_SIDE:
        return None
    if part.size[0] > 1660 and part.size[1] > 930:
        return None
    mock_grey = np.asarray(Image.open(mock_path).convert("RGB"),
                           dtype=float).mean(axis=2)
    got = match(mock_grey, part)
    if got is None:
        return None
    x, y, w, h, score = got
    return {"file": os.path.basename(path), "x": x, "y": y, "w": w, "h": h,
            "score": round(score, 2)}


def run(number):
    folder = os.path.join(CUTOUTS, "시안%d" % int(number))
    mock_path = os.path.join(MOCKUPS, "KK_HUD_Polish_%02d.png" % int(number))
    if not (os.path.isdir(folder) and os.path.exists(mock_path)):
        print("  없음: %s" % folder)
        return None

    # 자리는 글자 있는 조각으로 찾는다. 글자가 판마다 달라서 어느 조각이
    # 어느 칸인지 그림만으로 갈리기 때문이다 -- 글자 없는 판으로 맞췄더니
    # 3안 카드 여섯 칸 중 세 칸이 비었다. 빈 카드끼리는 서로 구별이 안 된다.
    jobs = [(mock_path, path)
            for path in sorted(glob.glob(os.path.join(folder, "*.png")))]
    workers = max(1, min(16, (os.cpu_count() or 4) - 2))
    with ProcessPoolExecutor(max_workers=workers) as pool:
        found = [item for item in pool.map(_one, jobs, chunksize=1)
                 if item is not None]

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
        # 같은 판에서 여러 번 오려 낸 것들이 한 자리를 다툰다. 위에서 겹침으로
        # 걸러지므로 여기서는 통과한 것만 남는다.
    kept.sort(key=lambda e: (e["y"], e["x"]))
    swap_blanks(folder, kept)

    print("=== 시안%s: 조각 %d개 (후보 %d개에서 추림) ===" % (
        number, len(kept), len(found)))
    for item in kept:
        print("  %-46s x=%4d y=%3d %4dx%-4d  차 %5.1f%s" % (
            item["file"], item["x"], item["y"], item["w"], item["h"],
            item["score"],
            "  -> %s" % item["blank"] if item.get("blank") else ""))
    return kept


def swap_blanks(folder, kept):
    """자리는 그대로 두고 그림만 글자 없는 판으로 바꾼다.

    조각에 글자가 그려져 있으면 런타임 글자와 겹쳐 두 번 찍힌다 -- 2안을
    구워 보고 알았다. "기사"도 "90/100"도 판에 있고 위젯에도 있었다.

    사람이 `01_top_left_parchment.png` 꼴로 이름 붙여 둔 것이 글자를 지운
    판이다. 같은 자리의 조각과 크기가 같으므로 크기로 짝을 짓는다. 짝이
    없으면 원래 조각을 그대로 둔다 -- 억지로 비슷한 것을 붙이면 엉뚱한 판이
    엉뚱한 자리에 온다.
    """
    named = sorted(glob.glob(os.path.join(folder, "[0-9][0-9]_*.png")))
    if not named:
        return
    sizes = []
    for path in named:
        with Image.open(path) as art:
            sizes.append((art.size[0], art.size[1], os.path.basename(path)))

    used = set()
    for item in kept:
        best, gap = None, None
        for w, h, name in sizes:
            if name in used:
                continue
            miss = abs(w - item["w"]) + abs(h - item["h"])
            if miss <= 6 and (gap is None or miss < gap):
                best, gap = name, miss
        if best is None:
            continue
        used.add(best)
        item["blank"] = best


def main():
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8")
    ap = argparse.ArgumentParser()
    ap.add_argument("--mockup", default=None, help="비우면 1~20 전부")
    args = ap.parse_args()

    numbers = ([args.mockup] if args.mockup
               else [str(i) for i in range(1, 21)])
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
