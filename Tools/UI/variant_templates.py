"""Five layout variants, each placed inside the frame's measured cells.

이전 판은 절대 좌표로 섹션을 얹어 그림의 분할선을 236/238 건 밟았다. 여기서는
frame_registry 가 잰 칸(column/window)을 받아 그 안에서만 배치한다. 다섯 안의
차이는 '어느 칸에 무엇을 넣고 어떻게 쌓느냐' 다.

    v01 안전안   좌 정체성 · 중 수치/규칙 · 우 목록/범위
    v02 재배치   열 순서를 바꿔 범위·목록을 먼저 보게
    v03 야간     좌 큰 아트, 어두운 톤
    v04 미니멀   프레임을 걷고 같은 x 격자에 글자만
    v05 실험     탭 + 카드 + 상세
"""

from frame_registry import column, stack, window


# ------------------------------------------------------------- 공통 조각
def _identity(paint, content, rect, big=False):
    x, y, w, h = rect
    if content.get("art"):
        size = min(w - 40, h * (0.46 if big else 0.32))
        paint.hero(content, (x + (w - size) / 2, y + 16.0), (size, size),
                   content.get("frame", "portrait"))
        cursor = y + 16.0 + size + 14.0
    else:
        cursor = y + 16.0
    paint.text("Name", content.get("name", ""), 46 if big else 40,
               (x, cursor), (w, 64.0), 12)
    paint.text("Meta", content.get("meta", ""), 24, (x, cursor + 66.0), (w, 38.0), 12,
               paint.theme["sub"])
    grade = content.get("grade")
    if grade and cursor + 170.0 <= y + h:
        # 등급은 색만으로는 안 읽힌다. 뱃지를 같이 둔다.
        paint.badge(grade[0], grade[1], (x + w / 2 - 110.0, cursor + 110.0))
        return cursor + 176.0
    return cursor + 112.0


def _chips_in(paint, content, rect, columns=2):
    if not content.get("chips"):
        return
    x, y, w, h = rect
    items = content["chips"][:6]
    columns = min(columns, len(items))
    gap = 14.0
    extent = min((w - gap * (columns - 1)) / columns, (h - gap) / 2 - 6.0)
    extent = max(72.0, min(extent, 150.0))
    total = extent * columns + gap * (columns - 1)
    paint.chips(items, (x + (w - total) / 2, y), extent, gap, columns=columns)


def _grids_in(paint, content, rect, count_limit=2):
    grids = content.get("grids", [])
    if not grids:
        return
    x, y, w, h = rect
    slots = grids[:count_limit]
    each = (w - 24.0 * (len(slots) - 1)) / len(slots)
    for index, (caption, pattern, count) in enumerate(slots):
        gx = x + (each + 24.0) * index
        paint.text("GridHeading", caption, 26, (gx, y), (each, 40.0), 12,
                   paint.theme["accent"], "left")
        extent = min((each - 6.0 * (count - 1)) / count, (h - 96.0 - 6.0 * (count - 1)) / count)
        extent = max(24.0, extent)
        span = extent * count + 6.0 * (count - 1)
        paint.grid((gx + (each - span) / 2, y + 46.0), extent, 6.0, count, pattern)
        # 칸 아래에 설명 줄을 둘 자리가 남을 때만 놓는다. 억지로 넣으면 칸을 넘는다.
        cap_y = y + 46.0 + span + 6.0
        if cap_y + 30.0 <= y + h:
            paint.text("GridCap", "", 20, (gx, cap_y), (each, 30.0), 12)


def _lines_in(paint, content, rect, size=25):
    if not content.get("lines"):
        return
    x, y, w, h = rect
    gap = min(46.0, h / max(1, len(content["lines"][:4])))
    paint.lines(content["lines"][:4], (x, y), w, gap, size)


def _rows_in(paint, content, rect, limit=6):
    if not content.get("rows"):
        return
    x, y, w, h = rect
    items = content["rows"][:limit]
    gap, floor = 10.0, 48.0
    fitted = (h - gap * (len(items) - 1)) / len(items)
    if fitted < floor:
        # 최소 높이로도 다 못 담으면 들어가는 만큼만. 높이를 억지로 지키면 칸을 넘는다.
        count = max(1, int((h + gap) // (floor + gap)))
        items, height = items[:count], floor
    else:
        height = min(82.0, fitted)
    paint.rows(items, (x, y), w, height, gap)


def _list_in(paint, content, rect):
    if not content.get("list"):
        return
    paint.list_panel(content["list"], (rect[0], rect[1]), (rect[2], rect[3]))


def _kv_in(paint, content, rect):
    if not content.get("kv"):
        return
    paint.kv(content["kv"], (rect[0], rect[1]), rect[2], 46.0, 25)


def _tags_in(paint, content, rect, limit=4):
    if not content.get("tags"):
        return
    x, y, w, h = rect
    items = content["tags"][:limit]
    # 칸 폭에 맞춰 줄인다. 아래 한계를 높게 잡으면 옆 칸을 밀고 들어간다.
    extent = max(56.0, min(96.0, (w - 18.0 * (len(items) - 1)) / len(items)))
    paint.tags(items, (x, y), extent, 18.0)


def _bar_in(paint, content, rect):
    if not content.get("bars"):
        return
    label, percent = content["bars"][0]
    paint.hp_bar((rect[0], rect[1]), rect[2], label, percent, min(44.0, rect[3]))


# --------------------------------------------------------- 3열 프레임용
def _three_col(paint, content, frame, order, big_art=False):
    """열마다 무엇을 넣을지 order 로 받는다. 열 자체는 그림이 정한 칸 그대로."""
    for slot, (col_index, plan) in enumerate(order):
        rect = column(frame, col_index)
        parts = stack(rect, [item[1] for item in plan])
        for (kind, _), piece in zip(plan, parts):
            if kind == "identity":
                paint.section(None, piece, pad=False)
                _identity(paint, content, piece, big_art)
            elif kind == "chips":
                paint.section(content.get("chipTitle"), piece)
                _chips_in(paint, content, _inset(piece, 56.0), 3 if piece[2] > 480 else 2)
            elif kind == "grids":
                paint.section(content.get("rightTitle"), piece)
                _grids_in(paint, content, _inset(piece, 56.0))
            elif kind == "rows":
                paint.section(content.get("rightTitle"), piece)
                _rows_in(paint, content, _inset(piece, 56.0))
            elif kind == "list":
                paint.section(content.get("leftTitle"), piece)
                _list_in(paint, content, _inset(piece, 56.0))
            elif kind == "lines":
                paint.section(content.get("bottomTitle"), piece)
                _lines_in(paint, content, _inset(piece, 56.0))
            elif kind == "kv":
                paint.section(content.get("leftTitle"), piece)
                _kv_in(paint, content, _inset(piece, 56.0))
            elif kind == "tags":
                paint.section("부여 효과", piece)
                _tags_in(paint, content, _inset(piece, 56.0))
            elif kind == "bar":
                _bar_in(paint, content, _inset(piece, 8.0))


def _inset(rect, top, side=22.0):
    return (rect[0] + side, rect[1] + top, rect[2] - side * 2, rect[3] - top - 18.0)


def cols_v01(paint, content, frame):
    _three_col(paint, content, frame, [
        (0, [("identity", 1.0)]),
        (1, [("chips", 0.42), ("kv", 0.30), ("tags", 0.28)]),
        (2, [("grids", 0.55), ("lines", 0.45)] if content.get("grids")
            else [("rows", 0.60), ("lines", 0.40)]),
    ])


def cols_v02(paint, content, frame):
    """열 순서를 뒤집어 범위/목록을 왼쪽에서 먼저 본다."""
    _three_col(paint, content, frame, [
        (0, [("chips", 0.46), ("tags", 0.54)]),
        (1, [("identity", 0.58), ("kv", 0.42)]),
        (2, [("grids", 0.52), ("lines", 0.48)] if content.get("grids")
            else [("list", 0.52), ("rows", 0.48)] if content.get("list")
            else [("rows", 0.55), ("lines", 0.45)]),
    ])


def cols_v03(paint, content, frame):
    """야간 - 왼쪽 아트를 크게, 오른쪽에 정보."""
    _three_col(paint, content, frame, [
        (0, [("identity", 0.66), ("bar", 0.09), ("chips", 0.25)]),
        (1, [("rows", 1.0)] if content.get("rows")
            else [("list", 1.0)] if content.get("list") else [("kv", 1.0)]),
        (2, [("grids", 0.58), ("lines", 0.42)] if content.get("grids")
            else [("lines", 0.55), ("tags", 0.45)]),
    ], big_art=True)


def cols_v04(paint, content, frame):
    """미니멀 - 프레임을 걷지만 열 x 는 같게 둬 다른 안과 견주기 쉽게."""
    left = column(frame, 0)
    mid = column(frame, 1)
    right = column(frame, 2)
    paint.text("Name", content.get("name", ""), 76, (left[0], left[1] + 10.0),
               (left[2] + mid[2], 100.0), 12, align="left")
    paint.text("Meta", content.get("meta", ""), 26, (left[0] + 4.0, left[1] + 116.0),
               (left[2] + mid[2], 40.0), 12, paint.theme["sub"], "left")
    if content.get("bars"):
        _bar_in(paint, content, (left[0], left[1] + 172.0, left[2] + 120.0, 44.0))
    _chips_in(paint, content, (left[0], left[1] + 240.0, left[2] + mid[2] + 20.0, 200.0),
              columns=min(5, len(content.get("chips", []) or [1])))
    if content.get("lines"):
        paint.text("LinesHeading", content.get("bottomTitle") or "효과", 26,
                   (left[0], left[1] + 470.0), (400.0, 40.0), 12, paint.theme["accent"], "left")
        _lines_in(paint, content, (left[0], left[1] + 520.0, left[2] + mid[2], 200.0), 28)
    if content.get("kv"):
        paint.text("KvHeading", content.get("leftTitle") or "규칙", 26,
                   (left[0], left[1] + 700.0), (400.0, 40.0), 12, paint.theme["accent"], "left")
        _kv_in(paint, content, (left[0], left[1] + 748.0, left[2] + mid[2], 120.0))
    if content.get("grids"):
        _grids_in(paint, content, (right[0], right[1] + 10.0, right[2], 460.0))
    elif content.get("rows"):
        _rows_in(paint, content, (right[0], right[1] + 10.0, right[2], 420.0))
    elif content.get("list"):
        _list_in(paint, content, (right[0], right[1] + 10.0, right[2], 460.0))
    if content.get("tags"):
        paint.text("TagHeading", "부여 효과", 26, (right[0], right[1] + 520.0),
                   (300.0, 40.0), 12, paint.theme["accent"], "left")
        _tags_in(paint, content, (right[0], right[1] + 572.0, right[2], 140.0), 4)


def cols_v05(paint, content, frame):
    """실험 - 왼쪽에 탭+명단, 가운데 카드, 오른쪽 상세."""
    left = column(frame, 0)
    mid = column(frame, 1)
    right = column(frame, 2)
    tabs = content.get("tabs", ["개요", "상세", "기록"])[:3]
    tab_height = 62.0
    for index, label in enumerate(tabs):
        y = left[1] + (tab_height + 8.0) * index
        paint.well("Tab", (left[0], y), (left[2], tab_height), 6,
                   paint.theme["accent"] if index == 0 else None)
        paint.text("TabText", label, 26, (left[0], y + 12.0), (left[2], 40.0), 12)
    rest = (left[0], left[1] + (tab_height + 8.0) * len(tabs) + 12.0,
            left[2], left[3] - (tab_height + 8.0) * len(tabs) - 12.0)
    paint.section(None, rest)
    if content.get("list"):
        _list_in(paint, content, _inset(rest, 24.0))
    else:
        _kv_in(paint, content, _inset(rest, 30.0))
    paint.section(None, mid)
    _identity(paint, content, _inset(mid, 20.0), big=True)
    _chips_in(paint, content, (mid[0] + 22.0, mid[1] + mid[3] - 200.0, mid[2] - 44.0, 180.0), 2)
    upper, lower = stack(right, [0.55, 0.45])
    paint.section(content.get("rightTitle"), upper)
    if content.get("grids"):
        _grids_in(paint, content, _inset(upper, 56.0))
    else:
        _rows_in(paint, content, _inset(upper, 56.0), 3)
    paint.section(content.get("bottomTitle"), lower)
    _lines_in(paint, content, _inset(lower, 56.0), 24)


# ------------------------------------------------------- 단일 창 프레임용
def _window_sections(paint, content, frame, plan, size=None):
    rect = window(frame, size=size)
    parts = stack(rect, [item[1] for item in plan])
    for (kind, _), piece in zip(plan, parts):
        if kind == "identityRow":
            paint.section(None, piece)
            inner = _inset(piece, 18.0)
            if content.get("art"):
                side = min(inner[3], 220.0)
                paint.hero(content, (inner[0], inner[1]), (side, side),
                           content.get("frame", "portrait"))
                text_x = inner[0] + side + 26.0
            else:
                text_x = inner[0]
            paint.text("Name", content.get("name", ""), 46, (text_x, inner[1] + 8.0),
                       (inner[2] * 0.5, 64.0), 12, align="left")
            paint.text("Meta", content.get("meta", ""), 24, (text_x, inner[1] + 76.0),
                       (inner[2] * 0.5, 38.0), 12, paint.theme["sub"], "left")
            if content.get("bars"):
                _bar_in(paint, content, (text_x, inner[1] + 122.0, inner[2] * 0.45, 44.0))
            if content.get("chips"):
                items = content["chips"][:4]
                _chips_in(paint, content,
                          (inner[0] + inner[2] * 0.56, inner[1], inner[2] * 0.44, inner[3]),
                          columns=min(4, len(items)))
        elif kind == "grids":
            paint.section(content.get("rightTitle"), piece)
            _grids_in(paint, content, _inset(piece, 56.0))
        elif kind == "rows":
            paint.section(content.get("rightTitle"), piece)
            _rows_in(paint, content, _inset(piece, 56.0))
        elif kind == "lines":
            paint.section(content.get("bottomTitle"), piece)
            _lines_in(paint, content, _inset(piece, 56.0))
        elif kind == "kvtags":
            paint.section(content.get("leftTitle"), piece)
            inner = _inset(piece, 56.0)
            _kv_in(paint, content, (inner[0], inner[1], inner[2] * 0.5, inner[3]))
            _tags_in(paint, content,
                     (inner[0] + inner[2] * 0.55, inner[1], inner[2] * 0.45, inner[3]))
        elif kind == "chips":
            paint.section(content.get("chipTitle"), piece)
            _chips_in(paint, content, _inset(piece, 56.0), 4)


def win_v01(paint, content, frame):
    _window_sections(paint, content, frame,
                     [("identityRow", 0.40), ("grids" if content.get("grids") else "rows", 0.34),
                      ("lines", 0.26)])


def win_v02(paint, content, frame):
    _window_sections(paint, content, frame,
                     [("identityRow", 0.34), ("kvtags", 0.30),
                      ("grids" if content.get("grids") else "rows", 0.36)])


def win_v03(paint, content, frame):
    _window_sections(paint, content, frame,
                     [("identityRow", 0.46), ("lines", 0.28),
                      ("grids" if content.get("grids") else "rows", 0.26)])


def win_v04(paint, content, frame):
    _window_sections(paint, content, frame,
                     [("identityRow", 0.36), ("chips", 0.24), ("lines", 0.40)])


def win_v05(paint, content, frame):
    _window_sections(paint, content, frame,
                     [("identityRow", 0.42), ("kvtags", 0.26),
                      ("grids" if content.get("grids") else "rows", 0.32)])


COLS = [cols_v01, cols_v02, cols_v03, cols_v04, cols_v05]
WINDOWS = [win_v01, win_v02, win_v03, win_v04, win_v05]
