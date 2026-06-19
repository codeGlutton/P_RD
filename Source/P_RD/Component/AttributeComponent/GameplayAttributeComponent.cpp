#include "Component/AttributeComponent/GameplayAttributeComponent.h"

UGameplayAttributeComponent::UGameplayAttributeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}

void UGameplayAttributeComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

void UGameplayAttributeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}
