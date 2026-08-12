#pragma once

#include "UI/Shop/ShopUIWidgetBase.h"
#include "ShopFullGeneratedWidgetBase.generated.h"

/**
 * Presentation parent for the all-generated shop variant.
 *
 * Gameplay icons continue to come from the shop DTO. Every chrome texture
 * resolved by native code is redirected to the ShopFullGenerated manifest, so
 * the legacy WBP and its legacy resolver behaviour remain unchanged.
 */
UCLASS(Abstract)
class P_RD_API UShopFullGeneratedWidgetBase : public UShopUIWidgetBase
{
	GENERATED_BODY()

protected:
	virtual UTexture2D* ResolveItemIcon(const FShopItemUI& Item) const override;
	virtual UTexture2D* ResolveUnitSelectionPlate(bool bSelected) const override;
	virtual UTexture2D* ResolveSkillSelectionPlate(bool bSelected) const override;
	virtual UTexture2D* ResolveOwnedArtifactPlate() const override;
	virtual UTexture2D* ResolveTabPlate(bool bSelected) const override;
};

