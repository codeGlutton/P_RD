# -*- coding: utf-8 -*-
"""구역 조정 쪽에서 내려받은 것을 자리표·그림표·붙임표로 푼다.

## 왜 바로 못 넣나

hud04_slots.py 는 prepare_hud04.py 가 만든다. 손으로 고치면 다음에 만들 때
조용히 사라진다 -- 그 파일 머리에도 그렇게 적혀 있다.

그래서 고친 값은 따로 적고, prepare_hud04.py 가 시안을 잰 **뒤에** 그것을
덮어씌운다. 잰 것과 정한 것을 갈라 두는 것이다.

## 좌표계가 둘이다

쪽이 내려주는 값은 **판 안** 자리다(판 왼쪽 위가 원점). hud04_slots.py 의
DETAIL 은 **화면** 자리다. 여기서는 판 안 자리 그대로 적어 두고, 화면 자리로
옮기는 것은 prepare_hud04.py 가 한다 -- 판 자리가 바뀌어도 따라가야 하므로
미리 더해 두면 안 된다.

## 그림은 내용째로 온다

쪽에서 고른 파일은 자료 URL(base64)로 실려 온다. 이름만 받으면 그 파일을
어디서 찾을지 알 수 없다 -- file:// 에서 고른 파일은 경로로 다시 못 읽는다.
받은 내용을 판 그림과 같은 폴더에 풀어 두면 import_hud04.py 가 다음에
통째로 넣는다.

    python apply_zones.py "C:/Users/2009e/Downloads/zones.json"
"""
import base64
import io
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "hud04_tuning.py")
ART_OUT = os.path.join(HERE, "KayKitUIKit", "HUD04")
ART_MAP = os.path.join(HERE, "hud04_zone_art.py")
ALIGN_MAP = os.path.join(HERE, "hud04_align.py")
PLATE_MAP = os.path.join(HERE, "hud04_plate_art.py")
Z_MAP = os.path.join(HERE, "hud04_zorder.py")

NL = "\n"

HEAD = '''# -*- coding: utf-8 -*-
"""구역 조정 쪽에서 손으로 맞춘 자리. apply_zones.py 가 만든다.

손으로 고쳐도 된다. 다만 쪽에서 다시 내려받아 넣으면 덮인다.

값은 **판 안** 자리다. 판 왼쪽 위가 원점이고 (x, y, w, h) 다.
화면 자리로 옮기는 것은 prepare_hud04.py 가 한다.
"""

#: 판 이름 -> 요소 -> (x, y, w, h). 시안을 잰 값을 이것으로 덮는다.
TUNING = {
'''

ART_HEAD = '''# -*- coding: utf-8 -*-
"""구역에 얹은 그림. apply_zones.py 가 만든다.

판 이름 -> 요소 -> {texture, fit}. texture 는 HUD04 폴더의 PNG 이름이고,
import_hud04.py 가 그 폴더를 통째로 넣는다.
"""

ZONE_ART = {
'''

ALIGN_HEAD = '''# -*- coding: utf-8 -*-
"""글자 붙임. apply_zones.py 가 만든다.

요소 이름 -> (가로, 세로). 여기 없는 것은 가로세로 모두 가운데다.
"""

TEXT_ALIGN = {
'''


def write_lines(path, lines):
    io.open(path, "w", encoding="utf-8", newline=NL).write(NL.join(lines) + NL)


def png_size(raw):
    """PNG 머리에서 가로세로를 꺼낸다.

    맞춤(비율 지켜 넣기)을 굽는 쪽에서 계산하려면 그림의 원래 크기를 알아야
    한다. 굽는 것은 언리얼 안 파이썬이라 PIL 이 없을 수 있어, 머리 스물다섯
    바이트만 직접 읽는다 -- IHDR 은 늘 맨 앞이고 자리가 정해져 있다.
    """
    if raw[:8] != bytes([137, 80, 78, 71, 13, 10, 26, 10]):
        return None
    width = int.from_bytes(raw[16:20], "big")
    height = int.from_bytes(raw[20:24], "big")
    return (width, height) if width and height else None


def write_art(art):
    """얹은 그림을 PNG 로 풀고 어느 구역이 쓰는지 적는다."""
    rows = {}
    for at, value in sorted(art.items()):
        png = (value or {}).get("png") or ""
        if not png.startswith("data:"):
            continue
        plate, _, element = at.partition("|")
        raw = base64.b64decode(png.split(",", 1)[1])
        # 이름은 구역에서 짓는다. 고른 파일 이름을 쓰면 같은 그림을 두 구역에
        # 넣었을 때 서로 덮는다.
        name = "KK_HUD04_zone_%s" % element
        io.open(os.path.join(ART_OUT, name + ".png"), "wb").write(raw)
        rows.setdefault(plate, {})[element] = (
            name, (value or {}).get("fit") or "contain", png_size(raw))

    lines = [ART_HEAD.rstrip(NL)]
    for plate in sorted(rows):
        lines.append('    "%s": {' % plate)
        for element in sorted(rows[plate]):
            name, fit, size = rows[plate][element]
            lines.append(
                '        "%s": {"texture": "%s", "fit": "%s", "size": %s},'
                % (element, name, fit, list(size) if size else None))
        lines.append("    },")
    lines.append("}")
    write_lines(ART_MAP, lines)
    return sum(len(v) for v in rows.values())


def write_plate(plate):
    """갈아 끼운 판 그림을 풀고 어느 판이 쓰는지 적는다.

    구역에 얹는 그림과 따로 둔다. 이쪽은 카드 껍데기 자체라, 갈면 그 판을 쓰는
    카드가 통째로 바뀐다 -- 여섯 장이 한 판을 나눠 쓰므로 한 번 갈면 여섯이
    같이 바뀐다.
    """
    # 쪽에서 올린 것만 들어온다. 코드에서 걸어 둔 것(메뉴 막대·턴 순서 판)은
    # 쪽이 모르므로, 새로 쓰면 그대로 지워진다 -- 한 번 지워 봤다.
    # 있던 것 위에 얹는다.
    try:
        from hud04_plate_art import PLATE_ART as KEPT
    except ImportError:
        KEPT = {}
    rows = dict(KEPT)
    for name, value in sorted(plate.items()):
        png = (value or {}).get("png") or ""
        if not png.startswith("data:"):
            continue
        raw = base64.b64decode(png.split(",", 1)[1])
        texture = "KK_HUD04_plate_%s" % name
        io.open(os.path.join(ART_OUT, texture + ".png"), "wb").write(raw)
        rows[name] = texture

    lines = ['# -*- coding: utf-8 -*-',
             '"""갈아 끼운 판 그림. apply_zones.py 가 만든다.',
             '',
             '판 이름 -> HUD04 폴더의 PNG 이름. 여기 없는 판은 시안에서',
             '오려 낸 것을 그대로 쓴다.',
             '"""',
             '',
             'PLATE_ART = {']
    for name in sorted(rows):
        lines.append('    "%s": "%s",' % (name, rows[name]))
    lines.append("}")
    write_lines(PLATE_MAP, lines)
    return len(rows)


def write_zorder(z):
    """구역이 몇 층에 놓이나. 굽는 쪽이 정해 둔 값을 이것이 이긴다."""
    lines = ['# -*- coding: utf-8 -*-',
             '"""구역의 층. apply_zones.py 가 만든다.',
             '',
             '판 이름 -> 요소 -> 층. 여기 없는 것은 굽는 쪽이 정한 대로 간다',
             '(판 0, 내용 10, 글자 15, 표시 40).',
             '"""',
             '',
             'Z_ORDER = {']
    rows = {}
    for at, value in z.items():
        if not isinstance(value, int):
            continue
        plate, _, element = at.partition("|")
        rows.setdefault(plate, {})[element] = value
    for plate in sorted(rows):
        lines.append('    "%s": {' % plate)
        for element in sorted(rows[plate]):
            lines.append('        "%s": %d,' % (element, rows[plate][element]))
        lines.append("    },")
    lines.append("}")
    write_lines(Z_MAP, lines)
    return sum(len(v) for v in rows.values())


def write_align(align):
    """가운데가 아닌 것만 적는다. 전부 적으면 무엇이 예외인지 안 보인다."""
    odd = {at: v for at, v in align.items()
           if isinstance(v, list) and len(v) == 2
           and (v[0] != "center" or v[1] != "middle")}
    lines = [ALIGN_HEAD.rstrip(NL)]
    for at in sorted(odd):
        element = at.partition("|")[2]
        lines.append('    "%s": ("%s", "%s"),' % (element, odd[at][0],
                                                  odd[at][1]))
    lines.append("}")
    write_lines(ALIGN_MAP, lines)
    return len(odd)


def write_tuning(table):
    lines = [HEAD.rstrip(NL)]
    count = 0
    for plate in sorted(table):
        rows = table[plate]
        if not isinstance(rows, dict):
            continue
        lines.append('    "%s": {' % plate)
        for element in sorted(rows):
            rect = rows[element]
            if not isinstance(rect, list) or len(rect) < 4:
                continue
            lines.append('        "%s": (%d, %d, %d, %d),'
                         % (element, rect[0], rect[1], rect[2], rect[3]))
            count += 1
        lines.append("    },")
    lines.append("}")
    write_lines(OUT, lines)
    return count


def main():
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8")
    if len(sys.argv) < 2:
        print("쓰기: python apply_zones.py <내려받은 json>")
        return
    source = sys.argv[1]
    if not os.path.exists(source):
        print("그런 파일이 없다:", source)
        return

    loaded = json.load(io.open(source, encoding="utf-8"))

    # 옛 파일은 자리표만 들어 있다. 새 파일은 자리·그림·붙임 셋이다.
    if "zones" in loaded:
        table = loaded["zones"]
        art = loaded.get("art") or {}
        align = loaded.get("align") or {}
    else:
        table, art, align = loaded, {}, {}

    print("자리 %d개 -> %s" % (write_tuning(table), OUT))
    print("구역 그림 %d장 -> %s" % (write_art(art), ART_MAP))
    print("갈아 끼운 판 %d장 -> %s"
          % (write_plate(loaded.get("plate") or {}), PLATE_MAP))
    print("층을 정한 구역 %d개 -> %s"
          % (write_zorder(loaded.get("z") or {}), Z_MAP))
    print("가운데가 아닌 글자 %d개 -> %s" % (write_align(align), ALIGN_MAP))
    print("이제 import_hud04.py · prepare_hud04.py · build_hud04.py 를 돌려라.")


main()
