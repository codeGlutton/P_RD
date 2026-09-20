#include "Animation/Notify/AnimNotifyState_ConditionalTimedNiagaraEffect.h"
#include "FunctionLibrary/VFXFunctionLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNotifyState_ConditionalTimedNiagaraEffect)

FLinearColor UAnimNotifyState_ConditionalTimedNiagaraEffect::GetEditorColor()
{
	FLinearColor Color = Super::GetEditorColor() * 0.5f;
	Color.A = 255;

	return Color;
}

UFXSystemComponent* UAnimNotifyState_ConditionalTimedNiagaraEffect::SpawnEffect(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) const
{
	/* 조건 검사 후 이펙트 재생 */

	if (ShouldPlayNotify(MeshComp) == false)
	{
		return nullptr;
	}
	return Super::SpawnEffect(MeshComp, Animation);
}

bool UAnimNotifyState_ConditionalTimedNiagaraEffect::ShouldPlayNotify(const USkeletalMeshComponent* MeshComp) const
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

	if (UVFXFunctionLibrary::IsVFXPossible(MeshComp->GetWorld()) == false)
	{
		return false;
	}

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
