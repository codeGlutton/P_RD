import unreal


ASSET = "/Game/UI/RewardSettlement/WBP_RewardSettlement_Runtime"
REQUIRED_WIDGETS = (
    "SettlementViewportRoot",
    "HeaderScale",
    "SummaryScale",
    "ExpScale",
    "mTitleText",
    "mGoldBalanceText",
    "mSummaryRowsBox",
    "mMercenaryRowsBox",
    "mNextButton",
    "mNextButtonText",
)


if unreal.EditorAssetLibrary.load_asset(ASSET) is None:
    raise RuntimeError(f"Missing WBP: {ASSET}")

generated_class = unreal.EditorAssetLibrary.load_blueprint_class(ASSET)
if generated_class is None:
    raise RuntimeError("Generated class did not load")

unreal.SystemLibrary.execute_console_command(None, "RD.Editor.VerifyRewardSettlement")
unreal.SystemLibrary.quit_editor()
