import unreal


COMMAND = "RD.Editor.BuildRewardSettlement"
ASSET = "/Game/UI/RewardSettlement/WBP_RewardSettlement_Runtime"


unreal.log("RD_REWARD_SETTLEMENT_SCRIPT begin")
if unreal.EditorAssetLibrary.does_asset_exist(ASSET):
    unreal.EditorAssetLibrary.delete_asset(ASSET)
unreal.SystemLibrary.execute_console_command(None, COMMAND)
unreal.EditorAssetLibrary.save_directory("/Game/UI/RewardSettlement", only_if_is_dirty=False, recursive=True)

if not unreal.EditorAssetLibrary.does_asset_exist(ASSET):
    raise RuntimeError(f"Reward settlement WBP was not created: {ASSET}")

asset = unreal.EditorAssetLibrary.load_asset(ASSET)
unreal.log(f"RD_REWARD_SETTLEMENT_SCRIPT success asset={asset.get_path_name()}")
unreal.SystemLibrary.quit_editor()
