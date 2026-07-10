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
	check(mWorld != nullptr);

	UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(mWorld);
	check(TacticalFrameworkModel != nullptr);

	TacticalFrameworkModel->PushCurrentAppliedTE(Spec, Model);
}

FScopeCurrentTacticalEffectBeingApplied::~FScopeCurrentTacticalEffectBeingApplied()
{
	UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(mWorld);
	check(TacticalFrameworkModel != nullptr);

	TacticalFrameworkModel->PopCurrentAppliedTE();
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
	--mGlobalBatchCount;
	if (mGlobalBatchCount == 0)
	{
		TSet<FTacticalAggregator*> LocalSet(MoveTemp(mDirtyAggregators));
		for (FTacticalAggregator* Agg : LocalSet)
		{
			Agg->BroadcastOnDirty();
		}
		LocalSet.Empty();
	}
}

void UTacticalFrameworkModel::AddAggregatorDirty(FTacticalAggregator* Aggregator)
{
	mDirtyAggregators.Add(Aggregator);
}

int32 UTacticalFrameworkModel::RemoveAggregatorDirty(FTacticalAggregator* Aggregator)
{
	return mDirtyAggregators.Remove(Aggregator);
}

int32 UTacticalFrameworkModel::GetGlobalBatchCount() const
{
	return mGlobalBatchCount;
}
