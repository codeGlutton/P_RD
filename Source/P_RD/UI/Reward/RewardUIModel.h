#pragma once

/** @brief 전투 보상 화면 UI와 게임플레이를 잇는 경계(뷰모델)입니다. */
// @file RewardUIModel.h
// 전투 뷰모델(UCombatUIModel)과 같은 계약:
// - 읽기(gameplay → UI): 게임플레이/어댑터가 SetReward()로 결과를 밀어넣고, 위젯은 GetReward()로 읽는다.
// - 주기(UI → gameplay): '받기' 버튼은 RequestClaim()으로 의도만 보내고, 게임플레이가 OnRewardClaimed를
// 구독해 다음 화면으로 넘어간다. UI는 보상을 직접 지급/소모하지 않는다.
// 지금은 돈/경험치만 다룬다(아이템 보상은 이후 도메인 추가).

#include "RDMinimal.h"
#include "UI/Reward/RewardUITypes.h"

#include "RewardUIModel.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRewardUIChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRewardClaimed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRewardChoicesChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRewardChosen, int32, ChoiceIndex);

/** @brief 보상 화면 뷰모델. 전투 종료 시 게임플레이가 하나 만들어 보상 위젯에 물린다. */
// 이 객체는 보상을 계산하거나 지급하지 않는다. 표시 스냅샷과 UI 입력 의도만 양방향으로 중계한다.
UCLASS(BlueprintType)
class P_RD_API URewardUIModel : public UObject
{
	GENERATED_BODY()

	/* ───────── 위젯이 구독하는 알림 ───────── */
public:
	/** @brief 보상값이 설정/갱신됐음을 알림. 위젯은 카운트업/막대 연출을 시작한다. */
	UPROPERTY(BlueprintAssignable, Category = "Reward|UI")
	FOnRewardUIChanged OnUIChanged;

	/* ───────── 게임플레이가 구독하는 입력(의도) ───────── */
public:
	/** @brief 위젯이 '받기'를 눌렀음(다음 화면으로). */
	UPROPERTY(BlueprintAssignable, Category = "Reward|Input")
	FOnRewardClaimed OnRewardClaimed;

	/** @brief 3택1 선택지가 설정/갱신됐음을 알림. 위젯은 선택지 카드를 다시 그린다. */
	UPROPERTY(BlueprintAssignable, Category = "Reward|UI")
	FOnRewardChoicesChanged OnChoicesChanged;

	/** @brief 위젯이 선택지 하나를 골랐음(ChoiceIndex). 게임플레이가 실제 보상을 지급한다. */
	UPROPERTY(BlueprintAssignable, Category = "Reward|Input")
	FOnRewardChosen OnRewardChosen;

	/* ───────── UI → gameplay : 의도만 보낸다 ───────── */
public:
	/** @brief UI가 '받기'를 눌렀다는 의도를 게임플레이 구독자에게 전달한다. */
	// 중복 클릭 방지는 실제 화면/게임플레이 플로우가 결정한다. UIModel은 순수 신호만 보낸다.
	UFUNCTION(BlueprintCallable, Category = "Reward|Input") void RequestClaim();

	/** @brief 3택1 보상에서 ChoiceIndex를 골랐다는 의도를 게임플레이 구독자에게 전달한다. */
	// 실제 지급(다이스/스킬/장비/골드 추가)은 게임플레이가 한다. UIModel은 선택 신호만 보낸다.
	UFUNCTION(BlueprintCallable, Category = "Reward|Input") void RequestChooseReward(int32 ChoiceIndex);

	/* ───────── gameplay → UI : 표시값을 밀어넣는다 ───────── */
public:
	/** @brief 게임플레이/어댑터가 확정한 보상 표시 스냅샷을 저장하고 변경 알림을 보낸다. */
	UFUNCTION(BlueprintCallable, Category = "Reward|Push") void SetReward(const FRewardUI& Reward);

	/** @brief 3택1 보상 선택지를 설정하고 변경 알림을 보낸다(보통 3개). */
	UFUNCTION(BlueprintCallable, Category = "Reward|Push") void SetRewardChoices(const TArray<FRewardChoiceUI>& Choices);

	/* ───────── 위젯이 읽는다 ───────── */
public:
	/** @brief 위젯이 현재 보상 스냅샷을 읽는다; 반환 참조는 다음 SetReward()까지 유효하다. */
	UFUNCTION(BlueprintPure, Category = "Reward|Read") const FRewardUI& GetReward() const { return mReward; }

	/** @brief 위젯이 현재 3택1 선택지 목록을 읽는다; 반환 참조는 다음 SetRewardChoices()까지 유효하다. */
	UFUNCTION(BlueprintPure, Category = "Reward|Read") const TArray<FRewardChoiceUI>& GetRewardChoices() const { return mChoices; }

private:
	/** @brief 마지막으로 Push된 보상 표시값; Transient라 세이브/에셋 상태로 남기지 않는다. */
	UPROPERTY(Transient) FRewardUI mReward;

	/** @brief 마지막으로 Push된 3택1 선택지 목록. */
	UPROPERTY(Transient) TArray<FRewardChoiceUI> mChoices;
};
