"""자리 계산. 숫자를 짐작하지 않기 위한 것들만 모아 둔다.

여기 있는 것은 전부 **순수한 계산**이다. 언리얼을 안 부르므로 그냥 파이썬으로도
돌려 보고 값을 확인할 수 있고, 같은 규칙을 C++ 빌더도 쓴다
(``Source/P_RDEditor/UI/UIPartRects.h`` -- export_inner_rects.py 가 만든다).

왜 모았나
---------
같은 실수를 화면마다 따로 하고 있었다.

* 글자를 칸 안에 넣을 때 **위쪽에 붙어** 그려졌다. 캔버스 슬롯에 크기를 주면
  글자는 그 칸의 왼쪽 위에서 시작한다. 가운데로 놓으려면 슬롯을 자동 크기로
  두고 **가운데를 기준점**으로 잡아야 한다.
* 늘리면 안 되는 그림(단추 · 체크 · 슬라이더 손잡이)을 칸 크기에 그대로
  맞춰 놓아 비율이 망가졌다. 슬라이더 홈은 596x88 짜리를 235x16 에 욱여넣어
  실 한 가닥처럼 보였다.
"""


def fit_aspect(rect, source, anchor=(0.5, 0.5)):
    """원본 비율을 지키면서 rect 안에 들어가는 가장 큰 사각.

    @param rect   놓고 싶은 자리 (x, y, w, h)
    @param source 그림의 원본 크기 (w, h)
    @param anchor 남는 자리에서 어디에 붙일지. (0.5,0.5) 면 가운데.
    @return (x, y, w, h)

    늘리지 말라고 적힌 그림은 이걸 거쳐야 한다. 596x88 짜리 홈을 235x16 에
    그냥 놓으면 세로로 5배 눌린다 -- 화면에서는 실 한 가닥이 된다.
    """
    x, y, width, height = rect
    source_w, source_h = source
    if source_w <= 0.0 or source_h <= 0.0 or width <= 0.0 or height <= 0.0:
        return rect
    scale = min(width / source_w, height / source_h)
    fit_w, fit_h = source_w * scale, source_h * scale
    return (x + (width - fit_w) * anchor[0], y + (height - fit_h) * anchor[1],
            fit_w, fit_h)


def cover_aspect(rect, source):
    """비율을 지키면서 rect 를 **덮는** 가장 작은 사각. 넘치는 쪽은 잘린다.

    초상화처럼 "칸을 꽉 채워야 하고 잘려도 되는" 것에 쓴다. 잘릴 수 있으므로
    부르는 쪽이 클리핑을 켜야 한다.
    """
    x, y, width, height = rect
    source_w, source_h = source
    if source_w <= 0.0 or source_h <= 0.0:
        return rect
    scale = max(width / source_w, height / source_h)
    fit_w, fit_h = source_w * scale, source_h * scale
    return (x + (width - fit_w) * 0.5, y + (height - fit_h) * 0.5, fit_w, fit_h)


def split_columns(rect, count, gap=0.0):
    """가로로 count 등분. 칸막이가 그려진 띠에 글자를 나눠 넣을 때."""
    x, y, width, height = rect
    span = (width - gap * (count - 1)) / count
    return [(x + (span + gap) * index, y, span, height) for index in range(count)]


def split_rows(rect, count, gap=0.0):
    """세로로 count 등분."""
    x, y, width, height = rect
    span = (height - gap * (count - 1)) / count
    return [(x, y + (span + gap) * index, width, span) for index in range(count)]


def inset(rect, amount):
    """사방으로 amount 만큼 안으로."""
    x, y, width, height = rect
    return (x + amount, y + amount, width - amount * 2.0, height - amount * 2.0)


def center_of(rect):
    x, y, width, height = rect
    return (x + width * 0.5, y + height * 0.5)
