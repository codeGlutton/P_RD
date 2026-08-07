"""Measure one delivered frame PNG and print the cell values to paste into frame_registry.

시안이 오면 제일 먼저 할 일은 **그림이 강제하는 칸을 다시 재는 것**이다. 배치
좌표는 이 값에서 나온다. 지난번에 이걸 안 하고 눈대중으로 잡아 236개 섹션이
분할선을 밟았다.

``catalog_frame_regions.py`` 는 언리얼에서 뽑아 둔 목록 전체를 훑는 도구라, 새
그림 한 장을 재기엔 무겁다. 이 스크립트는 같은 계산을 파일 하나에 대고 돌린다.

재는 방법은 같다. 밝기를 세로/가로로 눌러 평균을 낸 뒤, 어두운 골짜기를
분할선으로 본다. 나무 기둥은 양피지보다 어둡기 때문에 이렇게 잡힌다.

쓰는 법:
    python Tools/UI/measure_new_frame.py <프레임.png> [--name base3]
"""

import argparse
import sys

try:
    from PIL import Image
except ImportError:  # pragma: no cover
    sys.exit("PIL 이 없다: python -m pip install pillow")

W, H = 1920.0, 1080.0
PROFILE_W, PROFILE_H = 480, 270
MIN_CELL = 0.07          # 이보다 좁으면 콘텐츠 칸이 아니라 장식 틈이다


def bands(values, min_run=2):
    """어두운 골짜기 구간. 그림에 그려진 분할선이 여기에 걸린다."""
    low, high = min(values), max(values)
    if high - low < 4.0:
        return []
    threshold = low + (high - low) * 0.42
    out, start = [], None
    for index, value in enumerate(values):
        if value < threshold and start is None:
            start = index
        elif value >= threshold and start is not None:
            if index - start >= min_run:
                out.append((start, index))
            start = None
    if start is not None:
        out.append((start, len(values)))
    return out


def spans(divider_bands, total):
    starts = [0] + [band[1] for band in divider_bands]
    ends = [band[0] for band in divider_bands] + [total]
    return [(a / total, b / total) for a, b in zip(starts, ends)
            if (b - a) / total >= MIN_CELL]


def profile(pixels, along, across, lo, hi):
    out = []
    steps = range(int(across * lo), int(across * hi), 2)
    for index in range(along):
        total = 0
        for other in steps:
            r, g, b = pixels[index, other] if along == PROFILE_W else pixels[other, index]
            total += (r * 299 + g * 587 + b * 114) // 1000
        out.append(total / len(steps))
    return out


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("png")
    parser.add_argument("--name", default="base3")
    args = parser.parse_args()

    with Image.open(args.png) as source:
        image = source.convert("RGB").resize((PROFILE_W, PROFILE_H), Image.LANCZOS)
    pixels = image.load()

    columns = spans(bands(profile(pixels, PROFILE_W, PROFILE_H, 0.22, 0.88)), PROFILE_W)
    rows = spans(bands(profile(pixels, PROFILE_H, PROFILE_W, 0.06, 0.94)), PROFILE_H)

    print(f"# {args.png}")
    print(f"# 열 {len(columns)}개 · 단 {len(rows)}개")
    if not columns:
        print("# 세로 분할선을 못 찾았다. 기둥이 배경과 밝기가 비슷하면 이렇게 된다.")
    print()
    print("frame_registry.py 에 넣을 값:")
    print(f'    "{args.name}": dict(')
    print("        texture=..., kind=\"cols\",")
    print("        cols=[" + ", ".join(
        f"({a * W:.1f}, {b * W:.1f})" for a, b in columns) + "],")
    if rows:
        print(f"        rows=({rows[0][0] * H:.1f}, {rows[-1][1] * H:.1f})),")
    else:
        print("        rows=(0.0, 1080.0)),   # 가로 분할선을 못 찾음")
    print()
    print("비율(그림 크기가 달라져도 그대로):")
    for index, (a, b) in enumerate(columns):
        print(f"    열 {index}: {a:.3f} ~ {b:.3f}")
    for index, (a, b) in enumerate(rows):
        print(f"    단 {index}: {a:.3f} ~ {b:.3f}")


if __name__ == "__main__":
    main()
