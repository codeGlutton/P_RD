# -*- coding: utf-8 -*-
"""명세를 읽어 배치안 하나를 짓는다. 스무 안이 이 한 갈래로 나온다.

## 왜 하나로

배치안 열 개를 손으로 적었더니, 좌표를 옮기는 과정에서 매번 틀렸다 --
4안 타원을 가로로 만들고 7안 카드를 부채처럼 펼친 것은 시안을 보지 않고
이름에서 짐작해 벌어진 일이다. 스무 개를 손으로 적으면 스무 번 틀린다.

조각을 통째로 놓는 방식으로 바꾸면서 손으로 적을 것이 사라졌다. 자리는
매칭이 찾고 역할은 표에 있고 내용 자리는 조각에 뚫린 구멍이 알려 준다.
남은 것은 그 셋을 읽어 위젯을 만드는 일뿐이고, 그건 시안마다 다르지 않다.

## 내용은 어디에 놓나

조각에 뚫린 구멍을 읽는다. 얼굴이 들어갈 자리는 둥글게 뚫려 있고 카드 칸은
네모나게 뚫려 있다. 구멍이 없는 판(글자만 있는 라운드 표시)은 판 전체를
글자 자리로 쓴다.

구멍이 알려 주지 않는 것 -- 이름과 막대가 초상 오른쪽 어디에 오는지 -- 은
비율로 정한다. 시안마다 판 크기가 다르므로 절대값을 적으면 한 시안에만
맞는다.

## 이름 규칙

C++ 이 이름으로 위젯을 찾는다(CombatLayoutHUDWidget::CacheWidgets). 그래서
PartyCard_0 / CommandIcon_3 / EnemyHPBar 같은 이름을 그대로 쓴다. 조각이
없어 비는 구역은 위젯도 없고, C++ 은 없는 위젯을 건너뛰도록 되어 있다.
"""
import io
import json
import os

import combat_layout_kit as kit
from slot_table import slots

CHROME = r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitUIKit/Chrome"
CUTOUT_PACKAGE = "/Game/SVN/OutSideAsset/UI/KayKit/Cutouts"

#: 시안 좌표 -> 설계 캔버스(1920x1080) 좌표
K = kit.CHROME_SCALE

_MANIFEST = None


def manifest():
    global _MANIFEST
    if _MANIFEST is None:
        with io.open(os.path.join(CHROME, "cutout_manifest.json"),
                     encoding="utf-8") as handle:
            _MANIFEST = json.load(handle)
    return _MANIFEST


def _round_holes(holes, w, h):
    """얼굴이 들어갈 만한 구멍. 대체로 네모지고 판에 비해 크다."""
    out = []
    for hx, hy, hw, hh in holes:
        if hw < 28 or hh < 28:
            continue
        if not 0.65 < hw / float(hh) < 1.55:
            continue
        if hw > w * 0.7 or hh > h * 0.9:
            continue
        out.append((hx, hy, hw, hh))
    out.sort(key=lambda e: (e[0], e[1]))
    return out


def _plate(bp, root, row, name):
    """조각 한 장을 제자리에 놓는다. 늘리지 않는다 -- 위치만 맞춘다."""
    x, y, w, h = [v * K for v in row["rect"]]
    kit.add(bp, "CanvasPanel", name, root)
    kit.place(bp, name, x, y, w, h, row["anchor"], None, kit.Z_FILL)

    # 그림은 원래 크기로 놓는다. 글자 지운 판은 자리와 몇 px 다른데(13안 턴
    # 바는 화살표가 없어 14px 짧다), 칸에 맞춰 늘리면 몰딩이 뭉갠다 --
    # 조각을 통째로 쓰는 이유가 바로 그것이다.
    #
    # 한 자리에 여러 장인 경우가 있다. 붙어 있던 조각을 빈 판 여러 장으로
    # 덮은 것이라, 각자 제 자리에 놓는다.
    # 크기가 다를 때는 왼쪽 위에 붙인다. 가운데에 두었더니 캡처에서 잰
    # 자리가 시안보다 7~8px 아래로 나왔다 -- 빈 판은 글자 있는 쪽에 달린
    # 꼬리(13안 턴 바의 아래 화살표)가 없어 그만큼 짧고, 판 몸통은 위에서
    # 시작하기 때문이다.
    arts = row.get("arts") or [[row["asset"], 0, 0] + list(row["rect"][2:])]
    for slot, (asset, ax, ay, aw, ah) in enumerate(arts):
        ax, ay, aw, ah = [v * K for v in (ax, ay, aw, ah)]
        kit.image(bp, "%s_Art%d" % (name, slot), name, ax, ay, aw, ah,
                  (w, h), z_order=kit.Z_FILL,
                  texture="{}/{}".format(CUTOUT_PACKAGE, asset),
                  tint=kit.WHITE)
    return name, (w, h)


def _rows_of(boxes, slack=0.6):
    """상자를 줄로 묶는다. 세로로 겹치면 같은 줄이다."""
    rows, pool = [], sorted(boxes, key=lambda b: (b[1], b[0]))
    while pool:
        head = pool.pop(0)
        line = [head]
        for other in list(pool):
            top = max(head[1], other[1])
            bottom = min(head[1] + head[3], other[1] + other[3])
            if bottom - top > min(head[3], other[3]) * slack:
                line.append(other)
                pool.remove(other)
        rows.append(sorted(line, key=lambda b: b[0]))
    return rows


def _face_box(boxes, w, h):
    """얼굴이 들어갈 상자. 네모지고 크고 왼쪽에 있다."""
    picks = [b for b in boxes
             if 0.55 < b[2] / float(b[3]) < 1.8
             and b[2] > w * 0.10 and b[3] > h * 0.35]
    if not picks:
        return None
    return min(picks, key=lambda b: (b[0], -b[2] * b[3]))


def _inside(box, w, h, pad=0.02):
    """판 밖으로 나가지 않게 가둔다.

    잰 자리를 그대로 쓰면 대체로 맞지만, 옆에 붙이는 값 글자(체력 "90/100")
    까지 자리를 재 준 것은 아니라 판을 넘어가는 일이 있다 -- 6안 적 이름과
    체력 숫자가 판 오른쪽 밖으로 나갔다. 넘치면 안으로 민다.
    """
    x, y, bw, bh = box
    margin = min(w, h) * pad
    bw = min(bw, w - margin * 2)
    bh = min(bh, h - margin * 2)
    x = min(max(x, margin), w - bw - margin)
    y = min(max(y, margin), h - bh - margin)
    return x, y, bw, bh


def _bar_box(boxes):
    """막대 상자. 가로로 길고 납작한 것 중 제일 긴 것.

    줄 번호로 고르다 실패했다 -- 얼굴 옆에 이름과 막대가 같은 줄에 오는
    시안이 있어 줄이 한 칸씩 밀렸고, 체력 막대가 이름 상자를 쓰는 바람에
    빈 캡슐이 떴다. 막대는 생김새로 알아본다.
    """
    # 너무 납작한 것은 막대가 아니라 판에 그어진 선이다 -- 6안 기사 칸에서
    # 아래 가장자리 선을 체력 막대로 골라 바닥에 붙었다.
    picks = [b for b in boxes if 3.0 < b[2] / float(b[3]) < 22.0 and b[3] > 8]
    return max(picks, key=lambda b: b[2]) if picks else None


def _pip_box(boxes, bar, h):
    """AP 보석이 놓이던 상자. 막대보다 아래에 있고 가로로 넓다."""
    floor = (bar[1] + bar[3]) if bar else h * 0.5
    picks = [b for b in boxes if b[1] >= floor - 4 and b[2] > b[3] * 1.2]
    return max(picks, key=lambda b: b[2]) if picks else None


# ─── 역할별 내용 ───────────────────────────────────────────────────────────────

def _text_plate(bp, root, row, holder_name, widget, text, size, align):
    """글자 한 줄만 얹는 판. 라운드 표시와 목표 현판이 이렇다."""
    holder, (w, h) = _plate(bp, root, row, holder_name)
    pad = w * 0.06
    kit.label(bp, widget, holder, pad, h * 0.28, w - pad * 2, h * 0.46,
              text, size, kit.TEXT_ON_LIGHT, align, (w, h), bold=True)
    return holder


def party(bp, root, row, index, table=None):
    """아군 한 줄을 제 자리에 놓고 채운다."""
    holder, size = _plate(bp, root, row, "PartyPlate_%d" % index)
    if table:
        _party_by_table(bp, holder, size, index, table)
        return
    fill_party(bp, holder, size, (0, 0) + size, row["holes"], index, K,
               row.get("boxes"))


def _at(spot):
    """시안 픽셀로 적힌 자리를 설계 캔버스로 옮긴다."""
    return [v * K for v in spot]


def _party_by_table(bp, holder, size, index, table):
    """자리표대로 아군 줄을 채운다. 짐작할 것이 없다."""
    w, h = size
    card = "PartyCard_%d" % index
    kit.add(bp, "CanvasPanel", card, holder)
    kit.place(bp, card, 0, 0, w, h, "tl", size, kit.Z_CONTENT)
    body = "PartyContent_%d" % index
    kit.add(bp, "CanvasPanel", body, card)
    kit.place(bp, body, 0, 0, w, h, "tl", (w, h), kit.Z_CONTENT)

    px, py, pw, ph = _at(table["portrait"])
    kit.image(bp, "PartyPortrait_%d" % index, body, px, py, pw, ph, (w, h),
              texture=kit.PARTY_PORTRAITS[index % 3], tint=kit.WHITE)
    nx, ny, nw, nh = _at(table["name"])
    kit.label(bp, "PartyName_%d" % index, body, nx, ny, nw, nh, "이름", 19,
              kit.TEXT_COLOR, "left", (w, h), bold=True)
    ix, iy, iw, _ih = _at(table["status_icon"])
    kit.tag(bp, "PartyStatusIcon_%d" % index, body, ix, iy, iw, "Poison",
            (w, h))
    sx, sy, sw, sh = _at(table["status"])
    kit.label(bp, "PartyStatus_%d" % index, body, sx, sy, sw, sh, "", 14,
              kit.GOLD, "left", (w, h))
    bx, by, bw, bh = _at(table["hp_bar"])
    kit.bar(bp, "PartyHPBar_%d" % index, body, bx, by, bw, bh, kit.HP_GREEN,
            (w, h))
    tx, ty, tw, th = _at(table["hp_text"])
    kit.label(bp, "PartyHPText_%d" % index, body, tx, ty, tw, th, "0/0", 15,
              kit.HP_TEXT, "left", (w, h), bold=True)

    gx, gy, side, step, count = table["pips"]
    gx, gy, side, step = gx * K, gy * K, side * K, step * K
    for slot in range(count):
        for state, texture in (("Bg", "KK_Gem_Blue_Off"),
                               ("", "KK_Gem_Blue_On")):
            kit.image(bp, "PartyAPPip%s_%d_%d" % (state, index, slot), body,
                      gx + slot * step, gy, side, side, (w, h),
                      texture=kit.KK + "/" + texture, tint=kit.WHITE)
    ax, ay, aw, ah = _at(table["ap_text"])
    kit.label(bp, "PartyAPText_%d" % index, body, ax, ay, aw, ah, "0/0", 14,
              kit.AP_TEXT, "left", (w, h))
    select_mark(bp, "PartySelected_%d" % index, body, "party", w, h)


def fill_party(bp, holder, size, box, holes, index, scale, boxes=None):
    """아군 한 줄의 내용. 시안에서 잰 자리에 놓는다.

    boxes 는 글자 있는 판에서 빈 판을 빼서 구한 자리다 -- 얼굴 하나, 이름
    줄 하나, 체력 줄 하나, AP 줄 하나가 대체로 나온다. 그게 없으면(붙은
    조각이라 짝이 없는 경우) 비율로 잡는다.

    box 는 holder 안에서의 자리다. 조각을 통째로 놓을 때는 holder 전체이고,
    밴드 안에서는 밴드를 나눈 한 칸이다.
    """
    bx, by, w, h = box
    card = "PartyCard_%d" % index
    kit.add(bp, "CanvasPanel", card, holder)
    kit.place(bp, card, bx, by, w, h, "tl", size, kit.Z_CONTENT)
    body = "PartyContent_%d" % index
    kit.add(bp, "CanvasPanel", body, card)
    kit.place(bp, body, 0, 0, w, h, "tl", (w, h), kit.Z_CONTENT)
    K = scale

    slots = [[v * K for v in b] for b in (boxes or [])]
    face = _face_box(slots, w, h)
    if face:
        kit.image(bp, "PartyPortrait_%d" % index, body, face[0], face[1],
                  face[2], face[3], (w, h),
                  texture=kit.PARTY_PORTRAITS[index % 3], tint=kit.WHITE)
        slots = [b for b in slots if b is not face]
    else:
        # 잰 자리가 없으면 판 왼쪽에 비율로 잡는다. 얼굴을 통째로 빼면 어느
        # 아군인지 화면에서 알 수 없다.
        side = min(h * 0.74, w * 0.26)
        kit.image(bp, "PartyPortrait_%d" % index, body, w * 0.035,
                  (h - side) / 2.0, side, side, (w, h),
                  texture=kit.PARTY_PORTRAITS[index % 3], tint=kit.WHITE)

    left = (face[0] + face[2] + w * 0.03) if face else w * 0.30
    span = max(w - left - w * 0.04, w * 0.2)
    line = h * 0.24

    bar = _bar_box(slots)
    pips = _pip_box([e for e in slots if e is not bar], bar, h)
    rest = [e for e in slots if e is not bar and e is not pips]
    rest.sort(key=lambda e: (e[1], e[0]))

    # 이름은 남은 것 중 제일 위. 같은 줄에 하나 더 있으면 그게 상태 표시다.
    nx, ny, nw, nh = _inside(
        rest[0] if rest else (left, h * 0.06, span * 0.55, line), w, h)
    kit.label(bp, "PartyName_%d" % index, body, nx, ny, max(nw, span * 0.3),
              nh, "이름", 19, kit.TEXT_COLOR, "left", (w, h), bold=True)
    same = [e for e in rest[1:]
            if e[1] < ny + nh and e[1] + e[3] > ny and e[0] > nx]
    sx, sy, sw, sh = (same[0] if same
                      else (left + span * 0.62, ny, span * 0.36, nh))
    kit.tag(bp, "PartyStatusIcon_%d" % index, body, sx, sy, min(sh, 24),
            "Poison", (w, h))
    kit.label(bp, "PartyStatus_%d" % index, body, sx + min(sh, 24) + 4, sy,
              max(sw - sh - 4, span * 0.25), sh, "", 14, kit.GOLD, "left",
              (w, h))

    hx, hy, hw, hh = (bar if bar else (left, h * 0.42, span * 0.52,
                                       min(h * 0.14, 18)))
    kit.bar(bp, "PartyHPBar_%d" % index, body, hx, hy, hw, hh,
            kit.HP_GREEN, (w, h))
    tx, ty, tw, th = _inside((hx + hw + w * 0.02, hy - hh * 0.5,
                              span * 0.34, hh * 2.0), w, h)
    kit.label(bp, "PartyHPText_%d" % index, body, tx, ty, tw, th, "0/0", 15,
              kit.HP_TEXT, "left", (w, h), bold=True)

    ax, ay, aw, ah = (pips if pips
                      else (left, h * 0.70, span * 0.5, min(h * 0.20, 26)))
    pip = min(ah, aw / 4.4)
    for slot in range(4):
        for state, texture in (("Bg", "KK_Gem_Blue_Off"),
                               ("", "KK_Gem_Blue_On")):
            kit.image(bp, "PartyAPPip%s_%d_%d" % (state, index, slot), body,
                      ax + slot * pip * 1.1, ay, pip, pip, (w, h),
                      texture=kit.KK + "/" + texture, tint=kit.WHITE)
    kit.label(bp, "PartyAPText_%d" % index, body, ax + pip * 4.6, ay,
              span * 0.28, max(ah, 20), "0/0", 14, kit.AP_TEXT, "left",
              (w, h))
    select_mark(bp, "PartySelected_%d" % index, body, "party", w, h)


def skill(bp, root, row, index, table=None):
    """스킬 카드 한 장을 제 자리에 놓고 채운다."""
    holder, size = _plate(bp, root, row, "CommandPlate_%d" % index)
    if table:
        _skill_by_table(bp, holder, size, index, table)
        return
    fill_skill(bp, holder, size, (0, 0) + size, index, row.get("boxes"))


def _skill_by_table(bp, holder, size, index, table):
    """자리표대로 스킬 카드를 채운다."""
    w, h = size
    card = "CommandCard_%d" % index
    kit.add(bp, "CanvasPanel", card, holder)
    kit.place(bp, card, 0, 0, w, h, "tl", size, kit.Z_CONTENT)
    body = "CommandBody_%d" % index
    kit.add(bp, "CanvasPanel", body, card)
    kit.place(bp, body, 0, 0, w, h, "tl", (w, h), kit.Z_CONTENT)

    ix, iy, iw, ih = _at(table["icon"])
    kit.image(bp, "CommandIcon_%d" % index, body, ix, iy, iw, ih, (w, h),
              texture=kit.COMMAND_ICONS[index % 6], tint=kit.WHITE)
    # 값 배지는 시안에 둥근 판이 깔려 있다. 글자만 얹으면 허공에 뜬다.
    cx, cy, cw, ch = _at(table["cost"])
    kit.image(bp, "CommandCostPlate_%d" % index, body, cx, cy, cw, ch, (w, h),
              texture=kit.KK + "/KK_Badge_Round", tint=kit.WHITE)
    kit.label(bp, "CommandCost_%d" % index, body, cx, cy + ch * 0.16, cw,
              ch * 0.7, "0", 17, kit.BADGE_TEXT, "center", (w, h), bold=True)
    for key, widget, text, tint, size_pt in (
            ("name", "CommandName", "이름", kit.TEXT_COLOR, 16),
            ("damage", "CommandDamage", "0~0", kit.DAMAGE_TEXT, 13),
            ("cooldown", "CommandCooldown", "", kit.COOLDOWN_TEXT, 13),
            ("cost_line", "CommandCostLine", "", kit.AP_TEXT, 15)):
        x, y, bw, bh = _at(table[key])
        kit.label(bp, "%s_%d" % (widget, index), body, x, y, bw, bh, text,
                  size_pt, tint, "center", (w, h), bold=(key == "name"))
    # 쿨타임 아이콘은 두지 않는다. 시안은 "쿨 2턴" 글자만 있고, 아이콘을
    # 붙였더니 카드 왼쪽 가장자리에 모래시계가 하나 떠 있었다.

    kit.ghost_button(bp, "CommandButton_%d" % index, body, 0, 0, w, h, (w, h))
    kit.image(bp, "CommandDisabled_%d" % index, body, 0, 0, w, h, (w, h),
              z_order=kit.Z_OVERLAY, tint=kit.DISABLED_VEIL)
    select_mark(bp, "CommandSelected_%d" % index, body, "skill", w, h)


def fill_skill(bp, holder, size, box, index, boxes=None):
    """스킬 카드 내용. 시안에서 잰 자리에 놓는다.

    카드는 시안마다 생김새가 크게 다르다 -- 세로로 선 것, 가로로 누운 것,
    아이콘이 왼쪽인 것, 가운데인 것. 비율로 찍었더니 6안에서 이름이 가운데로
    가고 아이콘이 구석에 붙었다. 잰 자리가 있으면 그대로 쓴다.
    """
    bx, by, w, h = box
    card = "CommandCard_%d" % index
    kit.add(bp, "CanvasPanel", card, holder)
    kit.place(bp, card, bx, by, w, h, "tl", size, kit.Z_CONTENT)
    body = "CommandBody_%d" % index
    kit.add(bp, "CanvasPanel", body, card)
    kit.place(bp, body, 0, 0, w, h, "tl", (w, h), kit.Z_CONTENT)

    slots = [[v * K for v in e] for e in (boxes or [])]
    icon = _face_box(slots, w, h)
    if icon:
        kit.image(bp, "CommandIcon_%d" % index, body, icon[0], icon[1],
                  icon[2], icon[3], (w, h),
                  texture=kit.COMMAND_ICONS[index % 6], tint=kit.WHITE)
        slots = [e for e in slots if e is not icon]
    else:
        side = min(w * 0.52, h * 0.34)
        kit.image(bp, "CommandIcon_%d" % index, body, (w - side) / 2.0,
                  h * 0.22, side, side, (w, h),
                  texture=kit.COMMAND_ICONS[index % 6], tint=kit.WHITE)

    # 값 배지는 작고 구석에 있다. 이름 줄과 헷갈리지 않게 먼저 뺀다.
    corner = [e for e in slots
              if e[2] < w * 0.24 and e[3] < h * 0.24
              and (e[0] > w * 0.6 or e[0] + e[2] < w * 0.4)]
    badge = min(corner, key=lambda e: e[1]) if corner else None
    if badge:
        slots = [e for e in slots if e is not badge]
    cx, cy, cw, ch = (badge if badge
                      else (w * 0.76, h * 0.05, w * 0.20, h * 0.13))
    kit.label(bp, "CommandCost_%d" % index, body, cx, cy, cw, ch, "0", 16,
              kit.BADGE_TEXT, "center", (w, h), bold=True)

    rows = _rows_of(slots)
    lines = [row[0] if len(row) == 1 else
             [min(e[0] for e in row), min(e[1] for e in row),
              max(e[0] + e[2] for e in row) - min(e[0] for e in row),
              max(e[3] for e in row)] for row in rows]

    def line(slot, fallback):
        return lines[slot] if slot < len(lines) else fallback

    nx, ny, nw, nh = line(0, (w * 0.05, h * 0.05, w * 0.90, h * 0.13))
    kit.label(bp, "CommandName_%d" % index, body, nx, ny, nw, nh, "이름", 16,
              kit.TEXT_COLOR, "center" if nw > w * 0.7 else "left", (w, h),
              bold=True)
    dx, dy, dw, dh = line(1, (w * 0.05, h * 0.62, w * 0.90, h * 0.12))
    kit.label(bp, "CommandDamage_%d" % index, body, dx, dy, dw, dh, "0~0", 13,
              kit.DAMAGE_TEXT, "center" if dw > w * 0.7 else "left", (w, h))
    ox, oy, ow, oh = line(2, (w * 0.05, h * 0.76, w * 0.90, h * 0.12))
    kit.tag(bp, "CommandCooldownIcon_%d" % index, body, ox, oy,
            min(oh, 18), "Cooldown", (w, h))
    kit.label(bp, "CommandCooldown_%d" % index, body, ox + min(oh, 18) + 4,
              oy, max(ow - oh - 4, w * 0.4), oh, "", 12, kit.COOLDOWN_TEXT,
              "left", (w, h))
    px, py, pw, ph = line(3, (w * 0.05, h * 0.87, w * 0.90, h * 0.12))
    kit.label(bp, "CommandCostLine_%d" % index, body, px, py, pw, ph, "", 15,
              kit.AP_TEXT, "center" if pw > w * 0.7 else "left", (w, h),
              bold=True)

    kit.ghost_button(bp, "CommandButton_%d" % index, body, 0, 0, w, h, (w, h))
    kit.image(bp, "CommandDisabled_%d" % index, body, 0, 0, w, h, (w, h),
              z_order=kit.Z_OVERLAY, tint=kit.DISABLED_VEIL)
    select_mark(bp, "CommandSelected_%d" % index, body, "skill", w, h)


def _turn_by_table(bp, holder, size, index, table):
    """자리표대로 턴 칸 하나를 채운다. 칸 하나가 곧 한 사람이다."""
    w, h = size
    slot = "TurnToken_%d" % index
    kit.add(bp, "CanvasPanel", slot, holder)
    px, py, pw, ph = _at(table["portrait"])
    kit.place(bp, slot, px, py, pw, ph, "tl", size, kit.Z_CONTENT)
    face = (kit.TURN_PORTRAITS[index]
            if index < len(kit.TURN_PORTRAITS) else None)
    kit.image(bp, "TurnPortrait_%d" % index, slot, 0, 0, pw, ph, (pw, ph),
              texture=face, tint=kit.WHITE)
    select_mark(bp, "TurnCurrent_%d" % index, slot, "turn", pw, ph)


def _enemy_by_table(bp, holder, size, table):
    """자리표대로 적 정보를 채운다."""
    w, h = size
    px, py, pw, ph = _at(table["portrait"])
    kit.image(bp, "EnemyPortrait", holder, px, py, pw, ph, (w, h),
              texture=kit.HEADS + "/KK_Face_Enemy_Eagle_HeadV2",
              tint=kit.WHITE)
    nx, ny, nw, nh = _at(table["name"])
    kit.label(bp, "EnemyName", holder, nx, ny, nw, nh, "적", 19,
              kit.TEXT_COLOR, "left", (w, h), bold=True)
    bx, by, bw, bh = _at(table["hp_bar"])
    kit.bar(bp, "EnemyHPBar", holder, bx, by, bw, bh, kit.HP_RED, (w, h))
    tx, ty, tw, th = _at(table["hp_text"])
    kit.label(bp, "EnemyHPText", holder, tx, ty, tw, th, "0/0", 15,
              kit.TEXT_COLOR, "left", (w, h), bold=True)
    for icon_key, text_key, widget, tag, tint in (
            ("defense_icon", "defense", "EnemyDefense", "Defense",
             kit.TEXT_DIM),
            ("forecast_icon", "forecast", "EnemyForecast", "Damage",
             kit.DAMAGE_TEXT)):
        ix, iy, iw, _ih = _at(table[icon_key])
        kit.tag(bp, widget + "Icon", holder, ix, iy, iw, tag, (w, h))
        x, y, bw2, bh2 = _at(table[text_key])
        kit.label(bp, widget, holder, x, y, bw2, bh2, "", 14, tint, "left",
                  (w, h))
    sx, sy, sw, sh = _at(table["status"])
    kit.label(bp, "EnemyStatus", holder, sx, sy, sw, sh, "", 13, kit.GOLD,
              "center", (w, h))


def turn(bp, root, row, first):
    """턴 순서. 초상 구멍마다 한 칸이고, 구멍이 없으면 조각 하나가 한 칸이다.

    시안이 갈린다 -- 1안은 다섯 칸이 한 판에 뚫려 있고 6안 10안은 칸마다
    조각이 따로다. 구멍 개수로 저절로 갈린다.

    돌려주는 값은 다음 조각이 이어 쓸 칸 번호다.
    """
    holder, (w, h) = _plate(bp, root, row, "Chrome_Turn_%d" % first)
    faces = _round_holes(row["holes"], row["rect"][2], row["rect"][3])
    if len(faces) >= 2:
        cells = [(fx * K, fy * K, fw * K, fh * K) for fx, fy, fw, fh in faces]
    elif w > h * 2.2:
        # 빈 판에는 초상 구멍이 안 뚫려 있다. 가로로 긴 판은 다섯 칸으로
        # 나눈다 -- 안 그러면 한 칸이 판 전체를 덮어 얼굴 하나만 크게 뜬다.
        pad = h * 0.10
        step = (w - pad * 2) / 5.0
        cells = [(pad + i * step, pad, step * 0.92, h - pad * 2)
                 for i in range(5)]
    else:
        cells = [(0.0, 0.0, w, h)]
    for offset, (cx, cy, cw, ch) in enumerate(cells):
        index = first + offset
        slot = "TurnToken_%d" % index
        kit.add(bp, "CanvasPanel", slot, holder)
        kit.place(bp, slot, cx, cy, cw, ch, "tl", (w, h), kit.Z_CONTENT)
        face = (kit.TURN_PORTRAITS[index]
                if index < len(kit.TURN_PORTRAITS) else None)
        kit.image(bp, "TurnPortrait_%d" % index, slot, 0, 0, cw, ch,
                  (cw, ch), texture=face, tint=kit.WHITE)
        select_mark(bp, "TurnCurrent_%d" % index, slot, "turn", cw, ch)
    return first + len(cells)


def enemy(bp, root, row, ordinal):
    """적 정보. 초상이 있으면 그 오른쪽에, 없으면 판 전체에 글자를 쌓는다.

    적 판은 시안마다 가장 많이 달라진다 -- 2안은 세로로 길고 14안 20안은
    이름표만 하다. 초상 유무와 가로세로 비율로 갈린다. 이름은 하나뿐이라
    두 번째 조각(20안의 예상 피해 쪽지)은 글자만 얹는다.
    """
    if ordinal > 0:
        holder, (w, h) = _plate(bp, root, row, "EnemyExtra_%d" % ordinal)
        kit.label(bp, "EnemyForecast", holder, w * 0.06, h * 0.30,
                  w * 0.88, h * 0.44, "", 14, kit.DAMAGE_TEXT, "center",
                  (w, h))
        return

    holder, size = _plate(bp, root, row, "EnemyPanel")
    _enemy_body(bp, holder, size, row.get("boxes"))


def _enemy_body(bp, holder, size, boxes=None):
    """적 정보 내용. 시안에서 잰 자리에 놓는다.

    적 판은 시안마다 가장 많이 달라진다 -- 2안은 세로로 길고 14안 20안은
    이름표만 하다. 잰 자리를 쓰면 그 차이를 코드가 알 필요가 없다.
    """
    w, h = size
    slots = [[v * K for v in e] for e in (boxes or [])]
    face = _face_box(slots, w, h)
    if face:
        kit.image(bp, "EnemyPortrait", holder, face[0], face[1], face[2],
                  face[3], (w, h), texture=kit.HEADS + "/KK_Face_Enemy_Eagle_HeadV2",
                  tint=kit.WHITE)
        slots = [e for e in slots if e is not face]

    left = w * 0.06
    if face and face[3] < h * 0.55:
        left = face[0] + face[2] + w * 0.04
    span = max(w - left - w * 0.05, w * 0.3)
    step = h * 0.13

    bar = _bar_box(slots)
    rest = sorted([e for e in slots if e is not bar],
                  key=lambda e: (e[1], e[0]))

    nx, ny, nw, nh = _inside(
        rest[0] if rest else (left, h * 0.10, span, step), w, h)
    kit.label(bp, "EnemyName", holder, nx, ny, max(nw, span * 0.4), nh, "적",
              19, kit.TEXT_COLOR, "left", (w, h), bold=True)
    hx, hy, hw, hh = (bar if bar
                      else (left, ny + step * 1.2, span * 0.6,
                            min(step * 0.6, 18)))
    kit.bar(bp, "EnemyHPBar", holder, hx, hy, hw, hh, kit.HP_RED, (w, h))
    tx, ty, tw, th = _inside((hx + hw + w * 0.02, hy - hh * 0.5,
                              span * 0.3, hh * 2.0), w, h)
    kit.label(bp, "EnemyHPText", holder, tx, ty, tw, th, "0/0", 15,
              kit.TEXT_COLOR, "left", (w, h), bold=True)

    # 이름 아래에 남은 줄들이 방어 / 예상 피해 / 상태 순으로 온다.
    below = [e for e in rest[1:] if e[1] > hy]
    fallback = [(left, hy + step * (i + 1), span, step * 0.8)
                for i in range(3)]
    for slot, (name, icon, colour) in enumerate((
            ("EnemyDefense", "Defense", kit.TEXT_DIM),
            ("EnemyForecast", "Damage", kit.DAMAGE_TEXT),
            ("EnemyStatus", None, kit.GOLD))):
        ex, ey, ew, eh = _inside(below[slot] if slot < len(below)
                                 else fallback[slot], w, h)
        ew = max(ew, span * 0.4)
        if icon:
            kit.tag(bp, name + "Icon", holder, ex, ey, min(eh, 20), icon,
                    (w, h))
            ex, ew = ex + min(eh, 20) + 4, ew - min(eh, 20) - 4
        kit.label(bp, name, holder, ex, ey, ew, eh, "", 14, colour, "left",
                  (w, h))


def end_turn(bp, root, row):
    holder, (w, h) = _plate(bp, root, row, "Chrome_EndTurn")
    kit.ghost_button(bp, "EndTurnButton", holder, 0, 0, w, h, (w, h))
    kit.label(bp, "EndTurnLabel", holder, w * 0.06, h * 0.28, w * 0.88,
              h * 0.46, "턴 종료", 26, kit.TEXT_COLOR, "center", (w, h),
              bold=True)


# ─── 여러 역할이 한 조각에 든 것 ───────────────────────────────────────────────

#: 밴드를 나눌 때 역할이 차지하는 몫. 시안에서 재 본 대략의 비율이다.
BAND_WEIGHT = {"round": 1.0, "objective": 1.6, "turn": 2.2, "party": 3.2,
               "skill": 5.4, "enemy": 2.0, "endturn": 1.2}


def _merge(spans):
    """겹치거나 붙은 구간을 잇는다."""
    out = []
    for lo, hi in sorted(spans):
        if out and lo <= out[-1][1] + 1:
            out[-1][1] = max(out[-1][1], hi)
        else:
            out.append([lo, hi])
    return out


def _cuts(boxes, along_x, count, lo, hi):
    """잰 자리 사이의 빈틈에서 칸을 가른다.

    밴드를 내가 정한 몫으로 쪼개고 있었다 -- 아군 3.2, 스킬 5.4 같은 숫자다.
    시안에서 재어 적은 값이라 시안마다 몇 십 px 씩 어긋났고, 15안에서는
    글자가 카드 사이 이음매에 얹혔다.

    판에 무엇이 놓여 있었는지는 이미 재어 두었다. 그것들 사이의 빈틈이 곧
    칸 경계다. 큰 빈틈부터 필요한 수만큼 고른다.

    가를 수 없으면(잰 자리가 모자라면) 고르게 나눈다.
    """
    axis = 0 if along_x else 1
    size = 2 if along_x else 3
    spans = _merge([[b[axis], b[axis] + b[size]] for b in boxes
                    if lo - 1 <= b[axis] <= hi])
    holes = [(spans[i + 1][0] - spans[i][1], (spans[i][1] + spans[i + 1][0]) / 2.0)
             for i in range(len(spans) - 1)]
    holes.sort(reverse=True)
    picked = sorted(mid for _gap, mid in holes[:count - 1])
    if len(picked) < count - 1:
        step = (hi - lo) / float(count)
        picked = [lo + step * (i + 1) for i in range(count - 1)]
    edges = [lo] + picked + [hi]
    return [(edges[i], edges[i + 1]) for i in range(count)]


def band(bp, root, row, ordinal, counters):
    """여러 역할이 한 장에 든 조각. 판을 놓고 안을 몫대로 나눠 채운다.

    5안 9안 15안 19안처럼 아래(또는 위)가 통짜 판인 시안이 있다. 판을 쪼개
    따로 놓으면 이음매가 생기므로 통째로 놓고, 그 안을 나눠 내용만 얹는다.

    나누는 비율은 재서 적은 값이라 시안마다 몇 픽셀씩 어긋난다. 판 그림은
    시안 그대로이므로 어긋나는 것은 글자 자리뿐이고, 눈에 띄면 그 시안만
    BAND_WEIGHT 대신 손으로 잡아 주면 된다.
    """
    holder, size = _plate(bp, root, row, "Chrome_Band_%d" % ordinal)
    w, h = size
    contents = row.get("contents") or []
    if not contents:
        return

    slots = [[v * K for v in b] for b in (row.get("boxes") or [])]
    _split(bp, holder, size, (0.0, 0.0, w, h), contents, row, counters, slots)


def _weight(entry):
    if isinstance(entry, list):
        return max(BAND_WEIGHT.get(name, 1.0) for name in entry)
    return BAND_WEIGHT.get(entry, 1.0)


def _split(bp, holder, size, box, contents, row, counters, slots):
    """칸을 긴 쪽으로 나눠 내용을 앉힌다. 묶음은 다시 반대 방향으로 나뉜다.

    한 방향으로만 나누면 표현 못 하는 배치가 있다 -- 15안은 적 패널과 턴종료
    버튼이 오른쪽에 위아래로 포개져 있는데, 가로로만 나누면 둘이 나란히
    서서 턴종료 글자가 적 패널 위에 얹혔다. 표에 묶음으로 적어 두면 그
    묶음만 세로로 다시 나눈다.
    """
    bx, by, bw, bh = box
    along_x = bw >= bh
    lo = bx if along_x else by
    hi = lo + (bw if along_x else bh)
    parts = _cuts(slots, along_x, len(contents), lo, hi)

    for entry, (start, stop) in zip(contents, parts):
        cell = ((start, by, stop - start, bh) if along_x
                else (bx, start, bw, stop - start))
        if isinstance(entry, list):
            _split(bp, holder, size, cell, entry, row, counters, slots)
        else:
            _band_part(bp, holder, size, cell, entry, row, counters, slots)


def _grid(w, h, count, want):
    """칸 모양이 want(가로/세로)에 가장 가까운 열·행 수."""
    best = None
    for cols in range(1, count + 1):
        rows = (count + cols - 1) // cols
        if cols * rows > count + 1:
            continue
        got = (w / cols) / max(h / rows, 1.0)
        miss = abs(got - want)
        if best is None or miss < best[0]:
            best = (miss, cols, rows)
    return best[1], best[2]


def _band_part(bp, holder, size, box, role, row, counters, slots=()):
    """밴드에서 나눈 한 칸을 그 역할로 채운다."""
    bx, by, bw, bh = box
    if role == "round":
        kit.label(bp, "RoundText", holder, bx, by + bh * 0.30, bw, bh * 0.40,
                  "ROUND 1", 24, kit.TEXT_ON_LIGHT, "center", size, bold=True)
    elif role == "objective":
        kit.label(bp, "ObjectiveText", holder, bx, by + bh * 0.32, bw,
                  bh * 0.36, "목표", 18, kit.TEXT_ON_LIGHT, "center", size)
    elif role == "endturn":
        kit.ghost_button(bp, "EndTurnButton", holder, bx, by, bw, bh, size)
        kit.label(bp, "EndTurnLabel", holder, bx, by + bh * 0.34, bw,
                  bh * 0.34, "턴 종료", 24, kit.TEXT_COLOR, "center", size,
                  bold=True)
    elif role in ("party", "skill"):
        # 아군은 세 줄, 스킬은 여섯 장. 칸 모양이 카드에 가깝도록 격자를
        # 고른다 -- 한 줄로만 폈더니 15안 카드가 세로로 홀쭉해져, 판에
        # 그려진 3x2 자리와 글자가 어긋났다.
        count = 3 if role == "party" else 6
        want = 2.4 if role == "party" else 0.85
        cols, rows = _grid(bw, bh, count, want)
        # 열과 행 경계도 잰 자리의 빈틈에서 가른다.
        xs = _cuts(slots, True, cols, bx, bx + bw)
        ys = _cuts(slots, False, rows, by, by + bh)
        for slot in range(count):
            index = counters[role]
            counters[role] = index + 1
            x0, x1 = xs[slot % cols]
            y0, y1 = ys[slot // cols]
            cell = (x0, y0, x1 - x0, y1 - y0)
            inside = [b for b in slots
                      if x0 <= b[0] <= x1 and y0 <= b[1] <= y1]
            local = [[b[0] - x0, b[1] - y0, b[2], b[3]] for b in inside]
            if role == "party":
                fill_party(bp, holder, size, cell, [], index, 1.0, local)
            else:
                fill_skill(bp, holder, size, cell, index, local)
    elif role == "enemy":
        sub = "EnemyPanel"
        kit.add(bp, "CanvasPanel", sub, holder)
        kit.place(bp, sub, bx, by, bw, bh, "tl", size, kit.Z_CONTENT)
        _enemy_body(bp, sub, (bw, bh))
    elif role == "turn":
        faces = _round_holes(row["holes"], row["rect"][2], row["rect"][3])
        inside = [f for f in faces
                  if bx <= f[0] * K <= bx + bw and by <= f[1] * K <= by + bh]
        cells = ([(f[0] * K, f[1] * K, f[2] * K, f[3] * K) for f in inside]
                 if inside else
                 [(bx + i * bw / 5.0, by, bw / 5.0 * 0.94, bh)
                  for i in range(5)])
        for offset, (cx, cy, cw, ch) in enumerate(cells):
            index = counters["turn"]
            counters["turn"] = index + 1
            slot = "TurnToken_%d" % index
            kit.add(bp, "CanvasPanel", slot, holder)
            kit.place(bp, slot, cx, cy, cw, ch, "tl", size, kit.Z_CONTENT)
            face = (kit.TURN_PORTRAITS[index]
                    if index < len(kit.TURN_PORTRAITS) else None)
            kit.image(bp, "TurnPortrait_%d" % index, slot, 0, 0, cw, ch,
                      (cw, ch), texture=face, tint=kit.WHITE)
            select_mark(bp, "TurnCurrent_%d" % index, slot, "turn", cw, ch)


# ─── 배치안 한 벌 ──────────────────────────────────────────────────────────────

def _one_text(bp, holder, size, widget, text, points, colour, table, key,
              align):
    """글자 한 줄. 자리표가 있으면 그 자리, 없으면 판 가운데."""
    w, h = size
    if table and key in table:
        x, y, bw, bh = _at(table[key])
    else:
        pad = w * 0.06
        x, y, bw, bh = pad, h * 0.28, w - pad * 2, h * 0.46
    kit.label(bp, widget, holder, x, y, bw, bh, text, points, colour, align,
              (w, h), bold=True)


def build(bp, root, number):
    """시안 하나를 배치안으로 짓는다.

    조각이 없는 구역은 위젯도 만들지 않는다. 없는 것을 빈 상자로 채우면
    화면에서는 안 보이는데 코드에서는 있는 것처럼 보여, 나중에 왜 안 나오는지
    찾게 된다. C++ 은 못 찾은 위젯을 건너뛴다.
    """
    rows = manifest()[number]
    counters = {"party": 0, "skill": 0, "turn": 0, "enemy": 0, "band": 0}
    for row in rows:
        role = row["role"]
        # 자리표가 있으면 그대로 쓴다. 없으면 잰 상자로 짐작한다.
        table = slots(number, role)
        if role == "round":
            holder, size = _plate(bp, root, row, "Chrome_Round")
            _one_text(bp, holder, size, "RoundText", "ROUND 1", 26,
                      kit.TEXT_ON_LIGHT, table, "text", "center")
        elif role == "objective":
            holder, size = _plate(bp, root, row, "Chrome_Objective")
            if table:
                ix, iy, iw, ih = _at(table["icon"])
                kit.image(bp, "ObjectiveIcon", holder, ix, iy, iw, ih, size,
                          texture=kit.KK + "/KK_Icon_Objective",
                          tint=kit.WHITE)
            _one_text(bp, holder, size, "ObjectiveText", "목표", 19,
                      kit.TEXT_ON_LIGHT, table, "text",
                      (table or {}).get("align", "center"))
        elif role == "endturn":
            holder, size = _plate(bp, root, row, "Chrome_EndTurn")
            kit.ghost_button(bp, "EndTurnButton", holder, 0, 0,
                             size[0], size[1], size)
            _one_text(bp, holder, size, "EndTurnLabel", "턴 종료", 26,
                      kit.TEXT_COLOR, table, "text", "center")
        elif role == "party":
            party(bp, root, row, counters["party"], table)
            counters["party"] += 1
        elif role == "skill":
            skill(bp, root, row, counters["skill"], table)
            counters["skill"] += 1
        elif role == "turn":
            if table:
                holder, size = _plate(
                    bp, root, row, "Chrome_Turn_%d" % counters["turn"])
                _turn_by_table(bp, holder, size, counters["turn"], table)
                counters["turn"] += 1
            else:
                counters["turn"] = turn(bp, root, row, counters["turn"])
        elif role == "enemy":
            if table and counters["enemy"] == 0:
                holder, size = _plate(bp, root, row, "EnemyPanel")
                _enemy_by_table(bp, holder, size, table)
            else:
                enemy(bp, root, row, counters["enemy"])
            counters["enemy"] += 1
        elif role == "band":
            band(bp, root, row, counters["band"], counters)
            counters["band"] += 1
    return counters


def expected(number):
    """이 시안이 실제로 낼 수 있는 계약 이름. 검사는 이것과 견준다.

    계약을 6칸 3줄로 못 박아 두었더니, 조각이 덜 오려진 시안이 전부 실패로
    떨어졌다 -- 3안은 카드 두 장이 한 조각으로 붙어 다섯 장이고 16안은 적
    정보 하나뿐이다. 없는 조각을 빈 상자로 채우면 검사만 통과하고 화면은
    비므로, 검사 쪽을 조각에 맞춘다.
    """
    rows = manifest()[number]
    counts = {"party": 0, "skill": 0}
    have = set()
    for row in rows:
        role = row["role"]
        if role == "band":
            # 겹칸은 묶음으로 적혀 있다. 셀 때는 펴서 센다.
            flat = []
            for entry in row.get("contents", []):
                flat.extend(entry if isinstance(entry, list) else [entry])
            for name in flat:
                if name in counts:
                    counts[name] += 3 if name == "party" else 6
                else:
                    have.add(name)
        elif role in counts:
            counts[role] += 1
        else:
            have.add(role)

    names = []
    if "round" in have:
        names.append("RoundText")
    if "endturn" in have:
        names.append("EndTurnButton")
    for i in range(counts["party"]):
        names.append("PartyCard_%d" % i)
    for i in range(counts["skill"]):
        names += ["CommandCard_%d" % i, "CommandButton_%d" % i,
                  "CommandCost_%d" % i, "CommandSelected_%d" % i,
                  "CommandDisabled_%d" % i]
    return names


def select_mark(bp, name, parent, kind, w, h):
    """선택 표시. 조각 테두리가 맞으면 그걸 쓰고 아니면 금빛 덮개를 쓴다.

    테두리 조각은 1안에서 오려 낸 것이라 그 크기에만 맞는다. 2안에 그대로
    얹었더니 카드 절반 크기의 노란 사각형이 판 밖으로 삐져나왔다. 크기가
    한 뼘 이상 다르면 테두리 대신 판 전체를 금빛으로 덮는다 -- 늘린 테두리
    보다 낫고, 무엇이 골라졌는지도 분명하다.
    """
    _, sw, sh = kit.CHROME_SELECT[kind]
    aw, ah = sw * K, sh * K
    if abs(aw - w) < w * 0.25 and abs(ah - h) < h * 0.25:
        return kit.chrome_select(bp, name, parent, kind, w, h, (w, h))
    return kit.image(bp, name, parent, 0, 0, w, h, (w, h),
                     z_order=kit.Z_MARKER, tint=kit.SELECT_VEIL)
