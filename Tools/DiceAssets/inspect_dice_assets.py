import unreal


for path in (
    "/Game/BP/UI/Dice/RobotoMono-Bold",
    "/Game/BP/UI/Dice/T_DiceFace_Base",
    "/Game/BP/UI/Dice/M_DiceFace",
):
    asset_data = unreal.EditorAssetLibrary.find_asset_data(path)
    unreal.log(f"{path}: {asset_data.asset_class_path.asset_name}")
