#include "Component/AttributeComponent/AttributeSetComponentModel.h"

UAttributeSetComponentModel::UAttributeSetComponentModel()
{

}

void UAttributeSetComponentModel::Initialize()
{
	Super::Initialize();
}

void UAttributeSetComponentModel::BeginPlay()
{
	Super::BeginPlay();
}

UUnitAttributeSet* UAttributeSetComponentModel::GetUnitAttributeSet() const
{
	return mUnitAttributeSet;
}