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
    swap_blanks(folder, kept, mock_path)

    print("=== 시안%s: 조각 %d개 (후보 %d개에서 추림) ===" % (
        number, len(kept), len(found)))
    for item in kept:
        print("  %-46s x=%4d y=%3d %4dx%-4d  차 %5.1f%s" % (
            item["file"], item["x"], item["y"], item["w"], item["h"],
            item["score"],
            "  -> %s" % item["blank"] if item.get("blank") else ""))
    return kept


BLANK_DIR = "텍스트_아이콘_제거"

#: 빈 판과 자리의 크기 차가 이보다 크면 다른 부품으로 본다.
#:
#: 같은 부품이어도 몇 px 어긋난다 -- 13안 턴 바는 글자 있는 쪽에 아래로
#: 뻗은 화살표가 붙어 세로가 14px 더 길다. 넉넉히 주되, 40을 넘기면 이웃한
#: 다른 판이 짝으로 잡히기 시작한다.
BLANK_GAP = 34


def _rounds(folder):
    """글자 지운 판을 생성 회차별로 묶는다.

    한 시안에 회차가 서너 개씩 들어 있고 회차마다 그림결이 조금씩 다르다.
    섞어 쓰면 카드마다 결이 달라 보이므로 한 회차 안에서만 짝을 짓는다.
    """
    import re
    from collections import defaultdict
    groups = defaultdict(list)
    for path in sorted(glob.glob(os.path.join(folder, BLANK_DIR, "*.png"))):
        name = os.path.basename(path)
        # 생성기가 뱉은 것은 회차 아이디로 묶고, 사람이 이름 붙인 것
        # (`01_top_left_parchment.png`)은 그것대로 한 벌이다. 파일명으로
        # 묶었더니 이름 붙은 열넉 장이 열넉 회차로 쪼개져 2안이 한 장만
        # 교체됐다.
        match = re.match(r"(call_[A-Za-z0-9]+)", name)
        groups[match.group(1) if match else "이름붙음"].append(path)
    return list(groups.values())


def _fit(mock_grey, item, part, reach=40):
    """빈 판을 그 자리 둘레에 대 보고 가장 잘 맞는 점수.

    밝기는 맞추지 않고 그대로 견준다. 사용불가 카드는 판이 통째로 어두운
    것이 유일한 표시라(6안 돌파베기는 밝기 49, 나머지는 94), 밝기를 맞추면
    바로 그 단서가 지워진다. 실제로 맞춰 봤더니 어두운 판이 방패강타 칸으로
    갔다.

    글자 유무 때문에 어느 판을 대도 얼마간 차이는 남는다. 그 차이는 판마다
    비슷하고, 상태가 다른 판은 그보다 훨씬 크게 벌어진다.
    """
    arr = np.asarray(part.convert("RGBA"), dtype=float)
    grey = arr[:, :, :3].mean(axis=2)
    alpha = arr[:, :, 3] > 200
    th, tw = grey.shape
    mh, mw = mock_grey.shape
    if th > mh or tw > mw or alpha.sum() < 400:
        return None
    tpl = grey[alpha]

    best = None
    for y in range(max(0, min(item["y"] - reach, mh - th)),
                   min(mh - th, item["y"] + reach) + 1, 2):
        for x in range(max(0, min(item["x"] - reach, mw - tw)),
                       min(mw - tw, item["x"] + reach) + 1, 2):
            win = mock_grey[y:y + th, x:x + tw][alpha]
            score = float(np.abs(win - tpl).mean())
            if best is None or score < best:
                best = score
    return best


def _assign(mock_grey, kept, files):
    """자리마다 어느 빈 판이 맞는지, 그림을 대 보고 정한다.

    크기로 짝지었더니 상태가 뒤섞였다. 카드는 보통/선택됨/사용불가가 그림만
    다르고 크기는 같아서, 크기로는 갈릴 수가 없다 -- 6안에서 평타 칸에
    사용불가 검은 판이 갔고 방패강타 칸에 올리브색 판이 갔다.

    그래서 실제로 그 자리에 대 보고 얼마나 맞는지로 고른다. 똑같이 생긴
    보통 카드끼리는 점수가 같아 아무거나 가지만, 그건 어차피 같은 그림이라
    상관없다. 다른 것은 제 자리에서만 점수가 낮다.

    가까운 것부터 집으면 앞에서 집은 판 때문에 뒤가 밀리므로, 전체 점수가
    가장 낮아지는 짝을 한 번에 고른다.
    """
    from scipy.optimize import linear_sum_assignment

    arts = []
    for path in files:
        art = Image.open(path)
        arts.append((art, art.size[0], art.size[1], path))
    if not arts:
        return {}, 0

    cost = np.full((len(kept), len(arts)), 9999.0)
    for i, item in enumerate(kept):
        for j, (art, w, h, _path) in enumerate(arts):
            # 크기가 아주 다르면 볼 것도 없다. 대 보는 값이 비싸서 먼저 친다.
            if abs(w - item["w"]) + abs(h - item["h"]) > BLANK_GAP:
                continue
            got = _fit(mock_grey, item, art)
            if got is not None:
                cost[i, j] = got
    rows, cols = linear_sum_assignment(cost)

    pairs, hits = {}, 0
    for i, j in zip(rows, cols):
        if cost[i, j] > BLANK_SCORE:
            continue
        _art, w, h, path = arts[j]
        pairs[i] = (os.path.basename(path), w, h)
        hits += 1
    return pairs, hits


def _tile(mock_path, folder, item, files):
    """붙어 있는 조각을 빈 판 여러 장으로 덮는다.

    3안 카드 두 장과 7안 카드 세 장은 한 덩어리로 오려져 있는데, 글자 지운
    회차는 카드를 한 장씩 잘랐다. 크기가 안 맞으니 짝이 없다.

    덩어리가 놓인 자리만 잘라 놓고 그 안에서 빈 판을 다시 맞춘다. 겹치지
    않는 것만 남기고, 덩어리의 절반도 못 덮으면 포기한다 -- 어설프게 덮으면
    글자 있는 판이 반쯤 비쳐 더 나쁘다.
    """
    crop = Image.open(mock_path).convert("RGB").crop(
        (item["x"], item["y"], item["x"] + item["w"], item["y"] + item["h"]))
    work = np.asarray(crop, dtype=float).mean(axis=2)

    # 한 장 놓을 때마다 그 자리를 가린다. 빈 카드는 서로 똑같아서, 안 가리면
    # 여섯 장이 전부 같은 칸을 짚는다 -- 3안 덩어리에서 카드 하나만 놓이고
    # 나머지가 겹침으로 걸러졌다.
    arts = [(path, Image.open(path)) for path in files]
    pieces, covered = [], 0
    for _ in range(6):
        best = None
        for path, part in arts:
            got = match(work, part)
            if got and (best is None or got[4] < best[0][4]):
                best = (got, path)
        if best is None or best[0][4] > BLANK_SCORE:
            break
        x, y, w, h, _score = best[0]
        pieces.append({"blank": os.path.basename(best[1]),
                       "x": x, "y": y, "w": w, "h": h})
        covered += w * h
        work[y:y + h, x:x + w] = 1e6

    if covered < item["w"] * item["h"] * 0.5:
        return None
    pieces.sort(key=lambda p: (p["y"], p["x"]))
    return pieces


def swap_blanks(folder, kept, mock_path=None):
    """자리는 그대로 두고 그림만 글자 지운 판으로 바꾼다.

    조각에 글자가 그려져 있으면 런타임 글자와 겹쳐 두 번 찍힌다 -- 2안을
    구워 보고 알았다. "기사"도 "90/100"도 판에 있고 위젯에도 있었다.

    자리는 글자 있는 조각으로 찾는다. 빈 판끼리는 서로 똑같아서 어느 칸인지
    그림으로 못 가리기 때문이다 -- 빈 판으로 자리를 찾았더니 3안 카드 여섯
    칸 중 세 칸이 비었다. 찾아 둔 자리에 그림만 갈아 끼운다.

    세 번에 걸쳐 짝을 짓는다.

      1. 제일 많이 맞는 회차 하나로. 섞으면 카드마다 결이 달라 보인다
      2. 남은 것은 회차를 가리지 않고. 결이 조금 다른 편이 글자가 두 번
         찍히는 것보다 낫다
      3. 그래도 남은 붙은 조각은 빈 판 여러 장으로 덮는다
    """
    rounds = _rounds(folder)
    if not rounds:
        return

    mock_grey = np.asarray(Image.open(mock_path).convert("RGB"),
                           dtype=float).mean(axis=2) if mock_path else None
    best, hits, picked = {}, 0, []
    for files in rounds:
        pairs, got = _assign(mock_grey, kept, files)
        if got > hits:
            best, hits, picked = pairs, got, files
    for index, (name, w, h) in best.items():
        kept[index]["blank"] = name
        # 빈 판은 원래 크기로 놓는다. 자리 크기에 맞춰 늘리면 몰딩이 뭉갠다
        # -- 조각을 쓰는 이유가 그것이다.
        kept[index]["blank_w"], kept[index]["blank_h"] = w, h

    left = [i for i in range(len(kept)) if not kept[i].get("blank")]
    if left:
        pool = [f for files in rounds for f in files]
        pairs, _ = _assign(mock_grey, [kept[i] for i in left], pool)
        for slot, (name, w, h) in pairs.items():
            item = kept[left[slot]]
            item["blank"], item["blank_w"], item["blank_h"] = name, w, h

    if mock_path:
        for item in kept:
            if item.get("blank"):
                continue
            pieces = _tile(mock_path, folder, item, picked or rounds[0])
            if pieces:
                item["pieces"] = pieces


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
