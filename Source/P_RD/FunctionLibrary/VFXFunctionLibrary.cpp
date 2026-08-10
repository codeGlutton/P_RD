#include "FunctionLibrary/VFXFunctionLibrary.h"
#include "Components/PrimitiveComponent.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Component/VFXTimelineComponent/VFXTimelineComponent.h"

UNiagaraComponent* UVFXFunctionLibrary::SpawnNiagaraEffect(const TSoftObjectPtr<UNiagaraSystem>& NiagaraSystem, UObject* WorldContextObject, const FTransform& Transform)
{
	return SpawnNiagaraEffect(TObjectPtr<UNiagaraSystem>(NiagaraSystem.LoadSynchronous()), WorldContextObject, Transform);
}

UNiagaraComponent* UVFXFunctionLibrary::SpawnNiagaraEffect(const TObjectPtr<UNiagaraSystem>& NiagaraSystem, UObject* WorldContextObject, const FTransform& Transform)
{
	if (WorldContextObject == nullptr || NiagaraSystem == nullptr)
	{
		return nullptr;
	}

	// 나이아가라 이펙트 소환
	UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		WorldContextObject,
		NiagaraSystem,
		Transform.GetLocation(),
		Transform.Rotator(),
		Transform.GetScale3D()
	);

	return NiagaraComponent;
}

UNiagaraComponent* UVFXFunctionLibrary::SpawnNiagaraEffect(const FSoftNiagaraSpawnData& NiagaraSpawnData, const AActor* TargetActor)
{
	return SpawnNiagaraEffectWithDirection(NiagaraSpawnData, TargetActor, ETileActorDirection::Forward);
}

UNiagaraComponent* UVFXFunctionLibrary::SpawnNiagaraEffect(const FNiagaraSpawnData& NiagaraSpawnData, const AActor* TargetActor)
{
	return SpawnNiagaraEffectWithDirection(NiagaraSpawnData, TargetActor, ETileActorDirection::Forward);
}

UNiagaraComponent* UVFXFunctionLibrary::SpawnNiagaraEffect(const FSoftNiagaraSpawnData& NiagaraSpawnData, UPrimitiveComponent* TargetComponent)
{
	return SpawnNiagaraEffectWithDirection(NiagaraSpawnData, TargetComponent, ETileActorDirection::Forward);
}

UNiagaraComponent* UVFXFunctionLibrary::SpawnNiagaraEffect(const FNiagaraSpawnData& NiagaraSpawnData, UPrimitiveComponent* TargetComponent)
{
	return SpawnNiagaraEffectWithDirection(NiagaraSpawnData, TargetComponent, ETileActorDirection::Forward);
}

UNiagaraComponent* UVFXFunctionLibrary::SpawnNiagaraEffectWithDirection(const FSoftNiagaraSpawnData& NiagaraSpawnData, const AActor* TargetActor, ETileActorDirection Direction)
{
	return SpawnNiagaraEffectWithDirection_Internal(NiagaraSpawnData.mNiagaraSystem.LoadSynchronous(), TargetActor, nullptr, NiagaraSpawnData.mSocketName, NiagaraSpawnData.mRelativeTransform, NiagaraSpawnData.mAttached, Direction);
}

UNiagaraComponent* UVFXFunctionLibrary::SpawnNiagaraEffectWithDirection(const FNiagaraSpawnData& NiagaraSpawnData, const AActor* TargetActor, ETileActorDirection Direction)
{
	return SpawnNiagaraEffectWithDirection_Internal(NiagaraSpawnData.mNiagaraSystem, TargetActor, nullptr, NiagaraSpawnData.mSocketName, NiagaraSpawnData.mRelativeTransform, NiagaraSpawnData.mAttached, Direction);
}

UNiagaraComponent* UVFXFunctionLibrary::SpawnNiagaraEffectWithDirection(const FSoftNiagaraSpawnData& NiagaraSpawnData, UPrimitiveComponent* TargetComponent, ETileActorDirection Direction)
{
	if (TargetComponent == nullptr)
	{
		return nullptr;
	}
	return SpawnNiagaraEffectWithDirection_Internal(NiagaraSpawnData.mNiagaraSystem.LoadSynchronous(), TargetComponent->GetOwner(), TargetComponent, NiagaraSpawnData.mSocketName, NiagaraSpawnData.mRelativeTransform, NiagaraSpawnData.mAttached, Direction);
}

UNiagaraComponent* UVFXFunctionLibrary::SpawnNiagaraEffectWithDirection(const FNiagaraSpawnData& NiagaraSpawnData, UPrimitiveComponent* TargetComponent, ETileActorDirection Direction)
{
	if (TargetComponent == nullptr)
	{
		return nullptr;
	}
	return SpawnNiagaraEffectWithDirection_Internal(NiagaraSpawnData.mNiagaraSystem, TargetComponent->GetOwner(), TargetComponent, NiagaraSpawnData.mSocketName, NiagaraSpawnData.mRelativeTransform, NiagaraSpawnData.mAttached, Direction);
}

bool UVFXFunctionLibrary::ExecuteVFXTimeline(UVFXTimelineComponent* TimelineComp, const FVFXTimelineExecutionData& TimelineExecutionData, FVFXTimelineEventTarget EventTarget)
{
	if (TimelineComp != nullptr)
	{
		if (TimelineExecutionData.mDirection == ETimelineDirection::Forward)
		{
			if (TimelineExecutionData.mIsPlayFromStart == true)
			{
				return TimelineComp->PlayFromStart(TimelineExecutionData.mKeyName, EventTarget);
			}
			else
			{
				return TimelineComp->Play(TimelineExecutionData.mKeyName, EventTarget);
			}
		}
		else
		{
			if (TimelineExecutionData.mIsPlayFromStart == true)
			{
				return TimelineComp->ReverseFromEnd(TimelineExecutionData.mKeyName, EventTarget);
			}
			else
			{
				return TimelineComp->Reverse(TimelineExecutionData.mKeyName, EventTarget);
			}
		}
	}
	return false;
}

bool UVFXFunctionLibrary::ExecuteVFXTimeline(UVFXTimelineComponent* TimelineComp, const FVFXTimelineExecutionData& TimelineExecutionData, TArray<TWeakObjectPtr<UPrimitiveComponent>>& TargetMeshes, TArray<TWeakObjectPtr<UNiagaraComponent>>& TargetNiagaras)
{
	FVFXTimelineEventTarget EventTarget;
	EventTarget.mMeshComps = TargetMeshes;
	EventTarget.mNiagaraComps = TargetNiagaras;

	return ExecuteVFXTimeline(TimelineComp, TimelineExecutionData, EventTarget);
}

TArray<TObjectPtr<UNiagaraComponent>> UVFXFunctionLibrary::SpawnAndExecuteVFX(const FVFXSpawnData& VFXSpawnData, const AActor* TargetActor, UVFXTimelineComponent* TimelineComp, TArray<TWeakObjectPtr<UPrimitiveComponent>>& TargetMeshes, TArray<TWeakObjectPtr<UNiagaraComponent>>& TargetNiagaras, ETileActorDirection Direction)
{
	FVFXTimelineEventTarget EventTarget;
	EventTarget.mMeshComps = TargetMeshes;
	EventTarget.mNiagaraComps = TargetNiagaras;

	return SpawnAndExecuteVFX(VFXSpawnData, TargetActor, TimelineComp, EventTarget, Direction);
}

TArray<TObjectPtr<UNiagaraComponent>> UVFXFunctionLibrary::SpawnAndExecuteVFX(const FSoftVFXSpawnData& VFXSpawnData, const AActor* TargetActor, UVFXTimelineComponent* TimelineComp, TArray<TWeakObjectPtr<UPrimitiveComponent>>& TargetMeshes, TArray<TWeakObjectPtr<UNiagaraComponent>>& TargetNiagaras, ETileActorDirection Direction)
{
	return SpawnAndExecuteVFX(VFXSpawnData.LoadSynchronous(), TargetActor, TimelineComp, TargetMeshes, TargetNiagaras, Direction);
}

TArray<TObjectPtr<UNiagaraComponent>> UVFXFunctionLibrary::SpawnAndExecuteVFX(const FVFXSpawnData& VFXSpawnData, UPrimitiveComponent* TargetComponent, UVFXTimelineComponent* TimelineComp, TArray<TWeakObjectPtr<UPrimitiveComponent>>& TargetMeshes, TArray<TWeakObjectPtr<UNiagaraComponent>>& TargetNiagaras, ETileActorDirection Direction)
{
	FVFXTimelineEventTarget EventTarget;
	EventTarget.mMeshComps = TargetMeshes;
	EventTarget.mNiagaraComps = TargetNiagaras;

	return SpawnAndExecuteVFX(VFXSpawnData, TargetComponent, TimelineComp, EventTarget, Direction);
}

TArray<TObjectPtr<UNiagaraComponent>> UVFXFunctionLibrary::SpawnAndExecuteVFX(const FSoftVFXSpawnData& VFXSpawnData, UPrimitiveComponent* TargetComponent, UVFXTimelineComponent* TimelineComp, TArray<TWeakObjectPtr<UPrimitiveComponent>>& TargetMeshes, TArray<TWeakObjectPtr<UNiagaraComponent>>& TargetNiagaras, ETileActorDirection Direction)
{
	return SpawnAndExecuteVFX(VFXSpawnData.LoadSynchronous(), TargetComponent, TimelineComp, TargetMeshes, TargetNiagaras, Direction);
}

TArray<TObjectPtr<UNiagaraComponent>> UVFXFunctionLibrary::SpawnAndExecuteVFX(const FVFXSpawnData& VFXSpawnData, const AActor* TargetActor, UVFXTimelineComponent* TimelineComp, FVFXTimelineEventTarget EventTarget, ETileActorDirection Direction)
{
	TArray<TObjectPtr<UNiagaraComponent>> SpawnedNiagaras;
	if (TargetActor == nullptr)
	{
		return SpawnedNiagaras;
	}

	/* 나이아가라 이펙트 스폰 */
	SpawnedNiagaras.Reserve(VFXSpawnData.mNiagaraSpawnDatas.Num());
	for (const FNiagaraSpawnData& NiagaraSpawnData : VFXSpawnData.mNiagaraSpawnDatas)
	{
		UNiagaraComponent* NiagaraComp = SpawnNiagaraEffectWithDirection(NiagaraSpawnData, TargetActor, Direction);
		if (NiagaraComp != nullptr)
		{
			SpawnedNiagaras.Add(NiagaraComp);
		}
	}

	/* 타임라인 타겟 설정 및 나이아가라 컴포넌트 추가 옵션 반영 */
	if (VFXSpawnData.mIncludeSpawnedNiagaraInTimeline == true)
	{
		for (UNiagaraComponent* SpawnedNiagara : SpawnedNiagaras)
		{
			if (SpawnedNiagara != nullptr)
			{
				EventTarget.mNiagaraComps.Add(SpawnedNiagara);
			}
		}
	}

	/* 타임라인 실행 */
	if (TimelineComp != nullptr)
	{
		for (const FVFXTimelineExecutionData& TimelineExecutionData : VFXSpawnData.mTimelineExecutionDatas)
		{
			ExecuteVFXTimeline(TimelineComp, TimelineExecutionData, EventTarget);
		}
	}

	return SpawnedNiagaras;
}

TArray<TObjectPtr<UNiagaraComponent>> UVFXFunctionLibrary::SpawnAndExecuteVFX(const FSoftVFXSpawnData& VFXSpawnData, const AActor* TargetActor, UVFXTimelineComponent* TimelineComp, FVFXTimelineEventTarget EventTarget, ETileActorDirection Direction)
{
	return SpawnAndExecuteVFX(VFXSpawnData.LoadSynchronous(), TargetActor, TimelineComp, EventTarget, Direction);
}

TArray<TObjectPtr<UNiagaraComponent>> UVFXFunctionLibrary::SpawnAndExecuteVFX(const FVFXSpawnData& VFXSpawnData, UPrimitiveComponent* TargetComponent, UVFXTimelineComponent* TimelineComp, FVFXTimelineEventTarget EventTarget, ETileActorDirection Direction)
{
	TArray<TObjectPtr<UNiagaraComponent>> SpawnedNiagaras;
	if (TargetComponent == nullptr)
	{
		return SpawnedNiagaras;
	}

	/* 나이아가라 이펙트 스폰 */
	SpawnedNiagaras.Reserve(VFXSpawnData.mNiagaraSpawnDatas.Num());
	for (const FNiagaraSpawnData& NiagaraSpawnData : VFXSpawnData.mNiagaraSpawnDatas)
	{
		UNiagaraComponent* NiagaraComp = SpawnNiagaraEffectWithDirection(NiagaraSpawnData, TargetComponent, Direction);
		if (NiagaraComp != nullptr)
		{
			SpawnedNiagaras.Add(NiagaraComp);
		}
	}

	/* 타임라인 타겟 설정 및 나이아가라 컴포넌트 추가 옵션 반영 */
	if (VFXSpawnData.mIncludeSpawnedNiagaraInTimeline == true)
	{
		for (UNiagaraComponent* SpawnedNiagara : SpawnedNiagaras)
		{
			if (SpawnedNiagara != nullptr)
			{
				EventTarget.mNiagaraComps.Add(SpawnedNiagara);
			}
		}
	}

	/* 타임라인 실행 */
	if (TimelineComp != nullptr)
	{
		for (const FVFXTimelineExecutionData& TimelineExecutionData : VFXSpawnData.mTimelineExecutionDatas)
		{
			ExecuteVFXTimeline(TimelineComp, TimelineExecutionData, EventTarget);
		}
	}

	return SpawnedNiagaras;
}

TArray<TObjectPtr<UNiagaraComponent>> UVFXFunctionLibrary::SpawnAndExecuteVFX(const FSoftVFXSpawnData& VFXSpawnData, UPrimitiveComponent* TargetComponent, UVFXTimelineComponent* TimelineComp, FVFXTimelineEventTarget EventTarget, ETileActorDirection Direction)
{
	return SpawnAndExecuteVFX(VFXSpawnData.LoadSynchronous(), TargetComponent, TimelineComp, EventTarget, Direction);
}

UNiagaraComponent* UVFXFunctionLibrary::SpawnNiagaraEffectWithDirection_Internal(const TObjectPtr<UNiagaraSystem>& NiagaraSystem, const AActor* TargetActor, UPrimitiveComponent* TargetComponent, const FName& SocketName, const FTransform& RelativeTransform, bool Attached, ETileActorDirection Direction)
{
	if (TargetActor == nullptr || NiagaraSystem == nullptr)
	{
		return nullptr;
	}

	// 대상 확정
	USceneComponent* FinalTargetComponent = TargetActor->GetRootComponent();
	if (TargetComponent != nullptr)
	{
		FinalTargetComponent = TargetComponent;
	}

	// 대상 트랜스폼 수집
	const FTransform FinalTargetTransform = FinalTargetComponent->GetComponentTransform();
	const FTransform SocketLocalTransform = FinalTargetComponent->GetSocketTransform(SocketName, RTS_Component);

	// 히트한 방향 Transform
	const float LocalYaw = StaticCast<uint8>(Direction) * 90.f;
	const FTransform LocalDirectionTransform(FRotator(0.f, LocalYaw, 0.f).Quaternion());

	// 소켓 Transform을 컴포넌트 축으로 절대 회전 방향으로 돌리고, 그 다음에 로컬 트랜스폼 적용
	const FTransform FinalLocalTransform = RelativeTransform * LocalDirectionTransform * SocketLocalTransform;
	// 월드 트랜스폼으로 변환
	const FTransform FinalWorldTransform = FinalLocalTransform * FinalTargetTransform;

	// 나이아가라 이펙트 소환
	UNiagaraComponent* NiagaraComponent = nullptr;
	if (Attached == true)
	{
		NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			NiagaraSystem,
			FinalTargetComponent,
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
			FinalTargetComponent->GetWorld(),
			NiagaraSystem,
			FinalWorldTransform.GetLocation(),
			FinalWorldTransform.Rotator(),
			FinalWorldTransform.GetScale3D()
		);
	}

	return NiagaraComponent;
}

