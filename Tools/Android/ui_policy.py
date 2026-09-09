"""Android UI budgets: preserve chest animation frames and cap the map."""
TARGETS = {
    "/Game/SVN/OutSideAsset/AICreation/UI/RewardConcept03New/T_RCN_ChestTripleBurst_Atlas": 4096,
    "/Game/SVN/OutSideAsset/AICreation/UI/RunFlow/T_StageMap_Scroll_Flat": 2048,
}

# This is a 6x6 animation atlas, not one image. Retain its 682x455 source frames.
MINIMUMS = {"/Game/SVN/OutSideAsset/AICreation/UI/RewardConcept03New/T_RCN_ChestTripleBurst_Atlas": (4092, 2730)}


def android_downscale(width: int, height: int, maximum: int, previous: float) -> float:
    if width <= 0 or height <= 0 or maximum <= 0:
        raise ValueError("Texture dimensions and budget must be positive")
    # Preserve an existing stronger Android limit; never enlarge its cooked asset.
    return max(1.0, previous, max(width, height) / maximum)
