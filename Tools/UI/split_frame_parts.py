"""Cut the monolithic 3-column frame into composable parts.

왜 나누나
---------
받은 그림은 1920x1080 통짜다. 그대로 쓰면

* 화면 비율이 16:9 가 아닌 폰에서 나무가 늘어난다(요즘 폰은 20:9 가 흔하다),
* 열 비율이 그림에 박혀 있어 두 열짜리·네 열짜리 화면에 못 쓴다,
* 열 경계를 조금 옮기고 싶어도 그림을 새로 받아야 한다.

그림 자체는 나눌 수 있게 그려져 있다. 바깥 틀과 세로 기둥이 서로 안 겹친다.
그래서 둘로 자른다.

    T_KitA_Frame_Outer     바깥 틀만. 기둥 자리는 비운다. 9-slice 로 아무 크기나
    T_KitA_Frame_Divider   세로 기둥 하나. 위아래 장식만 고정하고 가운데를 늘린다

이러면 배치는 "틀 한 장 + 기둥 N개를 원하는 x 에" 가 된다. 열 개수도 위치도
코드가 정한다.

재는 방법
---------
알파만 본다. 세로로 눌러 알파 합이 큰 구간이 기둥, 가로로 눌러 큰 구간이
위아래 띠다. 바깥 틀의 좌우 기둥은 그림 끝에 붙어 있으므로 가장자리 것으로
가려낸다.

쓰는 법:
    python Tools/UI/split_frame_parts.py <프레임.png> <내보낼 폴더>
"""

import argparse
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:  # pragma: no cover
    sys.exit("PIL 이 없다: python -m pip install pillow")

SOLID = 0.55        # 알파 평균이 이 비율을 넘으면 나무가 채운 줄로 본다
FLOOR = 40          # 이보다 옅은 픽셀은 없는 것으로 친다
FEATHER = 10        # 메운 자리 가장자리를 이만큼 부드럽게 섞는다


def runs(flags):
    """True 가 이어지는 구간 [시작, 끝)."""
    out, start = [], None
    for index, value in enumerate(flags):
        if value and start is None:
            start = index
        elif not value and start is not None:
            out.append((start, index))
            start = None
    if start is not None:
        out.append((start, len(flags)))
    return out


def analyse(image):
    width, height = image.size
    alpha = image.getchannel("A").load()

    # 세로 줄마다 "나무가 차 있는 정도". 기둥은 위아래로 길게 차 있다.
    column_fill = []
    for x in range(width):
        filled = sum(1 for y in range(0, height, 2) if alpha[x, y] >= FLOOR)
        column_fill.append(filled / len(range(0, height, 2)))
    row_fill = []
    for y in range(height):
        filled = sum(1 for x in range(0, width, 2) if alpha[x, y] >= FLOOR)
        row_fill.append(filled / len(range(0, width, 2)))

    pillars = runs([value >= SOLID for value in column_fill])
    bands = runs([value >= SOLID for value in row_fill])
    return pillars, bands


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("png")
    parser.add_argument("out")
    args = parser.parse_args()

    image = Image.open(args.png).convert("RGBA")
    width, height = image.size
    pillars, bands = analyse(image)

    report = [f"# {Path(args.png).name}  {width}x{height}",
              f"# 세로 기둥 {len(pillars)}개: " + ", ".join(f"{a}~{b}" for a, b in pillars),
              f"# 가로 띠 {len(bands)}개: " + ", ".join(f"{a}~{b}" for a, b in bands)]

    if len(pillars) < 3 or len(bands) < 2:
        report.append("# 기대한 모양이 아니다(기둥 3개 이상 · 띠 2개 이상). 그만둔다.")
        print("\n".join(report))
        return

    left, right = pillars[0], pillars[-1]
    inner = pillars[1:-1]
    top, bottom = bands[0], bands[-1]

    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)

    # ── 바깥 틀. 안쪽 기둥을 지운다.
    #
    # 기둥의 위아래 캡(작은 황동 장식)은 띠 위로 걸쳐 그려져 있다. 띠 사이만
    # 지우면 그 걸친 부분이 띠에 붙어 남는다 -- 실제로 그렇게 나왔다. 캡은
    # 기둥의 것이므로 띠 구간도 지우되, 띠에 구멍이 나면 안 되니 **옆의 깨끗한
    # 나무를 복사해 메운다.** 띠는 가로로 이어지는 나뭇결이라 티가 안 난다.
    outer = image.copy()
    alpha = image.getchannel("A").load()
    blank = Image.new("RGBA", (1, 1), (0, 0, 0, 0))

    def cap_span(a, b):
        """캡까지 포함한 실제 폭.

        analyse() 가 준 기둥 폭은 **위아래로 길게 이어진** 부분만이라 캡보다
        좁다. 캡은 짧고 넓어서 그 문턱을 못 넘는다. 그대로 지우면 캡 가장자리
        4px 이 띠에 붙어 남는다 -- 실제로 그렇게 나왔다. 캡이 있는 줄에서
        기둥 중심을 지나는 구간을 직접 재서 넓은 쪽을 쓴다.
        """
        middle = (a + b) // 2
        low, high = a, b
        for y in range(top[1], min(top[1] + 70, height)):
            if alpha[middle, y] < FLOOR:
                continue
            x = middle
            while x > 0 and alpha[x - 1, y] >= FLOOR:
                x -= 1
            low = min(low, x)
            x = middle
            while x < width - 1 and alpha[x + 1, y] >= FLOOR:
                x += 1
            high = max(high, x + 1)
        return low, high

    bleed = 3               # 경계 부드러운 픽셀까지 마저 걷어낸다
    for raw_a, raw_b in inner:
        a, b = cap_span(raw_a, raw_b)
        a, b = a - bleed, b + bleed
        outer.paste(blank.resize((b - a, bottom[0] - top[1])), (a, top[1]))
        # 어디서 나무를 떠올까. 기둥에서 멀고 그림 안에 있는 쪽을 고른다.
        span = b - a
        donor = a - span * 3 if a - span * 3 > left[1] else b + span * 3
        for band in (top, bottom):
            # 가장자리를 부드럽게 섞는다. 그냥 붙이면 나뭇결이 어긋나는 자리에
            # 1px 세로 줄이 남는다 -- 기둥을 다른 자리에 놓으면 그 줄이 드러난다.
            wide = Image.new("L", (span + FEATHER * 2, band[1] - band[0]), 255)
            for step in range(FEATHER):
                level = int(255 * (step + 1) / (FEATHER + 1))
                for y in range(wide.size[1]):
                    wide.putpixel((step, y), level)
                    wide.putpixel((wide.size[0] - 1 - step, y), level)
            patch = image.crop((donor - FEATHER, band[0],
                                donor + span + FEATHER, band[1]))
            outer.paste(patch, (a - FEATHER, band[0]), wide)
    outer.save(out / "T_KitA_Frame_Outer.png")

    # ── 기둥 하나. 위아래 장식(캡)까지 통째로 뜬다.
    a, b = inner[0]
    pillar = image.crop((a - 4, top[0], b + 4, bottom[1]))
    pillar.save(out / "T_KitA_Frame_Divider.png")

    # 9-slice 여백: 바깥 틀은 모서리 장식 크기, 기둥은 캡 높이.
    corner = max(left[1] - left[0], top[1] - top[0])
    cap = (top[1] - top[0]) + 24        # 캡은 띠 두께보다 조금 길다
    report += [
        "",
        f"바깥 틀   {width}x{height}   9-slice 여백 {corner}px (네 변 같음)",
        f"          margin=({corner / width:.4f},{corner / height:.4f},"
        f"{corner / width:.4f},{corner / height:.4f})",
        f"기둥      {b - a}x{bottom[1] - top[0]}   위아래 캡 {cap}px, 좌우는 안 고정",
        f"          margin=(0,{cap / (bottom[1] - top[0]):.4f},0,"
        f"{cap / (bottom[1] - top[0]):.4f})",
        "",
        f"안쪽 기둥이 원래 있던 자리: " + ", ".join(f"x {a}~{b}" for a, b in inner),
        f"콘텐츠 세로 범위: y {top[1]} ~ {bottom[0]}",
    ]
    (out / "_frame_parts.txt").write_text("\n".join(report) + "\n", encoding="utf-8")
    print("\n".join(report))


if __name__ == "__main__":
    main()
