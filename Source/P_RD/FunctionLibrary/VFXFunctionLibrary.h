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
#include "Component/VFXTimelineComponent/VFXTimelineTrackEvent.h"
#include "VFXFunctionLibrary.generated.h"

class UNiagaraComponent;
class UPrimitiveComponent;
class UVFXTimelineComponent;

/**
 * @brief VFX 스폰 헬퍼 정적 함수 라이브러리
 */
UCLASS()
class P_RD_API UVFXFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static UNiagaraComponent* SpawnNiagaraEffect(const TSoftObjectPtr<UNiagaraSystem>& NiagaraSystem, UObject* WorldContextObject, const FTransform& Transform);
	static UNiagaraComponent* SpawnNiagaraEffect(const TObjectPtr<UNiagaraSystem>& NiagaraSystem, UObject* WorldContextObject, const FTransform& Transform);

public:
	/**
	 * @brief FNiagaraSpawnData 기반으로 나이아가라 VFX를 스폰합니다.
	 */
	static UNiagaraComponent* SpawnNiagaraEffect(const FSoftNiagaraSpawnData& NiagaraSpawnData, const AActor* TargetActor);
	static UNiagaraComponent* SpawnNiagaraEffect(const FNiagaraSpawnData& NiagaraSpawnData, const AActor* TargetActor);
	static UNiagaraComponent* SpawnNiagaraEffect(const FSoftNiagaraSpawnData& NiagaraSpawnData, UPrimitiveComponent* TargetComponent);
	static UNiagaraComponent* SpawnNiagaraEffect(const FNiagaraSpawnData& NiagaraSpawnData, UPrimitiveComponent* TargetComponent);

	/**
	 * @brief ETileActorDirection 방향을 반영하여 FNiagaraSpawnData 기반으로 나이아가라 VFX를 스폰합니다.
	 */
	static UNiagaraComponent* SpawnNiagaraEffectWithDirection(const FSoftNiagaraSpawnData& NiagaraSpawnData, const AActor* TargetActor, ETileActorDirection Direction);
	static UNiagaraComponent* SpawnNiagaraEffectWithDirection(const FNiagaraSpawnData& NiagaraSpawnData, const AActor* TargetActor, ETileActorDirection Direction);
	static UNiagaraComponent* SpawnNiagaraEffectWithDirection(const FSoftNiagaraSpawnData& NiagaraSpawnData, UPrimitiveComponent* TargetComponent, ETileActorDirection Direction);
	static UNiagaraComponent* SpawnNiagaraEffectWithDirection(const FNiagaraSpawnData& NiagaraSpawnData, UPrimitiveComponent* TargetComponent, ETileActorDirection Direction);

public:
	/**
	 * @brief FVFXTimelineExecutionData 기반으로 타임라인을 실행합니다.
	 */
	static bool ExecuteVFXTimeline(UVFXTimelineComponent* TimelineComp, const FVFXTimelineExecutionData& TimelineExecutionData, FVFXTimelineEventTarget EventTarget);
	static bool ExecuteVFXTimeline(UVFXTimelineComponent* TimelineComp, const FVFXTimelineExecutionData& TimelineExecutionData, TArray<TWeakObjectPtr<UPrimitiveComponent>>& TargetMeshes, TArray<TWeakObjectPtr<UNiagaraComponent>>& TargetNiagaras);

public:
	/**
	 * @brief FVFXSpawnData 기반으로 나이아가라 스폰 및 타임라인을 실행합니다.
	 */
	static TArray<TObjectPtr<UNiagaraComponent>> SpawnAndExecuteVFX(const FVFXSpawnData& VFXSpawnData, const AActor* TargetActor, UVFXTimelineComponent* TimelineComp, TArray<TWeakObjectPtr<UPrimitiveComponent>>& TargetMeshes, TArray<TWeakObjectPtr<UNiagaraComponent>>& TargetNiagaras, ETileActorDirection Direction = ETileActorDirection::Forward);
	static TArray<TObjectPtr<UNiagaraComponent>> SpawnAndExecuteVFX(const FSoftVFXSpawnData& VFXSpawnData, const AActor* TargetActor, UVFXTimelineComponent* TimelineComp, TArray<TWeakObjectPtr<UPrimitiveComponent>>& TargetMeshes, TArray<TWeakObjectPtr<UNiagaraComponent>>& TargetNiagaras, ETileActorDirection Direction = ETileActorDirection::Forward);

	static TArray<TObjectPtr<UNiagaraComponent>> SpawnAndExecuteVFX(const FVFXSpawnData& VFXSpawnData, UPrimitiveComponent* TargetComponent, UVFXTimelineComponent* TimelineComp, TArray<TWeakObjectPtr<UPrimitiveComponent>>& TargetMeshes, TArray<TWeakObjectPtr<UNiagaraComponent>>& TargetNiagaras, ETileActorDirection Direction = ETileActorDirection::Forward);
	static TArray<TObjectPtr<UNiagaraComponent>> SpawnAndExecuteVFX(const FSoftVFXSpawnData& VFXSpawnData, UPrimitiveComponent* TargetComponent, UVFXTimelineComponent* TimelineComp, TArray<TWeakObjectPtr<UPrimitiveComponent>>& TargetMeshes, TArray<TWeakObjectPtr<UNiagaraComponent>>& TargetNiagaras, ETileActorDirection Direction = ETileActorDirection::Forward);

	static TArray<TObjectPtr<UNiagaraComponent>> SpawnAndExecuteVFX(const FVFXSpawnData& VFXSpawnData, const AActor* TargetActor, UVFXTimelineComponent* TimelineComp, FVFXTimelineEventTarget EventTarget, ETileActorDirection Direction = ETileActorDirection::Forward);
	static TArray<TObjectPtr<UNiagaraComponent>> SpawnAndExecuteVFX(const FSoftVFXSpawnData& VFXSpawnData, const AActor* TargetActor, UVFXTimelineComponent* TimelineComp, FVFXTimelineEventTarget EventTarget, ETileActorDirection Direction = ETileActorDirection::Forward);

	static TArray<TObjectPtr<UNiagaraComponent>> SpawnAndExecuteVFX(const FVFXSpawnData& VFXSpawnData, UPrimitiveComponent* TargetComponent, UVFXTimelineComponent* TimelineComp, FVFXTimelineEventTarget EventTarget, ETileActorDirection Direction = ETileActorDirection::Forward);
	static TArray<TObjectPtr<UNiagaraComponent>> SpawnAndExecuteVFX(const FSoftVFXSpawnData& VFXSpawnData, UPrimitiveComponent* TargetComponent, UVFXTimelineComponent* TimelineComp, FVFXTimelineEventTarget EventTarget, ETileActorDirection Direction = ETileActorDirection::Forward);

private:
	static UNiagaraComponent* SpawnNiagaraEffectWithDirection_Internal(const TObjectPtr<UNiagaraSystem>& NiagaraSystem, const AActor* TargetActor, UPrimitiveComponent* TargetComponent, const FName& SocketName, const FTransform& RelativeTransform, bool Attached, ETileActorDirection Direction);
};


