# -*- coding: utf-8 -*-
"""용병 고용 화면을 WBP 로 굽는다.

## 어떻게 만드나

시안에서 뜯은 판을 제자리에 놓고, 그 안 잰 자리에 글자와 그림을 얹는다.
판은 늘리지 않는다 -- 시안에서 오려 낸 것이라 늘리면 몰딩이 뭉갠다.

이력서 여섯 장은 판도 자리도 같으므로 한 갈래로 짓는다. 손으로 여섯 번
적으면 여섯 번 틀린다.

## 이름 규칙

C++ 이 이름으로 위젯을 찾는다. 지금은 붙일 C++ 이 없지만, 나중에 붙일 때
고치지 않아도 되게 처음부터 규칙대로 짓는다.

    HireCard_0 ~ 5        이력서 뿌리 (버튼)
    HireName_0 ~ 5        이름
    HirePortrait_0 ~ 5    초상
    HireRole_0 ~ 5        역할
    HireHP_0 ~ 5          체력
    HireSkill_0_0 ~ 5_1   스킬 두 줄
    HireTrait_0 ~ 5       특성 한 줄
    HireSeal_0 ~ 5        고용 도장
    HireBadge_0 ~ 5       상태 배지
    PartySlot_0 ~ 2       파티 슬롯
    GoldText / SpentText / NoticeText / DepartButton

    python (RunEditorPython) build_hire_screen.py
"""
import os
import sys

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import combat_layout_kit as kit  # noqa: E402
from hire_slots import BAR, CARD, PLACE  # noqa: E402

ASSET = "WBP_MercenaryHire"
HIRE = "/Game/SVN/OutSideAsset/UI/KayKit/Hire"
HEADS = "/Game/SVN/OutSideAsset/UI/KayKit/Heads"

#: 시안 좌표 -> 설계 캔버스. 전투 HUD 와 같은 배율을 쓴다.
K = kit.CHROME_SCALE

#: 시안에 그려진 여섯 명. 실제 값은 나중에 데이터에서 온다.
#:
#: 값을 치르는 개념은 없다. 처음 시작할 때는 그냥 셋을 고른다 -- 돈으로 사는
#: 것은 나중에 상점에서 들어온다. 그 자리는 특성 한 줄이 대신한다.
CREW = (
    ("기사", "근접", "HP 100", "방패 강타", "반격 태세",
     "앞줄을 막고 아군 대신 맞는다", "KK_Face_Knight_HeadV2"),
    ("궁수", "원거리", "HP 80", "관통 사격", "조준",
     "멀리서 때린다. 맞으면 약하다", "KK_Face_Ranger_HeadV2"),
    ("마법사", "마법", "HP 70", "화염구", "빙결",
     "한 번에 여럿을 친다", "KK_Face_Mage_HeadV2"),
    ("도적", "근접", "HP 75", "기습", "은신",
     "먼저 움직이고 뒤를 노린다", "KK_Face_RogueHooded_HeadV2"),
    ("성직자", "지원", "HP 85", "치유", "축복",
     "아군을 회복시킨다", "KK_Face_Druid_HeadV2"),
    ("야만전사", "근접", "HP 120", "대검 휘두르기", "분노",
     "체력이 높고 크게 때린다", "KK_Face_BarbarianLarge_HeadV2"),
)

TEXT_DARK = unreal.LinearColor(0.16, 0.11, 0.07, 1.0)
TEXT_PALE = unreal.LinearColor(0.98, 0.95, 0.88, 1.0)


def at(spot):
    """시안 픽셀로 적힌 자리를 설계 캔버스로."""
    return [v * K for v in spot]


def plate(blueprint, root, name, texture, rect, z=kit.Z_FILL):
    """판 한 장을 제자리에. 늘리지 않는다."""
    x, y, w, h = at(rect)
    kit.add(blueprint, "CanvasPanel", name, root)
    kit.place(blueprint, name, x, y, w, h, "tl", None, z)
    kit.image(blueprint, name + "_Art", name, 0, 0, w, h, (w, h),
              z_order=z, texture="{}/{}".format(HIRE, texture),
              tint=kit.WHITE)
    return name, (w, h)


def card(blueprint, root, index):
    """이력서 한 장. 여섯 장이 판도 자리도 같아 한 갈래로 짓는다."""
    name, role, hp, skill_a, skill_b, trait, face = CREW[index]
    holder, size = plate(blueprint, root, "HireCard_%d" % index,
                         "KK_Hire_card_%d" % index,
                         PLACE["card_%d" % index], kit.Z_CONTENT)
    w, h = size

    px, py, pw, ph = at(CARD["portrait"])
    kit.image(blueprint, "HirePortrait_%d" % index, holder, px, py, pw, ph,
              size, texture="{}/{}".format(HEADS, face), tint=kit.WHITE)

    for key, widget, text, points, colour, align in (
            ("name", "HireName", name, 21, TEXT_DARK, "center"),
            ("role", "HireRole", role, 15, TEXT_PALE, "center"),
            ("hp", "HireHP", hp, 19, TEXT_DARK, "center"),
            ("skill_1", "HireSkill_%d_0" % index, skill_a, 15, TEXT_DARK,
             "center"),
            ("skill_2", "HireSkill_%d_1" % index, skill_b, 15, TEXT_DARK,
             "center"),
            ("trait", "HireTrait", trait, 15, TEXT_DARK, "center")):
        x, y, bw, bh = at(CARD[key])
        full = widget if "_%d_" % index in widget else "%s_%d" % (widget, index)
        kit.label(blueprint, full, holder, x, y, bw, bh, text, points,
                  colour, align, size, bold=(key in ("name", "hp")))

    # 상태 표시는 판에 없다. 낱장으로 받아 카드 위에 얹고, 접어 두고
    # 런타임이 켠다 -- 판에 그려 넣으면 그 카드만 영영 그 상태로 남는다.
    kit.image(blueprint, "HireSelected_%d" % index, holder, 0, 0, w, h, size,
              z_order=kit.Z_MARKER,
              texture="{}/KK_Hire_state_selected_frame".format(HIRE),
              tint=kit.WHITE)
    kit.fold(blueprint, "HireSelected_%d" % index)

    # 도장과 배지는 상태에 따라 하나만 켜진다.
    bx, by, bw, bh = at(CARD["badge"])
    kit.label(blueprint, "HireBadge_%d" % index, holder, bx, by, bw, bh,
              "모집 중", 14, TEXT_PALE, "center", size)
    kit.fold(blueprint, "HireBadge_%d" % index)

    sx, sy, sw, sh = at(CARD["seal"])
    kit.image(blueprint, "HireSeal_%d" % index, holder, sx, sy, sw, sh, size,
              z_order=kit.Z_MARKER,
              texture="{}/KK_Hire_state_seal".format(HIRE), tint=kit.WHITE)
    kit.fold(blueprint, "HireSeal_%d" % index)

    kit.ghost_button(blueprint, "HireButton_%d" % index, holder, 0, 0, w, h,
                     size)


def bottom_bar(blueprint, root):
    holder, size = plate(blueprint, root, "HireBottomBar",
                         "KK_Hire_bottombar", PLACE["bottombar"],
                         kit.Z_CONTENT)
    x, y, w, h = at(BAR["party_count"])
    kit.label(blueprint, "PartyCountText", holder, x, y, w, h, "파티\n0/3",
              15, TEXT_PALE, "center", size)

    for slot in range(3):
        x, y, w, h = at(BAR["slot_%d" % slot])
        kit.add(blueprint, "CanvasPanel", "PartySlot_%d" % slot, holder)
        kit.place(blueprint, "PartySlot_%d" % slot, x, y, w, h, "tl", size,
                  kit.Z_CONTENT)
        cell = (w, h)
        # 얼굴은 고용해야 생긴다. 그림 없이 펼쳐 두면 흰 네모가 뜬다.
        kit.image(blueprint, "PartySlotFace_%d" % slot, "PartySlot_%d" % slot,
                  0, 0, w, h * 0.78, cell, tint=kit.WHITE)
        kit.fold(blueprint, "PartySlotFace_%d" % slot)
        kit.label(blueprint, "PartySlotName_%d" % slot,
                  "PartySlot_%d" % slot, 0, h * 0.78, w, h * 0.22,
                  "빈 자리", 13, TEXT_PALE, "center", cell)

    # 금액 칸이 빠지면서 안내 칸이 넓어졌다. 두 줄이 들어갈 높이라 글자를
    # 가운데에 앉힌다.
    x, y, w, h = at(BAR["notice"])
    kit.label(blueprint, "NoticeText", holder, x, y + h * 0.30, w, h * 0.40,
              "용병 3명을 고르세요.", 19, TEXT_DARK, "center", size)

    x, y, w, h = at(BAR["depart"])
    kit.add(blueprint, "CanvasPanel", "DepartHolder", holder)
    kit.place(blueprint, "DepartHolder", x, y, w, h, "tl", size, kit.Z_CONTENT)
    kit.ghost_button(blueprint, "DepartButton", "DepartHolder", 0, 0, w, h,
                     (w, h))
    kit.label(blueprint, "DepartLabel", "DepartHolder", 0, h * 0.28, w,
              h * 0.44, "출발", 22, TEXT_PALE, "center", (w, h), bold=True)


def compose(blueprint, root):
    plate(blueprint, root, "Backdrop", "KK_Hire_backdrop", PLACE["backdrop"],
          kit.Z_SHADOW)
    plate(blueprint, root, "Board", "KK_Hire_board", PLACE["board"])
    for index in range(6):
        card(blueprint, root, index)
    bottom_bar(blueprint, root)


kit.reset_ledger()
# 부모를 C++ 클래스로 둔다. 굽고 나서 에디터에서 손으로 바꾸면 다시 구울
# 때마다 도로 풀린다 -- 굽기가 에셋을 새로 만들기 때문이다.
bp = kit.create_asset(ASSET, parent="/Script/P_RD.MercenaryHireWidget")
kit.add(bp, "CanvasPanel", "RootCanvas", "")
compose(bp, "RootCanvas")
unreal.EditorAssetLibrary.save_loaded_asset(bp, False)
kit.commit_asset(ASSET)
unreal.log("[Hire] {} 구움".format(ASSET))
