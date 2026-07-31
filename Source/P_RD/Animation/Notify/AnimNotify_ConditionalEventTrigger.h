/*****************************************************************//**
 * @file   AnimNotify_ConditionalEventTrigger.h
 * @brief  조건부 이벤트 호출 애님 노티파이 정의 헤더
 * @author 모호재
 * @date   2026-07-31
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Animation/Notify/AnimNotify_EventTrigger.h"
#include "Animation/Notify/AnimNotifyCondition.h"
#include "AnimNotify_ConditionalEventTrigger.generated.h"

/**
 * @brief  조건부 이벤트 호출 애님 노티파이
 */
UCLASS()
class P_RD_API UAnimNotify_ConditionalEventTrigger : public UAnimNotify_EventTrigger
{
	GENERATED_BODY()

	/* UAnimNotify 상속 */
public:
	FLinearColor GetEditorColor() override;

protected:
	void TriggerEvent(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

protected:
	/**
	 * @brief 이펙트를 재생해야 하는지 조건 상태를 검증합니다.
	 * @param MeshComp 대상 스켈레탈 메시 컴포넌트
	 * @return 이펙트 재생이 가능하면 true
	 */
	bool ShouldPlayNotify(const USkeletalMeshComponent* MeshComp) const;

protected:
#if WITH_EDITORONLY_DATA
	UPROPERTY(Category = "Condition", EditAnywhere, meta = (DisplayName = "AlwaysShowInPreview", ToolTip = "조건이 충족되지 않더라도 몽타주 프리뷰 월드에서는 항상 이펙트를 보여줄지 여부"))
	bool mAlwaysShowInPreview = true;
#endif // WITH_EDITORONLY_DATA

	UPROPERTY(Category = "Condition", EditAnywhere, meta = (DisplayName = "Conditions", ToolTip = "이펙트 재생 여부를 판단할 조건 목록"))
	TArray<TInstancedStruct<FAnimNotifyCondition>> mConditions;
};
