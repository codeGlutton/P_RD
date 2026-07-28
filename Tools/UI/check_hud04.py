# -*- coding: utf-8 -*-
"""구운 WBP 를 열어 카드들이 정말 같은지 본다.

말로 "같은 판을 쓴다" 고 하는 것과 구운 결과가 그런 것은 다른 이야기다.
굽는 코드를 읽어 짐작하지 말고 나온 것을 재어 본다.

    UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=check_hud04.py
"""
import io
import os
import sys

import unreal

REPORT = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                      "_hud04_check.txt")
LINES = []


def say(text):
    """로그로는 안 잡혀서 파일로 적는다. 커맨드릿이 unreal.log 를 삼킨다."""
    LINES.append(text)

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import combat_layout_kit as kit  # noqa: E402

ASSET = "/Game/UI/CombatLayouts/WBP_CombatHUD04"

CARDS = ["CommandCard_%d" % i for i in range(6)]
PARTY = ["PartyCard_%d" % i for i in range(3)]


def slot_rect(widget):
    slot = widget.get_editor_property("slot")
    layout = slot.get_editor_property("layout_data")
    offsets = layout.get_editor_property("offsets")
    return (round(offsets.get_editor_property("left"), 1),
            round(offsets.get_editor_property("top"), 1),
            round(offsets.get_editor_property("right"), 1),
            round(offsets.get_editor_property("bottom"), 1))


def brush_texture(widget):
    for field in ("brush", "background"):
        try:
            brush = widget.get_editor_property(field)
        except Exception:
            continue
        art = brush.get_editor_property("resource_object")
        return art.get_name() if art else "(없음)"
    return "(브러시 없음)"


def look(blueprint, name):
    return kit.helper.umg_find_widget(blueprint, name)


bp = unreal.EditorAssetLibrary.load_asset(ASSET)
if bp is None:
    raise RuntimeError("못 열었다: " + ASSET)

for group, plate_prefix, extra in (
        (CARDS, "CommandPlate", ("CommandIcon", "CommandName", "CommandCost")),
        (PARTY, "PartyPlate", ("PartyPortrait", "PartyHPIcon", "PartyName"))):
    say("")
    say("=== %s ===" % plate_prefix)
    for index, card in enumerate(group):
        root = look(bp, card)
        plate = look(bp, "%s_%d" % (plate_prefix, index))
        line = "%s 자리=%s" % (card, slot_rect(root) if root else "없음")
        if plate is not None:
            line += " 판=%s %s" % (brush_texture(plate), slot_rect(plate))
        say(line)
        for prefix in extra:
            piece = look(bp, "%s_%d" % (prefix, index))
            if piece is None:
                say("    %-16s 없음" % prefix)
                continue
            note = brush_texture(piece)
            say("    %-16s %-34s %s" % (prefix, note, slot_rect(piece)))

io.open(REPORT, "w", encoding="utf-8", newline="\n").write("\n".join(LINES))
