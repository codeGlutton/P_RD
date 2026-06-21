#pragma once

/** @brief 전투 보상 화면이 게임플레이에서 받는 '뷰 데이터' 정의입니다. */
// @file RewardUITypes.h
// 전투 뷰모델과 같은 원칙: UI는 게임플레이 객체를 직접 알지 않고 표시값만 받아 그린다.
// 지금은 보상 종류 중 '돈(골드)'과 '경험치'만 담는다. 아이템/주사위 보상은 이후 같은 패턴으로 확장한다.
// 카운트업/레벨업 막대 연출을 UI가 스스로 할 수 있게 '전/후' 값을 함께 준다(게임플레이는 결과만 알려줌).

#include "RDMinimal.h"

#include "RewardUITypes.generated.h"

class UTexture2D;

/** @brief 전투 결과 보상 한 건의 표시값(돈·경험치). */
// - 골드: 이번에 번 양(mGoldGained)과 합산 후 잔액(mGoldBalance). UI는 잔액으로 카운트업.
// - 경험치: 이번에 번 양(mExpGained)과, 막대 채움을 위한 레벨 내 전/후 값(mExpBefore→mExpAfter)
// + 해당 레벨 최대치(mMaxExp). 레벨업이 있으면 mLevelBefore != mLevelAfter.
USTRUCT(BlueprintType)
struct FRewardUI
{
	GENERATED_BODY()

	// 화면 제목 — 방 타입별로 다르다(전투 보상 / 보물방 등). 기획: ROOM RESULT / VICTORY REWARD / TREASURE ROOM.
	UPROPERTY(BlueprintReadOnly) FText mTitle;

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

/** @brief 보상 항목 한 칸의 종류. UI가 게임플레이 데이터 타입을 직접 모르게 어댑터가 변환해 넣는다. */
// 기획(방 별 보상 체계): 보상은 방 타입이 결정한다 — 엘리트=장비, 보스=주사위, 보물=장비. (3택1 아님)
UENUM(BlueprintType)
enum class ERewardChoiceKind : uint8
{
	Dice,
	Skill,
	Equipment,
	Gold
};

/** @brief 그 방이 지급하는 보상 항목 하나를 그리기 위한 표시값입니다(획득 = Claim, 선택 아님). */
// 골드/경험치(FRewardUI)와 함께, 방 타입이 주는 항목(엘리트=장비/보스=주사위 등)을 카드로 보여준다.
// UI 필요값:
// - mChoiceIndex: 항목 index(상세 요청 등 payload).
// - mKind: 다이스/스킬/장비/골드 구분(기본 아이콘 결정).
// - mName: 항목 이름(예: "철 검").
// - mIcon: 프레임+희귀도 내장 아이템 아이콘(어댑터가 실제 아이템 아이콘을 넣음).
// - mDescription: 보조 한 줄 — 장비=장착 부위/효과, 주사위=종류·면, 스킬=설명.
// - mRarityColor: 희귀도(아이콘에 틴트가 필요할 때만 사용; 보통 아이콘 프레임에 내장).
// [합의필요] 최종 소스 = 방 보상 롤 결과(게임플레이). 현재 Mock.
USTRUCT(BlueprintType)
struct FRewardChoiceUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 mChoiceIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) ERewardChoiceKind mKind = ERewardChoiceKind::Dice;
	UPROPERTY(BlueprintReadOnly) FText mName;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> mIcon = nullptr;
	UPROPERTY(BlueprintReadOnly) FText mDescription;
	UPROPERTY(BlueprintReadOnly) FLinearColor mRarityColor = FLinearColor::White;
};
