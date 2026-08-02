/*****************************************************************//**
 * @file   AnimNotifyState_ConditionalEventTrigger.h
 * @brief  조건부 시간이 있는 이벤트 호출 애님 노티파이 스테이트 정의 헤더
 * @author 모호재
 * @date   2026-08-01
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Animation/Notify/AnimNotifyState_EventTrigger.h"
#include "Animation/Notify/AnimNotifyCondition.h"
#include "AnimNotifyState_ConditionalEventTrigger.generated.h"

/**
 * @brief 조건부 시간이 있는 이벤트 호출 애님 노티파이 스테이트
 */
UCLASS()
class P_RD_API UAnimNotifyState_ConditionalEventTrigger : public UAnimNotifyState_EventTrigger
{
	GENERATED_BODY()

	/* UAnimNotifyState 상속 */
public:
	FLinearColor GetEditorColor() override;

protected:
	void TriggerEvent(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, FOnEndDurationEventTrigger& EndDurationEvent) override;

protected:
	/**
	 * @brief 이벤트를 실행해야 하는지 조건 상태를 검증합니다.
	 * @param MeshComp 대상 스켈레탈 메시 컴포넌트
	 * @return 이벤트 실행이 가능하면 true
	 */
	bool ShouldPlayNotify(const USkeletalMeshComponent* MeshComp) const;

protected:
#if WITH_EDITORONLY_DATA
	UPROPERTY(Category = "Condition", EditAnywhere, meta = (DisplayName = "AlwaysShowInPreview", ToolTip = "조건이 충족되지 않더라도 몽타주 프리뷰 월드에서는 항상 이벤트를 보여줄지 여부"))
	bool mAlwaysShowInPreview = true;
#endif // WITH_EDITORONLY_DATA

	UPROPERTY(Category = "Condition", EditAnywhere, meta = (DisplayName = "Conditions", ToolTip = "이벤트 실행 여부를 판단할 조건 목록"))
	TArray<TInstancedStruct<FAnimNotifyCondition>> mConditions;
};
