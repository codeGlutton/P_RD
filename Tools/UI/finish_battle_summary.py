"""임포트된 T_MB_Defeat_BattleSummary 의 UI 텍스처 설정을 마무리한다."""
from pathlib import Path

import unreal

PATH = ("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Defeat/"
        "T_MB_Defeat_BattleSummary.T_MB_Defeat_BattleSummary")
RESULT = Path("D:/UnrealProjects/P_RD_develop_20260803/Saved/LegacyAudit/import_plate.txt")

texture = unreal.EditorAssetLibrary.load_asset(PATH)
lines = [f"로드: {texture}"]
if texture is not None:
    lines.append(f"크기: {texture.blueprint_get_size_x()}x{texture.blueprint_get_size_y()}")
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property("mip_gen_settings",
                                unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property("srgb", True)
    saved = unreal.EditorAssetLibrary.save_loaded_asset(texture, False)
    lines.append(f"UI 그룹·밉 없음 설정, 저장={saved}")
RESULT.parent.mkdir(parents=True, exist_ok=True)
RESULT.write_text("\n".join(lines) + "\n", encoding="utf-8")
