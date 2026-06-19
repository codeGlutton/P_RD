#include "Actor/ActorView.h"
#include "Component/ComponentView.h"

void IActorView::BindModel(UObjectModel* Model)
{
	AActor* SelfActor = Cast<AActor>(this);
	if (SelfActor == nullptr)
	{
		return;
	}

	TArray<UActorComponent*> ComponentArray = SelfActor->GetComponentsByInterface(UComponentView::StaticClass());
	for (UActorComponent* Component : ComponentArray)
	{
		IComponentView* ComponentView = Cast<IComponentView>(Component);
		ComponentView->BindOwnerModel(Model);
	}
}

void IActorView::UnbindModel(UObjectModel* Model)
{
	AActor* SelfActor = Cast<AActor>(this);
	if (SelfActor == nullptr)
	{
		return;
	}

	TArray<UActorComponent*> ComponentArray = SelfActor->GetComponentsByInterface(UComponentView::StaticClass());
	for (UActorComponent* Component : ComponentArray)
	{
		IComponentView* ComponentView = Cast<IComponentView>(Component);
		ComponentView->UnbindOwnerModel(Model);
	}
}

