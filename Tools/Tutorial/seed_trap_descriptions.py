"""One-time authoring of missing trap descriptions. Never overwrite team edits."""
import unreal
DESCRIPTIONS = {
    "DA_Barrel_ExplosionGimmick": "폭발 통\n이 칸에 들어서면 폭발해 주변에 피해를 줍니다. 아군도 피해를 받으니 이동 경로를 확인하세요.",
    "DA_BlackHoleTotemGimmick": "흡인 토템\n이 칸에 들어서면 주변 대상을 토템 쪽으로 끌어당깁니다. 아군과 적 모두 영향을 받습니다.",
    "DA_BoobyTrapGimmick": "부비트랩\n이 칸에 들어서면 주변에 피해를 줍니다. 한 번 발동하면 사라집니다.",
    "DA_FirePuddleGimmick": "화염 장판\n들어설 때 피해를 줍니다. 위에 서서 라운드를 마쳐도 다시 발동하니 벗어나세요.",
    "DA_HealTotemGimmick": "회복 토템\n이 칸에 들어서면 주변 대상의 체력을 회복합니다. 적도 회복할 수 있습니다.",
    "DA_PoisonPuddleGimmick": "독 장판\n들어설 때 피해와 취약을 부여합니다. 위에 서서 라운드를 마쳐도 다시 발동합니다.",
    "DA_PushPlateGimmick": "밀치기 발판\n이 칸에 들어선 대상에게 피해를 주고 밀어냅니다. 한 번 발동하면 사라집니다.",
    "DA_StunTrapGimmick": "기절 함정\n이 칸에 들어서면 주변 대상에게 기절을 부여합니다. 한 번 발동하면 사라집니다.",
}
for name, description in DESCRIPTIONS.items():
    asset = unreal.load_asset('/Game/BP/DataAsset/CombatTargetObstacle/' + name)
    if not isinstance(asset, unreal.StaticGimmickSpawnData):
        raise RuntimeError('Missing trap DA: ' + name)
    if not str(asset.get_editor_property('m_description')).strip():
        asset.set_editor_property('m_description', description)
        unreal.EditorAssetLibrary.save_loaded_asset(asset)
        unreal.log('Authored encounter description: ' + name)
