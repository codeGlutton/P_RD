from __future__ import annotations

from pathlib import Path

import unreal


ROOT = Path(__file__).resolve().parents[2]
SOURCE_DIR = ROOT / "Content" / "Audio" / "SFX" / "ExternalActionCandidates"
DESTINATION_PATH = "/Game/Audio/SFX/ExternalActionCandidates"


def main() -> None:
    source_wavs = sorted(SOURCE_DIR.glob("S_*.wav"))
    if len(source_wavs) != 8:
        raise RuntimeError(f"Expected 8 external action WAVs, found {len(source_wavs)} in {SOURCE_DIR}")

    tasks = []
    for source_wav in source_wavs:
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
        asset_path = f"{DESTINATION_PATH}/{task.destination_name}"
        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        if not asset:
            missing_imports.append(asset_path)
            continue
        unreal.EditorAssetLibrary.save_loaded_asset(asset)

    if missing_imports:
        raise RuntimeError(f"Unreal did not import all requested audio assets: {missing_imports}")

    unreal.EditorAssetLibrary.save_directory(DESTINATION_PATH)
    unreal.log(f"Imported {len(tasks)} external action candidate SoundWaves to {DESTINATION_PATH}")


if __name__ == "__main__":
    main()
