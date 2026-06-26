#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

#include "TAS/Effect/TacticalEffect.h"
#include "TAS/Effect/TacticalEffectContext.h"
#include "TAS/Aggregator/TacticalAggregator.h"

DEFINE_LOG_CATEGORY(LogTacticalFramework)

FScopeCurrentTacticalEffectBeingApplied::FScopeCurrentTacticalEffectBeingApplied(UWorld* World, const FTacticalEffectSpec* Spec, UAttributeSetComponentModel* Model)
{
	mWorld = World;
	check(mWorld != nullptr);

	UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(mWorld);
	check(TacticalFrameworkModel != nullptr);

	TacticalFrameworkModel->PushCurrentAppliedGE(Spec, Model);
}

FScopeCurrentTacticalEffectBeingApplied::~FScopeCurrentTacticalEffectBeingApplied()
{
	UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(mWorld);
	check(TacticalFrameworkModel != nullptr);

	TacticalFrameworkModel->PopCurrentAppliedGE();
}

void UTacticalFrameworkModel::GlobalPreTacticalEffectSpecApply(FTacticalEffectSpec& Spec, UAttributeSetComponentModel* Model)
{
}

UTacticalEffectContext* UTacticalFrameworkModel::AllocTacticalEffectContext() const
{
	return NewObject<UTacticalEffectContext>(const_cast<UTacticalFrameworkModel*>(this));
}

void UTacticalFrameworkModel::PushCurrentAppliedGE(const FTacticalEffectSpec* Spec, UAttributeSetComponentModel* Model)
{
}

void UTacticalFrameworkModel::SetCurrentAppliedGE(const FTacticalEffectSpec* Spec)
{
}

void UTacticalFrameworkModel::PopCurrentAppliedGE()
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
