"""Cut a generated UI component sheet into one PNG per part.

시안은 부품 여러 개가 한 장에 담겨 온다. 언리얼에 넣으려면 부품마다 텍스처가
따로 있어야 하므로, 알파가 이어진 덩어리를 하나씩 떼어낸다.

어떻게 찾나
-----------
알파를 기준선으로 이진화한 뒤, **가로 런(run) 단위 union-find** 로 붙은 덩어리를
센다. 픽셀 단위 BFS 는 160만 픽셀에서 느리다. 런 단위면 한 줄에 몇 개뿐이라
같은 결과를 훨씬 빨리 얻는다.

작은 얼룩은 버린다. 그림 모델이 남긴 점 하나까지 텍스처로 만들 필요는 없다.

결과
----
    <out>/part_00.png ...            부품 하나씩 (투명 여백 8px)
    <out>/_contact.png               번호가 찍힌 대조표. 이걸 보고 이름을 정한다
    <out>/_parts.txt                 번호 · 원본 좌표 · 크기

이름은 여기서 정하지 않는다. 무엇이 단추이고 무엇이 칩인지는 사람이 보고
정해야 하고, 잘못 붙인 이름은 나중에 배선까지 끌고 간다.

쓰는 법:
    python Tools/UI/slice_component_kit.py <시트.png> <내보낼 폴더> [--min 40]
"""

import argparse
import sys
from pathlib import Path

try:
    from PIL import Image, ImageDraw
except ImportError:  # pragma: no cover
    sys.exit("PIL 이 없다: python -m pip install pillow")

ALPHA_FLOOR = 24        # 이보다 옅으면 배경으로 본다
MIN_SIDE = 40           # 가로세로 둘 다 이보다 작으면 얼룩으로 버린다
PAD = 8                 # 잘라낸 그림 둘레에 남길 투명 여백


class Union:
    """런 묶기용 union-find. 덩어리 번호를 합치는 데만 쓴다."""

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


def runs_of(alpha, width, y):
    """한 줄에서 알파가 살아 있는 구간을 [시작, 끝) 로 뽑는다."""
    out = []
    start = None
    base = y * width
    for x in range(width):
        if alpha[base + x] >= ALPHA_FLOOR:
            if start is None:
                start = x
        elif start is not None:
            out.append((start, x))
            start = None
    if start is not None:
        out.append((start, width))
    return out


def find_parts(image, min_side):
    width, height = image.size
    alpha = image.getchannel("A").tobytes()

    union = Union()
    previous = []           # 윗줄의 (시작, 끝, 라벨)
    boxes = {}              # 라벨 -> [x0, y0, x1, y1]

    for y in range(height):
        current = []
        for x0, x1 in runs_of(alpha, width, y):
            label = None
            for px0, px1, plabel in previous:
                if px1 <= x0:       # 윗줄 런이 왼쪽에 완전히 있음
                    continue
                if px0 >= x1:       # 오른쪽으로 지나쳤다. 정렬돼 있으니 그만
                    break
                if label is None:
                    label = plabel
                else:
                    union.join(label, plabel)
            if label is None:
                label = union.make()
                boxes[label] = [x0, y, x1, y + 1]
            current.append((x0, x1, label))
            box = boxes.setdefault(union.find(label), [x0, y, x1, y + 1])
            box[0] = min(box[0], x0)
            box[1] = min(box[1], y)
            box[2] = max(box[2], x1)
            box[3] = max(box[3], y + 1)
        previous = current

    # 합쳐진 라벨끼리 상자를 다시 모은다.
    merged = {}
    for label, box in boxes.items():
        root = union.find(label)
        if root not in merged:
            merged[root] = list(box)
            continue
        target = merged[root]
        target[0] = min(target[0], box[0])
        target[1] = min(target[1], box[1])
        target[2] = max(target[2], box[2])
        target[3] = max(target[3], box[3])

    parts = [tuple(box) for box in merged.values()
             if (box[2] - box[0]) >= min_side and (box[3] - box[1]) >= min_side]
    # 읽는 순서대로: 위에서 아래로, 같은 줄이면 왼쪽부터.
    parts.sort(key=lambda box: (round(box[1] / 40), box[0]))
    return parts


def contact_sheet(image, parts):
    """번호를 찍은 대조표. 어떤 번호가 무슨 부품인지 눈으로 짝지으라고 만든다."""
    sheet = Image.new("RGB", image.size, (46, 48, 52))
    sheet.paste(image, (0, 0), image)
    draw = ImageDraw.Draw(sheet)
    for index, (x0, y0, x1, y1) in enumerate(parts):
        draw.rectangle((x0, y0, x1 - 1, y1 - 1), outline=(255, 90, 90), width=2)
        label = str(index)
        draw.rectangle((x0, y0, x0 + 12 + 9 * len(label), y0 + 20), fill=(255, 90, 90))
        draw.text((x0 + 6, y0 + 5), label, fill=(20, 20, 20))
    return sheet


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("sheet")
    parser.add_argument("out")
    parser.add_argument("--min", type=int, default=MIN_SIDE)
    args = parser.parse_args()

    image = Image.open(args.sheet)
    if image.mode != "RGBA":
        sys.exit(f"알파가 없다: {args.sheet} (mode={image.mode})")

    parts = find_parts(image, args.min)
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)

    lines = [f"# {Path(args.sheet).name}  {image.size[0]}x{image.size[1]}",
             f"# 부품 {len(parts)}개  (최소 변 {args.min}px)",
             "# 번호  원본좌표(x,y)  크기(w,h)"]
    for index, (x0, y0, x1, y1) in enumerate(parts):
        piece = Image.new("RGBA", (x1 - x0 + PAD * 2, y1 - y0 + PAD * 2), (0, 0, 0, 0))
        piece.paste(image.crop((x0, y0, x1, y1)), (PAD, PAD))
        piece.save(out / f"part_{index:02d}.png")
        lines.append(f"{index:3d}  ({x0:4d},{y0:4d})  {x1 - x0:4d}x{y1 - y0:4d}")

    contact_sheet(image, parts).save(out / "_contact.png")
    (out / "_parts.txt").write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"{Path(args.sheet).name}: 부품 {len(parts)}개 -> {out}")


if __name__ == "__main__":
    main()
