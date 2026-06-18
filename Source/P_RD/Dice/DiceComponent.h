// @file DiceComponent.h
// @brief 플레이어 유닛이 소유하는 런타임 주사위(UDiceModel) 묶음 컴포넌트
// @date 2026-06-17

#pragma once

#include "RDMinimal.h"
#include "Components/ActorComponent.h"
#include "UObject/PrimaryAssetId.h"

#include "DiceComponent.generated.h"

class UDiceModel;

/** @brief 플레이어가 보유한 런타임 주사위들을 소유하고 굴림/사용 상태를 관리하는 컴포넌트입니다. */
// 주사위는 플레이어 전용 메커닉이라 AUnit 베이스가 아니라 APlayerUnit에 붙는다(적은 주사위를 굴리지 않음).
// 이 컴포넌트는 UI를 알지 않는다. UCombatUIAdapter가 GetDice()를 읽어 FDiceSlotUI로 변환해 push한다.
// 보유 구성은 런 mDiceIds 기반이며, 굴림 난수는 추후 런 RandomStream 주입으로 결정론을 맞춘다.
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class P_RD_API UDiceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDiceComponent();

	/** @brief 런 보유 주사위 id 목록으로 UDiceModel 인스턴스를 만든다(희귀도는 id로 해석). */
	void BuildFromDiceIds(const TArray<FPrimaryAssetId>& DiceIds);

	/** @brief 보유 주사위 전부를 주어진 난수 스트림으로 굴린다. */
	void RollAll(const FRandomStream& Stream);

	/** @brief 지정 index 주사위를 이번 턴에 사용됨으로 표시합니다. */
	void MarkDiceUsed(int32 DiceIndex);

	/** @brief 모든 주사위의 '사용됨'을 해제한다(턴 종료 훅용). */
	void ResetUsed();

	/** @brief 보유 주사위 묶음(읽기 전용). UI 어댑터가 뷰 변환에 사용. */
	const TArray<TObjectPtr<UDiceModel>>& GetDice() const { return mDice; }

	/** @brief 해당 index 주사위의 현재 굴림값(안 굴렸으면 0). */
	int32 GetRolledDiceValue(int32 DiceIndex) const;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UDiceModel>> mDice;
};
