#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Widget.h"
#include "Editor.h"
#include "Misc/AutomationTest.h"
#include "UI/TitleMenuWidget.h"
#include "Widgets/SWidget.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

namespace
{
	UWidget* FindNearestCanvasMountedAncestor(UWidget* Widget)
	{
		for (UWidget* Node = Widget != nullptr ? Widget->GetParent() : nullptr;
			Node != nullptr; Node = Node->GetParent())
		{
			if (Cast<UCanvasPanelSlot>(Node->Slot) != nullptr)
			{
				return Node;
			}
		}
		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTitleMenuInputLayerContractTest,
	"P_RD.UI.TitleMenu.InputLayerContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTitleMenuInputLayerContractTest::RunTest(const FString& Parameters)
{
	UClass* TitleClass = LoadClass<UTitleMenuWidget>(nullptr,
		TEXT("/Game/UI/WBP_TitleMenu.WBP_TitleMenu_C"));
	if (!TestNotNull(TEXT("타이틀 WBP 클래스"), TitleClass))
	{
		return false;
	}

	UWorld* World = GEditor != nullptr
		? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("에디터 월드"), World))
	{
		return false;
	}

	UTitleMenuWidget* Title = CreateWidget<UTitleMenuWidget>(World, TitleClass);
	if (!TestNotNull(TEXT("타이틀 위젯 인스턴스"), Title))
	{
		return false;
	}
	const TSharedRef<SWidget> TitleSlate = Title->TakeWidget();
	TestTrue(TEXT("타이틀 Slate 생성"), TitleSlate->GetType() != NAME_None);

	UWidgetTree* Tree = Title->WidgetTree;
	if (!TestNotNull(TEXT("타이틀 위젯 트리"), Tree))
	{
		return false;
	}

	UWidget* Logo = Tree->FindWidget(TEXT("TitleLogoImage__base_16_9"));
	UWidget* LogoMount = FindNearestCanvasMountedAncestor(Logo);
	if (TestNotNull(TEXT("프로필 타이틀 로고"), Logo))
	{
		TestEqual(TEXT("로고는 입력 비대상"), Logo->GetVisibility(),
			ESlateVisibility::HitTestInvisible);
	}
	if (TestNotNull(TEXT("로고가 속한 레이아웃 마운트"), LogoMount))
	{
		TestEqual(TEXT("레이아웃 마운트는 자식 버튼 입력 허용"),
			LogoMount->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	}

	const TCHAR* ButtonBaseNames[] =
	{
		TEXT("StartButton"),
		TEXT("SettingsButton"),
		TEXT("ExitButton"),
	};
	for (const TCHAR* ButtonBaseName : ButtonBaseNames)
	{
		const FString ButtonName = FString::Printf(TEXT("%s__base_16_9"), ButtonBaseName);
		UButton* Button = Cast<UButton>(Tree->FindWidget(FName(ButtonName)));
		if (TestNotNull(*FString::Printf(TEXT("%s 실제 버튼"), ButtonBaseName), Button))
		{
			TestEqual(*FString::Printf(TEXT("%s 버튼은 입력 가능"), ButtonBaseName),
				Button->GetVisibility(), ESlateVisibility::Visible);
			TestTrue(*FString::Printf(TEXT("%s 버튼 활성"), ButtonBaseName),
				Button->GetIsEnabled());
			TestTrue(*FString::Printf(TEXT("%s 클릭 이벤트 연결"), ButtonBaseName),
				Button->OnClicked.IsBound());
		}

		for (const TCHAR* DecorationSuffix : { TEXT("FrameImage"), TEXT("Text") })
		{
			const FString DecorationName = FString::Printf(TEXT("%s%s__base_16_9"),
				ButtonBaseName, DecorationSuffix);
			UWidget* Decoration = Tree->FindWidget(FName(DecorationName));
			if (TestNotNull(*FString::Printf(TEXT("%s 장식"), *DecorationName), Decoration))
			{
				TestEqual(*FString::Printf(TEXT("%s 장식은 입력 비대상"), *DecorationName),
					Decoration->GetVisibility(), ESlateVisibility::HitTestInvisible);
			}
		}
	}

	return true;
}

#endif
