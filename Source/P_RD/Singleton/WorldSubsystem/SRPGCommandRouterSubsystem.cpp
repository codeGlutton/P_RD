#include "Singleton/WorldSubsystem/SRPGCommandRouterSubsystem.h"
#include "Singleton/WorldSubsystem/SRPGCommandRouterModel.h"

void USRPGCommandRouterSubsystem::BindModel(UObjectModel* Model)
{
	mRouterModel = Cast<USRPGCommandRouterModel>(Model);
}

void USRPGCommandRouterSubsystem::UnbindModel(UObjectModel* Model)
{
	mRouterModel.Reset();
}

UObjectModel* USRPGCommandRouterSubsystem::GetModel_Internal() const
{
	return mRouterModel.Get();
}
