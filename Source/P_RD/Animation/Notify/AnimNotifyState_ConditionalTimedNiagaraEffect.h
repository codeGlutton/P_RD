/*****************************************************************//**
 * @file   AnimNotifyState_ConditionalTimedNiagaraEffect.h
 * @brief  조건부 재생 나이아가라 타이머 애님 노티파이 스테이트 정의 헤더
 * @author 모호재
 * @date   2026-07-15
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "AnimNotifyState_TimedNiagaraEffect.h"
#include "Animation/Notify/AnimNotifyCondition.h"
#include "AnimNotifyState_ConditionalTimedNiagaraEffect.generated.h"

/**
 * @brief 설정된 조건들을 검증하고 조건부로 작동하는 Niagara Timed Effect Notify State
 */
UCLASS()
class P_RD_API UAnimNotifyState_ConditionalTimedNiagaraEffect : public UAnimNotifyState_TimedNiagaraEffect
{
	GENERATED_BODY()

public:
	UAnimNotifyState_ConditionalTimedNiagaraEffect();

	/* UAnimNotifyState 상속 */
protected:
	UFXSystemComponent* SpawnEffect(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) const override;

protected:
	/**
	 * @brief 이펙트를 재생해야 하는지 조건 상태를 검증합니다.
	 * @param MeshComp 대상 스켈레탈 메시 컴포넌트
	 * @return 이펙트 재생이 가능하면 true
	 */
	bool ShouldPlayEffect(const USkeletalMeshComponent* MeshComp) const;

protected:
#if WITH_EDITORONLY_DATA
	UPROPERTY(Category = "Condition", EditAnywhere, meta = (DisplayName = "AlwaysShowInPreview", ToolTip = "조건이 충족되지 않더라도 몽타주 프리뷰 월드에서는 항상 이펙트를 보여줄지 여부"))
	bool mAlwaysShowInPreview = true;
#endif // WITH_EDITORONLY_DATA

	UPROPERTY(Category = "Condition", EditAnywhere, meta = (DisplayName = "Conditions", ToolTip = "이펙트 재생 여부를 판단할 조건 목록"))
	TArray<TInstancedStruct<FAnimNotifyCondition>> mConditions;
};
