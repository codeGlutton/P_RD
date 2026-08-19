"""Build the independently generated RewardConcept03 WBP after editor startup."""

import unreal


def main() -> None:
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.wait_for_completion()
    world = unreal.EditorLevelLibrary.get_editor_world()

    unreal.log("RD_REWARD_CONCEPT03_PYTHON build command")
    unreal.SystemLibrary.execute_console_command(
        world, "RD.Editor.BuildRewardConcept03New"
    )
    unreal.SystemLibrary.execute_console_command(
        world, "RD.Editor.VerifyRewardConcept03New"
    )
    unreal.EditorAssetLibrary.save_asset(
        "/Game/UI/RewardConcept03New/WBP_RewardConcept03_New",
        only_if_is_dirty=False,
    )
    unreal.EditorAssetLibrary.save_asset(
        "/Game/UI/RewardConcept03New/WBP_RewardConcept03_Frameless",
        only_if_is_dirty=False,
    )
    unreal.EditorAssetLibrary.save_asset(
        "/Game/UI/RewardConcept03New/WBP_RewardConcept03_Frameless_NoArtifact",
        only_if_is_dirty=False,
    )
    unreal.log("RD_REWARD_CONCEPT03_PYTHON complete")
    unreal.SystemLibrary.execute_console_command(world, "QUIT")


main()
