/*****************************************************************//**
 * @file   AnimNotifyState_EventTrigger.h
 * @brief  시간이 있는 이벤트 호출 애님 노티파이 스테이트 정의 헤더
 * @author 모호재
 * @date   2026-07-15
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Animation/Notify/EventTriggerPayload.h"
#include "AnimNotifyState_EventTrigger.generated.h"

/**
 * @brief  시간이 있는 이벤트 호출 애님 노티파이 스테이트
 */
UCLASS()
class P_RD_API UAnimNotifyState_EventTrigger : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UAnimNotifyState_EventTrigger();

	/* UAnimNotifyState 상속 */
public:
	FString GetNotifyName_Implementation() const override;
	FLinearColor GetEditorColor() override;

	void NotifyBegin(class USkeletalMeshComponent* MeshComp, class UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	void NotifyEnd(class USkeletalMeshComponent* MeshComp, class UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

#if WITH_EDITOR
	void ValidateAssociatedAssets() override;
#endif

protected:
	virtual void TriggerEvent(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, FOnEndDurationEventTrigger& EndDurationEvent);

protected:
	UPROPERTY(Category = "Event", EditAnywhere, meta = (DisplayName = "TargetEventTag", ToolTip = "호출할 이벤트 태그"))
	FGameplayTag mTargetEventTag;

	UPROPERTY(Category = "Event", EditAnywhere, meta = (DisplayName = "EventPayload", ToolTip = "같이 전달할 페이로드"))
	TInstancedStruct<FDurationEventTriggerPayload> mEventPayload;
};
