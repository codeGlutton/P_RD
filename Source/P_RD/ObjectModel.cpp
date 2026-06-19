#include "ObjectModel.h"
#include "ObjectView.h"

void UObjectModel::Initialize()
{
}

void UObjectModel::Uninitialize()
{
}

void UObjectModel::PostBindView(TScriptInterface<IObjectView> View)
{
	mView = View.GetObject();
}

void UObjectModel::BeginPlay()
{
}

void UObjectModel::EndPlay()
{
}

IObjectView* UObjectModel::GetView() const
{
	return Cast<IObjectView>(mView);
}
