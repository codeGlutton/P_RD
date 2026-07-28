# -*- coding: utf-8 -*-
"""시안4 전투 HUD 의 판과 자리표를 준비한다.

## 왜 손으로 안 적나

시안4 는 판을 뜯은 결과와 그 안의 자리를 전부 기계가 읽을 수 있는 형태로
받았다.

    cropped_ui-v2/manifest.json          판 열넷과 화면에서의 자리
    detail_coordinates/detail_coordinates.json   판마다의 글자/아이콘 자리

앞선 시안들은 색과 크기로 판정하는 기계를 만들었다가 시안마다 다르게 틀려서
버렸고, 그 뒤로는 눈으로 보고 적었다. 여기서는 적을 것이 없다 -- 이름이 이미
붙어 있다. 손으로 옮기면 옮기는 만큼 틀린다.

## 하는 일

    1. 투명 판 열넷을 아트 폴더로 옮긴다
    2. hud04_slots.py 를 만든다 (판 자리 + 판 안의 자리)

임포트는 언리얼 안에서 해야 하므로 import_hud04.py 가 따로 한다.

    python prepare_hud04.py
"""
import io
import json
import os
import shutil

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
SOURCE = r"D:/UnrealProjects/P_RD_develop/시안4"
PLATES = os.path.join(SOURCE, "클롭_투명")
MANIFEST = os.path.join(SOURCE, "cropped_ui-v2", "manifest.json")
DETAIL = os.path.join(SOURCE, "detail_coordinates", "detail_coordinates.json")

ART = os.path.join(HERE, "KayKitUIKit", "HUD04")
SLOTS = os.path.join(HERE, "hud04_slots.py")

#: 판 이름 -> 텍스처 이름. 번호 붙은 파일명을 그대로 쓰면 에셋 이름이
#: "01_top_left_parchment" 이 되어 코드에서 읽기 나쁘다.
def texture_name(stem):
    return "KK_HUD04_" + stem.split("_", 1)[1] if stem[:2].isdigit() else "KK_HUD04_" + stem


def collect_plates():
    """투명 판을 아트 폴더로 옮기고 이름을 돌려준다."""
    os.makedirs(ART, exist_ok=True)
    names = {}
    for entry in sorted(os.listdir(PLATES)):
        if not entry.lower().endswith(".png"):
            continue
        stem = os.path.splitext(entry)[0]
        target = texture_name(stem)
        shutil.copyfile(os.path.join(PLATES, entry),
                        os.path.join(ART, target + ".png"))
        # "01_top_left_parchment" -> "top_left_parchment"
        names[stem.split("_", 1)[1] if stem[:2].isdigit() else stem] = target
    return names


def write_slots(place, detail, textures):
    lines = [
        "# -*- coding: utf-8 -*-",
        '"""시안4 전투 HUD 의 자리표. prepare_hud04.py 가 만든다.',
        "",
        "손으로 고치지 마라. 시안이 바뀌면 다시 만든다 -- 손으로 고친 값은",
        "다음에 만들 때 조용히 사라진다.",
        "",
        "PLACE   판을 화면 어디에 놓나. 시안 1672 x 941 기준.",
        "DETAIL  판 안의 글자와 그림 자리. **화면 기준**이다.",
        "",
        "판 안 기준이 아니라 화면 기준으로 두는 이유: 판을 캔버스에 놓고 그 안에",
        "다시 얹으면 두 좌표계를 오가야 하고, 그러다 한 번 어긋나면 어느 쪽이",
        "틀렸는지 안 보인다. 전부 한 좌표계로 둔다.",
        '"""',
        "",
        "#: 판 이름 -> 텍스처 이름.",
        "TEXTURE = {",
    ]
    for key in sorted(textures):
        lines.append('    "%s": "%s",' % (key, textures[key]))
    lines += ["}", "", "#: 판을 화면 어디에 놓나. (x, y, w, h)", "PLACE = {"]
    for key in sorted(place):
        lines.append("    \"%s\": %s," % (key, tuple(place[key])))
    lines += ["}", "",
              "#: 판 안의 자리. 판 이름 -> {요소 이름: (x, y, w, h)}",
              "DETAIL = {"]
    for key in sorted(detail):
        lines.append('    "%s": {' % key)
        for name in detail[key]:
            lines.append("        \"%s\": %s," % (name, tuple(detail[key][name])))
        lines.append("    },")
    lines += ["}", ""]
    io.open(SLOTS, "w", encoding="utf-8", newline="\n").write("\n".join(lines))


textures = collect_plates()
detail_raw = json.load(io.open(DETAIL, encoding="utf-8"))
detail = {}
place = {}
for asset in detail_raw["assets"]:
    stem = os.path.splitext(asset["file"])[0]
    detail[stem] = {e["id"]: e["canvas_bbox"] for e in asset["elements"]}

    # 자리는 crop_origin 과 **실제 그림 크기**로 잡는다. 투명 판은 둘레에
    # 4픽셀 여유를 두고 잘려 있어서, manifest 의 source_box 와 size 를 쓰면
    # 판마다 4픽셀씩 어긋난다 -- 열넷이 다 같은 방향으로 밀린다.
    left, top = asset["crop_origin"]
    with Image.open(os.path.join(ART, textures[stem] + ".png")) as image:
        width, height = image.size
    place[stem] = (left, top, width, height)

missing = set(place) - set(textures)
if missing:
    raise RuntimeError("판은 있는데 그림이 없다: %s" % sorted(missing))

write_slots(place, detail, textures)
print("판 %d장, 자리 %d판 -> %s" % (len(textures), len(detail), SLOTS))
