/*****************************************************************//**
 * @file   AnimNotify_EventTrigger.h
 * @brief  이벤트 호출 애님 노티파이 정의 헤더
 * @author 모호재
 * @date   2026-07-05
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/Notify/EventTriggerPayload.h"
#include "AnimNotify_EventTrigger.generated.h"

/**
 * @brief  이벤트 호출 애님 노티파이
 */
UCLASS()
class P_RD_API UAnimNotify_EventTrigger : public UAnimNotify
{
	GENERATED_BODY()
	
public:
	UAnimNotify_EventTrigger();

	/* UAnimNotify 상속 */
public:
	FString GetNotifyName_Implementation() const override;
	FLinearColor GetEditorColor() override;

	void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

#if WITH_EDITOR
	void ValidateAssociatedAssets() override;
#endif

protected:
	virtual void TriggerEvent(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation);

protected:
	UPROPERTY(Category = "Event", EditAnywhere, meta = (DisplayName = "TargetEventTag", ToolTip = "호출할 이벤트 태그"))
	FGameplayTag mTargetEventTag;

	UPROPERTY(Category = "Event", EditAnywhere, meta = (DisplayName = "EventPayload", ToolTip = "같이 전달할 페이로드"))
	TInstancedStruct<FEventTriggerPayload> mEventPayload;
};
