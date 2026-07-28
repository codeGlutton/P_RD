# -*- coding: utf-8 -*-
"""구역 조정 쪽에서 내려받은 값을 자리표에 넣는다.

## 왜 바로 못 넣나

hud04_slots.py 는 prepare_hud04.py 가 만든다. 손으로 고치면 다음에 만들 때
조용히 사라진다 -- 그 파일 머리에도 그렇게 적혀 있다.

그래서 고친 값은 따로 hud04_tuning.py 에 적고, prepare_hud04.py 가 시안을
잰 **뒤에** 그것을 덮어씌운다. 잰 것과 정한 것을 갈라 두는 것이다.

## 좌표계가 둘이다

쪽이 내려주는 값은 **판 안** 자리다(판 왼쪽 위가 원점). hud04_slots.py 의
DETAIL 은 **화면** 자리다. 여기서는 판 안 자리 그대로 적어 두고, 화면 자리로
옮기는 것은 prepare_hud04.py 가 한다 -- 판 자리가 바뀌어도 따라가야 하므로
미리 더해 두면 안 된다.

    python apply_zones.py "C:/Users/2009e/Downloads/slots_01.json"
"""
import io
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "hud04_tuning.py")

HEAD = '''# -*- coding: utf-8 -*-
"""구역 조정 쪽에서 손으로 맞춘 자리. apply_zones.py 가 만든다.

손으로 고쳐도 된다. 다만 쪽에서 다시 내려받아 넣으면 덮인다.

값은 **판 안** 자리다. 판 왼쪽 위가 원점이고 (x, y, w, h) 다.
화면 자리로 옮기는 것은 prepare_hud04.py 가 한다.
"""

#: 판 이름 -> 요소 -> (x, y, w, h). 시안을 잰 값을 이것으로 덮는다.
TUNING = {
'''


def main():
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8")
    if len(sys.argv) < 2:
        print("쓰기: python apply_zones.py <내려받은 json>")
        return
    source = sys.argv[1]
    if not os.path.exists(source):
        print("그런 파일이 없다:", source)
        return

    table = json.load(io.open(source, encoding="utf-8"))

    lines = [HEAD]
    count = 0
    for plate in sorted(table):
        rows = table[plate]
        if not isinstance(rows, dict):
            continue
        lines.append('    "%s": {\n' % plate)
        for element in sorted(rows):
            rect = rows[element]
            if not isinstance(rect, list) or len(rect) < 4:
                continue
            lines.append('        "%s": (%d, %d, %d, %d),\n'
                         % (element, rect[0], rect[1], rect[2], rect[3]))
            count += 1
        lines.append("    },\n")
    lines.append("}\n")

    io.open(OUT, "w", encoding="utf-8", newline="\n").write("".join(lines))
    print("판 %d장 · 구역 %d개 -> %s" % (len(table), count, OUT))
    print("이제 prepare_hud04.py 와 build_hud04.py 를 다시 돌려라.")


main()
