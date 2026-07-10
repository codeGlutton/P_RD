#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

#include "TAS/AttributeSet/TacticalAttributeSet.h"
#include "TAS/Effect/TacticalEffect.h"
#include "TAS/Effect/TacticalEffectContext.h"
#include "TAS/Aggregator/TacticalAggregator.h"

#include "Setting/GameBalanceSettings.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

DEFINE_LOG_CATEGORY(LogTacticalFramework)

FScopeCurrentTacticalEffectBeingApplied::FScopeCurrentTacticalEffectBeingApplied(UWorld* World, const FTacticalEffectSpec* Spec, UAttributeSetComponentModel* Model)
{
	mWorld = World;
	UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(mWorld);
	if (TacticalFrameworkModel != nullptr)
	{
		TacticalFrameworkModel->PushCurrentAppliedTE(Spec, Model);
		mDidPush = true;
	}
}

FScopeCurrentTacticalEffectBeingApplied::~FScopeCurrentTacticalEffectBeingApplied()
{
	if (mDidPush == false)
	{
		return;
	}

	UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(mWorld);
	if (TacticalFrameworkModel != nullptr)
	{
		TacticalFrameworkModel->PopCurrentAppliedTE();
	}
}

void UTacticalFrameworkModel::Initialize()
{
	Super::Initialize();

	GetGlobalInitCurveTable();
	ReloadAttributeDefaults();
}

void UTacticalFrameworkModel::Uninitialize()
{
	Super::Uninitialize();
}

UCurveTable* UTacticalFrameworkModel::GetGlobalInitCurveTable()
{
	if (mGlobalInitCurveTable == nullptr)
	{
		const UGameBalanceSettings* GameBalanceSettings = GetDefault<UGameBalanceSettings>();
		if (GameBalanceSettings->mInitializeCurveTable.ToSoftObjectPath().IsNull() == false)
		{
			mGlobalInitCurveTable = Cast<UCurveTable>(GameBalanceSettings->mInitializeCurveTable.LoadSynchronous());
		}
	}
	return mGlobalInitCurveTable;
}

FTacticalAttributeSetInitter* UTacticalFrameworkModel::GetAttributeSetInitter()
{
	check(mGlobalAttributeSetInitter.IsValid() == true);
	return mGlobalAttributeSetInitter.Get();
}

void UTacticalFrameworkModel::ReloadAttributeDefaults()
{
	if (mGlobalInitCurveTable != nullptr)
	{
		AllocAttributeSetInitter();

		TArray<UCurveTable*> TmpTables = { mGlobalInitCurveTable };
		GetAttributeSetInitter()->PreloadAttributeSetData(TmpTables);

#if WITH_EDITOR
		if (GIsEditor == true)
		{
			GEditor->GetEditorSubsystem<UImportSubsystem>()->OnAssetReimport.AddUObject(this, &UTacticalFrameworkModel::OnTableReimported);
		}
#endif
	}
}

void UTacticalFrameworkModel::AllocAttributeSetInitter()
{
	mGlobalAttributeSetInitter = MakeShared<FTacticalAttributeSetInitterDiscreteLevels>();
}

#if WITH_EDITOR

void UTacticalFrameworkModel::OnTableReimported(UObject* InObject)
{
	if (GIsEditor && !IsRunningCommandlet() && InObject)
	{
		UCurveTable* ReimportedCurveTable = Cast<UCurveTable>(InObject);
		if (ReimportedCurveTable && mGlobalInitCurveTable == ReimportedCurveTable)
		{
			ReloadAttributeDefaults();
		}
	}
}

#endif

UTacticalEffectContext* UTacticalFrameworkModel::AllocTacticalEffectContext() const
{
	return NewObject<UTacticalEffectContext>(const_cast<UTacticalFrameworkModel*>(this));
}

void UTacticalFrameworkModel::GlobalPreTacticalEffectSpecApply(FTacticalEffectSpec& Spec, UAttributeSetComponentModel* Model)
{
}

void UTacticalFrameworkModel::PushCurrentAppliedTE(const FTacticalEffectSpec* Spec, UAttributeSetComponentModel* Model)
{
}

void UTacticalFrameworkModel::SetCurrentAppliedTE(const FTacticalEffectSpec* Spec)
{
}

void UTacticalFrameworkModel::PopCurrentAppliedTE()
{
}

void UTacticalFrameworkModel::BeginAggregatorDirtyBatch()
{
	++mGlobalBatchCount;
}

void UTacticalFrameworkModel::EndAggregatorDirtyBatch()
{
	if (mGlobalBatchCount <= 0)
	{
		UE_LOG(LogTacticalFramework, Warning, TEXT("Aggregator dirty batch 카운트 불일치"));
		mGlobalBatchCount = 0;
		return;
	}

	--mGlobalBatchCount;
	if (mGlobalBatchCount == 0)
	{
		TArray<TWeakPtr<FTacticalAggregator>> LocalAggregators = MoveTemp(mDirtyAggregators);
		mDirtyAggregators.Reset();
		for (const TWeakPtr<FTacticalAggregator>& WeakAggregator : LocalAggregators)
		{
			if (TSharedPtr<FTacticalAggregator> Aggregator = WeakAggregator.Pin())
			{
				Aggregator->BroadcastOnDirty();
			}
		}
	}
}

void UTacticalFrameworkModel::AddAggregatorDirty(FTacticalAggregator* Aggregator)
{
	if (Aggregator == nullptr)
	{
		return;
	}

	const bool AlreadyQueued = mDirtyAggregators.ContainsByPredicate([Aggregator](const TWeakPtr<FTacticalAggregator>& WeakAggregator) {
		return WeakAggregator.Pin().Get() == Aggregator;
		});
	if (AlreadyQueued == false)
	{
		mDirtyAggregators.Add(Aggregator->AsShared());
	}
}

int32 UTacticalFrameworkModel::RemoveAggregatorDirty(FTacticalAggregator* Aggregator)
{
	return mDirtyAggregators.RemoveAll([Aggregator](const TWeakPtr<FTacticalAggregator>& WeakAggregator) {
		const TSharedPtr<FTacticalAggregator> Pinned = WeakAggregator.Pin();
		return Pinned.IsValid() == false || Pinned.Get() == Aggregator;
		});
}

int32 UTacticalFrameworkModel::GetGlobalBatchCount() const
{
	return mGlobalBatchCount;
}
