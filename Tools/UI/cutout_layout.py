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
    arts = row.get("arts") or [[row["asset"], 0, 0] + list(row["rect"][2:])]
    for slot, (asset, ax, ay, aw, ah) in enumerate(arts):
        ax, ay, aw, ah = [v * K for v in (ax, ay, aw, ah)]
        if len(arts) == 1:
            ax, ay = (w - aw) / 2.0, (h - ah) / 2.0
        kit.image(bp, "%s_Art%d" % (name, slot), name, ax, ay, aw, ah,
                  (w, h), z_order=kit.Z_FILL,
                  texture="{}/{}".format(CUTOUT_PACKAGE, asset),
                  tint=kit.WHITE)
    return name, (w, h)


# ─── 역할별 내용 ───────────────────────────────────────────────────────────────

def _text_plate(bp, root, row, holder_name, widget, text, size, align):
    """글자 한 줄만 얹는 판. 라운드 표시와 목표 현판이 이렇다."""
    holder, (w, h) = _plate(bp, root, row, holder_name)
    pad = w * 0.06
    kit.label(bp, widget, holder, pad, h * 0.28, w - pad * 2, h * 0.46,
              text, size, kit.TEXT_ON_LIGHT, align, (w, h), bold=True)
    return holder


def party(bp, root, row, index):
    """아군 한 줄을 제 자리에 놓고 채운다."""
    holder, size = _plate(bp, root, row, "PartyPlate_%d" % index)
    fill_party(bp, holder, size, (0, 0) + size, row["holes"], index, K)


def fill_party(bp, holder, size, box, holes, index, scale):
    """아군 한 줄의 내용. 초상은 구멍 자리에, 나머지는 그 오른쪽에 세 줄로.

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
    faces = _round_holes(holes, w / K, h / K)
    if faces:
        fx, fy, fw, fh = [v * K for v in faces[0]]
        kit.image(bp, "PartyPortrait_%d" % index, body, fx, fy, fw, fh, (w, h),
                  texture=kit.PARTY_PORTRAITS[index % 3], tint=kit.WHITE)
        left = fx + fw + w * 0.03
    else:
        # 구멍이 안 잡히는 판이 있다. 빈 판은 결이 고르고, 20안 이름표는
        # 애초에 초상이 없다. 판 왼쪽에 비율로 자리를 잡는다 -- 얼굴을
        # 통째로 빼면 어느 아군인지 화면에서 알 수 없다.
        size_face = min(h * 0.74, w * 0.26)
        kit.image(bp, "PartyPortrait_%d" % index, body, w * 0.035,
                  (h - size_face) / 2.0, size_face, size_face, (w, h),
                  texture=kit.PARTY_PORTRAITS[index % 3], tint=kit.WHITE)
        left = w * 0.035 + size_face + w * 0.03

    span = w - left - w * 0.04
    line = h * 0.26
    kit.label(bp, "PartyName_%d" % index, body, left, h * 0.06,
              span * 0.55, line, "이름", 19, kit.TEXT_COLOR, "left", (w, h),
              bold=True)
    kit.tag(bp, "PartyStatusIcon_%d" % index, body, left + span * 0.60,
            h * 0.09, min(h * 0.20, 24), "Poison", (w, h))
    kit.label(bp, "PartyStatus_%d" % index, body, left + span * 0.60 + 26,
              h * 0.08, span * 0.36, line * 0.8, "", 14, kit.GOLD, "left",
              (w, h))

    kit.bar(bp, "PartyHPBar_%d" % index, body, left, h * 0.42,
            span * 0.52, min(h * 0.14, 18), kit.HP_GREEN, (w, h))
    kit.label(bp, "PartyHPText_%d" % index, body, left + span * 0.56,
              h * 0.36, span * 0.42, line, "0/0", 15, kit.HP_TEXT, "left",
              (w, h), bold=True)

    pip = min(h * 0.20, span * 0.11)
    for slot in range(4):
        for state, texture in (("Bg", "KK_Gem_Blue_Off"),
                               ("", "KK_Gem_Blue_On")):
            kit.image(bp, "PartyAPPip%s_%d_%d" % (state, index, slot), body,
                      left + slot * pip * 1.28, h * 0.68, pip, pip, (w, h),
                      texture=kit.KK + "/" + texture, tint=kit.WHITE)
    kit.label(bp, "PartyAPText_%d" % index, body, left + pip * 5.4, h * 0.66,
              span * 0.30, line, "0/0", 14, kit.AP_TEXT, "left", (w, h))
    select_mark(bp, "PartySelected_%d" % index, body, "party", w, h)


def skill(bp, root, row, index):
    """스킬 카드 한 장을 제 자리에 놓고 채운다."""
    holder, size = _plate(bp, root, row, "CommandPlate_%d" % index)
    fill_skill(bp, holder, size, (0, 0) + size, index)


def fill_skill(bp, holder, size, box, index):
    """스킬 카드 내용. 누운 카드와 선 카드를 가로세로 비율로 나눠 다룬다."""
    bx, by, w, h = box
    card = "CommandCard_%d" % index
    kit.add(bp, "CanvasPanel", card, holder)
    kit.place(bp, card, bx, by, w, h, "tl", size, kit.Z_CONTENT)
    body = "CommandBody_%d" % index
    kit.add(bp, "CanvasPanel", body, card)
    kit.place(bp, body, 0, 0, w, h, "tl", (w, h), kit.Z_CONTENT)

    wide = w > h * 1.35
    if wide:
        # 13안 18안처럼 누운 카드: 왼쪽에 아이콘, 오른쪽에 글자.
        icon = min(h * 0.66, w * 0.20)
        kit.image(bp, "CommandIcon_%d" % index, body, w * 0.04,
                  (h - icon) / 2.0, icon, icon, (w, h),
                  texture=kit.COMMAND_ICONS[index % 6], tint=kit.WHITE)
        left = w * 0.04 + icon + w * 0.04
        kit.label(bp, "CommandName_%d" % index, body, left, h * 0.12,
                  w - left - w * 0.18, h * 0.34, "이름", 17, kit.TEXT_COLOR,
                  "left", (w, h), bold=True)
        kit.label(bp, "CommandCostLine_%d" % index, body, left, h * 0.52,
                  (w - left) * 0.42, h * 0.30, "", 14, kit.AP_TEXT, "left",
                  (w, h))
        kit.label(bp, "CommandDamage_%d" % index, body,
                  left + (w - left) * 0.44, h * 0.52, (w - left) * 0.40,
                  h * 0.30, "0~0", 13, kit.DAMAGE_TEXT, "left", (w, h))
        kit.tag(bp, "CommandCooldownIcon_%d" % index, body, left,
                h * 0.84, min(h * 0.14, 18), "Cooldown", (w, h))
        kit.label(bp, "CommandCooldown_%d" % index, body, left + 22,
                  h * 0.82, (w - left) * 0.50, h * 0.18, "", 12,
                  kit.COOLDOWN_TEXT, "left", (w, h))
        badge = min(h * 0.34, w * 0.11)
        kit.label(bp, "CommandCost_%d" % index, body, w - badge - w * 0.04,
                  (h - badge) / 2.0, badge, badge, "0", 16, kit.BADGE_TEXT,
                  "center", (w, h), bold=True)
    else:
        # 1안 2안처럼 선 카드: 이름 - 아이콘 - 수치 순으로 쌓인다.
        kit.label(bp, "CommandName_%d" % index, body, w * 0.05, h * 0.05,
                  w * 0.90, h * 0.13, "이름", 16, kit.TEXT_COLOR, "center",
                  (w, h), bold=True)
        icon = min(w * 0.52, h * 0.34)
        kit.image(bp, "CommandIcon_%d" % index, body, (w - icon) / 2.0,
                  h * 0.22, icon, icon, (w, h),
                  texture=kit.COMMAND_ICONS[index % 6], tint=kit.WHITE)
        kit.label(bp, "CommandDamage_%d" % index, body, w * 0.05, h * 0.62,
                  w * 0.90, h * 0.12, "0~0", 13, kit.DAMAGE_TEXT, "center",
                  (w, h))
        kit.tag(bp, "CommandCooldownIcon_%d" % index, body, w * 0.24,
                h * 0.77, min(w * 0.11, 18), "Cooldown", (w, h))
        kit.label(bp, "CommandCooldown_%d" % index, body, w * 0.36, h * 0.75,
                  w * 0.56, h * 0.12, "", 12, kit.COOLDOWN_TEXT, "left",
                  (w, h))
        kit.label(bp, "CommandCostLine_%d" % index, body, w * 0.05, h * 0.87,
                  w * 0.90, h * 0.12, "", 15, kit.AP_TEXT, "center", (w, h),
                  bold=True)
        badge = min(w * 0.22, h * 0.13)
        kit.label(bp, "CommandCost_%d" % index, body, w - badge - w * 0.05,
                  h * 0.05, badge, badge, "0", 16, kit.BADGE_TEXT, "center",
                  (w, h), bold=True)

    kit.ghost_button(bp, "CommandButton_%d" % index, body, 0, 0, w, h, (w, h))
    kit.image(bp, "CommandDisabled_%d" % index, body, 0, 0, w, h, (w, h),
              z_order=kit.Z_OVERLAY, tint=kit.DISABLED_VEIL)
    select_mark(bp, "CommandSelected_%d" % index, body, "skill", w, h)


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
    _enemy_body(bp, holder, size, row["holes"], row["rect"][2])


def _enemy_body(bp, holder, size, holes, source_w):
    """적 정보 내용. 초상이 있으면 그 오른쪽에, 없으면 판 전체에 쌓는다.

    적 판은 시안마다 가장 많이 달라진다 -- 2안은 세로로 길고 14안 20안은
    이름표만 하다. 초상 유무와 가로세로 비율로 갈린다.
    """
    w, h = size
    faces = _round_holes(holes, w / K, h / K) if holes else []
    tall = h > w * 1.15
    if faces:
        fx, fy, fw, fh = [v * K for v in faces[0]]
        kit.image(bp, "EnemyPortrait", holder, fx, fy, fw, fh, (w, h),
                  texture=kit.KK + "/KK_Face_Eagle", tint=kit.WHITE)
        left = w * 0.06 if tall else fx + fw + w * 0.05
        top = fy + fh + h * 0.04 if tall else h * 0.10
    else:
        left, top = w * 0.06, h * 0.10

    span = w - left - w * 0.05
    line = (h - top) * (0.20 if tall else 0.24)
    kit.label(bp, "EnemyName", holder, left, top, span, line, "적", 19,
              kit.TEXT_COLOR, "left", (w, h), bold=True)
    kit.bar(bp, "EnemyHPBar", holder, left, top + line * 1.15,
            span * 0.62, min(line * 0.55, 18), kit.HP_RED, (w, h))
    kit.label(bp, "EnemyHPText", holder, left + span * 0.66,
              top + line * 1.05, span * 0.34, line, "0/0", 15,
              kit.TEXT_COLOR, "left", (w, h), bold=True)
    kit.tag(bp, "EnemyDefenseIcon", holder, left, top + line * 2.15,
            min(line * 0.6, 20), "Defense", (w, h))
    kit.label(bp, "EnemyDefense", holder, left + 24, top + line * 2.10,
              span - 24, line, "", 14, kit.TEXT_DIM, "left", (w, h))
    kit.tag(bp, "EnemyForecastIcon", holder, left, top + line * 3.15,
            min(line * 0.6, 20), "Damage", (w, h))
    kit.label(bp, "EnemyForecast", holder, left + 24, top + line * 3.10,
              span - 24, line, "", 14, kit.DAMAGE_TEXT, "left", (w, h))
    kit.label(bp, "EnemyStatus", holder, left, top + line * 4.10, span,
              line, "", 13, kit.GOLD, "center", (w, h))


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

    _split(bp, holder, size, (0.0, 0.0, w, h), contents, row, counters)


def _weight(entry):
    if isinstance(entry, list):
        return max(BAND_WEIGHT.get(name, 1.0) for name in entry)
    return BAND_WEIGHT.get(entry, 1.0)


def _split(bp, holder, size, box, contents, row, counters):
    """칸을 긴 쪽으로 나눠 내용을 앉힌다. 묶음은 다시 반대 방향으로 나뉜다.

    한 방향으로만 나누면 표현 못 하는 배치가 있다 -- 15안은 적 패널과 턴종료
    버튼이 오른쪽에 위아래로 포개져 있는데, 가로로만 나누면 둘이 나란히
    서서 턴종료 글자가 적 패널 위에 얹혔다. 표에 묶음으로 적어 두면 그
    묶음만 세로로 다시 나눈다.
    """
    bx, by, bw, bh = box
    along_x = bw >= bh
    weights = [_weight(entry) for entry in contents]
    total = sum(weights)
    cursor, pad = 0.0, (bw if along_x else bh) * 0.008
    for entry, weight in zip(contents, weights):
        length = (bw if along_x else bh) * weight / total
        cell = ((bx + cursor + pad, by, length - pad * 2, bh) if along_x
                else (bx, by + cursor + pad, bw, length - pad * 2))
        cursor += length
        if isinstance(entry, list):
            _split(bp, holder, size, cell, entry, row, counters)
        else:
            _band_part(bp, holder, size, cell, entry, row, counters)


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


def _band_part(bp, holder, size, box, role, row, counters):
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
        cw, ch = bw / cols, bh / rows
        for slot in range(count):
            index = counters[role]
            counters[role] = index + 1
            fill = fill_party if role == "party" else fill_skill
            cell = (bx + (slot % cols) * cw, by + (slot // cols) * ch,
                    cw * 0.97, ch * 0.97)
            if role == "party":
                fill(bp, holder, size, cell, [], index, K)
            else:
                fill(bp, holder, size, cell, index)
    elif role == "enemy":
        sub = "EnemyPanel"
        kit.add(bp, "CanvasPanel", sub, holder)
        kit.place(bp, sub, bx, by, bw, bh, "tl", size, kit.Z_CONTENT)
        _enemy_body(bp, sub, (bw, bh), [], 0)
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
        if role == "round":
            _text_plate(bp, root, row, "Chrome_Round", "RoundText",
                        "ROUND 1", 26, "center")
        elif role == "objective":
            _text_plate(bp, root, row, "Chrome_Objective", "ObjectiveText",
                        "목표", 19, "center")
        elif role == "endturn":
            end_turn(bp, root, row)
        elif role == "party":
            party(bp, root, row, counters["party"])
            counters["party"] += 1
        elif role == "skill":
            skill(bp, root, row, counters["skill"])
            counters["skill"] += 1
        elif role == "turn":
            counters["turn"] = turn(bp, root, row, counters["turn"])
        elif role == "enemy":
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
