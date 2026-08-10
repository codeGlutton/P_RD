#include "Singleton/WorldSubsystem/TacticalFrameworkSubsystem.h"
#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"

#include "Setting/GamePlaySettings.h"
#include "FunctionLibrary/VFXFunctionLibrary.h"

#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Component/VFXTimelineComponent/VFXTimelineComponent.h"

#include "Actor/ActorModel.h"
#include "ObjectView.h"
#include "Actor/BoardActor/BoardCombatTargetView.h"

void UTacticalFrameworkSubsystem::BindModel(UObjectModel* Model)
{
	mFrameworkModel = Cast<UTacticalFrameworkModel>(Model);

	if (mFrameworkModel != nullptr)
	{
		mFrameworkModel->OnPreTacticalEffectSpecApplyUI.AddUObject(this, &UTacticalFrameworkSubsystem::ApplyGlobalVFXEffect);
	}
}

void UTacticalFrameworkSubsystem::UnbindModel(UObjectModel* Model)
{
	if (mFrameworkModel != nullptr)
	{
		mFrameworkModel->OnPreTacticalEffectSpecApplyUI.RemoveAll(this);
	}

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
			FVFXTimelineEventTarget EventTarget;
			UPrimitiveComponent* TargetMeshComp = CombatTargetView->GetTargetMeshComponent();
			for (const TObjectPtr<USceneComponent>& ChildComponent : TargetMeshComp->GetAttachChildren())
			{
				UPrimitiveComponent* ChildMeshComp = Cast<UPrimitiveComponent>(ChildComponent);
				if (ChildMeshComp != nullptr)
				{
					EventTarget.mMeshComps.Add(ChildMeshComp);
				}
			}
			EventTarget.mMeshComps.Add(TargetMeshComp);

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

