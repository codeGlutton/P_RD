#pragma once

/**
 * @file DiceViewData.h
 * @brief 주사위 UI에 필요한 표시값을 모아 둔 파일입니다.
 *
 * 전투 HUD와 주사위 패널이 같은 색·문구·종류 표시를 쓰도록, 주사위 희귀도/종류 관련 처리를 한곳에 모았습니다.
 * 위젯 쪽에서는 이 함수들만 쓰면 됩니다.
 */

#include "RDMinimal.h"
#include "DataAsset/RarityType.h"

/**
 * @brief 주사위 종류(면 수). 동전은 2면, 나머지는 다면체.
 *
 * @details
 * 면 수가 곧 종류 — Coin=2, D4=4 … D20=20. 굴림 결과는 1..면수.
 * 런타임 주사위(UDiceData)는 면 수를 int(가변)로 들고 있어, 화면에 그릴 때 이 enum으로 바꿔 라벨/모양을 정한다.
 * (면별 특수 효과가 생기면 이 enum이 아니라 데이터 쪽에 면 데이터를 더하는 방향이 맞다.)
 */
enum class EDiceType : uint8
{
	Coin,   // 2면(동전)
	D4,     // 4면체
	D6,     // 6면체(기본)
	D8,     // 8면체
	D10,    // 10면체
	D12,    // 12면체
	D20,    // 20면체
};

/** @brief 주사위 한 칸을 화면에 그릴 때 필요한 값입니다. */
struct P_RD_API FDiceViewData
{
	FPrimaryAssetId mDiceId;
	ERarityType mRarityType = ERarityType::Common;
	EDiceType mDiceType = EDiceType::D6;   // 주사위 종류(면 수). 굴림 결과 범위·표시 라벨을 정함.
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

	/** @brief 주사위 종류의 면 수를 돌려줍니다(Coin=2, D4=4 … D20=20). */
	P_RD_API int32 GetDiceFaceCount(EDiceType DiceType);

	/**
	 * @brief 면 수로 주사위 종류를 판정합니다.
	 *
	 * 런타임 UDiceData가 들고 있는 면 수(int)를 표시용 enum으로 바꿀 때 씁니다.
	 * 아는 면 수(2/4/6/8/10/12/20)면 그 종류, 아니면 기본 D6으로 둡니다.
	 */
	P_RD_API EDiceType ResolveDiceTypeFromFaceCount(int32 FaceCount);

	/** @brief 종류 표시 문구("동전"/"d4"/"d6"…/"d20")를 돌려줍니다. */
	P_RD_API FText GetDiceTypeText(EDiceType DiceType);
}
