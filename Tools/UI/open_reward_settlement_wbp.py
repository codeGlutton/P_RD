import unreal


ASSET = "/Game/UI/RewardSettlement/WBP_RewardSettlement_Runtime"
asset = unreal.EditorAssetLibrary.load_asset(ASSET)
if asset is None:
    raise RuntimeError(f"Could not load reward settlement WBP: {ASSET}")

subsystem = unreal.get_editor_subsystem(unreal.AssetEditorSubsystem)
subsystem.open_editor_for_assets([asset])
unreal.log(f"RD_REWARD_SETTLEMENT_OPEN success asset={ASSET}")
