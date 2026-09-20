#include "Singleton/WorldSubsystem/TacticalFrameworkSubsystem.h"
#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"

#include "Setting/GamePlaySettings.h"
#include "FunctionLibrary/VFXFunctionLibrary.h"

#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Component/VFXTimelineComponent/VFXTimelineComponent.h"
#include "NiagaraComponent.h"

#include "Actor/ActorModel.h"
#include "ObjectView.h"
#include "Actor/BoardActor/BoardCombatTargetView.h"

void UTacticalFrameworkSubsystem::BindModel(UObjectModel* Model)
{
	mFrameworkModel = Cast<UTacticalFrameworkModel>(Model);

	if (mFrameworkModel != nullptr)
	{
		mFrameworkModel->OnPreTacticalEffectSpecApplyUI.AddUObject(this, &UTacticalFrameworkSubsystem::ApplyGlobalVFXEffect);
		mFrameworkModel->OnPostTacticalEffectSpecAddedUI.AddUObject(this, &UTacticalFrameworkSubsystem::SpawnGlobalInfiniteVFXEffect);
		mFrameworkModel->OnPreTacticalEffectSpecRemovedUI.AddUObject(this, &UTacticalFrameworkSubsystem::DestroyGlobalInfiniteVFXEffect);
	}
}

void UTacticalFrameworkSubsystem::UnbindModel(UObjectModel* Model)
{
	if (mFrameworkModel != nullptr)
	{
		mFrameworkModel->OnPreTacticalEffectSpecApplyUI.RemoveAll(this);
		mFrameworkModel->OnPostTacticalEffectSpecAddedUI.RemoveAll(this);
		mFrameworkModel->OnPreTacticalEffectSpecRemovedUI.RemoveAll(this);
	}

	mSpawnedVFXComponents.Reset();
	mFrameworkModel.Reset();
}

UObjectModel* UTacticalFrameworkSubsystem::GetModel_Internal() const
{
	return mFrameworkModel.Get();
}

void UTacticalFrameworkSubsystem::ApplyGlobalVFXEffect(const FTacticalEffectSpec& Spec, const UAttributeSetComponentModel* Model) const
{
	const UGamePlaySettings* GamePlaySettings = GetDefault<UGamePlaySettings>();
	checkf(GamePlaySettings != nullptr, TEXT("게임 플레이 세팅 nullptr 오류"));

	/* VFX 생성 */

	const FSoftVFXSpawnData* VFXSpawnData = GamePlaySettings->mGlobalStatusEffectVFXSetting.mEffectVFXs.Find(Spec.mEffectClass->GetClass());

	const UActorModel* Instigator = Model->GetOwnerModel();
	const AActor* ActorView = Instigator->GetView<AActor>();
	if (ActorView != nullptr && VFXSpawnData != nullptr)
	{
		const IBoardCombatTargetView* CombatTargetView = Instigator->GetView<IBoardCombatTargetView>();
		if (CombatTargetView != nullptr)
		{
			UPrimitiveComponent* TargetMeshComp = CombatTargetView->GetTargetMeshComponent();
			const FVFXTimelineEventTarget EventTarget = UVFXFunctionLibrary::MakeTimelineEventTarget(TargetMeshComp);

			UVFXFunctionLibrary::SpawnAndExecuteVFX(*VFXSpawnData, TargetMeshComp, CombatTargetView->GetCombatTargetVFXTimelineComponent(), EventTarget);
		}
		else
		{
			for (const FSoftNiagaraSpawnData& NiagaraSpawnData : VFXSpawnData->mNiagaraSpawnDatas)
			{
				UVFXFunctionLibrary::SpawnNiagaraEffect(NiagaraSpawnData, ActorView);
			}
		}
	}
}

void UTacticalFrameworkSubsystem::SpawnGlobalInfiniteVFXEffect(const FTacticalEffectSpec& Spec, FActiveTacticalEffectHandle ActiveHandle, const UAttributeSetComponentModel* Model)
{
	const UGamePlaySettings* GamePlaySettings = GetDefault<UGamePlaySettings>();
	checkf(GamePlaySettings != nullptr, TEXT("게임 플레이 세팅 nullptr 오류"));

	/* VFX 생성 */

	const FSoftVFXSpawnData* VFXSpawnData = GamePlaySettings->mGlobalStatusEffectVFXSetting.mInfiniteEffectVFXs.Find(Spec.mEffectClass->GetClass());

	const UActorModel* Instigator = Model->GetOwnerModel();
	const AActor* ActorView = Instigator->GetView<AActor>();
	if (ActorView != nullptr && VFXSpawnData != nullptr)
	{
		const IBoardCombatTargetView* CombatTargetView = Instigator->GetView<IBoardCombatTargetView>();
		TArray<TObjectPtr<UNiagaraComponent>> SpawnedComponents;
		if (CombatTargetView != nullptr)
		{
			UPrimitiveComponent* TargetMeshComp = CombatTargetView->GetTargetMeshComponent();
			const FVFXTimelineEventTarget EventTarget = UVFXFunctionLibrary::MakeTimelineEventTarget(TargetMeshComp);

			SpawnedComponents = UVFXFunctionLibrary::SpawnAndExecuteVFX(*VFXSpawnData, TargetMeshComp, CombatTargetView->GetCombatTargetVFXTimelineComponent(), EventTarget);
		}
		else
		{
			SpawnedComponents.Reserve(VFXSpawnData->mNiagaraSpawnDatas.Num());
			for (const FSoftNiagaraSpawnData& NiagaraSpawnData : VFXSpawnData->mNiagaraSpawnDatas)
			{
				SpawnedComponents.Add(UVFXFunctionLibrary::SpawnNiagaraEffect(NiagaraSpawnData, ActorView));
			}
		}

		FSpawnedNiagaraComponentList& NiagaraComponentList = mSpawnedVFXComponents.FindOrAdd(ActiveHandle);
		for (const TObjectPtr<UNiagaraComponent>& SpawnedComponent : SpawnedComponents)
		{
			NiagaraComponentList.mComponents.Add(SpawnedComponent);
		}
	}
}

void UTacticalFrameworkSubsystem::DestroyGlobalInfiniteVFXEffect(const FTacticalEffectSpec& Spec, FActiveTacticalEffectHandle ActiveHandle, const UAttributeSetComponentModel* Model)
{
	FSpawnedNiagaraComponentList* NiagaraComponentList = mSpawnedVFXComponents.Find(ActiveHandle);
	if (NiagaraComponentList != nullptr)
	{
		for (const TWeakObjectPtr<UNiagaraComponent>& NiagaraComponent : NiagaraComponentList->mComponents)
		{
			if (NiagaraComponent.IsValid() == true)
			{
				NiagaraComponent->Deactivate();
			}
		}

		mSpawnedVFXComponents.Remove(ActiveHandle);
	}
}

