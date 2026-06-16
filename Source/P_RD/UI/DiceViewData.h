#pragma once

/**
 * @file DiceViewData.h
 * @brief 주사위 UI에 필요한 표시값을 모아 둔 파일입니다.
 *
 * 전투 HUD와 주사위 패널이 같은 색과 문구를 쓰도록, 주사위 희귀도 관련 처리를 한곳에 모았습니다.
 * 위젯 쪽에서는 이 함수들만 쓰면 됩니다.
 */

#include "RDMinimal.h"
#include "DataAsset/RarityType.h"

/** @brief 주사위 한 칸을 화면에 그릴 때 필요한 값입니다. */
struct P_RD_API FDiceViewData
{
	FPrimaryAssetId mDiceId;
	ERarityType mRarityType = ERarityType::Common;
	int32 mResultValue = 1;
	bool mIsRolled = false;
	bool mIsUsed = false;   // 이번 턴에 이미 쓴 주사위(다음 굴림까지 잠금)
};

namespace RDUIDice
{
	/**
	 * @brief 주사위 색을 어디에 그릴지 구분합니다.
	 *
	 * 전투 HUD와 주사위 패널은 배경이 달라서 같은 희귀도라도 색 밝기를 조금 다르게 씁니다.
	 */
	enum class EDiceRarityColorTone : uint8
	{
		CombatHUD,
		DicePanel
	};

	/**
	 * @brief 주사위 id로 희귀도를 찾습니다.
	 *
	 * 주사위 데이터가 이미 로드돼 있으면 그 값을 쓰고, 없으면 id 이름에 들어간 Rare/Epic으로 판단합니다.
	 */
	P_RD_API ERarityType ResolveDiceRarity(const FPrimaryAssetId& DiceId);

	/** @brief 희귀도에 맞는 색을 돌려줍니다. */
	P_RD_API FLinearColor GetDiceRarityColor(ERarityType RarityType, EDiceRarityColorTone ColorTone = EDiceRarityColorTone::CombatHUD);

	/** @brief 희귀도 표시 문구(Common/Rare/Epic)를 돌려줍니다. */
	P_RD_API FText GetDiceRarityText(ERarityType RarityType);
}
