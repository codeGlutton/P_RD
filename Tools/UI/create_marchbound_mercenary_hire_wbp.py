"""Create the separate Marchbound mercenary-hire WBP once."""

import unreal


SOURCE = "/Game/UI/CombatLayouts/WBP_MercenaryHire"
DESTINATION = "/Game/UI/CombatLayouts/WBP_MercenaryHire_Marchbound"

if unreal.EditorAssetLibrary.does_asset_exist(DESTINATION):
    unreal.log(f"RD_MB_HIRE_CREATE already_exists={DESTINATION}")
else:
    duplicated = unreal.EditorAssetLibrary.duplicate_asset(SOURCE, DESTINATION)
    if duplicated is None:
        raise RuntimeError(f"Could not duplicate {SOURCE} to {DESTINATION}")
    if not unreal.EditorAssetLibrary.save_asset(
        DESTINATION, only_if_is_dirty=False
    ):
        raise RuntimeError(f"Could not save {DESTINATION}")
    unreal.log(f"RD_MB_HIRE_CREATE success={DESTINATION}")
