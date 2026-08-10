#include "Animation/Notify/AnimNotifyCondition.h"
#include "FunctionLibrary/VFXFunctionLibrary.h"
#include "FunctionLibrary/CameraFunctionLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNotifyCondition)

bool FAnimNotifyCondition_EffectOption::EvaluateCondition(const USkeletalMeshComponent* MeshComp) const
{
	if (MeshComp == nullptr)
	{
		return false;
	}

	return UVFXFunctionLibrary::IsVFXPossible(MeshComp->GetWorld());
}

bool FAnimNotifyCondition_CameraOption::EvaluateCondition(const USkeletalMeshComponent* MeshComp) const
{
	if (MeshComp == nullptr)
	{
		return false;
	}

	return UCameraFunctionLibrary::IsCameraShakePossible(MeshComp->GetWorld());
}
