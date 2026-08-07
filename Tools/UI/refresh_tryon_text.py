"""tryon.json 의 글자 설정을 **판의 지금 값**으로 다시 맞춘다.

왜 필요한가
-----------
글자 설정을 손대는 곳이 둘이다.

    tryon.json          사람이 갤러리에서 고른 것 -> apply_tryon.py
    text_fit.json       칸에 맞춰 잰 크기         -> center_all_text.py

두 번째가 크기를 24pt 로 맞춰 놨는데 tryon.json 에는 예전에 넣은 39pt 가
남아 있었다. 그대로 두면 apply_tryon 을 한 번만 더 돌려도 39 로 되돌아간다.
크기는 재서 정하는 쪽이 맞으니, tryon.json 쪽을 판의 지금 값으로 씻어 준다.

글꼴과 색은 사람이 고른 것이므로 **안 건드린다** -- 크기만 맞춘다.

    python Tools/UI/refresh_tryon_text.py
"""

import json
from pathlib import Path

MOCKUPS = Path("D:/UnrealProjects/P_RD_develop_20260803/Tools/UI/mockups")


def main():
    tryon_path = MOCKUPS / "tryon.json"
    picks = json.loads(tryon_path.read_text(encoding="utf-8"))
    styles = json.loads((MOCKUPS / "text_style.json").read_text(
        encoding="utf-8"))["styles"]

    # 판에서 정하는 것들. 여기 적힌 옛 값이 판을 덮으면 안 된다.
    #
    # 크기만 맞추다가 정렬을 빠뜨려, 판은 위쪽 정렬로 바꿨는데 tryon 에 남은
    # `vert: CENTER` 가 갤러리에서 그걸 되돌리고 있었다.
    VALIGN = {"V_ALIGN_TOP": "TOP", "V_ALIGN_CENTER": "CENTER",
              "V_ALIGN_BOTTOM": "BOTTOM", "V_ALIGN_FILL": "CENTER"}

    changed = []
    for key, choice in picks.items():
        if not isinstance(choice, dict):
            continue
        look = styles.get(key)
        if look is None:
            continue
        leaf = key.rsplit("/", 1)[-1]
        was = choice.get("size")
        now = round(float(look["size"]))
        if was != now:
            choice["size"] = now
            changed.append(f"  {leaf}  크기 {was} -> {now}pt")
        for field, value in (("just", str(look.get("just", "")).upper()),
                             ("vert", VALIGN.get(str(look.get("valign", "")).upper()))):
            if value and choice.get(field) != value:
                changed.append(f"  {leaf}  {field} {choice.get(field)} -> {value}")
                choice[field] = value

    if changed:
        (MOCKUPS / "tryon.prev.json").write_text(
            json.dumps(picks, ensure_ascii=False, indent=1), encoding="utf-8")
        tryon_path.write_text(json.dumps(picks, ensure_ascii=False, indent=1),
                              encoding="utf-8")
    print(f"글자 설정 {len(changed)}개를 판의 값으로 맞춤")
    for line in changed:
        print(line)


if __name__ == "__main__":
    main()
