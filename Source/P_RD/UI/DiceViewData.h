#pragma once

/** @brief 주사위 UI에 필요한 표시값을 모아 둔 파일입니다. */
// @file DiceViewData.h
// 전투 HUD와 주사위 패널이 같은 색과 문구를 쓰도록, 주사위 희귀도 관련 처리를 한곳에 모았습니다.
// 위젯 쪽에서는 이 함수들만 쓰면 됩니다.

#include "RDMinimal.h"
#include "DataAsset/RarityType.h"

class UTexture;

/** @brief 주사위 한 칸을 화면에 그릴 때 필요한 값입니다. */
struct P_RD_API FDiceViewData
{
	// 런 보유 주사위 식별자. 표시/추적용이며 UI가 직접 에셋을 새로 구성하지 않는다.
	FPrimaryAssetId mDiceId;
	ERarityType mRarityType = ERarityType::Common;
	// 0은 미굴림 sentinel로 쓰인다. 실제 굴림값은 1 이상이어야 한다.
	int32 mResultValue = 1;
	int32 mRolledFaceIndex = INDEX_NONE;   // 물리적으로 굴러진 면 index(0-base). 회전/정지 자세는 이 값을 쓴다.
	bool mIsRolled = false;
	bool mIsUsed = false;   // 이번 턴에 이미 쓴 주사위(다음 굴림까지 잠금)
	bool mIsSelected = false;   // 스킬 빌드에 올린(선택된) 주사위. 여러 개 동시 선택 가능(다중 강조).
	int32 mFaceCount = 6;   // 면 수(종류 표시용: 2=동전 … 20=d20)
	// 아래 두 배열은 UDiceModel/FDiceSlotUI와 같은 물리 면 0-base 순서다.
	TArray<int32> mFaceValues;   // 각 물리 면에 적힌 실제 값
	TArray<TObjectPtr<UTexture>> mFaceTextures;   // 각 물리 면에 덮을 선택 텍스처
};

namespace RDUIDice
{
	/** @brief 주사위 색을 어디에 그릴지 구분합니다. */
	// 전투 HUD와 주사위 패널은 배경이 달라서 같은 희귀도라도 색 밝기를 조금 다르게 씁니다.
	enum class EDiceRarityColorTone : uint8
	{
		CombatHUD,
		DicePanel
	};

	/** @brief 주사위 id로 희귀도를 찾습니다. */
	// 주사위 데이터가 이미 로드돼 있으면 그 값을 쓰고, 없으면 id 이름에 들어간 Rare/Epic으로 판단합니다.
	P_RD_API ERarityType ResolveDiceRarity(const FPrimaryAssetId& DiceId);

	/** @brief 희귀도에 맞는 색을 돌려줍니다. */
	P_RD_API FLinearColor GetDiceRarityColor(ERarityType RarityType, EDiceRarityColorTone ColorTone = EDiceRarityColorTone::CombatHUD);

	/** @brief 희귀도 표시 문구(Common/Rare/Epic)를 돌려줍니다. */
	P_RD_API FText GetDiceRarityText(ERarityType RarityType);
}
