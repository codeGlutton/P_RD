#include "UI/DiceViewData.h"

ERarityType RDUIDice::ResolveDiceRarity(const FPrimaryAssetId& DiceId)
{
	// 런/전투 뷰모델이 없는 fallback 표시에서만 id 이름으로 희귀도를 추정한다.
	const FString DiceTypeName = DiceId.PrimaryAssetType.ToString();
	if (DiceTypeName.Contains(TEXT("Epic")))
	{
		return ERarityType::Epic;
	}
	if (DiceTypeName.Contains(TEXT("Rare")))
	{
		return ERarityType::Rare;
	}

	return ERarityType::Common;
}

FLinearColor RDUIDice::GetDiceRarityColor(ERarityType RarityType, EDiceRarityColorTone)
{
	const float Alpha = 0.92f;
	switch (RarityType)
	{
	case ERarityType::Rare:
		return FLinearColor(0.55f, 0.72f, 1.0f, Alpha);
	case ERarityType::Epic:
		return FLinearColor(0.82f, 0.58f, 1.0f, Alpha);
	case ERarityType::Common:
	default:
		return FLinearColor(0.86f, 0.98f, 0.94f, Alpha);
	}
}

FLinearColor RDUIDice::GetDiceRarityColor(const FDiceViewData& DiceView, EDiceRarityColorTone ColorTone)
{
	return DiceView.mRarityColor != FLinearColor::White
		? DiceView.mRarityColor
		: GetDiceRarityColor(DiceView.mRarityType, ColorTone);
}

FText RDUIDice::GetDiceRarityText(ERarityType RarityType)
{
	switch (RarityType)
	{
	case ERarityType::Rare:
		return NSLOCTEXT("RDUIDice", "DiceRarityRare", "Rare");
	case ERarityType::Epic:
		return NSLOCTEXT("RDUIDice", "DiceRarityEpic", "Epic");
	case ERarityType::Common:
	default:
		return NSLOCTEXT("RDUIDice", "DiceRarityCommon", "Common");
	}
}
