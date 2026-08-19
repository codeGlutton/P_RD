#include "UI/Shop/ShopFullGeneratedWidgetBase.h"

#include "Blueprint/WidgetTree.h"
#include "Engine/Texture2D.h"

namespace
{
	UTexture2D* LoadFullGeneratedTexture(const TCHAR* ObjectPath)
	{
		return LoadObject<UTexture2D>(nullptr, ObjectPath);
	}
}

void UShopFullGeneratedWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();
	CacheResponsiveZones();
	mLastResponsiveViewportSize = FVector2D(-1.f, -1.f);
}

void UShopFullGeneratedWidgetBase::NativeTick(
	const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const FVector2D ViewportSize = MyGeometry.GetLocalSize();
	if (!ViewportSize.Equals(mLastResponsiveViewportSize, .5f))
	{
		UpdateResponsiveZoneLayout(ViewportSize);
		mLastResponsiveViewportSize = ViewportSize;
	}
}

void UShopFullGeneratedWidgetBase::CacheResponsiveZones()
{
	mResponsiveTopZone = WidgetTree != nullptr
		? WidgetTree->FindWidget(TEXT("TopZone")) : nullptr;
	mResponsiveCenterZone = WidgetTree != nullptr
		? WidgetTree->FindWidget(TEXT("CenterZone")) : nullptr;
	mResponsiveBottomZone = WidgetTree != nullptr
		? WidgetTree->FindWidget(TEXT("BottomZone")) : nullptr;
}

void UShopFullGeneratedWidgetBase::UpdateResponsiveZoneLayout(
	const FVector2D& ViewportSize)
{
	constexpr float DesignWidth = 1600.f;
	constexpr float DesignHeight = 1000.f;

	if (ViewportSize.X <= 0.f || ViewportSize.Y <= 0.f)
	{
		return;
	}

	// ShopMasterScale uses ScaleToFit. On tall screens this leaves equal vertical
	// gutters around the 1600x1000 design board. Counter-translate only the edge
	// zones in design-local units: header reaches the viewport top, actions reach
	// the viewport bottom, while CenterZone remains on the viewport center line.
	const float MasterScale = FMath::Min(
		ViewportSize.X / DesignWidth, ViewportSize.Y / DesignHeight);
	if (MasterScale <= UE_SMALL_NUMBER)
	{
		return;
	}

	const float ScaledDesignHeight = DesignHeight * MasterScale;
	const float VerticalGutter = FMath::Max(0.f,
		(ViewportSize.Y - ScaledDesignHeight) * .5f);
	const float LocalEdgeOffset = VerticalGutter / MasterScale;

	if (mResponsiveTopZone != nullptr)
	{
		mResponsiveTopZone->SetRenderTranslation(FVector2D(0.f, -LocalEdgeOffset));
	}
	if (mResponsiveCenterZone != nullptr)
	{
		mResponsiveCenterZone->SetRenderTranslation(FVector2D::ZeroVector);
	}
	if (mResponsiveBottomZone != nullptr)
	{
		mResponsiveBottomZone->SetRenderTranslation(FVector2D(0.f, LocalEdgeOffset));
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
	const bool) const
{
	return LoadFullGeneratedTexture(
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_Cell_Normal.T_KitA_Cell_Normal"));
}

UTexture2D* UShopFullGeneratedWidgetBase::ResolveSkillSelectionPlate(
	const bool bSelected) const
{
	return LoadFullGeneratedTexture(bSelected
		? TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/T_ShopFG_SkillSlot07_Square_Selected.T_ShopFG_SkillSlot07_Square_Selected")
		: TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/T_ShopFG_SkillSlot07_Square.T_ShopFG_SkillSlot07_Square"));
}

UTexture2D* UShopFullGeneratedWidgetBase::ResolveOwnedArtifactPlate() const
{
	return LoadFullGeneratedTexture(
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated/Chrome/T_ShopFG_RailCard_Normal.T_ShopFG_RailCard_Normal"));
}

UTexture2D* UShopFullGeneratedWidgetBase::ResolveTabPlate(
	const bool) const
{
	return LoadFullGeneratedTexture(
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_Button_Wide_Normal.T_KitA_Button_Wide_Normal"));
}
