import os
import unreal


PROJECT_DIR = unreal.Paths.project_dir()
DESTINATION = "/Game/UI/ResultBoards/GeneratedArt"

IMPORTS = {
    os.path.join("RewardParts_20260816", "Reward_Chest_Closed_v1.png"):
        "T_RS_Generated_ChestClosed",
    os.path.join("RewardParts_20260816", "Reward_Chest_HalfOpen_v1.png"):
        "T_RS_Generated_ChestHalfOpen",
    os.path.join("RewardParts_20260816", "Reward_Chest_Open_v1.png"):
        "T_RS_Generated_ChestOpen",
    os.path.join("RewardParts_20260816", "Reward_Chest_RevealBurst_v1.png"):
        "T_RS_Generated_ChestRevealBurst",
    os.path.join("RewardParts_20260816", "Reward_ArtifactCard_Selected_v1.png"):
        "T_RS_Generated_ArtifactCardSelected",
    os.path.join("RewardParts_20260816", "Reward_EXP_Row_Blank_v2.png"):
        "T_RS_Generated_EXPRowV2",
    os.path.join("RewardRebuild_20260816", "Reward_SettlementFrame_Blank_v3_20260816_1809.png"):
        "T_RS_Generated_SettlementFrameV3",
    os.path.join("RewardRebuild_20260816", "Reward_ChestRevealAura_v2_20260816_1809.png"):
        "T_RS_Generated_ChestRevealAuraV2",
    os.path.join("RewardModular_20260816", "Reward_OuterFrame_Modular_v1_20260816.png"):
        "T_RS_Modular_OuterFrameV1",
    os.path.join("RewardModular_20260816", "Reward_ContentPanel_Modular_v1_20260816.png"):
        "T_RS_Modular_ContentPanelV1",
    os.path.join("RewardModular_20260816", "Reward_EXPRowBase_Modular_v1_20260816.png"):
        "T_RS_Modular_EXPRowBaseV1",
    os.path.join("RewardModular_20260816", "Reward_ProgressFill_Modular_v1_20260816.png"):
        "T_RS_Modular_ProgressFillV1",
    os.path.join("RewardModular_20260816", "Reward_XPBadge_Modular_v1_20260816.png"):
        "T_RS_Modular_XPBadgeV1",
    os.path.join("RewardModular_20260816", "Reward_ChoiceCard_Normal_Modular_v1_20260816.png"):
        "T_RS_Modular_ChoiceCardNormalV1",
    os.path.join("RewardModular_20260816", "Reward_ChoiceCard_Selected_Modular_v1_20260816.png"):
        "T_RS_Modular_ChoiceCardSelectedV1",
    os.path.join("RewardWireframeV4_20260816", "reward-v4-outer-shell-frame-20260816.png"):
        "T_RS_WireframeV4_OuterShell",
    os.path.join("RewardWireframeV4_20260816", "reward-v4-header-plate-20260816.png"):
        "T_RS_WireframeV4_HeaderPlate",
    os.path.join("RewardWireframeV4_20260816", "reward-v4-content-panel-20260816.png"):
        "T_RS_WireframeV4_ContentPanel",
    os.path.join("RewardWireframeV4_20260816", "reward-v4-exp-row-20260816.png"):
        "T_RS_WireframeV4_EXPRow",
    os.path.join("RewardWireframeV4_20260816", "reward-v4-choice-card-20260816.png"):
        "T_RS_WireframeV4_ChoiceCard",
    os.path.join("RewardWireframeV4_20260816", "reward-v4-primary-button-cropped-20260816.png"):
        "T_RS_WireframeV4_PrimaryButton",
    os.path.join("RewardWireframeV4_20260816", "reward-v4-step-badge-cropped-20260816.png"):
        "T_RS_WireframeV4_StepBadge",
    os.path.join("RewardWireframeV4_20260816", "reward-v4-chest-closed-20260816.png"):
        "T_RS_WireframeV4_ChestClosed",
    os.path.join("RewardWireframeV4_20260816", "reward-v4-chest-open-20260816.png"):
        "T_RS_WireframeV4_ChestOpen",
    os.path.join("RewardWireframeV4_20260816", "reward-v4-chest-burst-20260816.png"):
        "T_RS_WireframeV4_ChestBurst",
}


tasks = []
for relative_path, destination_name in IMPORTS.items():
    source = os.path.join(PROJECT_DIR, "Saved", "DesignAssets", relative_path)
    if not os.path.isfile(source):
        raise RuntimeError("Missing generated reward art: " + source)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", source)
    task.set_editor_property("destination_path", DESTINATION)
    task.set_editor_property("destination_name", destination_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", False)
    tasks.append(task)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

for destination_name in IMPORTS.values():
    object_path = f"{DESTINATION}/{destination_name}.{destination_name}"
    texture = unreal.EditorAssetLibrary.load_asset(object_path)
    if texture is None:
        raise RuntimeError("Reward texture import failed: " + object_path)
    texture.set_editor_property(
        "compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON
    )
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property(
        "mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS
    )
    texture.set_editor_property("never_stream", True)
    texture.set_editor_property("srgb", True)
    texture.modify()
    unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)

unreal.log(f"RD_REWARD_GENERATED_IMPORT success count={len(IMPORTS)}")
unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
