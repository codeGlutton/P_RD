# -*- coding: utf-8 -*-
"""용병 고용 화면 시안에서 판을 뜯고 글자 자리를 잰다.

## 이 시안이 다른 점

완성본과 빈 판이 한 픽셀도 안 어긋나게 왔다. 그래서 전투 HUD 때 만들었던
것들 -- 조각을 시안 위에서 밀어 자리를 찾고, 색과 크기로 역할을 판정하고,
빈 판 짝을 맞추던 도구 천이백 줄 -- 이 전부 필요 없다.

빈 판에서 판을 뜯고, 두 장을 빼서 글자 자리를 얻는다. 그게 전부다.

## 판을 어떻게 가르나

빈 판은 마젠타 바탕이라 배경과 판이 색으로 갈린다. 이력서는 게시판 위에
붙어 있어 한 덩어리로 잡히므로, 양피지 색으로 한 번 더 가른다.

예산 부족한 이력서는 흐리게 그려져 있어 양피지 색에서 샌다. 여섯 장이
가지런한 격자라 빠진 자리를 이웃에서 채운다 -- 색 규칙을 느슨하게 풀면
나뭇결까지 딸려 온다.

    python cut_hire_screen.py
"""
import io
import json
import os
import sys

import numpy as np
from PIL import Image
from scipy import ndimage

HERE = r"D:/UnrealProjects/P_RD_develop/시안2/용병고용화면"
COMPLETE = os.path.join(HERE, "mercenary_select_complete.png")
BLANK = os.path.join(HERE, "mercenary_select_blank_magenta.png")

#: 상태 표시는 판에 안 그려져 있다. 게임이 돌면서 켜고 끄는 것이라, 판에
#: 박히면 그 카드만 영영 그 상태로 남는다 -- 앞 판이 그래서 6번 이력서가
#: 늘 흐렸고 3번은 늘 금테두리였다.
STATE_ART = ("state_selected_frame.png", "state_seal.png")
OUT = os.path.join(HERE, "_조각")

KEY = np.array([255.0, 0.0, 255.0])
REACH = 60.0

#: 글자·아이콘으로 볼 최소 밝기 차이. 이보다 작으면 판 결의 흔들림이다.
CONTENT_GAP = 26


def unkey(path):
    """마젠타를 지워 알파로 만든다."""
    rgb = np.asarray(Image.open(path).convert("RGB"), dtype=float)
    far = np.sqrt(((rgb - KEY) ** 2).sum(axis=2))
    alpha = np.where(far < REACH, 0, 255).astype(np.uint8)
    return rgb, alpha


def lumps(mask, floor):
    out = []
    labels, count = ndimage.label(mask)
    if not count:
        return out
    sizes = ndimage.sum(mask, labels, range(1, count + 1))
    for i in np.argsort(sizes)[::-1]:
        if sizes[i] < floor:
            break
        ys, xs = np.where(labels == i + 1)
        out.append([int(xs.min()), int(ys.min()),
                    int(xs.max() - xs.min() + 1),
                    int(ys.max() - ys.min() + 1)])
    return out


def cards_of(rgb, alpha):
    """이력서 여섯 장. 격자라 빠진 자리를 이웃에서 채운다."""
    r, g, b = rgb[:, :, 0], rgb[:, :, 1], rgb[:, :, 2]
    paper = ((alpha > 0) & (r > 150) & (g > 140) & (b > 100)
             & (r - b < 130) & (np.abs(r - g) < 45))
    paper = ndimage.binary_opening(
        ndimage.binary_closing(paper, np.ones((9, 9))), np.ones((11, 11)))

    found = [c for c in lumps(paper, 40000) if 300 < c[2] < 460]
    if len(found) < 2:
        raise SystemExit("이력서를 못 찾았습니다")

    # 열과 행을 이웃에서 읽는다. 여섯 자리를 채운 뒤 넓이는 가운데값으로.
    xs = sorted({c[0] for c in found})
    ys = sorted({c[1] for c in found})
    cols = merge(xs, 60)
    rows = merge(ys, 60)
    wide = int(np.median([c[2] for c in found]))
    tall = int(np.median([c[3] for c in found]))

    out = []
    for row in rows:
        for col in cols:
            near = [c for c in found
                    if abs(c[0] - col) < 60 and abs(c[1] - row) < 60]
            out.append(near[0] if near else [col, row, wide, tall])
    return out


def merge(values, gap):
    """가까운 값끼리 묶어 대표값 하나로."""
    out = []
    for v in values:
        if out and v - out[-1][-1] <= gap:
            out[-1].append(v)
        else:
            out.append([v])
    return [int(np.median(group)) for group in out]


def content_of(full, blank, box):
    """두 장을 빼서 글자·아이콘이 놓인 자리."""
    x, y, w, h = box
    gap = np.abs(full[y:y + h, x:x + w] - blank[y:y + h, x:x + w]).mean(axis=2)
    inner = ndimage.binary_erosion(np.ones((h, w), bool), np.ones((7, 7)))
    mask = ndimage.binary_closing(inner & (gap > CONTENT_GAP), np.ones((3, 7)))
    floor = max(140, int(w * h / 900.0))
    out = []
    for spot in lumps(mask, floor)[:14]:
        if spot[2] < 9 or spot[3] < 9:
            continue
        if spot[2] > w * 0.94 and spot[3] > h * 0.94:
            continue
        out.append(spot)
    return join(out)


def join(boxes, gap=26):
    """한 줄에 이어진 낱자를 한 칸으로 묶는다.

    글자는 획이 끊긴 자리마다 따로 잡힌다 -- "HP 100" 이 둘로, "40 골드" 가
    셋으로 나온다. 위젯은 줄 단위로 놓으므로 묶어야 쓸 수 있다.

    세로 범위는 **처음 상자**의 것으로 잰다. 자라나는 줄 전체로 재면 줄이
    커질수록 세로가 넓어져 아래 줄까지 빨아들인다 -- 이력서 열네 칸이 네
    칸으로 뭉쳤다.
    """
    left = sorted(boxes, key=lambda e: (e[1], e[0]))
    out = []
    while left:
        seed = left.pop(0)
        line = [seed]
        top, bottom = seed[1], seed[1] + seed[3]
        changed = True
        while changed:
            changed = False
            x0 = min(b[0] for b in line)
            x1 = max(b[0] + b[2] for b in line)
            for other in list(left):
                # 키가 크게 다르면 다른 것이다. 초상(131px)이 옆에 붙은
                # 이름 글자(27px)를 삼켜 이름 자리가 통째로 사라졌다.
                tall = max(bottom - top, other[3])
                short = min(bottom - top, other[3])
                if tall > short * 1.9:
                    continue
                share = min(bottom, other[1] + other[3]) - max(top, other[1])
                if share <= short * 0.5:
                    continue
                if other[0] > x1 + gap or other[0] + other[2] < x0 - gap:
                    continue
                line.append(other)
                left.remove(other)
                changed = True
        out.append([min(b[0] for b in line), min(b[1] for b in line),
                    max(b[0] + b[2] for b in line) - min(b[0] for b in line),
                    max(b[1] + b[3] for b in line) - min(b[1] for b in line)])
    out.sort(key=lambda e: (e[1], e[0]))
    return out


def main():
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8")
    blank_rgb, alpha = unkey(BLANK)
    full_rgb = np.asarray(Image.open(COMPLETE).convert("RGB"), dtype=float)
    if full_rgb.shape != blank_rgb.shape:
        raise SystemExit("두 장의 크기가 다릅니다")

    board, bar = sorted(lumps(alpha > 0, 100000), key=lambda e: e[1])[:2]
    cards = cards_of(blank_rgb, alpha)

    pieces = [("board", board), ("bottombar", bar)]
    pieces += [("card_%d" % i, c) for i, c in enumerate(cards)]

    os.makedirs(OUT, exist_ok=True)
    art = Image.fromarray(np.dstack(
        [blank_rgb.astype(np.uint8), alpha]), "RGBA")

    rows = []
    for name, (x, y, w, h) in pieces:
        art.crop((x, y, x + w, y + h)).save(
            os.path.join(OUT, name + ".png"))
        boxes = content_of(full_rgb, blank_rgb, (x, y, w, h))
        rows.append({"name": name, "rect": [x, y, w, h], "boxes": boxes})
        print("  %-10s x=%4d y=%3d %4dx%-4d  내용 %2d칸"
              % (name, x, y, w, h, len(boxes)))

    # 배경은 빈 판에 없다(마젠타로 덮여 있다). 완성본에서 통째로 뜬다.
    Image.open(COMPLETE).convert("RGB").save(os.path.join(OUT, "backdrop.png"))

    # 상태 낱장은 그대로 옮긴다. 카드 위에 얹을 것이라 자를 것이 없다.
    for name in STATE_ART:
        src = os.path.join(HERE, name)
        if os.path.exists(src):
            Image.open(src).convert("RGBA").save(os.path.join(OUT, name))
            print("  %-10s 상태 그림" % os.path.splitext(name)[0])

    path = os.path.join(OUT, "pieces.json")
    with io.open(path, "w", encoding="utf-8") as handle:
        json.dump(rows, handle, ensure_ascii=False, indent=1)
    print()
    print("판 %d장 + 배경 -> %s" % (len(rows), OUT))


if __name__ == "__main__":
    main()
