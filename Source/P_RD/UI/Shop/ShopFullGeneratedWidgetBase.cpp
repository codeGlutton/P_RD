#include "UI/Shop/ShopFullGeneratedWidgetBase.h"

#include "Engine/Texture2D.h"

namespace
{
	UTexture2D* LoadFullGeneratedTexture(const TCHAR* ObjectPath)
	{
		return LoadObject<UTexture2D>(nullptr, ObjectPath);
	}
}

UTexture2D* UShopFullGeneratedWidgetBase::ResolveItemIcon(
	const FShopItemUI& Item) const
{
	// Product/skill icons are dynamic content, not generated chrome. Do not fall
	// back to a baked legacy icon when a DTO intentionally has no icon.
	return Item.mIcon;
}

UTexture2D* UShopFullGeneratedWidgetBase::ResolveUnitSelectionPlate(
	const bool bSelected) const
{
	return LoadFullGeneratedTexture(bSelected
		? TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Chrome/T_ShopFG_UnitSlot_Selected.T_ShopFG_UnitSlot_Selected")
		: TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Chrome/T_ShopFG_UnitSlot_Normal.T_ShopFG_UnitSlot_Normal"));
}

UTexture2D* UShopFullGeneratedWidgetBase::ResolveSkillSelectionPlate(
	const bool bSelected) const
{
	return LoadFullGeneratedTexture(bSelected
		? TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Chrome/T_ShopFG_SkillSlot_Selected.T_ShopFG_SkillSlot_Selected")
		: TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Chrome/T_ShopFG_SkillSlot_Normal.T_ShopFG_SkillSlot_Normal"));
}

UTexture2D* UShopFullGeneratedWidgetBase::ResolveOwnedArtifactPlate() const
{
	return LoadFullGeneratedTexture(
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Chrome/T_ShopFG_RailCard_Normal.T_ShopFG_RailCard_Normal"));
}

UTexture2D* UShopFullGeneratedWidgetBase::ResolveTabPlate(
	const bool bSelected) const
{
	return LoadFullGeneratedTexture(bSelected
		? TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Chrome/T_ShopFG_Tab_Selected.T_ShopFG_Tab_Selected")
		: TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Chrome/T_ShopFG_Tab_Normal.T_ShopFG_Tab_Normal"));
}
