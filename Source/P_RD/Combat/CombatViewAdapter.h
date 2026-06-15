/*****************************************************************//**
 * @file   CombatViewAdapter.h
 * @brief  전투 유닛/메타/턴 표시값을 비GAS 소스에서 만들어 전투 뷰모델에 밀어넣는 어댑터
 * @date   2026-06-16
 *********************************************************************/

#pragma once

#include "RDMinimal.h"

#include "CombatViewAdapter.generated.h"

class UCombatViewModel;
class USRPGCombatSubsystem;
class URunPersistData;

/**
 * @brief 전투 HUD의 유닛 HP바·상단 메타(HP/Gold/Lv)·턴 표시를 구동하는 임시 게임플레이 대역(비GAS).
 *
 * @details
 * GAS는 폐기 방향이라 AttributeSet 라이브 읽기를 하지 않는다. 데이터 소스는 전부 비GAS:
 * - 유닛 목록/위치/플레이어여부: USRPGCombatSubsystem이 스폰한 AUnit들에서.
 * - 레벨: URunPersistData.
 * - HP/Gold: 아직 데이터 계층(UUnitData)이 없어 플레이스홀더(맞추면 됨). 실값은 추후 UUnitData에서.
 *
 * UI(전투 HUD)는 이 어댑터가 SetUnitViews/SetPlayerMeta/SetTurnView로 밀어넣은 표시값만 읽는다.
 * UUnitData가 생기면 이 어댑터의 소스만 바꾸고 UI/뷰모델 계약은 그대로 둔다.
 */
UCLASS(BlueprintType)
class P_RD_API UCombatViewAdapter : public UObject
{
	GENERATED_BODY()

public:
	/** @brief 전투 서브시스템/런 데이터를 소스로 잡는다(아직 push는 BindViewModel 시점). */
	void Build(USRPGCombatSubsystem* InCombat, const URunPersistData* InRun);

	/** @brief 뷰모델에 연결하고 현재 표시값을 push한다. */
	void BindViewModel(UCombatViewModel* InViewModel);

	/** @brief 현재 소스로 유닛/메타/턴 표시값을 다시 만들어 뷰모델에 push한다. */
	void PushAll();

private:
	UPROPERTY(Transient)
	TObjectPtr<UCombatViewModel> mViewModel;

	UPROPERTY(Transient)
	TObjectPtr<USRPGCombatSubsystem> mCombat;

	int32 mPlayerLevel = 1;
};
