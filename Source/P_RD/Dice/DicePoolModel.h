// @file DiceComponent.h
// @brief 플레이어 유닛이 소유하는 런타임 주사위(UDiceModel) 묶음 컴포넌트
// @date 2026-06-17

#pragma once

#include "RDMinimal.h"
#include "Component/ComponentModel.h"
#include "UObject/PrimaryAssetId.h"

#include "DicePoolModel.generated.h"

class UDiceModel;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnRollAllDicesUI, const TArray<TObjectPtr<UDiceModel>>& /*Dices*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnUseDiceUI, const UDiceModel* /*Dice*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnResetAllDiceUI, const TArray<TObjectPtr<UDiceModel>>& /*Dices*/);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnSelectedDiceUI, const UDiceModel* /*Dice*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnUnselectedDiceUI, const UDiceModel* /*Dice*/);

/** @brief 플레이어가 보유한 런타임 주사위들을 소유하고 굴림/사용 상태를 관리하는 컴포넌트 모델입니다. */
// 주사위 풀은 플레이어 전용 메커닉이라 APlayerUnit이 소유한다(적은 주사위를 굴리지 않음).
// 규칙: 컴포넌트 형식 Model이므로 UComponentModel(IObjectModel)을 상속한다.
// 이 모델은 UI를 알지 않는다(단방향). UCombatUIAdapter가 GetDices()를 읽어 FDiceSlotUI로 변환해 push한다.
// 보유 구성은 런 mDiceIds 기반이며, 굴림 난수는 추후 런 RandomStream 주입으로 결정론을 맞춘다.
UCLASS()
class P_RD_API UDicePoolModel : public UComponentModel
{
	GENERATED_BODY()

public:
	/** @brief 런 보유 주사위 id 목록으로 UDiceModel 인스턴스를 만든다(희귀도는 id로 해석). */
	void BuildFromDiceIds(const TArray<FPrimaryAssetId>& DiceIds);

	/** @brief 보유 주사위 전부를 주어진 난수 스트림으로 굴린다. */
	void RollAll(const FRandomStream& Stream);

	/** @brief 지정 index 주사위를 선택됨으로 표시합니다. */
	void MarkDiceSelected(int32 DiceIndex);

	/** @brief 지정 index 주사위를 선택됨을 해제합니다. */
	void MarkDiceUnselected(int32 DiceIndex);

	/** @brief 모든 주사위의 '선택됨'을 해제한다. */
	void ResetSelected();

	/** @brief 지정 index 주사위를 이번 턴에 사용됨으로 표시합니다. */
	void MarkDiceUsed(int32 DiceIndex);

	/** @brief 선택된 주사위들을 이번 턴에 사용됨으로 변경합니다. */
	void MarkSelectedDiceAsUsed();

	/** @brief 모든 주사위의 '사용됨'을 해제한다(턴 종료 훅용). */
	void ResetUsed();

	/** @brief 보유 주사위 묶음(읽기 전용). UI 어댑터가 뷰 변환에 사용. */
	const UDiceModel* GetDice(int32 DiceIndex) const
	{ 
		if (mDices.IsValidIndex(DiceIndex) == false)
		{
			return nullptr;
		}
		return mDices[DiceIndex];
	}
	const TArray<TObjectPtr<UDiceModel>>& GetDices() const 
	{ 
		return mDices; 
	}

	/** @brief 해당 index 주사위의 현재 굴림값(안 굴렸으면 0). */
	int32 GetRolledDiceValue(int32 DiceIndex) const;

	/** @brief 선택된 주사위인지 여부 */
	bool IsSelectedDice(int32 DiceIndex) const;
	/** @brief 총 선택된 주사위 갯수 */
	int32 GetSelectedDiceNum() const;
	/** @brief 총 선택된 주사위 합산값 */
	int32 GetSelectedDiceSum() const;

public:
	FOnRollAllDicesUI OnRollAllDicesUI;
	FOnUseDiceUI OnUseDiceUI;
	FOnResetAllDiceUI OnResetAllDiceUI;

	FOnSelectedDiceUI OnSelectedDiceUI;
	FOnUnselectedDiceUI OnUnselectedDiceUI;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UDiceModel>> mDices;
};
