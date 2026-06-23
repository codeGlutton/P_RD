#include "Singleton/WorldSubsystem/TacticalFrameworkSubsystem.h"
#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"

void UTacticalFrameworkSubsystem::BindModel(UObjectModel* Model)
{
	mFrameworkModel = Cast<UTacticalFrameworkModel>(Model);
}

void UTacticalFrameworkSubsystem::UnbindModel(UObjectModel* Model)
{
	mFrameworkModel.Reset();
}

UObjectModel* UTacticalFrameworkSubsystem::GetModel_Internal() const
{
	return mFrameworkModel.Get();
}
