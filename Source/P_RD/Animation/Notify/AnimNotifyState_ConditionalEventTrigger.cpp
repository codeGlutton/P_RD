#include "Animation/Notify/AnimNotifyState_ConditionalEventTrigger.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNotifyState_ConditionalEventTrigger)

FLinearColor UAnimNotifyState_ConditionalEventTrigger::GetEditorColor()
{
	FLinearColor Color = Super::GetEditorColor() * 0.5f;
	Color.A = 255;

	return Color;
}

void UAnimNotifyState_ConditionalEventTrigger::TriggerEvent(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, FOnEndDurationEventTrigger& EndDurationEvent)
{
	/* 조건 검사 후 이벤트 재생 */

	if (ShouldPlayNotify(MeshComp) == false)
	{
		return;
	}
	Super::TriggerEvent(MeshComp, Animation, EndDurationEvent);
}

bool UAnimNotifyState_ConditionalEventTrigger::ShouldPlayNotify(const USkeletalMeshComponent* MeshComp) const
{
	if (MeshComp == nullptr)
	{
		return false;
	}

#if WITH_EDITORONLY_DATA
	const UWorld* World = MeshComp->GetWorld();
	if (World != nullptr)
	{
		if (mAlwaysShowInPreview == true && World->WorldType == EWorldType::EditorPreview)
		{
			return true;
		}
	}
#endif // WITH_EDITORONLY_DATA

	/* 모든 등록된 조건 검사 */
	for (const TInstancedStruct<FAnimNotifyCondition>& Condition : mConditions)
	{
		if (Condition.IsValid() == true)
		{
			if (Condition.Get().EvaluateCondition(MeshComp) == false)
			{
				return false;
			}
		}
	}

	return true;
}
