/*****************************************************************//**
 * @file   VFXTimelineTrackEvent.h
 * @brief  VFX 타임라인 스폰 데이터 및 파라미터 트랙 이벤트 정의 헤더
 * @author 모호재
 * @date   2026-08-06
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "VFXTimelineTrackEvent.generated.h"

class UNiagaraComponent;

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EVFXTimelineSyncTarget : uint8
{
	None = 0							UMETA(Hidden),
	PrimitiveData = 1 << 0,
	NiagaraUserParameter = 1 << 1,
};
ENUM_CLASS_FLAGS(EVFXTimelineSyncTarget);

/**
 * @brief 타임라인 트랙 이벤트 데이터
 */
USTRUCT(BlueprintType)
struct FVFXTimelineTrackEvent
{
	GENERATED_BODY()

public:
	void Trigger(float Value, const TArray<TWeakObjectPtr<UPrimitiveComponent>>& MeshComps, const TArray<TWeakObjectPtr<UNiagaraComponent>>& NiagaraComps) const;
	void Trigger(const FVector& Value, const TArray<TWeakObjectPtr<UPrimitiveComponent>>& MeshComps, const TArray<TWeakObjectPtr<UNiagaraComponent>>& NiagaraComps) const;
	void Trigger(const FLinearColor& Value, const TArray<TWeakObjectPtr<UPrimitiveComponent>>& MeshComps, const TArray<TWeakObjectPtr<UNiagaraComponent>>& NiagaraComps) const;

public:
	// @brief 타임라인 타겟 플래그
	UPROPERTY(Category = "Event", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "SyncTarget", Bitmask, BitmaskEnum = "/Script/P_RD.EVFXTimelineSyncTarget"))
	int32 mSyncTarget = static_cast<int32>(EVFXTimelineSyncTarget::None);

public:
	// @brief 변경할 파라미터 이름
	UPROPERTY(Category = "Parameter", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "ParameterName"))
	FName mParameterName = NAME_None;

	// @brief 적용할 CPD (Custom Primitive Data) 인덱스
	UPROPERTY(Category = "CPD", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "CPDIndex"))
	int32 mCPDIndex = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct FVFXTimelineEventTarget
{
	GENERATED_BODY()

public:
	void Clear();

public:
	TArray<TWeakObjectPtr<UPrimitiveComponent>> mMeshComps;
	TArray<TWeakObjectPtr<UNiagaraComponent>> mNiagaraComps;
};

