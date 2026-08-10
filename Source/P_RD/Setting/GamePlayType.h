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
#include "Component/VFXTimelineComponent/VFXTimelineTrackEvent.h"
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
 * @brief VFX 타임라인 실행 데이터 구조체
 */
USTRUCT(BlueprintType)
struct FVFXTimelineExecutionData
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = "Timeline", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "KeyName"))
	FName mKeyName;
	UPROPERTY(Category = "Timeline", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Direction"))
	TEnumAsByte<ETimelineDirection::Type> mDirection = ETimelineDirection::Forward;
	UPROPERTY(Category = "Timeline", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "IsPlayFromStart"))
	bool mIsPlayFromStart = true;
};

/**
 * @brief VFX 스폰 데이터 구조체
 */
USTRUCT(BlueprintType)
struct FVFXSpawnData
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = "Niagara", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "NiagaraSpawnDatas"))
	TArray<FNiagaraSpawnData> mNiagaraSpawnDatas;

public:
	UPROPERTY(Category = "Timeline", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "TimelineExecutionDatas"))
	TArray<FVFXTimelineExecutionData> mTimelineExecutionDatas;
	UPROPERTY(Category = "Timeline", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "IncludeSpawnedNiagaraInTimeline"))
	bool mIncludeSpawnedNiagaraInTimeline = true;
};

/**
 * @brief VFX 스폰 데이터 구조체
 */
USTRUCT(BlueprintType)
struct FSoftVFXSpawnData
{
	GENERATED_BODY()

public:
	FVFXSpawnData LoadSynchronous() const;

public:
	UPROPERTY(Category = "Niagara", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "NiagaraSpawnDatas"))
	TArray<FSoftNiagaraSpawnData> mNiagaraSpawnDatas;

public:
	UPROPERTY(Category = "Timeline", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "TimelineExecutionDatas"))
	TArray<FVFXTimelineExecutionData> mTimelineExecutionDatas;
	UPROPERTY(Category = "Timeline", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "IncludeSpawnedNiagaraInTimeline"))
	bool mIncludeSpawnedNiagaraInTimeline = true;
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
	TMap<TSoftClassPtr<UTacticalEffect>, FSoftVFXSpawnData> mEffectVFXs;
};

/**
 * @brief 전투 대상 VFX 설정 값 객체
 */
USTRUCT(BlueprintType)
struct FCombatTargetVFXTimelineSetting
{
	GENERATED_BODY()

public:
	// @brief 활용될 타임라인의 키 네임
	UPROPERTY(Config, Category = VFX, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "TimelineKeyName", ToolTip = "활용될 타임라인의 키 네임"))
	FName mTimelineKeyName;
	// @brief 활용될 타임라인의 커브 데이터
	UPROPERTY(Config, Category = VFX, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "TimelineCurve", ToolTip = "활용될 타임라인의 커브 데이터"))
	TSoftObjectPtr<UCurveBase> mTimelineCurve;
	// @brief 활용될 타임라인의 이벤트 데이터
	UPROPERTY(Config, Category = VFX, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "TimelineEvent", ToolTip = "활용될 타임라인의 이벤트 데이터"))
	FVFXTimelineTrackEvent mTimelineEvent;
};

