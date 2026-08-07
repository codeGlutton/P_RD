"""빈 전투요약 판 PNG 를 T_MB_Defeat_BattleSummary 로 임포트한다.

지워졌던 이름 그대로 되살린다 -- 카탈로그와 편집기 픽이 그 이름을 알고 있다.
UI 텍스처 설정(UserInterface2D, 밉 없음)까지 맞춘다.
"""

from pathlib import Path

import unreal

SOURCE = ("D:/UnrealProjects/P_RD_develop_20260803/SourceArt/UI/Marchbound/"
          "DefeatParts/MARCHBOUND_Defeat_Part_BattleSummary_Empty_20260803_v01.png")
DEST_DIR = "/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Defeat"
NAME = "T_MB_Defeat_BattleSummary"
RESULT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/import_plate.txt")

task = unreal.AssetImportTask()
task.set_editor_property("filename", SOURCE)
task.set_editor_property("destination_path", DEST_DIR)
task.set_editor_property("destination_name", NAME)
task.set_editor_property("automated", True)
task.set_editor_property("save", True)
task.set_editor_property("replace_existing", True)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

texture = unreal.EditorAssetLibrary.load_asset(f"{DEST_DIR}/{NAME}.{NAME}")
lines = [f"임포트: {texture}"]
if texture is not None:
    texture.set_editor_property(
        "compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property("mip_gen_settings",
                                unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    unreal.EditorAssetLibrary.save_loaded_asset(texture, False)
    lines.append("UI 텍스처 설정 + 저장 완료")
RESULT.parent.mkdir(parents=True, exist_ok=True)
RESULT.write_text("\n".join(lines) + "\n", encoding="utf-8")
