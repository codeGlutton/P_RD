#pragma once

#include "CoreMinimal.h"
#include "UI/Combat/CombatUITypes.h"

/**
 * @brief 전투 상태 태그를 화면용 의미로 바꾸는 공용 변환표.
 *
 * 요약판과 플로팅 로그가 각자 이름/색/아이콘을 따로 해석하면 같은 상태가
 * 서로 다르게 보인다. 게임플레이는 태그만 넘기고, 화면 표현은 이 표 하나를
 * 사용한다.
 */
namespace CombatStatusUI
{
	struct P_RD_API FPresentation
	{
		FText mDisplayName;
		EFloatingLogIconType mFloatingIcon = EFloatingLogIconType::None;
		EFloatingLogColorType mColor = EFloatingLogColorType::Neutral;
		int32 mSortPriority = MAX_int32;
		bool mIsBuff = false;
		bool mIsDebuff = false;
		bool mIsRoundDuration = false;
		bool mIsInfinite = false;
	};

	/** @brief 상태 태그 하나의 이름/색/로그 아이콘/정렬 우선순위를 구한다. */
	P_RD_API FPresentation Resolve(const FGameplayTag& StatusTag);

	/** @brief "기절 +1"처럼 상태 변화량까지 포함한 플로팅 문구를 만든다. */
	P_RD_API FText FormatDelta(const FGameplayTag& StatusTag, int32 Delta);

	/** @brief 행동 불가 계열을 앞에 두고 나머지를 항상 같은 순서로 정렬한다. */
	P_RD_API void SortForDisplay(TArray<FStatusEffectUI>& Statuses);
}
