/*****************************************************************//**
 * @file   TacticalFrameworkModel.h
 * @brief  TAS의 글로벌 서브시스템 모델 정의 헤더
 * @author 모호재
 * @date   2026-06-23
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "ObjectModel.h"
#include "TAS/Effect/ActiveTacticalEffect.h"
#include "TacticalFrameworkModel.generated.h"

class UAttributeSetComponentModel;

struct FTacticalEffectSpec;
class UTacticalEffectContext;

struct FTacticalAggregator;

// Tactical Framework 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogTacticalFramework, Log, All)

struct FScopeCurrentTacticalEffectBeingApplied
{
public:
	FScopeCurrentTacticalEffectBeingApplied(UWorld* World, const FTacticalEffectSpec* Spec, UAttributeSetComponentModel* Model);
	~FScopeCurrentTacticalEffectBeingApplied();

private:
	TObjectPtr<UWorld> mWorld;
};

/**
 * @brief  SRPG 유저 명령을 전달하는 서브시스템 모델
 */
UCLASS()
class P_RD_API UTacticalFrameworkModel : public UObjectModel
{
	GENERATED_BODY()

	friend struct FActiveTacticalEffectHandle;

public:
	void Initialize() override;
	void Uninitialize() override;

public:
	UCurveTable* GetGlobalInitCurveTable();
	FTacticalAttributeSetInitter* GetAttributeSetInitter();

protected:
	void ReloadAttributeDefaults();
	void AllocAttributeSetInitter();

#if WITH_EDITOR
	void OnTableReimported(UObject* InObject);
#endif

	/* Effect Context 제작 */
public:
	virtual UTacticalEffectContext* AllocTacticalEffectContext() const;

	/* 특정 시기 호출 */
public:
	void GlobalPreTacticalEffectSpecApply(FTacticalEffectSpec& Spec, UAttributeSetComponentModel* Model);

	/* Effect 적용 시 최상위 객체 추적 */
public:
	void PushCurrentAppliedTE(const FTacticalEffectSpec* Spec, UAttributeSetComponentModel* Model);
	void SetCurrentAppliedTE(const FTacticalEffectSpec* Spec);
	void PopCurrentAppliedTE();

	/* Aggregator 배칭 */
public:
	void BeginAggregatorDirtyBatch();
	void EndAggregatorDirtyBatch();

	void AddAggregatorDirty(FTacticalAggregator* Aggregator);
	int32 RemoveAggregatorDirty(FTacticalAggregator* Aggregator);

	int32 GetGlobalBatchCount() const;

	/* 초기 구성 데이터 */
protected:
	UPROPERTY(Category = "Attribute", DuplicateTransient, VisibleAnywhere, meta = (DisplayName = "GlobalInitCurveTable"))
	TObjectPtr<UCurveTable> mGlobalInitCurveTable;

	TSharedPtr<FTacticalAttributeSetInitter> mGlobalAttributeSetInitter;

	/* Effect 매핑 데이터 */
protected:
	UPROPERTY(Category = "Effect", VisibleAnywhere, meta = (DisplayName = "EffectOwningModelMap"))
	TMap<FActiveTacticalEffectHandle, TWeakObjectPtr<UAttributeSetComponentModel>> mEffectOwningModelMap;

	/* 임시 데이터 */
protected:
	int32 mGlobalBatchCount;
	TSet<FTacticalAggregator*> mDirtyAggregators;
};
