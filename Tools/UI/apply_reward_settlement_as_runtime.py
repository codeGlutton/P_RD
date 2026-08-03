import unreal


RUNTIME_MAIN = "/Game/UI/WBP_Reward"
RUNTIME_ROW = "/Game/UI/WBP_RewardRow"
NEW_MAIN = "/Game/UI/RewardSettlement/WBP_RewardSettlement_New"
NEW_ROW = "/Game/UI/RewardSettlement/WBP_RewardSettlementRow_New"

BACKUP_DIR = "/Game/UI/LegacyReward"
BACKUP_MAIN = BACKUP_DIR + "/WBP_Reward_Legacy_20260803"
BACKUP_ROW = BACKUP_DIR + "/WBP_RewardRow_Legacy_20260803"


def require_asset(path):
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        raise RuntimeError("Required asset does not exist: " + path)


def duplicate_checked(source, destination):
    duplicated = unreal.EditorAssetLibrary.duplicate_asset(source, destination)
    if not duplicated or not unreal.EditorAssetLibrary.does_asset_exist(destination):
        raise RuntimeError("Failed to duplicate {} -> {}".format(source, destination))
    if not unreal.EditorAssetLibrary.save_asset(destination, False):
        raise RuntimeError("Failed to save duplicated asset: " + destination)


def delete_checked(path):
    if not unreal.EditorAssetLibrary.delete_asset(path):
        raise RuntimeError("Failed to delete asset: " + path)
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        raise RuntimeError("Asset still exists after deletion: " + path)


def restore(backup, destination):
    if unreal.EditorAssetLibrary.does_asset_exist(destination):
        unreal.EditorAssetLibrary.delete_asset(destination)
    duplicate_checked(backup, destination)


def replace(source, destination, backup):
    delete_checked(destination)
    try:
        duplicate_checked(source, destination)
    except Exception:
        unreal.log_error("Replacement failed; restoring " + destination)
        restore(backup, destination)
        raise


def main():
    for path in (RUNTIME_MAIN, RUNTIME_ROW, NEW_MAIN, NEW_ROW):
        require_asset(path)

    for path in (BACKUP_MAIN, BACKUP_ROW):
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            raise RuntimeError("Backup already exists; refusing to overwrite: " + path)

    if not unreal.EditorAssetLibrary.does_directory_exist(BACKUP_DIR):
        unreal.EditorAssetLibrary.make_directory(BACKUP_DIR)

    unreal.log("[RewardRuntimeSwap20260803] Backing up actual runtime assets")
    duplicate_checked(RUNTIME_MAIN, BACKUP_MAIN)
    duplicate_checked(RUNTIME_ROW, BACKUP_ROW)

    unreal.log("[RewardRuntimeSwap20260803] Replacing /Game/UI/WBP_RewardRow")
    replace(NEW_ROW, RUNTIME_ROW, BACKUP_ROW)

    try:
        unreal.log("[RewardRuntimeSwap20260803] Replacing /Game/UI/WBP_Reward")
        replace(NEW_MAIN, RUNTIME_MAIN, BACKUP_MAIN)
    except Exception:
        restore(BACKUP_ROW, RUNTIME_ROW)
        raise

    main_class = unreal.EditorAssetLibrary.load_blueprint_class(RUNTIME_MAIN)
    row_class = unreal.EditorAssetLibrary.load_blueprint_class(RUNTIME_ROW)
    if not main_class or not row_class:
        unreal.log_error("Class validation failed; restoring both old runtime assets")
        restore(BACKUP_MAIN, RUNTIME_MAIN)
        restore(BACKUP_ROW, RUNTIME_ROW)
        raise RuntimeError("Replacement assets do not have loadable Blueprint classes")

    unreal.EditorAssetLibrary.save_directory("/Game/UI", False, True)
    unreal.log("[RewardRuntimeSwap20260803] SUCCESS")
    unreal.log("[RewardRuntimeSwap20260803] Runtime main: " + RUNTIME_MAIN)
    unreal.log("[RewardRuntimeSwap20260803] Runtime row: " + RUNTIME_ROW)
    unreal.log("[RewardRuntimeSwap20260803] Backup: " + BACKUP_DIR)


try:
    main()
except Exception as exc:
    unreal.log_error("[RewardRuntimeSwap20260803] FAILED: " + str(exc))
    raise
finally:
    unreal.SystemLibrary.quit_editor()
