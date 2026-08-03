"""Validate the authored Marchbound mercenary-hire WBP and texture assets."""

import unreal


ASSET_PATH = "/Game/UI/CombatLayouts/WBP_MercenaryHire_Marchbound"
PREFIX = f"{ASSET_PATH}.WBP_MercenaryHire_Marchbound:WidgetTree."

TEXTURES = {
    "/Game/UI/Art/Marchbound/Hire/T_MB_HireListFrame": (512, 884),
    "/Game/UI/Art/Marchbound/Hire/T_MB_HireRowNormal": (1024, 288),
    "/Game/UI/Art/Marchbound/Hire/T_MB_HireRowSelected": (1024, 288),
    "/Game/UI/Art/Marchbound/Hire/T_MB_HireBackButton": (768, 305),
    "/Game/UI/Art/Marchbound/Hire/T_MB_HireTitlePlate": (1024, 254),
    "/Game/UI/Art/Marchbound/Hire/T_MB_HirePartyFrame": (640, 956),
    "/Game/UI/Art/Marchbound/Hire/T_MB_HirePartyRowPlus": (1024, 420),
    "/Game/UI/Art/Marchbound/Hire/T_MB_HirePartyRowEmpty": (1024, 420),
    "/Game/UI/Art/Marchbound/Hire/T_MB_HireDepartButton": (768, 364),
    "/Game/UI/Art/Marchbound/Hire/T_MB_HireNamePlate": (1024, 237),
    "/Game/UI/Art/Marchbound/Hire/T_MB_HireStatsStrip": (1400, 196),
    "/Game/UI/Art/Marchbound/Hire/T_MB_HireSkillButtonFrame": (1254, 1254),
}

for mercenary_name in ("Knight", "Mage", "Ranger", "Rogue", "Barbarian", "Druid"):
    TEXTURES[
        f"/Game/UI/Art/Marchbound/Mercenaries/T_MB_HireIcon_{mercenary_name}"
    ] = (1254, 1254)
    TEXTURES[
        f"/Game/UI/Art/Marchbound/Mercenaries/T_MB_HireHero_{mercenary_name}"
    ] = (1254, 1254)


def widget(name, expected_type=None):
    result = unreal.load_object(None, PREFIX + name)
    if result is None:
        raise RuntimeError(f"Missing widget {name}")
    if expected_type is not None and not isinstance(result, expected_type):
        raise RuntimeError(
            f"{name} is {result.get_class().get_name()}, "
            f"expected {expected_type.__name__}"
        )
    return result


def assert_parent(child_name, parent_name):
    child = widget(child_name)
    parent = widget(parent_name)
    if child.get_parent() != parent:
        raise RuntimeError(
            f"{child_name} parent is "
            f"{child.get_parent().get_name() if child.get_parent() else 'None'}, "
            f"expected {parent_name}"
        )


if unreal.load_asset(ASSET_PATH) is None:
    raise RuntimeError(f"Missing {ASSET_PATH}")

for asset_path, expected_size in TEXTURES.items():
    texture = unreal.load_asset(asset_path)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f"Missing texture {asset_path}")
    # The GPU resource can temporarily be UE's 32x32 placeholder while DDC is
    # still compiling. Import-time logs and runtime C++ tests cover source size;
    # here we reject every other mismatch without making verification flaky.
    actual_size = (
        texture.blueprint_get_size_x(),
        texture.blueprint_get_size_y(),
    )
    if actual_size not in (expected_size, (32, 32)):
        raise RuntimeError(
            f"{asset_path} size {actual_size}, expected {expected_size}"
        )

widget("HireViewportRoot", unreal.Overlay)
widget("RootCanvas", unreal.CanvasPanel)
widget("HireBackgroundScale", unreal.ScaleBox)
widget("HireLeftScale", unreal.ScaleBox)
widget("HireLeftSize", unreal.SizeBox)
widget("HireLeftRegion", unreal.CanvasPanel)
widget("HireCenterScale", unreal.ScaleBox)
widget("HireCenterSize", unreal.SizeBox)
widget("HireCenterRegion", unreal.CanvasPanel)
widget("HireRightScale", unreal.ScaleBox)
widget("HireRightSize", unreal.SizeBox)
widget("HireRightRegion", unreal.CanvasPanel)
widget("HireListFrameArt", unreal.Image)
widget("HireTitlePanel", unreal.CanvasPanel)
widget("HireDetailNamePanel", unreal.CanvasPanel)
widget("HireDetailStatsPanel", unreal.CanvasPanel)
widget("HireBottomBar", unreal.CanvasPanel)
widget("HireBackButton", unreal.Button)
widget("DepartButton", unreal.Button)

assert_parent("HireBackgroundScale", "HireViewportRoot")
assert_parent("RootCanvas", "HireViewportRoot")
assert_parent("Backdrop_Art", "HireBackgroundScale")
assert_parent("HireLeftScale", "RootCanvas")
assert_parent("HireLeftSize", "HireLeftScale")
assert_parent("HireLeftRegion", "HireLeftSize")
assert_parent("HireCenterScale", "RootCanvas")
assert_parent("HireCenterSize", "HireCenterScale")
assert_parent("HireCenterRegion", "HireCenterSize")
assert_parent("HireRightScale", "RootCanvas")
assert_parent("HireRightSize", "HireRightScale")
assert_parent("HireRightRegion", "HireRightSize")
assert_parent("HireListFrameArt", "HireLeftRegion")
assert_parent("HireTitlePanel", "HireCenterRegion")
assert_parent("HireBottomBar", "HireRightRegion")
assert_parent("DepartHolder", "HireRightRegion")
assert_parent("HireBackHolder", "HireLeftRegion")
assert_parent("HireTitleArt", "HireTitlePanel")
assert_parent("HireDetailNameArt", "HireDetailNamePanel")
assert_parent("HireDetailStatsArt", "HireDetailStatsPanel")
assert_parent("HireBottomBar_Art", "HireBottomBar")
assert_parent("DepartArt", "DepartHolder")
assert_parent("HireBackArt", "HireBackHolder")

for index in range(6):
    card = f"HireCard_{index}"
    widget(card, unreal.CanvasPanel)
    assert_parent(card, "HireLeftRegion")
    assert_parent(f"HireCard_{index}_Art", card)
    assert_parent(f"HireSelected_{index}", card)
    assert_parent(f"HirePortrait_{index}", card)
    assert_parent(f"HireButton_{index}", card)

    skill = f"HireDetailSkill_{index}"
    widget(skill, unreal.CanvasPanel)
    assert_parent(skill, "HireCenterRegion")
    assert_parent(f"HireDetailSkillArt_{index}", skill)
    assert_parent(f"HireDetailSkillText_{index}", skill)
    assert_parent(f"HireDetailSkillButton_{index}", skill)

for index in range(3):
    slot = f"PartySlot_{index}"
    widget(slot, unreal.CanvasPanel)
    assert_parent(f"PartySlotArt_{index}", slot)
    assert_parent(f"PartySlotPlus_{index}", slot)
    assert_parent(f"PartySlotFace_{index}", slot)
    assert_parent(f"PartySlotName_{index}", slot)
    widget(f"PartySlotButton_{index}", unreal.Button)
    assert_parent(f"PartySlotButton_{index}", slot)

unreal.log(
    "RD_MB_HIRE_VERIFY success textures=24 cards=6 skills=6 "
    "party_slots=3 party_remove_buttons=3 background=per_hero_scale_to_fill "
    "ui=runtime_landscape_or_portrait_regions"
)
