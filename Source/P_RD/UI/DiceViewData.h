#pragma once

#include "RDMinimal.h"
#include "DataAsset/RarityType.h"

struct P_RD_API FDiceViewData
{
	FPrimaryAssetId mDiceId;
	ERarityType mRarityType = ERarityType::Common;
	int32 mResultValue = 1;
	bool mIsRolled = false;
};

namespace RDUIDice
{
	enum class EDiceRarityColorTone : uint8
	{
		CombatHUD,
		DicePanel
	};

	P_RD_API ERarityType ResolveDiceRarity(const FPrimaryAssetId& DiceId);
	P_RD_API FLinearColor GetDiceRarityColor(ERarityType RarityType, EDiceRarityColorTone ColorTone = EDiceRarityColorTone::CombatHUD);
	P_RD_API FText GetDiceRarityText(ERarityType RarityType);
}
