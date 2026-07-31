#include "Animation/Notify/AnimNotify_ConditionalPlayNiagaraEffect.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNotify_ConditionalPlayNiagaraEffect)

FLinearColor UAnimNotify_ConditionalPlayNiagaraEffect::GetEditorColor()
{
	FLinearColor Color = Super::GetEditorColor() * 0.5f;
	Color.A = 255;

	return Color;
}

UFXSystemComponent* UAnimNotify_ConditionalPlayNiagaraEffect::SpawnEffect(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	/* 조건 검사 후 이펙트 재생 */

	if (ShouldPlayNotify(MeshComp) == false)
	{
		return nullptr;
	}
	return Super::SpawnEffect(MeshComp, Animation);
}

bool UAnimNotify_ConditionalPlayNiagaraEffect::ShouldPlayNotify(const USkeletalMeshComponent* MeshComp) const
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
