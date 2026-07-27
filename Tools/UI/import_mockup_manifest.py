# -*- coding: utf-8 -*-
"""이름과 자리가 적힌 명세를 그대로 읽어 배치 재료로 삼는다.

## 왜 이 길이 다른가

여태 세 단계를 거쳤다. 조각을 시안 위에서 밀어 자리를 찾고, 그것이 무엇인지
눈으로 보고 표에 적고, 글자 있는 판과 빈 판을 짝지었다. 단계마다 틀릴 구석이
있었고 실제로 다 틀려 봤다 -- 빈 판이 안 비어 있었고, 카드 상태가 뒤섞였고,
같은 자리를 두 조각이 다퉜다.

명세에는 그 셋이 이미 들어 있다.

    이름   party_status_row_01  ->  아군 첫째 줄. 추측할 것이 없다
    자리   [16, 565, 450, 678]  ->  시안 1672x941 에서의 칸
    그림   빈 판 그대로.           마젠타를 깔고 뽑아 배경만 지운 것

그래서 매칭도 역할 판정도 짝짓기도 필요 없다. 읽어서 옮기면 끝이다.

## 내용 자리는 여전히 뺀다

이름과 막대와 얼굴을 판 안 어디에 놓을지는 명세에 없다. 채워진 시안에서
빈 판을 빼면 그 자리가 남으므로 지금까지 쓰던 방법을 그대로 쓴다. 다만
자리가 명세에서 오므로 잘라 내는 칸이 정확하다.

    python import_mockup_manifest.py --mockup 1 --folder D:/.../시안1/cropped_ui-v2
"""
import argparse
import io
import json
import os
import re
import shutil
import sys

from classify_cutouts import content_boxes

CHROME = r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitUIKit/Chrome"
DEST = r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitUIKit/Cutouts"
MOCKUPS = r"D:/UnrealProjects/P_RD_develop/Tools/UI/KayKitHUDMockups/Raw"

SCREEN_W, SCREEN_H = 1672.0, 941.0

#: 조각 이름 -> 역할. 이름이 곧 역할이라 표를 따로 둘 필요가 없다.
ROLE_OF = (
    ("top_left_panel", "round"),
    ("top_right_panel", "objective"),
    ("portrait_slot", "turn"),
    ("party_status_row", "party"),
    ("action_slot", "skill"),
    ("enemy_info_panel", "enemy"),
    ("end_turn_button", "endturn"),
)

WORD = {"round": "Round", "objective": "Objective", "turn": "Turn",
        "party": "Party", "skill": "Skill", "enemy": "Enemy",
        "endturn": "EndTurn"}


def role_of(name):
    for key, role in ROLE_OF:
        if key in name:
            return role
    return None


def anchor_of(x, y, w, h):
    """조각이 붙어 있을 모서리. 화면이 넓어지면 벌어지고 좁아지면 줄어든다."""
    cx, cy = (x + w / 2.0) / SCREEN_W, (y + h / 2.0) / SCREEN_H
    across = "l" if cx < 1 / 3.0 else ("r" if cx > 2 / 3.0 else "c")
    down = "t" if cy < 1 / 3.0 else ("b" if cy > 2 / 3.0 else "t")
    return down + across


def main():
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8")
    ap = argparse.ArgumentParser()
    ap.add_argument("--mockup", required=True)
    ap.add_argument("--folder", required=True)
    args = ap.parse_args()

    number = "%02d" % int(args.mockup)
    folder = args.folder
    with io.open(os.path.join(folder, "manifest.json"),
                 encoding="utf-8") as handle:
        spec = json.load(handle)

    mock_path = os.path.join(MOCKUPS, "KK_HUD_Polish_%s.png" % number)
    if not os.path.exists(mock_path):
        raise SystemExit("채워진 시안이 없다: %s" % mock_path)
    os.makedirs(DEST, exist_ok=True)

    # 이 시안이 전에 내보낸 것은 지운다. 이름이 역할·순번으로 정해지므로
    # 남아 있으면 다음 굽기에서 옛 그림이 붙는다.
    for stale in os.listdir(DEST):
        if stale.startswith("KK_Cut_%s_" % number):
            os.remove(os.path.join(DEST, stale))

    entries = sorted(spec["individual"], key=lambda e: e["file"])
    seen, rows, skipped = {}, [], []
    for entry in entries:
        stem = os.path.splitext(os.path.basename(entry["file"]))[0]
        role = role_of(stem)
        if role is None:
            skipped.append(stem)
            continue
        ordinal = seen.get(role, 0)
        seen[role] = ordinal + 1

        x0, y0, x1, y1 = entry["source_box"]
        w, h = entry["size"]
        asset = "KK_Cut_%s_%s%s" % (
            number, WORD[role],
            "" if role in ("round", "objective", "endturn") else
            "_%d" % ordinal)
        src = os.path.join(folder, entry["file"].replace("/", os.sep))
        shutil.copyfile(src, os.path.join(DEST, asset + ".png"))

        rows.append({
            "asset": asset, "role": role, "index": len(rows) + 1,
            "arts": [[asset, 0, 0, w, h]],
            "rect": [x0, y0, w, h],
            "anchor": anchor_of(x0, y0, w, h),
            "holes": [],
            # 채워진 시안에서 빈 판을 빼면 글자와 얼굴이 있던 자리가 남는다.
            "boxes": content_boxes(mock_path, src, crop=(x0, y0, x1, y1)),
            "contents": [],
        })

    # 읽는 순서로 다시 매긴다. 아군 첫째 줄이 PartyCard_0 이어야 한다.
    rows.sort(key=lambda r: (r["role"], r["rect"][1], r["rect"][0]))
    seen = {}
    for row in rows:
        ordinal = seen.get(row["role"], 0)
        seen[row["role"]] = ordinal + 1
        want = "KK_Cut_%s_%s%s" % (
            number, WORD[row["role"]],
            "" if row["role"] in ("round", "objective", "endturn") else
            "_%d" % ordinal)
        if want != row["asset"]:
            os.replace(os.path.join(DEST, row["asset"] + ".png"),
                       os.path.join(DEST, want + ".png"))
            row["asset"] = want
            row["arts"][0][0] = want
    rows.sort(key=lambda r: (r["rect"][1], r["rect"][0]))
    for index, row in enumerate(rows, 1):
        row["index"] = index

    path = os.path.join(CHROME, "cutout_manifest.json")
    with io.open(path, encoding="utf-8") as handle:
        manifest = json.load(handle)
    manifest[number] = rows
    with io.open(path, "w", encoding="utf-8") as handle:
        json.dump(manifest, handle, ensure_ascii=False, indent=1)

    for row in rows:
        print("  %-22s %-9s x=%4d y=%3d %3dx%-3d  내용 %2d칸" % (
            row["asset"], row["role"], row["rect"][0], row["rect"][1],
            row["rect"][2], row["rect"][3], len(row["boxes"])))
    if skipped:
        print("  이름을 못 읽어 건너뜀: %s" % ", ".join(skipped))
    print("시안%s: 조각 %d장을 명세에서 그대로 읽었다" % (number, len(rows)))


if __name__ == "__main__":
    main()
