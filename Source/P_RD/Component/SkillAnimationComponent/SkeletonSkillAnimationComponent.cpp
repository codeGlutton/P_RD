#include "Component/SkillAnimationComponent/SkeletonSkillAnimationComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "Singleton/WorldSubsystem/PresentationBarrier.h"

#include "Animation/BoardActorAnimInstance.h"

#include "Animation/SkillAnimationMetaData.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

void USkeletonSkillAnimationComponent::OnRegister()
{
	Super::OnRegister();

	if (mTargetMeshComponent != nullptr)
	{
		mTargetAnimInstance = Cast<UBoardActorAnimInstance>(mTargetMeshComponent->GetAnimInstance());
	}
}

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
		Context.mMontageEndEvent.AddLambda([SkillEndBarrier](const FBoardActorAnimationContext& Context, UAnimMontage* EndAnim, bool IsInterrupted) {
			});
	}

	AnimInstance->PlayMontageUsingTag(Context);
}

void USkeletonSkillAnimationComponent::SpawnHitVFX(TSharedPtr<FPresentationBarrier> SkillEndBarrier, const TArray<FApplyNiagaraSpawnData>& NiagaraSpawnDatas, ETileActorDirection LocalDirection) const
{
	// 히트한 방향 Transform
	const float LocalYaw = StaticCast<uint8>(LocalDirection) * 90.f;
	const FTransform LocalDirectionTransform(FRotator(0.f, LocalYaw, 0.f).Quaternion());

	for (const FApplyNiagaraSpawnData& NiagaraSpawnData : NiagaraSpawnDatas)
	{
		if (NiagaraSpawnData.mNiagaraSystem == nullptr)
		{
			return;
		}

		// 소켓의 컴포넌트 기준 Transform
		const FTransform SocketLocalTransform = GetTargetMeshComponent()->GetSocketTransform(NiagaraSpawnData.mSocketName, RTS_Component);

		// 소켓 Transform을 컴포넌트 축으로 절대 회전 방향으로 돌리고, 그 다음에 로컬 트랜스폼 적용
		const FTransform FinalLocalTransform = NiagaraSpawnData.mRelativeTransform * LocalDirectionTransform * SocketLocalTransform;
		// 월드 트랜스폼으로 변환
		const FTransform FinalWorldTransform = FinalLocalTransform * GetTargetMeshComponent()->GetComponentTransform();

		// 나이아가라 이펙트 소환
		UNiagaraComponent* NiagaraComponent = nullptr;
		if (NiagaraSpawnData.mAttached == true)
		{
			NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
				NiagaraSpawnData.mNiagaraSystem,
				GetTargetMeshComponent(),
				NAME_None,
				FinalLocalTransform.GetLocation(),
				FinalLocalTransform.Rotator(),
				FinalLocalTransform.GetScale3D(),
				EAttachLocation::KeepRelativeOffset,
				true,
				ENCPoolMethod::None
			);
		}
		else
		{
			NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(),
				NiagaraSpawnData.mNiagaraSystem,
				FinalWorldTransform.GetLocation(),
				FinalWorldTransform.Rotator(),
				FinalWorldTransform.GetScale3D()
			);
		}
	}
}

void USkeletonSkillAnimationComponent::SetTargetMeshComponent(USkeletalMeshComponent* TargetMesh)
{
	mTargetMeshComponent = TargetMesh;
	mTargetAnimInstance.Reset();

	if (TargetMesh != nullptr)
	{
		mTargetAnimInstance = Cast<UBoardActorAnimInstance>(TargetMesh->GetAnimInstance());
	}
}

USkeletalMeshComponent* USkeletonSkillAnimationComponent::GetTargetMeshComponent() const
{
	return mTargetMeshComponent.Get();
}

UBoardActorAnimInstance* USkeletonSkillAnimationComponent::GetTargetAnimInstance() const
{
	return mTargetAnimInstance.Get();
}

