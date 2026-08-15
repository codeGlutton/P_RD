#include "Component/SkillAnimationComponent/SkeletonSkillAnimationComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "Actor/BoardActor/BoardCombatTargetView.h"

#include "Singleton/WorldSubsystem/PresentationBarrier.h"

#include "Animation/BoardActorAnimInstance.h"
#include "Animation/SkillAnimationMetaData.h"

void USkeletonSkillAnimationComponent::PlayApplyAnimation(const FBoardActorAnimationContext& Context)
{
	UBoardActorAnimInstance* AnimInstance = GetTargetAnimInstance();
	if (AnimInstance == nullptr)
	{
		return;
	}

	AnimInstance->PlayMontageUsingTag(Context);
}

void USkeletonSkillAnimationComponent::PlayHitAnimation(TSharedPtr<FPresentationBarrier> SkillEndBarrier, FGameplayTag MontageTag, ETileActorDirection MontageDir) const
{
	UBoardActorAnimInstance* AnimInstance = GetTargetAnimInstance();
	if (AnimInstance == nullptr)
	{
		return;
	}

	FBoardActorAnimationContext Context(MontageTag, MontageDir);
	
	/* 연출 대기 */
	{
		Context.mMontageEndEvent.AddLambda([SkillEndBarrier](const FBoardActorAnimationContext& Context, UObject* EndAnim, bool IsInterrupted) {
			});
	}

	AnimInstance->PlayMontageUsingTag(Context);
}

USkeletalMeshComponent* USkeletonSkillAnimationComponent::GetTargetMeshComponent() const
{
	if (mTargetMeshComponent.IsValid() == false)
	{
		mTargetMeshComponent = Cast<USkeletalMeshComponent>(GetOwner<IBoardCombatTargetView>()->GetTargetMeshComponent());
	}
	return mTargetMeshComponent.Get();
}

UBoardActorAnimInstance* USkeletonSkillAnimationComponent::GetTargetAnimInstance() const
{
	if (mTargetAnimInstance.IsValid() == false)
	{
		mTargetAnimInstance = Cast<UBoardActorAnimInstance>(GetTargetMeshComponent()->GetAnimInstance());
	}
	return mTargetAnimInstance.Get();
}

