/*****************************************************************//**
 * @file   VFXFunctionLibrary.h
 * @brief  VFX 스폰 관련 정적 헬퍼 함수 라이브러리 헤더
 * @author 모호재
 * @date   2026-08-03
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Setting/GamePlayType.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "VFXFunctionLibrary.generated.h"

class UNiagaraComponent;
class UPrimitiveComponent;

/**
 * @brief VFX 스폰 헬퍼 정적 함수 라이브러리
 */
UCLASS()
class P_RD_API UVFXFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * @brief FNiagaraSpawnData와 UPrimitiveComponent를 기반으로 나이아가라 VFX를 스폰합니다.
	 */
	static UNiagaraComponent* SpawnNiagaraEffect(const FSoftNiagaraSpawnData& NiagaraSpawnData, UPrimitiveComponent* TargetComponent);
	static UNiagaraComponent* SpawnNiagaraEffect(const FNiagaraSpawnData& NiagaraSpawnData, UPrimitiveComponent* TargetComponent);

	/**
	 * @brief FNiagaraSpawnData와 UPrimitiveComponent, ETileActorDirection 방향을 반영하여 나이아가라 VFX를 스폰합니다.
	 */
	static UNiagaraComponent* SpawnNiagaraEffectWithDirection(const FSoftNiagaraSpawnData& NiagaraSpawnData, UPrimitiveComponent* TargetComponent, ETileActorDirection Direction);
	static UNiagaraComponent* SpawnNiagaraEffectWithDirection(const FNiagaraSpawnData& NiagaraSpawnData, UPrimitiveComponent* TargetComponent, ETileActorDirection Direction);

private:
	static UNiagaraComponent* SpawnNiagaraEffectWithDirection_Internal(const TObjectPtr<UNiagaraSystem>& NiagaraSystem, UPrimitiveComponent* TargetComponent, const FName& SocketName, const FTransform& RelativeTransform, bool Attached, ETileActorDirection Direction);
};

