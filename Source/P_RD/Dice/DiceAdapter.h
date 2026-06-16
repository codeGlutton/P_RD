/*****************************************************************//**
 * @file   DiceAdapter.h
 * @brief  런타임 주사위(UDiceData) 묶음을 소유하고 전투 뷰모델의 주사위 도메인을 구동하는 어댑터
 * @date   2026-06-16
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "UObject/PrimaryAssetId.h"

#include "DiceAdapter.generated.h"

class UCombatViewModel;
class UDiceData;
enum class ECombatInputType : uint8;

/**
 * @brief 보유 주사위(UDiceData들)를 소유하고, 전투 뷰모델의 주사위 도메인을 구동하는 임시 게임플레이 대역.
 *
 * @details
 * 아직 모호재의 UUnitData 데이터 계층이 없어, 다이스 소유를 이 어댑터가 "독립적으로" 가진다.
 * (나중에 UUnitData가 생기면 이 책임을 거기로 흡수하고 어댑터는 얇아지거나 사라진다.)
 *
 * 경계 규약(UCombatViewModel)을 그대로 따른다:
 * - 입력(UI→gameplay): 뷰모델 OnCombatCommand(RollDice/ToggleDice)를 구독해 데이터 위에서 처리.
 * - 출력(gameplay→UI): 굴림 결과를 FDiceSlotView로 변환해 ViewModel->SetDiceViews()로 push.
 * UI(HUD/패널)는 RNG/상태를 갖지 않고 뷰모델만 읽는다. 진실은 이 어댑터의 UDiceData에 있다.
 */
UCLASS(BlueprintType)
class P_RD_API UDiceAdapter : public UObject
{
	GENERATED_BODY()

public:
	/** @brief 런 보유 주사위 id 목록으로 UDiceData 인스턴스를 만든다(희귀도는 id로 해석). */
	void BuildFromDiceIds(const TArray<FPrimaryAssetId>& DiceIds);

	/** @brief 뷰모델에 연결해 입력(굴림/선택)을 구독하고 현재 상태를 push한다. nullptr이면 구독 해제. */
	void BindViewModel(UCombatViewModel* InViewModel);

	/** @brief 보유 주사위 전부를 주어진 난수 스트림으로 굴린 뒤 뷰모델에 push한다. */
	void RollAll(const FRandomStream& Stream);

	/** @brief 지정 index 주사위를 '사용됨'으로 표시(다음 굴림까지 재사용 불가)하고 뷰모델에 반영한다. */
	void MarkDiceUsed(int32 DiceIndex);

	const TArray<TObjectPtr<UDiceData>>& GetDice() const { return mDice; }

private:
	/** @brief 뷰모델이 보낸 UI 의도(굴림/선택)를 데이터 위에서 처리한다. */
	UFUNCTION()
	void HandleCombatCommand(ECombatInputType Type, int32 IntPayload);

	/** @brief 현재 UDiceData 상태를 FDiceSlotView[]로 변환해 ViewModel->SetDiceViews()로 push한다. */
	void PushDiceViews() const;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UDiceData>> mDice;

	UPROPERTY(Transient)
	TObjectPtr<UCombatViewModel> mViewModel;
};
