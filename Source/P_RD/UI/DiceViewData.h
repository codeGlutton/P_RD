#pragma once

/**
 * @file DiceViewData.h
 * @brief 주사위 하나를 UI에 그릴 때 필요한 표시용 데이터와 희귀도 색/문구 헬퍼입니다.
 *
 * 전투 HUD와 주사위 패널이 같은 주사위를 각자 다르게 그리지 않도록, 희귀도 판별·색·문구를
 * 한곳에 모았습니다. 위젯은 게임 데이터(StaticDiceData)를 직접 들추지 않고 이 헬퍼만 부르면 됩니다.
 */

#include "RDMinimal.h"
#include "DataAsset/RarityType.h"

/** @brief 주사위 한 칸을 그리는 데 필요한 최소 표시 상태(런타임 갱신용 뷰 데이터). */
struct P_RD_API FDiceViewData
{
	FPrimaryAssetId mDiceId;
	ERarityType mRarityType = ERarityType::Common;
	int32 mResultValue = 1;
	bool mIsRolled = false;
};

namespace RDUIDice
{
	/**
	 * @brief 같은 희귀도라도 패널 배경이 달라 색을 약간 다르게 줘야 하는 두 곳을 구분합니다.
	 *
	 * 어두운 전투 HUD와 밝은 주사위 패널에서 같은 색을 쓰면 한쪽이 묻히므로, 톤만 살짝 나눕니다.
	 */
	enum class EDiceRarityColorTone : uint8
	{
		CombatHUD,
		DicePanel
	};

	/**
	 * @brief 주사위 에셋 id로 희귀도를 알아냅니다.
	 *
	 * StaticDiceData가 로드돼 있으면 그 값을 쓰고, 아직 없으면 에셋 타입 이름(Epic/Rare)으로 추정합니다.
	 * 로딩 타이밍과 무관하게 색/문구를 바로 정할 수 있게 한 폴백입니다.
	 */
	P_RD_API ERarityType ResolveDiceRarity(const FPrimaryAssetId& DiceId);

	/** @brief 희귀도에 맞는 색을 돌려줍니다. 그리는 곳(전투 HUD/주사위 패널)에 따라 톤만 다릅니다. */
	P_RD_API FLinearColor GetDiceRarityColor(ERarityType RarityType, EDiceRarityColorTone ColorTone = EDiceRarityColorTone::CombatHUD);

	/** @brief 희귀도 표시 문구(Common/Rare/Epic)를 돌려줍니다. */
	P_RD_API FText GetDiceRarityText(ERarityType RarityType);
}
