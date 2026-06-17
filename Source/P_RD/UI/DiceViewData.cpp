#include "UI/DiceViewData.h"

#include "DataAsset/DiceData/StaticDiceData.h"
#include "Engine/AssetManager.h"

/*
 * 주사위 희귀도 → 색/문구 변환을 한곳에 모은 헬퍼.
 * 전투 HUD와 주사위 패널이 "같은 희귀도 = 같은 색/문구"를 쓰도록 공유한다.
 * UI 위젯이 게임플레이 enum(ERarityType)을 직접 해석하지 않게, 여기서 표시값으로 번역해 준다.
 */

// 주사위 id로 희귀도를 판정한다.
ERarityType RDUIDice::ResolveDiceRarity(const FPrimaryAssetId& DiceId)
{
	// 1순위: 로드된 StaticDiceData의 실제 희귀도 값을 신뢰(정확).
	if (UAssetManager* AssetManager = UAssetManager::GetIfInitialized())
	{
		if (const UStaticDiceData* StaticDiceData = AssetManager->GetPrimaryAssetObject<UStaticDiceData>(DiceId))
		{
			return StaticDiceData->mRarityType;
		}
	}

	// 2순위(폴백): 에셋 프리로드 전 초기 UI 구성 시점엔 데이터가 없으므로,
	//   id 타입 이름의 "Epic"/"Rare" 글자로 임시 판정한다(둘 다 없으면 Common).
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

// 희귀도별 고정 팔레트(Rare=푸른빛, Epic=보랏빛, Common=옅은 민트)로 한눈에 등급 구분.
FLinearColor RDUIDice::GetDiceRarityColor(ERarityType RarityType, EDiceRarityColorTone ColorTone)
{
	// 톤 구분 이유: 같은 색이라도 HUD(어두운 배경)와 주사위 패널(밝은 배경)에서
	//   체감 가독성이 달라 알파를 살짝 다르게 준다.
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

// 종류 → 면 수. 굴림 범위(1..면수)와 표시에 쓰는 단일 진실값.
int32 RDUIDice::GetDiceFaceCount(EDiceType DiceType)
{
	switch (DiceType)
	{
	case EDiceType::Coin: return 2;
	case EDiceType::D4:   return 4;
	case EDiceType::D6:   return 6;
	case EDiceType::D8:   return 8;
	case EDiceType::D10:  return 10;
	case EDiceType::D12:  return 12;
	case EDiceType::D20:  return 20;
	default:              return 6;   // 알 수 없으면 기본 6면체
	}
}

// 면 수 → 종류. 런타임 UDiceData의 면 수(int)를 표시용 enum으로 역변환.
EDiceType RDUIDice::ResolveDiceTypeFromFaceCount(int32 FaceCount)
{
	switch (FaceCount)
	{
	case 2:  return EDiceType::Coin;
	case 4:  return EDiceType::D4;
	case 6:  return EDiceType::D6;
	case 8:  return EDiceType::D8;
	case 10: return EDiceType::D10;
	case 12: return EDiceType::D12;
	case 20: return EDiceType::D20;
	default: return EDiceType::D6;   // 규격 외 면 수는 기본 6면체로 표시
	}
}

// 종류 표시 문구. 동전은 "동전", 다면체는 통용 표기 "dN".
FText RDUIDice::GetDiceTypeText(EDiceType DiceType)
{
	switch (DiceType)
	{
	case EDiceType::Coin: return NSLOCTEXT("RDUIDice", "DiceTypeCoin", "동전");
	case EDiceType::D4:   return NSLOCTEXT("RDUIDice", "DiceTypeD4", "d4");
	case EDiceType::D6:   return NSLOCTEXT("RDUIDice", "DiceTypeD6", "d6");
	case EDiceType::D8:   return NSLOCTEXT("RDUIDice", "DiceTypeD8", "d8");
	case EDiceType::D10:  return NSLOCTEXT("RDUIDice", "DiceTypeD10", "d10");
	case EDiceType::D12:  return NSLOCTEXT("RDUIDice", "DiceTypeD12", "d12");
	case EDiceType::D20:  return NSLOCTEXT("RDUIDice", "DiceTypeD20", "d20");
	default:              return NSLOCTEXT("RDUIDice", "DiceTypeUnknown", "?");
	}
}
