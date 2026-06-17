#pragma once

/** @brief 전투 보상 화면이 게임플레이에서 받는 '뷰 데이터' 정의입니다. */
// @file RewardViewTypes.h
// 전투 뷰모델과 같은 원칙: UI는 게임플레이 객체를 직접 알지 않고 표시값만 받아 그린다.
// 지금은 보상 종류 중 '돈(골드)'과 '경험치'만 담는다. 아이템/주사위 보상은 이후 같은 패턴으로 확장한다.
// 카운트업/레벨업 막대 연출을 UI가 스스로 할 수 있게 '전/후' 값을 함께 준다(게임플레이는 결과만 알려줌).

#include "RDMinimal.h"

#include "RewardViewTypes.generated.h"

/** @brief 전투 결과 보상 한 건의 표시값(돈·경험치). */
// - 골드: 이번에 번 양(mGoldGained)과 합산 후 잔액(mGoldBalance). UI는 잔액으로 카운트업.
// - 경험치: 이번에 번 양(mExpGained)과, 막대 채움을 위한 레벨 내 전/후 값(mExpBefore→mExpAfter)
// + 해당 레벨 최대치(mMaxExp). 레벨업이 있으면 mLevelBefore != mLevelAfter.
USTRUCT(BlueprintType)
struct FRewardView
{
	GENERATED_BODY()

	// 돈
	// mGoldGained는 이번 전투 증분, mGoldBalance는 지급 반영 후 총액이다. UI 카운트업은 둘을 섞어 계산하지 않는다.
	UPROPERTY(BlueprintReadOnly) int32 mGoldGained = 0;
	UPROPERTY(BlueprintReadOnly) int32 mGoldBalance = 0;   // 보상 적용 후 총 보유

	// 경험치
	// 경험치 막대는 "현재 표시 레벨 안에서의 전/후 채움"을 받는다. 레벨업 횟수 표시는 LevelBefore/After 차이로 판단한다.
	UPROPERTY(BlueprintReadOnly) int32 mExpGained = 0;
	UPROPERTY(BlueprintReadOnly) int32 mLevelBefore = 0;
	UPROPERTY(BlueprintReadOnly) int32 mLevelAfter = 0;
	UPROPERTY(BlueprintReadOnly) float mExpBefore = 0.f;   // 레벨업 후 레벨 기준 시작 채움
	UPROPERTY(BlueprintReadOnly) float mExpAfter = 0.f;    // 최종 채움
	UPROPERTY(BlueprintReadOnly) float mMaxExp = 0.f;      // 현재 레벨 최대 경험치
};
