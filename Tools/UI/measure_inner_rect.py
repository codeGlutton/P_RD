"""Measure the usable interior of each UI part — not just its outline.

왜 만드나
---------
지금까지 그림의 **바깥**만 재고 있었다. 알파로 실루엣을 찾고, 밝기로 분할선을
찾았다. 그런데 배치가 실제로 필요한 값은 **안쪽**이다.

    칩 링      글자를 넣을 수 있는 안쪽 원은 어디까지인가
    단추 판    글자가 테두리 장식을 밟지 않는 안쪽 사각은 어디까지인가
    바깥 틀    콘텐츠가 들어갈 구멍은 정확히 어디인가
    초상화 틀  그림을 앉힐 안쪽 사각은 어디인가

이걸 몰라서 `chip * 0.20`, `chip * 0.44` 처럼 비율로 짐작해 왔다. 짐작이 틀리면
글자가 테두리를 밟거나 가운데가 안 맞는다.

두 가지를 잰다
--------------
**구멍** -- 안쪽이 비어 있는 것(틀·링). 실루엣 안에서 알파가 없는 가장 큰
사각을 찾는다. 히스토그램 방식이라 1920x1080 도 금방 끝난다.

**평평한 속** -- 안쪽이 채워진 것(단추·판). 가장자리에서 안으로 들어가며 색이
잦아드는 지점을 찾는다. 장식은 색이 튀고 채움면은 평평하다.

구멍이 실루엣의 4% 이상이면 구멍으로 보고, 아니면 평평한 속으로 잰다.

쓰는 법:
    python Tools/UI/measure_inner_rect.py Saved/UIKit/ConceptA Saved/UIKit/FrameA
"""

import argparse
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:  # pragma: no cover
    sys.exit("PIL 이 없다: python -m pip install pillow")

CLEAR = 40          # 이보다 옅으면 비었다고 본다
FLAT = 12.0         # 이웃 픽셀 차이가 이보다 작으면 평평하다
RUN = 5             # 평평함이 이만큼 이어지면 장식이 끝난 것으로 본다
HOLE_MIN = 0.04     # 실루엣의 이 비율보다 작으면 구멍이 아니라 틈이다


def opaque_box(image):
    """실루엣의 바깥 사각. 투명 여백을 뺀 진짜 그림 범위다."""
    return image.getchannel("A").point(lambda v: 255 if v >= CLEAR else 0).getbbox()


def largest_clear_rect(image, box):
    """실루엣 안에서 알파가 없는 가장 큰 사각.

    각 줄마다 "위로 몇 칸이 비었는가"를 세고, 그 히스토그램에서 가장 넓은
    직사각형을 찾는다. 픽셀마다 사방으로 넓히는 방식보다 훨씬 빠르다.
    """
    x0, y0, x1, y1 = box
    alpha = image.getchannel("A").load()
    width = x1 - x0
    heights = [0] * width
    best = (0, None)

    for y in range(y0, y1):
        for index in range(width):
            heights[index] = heights[index] + 1 if alpha[x0 + index, y] < CLEAR else 0
        # 히스토그램 안의 최대 직사각형. 스택으로 한 번에 훑는다.
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
    return best


def flat_inset(image, box):
    """채워진 그림의 안쪽 사각. 가장자리 장식이 끝나는 지점을 네 변에서 찾는다."""
    x0, y0, x1, y1 = box
    art = image.crop(box).convert("RGB")
    width, height = art.size
    pixels = art.load()

    def edge(series):
        calm = 0
        for index in range(len(series) - 1):
            a, b = series[index], series[index + 1]
            if sum(abs(a[c] - b[c]) for c in range(3)) <= FLAT:
                calm += 1
                if calm >= RUN:
                    return max(0, index - RUN + 1)
            else:
                calm = 0
        return len(series) // 3

    row = [pixels[x, height // 2] for x in range(width)]
    col = [pixels[width // 2, y] for y in range(height)]
    left, right = edge(row), edge(row[::-1])
    top, bottom = edge(col), edge(col[::-1])
    # 좌우·상하는 같은 모양이다. 작은 쪽을 택해 안쪽을 넉넉히 남긴다.
    left = right = min(left, right)
    top = bottom = min(top, bottom)
    return (x0 + left, y0 + top, x1 - right, y1 - bottom)


def centred(hole, box):
    """구멍이 실루엣 안에 떠 있는가.

    명패나 기둥처럼 속이 꽉 찬 그림은 구멍이 없는데, 모서리가 둥글어 생긴
    바깥쪽 빈 자리가 "가장 큰 빈 사각" 으로 잡힌다. 실제로 명패는 왼쪽 위
    귀퉁이 틈(317x19)을 구멍으로 내놨다. 진짜 구멍은 네 변 모두에서 떨어져
    있으므로, 한 변이라도 실루엣에 붙어 있으면 구멍이 아니다.
    """
    inset = max(4, min(box[2] - box[0], box[3] - box[1]) // 20)
    return (hole[0] >= box[0] + inset and hole[1] >= box[1] + inset
            and hole[2] <= box[2] - inset and hole[3] <= box[3] - inset)


def measure(path):
    image = Image.open(path).convert("RGBA")
    box = opaque_box(image)
    if box is None:
        return None
    silhouette = (box[2] - box[0]) * (box[3] - box[1])
    area, hole = largest_clear_rect(image, box)
    if hole is not None and area >= silhouette * HOLE_MIN and centred(hole, box):
        return box, hole, "구멍"
    return box, flat_inset(image, box), "평평한 속"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("folders", nargs="+")
    args = parser.parse_args()

    lines = ["# UI 부품의 안쪽 쓸 수 있는 자리",
             "# 겉 = 실루엣 사각(투명 여백 제외), 안 = 콘텐츠를 놓을 수 있는 사각",
             "# 비율은 텍스처 전체 크기 기준이라 어떤 크기로 늘려도 그대로 쓴다",
             ""]
    for folder in args.folders:
        for path in sorted(Path(folder).glob("*.png")):
            if path.name.startswith("_"):
                continue
            result = measure(path)
            if result is None:
                lines.append(f"{path.stem:32s} 전부 투명")
                continue
            box, inner, kind = result
            width, height = Image.open(path).size
            lines.append(
                f"{path.stem:32s} {width:4d}x{height:4d}  {kind:8s}\n"
                f"    겉 ({box[0]:4d},{box[1]:4d})-({box[2]:4d},{box[3]:4d})"
                f"   안 ({inner[0]:4d},{inner[1]:4d})-({inner[2]:4d},{inner[3]:4d})"
                f"   안 크기 {inner[2] - inner[0]:4d}x{inner[3] - inner[1]:4d}\n"
                f"    안 비율 ({inner[0] / width:.4f},{inner[1] / height:.4f})"
                f"-({inner[2] / width:.4f},{inner[3] / height:.4f})")
        lines.append("")

    out = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/UIKit/_inner_rects.txt")
    out.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print("\n".join(lines))


if __name__ == "__main__":
    main()
