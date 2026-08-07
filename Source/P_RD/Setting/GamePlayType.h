/*****************************************************************//**
 * @file   GamePlayType.h
 * @brief  게임 플레이 연관 공통 타입 및 VFX 설정 정의 헤더
 * @author 모호재
 * @date   2026-08-02
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "GameplayTagContainer.h"
#include "NiagaraSystem.h"
#include "GamePlayType.generated.h"

class UTacticalEffect;

/**
 * @brief 나이아가라 VFX 스폰 및 어태치 데이터 구조체
 */
USTRUCT(BlueprintType)
struct FNiagaraSpawnData
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = "Niagara", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "NiagaraSystem"))
	TObjectPtr<UNiagaraSystem> mNiagaraSystem = nullptr;

	UPROPERTY(Category = "Niagara", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "SocketName"))
	FName mSocketName = NAME_None;

	UPROPERTY(Category = "Niagara", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "RelativeTransform"))
	FTransform mRelativeTransform = FTransform::Identity;

	UPROPERTY(Category = "Niagara", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Attached"))
	bool mAttached = true;
};

/**
 * @brief 소프트 나이아가라 VFX 스폰 및 어태치 데이터 구조체
 */
USTRUCT(BlueprintType)
struct FSoftNiagaraSpawnData
{
	GENERATED_BODY()

public:
	FNiagaraSpawnData LoadSynchronous() const;

public:
	UPROPERTY(Category = "Niagara", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "NiagaraSystem"))
	TSoftObjectPtr<UNiagaraSystem> mNiagaraSystem = nullptr;

	UPROPERTY(Category = "Niagara", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "SocketName"))
	FName mSocketName = NAME_None;

	UPROPERTY(Category = "Niagara", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "RelativeTransform"))
	FTransform mRelativeTransform = FTransform::Identity;

	UPROPERTY(Category = "Niagara", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Attached"))
	bool mAttached = true;
};

/**
 * @brief 전역 상태 이상 VFX 설정 값 객체
 */
USTRUCT(BlueprintType)
struct FGlobalStatusEffectVFXSetting
{
	GENERATED_BODY()

public:
	// @brief 상태이상에 따른 VFX 매핑 정보
	UPROPERTY(Category = StatusEffect, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "EffectVFXs"))
	TMap<TSoftClassPtr<UTacticalEffect>, FSoftNiagaraSpawnData> mEffectVFXs;
};

/**
 * @brief 전투 대상 디졸브 VFX 설정 값 객체
 */
USTRUCT(BlueprintType)
struct FCombatTargetDissolveVFXSetting
{
	GENERATED_BODY()

public:
	// @brief 전투 대상이 타일맵에서 사라질 때 사용되는 전역 VFX 설정 값
	UPROPERTY(Config, Category = VFX, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "CombatTargetDissolveVFX", ToolTip = "전투 대상이 타일맵에서 사라질 때 사용되는 전역 VFX 설정 값"))
	FSoftNiagaraSpawnData mCombatTargetDissolveVFX;
	// @brief 전투 대상이 타일맵에서 사라질 때 사용되는 커브 데이터
	UPROPERTY(Config, Category = VFX, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "CombatTargetDissolveCurve", ToolTip = "전투 대상이 타일맵에서 사라질 때 사용되는 커브 데이터"))
	TSoftObjectPtr<UCurveFloat> mCombatTargetDissolveCurve;
};
