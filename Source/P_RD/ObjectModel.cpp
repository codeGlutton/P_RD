#include "ObjectModel.h"
#include "ObjectView.h"

#include "Simulation/Factory/ObjectModelFactory.h"

void UObjectModel::Initialize()
{
	checkf(mIsInitialized == false, TEXT("모델 중복 초기화"));
	mIsInitialized = true;
}

void UObjectModel::Uninitialize()
{
	checkf(mIsPendingDestroy == false, TEXT("모델 중복 삭제"));
	mIsPendingDestroy = true;
}

void UObjectModel::PostBindView(TScriptInterface<IObjectView> View)
{
	mView = View.GetObject();
}

void UObjectModel::FinishCreating(const FTransform& ViewTransform)
{
	GetWorldModelFactory(this)->FinishCreatingModel(this, ViewTransform);
}

void UObjectModel::Destroy()
{
	GetWorldModelFactory(this)->DestroyModel(this);
}

IObjectView* UObjectModel::GetView() const
{
	return Cast<IObjectView>(mView);
}
