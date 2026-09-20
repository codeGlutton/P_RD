/*****************************************************************//**
 * @file   SRPGCompositeCombatRoundEvent.h
 * @brief  복합 라운드 이벤트 구조체 정의 헤더
 * @author 모호재
 * @date   2026-08-23
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "SRPGCompositeCombatRoundEvent.generated.h"

/**
 * @brief 여러 라운드 이벤트를 순차적/복합적으로 실행 관리하는 Composite 라운드 이벤트
 */
USTRUCT(BlueprintType)
struct FSRPGCompositeCombatRoundEvent : public FSRPGCombatRoundEvent
{
	GENERATED_BODY()

public:
	FSRPGCompositeCombatRoundEvent() = default;

public:
	void AddEvent(TInstancedStruct<FSRPGCombatRoundEvent> Event);

	FSRPGCombatRoundEvent* FindEvent(const FName& EventName);
	const FSRPGCombatRoundEvent* FindEvent(const FName& EventName) const;

private:
	bool CanTrigger_Internal(USRPGCombatModel* Model) const override;
	void Warning_Internal(USRPGCombatModel* Model) const override;
	void PreTrigger_Internal(USRPGCombatModel* Model) override;
	ESRPGCombatRoundEventResult Trigger_Internal(TSharedPtr<FPresentationBarrier> RoundBarrier, USRPGCombatModel* Model) override;
	void Reset_Internal(USRPGCombatModel* Model) override;

protected:
	UPROPERTY(Category = "Event", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Events"))
	FSRPGCombatRoundEventContainer mEvents;
};

