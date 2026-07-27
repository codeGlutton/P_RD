# -*- coding: utf-8 -*-
"""처음 고르는 여섯 명을 만든다.

## 왜 여섯을 새로 만드나

게시판은 여섯 칸인데 플레이어 유닛 데이터가 하나뿐이었다. 나머지 다섯 칸은
빈 구멍이 되고, 코드에 박힌 잠김 자리표시자 둘까지 합쳐도 셋이라 세 명을
고를 수가 없었다.

## 2 / 2 / 2

직업 열거형은 Knight / Archer / Mage 셋뿐이다. 근접 둘, 원거리 둘, 마법 둘을
맞추려면 직업당 둘씩 두는 수밖에 없다. 열거형을 늘리는 것은 스킬 클래스와
모션 태그가 걸린 자리라 화면 쪽에서 손댈 데가 아니다.

## 무엇이 진짜고 무엇이 임시인가

진짜   직업, 스킬 (직업별 스킬 데이터가 이미 있다), 이름, 설명, 초상
임시   생김새. 플레이어 폰이 기사 하나뿐이라 여섯이 전부 기사로 나온다.
       스탯. 원본을 복제한 것이라 여섯이 같다.

굽는 것이 아니라 복제다. 유닛 데이터에는 어트리뷰트 초기화와 뷰 클래스처럼
파이썬으로 다시 짜기 어려운 것이 얽혀 있어, 도는 것을 복제해 다른 데만
바꾼다.

    python (RunEditorPython) make_starting_crew.py
"""
import unreal

SOURCE = "/Game/BP/DataAsset/Unit/PlayerUnit/DA_TestPlayerUnit"
FOLDER = "/Game/BP/DataAsset/Unit/PlayerUnit"
FRONTEND = "/Game/BP/DataAsset/Room/Frontend/DA_TestFrontend"
HEADS = "/Game/SVN/OutSideAsset/UI/KayKit/Heads"
SKILL = "/Game/BP/DataAsset/Skill"

JOB = {
    "Knight": unreal.PlayerJobType.KNIGHT,
    "Archer": unreal.PlayerJobType.ARCHER,
    "Mage": unreal.PlayerJobType.MAGE,
}

#: 직업별 스킬. 카드에 두 줄이 뜨므로 앞 둘이 곧 화면에 나오는 것이다.
#: 이동은 공용이라 어느 직업이든 뒤에 붙인다.
SKILLS = {
    "Knight": ("Knight/DA_Attack_SwordNormalSlash_Attack_Common",
               "Knight/DA_Attack_SwordNormalSmash_Attack_Common",
               "Knight/DA_Attack_SwordBlade_Attack_Rare"),
    "Archer": ("Archer/DA_TestArcher_Spell_Common",
               "Archer/DA_TestArcher_Spell_Rare",
               "Archer/DA_TestArcher_Spell_Epic"),
    "Mage": ("Mage/DA_TestMage_Spell_Common",
             "Mage/DA_TestMage_Spell_Rare",
             "Mage/DA_TestMage_Spell_Epic"),
}
COMMON = ("Common/DA_Spell_NomalMove_Spell_Common",
          "Common/DA_Spell_NomalDefense_Spell_Common")

#: 이름이 비어 있는 스킬에 이름을 넣는다.
#:
#: 기사 스킬만 이름이 적혀 있고 궁수와 마법사 것은 비어 있었다. 카드에 스킬
#: 두 줄이 뜨는데 넉 장이 빈 줄로 나온다. 표시용 칸이라 채워도 규칙은 안
#: 건드린다. 이미 적힌 것은 손대지 않는다 -- 남이 정한 이름이다.
SKILL_NAMES = {
    "Archer/DA_TestArcher_Spell_Common": "사격",
    "Archer/DA_TestArcher_Spell_Rare": "관통 사격",
    "Archer/DA_TestArcher_Spell_Epic": "난사",
    "Mage/DA_TestMage_Spell_Common": "화염구",
    "Mage/DA_TestMage_Spell_Rare": "빙결",
    "Mage/DA_TestMage_Spell_Epic": "유성",
    "Common/DA_Spell_NomalMove_Spell_Common": "이동",
    "Common/DA_Spell_NomalDefense_Spell_Common": "방어",
}

#: 여섯 명. 이름, 직업, 초상, 왜 데려가는지 한 줄.
#:
#: 첫 칸은 원본을 그대로 쓴다. 이미 돌고 있는 것을 지우고 새로 만들 이유가
#: 없고, 다른 데서 이 자산을 가리키고 있을 수도 있다.
CREW = (
    ("DA_TestPlayerUnit", "기사", "Knight", "KK_Face_Knight_HeadV2",
     "앞줄을 막고 아군 대신 맞는다"),
    ("DA_PlayerUnit_Barbarian", "야만전사", "Knight",
     "KK_Face_BarbarianLarge_HeadV2", "체력이 높고 크게 때린다"),
    ("DA_PlayerUnit_Ranger", "궁수", "Archer", "KK_Face_Ranger_HeadV2",
     "멀리서 때린다. 맞으면 약하다"),
    ("DA_PlayerUnit_Rogue", "도적", "Archer", "KK_Face_RogueHooded_HeadV2",
     "먼저 움직이고 뒤를 노린다"),
    ("DA_PlayerUnit_Mage", "마법사", "Mage", "KK_Face_Mage_HeadV2",
     "한 번에 여럿을 친다"),
    ("DA_PlayerUnit_Cleric", "성직자", "Mage", "KK_Face_Druid_HeadV2",
     "아군을 회복시킨다"),
)


def load(path):
    found = unreal.load_asset(path)
    if found is None:
        raise RuntimeError("자산을 못 읽음: " + path)
    return found


def ensure(asset_name):
    """없으면 원본에서 복제해 만든다. 있으면 그것을 쓴다."""
    path = "{}/{}".format(FOLDER, asset_name)
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return load(path)
    made = unreal.EditorAssetLibrary.duplicate_asset(SOURCE, path)
    if made is None:
        raise RuntimeError("복제 실패: " + path)
    return made


def dress(asset, display, job, face, trait):
    asset.set_editor_property("mJobType", JOB[job])
    asset.set_editor_property("mDisplayName", unreal.Text(display))
    asset.set_editor_property("mDescription", unreal.Text(trait))
    asset.set_editor_property(
        "mPortrait", load("{}/{}".format(HEADS, face)))

    skills = []
    for tail in SKILLS[job] + COMMON:
        skills.append(load("{}/{}".format(SKILL, tail)))
    asset.set_editor_property("mSkillDatas", skills)

    unreal.EditorAssetLibrary.save_loaded_asset(asset, False)


def name_skills():
    """빈 이름만 채운다. 이미 적힌 것은 그대로 둔다."""
    for tail, label in SKILL_NAMES.items():
        skill = load("{}/{}".format(SKILL, tail))
        if not skill.get_editor_property("mName").is_empty():
            continue
        skill.set_editor_property("mName", unreal.Text(label))
        unreal.EditorAssetLibrary.save_loaded_asset(skill, False)
        unreal.log("[크루] 스킬 이름 {} <- {}".format(tail, label))


name_skills()

made = []
for asset_name, display, job, face, trait in CREW:
    asset = ensure(asset_name)
    dress(asset, display, job, face, trait)
    made.append(asset)
    unreal.log("[크루] {} <- {} ({})".format(asset_name, display, job))

# 게시판에 여섯을 건다. 순서가 곧 카드 순서다.
frontend = load(FRONTEND)
frontend.set_editor_property("mPlayableUnits", made)
unreal.EditorAssetLibrary.save_loaded_asset(frontend, False)
unreal.log("[크루] 게시판에 {}명 걸음".format(len(made)))
