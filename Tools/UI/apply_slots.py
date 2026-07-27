# -*- coding: utf-8 -*-
"""쪽에서 내려받은 구역 값을 자리표에 넣는다.

## 왜 따로 넣나

브라우저는 파일을 제자리에 못 쓴다. 조정 쪽(아트목록.html)은 고친 값을
내려받기로만 내보낼 수 있고, 그것을 slot_table.py 에 넣는 일은 여기서 한다.

## 숫자만 갈아 끼운다

블록을 통째로 다시 써 봤더니 주석이 날아갔다 -- 왜 그 값인지 적어 둔
줄들이고(체력 막대 폭을 왜 줄였는지 같은 것), 그게 사라지면 다음 사람이 잰
값으로 되돌려 놓는다. 값만 바꾸고 나머지 줄은 손대지 않는다.

    python apply_slots.py                 내려받기 폴더에서 자동으로 찾음
    python apply_slots.py --file <경로>
"""
import argparse
import io
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
TABLE = os.path.join(HERE, "slot_table.py")
DOWNLOADS = os.path.join(os.path.expanduser("~"), "Downloads")

#: 역할 줄과 값 줄. 자리표의 들여쓰기(8칸, 12칸)를 그대로 따른다.
ROLE_LINE = re.compile(r'\s{8}"(\w+)": \{')
SLOT_LINE = re.compile(r'(\s{12}"(\w+)": )\(([^)]*)\)(,?)\s*$')


def newest_drop():
    """내려받기 폴더에서 제일 최근 slots_*.json."""
    if not os.path.isdir(DOWNLOADS):
        return None
    found = [os.path.join(DOWNLOADS, name) for name in os.listdir(DOWNLOADS)
             if re.match(r"slots_\d+.*\.json$", name)]
    return max(found, key=os.path.getmtime) if found else None


def number_text(value):
    """파이썬 튜플 표기. 정수는 정수로 둔다."""
    inner = ", ".join(("%g" % v) if isinstance(v, float) else str(v)
                      for v in value)
    return "(%s)" % inner


def graft(block, table):
    """블록 안의 숫자만 제자리에서 갈아 끼운다.

    돌려주는 값은 (바뀐 블록, 갈아 끼운 칸 수, 못 찾은 칸 목록).
    """
    want = {}
    for role, keys in table.items():
        for key, value in keys.items():
            if isinstance(value, (list, tuple)):
                want[(role, key)] = number_text(value)

    role, done, out = None, set(), []
    for line in block.split("\n"):
        head = ROLE_LINE.match(line)
        if head:
            role = head.group(1)
        spot = SLOT_LINE.match(line)
        if spot and role is not None and (role, spot.group(2)) in want:
            key = (role, spot.group(2))
            line = spot.group(1) + want[key] + (spot.group(4) or ",")
            done.add(key)
        out.append(line)

    missing = sorted("%s.%s" % k for k in want if k not in done)
    return "\n".join(out), len(done), missing


def main():
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8")
    ap = argparse.ArgumentParser()
    ap.add_argument("--file", default=None)
    ap.add_argument("--mockup", default="1")
    args = ap.parse_args()
    number = "%02d" % int(args.mockup)

    drop = args.file or newest_drop()
    if not drop or not os.path.exists(drop):
        raise SystemExit("구역 파일을 못 찾았습니다. 쪽에서 '파일로 저장'을 "
                         "누른 뒤 다시 돌려 주세요.")
    with io.open(drop, encoding="utf-8") as handle:
        table = json.load(handle)

    source = io.open(TABLE, encoding="utf-8").read()
    head = '    "%s": {' % number
    if head not in source:
        raise SystemExit("자리표에 시안%s 묶음이 없습니다." % number)
    start = source.index(head)
    # 이 시안 묶음의 끝. 같은 깊이의 닫는 줄을 찾는다.
    end = source.index("\n    },\n", start) + len("\n    },\n")

    before = source[start:end]
    after, changed, missing = graft(before, table)
    if before == after:
        print("바뀐 값이 없습니다.")
        return
    io.open(TABLE, "w", encoding="utf-8", newline="").write(
        source[:start] + after + source[end:])

    print("시안%s 구역 %d칸을 자리표에 넣었습니다." % (number, changed))
    if missing:
        print("  자리표에 없는 칸이라 못 넣음: %s" % ", ".join(missing))
    print("  받은 파일: %s" % drop)
    print("  자리표:   %s" % TABLE)
    print()
    print("이어서 구우면 반영됩니다:")
    print("  .\\Tools\\RunEditorPython.ps1 -Script "
          "$PWD\\Tools\\UI\\build_combat_layouts.py")


if __name__ == "__main__":
    main()
