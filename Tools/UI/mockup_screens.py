"""Screen content and five variants each, for the HTML mockups.

시안이 서로 **정말로 다르게** 하려고 바꾸는 축을 정해 두었다. 색만 다른 다섯
개는 고를 이유가 없다.

    열 수와 비율     정보를 몇 덩어리로 나눌 것인가
    제목 자리        틀 위에 걸칠 것인가, 안으로 넣을 것인가
    수치 표현        둥근 칩인가, 한 줄 표인가
    빈 칸 처리       내용이 없을 때 그 열을 없앨 것인가 남길 것인가

각 시안의 `why` 에 무엇을 시험하는지 적어 둔다. 나중에 왜 이걸 골랐는지
되짚을 수 있어야 한다.
"""

CHIPS = [("AP", "1"), ("피해", "6~10"), ("쿨타임", "-"), ("사거리", "1"), ("치명", "15")]
SKILLS = ["이동", "베기", "강타", "연속 찌르기"]


def _chip_grid(part, x, y, width, chips, per_row=2, size=150, gap=20):
    """칩을 격자로. 마지막 홀수 칩은 가운데로 옮긴다."""
    out = []
    rows = (len(chips) + per_row - 1) // per_row
    span = size * per_row + gap * (per_row - 1)
    left = x + (width - span) / 2
    for index, (label, value) in enumerate(chips):
        row, col = divmod(index, per_row)
        alone = row == rows - 1 and len(chips) % per_row == 1
        cx = x + (width - size) / 2 if alone else left + (size + gap) * col
        cy = y + (size + gap) * row
        out.append(part("chip", cx, cy, size, size, f"{label}<br>{value}"))
    return "".join(out)


def _stat_rows(part, x, y, width, chips, height=64, gap=12):
    """칩 대신 한 줄 표. 좁은 열에서 더 많이 들어간다."""
    return "".join(
        part("row", x, y + (height + gap) * index, width, height,
             f"{label}<span style='margin-left:24px'>{value}</span>", size=30)
        for index, (label, value) in enumerate(chips))


def _grid(text, x, y, cell, extent=5, painted=()):
    """사거리 판. 칠한 칸만 색을 준다."""
    out = []
    for row in range(extent):
        for col in range(extent):
            klass = "cellbox"
            if (row, col) in painted:
                klass = "hit" if (row, col) == (2, 2) else "aim"
            out.append(f'<div class="{klass}" style="left:{x + col * (cell + 5)}px;'
                       f'top:{y + row * (cell + 5)}px;width:{cell}px;height:{cell}px"></div>')
    return "".join(out)


CROSS = {(2, 2), (1, 2), (3, 2), (2, 1), (2, 3)}


def draw_skill(columns, variant, part, text):
    out = []
    left = columns[0]
    top = left[1] + variant["contentTop"]
    icon = min(left[2] - 60, 300)
    out.append(part("portrait", left[0] + (left[2] - icon) / 2, top, icon, icon))
    out.append(text(left[0] + 24, top + icon + 20, left[2] - 48, 44,
                    "AP 1 · 피해 6~10", size=28))
    out.append(text(left[0] + 24, top + icon + 78, left[2] - 48, 40,
                    "설명", cls="heading", size=28, colour="#7a4a12"))
    out.append(text(left[0] + 24, top + icon + 124, left[2] - 48, 220,
                    "대상에게 6~10의 피해를 준다. 사거리 1, 십자 모양으로 고른다.",
                    size=24))

    mid = columns[1] if len(columns) > 2 else columns[0]
    if len(columns) > 2:
        out.append(text(mid[0] + 24, mid[1] + variant["contentTop"], mid[2] - 48, 44,
                        "수치", cls="heading", size=30, colour="#7a4a12"))
        body = variant["stats"]
        if body == "chip":
            out.append(_chip_grid(part, mid[0], mid[1] + variant["contentTop"] + 60,
                                  mid[2], CHIPS, size=min(150, (mid[2] - 60) / 2)))
        else:
            out.append(_stat_rows(part, mid[0] + 24, mid[1] + variant["contentTop"] + 60,
                                  mid[2] - 48, CHIPS))

    right = columns[-1]
    ry = right[1] + variant["contentTop"]
    half = (right[2] - 70) / 2
    cell = min((half - 20) / 5, 62)
    out.append(text(right[0] + 24, ry, half, 40, "사거리", cls="heading",
                    size=28, colour="#7a4a12"))
    out.append(text(right[0] + 46 + half, ry, half, 40, "영향 범위",
                    cls="heading", size=28, colour="#7a4a12"))
    out.append(_grid(text, right[0] + 24, ry + 52, cell, painted=CROSS))
    out.append(_grid(text, right[0] + 46 + half, ry + 52, cell, painted={(2, 2)}))
    blocker_y = ry + 52 + cell * 5 + 40
    out.append(text(right[0] + 24, blocker_y, right[2] - 48, 40, "차단 규칙",
                    cls="heading", size=28, colour="#7a4a12"))
    for index, label in enumerate(("조준 차단", "영향 차단")):
        out.append(part("row", right[0] + 24, blocker_y + 50 + index * 62,
                        right[2] - 48, 54,
                        f"{label}<span style='margin-left:40px'>장애물·유닛</span>",
                        size=26))
    return "".join(out)


def draw_artifact(columns, variant, part, text):
    out = []
    left = columns[0]
    top = left[1] + variant["contentTop"]
    icon = min(left[2] - 60, 300)
    out.append(part("portrait", left[0] + (left[2] - icon) / 2, top, icon, icon))
    out.append(text(left[0] + 24, top + icon + 24, left[2] - 48, 44,
                    "희귀 · 120골드", size=28))
    right = columns[-1]
    ry = right[1] + variant["contentTop"]
    out.append(text(right[0] + 24, ry, right[2] - 48, 44, "효과",
                    cls="heading", size=30, colour="#7a4a12"))
    for index, line in enumerate((
            "전투를 시작할 때 행동력 1을 더 얻는다.",
            "처치할 때마다 다음 턴 피해가 2 오른다.")):
        out.append(part("row", right[0] + 24, ry + 60 + index * 76,
                        right[2] - 48, 64, line, size=26))
    return "".join(out)


def draw_monster(columns, variant, part, text):
    out = []
    left = columns[0]
    top = left[1] + variant["contentTop"]
    out.append(text(left[0] + 24, top, left[2] - 48, 40, "출현 몬스터",
                    cls="heading", size=28, colour="#7a4a12"))
    row_h = 140
    for index, name in enumerate(("Slime", "Mushroom", "Spider")):
        out.append(part("row", left[0] + 24, top + 52 + index * (row_h + 14),
                        left[2] - 48, row_h, name, size=32))
    mid = columns[1] if len(columns) > 2 else columns[0]
    if len(columns) > 2:
        icon = min(mid[2] - 80, 340)
        out.append(part("portrait", mid[0] + (mid[2] - icon) / 2,
                        mid[1] + variant["contentTop"], icon, icon))
        out.append(text(mid[0] + 24, mid[1] + variant["contentTop"] + icon + 24,
                        mid[2] - 48, 56, "Slime", size=40))
    right = columns[-1]
    ry = right[1] + variant["contentTop"]
    out.append(part("hp", right[0] + 24, ry, right[2] - 48, 56, "50 / 50", size=30))
    out.append(text(right[0] + 24, ry + 76, right[2] - 48, 44,
                    "AP 0     속도 5", size=30))
    out.append(text(right[0] + 24, ry + 136, right[2] - 48, 44, "스킬",
                    cls="heading", size=28, colour="#7a4a12"))
    slot = 128 if variant["skills"] == "row" else 150
    for index, name in enumerate(SKILLS):
        if variant["skills"] == "row":
            out.append(part("row", right[0] + 24, ry + 188 + index * (slot + 14),
                            right[2] - 48, slot, name, size=32))
        else:
            col = index % 3
            row = index // 3
            out.append(part("cell", right[0] + 24 + col * (slot + 16),
                            ry + 188 + row * (slot + 16), slot, slot, name, size=24))
    return "".join(out)


def draw_settings(columns, variant, part, text):
    out = []
    rows = [("전체", "소리"), ("배경음", "소리"), ("효과음", "소리"), ("조작음", "소리"),
            ("화면 흔들림", "화면"), ("진동", "화면"), ("연출", "화면"),
            ("품질", "화면"), ("프레임", "화면"), ("언어", "조작")]
    per = (len(rows) + len(columns) - 1) // len(columns)
    # 받침 그림이 526x140 이고 세로 마진이 64+64 다. 이보다 낮게 그리면
    # 위아래 테두리가 겹치거나, 겹침을 막으려 줄이면 장식이 작아져 다른 줄과
    # 굵기가 달라 보인다. 그림 크기 그대로 놓는다.
    height, gap = 140, 14
    for index, (label, _group) in enumerate(rows):
        column = columns[min(index // per, len(columns) - 1)]
        y = column[1] + variant["contentTop"] + (index % per) * (height + gap)
        out.append(part("row", column[0] + 24, y, column[2] - 48, height,
                        f"{label}<span style='margin-left:40px'>———</span>", size=28))
    bar_y = columns[0][1] + columns[0][3] - 96
    labels = ("뒤로", "되돌리기", "저장하고 나가기", "탐험 포기")
    span = (1854 - 65 - 36) / 4
    for index, label in enumerate(labels):
        kind = "small" if index < 2 else "button"
        out.append(part(kind, 65 + 12 + index * span, bar_y, span - 12, 84,
                        label, size=30))
    return "".join(out)


def draw_mercenary(columns, variant, part, text):
    out = []
    left = columns[0]
    top = left[1] + variant["contentTop"]
    for index, name in enumerate(("Knight", "빈 자리", "빈 자리")):
        out.append(part("row", left[0] + 24, top + index * 214, left[2] - 48, 196,
                        name, size=32))
    mid = columns[1] if len(columns) > 2 else columns[0]
    if len(columns) > 2:
        icon = min(mid[2] - 60, 420)
        out.append(part("portrait", mid[0] + (mid[2] - icon) / 2,
                        mid[1] + variant["contentTop"], icon, icon))
    right = columns[-1]
    ry = right[1] + variant["contentTop"]
    out.append(text(right[0] + 24, ry, right[2] - 48, 60, "Knight", size=46))
    out.append(part("hp", right[0] + 24, ry + 76, right[2] - 48, 56,
                    "100 / 100", size=30))
    out.append(text(right[0] + 24, ry + 152, right[2] - 48, 44,
                    "AP 10 / 10     속도 5", size=30))
    out.append(text(right[0] + 24, ry + 212, right[2] - 48, 44, "스킬",
                    cls="heading", size=28, colour="#7a4a12"))
    size = 150
    for index in range(6):
        col, row = index % 3, index // 3
        out.append(part("cell", right[0] + 24 + col * (size + 16),
                        ry + 264 + row * (size + 16), size, size,
                        SKILLS[index % len(SKILLS)], size=22))
    return "".join(out)


SCREENS = {
    "skill": dict(title="스킬 상세", draw=draw_skill,
                  note="아이콘·수치·표적 셋을 어떻게 나눌지. 수치를 칩으로 둘지 표로 둘지가 축이다."),
    "artifact": dict(title="아티팩트 상세", draw=draw_artifact,
                     note="AP도 사거리도 없다. 가운데 수치 열을 없앨지가 축이다."),
    "monster": dict(title="몬스터 상세", draw=draw_monster,
                    note="명단·모습·수치와 스킬. 스킬을 줄로 둘지 칸으로 둘지가 축이다."),
    "mercenary": dict(title="용병", draw=draw_mercenary,
                      note="파티 카드·초상화·수치. 빈 자리를 어떻게 보일지가 축이다."),
    "settings": dict(title="설정", draw=draw_settings,
                     note="줄이 열 개다. 몇 열로 나눌지, 단추 줄을 어디 둘지가 축이다."),
}

_BASE = dict(contentTop=130, stats="chip", skills="row", title="top")


def _v(name, why, **kwargs):
    out = dict(_BASE)
    out.update(kwargs)
    out["name"], out["why"] = name, why
    return out


VARIANTS = {
    "skill": [
        _v("v01 균형", "지금 배선과 같은 3열. 기준으로 삼는다.",
           columns=(0.24, 0.29, 0.47)),
        _v("v02 표적 우선", "표적 판을 가장 넓게. 사거리를 자주 본다는 가정.",
           columns=(0.20, 0.24, 0.56)),
        _v("v03 수치 표", "칩 대신 한 줄 표. 좁은 열에도 다 들어간다.",
           columns=(0.26, 0.26, 0.48), stats="row"),
        _v("v04 두 열", "수치를 왼쪽에 합쳐 열을 줄인다. 기둥이 하나뿐이라 시원하다.",
           columns=(0.42, 0.58), stats="row"),
        _v("v05 제목 안쪽", "제목을 틀 안으로. 위쪽 장식과 안 겹친다.",
           columns=(0.24, 0.29, 0.47), title="inside", contentTop=200),
    ],
    "artifact": [
        _v("v01 두 열", "수치 열을 없앤다. 빈 칩 다섯이 뜨던 자리다.",
           columns=(0.34, 0.66)),
        _v("v02 세 열 유지", "다른 상세와 열 수를 맞춘다. 가운데는 비운다.",
           columns=(0.24, 0.29, 0.47)),
        _v("v03 그림 크게", "왼쪽을 넓혀 그림을 키운다.",
           columns=(0.46, 0.54)),
        _v("v04 효과 넓게", "효과 줄을 가장 넓게. 글이 길어질 것을 본다.",
           columns=(0.26, 0.74)),
        _v("v05 제목 안쪽", "제목을 틀 안으로 넣은 두 열.",
           columns=(0.34, 0.66), title="inside", contentTop=200),
    ],
    "monster": [
        _v("v01 균형", "명단·모습·수치 3열. 스킬은 줄.",
           columns=(0.24, 0.29, 0.47)),
        _v("v02 스킬 칸", "스킬을 정사각 칸으로. 용병탭과 같은 규칙.",
           columns=(0.24, 0.29, 0.47), skills="cell"),
        _v("v03 모습 크게", "가운데를 넓혀 몬스터를 크게 본다.",
           columns=(0.20, 0.40, 0.40)),
        _v("v04 두 열", "모습과 수치를 합친다. 명단은 좁게.",
           columns=(0.28, 0.72), skills="cell"),
        _v("v05 명단 넓게", "명단 줄에 HP까지 넣을 수 있게 왼쪽을 넓힌다.",
           columns=(0.34, 0.26, 0.40)),
    ],
    "mercenary": [
        _v("v01 균형", "카드·초상화·수치 3열.", columns=(0.24, 0.29, 0.47)),
        _v("v02 초상화 크게", "가운데를 넓힌다. 일러스트가 1:1 이라 잘 맞는다.",
           columns=(0.22, 0.38, 0.40)),
        _v("v03 수치 넓게", "스킬 여섯 칸을 넉넉히 놓는다.",
           columns=(0.20, 0.26, 0.54)),
        _v("v04 두 열", "초상화를 수치 열 위로 올려 열을 줄인다.",
           columns=(0.30, 0.70)),
        _v("v05 제목 안쪽", "제목을 틀 안으로.",
           columns=(0.24, 0.29, 0.47), title="inside", contentTop=200),
    ],
    "settings": [
        _v("v01 두 열", "열당 다섯 줄. 지금 배선과 같다.", columns=(0.5, 0.5)),
        _v("v02 세 열", "열당 네 줄. 줄이 짧아져 이름과 값이 붙는다.",
           columns=(0.34, 0.33, 0.33)),
        # 한 열은 버렸다. 줄이 열 개인데 받침 그림 높이가 140 이라 1540px 이
        # 필요하고, 창은 939px 뿐이다. 스크롤을 넣지 않는 한 안 들어간다.
        _v("v03 네 열", "소리·화면·조작을 열로 가른다. 열마다 세 줄 이하.",
           columns=(0.25, 0.25, 0.25, 0.25)),
        _v("v04 왼쪽 넓게", "이름이 긴 줄을 왼쪽에 몰아넣는다.",
           columns=(0.6, 0.4)),
        _v("v05 제목 안쪽", "제목을 틀 안으로 넣은 두 열.",
           columns=(0.5, 0.5), title="inside", contentTop=200),
    ],
}
