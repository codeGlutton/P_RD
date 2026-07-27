# -*- coding: utf-8 -*-
"""빈 판과 채운 판을 견줘 무엇이 어디에 놓였는지 재고 자리표를 뽑는다.

## 왜 이제야 정확한가

여태 채워진 시안 전체에서 판 자리를 잘라 내 빈 판과 견줬다. 두 그림이 따로
만들어진 것이라 테두리가 몇 px 어긋났고, 그 띠가 판을 빙 둘러 하나의 큰
상자로 잡히거나 얼굴과 글자를 이어 붙였다. 걸러 내는 규칙을 얹을수록 어느
판에서 무엇이 걸러지는지 알 수 없어졌다.

이번 판은 빈 판을 기준으로 그 안만 채워 만든 것이라 크기가 한 픽셀도 다르지
않다. 빼면 채워 넣은 것만 정확히 남는다.

## 무엇으로 알아보나

색이 알려 준다. 추측이 아니다.

    초록  체력 막대        파랑 마름모  AP 보석
    빨강  적 체력 막대     보라 원      상태 표시
    흰    이름과 수치      나머지 큰 덩어리  초상과 아이콘

## 남는 몫

이름과 수치는 둘 다 흰 글자라 색으로는 안 갈린다. 자리로 가른다 -- 이름은
위, 체력 수치는 막대 옆, AP 수치는 오른쪽 아래. 판 종류가 일곱이고 이 규칙이
그 안에서만 쓰이므로 시안마다 흔들리지 않는다.

    python measure_slots.py --mockup 1 --blank <빈 판 폴더> --filled <채운 폴더>
"""
import argparse
import io
import os
import sys

import numpy as np
from PIL import Image
from scipy import ndimage


def added(blank_path, filled_path):
    """채운 판에서 빈 판을 뺀 자리. 크기가 같아 어긋남이 없다."""
    blank = np.asarray(Image.open(blank_path).convert("RGBA"), dtype=float)
    filled = np.asarray(Image.open(filled_path).convert("RGBA"), dtype=float)
    if blank.shape != filled.shape:
        return None, None, None
    gap = np.abs(filled[:, :, :3] - blank[:, :, :3]).mean(axis=2)
    solid = (filled[:, :, 3] > 180)
    return filled[:, :, :3], solid & (gap > 30), blank.shape[1::-1]


def blobs(mask, floor=60, kernel=(3, 5)):
    """이어진 덩어리들의 칸. 왼쪽 위부터."""
    mask = ndimage.binary_closing(mask, np.ones(kernel))
    labels, count = ndimage.label(mask)
    out = []
    if count:
        sizes = ndimage.sum(mask, labels, range(1, count + 1))
        for i in np.argsort(sizes)[::-1]:
            if sizes[i] < floor:
                break
            ys, xs = np.where(labels == i + 1)
            out.append((int(xs.min()), int(ys.min()),
                        int(xs.max() - xs.min() + 1),
                        int(ys.max() - ys.min() + 1)))
    out.sort(key=lambda e: (e[1], e[0]))
    return out


def union(boxes):
    if not boxes:
        return None
    x0 = min(b[0] for b in boxes)
    y0 = min(b[1] for b in boxes)
    x1 = max(b[0] + b[2] for b in boxes)
    y1 = max(b[1] + b[3] for b in boxes)
    return (x0, y0, x1 - x0, y1 - y0)


def line_of(boxes, gap=18):
    """가로로 이웃한 글자 덩어리를 한 줄로 잇는다."""
    rows, pool = [], sorted(boxes, key=lambda b: (b[1], b[0]))
    while pool:
        head = pool.pop(0)
        line = [head]
        changed = True
        while changed:
            changed = False
            for other in list(pool):
                span = union(line)
                near_y = (other[1] < span[1] + span[3] + 6
                          and other[1] + other[3] > span[1] - 6)
                near_x = (other[0] < span[0] + span[2] + gap
                          and other[0] + other[2] > span[0] - gap)
                if near_y and near_x:
                    line.append(other)
                    pool.remove(other)
                    changed = True
        rows.append(union(line))
    rows.sort(key=lambda b: (b[1], b[0]))
    return rows


def measure(blank_path, filled_path):
    """판 하나에서 색깔별로 무엇이 어디 있는지."""
    rgb, mask, size = added(blank_path, filled_path)
    if rgb is None:
        return None
    r, g, b = rgb[:, :, 0], rgb[:, :, 1], rgb[:, :, 2]

    green = mask & (g > 130) & (r > 90) & (r < 215) & (b < 120)
    red = mask & (r > 140) & (g < 95) & (b < 95)
    blue = mask & (b > 150) & (g > 100) & (r < 140)
    white = mask & (r > 200) & (g > 200) & (b > 195)
    rest = mask & ~(green | red | blue | white)

    return {
        "size": size,
        "green": union(blobs(green, 40)),
        "red": union(blobs(red, 40)),
        "pips": blobs(blue, 60, (3, 3)),
        "text": line_of(blobs(white, 30)),
        "rest": blobs(rest, 200, (5, 5)),
    }


PLATES = (
    ("top_left_panel", "round"),
    ("top_right_panel", "objective"),
    ("portrait_slot_01", "turn"),
    ("party_status_row_01", "party"),
    ("action_slot_03", "skill"),
    ("enemy_info_panel", "enemy"),
    ("end_turn_button", "endturn"),
)


def main():
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8")
    ap = argparse.ArgumentParser()
    ap.add_argument("--blank", required=True)
    ap.add_argument("--filled", required=True)
    args = ap.parse_args()

    for stem, role in PLATES:
        blank = os.path.join(args.blank, stem + ".png")
        filled = os.path.join(args.filled, stem + ".png")
        if not (os.path.exists(blank) and os.path.exists(filled)):
            print("%-10s 판 없음" % role)
            continue
        got = measure(blank, filled)
        if got is None:
            print("%-10s 크기 다름" % role)
            continue
        print("%-10s %s" % (role, got["size"]))
        if got["green"]:
            print("     초록막대 %s" % (got["green"],))
        if got["red"]:
            print("     빨강막대 %s" % (got["red"],))
        if got["pips"]:
            xs = [p[0] for p in got["pips"]]
            step = ((max(xs) - min(xs)) / float(len(xs) - 1)
                    if len(xs) > 1 else 0)
            print("     보석 %d개 x=%d y=%d 한변 %d 간격 %.1f"
                  % (len(got["pips"]), min(xs), got["pips"][0][1],
                     max(p[2] for p in got["pips"]), step))
        for i, box in enumerate(got["text"], 1):
            print("     글자 %d %s" % (i, box))
        for i, box in enumerate(got["rest"][:4], 1):
            print("     그림 %d %s" % (i, box))
        print()


if __name__ == "__main__":
    main()
