#pragma once

/**
 * @file RewardViewModel.h
 * @brief 전투 보상 화면 UI와 게임플레이를 잇는 경계(뷰모델)입니다.
 *
 * @details
 * 전투 뷰모델(UCombatViewModel)과 같은 계약:
 * - 읽기(gameplay → UI): 게임플레이/어댑터가 SetReward()로 결과를 밀어넣고, 위젯은 GetReward()로 읽는다.
 * - 주기(UI → gameplay): '받기' 버튼은 RequestClaim()으로 의도만 보내고, 게임플레이가 OnRewardClaimed를
 *   구독해 다음 화면으로 넘어간다. UI는 보상을 직접 지급/소모하지 않는다.
 *
 * 지금은 돈/경험치만 다룬다(아이템 보상은 이후 도메인 추가).
 */

#include "RDMinimal.h"
#include "UI/Reward/RewardViewTypes.h"

#include "RewardViewModel.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRewardViewChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRewardClaimed);

/**
 * @brief 보상 화면 뷰모델. 전투 종료 시 게임플레이가 하나 만들어 보상 위젯에 물린다.
 */
UCLASS(BlueprintType)
class P_RD_API URewardViewModel : public UObject
{
	GENERATED_BODY()

	/* ───────── 위젯이 구독하는 알림 ───────── */
public:
	/** @brief 보상값이 설정/갱신됐음을 알림. 위젯은 카운트업/막대 연출을 시작한다. */
	UPROPERTY(BlueprintAssignable, Category = "Reward|View")
	FOnRewardViewChanged OnViewChanged;

	/* ───────── 게임플레이가 구독하는 입력(의도) ───────── */
public:
	/** @brief 위젯이 '받기'를 눌렀음(다음 화면으로). */
	UPROPERTY(BlueprintAssignable, Category = "Reward|Input")
	FOnRewardClaimed OnRewardClaimed;

	/* ───────── UI → gameplay : 의도만 보낸다 ───────── */
public:
	UFUNCTION(BlueprintCallable, Category = "Reward|Input") void RequestClaim();

	/* ───────── gameplay → UI : 표시값을 밀어넣는다 ───────── */
public:
	UFUNCTION(BlueprintCallable, Category = "Reward|Push") void SetReward(const FRewardView& Reward);

	/* ───────── 위젯이 읽는다 ───────── */
public:
	UFUNCTION(BlueprintPure, Category = "Reward|Read") const FRewardView& GetReward() const { return mReward; }

private:
	UPROPERTY(Transient) FRewardView mReward;
};
