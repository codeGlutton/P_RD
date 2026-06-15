#include "UI/DiceViewData.h"

#include "DataAsset/DiceData/StaticDiceData.h"
#include "Engine/AssetManager.h"

ERarityType RDUIDice::ResolveDiceRarity(const FPrimaryAssetId& DiceId)
{
	if (UAssetManager* AssetManager = UAssetManager::GetIfInitialized())
	{
		if (const UStaticDiceData* StaticDiceData = AssetManager->GetPrimaryAssetObject<UStaticDiceData>(DiceId))
		{
			return StaticDiceData->mRarityType;
		}
	}

	// 에셋이 아직 로드 전이면 색/문구를 마냥 기다릴 수 없으므로, 타입 이름으로 희귀도를 추정한다(폴백).
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

FLinearColor RDUIDice::GetDiceRarityColor(ERarityType RarityType, EDiceRarityColorTone ColorTone)
{
	const float Alpha = ColorTone == EDiceRarityColorTone::DicePanel ? 0.95f : 0.92f;
	switch (RarityType)
	{
	case ERarityType::Rare:
		return FLinearColor(0.55f, 0.72f, 1.0f, Alpha);
	case ERarityType::Epic:
		return FLinearColor(0.82f, 0.58f, 1.0f, Alpha);
	case ERarityType::Common:
	default:
		if (ColorTone == EDiceRarityColorTone::DicePanel)
		{
			return FLinearColor(0.84f, 1.0f, 0.94f, Alpha);
		}
		return FLinearColor(0.86f, 0.98f, 0.94f, Alpha);
	}
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
