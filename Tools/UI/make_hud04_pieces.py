# -*- coding: utf-8 -*-
"""보석 낱개와 선택 테두리를 만든다.

## 보석

AP 는 런타임이 개수를 세어 하나씩 켠다. 시안에서 뗀 보석 묶음은 네 개가 한
장이라 그대로는 못 쓴다. 알파가 비는 곳으로 잘라 낱개로 나눈다.

찬 것과 빈 것을 다른 판에서 가져온다. 시안의 기사 줄은 3/4 이라 넷째가 어둡고,
궁수 줄은 4/4 이라 전부 밝다 -- 두 판이 곧 두 상태다.

## 선택 테두리

시안에는 테두리만 따로 있지 않다. 골라진 칸을 통째로 다시 그려 놓아서, 빼면
안쪽 그림까지 딸려 온다. 그래서 여기서 만든다. 가운데가 빈 금색 테두리를
찍어 9슬라이스로 늘린다 -- 테두리는 늘려도 무늬가 안 뭉갠다.
"""
import os

import numpy as np
from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))
ART = os.path.join(HERE, "KayKitUIKit", "HUD04")


def split_gems(path):
    """보석 묶음을 알파가 비는 곳으로 잘라 낸다."""
    image = np.asarray(Image.open(path).convert("RGBA"))
    filled = (image[..., 3] > 10).mean(axis=0) > 0.15
    pieces, start = [], None
    for index, on in enumerate(filled):
        if on and start is None:
            start = index
        elif not on and start is not None:
            pieces.append((start, index))
            start = None
    if start is not None:
        pieces.append((start, len(filled)))
    # 한두 픽셀짜리는 뺀다. 뺄셈이 남긴 잡티지 보석이 아니다 -- 이걸 안
    # 거르면 첫 조각이 1픽셀 폭으로 나온다.
    return [image[:, a:b] for a, b in pieces if b - a >= 8]


lit = split_gems(os.path.join(ART, "KK_HUD04_bottom_status_center__ap_gems.png"))
dim = split_gems(os.path.join(ART, "KK_HUD04_bottom_status_left__ap_gems.png"))
if not lit or not dim:
    raise RuntimeError("보석을 못 갈랐다")

Image.fromarray(lit[0]).save(os.path.join(ART, "KK_HUD04_ap_pip_lit.png"))
# 어두운 것은 기사 줄의 마지막 칸이다. 3/4 이라 넷째가 비어 있다.
Image.fromarray(dim[-1]).save(os.path.join(ART, "KK_HUD04_ap_pip_dim.png"))

# 선택 테두리. 가운데가 비어 있어야 밑그림이 비친다.
SIZE, EDGE, RADIUS = 96, 6, 18
outline = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
draw = ImageDraw.Draw(outline)
draw.rounded_rectangle([1, 1, SIZE - 2, SIZE - 2], RADIUS,
                       outline=(255, 209, 82, 255), width=EDGE)
draw.rounded_rectangle([1, 1, SIZE - 2, SIZE - 2], RADIUS,
                       outline=(255, 240, 180, 160), width=2)
outline.save(os.path.join(ART, "KK_HUD04_selected_outline.png"))

print("보석 %d + %d, 테두리 1" % (len(lit), len(dim)))
