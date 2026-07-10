import json
import unreal

WPATH = "/Game/BP/UI/WBP_Reward"
ROW_WPATH = "/Game/BP/UI/WBP_RewardRow"
REQUIRED_WIDGETS = [
    "RewardRootCanvas",
    "RewardAspectScaleBox",
    "RewardDesignSizeBox",
    "RewardDesignCanvas",
    "RewardElem_02_widget_Canvas",
    "RewardElem_02_widget_00_widget",
    "RewardRowsScrollBox",
    "mRewardRowsBox",
    "RewardScrollbarTrack",
    "RewardScrollbarThumb",
    "mCloseButton",
    "mCloseButtonCanvas",
    "mCloseButtonFrame",
    "mCloseButtonText",
]

REQUIRED_ROW_WIDGETS = [
    "RewardRowRootSizeBox",
    "RewardRowCanvas",
    "mRowIconFrame",
    "mRewardIcon",
    "mRewardSingleText",
    "mRewardMainText",
    "mRewardSubText",
]

REQUIRED_TEXTURES = [
    "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/RewardV4_11/Tex/T_reward_v4_bg",
    "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/RewardV4_11/Tex/T_reward_v4_panel_frame",
    "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/RewardV4_11/Tex/T_reward_v4_scrollbar_track",
    "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/RewardV4_11/Tex/T_reward_v4_scrollbar_thumb",
    "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/RewardV4_11/Tex/T_reward_v4_row_icon_frame",
    "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/RewardV4_11/Tex/T_reward_v4_gold_icon",
    "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/RewardV4_11/Tex/T_reward_v4_btn_frame_normal",
]


def find(bp, name):
    try:
        return unreal.EditorUtilityLibrary.find_source_widget_by_name(bp, name)
    except Exception:
        return None


bp = unreal.EditorAssetLibrary.load_asset(WPATH)
result = {
    "asset": WPATH,
    "loaded": bp is not None,
    "missing": [],
    "found": [],
    "parent_class": None,
    "generated_class": None,
    "missing_textures": [],
    "found_textures": [],
}

if bp is not None:
    try:
        parent = bp.get_editor_property("parent_class")
        if parent is not None:
            result["parent_class"] = parent.get_path_name()
    except Exception:
        pass

    try:
        generated_class = bp.generated_class()
    except Exception:
        generated_class = None

    if generated_class is not None:
        result["generated_class"] = generated_class.get_path_name()
        if result["parent_class"] is None:
            for getter_name in ["get_super_struct", "get_super_class"]:
                try:
                    super_obj = getattr(generated_class, getter_name)()
                    if super_obj is not None:
                        result["parent_class"] = super_obj.get_path_name()
                        break
                except Exception:
                    pass
        try:
            cdo = unreal.get_default_object(generated_class)
            result["cdo_class"] = cdo.get_class().get_path_name() if cdo is not None else None
            result["is_reward_base"] = isinstance(cdo, unreal.RewardUIWidgetBase)
        except Exception as exc:
            result["cdo_error"] = str(exc)[:160]

    for name in REQUIRED_WIDGETS:
        widget = find(bp, name)
        if widget is None:
            result["missing"].append(name)
        else:
            result["found"].append({"name": name, "class": widget.get_class().get_name()})

for path in REQUIRED_TEXTURES:
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        result["found_textures"].append(path)
    else:
        result["missing_textures"].append(path)

print("REWARD_WBP_VERIFY|" + json.dumps(result, ensure_ascii=False))

row_bp = unreal.EditorAssetLibrary.load_asset(ROW_WPATH)
row_result = {
    "asset": ROW_WPATH,
    "loaded": row_bp is not None,
    "missing": [],
    "found": [],
    "parent_class": None,
    "generated_class": None,
}

if row_bp is not None:
    try:
        row_parent = row_bp.get_editor_property("parent_class")
        if row_parent is not None:
            row_result["parent_class"] = row_parent.get_path_name()
    except Exception:
        pass

    try:
        row_generated_class = row_bp.generated_class()
    except Exception:
        row_generated_class = None

    if row_generated_class is not None:
        row_result["generated_class"] = row_generated_class.get_path_name()
        if row_result["parent_class"] is None:
            for getter_name in ["get_super_struct", "get_super_class"]:
                try:
                    super_obj = getattr(row_generated_class, getter_name)()
                    if super_obj is not None:
                        row_result["parent_class"] = super_obj.get_path_name()
                        break
                except Exception:
                    pass
        try:
            row_cdo = unreal.get_default_object(row_generated_class)
            row_result["cdo_class"] = row_cdo.get_class().get_path_name() if row_cdo is not None else None
            row_result["is_reward_row_base"] = isinstance(row_cdo, unreal.RewardRowWidgetBase)
        except Exception as exc:
            row_result["cdo_error"] = str(exc)[:160]

    for name in REQUIRED_ROW_WIDGETS:
        widget = find(row_bp, name)
        if widget is None:
            row_result["missing"].append(name)
        else:
            row_result["found"].append({"name": name, "class": widget.get_class().get_name()})

print("REWARD_ROW_WBP_VERIFY|" + json.dumps(row_result, ensure_ascii=False))
