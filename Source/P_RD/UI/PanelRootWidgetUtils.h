#pragma once

#include "CoreMinimal.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"

namespace RDPanelWidgetUtils
{
	/**
	 * @brief WidgetTree에서 Name 위젯을 찾고, 없으면 새로 만들어 RootCanvas에 붙인 뒤 반환한다.
	 *        절차적 카드 패널 위젯들이 공유하는 헬퍼. 각 .cpp의 익명 네임스페이스에
	 *        동일 템플릿을 중복 정의하면 유니티 빌드에서 C2995로 충돌하므로 한 곳으로 모은다.
	 */
	template <typename T>
	T* FindOrCreateRootWidget(UWidgetTree* WidgetTree, UCanvasPanel* RootCanvas, FName Name)
	{
		if (WidgetTree == nullptr)
		{
			return nullptr;
		}

		if (T* ExistingWidget = Cast<T>(WidgetTree->FindWidget(Name)))
		{
			return ExistingWidget;
		}

		T* NewWidget = WidgetTree->ConstructWidget<T>(T::StaticClass(), Name);
		if (NewWidget != nullptr && RootCanvas != nullptr)
		{
			RootCanvas->AddChildToCanvas(NewWidget);
		}
		return NewWidget;
	}
}
