# -*- coding: utf-8 -*-
"""초록 배경을 걷어 내고 빈 테두리를 잘라 낸다.

## 왜 이걸 따로 두나

생성기에서 받은 그림은 초록 판 위에 물건 하나가 놓여 있다. 그대로 넣으면
초록이 화면에 그대로 뜨고, 걷어 내기만 하면 사방이 빈 채로 남아 자리를
잡을 때 빈 곳까지 자리로 잡힌다 -- 구역을 아무리 맞춰도 그림이 가운데로 안
온다.

그래서 둘을 한꺼번에 한다. **걷어 내고, 남은 것에 딱 맞게 자른다.**

## 초록을 어떻게 가려내나

색 거리로만 자르면 물건 안의 초록빛(지도의 풀색, 금속의 반사)까지 뚫린다.
그래서 **바깥에서 이어진 초록만** 지운다 -- 네 귀퉁이에서 시작해 번져
나가며 이어진 것만 걷는다. 안쪽에 갇힌 초록은 물건의 일부로 본다.

가장자리는 한 겹 부드럽게 깎는다. 안 깎으면 잘린 자리에 초록 실선이 남는다.
"""
import io
import os
import sys
from collections import deque

from PIL import Image


def _is_key(px, key, tol):
    """@brief 이 화소가 배경색인가. @return 참이면 배경"""
    return (abs(px[0] - key[0]) <= tol[0]
            and abs(px[1] - key[1]) <= tol[1]
            and abs(px[2] - key[2]) <= tol[2])


def strip_key(image, key=(0, 255, 0), tol=(90, 60, 90)):
    """@brief 바깥에서 이어진 배경색을 지운다. @return RGBA 이미지"""
    image = image.convert("RGBA")
    w, h = image.size
    px = image.load()

    seen = bytearray(w * h)
    queue = deque()
    for x in range(w):
        for y in (0, h - 1):
            queue.append((x, y))
    for y in range(h):
        for x in (0, w - 1):
            queue.append((x, y))

    while queue:
        x, y = queue.popleft()
        if x < 0 or y < 0 or x >= w or y >= h:
            continue
        i = y * w + x
        if seen[i]:
            continue
        seen[i] = 1
        if not _is_key(px[x, y], key, tol):
            continue
        px[x, y] = (0, 0, 0, 0)
        queue.extend(((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)))

    # 잘린 자리에 남는 초록 실선을 한 겹 깎는다.
    for y in range(h):
        for x in range(w):
            if px[x, y][3] == 0:
                continue
            near_hole = any(
                0 <= x + dx < w and 0 <= y + dy < h
                and px[x + dx, y + dy][3] == 0
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
            if not near_hole:
                continue
            r, g, b, a = px[x, y]
            if g > r + 40 and g > b + 40:
                px[x, y] = (0, 0, 0, 0)
    return image


def trim(image, threshold=8):
    """@brief 비어 있는 테두리를 잘라 낸다. @return 잘린 이미지"""
    image = image.convert("RGBA")
    alpha = image.getchannel("A").point(lambda v: 255 if v > threshold else 0)
    box = alpha.getbbox()
    return image.crop(box) if box else image


def cutout(src, dest, key=(0, 255, 0), tol=(90, 60, 90)):
    """@brief 걷어 내고 잘라 저장한다. @return (원래 크기, 잘린 크기)"""
    image = Image.open(src)
    before = image.size
    if image.mode != "RGBA" or _looks_keyed(image, key, tol):
        image = strip_key(image, key, tol)
    out = trim(image)
    os.makedirs(os.path.dirname(dest), exist_ok=True)
    out.save(dest)
    return before, out.size


def _looks_keyed(image, key, tol):
    """@brief 귀퉁이가 배경색인가. 이미 투명한 그림은 건드리지 않는다."""
    rgba = image.convert("RGBA")
    w, h = rgba.size
    px = rgba.load()
    corners = (px[0, 0], px[w - 1, 0], px[0, h - 1], px[w - 1, h - 1])
    return any(c[3] > 0 and _is_key(c, key, tol) for c in corners)


if __name__ == "__main__":
    for pair in sys.argv[1:]:
        src, dest = pair.split("|")
        before, after = cutout(src, dest)
        print("%-34s %sx%s -> %sx%s" % (
            os.path.basename(dest), before[0], before[1], after[0], after[1]))
