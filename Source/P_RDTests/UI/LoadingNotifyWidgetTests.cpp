#include "Editor.h"
#include "Misc/AutomationTest.h"
#include "UI/LoadingNotifyWidget.h"
#include "UI/RunOptionsRailWidget.h"
#include "Widgets/SWidget.h"
#include "UI/FadeInOutWidget.h"
#include "TimerManager.h"
#include "Slate/WidgetRenderer.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Engine/TextureRenderTarget2D.h"
#include "TextureResource.h"
#include "Brushes/SlateColorBrush.h"
#include "RHI.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLoadingNotifyOptionsRailVisibilityLifecycleTest,
	"P_RD.UI.LoadingNotify.OptionsRailVisibilityLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLoadingNotifyOptionsRailVisibilityLifecycleTest::RunTest(
	const FString& Parameters)
{
	UWorld* World = GEditor != nullptr
		? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("에디터 월드"), World))
	{
		return false;
	}

	URunOptionsRailWidget* VisibleRail =
		CreateWidget<URunOptionsRailWidget>(World, URunOptionsRailWidget::StaticClass());
	URunOptionsRailWidget* HiddenRail =
		CreateWidget<URunOptionsRailWidget>(World, URunOptionsRailWidget::StaticClass());
	URunOptionsRailWidget* CollapsedRail =
		CreateWidget<URunOptionsRailWidget>(World, URunOptionsRailWidget::StaticClass());
	ULoadingNotifyWidget* Loading =
		CreateWidget<ULoadingNotifyWidget>(World, ULoadingNotifyWidget::StaticClass());
	if (!TestNotNull(TEXT("표시 옵션 레일"), VisibleRail)
		|| !TestNotNull(TEXT("숨김 옵션 레일"), HiddenRail)
		|| !TestNotNull(TEXT("접힌 옵션 레일"), CollapsedRail)
		|| !TestNotNull(TEXT("로딩 알림"), Loading))
	{
		return false;
	}

	const TSharedRef<SWidget> VisibleRailSlate = VisibleRail->TakeWidget();
	const TSharedRef<SWidget> HiddenRailSlate = HiddenRail->TakeWidget();
	const TSharedRef<SWidget> CollapsedRailSlate = CollapsedRail->TakeWidget();
	const TSharedRef<SWidget> LoadingSlate = Loading->TakeWidget();
	(void)VisibleRailSlate;
	(void)HiddenRailSlate;
	(void)CollapsedRailSlate;
	(void)LoadingSlate;

	VisibleRail->SetVisibility(ESlateVisibility::Visible);
	HiddenRail->SetVisibility(ESlateVisibility::Hidden);
	CollapsedRail->SetVisibility(ESlateVisibility::Collapsed);
	Loading->SetVisibleDurationsForTest(0.0f, 0.0f, 0.0f);

	Loading->OpenUI();
	TestEqual(TEXT("로딩 중 표시 레일 숨김"), VisibleRail->GetVisibility(),
		ESlateVisibility::Collapsed);
	TestEqual(TEXT("로딩 중 Hidden 레일 숨김"), HiddenRail->GetVisibility(),
		ESlateVisibility::Collapsed);
	TestEqual(TEXT("원래 Collapsed 레일 유지"), CollapsedRail->GetVisibility(),
		ESlateVisibility::Collapsed);

	Loading->CloseUI();
	TestEqual(TEXT("닫힘 뒤 Visible 복구"), VisibleRail->GetVisibility(),
		ESlateVisibility::Visible);
	TestEqual(TEXT("닫힘 뒤 Hidden 복구"), HiddenRail->GetVisibility(),
		ESlateVisibility::Hidden);
	TestEqual(TEXT("닫힘 뒤 기존 Collapsed 유지"), CollapsedRail->GetVisibility(),
		ESlateVisibility::Collapsed);

	// A room can finish creating its independent rail after loading enters Complete.
	Loading->SetVisibleDurationsForTest(0.f, .5f, 0.f);
	Loading->OpenUI();
	Loading->CloseUI();
	auto* LateRail = CreateWidget<URunOptionsRailWidget>(World);
	const auto LateSlate = LateRail->TakeWidget();
	LateRail->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Loading->NativeTick(FGeometry(), .016f);
	TestEqual(TEXT("완료 표시 중 새로 만들어진 레일도 숨김"), LateRail->GetVisibility(), ESlateVisibility::Collapsed);
	World->GetTimerManager().ClearAllTimersForObject(Loading);
	Loading->FinishCompletedState();
	TestEqual(TEXT("완료 표시가 닫힌 뒤 새 레일 복구"), LateRail->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	Loading->NativeTick(FGeometry(), .016f);
	TestEqual(TEXT("닫힘 뒤에는 레일을 다시 숨기지 않음"), LateRail->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);

	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLoadingTransitionLayerTest,
	"P_RD.UI.LoadingNotify.TransitionCoversLateRail",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLoadingTransitionLayerTest::RunTest(const FString&)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	auto* Fade = CreateWidget<UFadeInOutWidget>(World);
	auto* Loading = CreateWidget<ULoadingNotifyWidget>(World);
	// Simulate existing WBPs retaining the old serialized priorities.
	Fade->mViewportZOrder = 20;
	Loading->mViewportZOrder = 30;
	TestTrue(TEXT("Transition fade covers rail, detail dialogs and room cinematics"), Fade->GetViewportZOrder() > 12000);
	TestTrue(TEXT("Loading message stays above transition fade"), Loading->GetViewportZOrder() > Fade->GetViewportZOrder());
	if (GUsingNullRHI) { AddError(TEXT("Layer paint regression requires RHI")); return false; }

	const FSlateColorBrush Red(FLinearColor::Red);
	const auto Canvas = SNew(SConstraintCanvas);
	Canvas->AddSlot().Anchors(FAnchors(0,0,1,1)).Offset(FMargin(0)).Alignment(FVector2D::ZeroVector)
		.ZOrder(Fade->GetViewportZOrder())[Fade->TakeWidget()];
	// Add the room rail AFTER the fade, as happens when loading finishes.
	Canvas->AddSlot().Anchors(FAnchors(0,0,1,1)).Offset(FMargin(0)).Alignment(FVector2D::ZeroVector)
		.ZOrder(10001)[SNew(SBorder).BorderImage(&Red)];
	Fade->OpenUI();
	Fade->StartFadeOut();
	Fade->NativeTick(FGeometry(), 1.f);
	FWidgetRenderer Renderer(true, false);
	auto CenterPixel = [&]()
	{
		UTextureRenderTarget2D* Target = Renderer.DrawWidget(Canvas, FVector2D(320,180));
		if (!TestNotNull(TEXT("Rendered transition"), Target)) return FColor::White;
		FlushRenderingCommands();
		TArray<FColor> Pixels;
		Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels);
		if (!TestEqual(TEXT("Rendered pixels"), Pixels.Num(), 320*180)) return FColor::White;
		return Pixels[90*320+160];
	};
	const FColor Covered = CenterPixel();
	TestTrue(TEXT("Newly attached rail remains behind opaque loading fade before any rail suppression tick"),
		Covered.R < 8 && Covered.G < 8 && Covered.B < 8);
	Fade->StartFadeIn();
	Fade->NativeTick(FGeometry(), 1.f);
	const FColor Revealed = CenterPixel();
	TestTrue(TEXT("Rail becomes visible once the fade finishes"), Revealed.R > 200 && Revealed.G < 8 && Revealed.B < 8);
	Fade->CloseUI();
	return !HasAnyErrors();
}

#endif
