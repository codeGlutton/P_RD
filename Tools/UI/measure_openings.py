"""Find *every* opening in a UI frame, not just the biggest one.

왜 다시 만드나
--------------
``measure_inner_rect.py`` 는 실루엣 안에서 **가장 큰 빈 사각 하나**를 찾는다.
구멍이 하나인 틀에는 맞지만, 프레임 그림은 그렇지 않은 것이 많다.

    3열 전면 틀      구멍 3개
    요약 카드        초상화 구멍 + 본문 구멍 + 아래 띠
    설정 패널        바깥 테두리 안에 또 안쪽 테두리(이중 액자)

가장 큰 것 하나만 보면 나머지 칸은 못 본 채로 배치하게 된다. 지금까지 그렇게
하고 있었다.

어떻게 찾나
-----------
알파가 없는 픽셀을 **덩어리로 묶는다**(가로 런 union-find). 그중

* 실루엣 바깥으로 새는 덩어리는 버린다 -- 그건 그림 밖이지 구멍이 아니다,
* 너무 작은 덩어리도 버린다 -- 장식 사이 틈이다.

남은 덩어리마다 사각을 적는다. 구멍이 여럿이면 왼쪽 위부터 차례로 번호를 준다.

**이중 액자도 잡는다.** 바깥 테두리와 안쪽 테두리 사이가 채워져 있으면 구멍은
안쪽 하나로 잡히고, 비어 있으면 두 덩어리로 잡힌다. 어느 쪽인지 보고서에
칸 수로 드러난다.

쓰는 법:
    python Tools/UI/measure_openings.py <그림.png> [그림.png ...]
"""

import argparse
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:  # pragma: no cover
    sys.exit("PIL 이 없다: python -m pip install pillow")

CLEAR = 40          # 이보다 옅으면 빈 것으로 본다
MIN_AREA = 0.005    # 실루엣의 이 비율보다 작은 덩어리는 틈이다
OUT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/UIKit/_openings.txt")


class Union:
    def __init__(self):
        self.parent = []

    def make(self):
        self.parent.append(len(self.parent))
        return len(self.parent) - 1

    def find(self, node):
        while self.parent[node] != node:
            self.parent[node] = self.parent[self.parent[node]]
            node = self.parent[node]
        return node

    def join(self, left, right):
        left, right = self.find(left), self.find(right)
        if left != right:
            self.parent[right] = left


def clear_runs(alpha, y, x0, x1):
    """한 줄에서 비어 있는 구간."""
    out, start = [], None
    for x in range(x0, x1):
        if alpha[x, y] < CLEAR:
            if start is None:
                start = x
        elif start is not None:
            out.append((start, x))
            start = None
    if start is not None:
        out.append((start, x1))
    return out


def openings(image):
    alpha = image.getchannel("A")
    box = alpha.point(lambda v: 255 if v >= CLEAR else 0).getbbox()
    if box is None:
        return None, []
    x0, y0, x1, y1 = box
    pixels = alpha.load()

    union = Union()
    boxes = {}
    touches = set()         # 실루엣 가장자리에 닿은 덩어리 = 그림 밖
    previous = []

    for y in range(y0, y1):
        current = []
        for a, b in clear_runs(pixels, y, x0, x1):
            label = None
            for pa, pb, plabel in previous:
                if pb <= a:
                    continue
                if pa >= b:
                    break
                if label is None:
                    label = plabel
                else:
                    union.join(label, plabel)
            if label is None:
                label = union.make()
                boxes[label] = [a, y, b, y + 1]
            current.append((a, b, label))
            root = union.find(label)
            rect = boxes.setdefault(root, [a, y, b, y + 1])
            rect[0], rect[1] = min(rect[0], a), min(rect[1], y)
            rect[2], rect[3] = max(rect[2], b), max(rect[3], y + 1)
            # 실루엣 테두리에 닿으면 바깥과 이어진 빈 자리다.
            if a <= x0 or b >= x1 or y <= y0 or y + 1 >= y1:
                touches.add(root)
        previous = current

    merged, outside = {}, set()
    for label, rect in boxes.items():
        root = union.find(label)
        target = merged.setdefault(root, list(rect))
        target[0], target[1] = min(target[0], rect[0]), min(target[1], rect[1])
        target[2], target[3] = max(target[2], rect[2]), max(target[3], rect[3])
    for label in touches:
        outside.add(union.find(label))

    area = (x1 - x0) * (y1 - y0)
    holes = [tuple(rect) for root, rect in merged.items()
             if root not in outside
             and (rect[2] - rect[0]) * (rect[3] - rect[1]) >= area * MIN_AREA]
    holes.sort(key=lambda rect: (round(rect[1] / 40), rect[0]))
    return box, holes


def inscribed(image, hole):
    """구멍 안에 들어가는 **가장 큰 사각**.

    구멍의 바깥 사각(bbox)과 다르다. 둥근 구멍이면 bbox 는 원의 지름이지만,
    그 안에 들어가는 사각은 지름/√2 뿐이다. 실제로 칩 링은 bbox 95x95,
    들어가는 사각 69x67 로 38% 차이가 났다.

    쓰임이 다르다.
        bbox      장식이 끝나는 자리. 9-slice 마진이 여기까지 와야 한다
        들어가는 사각  글자·아이콘을 놓아도 삐져나오지 않는 자리
    """
    x0, y0, x1, y1 = hole
    alpha = image.getchannel("A").load()
    width = x1 - x0
    heights = [0] * width
    best = (0, None)
    for y in range(y0, y1):
        for index in range(width):
            heights[index] = heights[index] + 1 if alpha[x0 + index, y] < CLEAR else 0
        stack = []
        for index in range(width + 1):
            current = heights[index] if index < width else 0
            while stack and heights[stack[-1]] >= current:
                top = stack.pop()
                left = stack[-1] + 1 if stack else 0
                area = heights[top] * (index - left)
                if area > best[0]:
                    best = (area, (x0 + left, y - heights[top] + 1,
                                   x0 + index, y + 1))
            stack.append(index)
    return best[1] or hole


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("images", nargs="+")
    args = parser.parse_args()

    lines = ["# 프레임 그림의 구멍 전부",
             "# 구멍이 하나가 아닌 그림이 많다. 가장 큰 것만 보면 나머지 칸을 못 본다.",
             ""]
    for name in args.images:
        path = Path(name)
        image = Image.open(path).convert("RGBA")
        width, height = image.size
        box, holes = openings(image)
        lines.append(f"{path.name}  {width}x{height}")
        if box is None:
            lines.append("    전부 투명")
            continue
        lines.append(f"    실루엣 ({box[0]},{box[1]})-({box[2]},{box[3]})"
                     f"   구멍 {len(holes)}개")
        for index, (a, b, c, d) in enumerate(holes):
            ia, ib, ic, id_ = inscribed(image, (a, b, c, d))
            lines.append(
                f"    구멍{index}  겉 ({a:4d},{b:4d})-({c:4d},{d:4d})  {c - a:4d}x{d - b:4d}"
                f"   비율 ({a / width:.4f},{b / height:.4f})-({c / width:.4f},{d / height:.4f})")
            lines.append(
                f"            안 ({ia:4d},{ib:4d})-({ic:4d},{id_:4d})  {ic - ia:4d}x{id_ - ib:4d}"
                f"   비율 ({ia / width:.4f},{ib / height:.4f})"
                f"-({ic / width:.4f},{id_ / height:.4f})")
        lines.append("")

    OUT.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print("\n".join(lines))


if __name__ == "__main__":
    main()
