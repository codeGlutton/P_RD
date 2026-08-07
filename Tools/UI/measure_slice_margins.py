"""Find the smallest 9-slice margin that does not smear the ornament.

왜 다시 재나
------------
지금 마진은 "가장자리에서 안으로 들어가며 색이 잦아드는 지점" 으로 쟀다. 그건
**테두리가 끝나는 자리**지 **장식이 끝나는 자리**가 아니다. 바깥 틀에서 실제로
어긋났다 -- 구멍은 x65 에서 시작하는데 모서리 황동은 x90 까지 뻗어 있어서,
65 로 자르니 장식 25px 이 늘어나는 구간에 들어가 이중선이 됐다.

무엇을 시험하나
---------------
9-slice 는 이렇게 그린다.

    모서리 4칸   그대로 (안 늘어남)
    위/아래 띠   가로로만 늘어남
    좌/우 띠     세로로만 늘어남
    가운데       양쪽으로 늘어남

그러면 **늘어나는 방향으로 그림이 한결같아야** 안 뭉개진다. 위 띠는 가로로
늘어나니 가로로 한결같아야 하고, 좌 띠는 세로로 늘어나니 세로로 한결같아야 한다.

그래서 마진 후보를 키워 가며 "그 바깥이 늘어나는 방향으로 한결같은가"를 직접
본다. 색이 어디서 잦아드는지 짐작하지 않는다.

한결같음은 **알파까지 넣어** 잰다. 틀 그림은 속이 비어 있어서 색만 보면
못 가린다.

쓰는 법:
    python Tools/UI/measure_slice_margins.py Saved/UIKit/ConceptA Saved/UIKit/FrameA
"""

import argparse
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:  # pragma: no cover
    sys.exit("PIL 이 없다: python -m pip install pillow")

TOL = 16.0          # 채널 평균 차이가 이보다 크면 "한결같지 않다"
RUN = 3             # 기준을 넘는 칸이 이만큼 이어지면 장식이다
SAFETY = 1.15       # 줄여서 재느라 작게 나오는 만큼 넉넉히 잡는다
MAX_FRAC = 0.42     # 마진이 이보다 크면 늘릴 가운데가 없다
WORK = 320          # 이 크기로 줄여서 잰다. 큰 그림도 금방 끝난다
OUT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/UIKit/_slice_margins.txt")


def column_means(pixels, x0, x1, y0, y1):
    """세로로 눌러 만든 가로 방향 신호. 가로로 늘어나는 구간을 볼 때 쓴다."""
    out = []
    rows = range(y0, y1)
    if not rows:
        return out
    for x in range(x0, x1):
        total = [0, 0, 0, 0]
        for y in rows:
            pixel = pixels[x, y]
            for channel in range(4):
                total[channel] += pixel[channel]
        out.append([value / len(rows) for value in total])
    return out


def row_means(pixels, x0, x1, y0, y1):
    out = []
    cols = range(x0, x1)
    if not cols:
        return out
    for y in range(y0, y1):
        total = [0, 0, 0, 0]
        for x in cols:
            pixel = pixels[x, y]
            for channel in range(4):
                total[channel] += pixel[channel]
        out.append([value / len(cols) for value in total])
    return out


def uniform(series):
    """한 줄로 눌러 놓은 값들이 서로 비슷한가.

    가장 큰 차이로 재면 손으로 그린 나뭇결까지 걸린다. 위쪽 몇 퍼센트를 버리는
    방법도 안 된다 -- 넓은 부품에서는 장식이 전체의 2% 밖에 안 돼서 그냥
    통과한다. 실제로 바깥 틀의 모서리 장식(34px / 1808px = 1.9%)이 그렇게
    빠져나갔다.

    그래서 **이어진 구간**으로 본다. 나뭇결은 짧게 들쭉날쭉하지만 장식은
    한 덩어리로 이어져 다르다. 기준을 넘는 칸이 연달아 RUN 개 이상이면
    장식이 있는 것으로 본다.
    """
    if len(series) < 2:
        return True
    middle = series[len(series) // 2]
    run = 0
    for entry in series:
        gap = sum(abs(entry[c] - middle[c]) for c in range(4)) / 4.0
        if gap > TOL:
            run += 1
            if run >= RUN:
                return False
        else:
            run = 0
    return True


def measure(path):
    image = Image.open(path).convert("RGBA")
    full_w, full_h = image.size
    scale = min(1.0, WORK / max(full_w, full_h))
    work = image if scale >= 1.0 else image.resize(
        (max(8, int(full_w * scale)), max(8, int(full_h * scale))), Image.LANCZOS)
    width, height = work.size
    pixels = work.load()

    limit_x, limit_y = int(width * MAX_FRAC), int(height * MAX_FRAC)
    candidates_x = list(range(2, max(3, limit_x)))
    candidates_y = list(range(2, max(3, limit_y)))

    def passes(mx, my):
        # 위·아래 띠는 가로로 늘어난다 -> 가로로 한결같아야 한다.
        for y0, y1 in ((0, my), (height - my, height)):
            if not uniform(column_means(pixels, mx, width - mx, y0, y1)):
                return False
        # 좌·우 띠는 세로로 늘어난다 -> 세로로 한결같아야 한다.
        for x0, x1 in ((0, mx), (width - mx, width)):
            if not uniform(row_means(pixels, x0, x1, my, height - my)):
                return False
        # 가운데는 양쪽으로 늘어난다 -> 두 방향 모두 한결같아야 한다.
        if not uniform(column_means(pixels, mx, width - mx, my, height - my)):
            return False
        if not uniform(row_means(pixels, mx, width - mx, my, height - my)):
            return False
        return True

    # 합이 작은 것부터 본다. 가로만 훑으면 "가로 68 · 세로 272" 처럼 한쪽으로
    # 치우친 답이 먼저 걸린다 -- 실제로 바깥 틀이 그렇게 나왔다.
    best = None
    for total in range(4, limit_x + limit_y + 1):
        for mx in candidates_x:
            my = total - mx
            if my < candidates_y[0] or my > candidates_y[-1]:
                continue
            if passes(mx, my):
                best = (mx, my)
                break
        if best is not None:
            break

    if best is None:
        return None, (full_w, full_h)
    mx, my = best
    # 줄여서 잰 값을 원래 크기로 되돌린다.
    #
    # 줄여서 재면 두 가지로 작게 나온다. 한 칸이 원본 여러 픽셀이라 반올림에서
    # 깎이고, 띠 전체를 세로로 눌러 평균 내는 과정에서 장식 신호가 옅어진다.
    # 실제로 바깥 틀은 손으로 잰 장식 끝(90)보다 작은 80 이 나왔다.
    #
    # 모자라면 장식이 늘어나고(눈에 띈다), 남으면 가운데가 조금 좁아질 뿐이다
    # (안 보인다). 그래서 넉넉한 쪽으로 기울인다.
    return (int((mx / scale) * SAFETY) + 2, int((my / scale) * SAFETY) + 2), (full_w, full_h)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("folders", nargs="+")
    args = parser.parse_args()

    lines = ["# 9-slice 마진 -- 늘려도 장식이 안 뭉개지는 가장 작은 값",
             "# 색이 잦아드는 자리가 아니라, 늘어나는 방향으로 그림이 한결같아지는 자리다.",
             "# 부품                          크기        마진(가로,세로)   비율",
             ""]
    for folder in args.folders:
        for path in sorted(Path(folder).glob("*.png")):
            if path.name.startswith("_"):
                continue
            margin, (width, height) = measure(path)
            if margin is None:
                lines.append(f"{path.stem:30s} {width:5d}x{height:5d}   못 찾음"
                             f" -- 무늬가 강해 어디를 잘라도 한결같지 않다")
                continue
            mx, my = margin
            lines.append(f"{path.stem:30s} {width:5d}x{height:5d}   ({mx:4d},{my:4d})"
                         f"      ({mx / width:.4f},{my / height:.4f})")
        lines.append("")

    OUT.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print("\n".join(lines))


if __name__ == "__main__":
    main()
