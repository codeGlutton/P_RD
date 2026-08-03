import unreal


PAIRS = (
    ("/Game/UI/LegacyReward/WBP_RewardRow_Legacy_20260803", "/Game/UI/WBP_RewardRow"),
    ("/Game/UI/LegacyReward/WBP_Reward_Legacy_20260803", "/Game/UI/WBP_Reward"),
)


for source, destination in PAIRS:
    if not unreal.EditorAssetLibrary.does_asset_exist(source):
        raise RuntimeError(f"Legacy reward backup is missing: {source}")

# The main widget references the row widget. Remove both destinations first so
# the row can be restored before the main widget is loaded again.
for _, destination in reversed(PAIRS):
    if unreal.EditorAssetLibrary.does_asset_exist(destination):
        if not unreal.EditorAssetLibrary.delete_asset(destination):
            raise RuntimeError(f"Could not remove replaced reward asset: {destination}")

for source, destination in PAIRS:
    if not unreal.EditorAssetLibrary.duplicate_asset(source, destination):
        raise RuntimeError(f"Could not restore {destination} from {source}")
    unreal.EditorAssetLibrary.save_asset(destination, only_if_is_dirty=False)

unreal.log("RD_REWARD_LEGACY_RESTORE success originals=2 backups_preserved=2")
unreal.SystemLibrary.quit_editor()
