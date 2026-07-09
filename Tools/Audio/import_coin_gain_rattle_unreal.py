from __future__ import annotations

from pathlib import Path

import unreal


ROOT = Path(__file__).resolve().parents[2]
SOURCE_WAV = ROOT / "Content" / "Audio" / "SFX" / "S_CoinGain_Rattle_01.wav"
DESTINATION_PATH = "/Game/Audio/SFX"
DESTINATION_NAME = "S_CoinGain_Rattle_01"


def main() -> None:
    if not SOURCE_WAV.exists():
        raise RuntimeError(f"Missing source WAV: {SOURCE_WAV}")

    task = unreal.AssetImportTask()
    task.filename = str(SOURCE_WAV)
    task.destination_path = DESTINATION_PATH
    task.destination_name = DESTINATION_NAME
    task.automated = True
    task.replace_existing = True
    task.save = True

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    asset_tools.import_asset_tasks([task])

    imported_paths = list(task.imported_object_paths)
    if not imported_paths:
        raise RuntimeError("Unreal did not report an imported asset path.")

    asset_path = f"{DESTINATION_PATH}/{DESTINATION_NAME}"
    asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not asset:
        raise RuntimeError(f"Could not load imported asset: {asset_path}")

    unreal.EditorAssetLibrary.save_loaded_asset(asset)
    unreal.EditorAssetLibrary.save_directory(DESTINATION_PATH)
    unreal.log(f"Imported coin gain rattle SoundWave: {asset_path}")


if __name__ == "__main__":
    main()
