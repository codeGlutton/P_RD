from __future__ import annotations

from pathlib import Path

import unreal


ROOT = Path(__file__).resolve().parents[2]
SOURCE_DIR = ROOT / "Content" / "SVN" / "OutSideAsset" / "SFX" / "DevelopmentCandidates"
SOURCE_WAVS = [
    SOURCE_DIR / "S_CoinGain_Rattle_01.wav",
    *[SOURCE_DIR / f"S_CoinGain_Rattle_Alt_{index:02d}.wav" for index in range(1, 11)],
]
DESTINATION_PATH = "/Game/SVN/OutSideAsset/SFX/DevelopmentCandidates"


def main() -> None:
    missing = [path for path in SOURCE_WAVS if not path.exists()]
    if missing:
        raise RuntimeError(f"Missing source WAVs: {missing}")

    tasks = []
    for source_wav in SOURCE_WAVS:
        task = unreal.AssetImportTask()
        task.filename = str(source_wav)
        task.destination_path = DESTINATION_PATH
        task.destination_name = source_wav.stem
        task.automated = True
        task.replace_existing = True
        task.save = True
        tasks.append(task)

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    asset_tools.import_asset_tasks(tasks)

    missing_imports = []
    for task in tasks:
        imported_paths = list(task.imported_object_paths)
        if not imported_paths:
            missing_imports.append(task.destination_name)

        asset_path = f"{DESTINATION_PATH}/{task.destination_name}"
        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        if not asset:
            missing_imports.append(asset_path)
            continue

        unreal.EditorAssetLibrary.save_loaded_asset(asset)

    if missing_imports:
        raise RuntimeError(f"Unreal did not import all requested audio assets: {missing_imports}")

    unreal.EditorAssetLibrary.save_directory(DESTINATION_PATH)
    unreal.log(f"Imported {len(tasks)} coin gain rattle SoundWaves to {DESTINATION_PATH}")


if __name__ == "__main__":
    main()
