#include "Actor/ActorModel.h"
#include "Component/ComponentModel.h"

void UActorModel::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		ResetComponentModels();
	}
}

void UActorModel::PostLoadSubobjects(FObjectInstancingGraph* OuterInstanceGraph)
{
	Super::PostLoadSubobjects(OuterInstanceGraph);
	ResetComponentModels();
}

void UActorModel::ResetComponentModels()
{
	mComponentModels.Reset();

	ForEachObjectWithOuter(this, [this](UObject* Child)
		{
			UComponentModel* ComponentModel = Cast<UComponentModel>(Child);
			if (ComponentModel && ComponentModel->GetOwnerModel() == this)
			{
				mComponentModels.Add(ComponentModel);
			}
		}, true, RF_NoFlags, EInternalObjectFlags::Garbage);
}

void UActorModel::Initialize()
{
	Super::Initialize();

	PreInitializeComponentModels();
	InitializeComponentModels();
	PostInitializeComponentModels();
}

void UActorModel::Uninitialize()
{
	Super::Uninitialize();

	UninitializeComponentModels();
}

void UActorModel::InitializeComponentModels()
{
	for (UComponentModel* ComponentModel : mComponentModels)
	{
		if (ComponentModel != nullptr)
		{
			ComponentModel->Initialize();
		}
	}
}

void UActorModel::UninitializeComponentModels()
{
	for (UComponentModel* ComponentModel : mComponentModels)
	{
		if (ComponentModel != nullptr)
		{
			ComponentModel->Uninitialize();
		}
	}
}

UComponentModel* UActorModel::AddComponentModelByClass(UClass* ComponentModelClass)
{
	UComponentModel* ComponentModel = NewObject<UComponentModel>(this, ComponentModelClass);
	mComponentModels.Add(ComponentModel);

	return ComponentModel;
}

UComponentModel* UActorModel::FindComponentModelByInterface(UClass* InterfaceClass) const
{
	UComponentModel* FoundComponentModel = nullptr;

	if (InterfaceClass != nullptr)
	{
		for (UComponentModel* ComponentModel : mComponentModels)
		{
			if (ComponentModel != nullptr && ComponentModel->GetClass()->ImplementsInterface(InterfaceClass))
			{
				FoundComponentModel = ComponentModel;
				break;
			}
		}
	}

	return FoundComponentModel;
}

TArray<UComponentModel*> UActorModel::FindComponentModelsByInterface(UClass* InterfaceClass) const
{
	TArray<UComponentModel*> FoundComponentModels;

	if (InterfaceClass != nullptr)
	{
		for (UComponentModel* ComponentModel : mComponentModels)
		{
			if (ComponentModel != nullptr && ComponentModel->GetClass()->ImplementsInterface(InterfaceClass))
			{
				FoundComponentModels.Add(ComponentModel);
			}
		}
	}

	return FoundComponentModels;
}

UComponentModel* UActorModel::FindComponentModelByClass(TSubclassOf<UComponentModel> ComponentModelClass) const
{
	UComponentModel* FoundComponentModel = nullptr;

	if (UClass* TargetClass = ComponentModelClass.Get())
	{
		for (UComponentModel* ComponentModel : mComponentModels)
		{
			if (ComponentModel != nullptr && ComponentModel->IsA(TargetClass) == true)
			{
				FoundComponentModel = ComponentModel;
				break;
			}
		}
	}

	return FoundComponentModel;
}

TArray<UComponentModel*> UActorModel::FindComponentModelsByClass(TSubclassOf<UComponentModel> ComponentModelClass) const
{
	TArray<UComponentModel*> FoundComponentModels;

	if (UClass* TargetClass = ComponentModelClass.Get())
	{
		for (UComponentModel* ComponentModel : mComponentModels)
		{
			if (ComponentModel != nullptr && ComponentModel->IsA(TargetClass) == true)
			{
				FoundComponentModels.Add(ComponentModel);
			}
		}
	}

	return FoundComponentModels;
}

const TSet<TObjectPtr<UComponentModel>>& UActorModel::GetComponentModels() const
{
	return mComponentModels;
}
