/*****************************************************************//**
 * @file   TacticalFrameworkSubsystem.h
 * @brief  TAS의 글로벌 서브시스템 정의 헤더
 * @author 모호재
 * @date   2026-06-23
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "ObjectView.h"
#include "Subsystems/WorldSubsystem.h"
#include "TAS/Effect/ActiveTacticalEffect.h"
#include "TacticalFrameworkSubsystem.generated.h"

class UTacticalFrameworkModel;

struct FTacticalEffectSpec;
class UAttributeSetComponentModel;

class UNiagaraComponent;

USTRUCT()
struct FSpawnedNiagaraComponentList
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = Niagara, VisibleAnywhere, meta = (DisplayName = "Components"))
	TArray<TWeakObjectPtr<UNiagaraComponent>> mComponents;
};

/**
 * @brief  TAS의 글로벌 서브시스템
 */
UCLASS()
class P_RD_API UTacticalFrameworkSubsystem : public UWorldSubsystem, public IObjectView
{
	GENERATED_BODY()

	/* IObjectView 상속 */
public:
	void BindModel(UObjectModel* Model) override;
	void UnbindModel(UObjectModel* Model) override;

protected:
	UObjectModel* GetModel_Internal() const override;

protected:
	void ApplyGlobalVFXEffect(const FTacticalEffectSpec& Spec, const UAttributeSetComponentModel* Model) const;
	void SpawnGlobalInfiniteVFXEffect(const FTacticalEffectSpec& Spec, FActiveTacticalEffectHandle ActiveHandle, const UAttributeSetComponentModel* Model);
	void DestroyGlobalInfiniteVFXEffect(const FTacticalEffectSpec& Spec, FActiveTacticalEffectHandle ActiveHandle, const UAttributeSetComponentModel* Model);

protected:
	UPROPERTY(Category = VFX, VisibleAnywhere, meta = (DisplayName = "SpawnedVFXComponents"))
	TMap<FActiveTacticalEffectHandle, FSpawnedNiagaraComponentList> mSpawnedVFXComponents;

protected:
	UPROPERTY(Category = Model, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "FrameworkModel"))
	TWeakObjectPtr<UTacticalFrameworkModel> mFrameworkModel;
};
