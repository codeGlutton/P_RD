#include "Component/SkillAnimationComponent/SkillAnimationComponent.h"
#include "Actor/TileMap/TileMapModel.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"

#include "ObjectView.h"
#include "Actor/BoardActor/BoardCombatTargetView.h"

#include "Animation/BoardActorAnimInstance.h"
#include "Singleton/WorldSubsystem/PresentationBarrier.h"

#include "Component/SkillComponent/SkillComponentModel.h"
#include "Animation/SkillAnimationMetaData.h"

#include "Pawn/Camera/CombatCameraPawn.h"
#include "Component/CameraMovementComponent/CameraMovementComponent.h"
#include "Component/TimeScaleComponent/TimeScaleComponent.h"

#include "FunctionLibrary/VFXFunctionLibrary.h"

USkillAnimationComponent::USkillAnimationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USkillAnimationComponent::BindOwnerModel(UObjectModel* Model)
{
	UBoardActorModel* BoardActorModel = Cast<UBoardActorModel>(Model);
	if (BoardActorModel == nullptr)
	{
		return;
	}

	mOwnerModel = BoardActorModel;

	/* 애니메이션 연출 요청 대리자 구독 */

	if (mOwnerModel.IsValid() == true)
	{
		mOwnerModel->OnPlayAnimationUI.AddUObject(this, &USkillAnimationComponent::HandleToPlayAnimation);
	}
}

void USkillAnimationComponent::UnbindOwnerModel(UObjectModel* Model)
{
	/* 애니메이션 연출 요청 대리자 구독 해제 */

	if (mOwnerModel.IsValid() == true)
	{
		mOwnerModel->OnPlayAnimationUI.RemoveAll(this);
		mOwnerModel.Reset();
	}
}

void USkillAnimationComponent::HandleToPlayAnimation(TSharedPtr<FPresentationBarrier> MotionEndBarrier, const FBoardActorAnimationContext& Context)
{
	FBoardActorAnimationContext PlayAnimContext = Context;

	/* 연출 대기 */
	{
		PlayAnimContext.mMontageEndEvent.AddLambda([MotionEndBarrier](const FBoardActorAnimationContext& Context, UAnimMontage* EndAnim, bool IsInterrupted) {
			});
	}

	/* 타격 애님 노티파이 수신 */
	{
		FBoardActorAnimationEvent HitAnimEvent;
		HitAnimEvent.mIsOneTimeEvent = false;
		HitAnimEvent.OnTriggerAnimationEvent.AddUObject(this, &USkillAnimationComponent::OnHandleHitAnimationEvent);
		PlayAnimContext.mMontageEvents.Add(AnimationTags::Animation_Event_Skill_HitAnimation, HitAnimEvent);
	}

	/* 타격 VFX 노티파이 수신 */
	{
		FBoardActorAnimationEvent HitVFXEvent;
		HitVFXEvent.mIsOneTimeEvent = false;
		HitVFXEvent.OnTriggerAnimationEvent.AddUObject(this, &USkillAnimationComponent::OnHandleHitVFXEvent);
		PlayAnimContext.mMontageEvents.Add(AnimationTags::Animation_Event_Skill_HitVFX, HitVFXEvent);
	}

	/* 카메라 쉐이크 노티파이 수신 */
	{
		FBoardActorAnimationEvent CameraShakeEvent;
		CameraShakeEvent.mIsOneTimeEvent = false;
		CameraShakeEvent.OnTriggerAnimationEvent.AddUObject(this, &USkillAnimationComponent::OnHandleCameraShakeEvent);
		PlayAnimContext.mMontageEvents.Add(AnimationTags::Animation_Event_Skill_CameraShake, CameraShakeEvent);
	}

	/* 카메라 줌 노티파이 스테이트 수신 */
	{
		FBoardActorAnimationEvent CameraZoomEvent;
		CameraZoomEvent.mIsOneTimeEvent = false;
		CameraZoomEvent.OnTriggerAnimationEvent.AddUObject(this, &USkillAnimationComponent::OnHandleCameraZoomEvent);
		PlayAnimContext.mMontageEvents.Add(AnimationTags::Animation_Event_Skill_CameraZoom, CameraZoomEvent);
	}

	/* 시간 조정 노티파이 수신 */
	{
		FBoardActorAnimationEvent TimeScaleEvent;
		TimeScaleEvent.mIsOneTimeEvent = false;
		TimeScaleEvent.OnTriggerAnimationEvent.AddUObject(this, &USkillAnimationComponent::OnHandleTimeScaleEvent);
		PlayAnimContext.mMontageEvents.Add(AnimationTags::Animation_Event_Skill_TimeScale, TimeScaleEvent);
	}

	PlayApplyAnimation(PlayAnimContext);
}

void USkillAnimationComponent::OnHandleHitAnimationEvent(const FBoardActorAnimationContext& Context, UAnimMontage* EndAnim, const FEventTriggerPayloadBase* Payload)
{
	const FApplyAnimationEventTriggerPayload* AnimationPayload = StaticCast<const FApplyAnimationEventTriggerPayload*>(Payload);

	const USkillComponentModel* OwnerSkillComp = Context.mMetaData.Get<FSkillAnimationMetaData>().mInstigator.Get();
	if (OwnerSkillComp == nullptr)
	{
		return;
	}

	const UBoardActorModel* OwnerActorModel = OwnerSkillComp->GetOwnerModel<UBoardActorModel>();
	const FActiveSkillContext& ActiveSkillContext = OwnerSkillComp->GetActiveSkillContext();
	for (IBoardCombatTarget* OtherCombatTarget : ActiveSkillContext.mFinalCombatTargets)
	{
		UBoardActorModel* OtherActorModel = Cast<UBoardActorModel>(OtherCombatTarget);
		if (OtherActorModel == nullptr || OwnerActorModel == OtherActorModel)
		{
			continue;
		}

		IBoardCombatTargetView* OtherActorView = OtherActorModel->GetView<IBoardCombatTargetView>();
		if (OtherActorView == nullptr)
		{
			continue;
		}

		const ETileActorDirection OtherAnimationDir = ConvertOtherLocalTileMapDirection(
			Context.mMontageDir, 
			OwnerActorModel->GetTileTransform().mDirection, 
			OtherActorModel->GetTileTransform().mDirection
		);
		const USkillAnimationComponent* OtherSkillAnimComp = OtherActorView->GetSkillAnimationComponent();
		OtherSkillAnimComp->PlayHitAnimation(ActiveSkillContext.mSkillEndBarrier, AnimationPayload->mAnimationTag, OtherAnimationDir);
	}
}

void USkillAnimationComponent::OnHandleHitVFXEvent(const FBoardActorAnimationContext& Context, UAnimMontage* EndAnim, const FEventTriggerPayloadBase* Payload)
{
	const FApplyNiagaraEventTriggerPayload* NiagaraPayload = StaticCast<const FApplyNiagaraEventTriggerPayload*>(Payload);

	const USkillComponentModel* OwnerSkillComp = Context.mMetaData.Get<FSkillAnimationMetaData>().mInstigator.Get();
	if (OwnerSkillComp == nullptr)
	{
		return;
	}

	const UBoardActorModel* OwnerActorModel = OwnerSkillComp->GetOwnerModel<UBoardActorModel>();
	const FActiveSkillContext& ActiveSkillContext = OwnerSkillComp->GetActiveSkillContext();

	switch (NiagaraPayload->mTargetType)
	{
	case EApplyNiagaraTargetType::Actor:
	{
		for (IBoardCombatTarget* OtherCombatTarget : ActiveSkillContext.mFinalCombatTargets)
		{
			UBoardActorModel* OtherActorModel = Cast<UBoardActorModel>(OtherCombatTarget);
			if (OtherActorModel == nullptr)
			{
				continue;
			}

			IBoardCombatTargetView* OtherActorView = OtherActorModel->GetView<IBoardCombatTargetView>();
			if (OtherActorView == nullptr)
			{
				continue;
			}

			const ETileActorDirection OtherVFXDir = ConvertOtherLocalTileMapDirection(
				Context.mMontageDir,
				OwnerActorModel->GetTileTransform().mDirection,
				OtherActorModel->GetTileTransform().mDirection
			);
			const USkillAnimationComponent* OtherSkillAnimComp = OtherActorView->GetSkillAnimationComponent();
			OtherSkillAnimComp->SpawnHitVFXOnSelf(ActiveSkillContext.mSkillEndBarrier, NiagaraPayload->mNiagaraSpawnDatas, OtherVFXDir);
		}
		break;
	}
	case EApplyNiagaraTargetType::EffectTile:
	{
		for (const FTileIndex& EffectTileIndex : ActiveSkillContext.mEffectTileIndexes)
		{
			const FTileTransform EffectTileTransform(EffectTileIndex, Context.mMontageDir);
			const FTransform EffectTransform = ActiveSkillContext.mMapModel->TileToWorldTransform(EffectTileTransform);
			SpawnHitVFXOnTile(ActiveSkillContext.mSkillEndBarrier, NiagaraPayload->mNiagaraSpawnDatas, EffectTransform);
		}
		break;
	}
	case EApplyNiagaraTargetType::TargetTile:
	{
		for (const FTileIndex& TargetTileIndex : ActiveSkillContext.mTargetTileIndexes)
		{
			const FTileTransform TargetTileTransform(TargetTileIndex, Context.mMontageDir);
			const FTransform TargetTransform = ActiveSkillContext.mMapModel->TileToWorldTransform(TargetTileTransform);
			SpawnHitVFXOnTile(ActiveSkillContext.mSkillEndBarrier, NiagaraPayload->mNiagaraSpawnDatas, TargetTransform);
		}
		break;
	}
	}
}

void USkillAnimationComponent::OnHandleCameraShakeEvent(const FBoardActorAnimationContext& Context, UAnimMontage* EndAnim, const FEventTriggerPayloadBase* Payload)
{
	const FCameraShakeEventTriggerPayload* CameraShakePayload = StaticCast<const FCameraShakeEventTriggerPayload*>(Payload);

	ShakeCamera(CameraShakePayload->mCameraShakeClass);
}

void USkillAnimationComponent::OnHandleCameraZoomEvent(const FBoardActorAnimationContext& Context, UAnimMontage* EndAnim, const FEventTriggerPayloadBase* Payload)
{
	const FCameraZoomEventTriggerPayload* CameraZoomPayload = StaticCast<const FCameraZoomEventTriggerPayload*>(Payload);

	const USkillComponentModel* OwnerSkillComp = Context.mMetaData.Get<FSkillAnimationMetaData>().mInstigator.Get();
	if (OwnerSkillComp == nullptr)
	{
		return;
	}

	const UBoardActorModel* OwnerActorModel = OwnerSkillComp->GetOwnerModel<UBoardActorModel>();
	const FActiveSkillContext& ActiveSkillContext = OwnerSkillComp->GetActiveSkillContext();
	
	bool IsZoomTargetSelfActor = false;
	bool ContainSelfTileLocation = false;
	bool ContainTargetTileLocations = false;

	const FCameraZoomEventData& ZoomEventData = CameraZoomPayload->mCameraZoomEventData;
	switch (ZoomEventData.mTargetType)
	{
	case ECameraZoomTargetType::SelfActor:
	{
		IsZoomTargetSelfActor = true;
		ContainSelfTileLocation = true;
		ContainTargetTileLocations = false;
		break;
	}
	case ECameraZoomTargetType::SelfLocation:
	{
		IsZoomTargetSelfActor = false;
		ContainSelfTileLocation = true;
		ContainTargetTileLocations = false;
		break;
	}
	case ECameraZoomTargetType::TargetLocation:
	{
		IsZoomTargetSelfActor = false;
		ContainSelfTileLocation = false;
		ContainTargetTileLocations = true;
		break;
	}
	case ECameraZoomTargetType::AllLocation:
	{
		IsZoomTargetSelfActor = false;
		ContainSelfTileLocation = true;
		ContainTargetTileLocations = true;
		break;
	}
	}

	TArray<FVector> CollectedLocations;
	if (ContainSelfTileLocation == true)
	{
		CollectedLocations.Add(ActiveSkillContext.mMapModel->TileToWorldLocation(ActiveSkillContext.mSelfTileIndex));
	}
	if (ContainTargetTileLocations == true)
	{
		for (const FTileIndex& EffectTileIndex : ActiveSkillContext.mEffectTileIndexes)
		{
			CollectedLocations.Add(ActiveSkillContext.mMapModel->TileToWorldLocation(EffectTileIndex));
		}
	}

	float FinalZoomScale = 0.f;
	FVector FinalZoomLocation = FVector::ZeroVector;
	ZoomEventData.CalculateCameraZoomScaleAndLocation(CollectedLocations, OUT FinalZoomScale, OUT FinalZoomLocation);

	if (IsZoomTargetSelfActor == true)
	{
		ZoomInCamera(CameraZoomPayload->OnEndDurationEventTrigger, FinalZoomScale, GetOwner());
	}
	else
	{
		ZoomInCamera(CameraZoomPayload->OnEndDurationEventTrigger, FinalZoomScale, GetOwner());
	}
}

void USkillAnimationComponent::OnHandleTimeScaleEvent(const FBoardActorAnimationContext& Context, UAnimMontage* EndAnim, const FEventTriggerPayloadBase* Payload)
{
	const FTimeScaleEventTriggerPayload* TimeScalePayload = StaticCast<const FTimeScaleEventTriggerPayload*>(Payload);

	RequestTimeScale(TimeScalePayload->OnEndDurationEventTrigger, this, TimeScalePayload->mTimeScale, TimeScalePayload->mBlendSpeed, -1);
}

void USkillAnimationComponent::PlayApplyAnimation(const FBoardActorAnimationContext& Context)
{
}

void USkillAnimationComponent::PlayHitAnimation(TSharedPtr<FPresentationBarrier> SkillEndBarrier, FGameplayTag MontageTag, ETileActorDirection MontageDir) const
{
}

void USkillAnimationComponent::SpawnHitVFXOnSelf(TSharedPtr<FPresentationBarrier> SkillEndBarrier, const TArray<FNiagaraSpawnData>& NiagaraSpawnDatas, ETileActorDirection LocalDirection) const
{
	// 자기 자신
	UPrimitiveComponent* TargetMeshComponent = GetOwner<IBoardCombatTargetView>()->GetTargetMeshComponent();
	if (TargetMeshComponent == nullptr)
	{
		return;
	}

	for (const FNiagaraSpawnData& NiagaraSpawnData : NiagaraSpawnDatas)
	{
		UVFXFunctionLibrary::SpawnNiagaraEffectWithDirection(NiagaraSpawnData, TargetMeshComponent, LocalDirection);
	}
}

void USkillAnimationComponent::SpawnHitVFXOnTile(TSharedPtr<FPresentationBarrier> SkillEndBarrier, const TArray<FNiagaraSpawnData>& NiagaraSpawnDatas, const FTransform& Transform) const
{
	for (const FNiagaraSpawnData& NiagaraSpawnData : NiagaraSpawnDatas)
	{
		UVFXFunctionLibrary::SpawnNiagaraEffect(NiagaraSpawnData.mNiagaraSystem, GetWorld(), Transform);
	}
}

void USkillAnimationComponent::ZoomInCamera(FOnEndDurationEventTrigger& EndEvent, float TargetZoom, FVector WorldPosition) const
{
	ACombatCameraPawn* CameraPawn = GetWorld()->GetFirstPlayerController()->GetPawn<ACombatCameraPawn>();
	if (CameraPawn != nullptr)
	{
		CameraPawn->GetCameraMovementComponent()->StartEmphasisToWorldPositionWithZoom(TargetZoom, WorldPosition);
		EndEvent.BindWeakLambda(CameraPawn, [CameraPawn]() {
			CameraPawn->GetCameraMovementComponent()->EndEmphasis();
			});
	}
}

void USkillAnimationComponent::ZoomInCamera(FOnEndDurationEventTrigger& EndEvent, float TargetZoom, AActor* EmphasisActor) const
{
	ACombatCameraPawn* CameraPawn = GetWorld()->GetFirstPlayerController()->GetPawn<ACombatCameraPawn>();
	if (CameraPawn != nullptr)
	{
		CameraPawn->GetCameraMovementComponent()->StartEmphasisToActorWithZoom(TargetZoom, EmphasisActor);
		EndEvent.BindWeakLambda(CameraPawn, [CameraPawn]() {
			CameraPawn->GetCameraMovementComponent()->EndEmphasis();
			});
	}
}

void USkillAnimationComponent::ShakeCamera(TSubclassOf<UCameraShakeBase> CameraShakeClass) const
{
	ACombatCameraPawn* CameraPawn = GetWorld()->GetFirstPlayerController()->GetPawn<ACombatCameraPawn>();
	if (CameraPawn != nullptr)
	{
		CameraPawn->GetCameraMovementComponent()->StartCameraShake(CameraShakeClass);
	}
}

void USkillAnimationComponent::RequestTimeScale(FOnEndDurationEventTrigger& EndEvent, UObject* Requester, float TargetTimeScale, float BlendSpeed, float Duration) const
{
	ACombatCameraPawn* CameraPawn = GetWorld()->GetFirstPlayerController()->GetPawn<ACombatCameraPawn>();
	if (CameraPawn != nullptr)
	{
		FTimeScaleHandle Handle = CameraPawn->GetTimeScaleComponent()->RequestTimeScale(Requester, TargetTimeScale, BlendSpeed, Duration);
		EndEvent.BindWeakLambda(CameraPawn, [CameraPawn, Handle]() {
			CameraPawn->GetTimeScaleComponent()->ReleaseTimeScale(Handle);
			});
	}
}

