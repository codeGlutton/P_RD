#pragma once

/** @brief 전투 보상 화면이 게임플레이에서 받는 '뷰 데이터' 정의입니다. */
// @file RewardUITypes.h
// 전투 뷰모델과 같은 원칙: UI는 게임플레이 객체를 직접 알지 않고 표시값만 받아 그린다.
// 돈(골드), 경험치, 룸이 지급하는 항목 보상을 같은 패턴으로 표시한다.
// 정산 화면은 전/후 값을 즉시 그릴 수 있고, RewardConcept03은 같은 데이터를
// 레벨 구간별 게이지 애니메이션으로 재생한다.

#include "RDMinimal.h"

#include "RewardUITypes.generated.h"

class UTexture2D;

/** @brief 경험치 보상 한 번이 한 레벨 구간에서 이동하는 진행도. */
USTRUCT(BlueprintType)
struct FRewardExpProgressStepUI
{
	GENERATED_BODY()

	/** 이 구간을 시작할 때의 레벨. */
	UPROPERTY(BlueprintReadOnly) int32 mLevelBefore = 1;
	/** 이 구간을 마친 뒤의 레벨. 임계치를 넘지 않으면 mLevelBefore와 같다. */
	UPROPERTY(BlueprintReadOnly) int32 mLevelAfter = 1;
	UPROPERTY(BlueprintReadOnly) float mExpBefore = 0.f;
	UPROPERTY(BlueprintReadOnly) float mExpAfter = 0.f;
	/** 이 구간 레벨의 실제 필요 경험치. */
	UPROPERTY(BlueprintReadOnly) float mMaxExp = 0.f;

	bool IsLevelUp() const { return mLevelAfter > mLevelBefore; }
};

/** @brief 경험치 보상을 받는 용병 한 명의 지급 전/후 표시값. */
USTRUCT(BlueprintType)
struct FRewardMercenaryExpUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FText mName;
	/** @brief 정산 행에 표시할 실제 용병 얼굴 아이콘. */
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> mPortrait = nullptr;
	/** 구형 WBP 호환 필드. 새 데이터에서는 최종 레벨과 같다. */
	UPROPERTY(BlueprintReadOnly) int32 mLevel = 1;
	/** 0은 구형 단일 레벨 payload에서 범위가 지정되지 않았다는 뜻이다. */
	UPROPERTY(BlueprintReadOnly) int32 mLevelBefore = 0;
	UPROPERTY(BlueprintReadOnly) int32 mLevelAfter = 0;
	UPROPERTY(BlueprintReadOnly) float mExpBefore = 0.f;
	UPROPERTY(BlueprintReadOnly) float mExpAfter = 0.f;
	/** 최종 레벨에서 다음 레벨까지 필요한 경험치. */
	UPROPERTY(BlueprintReadOnly) float mMaxExp = 0.f;
	/**
	 * 레벨업 rollover를 포함한 표시 구간. 비어 있으면 위 구형 단일 구간
	 * 필드로 그려 기존 Blueprint/테스트 데이터를 계속 지원한다.
	 */
	UPROPERTY(BlueprintReadOnly) TArray<FRewardExpProgressStepUI> mProgressSteps;

	int32 GetLevelUpCount() const
	{
		return FMath::Max(0, mLevelAfter - mLevelBefore);
	}
};

/** @brief 전투 결과 보상 한 건의 표시값(돈·경험치). */
// - 골드: 이번에 번 양(mGoldGained)과 합산 후 잔액(mGoldBalance)을 함께 표시한다.
// - 경험치: 이번에 번 양(mExpGained)과 레벨 내 전/후 값(mExpBefore→mExpAfter),
// + 해당 레벨 최대치(mMaxExp)를 정적인 문구로 표시한다. 레벨업이 있으면 mLevelBefore != mLevelAfter.
USTRUCT(BlueprintType)
struct FRewardUI
{
	GENERATED_BODY()

	// 화면 제목 — 방 타입별로 다르다(전투 보상 / 보물방 등). 기획: ROOM RESULT / VICTORY REWARD / TREASURE ROOM.
	UPROPERTY(BlueprintReadOnly) FText mTitle;

	// 돈
	// mGoldGained는 이번 전투 증분, mGoldBalance는 지급 반영 후 총액이다.
	UPROPERTY(BlueprintReadOnly) int32 mGoldGained = 0;
	UPROPERTY(BlueprintReadOnly) int32 mGoldBalance = 0;   // 보상 적용 후 총 보유

	// 경험치
	// 전/후 값은 레벨과 현재 진행도를 정적으로 표시하는 데 사용한다.
	UPROPERTY(BlueprintReadOnly) int32 mExpGained = 0;
	UPROPERTY(BlueprintReadOnly) int32 mLevelBefore = 0;
	UPROPERTY(BlueprintReadOnly) int32 mLevelAfter = 0;
	UPROPERTY(BlueprintReadOnly) float mExpBefore = 0.f;   // 레벨업 후 레벨 기준 시작 채움
	UPROPERTY(BlueprintReadOnly) float mExpAfter = 0.f;    // 최종 채움
	UPROPERTY(BlueprintReadOnly) float mMaxExp = 0.f;      // 현재 레벨 최대 경험치

	/** @brief EXP가 용병별 값으로 바뀐 뒤의 실제 지급 대상과 각 진행도. */
	UPROPERTY(BlueprintReadOnly) TArray<FRewardMercenaryExpUI> mMercenaryExp;
};

/** @brief 보상 항목 한 칸의 종류. UI가 게임플레이 데이터 타입을 직접 모르게 어댑터가 변환해 넣는다. */
// 보상은 방 타입이 결정하며 고정 개수가 아니다.
UENUM(BlueprintType)
enum class ERewardChoiceKind : uint8
{
	Skill,
	Equipment,
	Gold,
	Artifact
};

UENUM(BlueprintType)
enum class ERewardAcquisitionPolicy : uint8
{
	None,
	GrantAll,
	SelectOne
};

/** @brief UI에서 클릭한 보상 행이 어떤 실제 지급 항목인지 구분한다. */
UENUM(BlueprintType)
enum class ERewardClaimKind : uint8
{
	Gold,
	Exp,
	Choice
};

/** @brief 그 방이 지급하는 보상 항목 하나를 그리기 위한 표시값입니다(획득 = Claim, 선택 아님). */
// 골드/경험치(FRewardUI)와 함께 방이 주는 항목을 카드로 보여준다.
// UI 필요값:
// - mChoiceIndex: 항목 index(상세 요청 등 payload).
// - mKind: 스킬/장비/골드 구분(기본 아이콘 결정).
// - mName: 항목 이름(예: "철 검").
// - mIcon: 프레임+희귀도 내장 아이템 아이콘(어댑터가 실제 아이템 아이콘을 넣음).
// - mDescription: 보조 한 줄 — 장비=장착 부위/효과, 스킬=설명.
// - mRarityColor: 희귀도(아이콘에 틴트가 필요할 때만 사용; 보통 아이콘 프레임에 내장).
// 최종 소스 = 방 보상 롤 결과(게임플레이).
USTRUCT(BlueprintType)
struct FRewardChoiceUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 mChoiceIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) ERewardChoiceKind mKind = ERewardChoiceKind::Equipment;
	UPROPERTY(BlueprintReadOnly) FPrimaryAssetId mSourceAssetId;
	UPROPERTY(BlueprintReadOnly) FText mName;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> mIcon = nullptr;
	UPROPERTY(BlueprintReadOnly) FText mDescription;
	UPROPERTY(BlueprintReadOnly) FLinearColor mRarityColor = FLinearColor::White;
	/** Shared CombatDetail overlay subtitle. */
	UPROPERTY(BlueprintReadOnly) FText mRarityName;
	/** Common=0, Rare=1, Epic=2; drives the existing 1/3/5 rarity gems. */
	UPROPERTY(BlueprintReadOnly) int32 mRarityLevel = 0;
};

USTRUCT(BlueprintType)
struct FRewardSelectionOfferUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) TArray<FRewardChoiceUI> mOptions;
	UPROPERTY(BlueprintReadOnly) int32 mSelectionCount = 1;
};

USTRUCT(BlueprintType)
struct FRewardGrantBundleUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) TArray<FRewardChoiceUI> mItems;
};

USTRUCT(BlueprintType)
struct FRewardGrantBundleResultUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) TArray<FPrimaryAssetId> mGrantedItemIds;
	UPROPERTY(BlueprintReadOnly) TArray<FPrimaryAssetId> mFailedItemIds;
};
