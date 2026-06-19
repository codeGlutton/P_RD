#include "Component/ComponentModel.h"
#include "Actor/ActorModel.h"

UActorModel* UComponentModel::GetOwnerModel() const
{
	return GetTypedOuter<UActorModel>();
}