#include "FunctionLibrary/VFXFunctionLibrary.h"
#include "Components/PrimitiveComponent.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

UNiagaraComponent* UVFXFunctionLibrary::SpawnNiagaraEffect(const FSoftNiagaraSpawnData& NiagaraSpawnData, UPrimitiveComponent* TargetComponent)
{
	return SpawnNiagaraEffectWithDirection(NiagaraSpawnData, TargetComponent, ETileActorDirection::Forward);
}

UNiagaraComponent* UVFXFunctionLibrary::SpawnNiagaraEffect(const FNiagaraSpawnData& NiagaraSpawnData, UPrimitiveComponent* TargetComponent)
{
	return SpawnNiagaraEffectWithDirection(NiagaraSpawnData, TargetComponent, ETileActorDirection::Forward);
}

UNiagaraComponent* UVFXFunctionLibrary::SpawnNiagaraEffectWithDirection(const FSoftNiagaraSpawnData& NiagaraSpawnData, UPrimitiveComponent* TargetComponent, ETileActorDirection Direction)
{
	return SpawnNiagaraEffectWithDirection_Internal(NiagaraSpawnData.mNiagaraSystem.LoadSynchronous(), TargetComponent, NiagaraSpawnData.mSocketName, NiagaraSpawnData.mRelativeTransform, NiagaraSpawnData.mAttached, Direction);
}

UNiagaraComponent* UVFXFunctionLibrary::SpawnNiagaraEffectWithDirection(const FNiagaraSpawnData& NiagaraSpawnData, UPrimitiveComponent* TargetComponent, ETileActorDirection Direction)
{
	return SpawnNiagaraEffectWithDirection_Internal(NiagaraSpawnData.mNiagaraSystem, TargetComponent, NiagaraSpawnData.mSocketName, NiagaraSpawnData.mRelativeTransform, NiagaraSpawnData.mAttached, Direction);
}

UNiagaraComponent* UVFXFunctionLibrary::SpawnNiagaraEffectWithDirection_Internal(const TObjectPtr<UNiagaraSystem>& NiagaraSystem, UPrimitiveComponent* TargetComponent, const FName& SocketName, const FTransform& RelativeTransform, bool Attached, ETileActorDirection Direction)
{
	if (TargetComponent == nullptr || NiagaraSystem == nullptr)
	{
		return nullptr;
	}

	// 히트한 방향 Transform
	const float LocalYaw = StaticCast<uint8>(Direction) * 90.f;
	const FTransform LocalDirectionTransform(FRotator(0.f, LocalYaw, 0.f).Quaternion());

	// 소켓의 컴포넌트 기준 Transform
	const FTransform SocketLocalTransform = TargetComponent->GetSocketTransform(SocketName, RTS_Component);

	// 소켓 Transform을 컴포넌트 축으로 절대 회전 방향으로 돌리고, 그 다음에 로컬 트랜스폼 적용
	const FTransform FinalLocalTransform = RelativeTransform * LocalDirectionTransform * SocketLocalTransform;
	// 월드 트랜스폼으로 변환
	const FTransform FinalWorldTransform = FinalLocalTransform * TargetComponent->GetComponentTransform();

	// 나이아가라 이펙트 소환
	UNiagaraComponent* NiagaraComponent = nullptr;
	if (Attached == true)
	{
		NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			NiagaraSystem,
			TargetComponent,
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
			TargetComponent->GetWorld(),
			NiagaraSystem,
			FinalWorldTransform.GetLocation(),
			FinalWorldTransform.Rotator(),
			FinalWorldTransform.GetScale3D()
		);
	}

	return NiagaraComponent;
}
