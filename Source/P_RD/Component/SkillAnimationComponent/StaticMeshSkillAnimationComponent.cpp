#include "Component/SkillAnimationComponent/StaticMeshSkillAnimationComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Animation/BoardActorSequencePlayer.h"

#include "Actor/BoardActor/BoardCombatTargetView.h"

#include "Singleton/WorldSubsystem/PresentationBarrier.h"

UStaticMeshSkillAnimationComponent::UStaticMeshSkillAnimationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UStaticMeshSkillAnimationComponent::OnRegister()
{
	Super::OnRegister();

	if (mSequencePlayerClass != nullptr)
	{
		mSequencePlayer = NewObject<UBoardActorSequencePlayer>(this, mSequencePlayerClass);
	}
}

void UStaticMeshSkillAnimationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (mSequencePlayer != nullptr)
	{
		mSequencePlayer->NativeUpdateSequence(DeltaTime);
	}
}

void UStaticMeshSkillAnimationComponent::PlayApplyAnimation(const FBoardActorAnimationContext& Context)
{
	UBoardActorSequencePlayer* SequencePlayer = GetTargetSequencePlayer();
	if (SequencePlayer == nullptr)
	{
		return;
	}

	SequencePlayer->PlaySequenceUsingTag(Context);
}

void UStaticMeshSkillAnimationComponent::PlayHitAnimation(TSharedPtr<FPresentationBarrier> SkillEndBarrier, FGameplayTag MontageTag, ETileActorDirection MontageDir) const
{
	UBoardActorSequencePlayer* SequencePlayer = GetTargetSequencePlayer();
	if (SequencePlayer == nullptr)
	{
		return;
	}

	FBoardActorAnimationContext Context(MontageTag, MontageDir);

	/* 연출 대기 */
	{
		Context.mMontageEndEvent.AddLambda([SkillEndBarrier](const FBoardActorAnimationContext& Context, UObject* EndAnim, bool IsInterrupted) {
			});
	}

	SequencePlayer->PlaySequenceUsingTag(Context);
}

UBoardActorSequencePlayer* UStaticMeshSkillAnimationComponent::GetTargetSequencePlayer() const
{
	return mSequencePlayer;
}

UStaticMeshComponent* UStaticMeshSkillAnimationComponent::GetTargetMeshComponent() const
{
	if (mTargetMeshComponent.IsValid() == false)
	{
		mTargetMeshComponent = Cast<UStaticMeshComponent>(GetOwner<IBoardCombatTargetView>()->GetTargetMeshComponent());
	}
	return mTargetMeshComponent.Get();
}
