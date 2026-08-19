#pragma once

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/UserWidget.h"

namespace RDDetailOverlay
{
	/**
	 * 공용 상세 WBP의 기존 DetailCloseCatch에는 과거 Blueprint 클릭 이벤트가
	 * 남아 있을 수 있다. 그 버튼은 완전히 접고, 아무 델리게이트도 없는 런타임
	 * 버튼을 같은 부모의 최하단에 전 화면 앵커로 배치한다. 따라서 빈 영역은
	 * 입력을 삼키지만 상세 종료는 절대로 발생하지 않는다.
	 */
	inline UButton* EnsureModalInputShield(UUserWidget* Overlay)
	{
		if (Overlay == nullptr || Overlay->WidgetTree == nullptr)
		{
			return nullptr;
		}

		UButton* LegacyCatch = Cast<UButton>(
			Overlay->GetWidgetFromName(TEXT("DetailCloseCatch")));
		if (LegacyCatch == nullptr)
		{
			return nullptr;
		}
		LegacyCatch->OnClicked.Clear();
		LegacyCatch->SetVisibility(ESlateVisibility::Collapsed);

		UCanvasPanel* ParentCanvas = Cast<UCanvasPanel>(LegacyCatch->GetParent());
		if (ParentCanvas == nullptr)
		{
			ParentCanvas = Cast<UCanvasPanel>(Overlay->WidgetTree->RootWidget);
		}
		if (ParentCanvas == nullptr)
		{
			return nullptr;
		}

		UButton* Shield = Cast<UButton>(
			Overlay->GetWidgetFromName(TEXT("RuntimeDetailModalInputShield")));
		if (Shield == nullptr)
		{
			Shield = Overlay->WidgetTree->ConstructWidget<UButton>(
				UButton::StaticClass(), TEXT("RuntimeDetailModalInputShield"));
			Shield->SetStyle(LegacyCatch->GetStyle());
			ParentCanvas->AddChildToCanvas(Shield);
		}

		if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Shield->Slot))
		{
			Slot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			Slot->SetOffsets(FMargin(0.f));
			Slot->SetAlignment(FVector2D::ZeroVector);
			// 상세 프레임과 닫기 버튼은 위에 남고, 그 밖의 모든 화면만 받는다.
			Slot->SetZOrder(-1000);
		}
		Shield->OnClicked.Clear();
		Shield->SetIsEnabled(true);
		Shield->SetTouchMethod(EButtonTouchMethod::PreciseTap);
		Shield->SetClickMethod(EButtonClickMethod::PreciseClick);
		Shield->SetVisibility(ESlateVisibility::Visible);
		return Shield;
	}
}
