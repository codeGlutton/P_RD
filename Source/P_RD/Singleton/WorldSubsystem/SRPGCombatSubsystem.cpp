#include "Singleton/WorldSubsystem/SRPGCombatSubsystem.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "UI/Combat/CombatUIModel.h"

void USRPGCombatSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

TStatId USRPGCombatSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USRPGCombatSubsystem, STATGROUP_Tickables);
}

void USRPGCombatSubsystem::BindModel(UObjectModel* Model)
{
	mCombatModel = Cast<USRPGCombatModel>(Model);
}

void USRPGCombatSubsystem::UnbindModel(UObjectModel* Model)
{
	mCombatModel.Reset();
}

UObjectModel* USRPGCombatSubsystem::GetModel_Internal() const
{
	return mCombatModel.Get();
}

UCombatUIModel* USRPGCombatSubsystem::GetCombatUIModel()
{
	if (mCombatUIModel == nullptr)
	{
		mCombatUIModel = NewObject<UCombatUIModel>(this);
	}
	return mCombatUIModel;
}
