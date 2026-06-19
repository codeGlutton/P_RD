#include "Singleton/WorldSubsystem/SRPGCombatSubsystem.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"

DEFINE_LOG_CATEGORY(LogSRPGCombat)

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
}

UObjectModel* USRPGCombatSubsystem::GetModel_Internal() const
{
	return mCombatModel.Get();
}
