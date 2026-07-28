# -*- coding: utf-8 -*-
"""이동 스킬을 "AP 하나에 한 칸" 규칙에 맞춘다.

이동에 쿨타임 2턴이 걸려 있었다. 두 턴에 한 번만 움직인다는 뜻인데, 이동을
AP 로 사는 규칙과 같이 놓으면 AP 가 남아도 못 움직인다. 기획이 정한 것은
"AP 당 한 칸" 이므로 쿨은 없다.

    python (RunEditorPython) fix_move_skill.py
"""
import unreal

MOVE = "/Game/BP/DataAsset/Skill/Common/DA_Spell_NomalMove_Spell_Common"

skill = unreal.load_asset(MOVE)
if skill is None:
    raise RuntimeError("이동 스킬을 못 찾음: " + MOVE)

before = skill.get_editor_property("mCooldownDuration")
skill.set_editor_property("mCooldownDuration", 0)
skill.set_editor_property("mRequiredMovement", 1)
unreal.EditorAssetLibrary.save_loaded_asset(skill, False)

after = unreal.load_asset(MOVE)
unreal.log("[이동] 쿨 %s -> %s, AP %s" % (
    before, after.get_editor_property("mCooldownDuration"),
    after.get_editor_property("mRequiredMovement")))
