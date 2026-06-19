#include "Component/ComponentView.h"
#include "Actor/ActorView.h"

IActorView* IComponentView::GetOwnerView() const
{
	// 자신이 컴포넌트임을 확인
	const UActorComponent* SelfComponent = Cast<UActorComponent>(this);
	if (SelfComponent == nullptr)
	{
		return nullptr;
	}

	IActorView* ActorView = Cast<IActorView>(SelfComponent->GetOwner());
	return ActorView;
}

UObjectModel* IComponentView::GetOwnerModel() const
{
	// 부모 액터가 오브젝트 뷰임을 확인
	IActorView* ActorView = GetOwnerView();
	if (ActorView == nullptr)
	{
		return nullptr;
	}

	// 부모 액터의 모델을 반환
	return ActorView->GetModel();
}


