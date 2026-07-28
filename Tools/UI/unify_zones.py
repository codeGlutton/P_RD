# -*- coding: utf-8 -*-
"""묶인 판들의 자리를 본 하나로 한 번 맞춘다.

## 왜 따로 두나

prepare_hud04.py 는 이제 **덮지 않는다.** 빈 자리만 메운다 -- 구역 조정 쪽과
같은 일을 두 곳에서 하다가 열 때마다 값이 밀렸기 때문이다.

그래서 "여섯을 하나로 맞춰라" 는 이제 **한 번 하는 일**이 됐다. 그 한 번을
여기서 한다. 손으로 맞춘 값이 적힌 hud04_tuning.py 를 직접 고치므로, 맞춘
뒤에는 굽든 쪽을 열든 그대로 남는다.

    python unify_zones.py                 (본은 아래 기본값)
    python unify_zones.py card=action_top party=bottom_status_left
"""
import io
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
TUNING = os.path.join(HERE, "hud04_tuning.py")

CARDS = ("action_top", "action_left_upper", "action_right_upper",
         "action_left_lower", "action_right_lower", "action_bottom")
PARTY = ("bottom_status_left", "bottom_status_center", "bottom_status_right")

HEAD = '''# -*- coding: utf-8 -*-
"""구역 조정 쪽에서 손으로 맞춘 자리. apply_zones.py 가 만든다.

손으로 고쳐도 된다. 다만 쪽에서 다시 내려받아 넣으면 덮인다.

값은 **판 안** 자리다. 판 왼쪽 위가 원점이고 (x, y, w, h) 다.
화면 자리로 옮기는 것은 prepare_hud04.py 가 한다.
"""

#: 판 이름 -> 요소 -> (x, y, w, h). 시안을 잰 값을 이것으로 덮는다.
TUNING = {
'''


def load():
    scope = {}
    exec(compile(io.open(TUNING, encoding="utf-8").read(), TUNING, "exec"),
         scope)
    return scope["TUNING"]


def save(table):
    lines = [HEAD.rstrip("\n")]
    for plate in sorted(table):
        lines.append('    "%s": {' % plate)
        for element in sorted(table[plate]):
            x, y, w, h = table[plate][element]
            lines.append('        "%s": (%d, %d, %d, %d),'
                         % (element, x, y, w, h))
        lines.append("    },")
    lines.append("}")
    io.open(TUNING, "w", encoding="utf-8", newline="\n").write(
        "\n".join(lines) + "\n")


def flatten(table, plates, template):
    """본의 자리를 나머지에 그대로 옮긴다. 값은 판 안 자리라 그대로 쓴다."""
    if template not in table:
        print("본이 표에 없다: %s" % template)
        return 0
    base = dict(table[template])
    moved = 0
    for plate in plates:
        if plate == template:
            continue
        table[plate] = {k: tuple(v) for k, v in base.items()}
        moved += 1
    return moved


def main():
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8")
    picked = dict(card="action_top", party="bottom_status_left")
    for arg in sys.argv[1:]:
        key, _, value = arg.partition("=")
        if key in picked and value:
            picked[key] = value

    table = load()
    for name, plates, template in (("명령 카드", CARDS, picked["card"]),
                                   ("아군 칸", PARTY, picked["party"])):
        moved = flatten(table, plates, template)
        print("%s %d장을 %s 로 맞췄다 (요소 %d개)"
              % (name, moved, template, len(table.get(template, {}))))
    save(table)
    print("적었다: %s" % TUNING)
    print("이제 prepare_hud04.py 와 build_hud04.py 를 다시 돌려라.")


main()
