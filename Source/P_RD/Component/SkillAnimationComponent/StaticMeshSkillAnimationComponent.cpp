#include "Component/SkillAnimationComponent/StaticMeshSkillAnimationComponent.h"
#include "Components/StaticMeshComponent.h"

#include "Actor/BoardActor/BoardCombatTargetView.h"

#include "Singleton/WorldSubsystem/PresentationBarrier.h"

void UStaticMeshSkillAnimationComponent::PlayApplyAnimation(const FBoardActorAnimationContext& Context)
{
	Super::PlayApplyAnimation(Context);
}

void UStaticMeshSkillAnimationComponent::PlayHitAnimation(TSharedPtr<FPresentationBarrier> SkillEndBarrier, FGameplayTag MontageTag, ETileActorDirection MontageDir) const
{
	Super::PlayHitAnimation(SkillEndBarrier, MontageTag, MontageDir);
}

UStaticMeshComponent* UStaticMeshSkillAnimationComponent::GetTargetMeshComponent() const
{
	if (mTargetMeshComponent.IsValid() == false)
	{
		mTargetMeshComponent = Cast<UStaticMeshComponent>(GetOwner<IBoardCombatTargetView>()->GetTargetMeshComponent());
	}
	return mTargetMeshComponent.Get();
}
