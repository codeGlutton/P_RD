#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/TextBlock.h"
#include "Editor.h"
#include "Misc/AutomationTest.h"
#include "UI/RDUserWidget.h"
#include "Widgets/SWidget.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

namespace
{
	struct FScreenContract
	{
		const TCHAR* Label;
		const TCHAR* ClassPath;
	};

	constexpr FScreenContract MainScreens[] =
	{
		{ TEXT("Title"), TEXT("/Game/UI/WBP_TitleMenu.WBP_TitleMenu_C") },
		{ TEXT("Settings"), TEXT("/Game/UI/WBP_SettingsPanel.WBP_SettingsPanel_C") },
		{ TEXT("FrontendMap"), TEXT("/Game/UI/WBP_FrontendMap.WBP_FrontendMap_C") },
		{ TEXT("MercenaryHire"), TEXT("/Game/UI/CombatLayouts/WBP_MercenaryHire_Marchbound.WBP_MercenaryHire_Marchbound_C") },
		{ TEXT("CombatHUD"), TEXT("/Game/UI/CombatLayouts/WBP_CombatHUD04.WBP_CombatHUD04_C") },
		{ TEXT("Shop"), TEXT("/Game/UI/Shop/WBP_Shop_FullGenerated.WBP_Shop_FullGenerated_C") },
		{ TEXT("Treasure"), TEXT("/Game/UI/Treasure/WBP_Treasure.WBP_Treasure_C") },
		{ TEXT("Reward"), TEXT("/Game/UI/RewardConcept03New/WBP_RewardConcept03_Frameless.WBP_RewardConcept03_Frameless_C") },
		{ TEXT("Defeat"), TEXT("/Game/UI/CombatResult/WBP_CombatDefeat.WBP_CombatDefeat_C") },
		{ TEXT("Settlement"), TEXT("/Game/UI/RewardSettlement/WBP_RewardSettlement_Runtime.WBP_RewardSettlement_Runtime_C") },
	};

	bool IsIntentionalInputShield(const FString& Name)
	{
		return Name.Contains(TEXT("InputBlocker"))
			|| Name.Contains(TEXT("InputShield"))
			|| Name.Contains(TEXT("CloseCatch"));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommonButtonBindingContractTest,
	"P_RD.UI.Common.AllMainScreenButtonsBound",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommonButtonBindingContractTest::RunTest(const FString& Parameters)
{
	UWorld* World = GEditor != nullptr
		? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("에디터 월드"), World))
	{
		return false;
	}

	int32 AuditedButtonCount = 0;
	for (const FScreenContract& Screen : MainScreens)
	{
		UClass* WidgetClass = LoadClass<UUserWidget>(nullptr, Screen.ClassPath);
		if (!TestNotNull(*FString::Printf(TEXT("%s WBP"), Screen.Label), WidgetClass))
		{
			continue;
		}
		UUserWidget* Widget = CreateWidget<UUserWidget>(World, WidgetClass);
		if (!TestNotNull(*FString::Printf(TEXT("%s instance"), Screen.Label), Widget))
		{
			continue;
		}
		const TSharedRef<SWidget> SlateWidget = Widget->TakeWidget();
		TestTrue(*FString::Printf(TEXT("%s Slate"), Screen.Label),
			SlateWidget->GetType() != NAME_None);

		if (Widget->WidgetTree == nullptr)
		{
			AddError(FString::Printf(TEXT("%s: WidgetTree missing"), Screen.Label));
			continue;
		}

		Widget->WidgetTree->ForEachWidgetAndDescendants(
			[this, &AuditedButtonCount, &Screen, Widget](UWidget* Child)
			{
				UButton* Button = Cast<UButton>(Child);
				if (Button == nullptr)
				{
					return;
				}
				++AuditedButtonCount;
				const FString Name = Button->GetName();
				if (IsIntentionalInputShield(Name))
				{
					return;
				}
				// HUD에는 디자이너 비교용 WBP_MercenaryPanel 복제본이 영구
				// Collapsed 상태로 남아 있다. 실제 런타임은 같은 이름의 인라인 판을
				// 사용하므로, 표시될 수 없는 비교용 복제 버튼은 동작 계약 대상이 아니다.
				if (const UUserWidget* Owner = Button->GetTypedOuter<UUserWidget>();
					Owner != nullptr && Owner != Widget
					&& Owner->GetName().StartsWith(TEXT("MercenaryPanelHost"))
					&& Owner->GetVisibility() == ESlateVisibility::Collapsed)
				{
					return;
				}

				// 일반 버튼은 Clicked, 상세용 길게 누르기 버튼은 Pressed+Released가
				// 실제 동작 계약이다. 둘 다 없으면 화면에 있어도 절대 반응하지 않는다.
				const bool bClickAction = Button->OnClicked.IsBound();
				const bool bPressAction = Button->OnPressed.IsBound()
					&& Button->OnReleased.IsBound();
				if (!bClickAction && !bPressAction)
				{
					AddError(FString::Printf(TEXT("%s: button '%s' has no action delegate"),
						Screen.Label, *Name));
				}
			});
	}

	TestTrue(TEXT("메인 화면 버튼을 실제로 검사함"), AuditedButtonCount > 30);
	AddInfo(FString::Printf(TEXT("Audited %d button instances across main screens"),
		AuditedButtonCount));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommonPassiveLayerContractTest,
	"P_RD.UI.Common.PassiveLayersDoNotBlockInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommonPassiveLayerContractTest::RunTest(const FString& Parameters)
{
	UWorld* World = GEditor != nullptr
		? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("에디터 월드"), World))
	{
		return false;
	}

	int32 AuditedDecorationCount = 0;
	for (const FScreenContract& Screen : MainScreens)
	{
		UClass* WidgetClass = LoadClass<UUserWidget>(nullptr, Screen.ClassPath);
		if (WidgetClass == nullptr || !WidgetClass->IsChildOf(URDUserWidget::StaticClass()))
		{
			// RunOptionsRail 같은 직접 UUserWidget 계열은 공통 베이스 계약 밖이다.
			continue;
		}
		URDUserWidget* Widget = CreateWidget<URDUserWidget>(World, WidgetClass);
		if (Widget == nullptr)
		{
			AddError(FString::Printf(TEXT("%s: instance creation failed"), Screen.Label));
			continue;
		}
		Widget->TakeWidget();
		// 실제 게임에서는 NativeConstruct 뒤 다음 틱에 같은 재검사가 돈다.
		// 자동화는 월드를 진행시키지 않으므로 여기서 동일 경로를 즉시 호출한다.
		Widget->NormalizeCommonUIContractForTest();
		Widget->WidgetTree->ForEachWidget(
			[this, &AuditedDecorationCount, &Screen](UWidget* Child)
			{
				if (Child == nullptr
					|| (!Child->IsA<UImage>() && !Child->IsA<UTextBlock>()))
				{
					return;
				}
				if (Child->GetVisibility() == ESlateVisibility::Collapsed
					|| Child->GetVisibility() == ESlateVisibility::Hidden)
				{
					return;
				}
				++AuditedDecorationCount;
				if (Child->GetVisibility() != ESlateVisibility::HitTestInvisible)
				{
					AddError(FString::Printf(TEXT("%s: decoration '%s' still blocks hit testing"),
						Screen.Label, *Child->GetName()));
				}
			});
	}

	TestTrue(TEXT("메인 화면 장식 레이어를 실제로 검사함"),
		AuditedDecorationCount > 100);
	return !HasAnyErrors();
}

/**
 * @brief 타이틀 EXIT 줄이 실제로 눌리는지 본다.
 *
 * @details 저작 WBP는 EXIT 줄에 테두리와 글자만 두고 버튼을 빠뜨려,
 * 화면에는 보이는데 눌러도 아무 일이 없었다(0824 검수). 위의 전수 감사는
 * "있는 버튼이 배선됐나"만 보므로 없는 버튼은 잡지 못한다. 이 시험은
 * 네 줄 모두에 대해 **버튼의 존재와 배선을 함께** 요구한다.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTitleMenuRowButtonContractTest,
	"P_RD.UI.Title.MenuRowsClickable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTitleMenuRowButtonContractTest::RunTest(const FString& Parameters)
{
	UWorld* World = GEditor != nullptr
		? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("에디터 월드"), World))
	{
		return false;
	}

	UClass* WidgetClass = LoadClass<UUserWidget>(nullptr,
		TEXT("/Game/UI/WBP_TitleMenu.WBP_TitleMenu_C"));
	if (!TestNotNull(TEXT("타이틀 WBP"), WidgetClass))
	{
		return false;
	}
	UUserWidget* Title = CreateWidget<UUserWidget>(World, WidgetClass);
	if (!TestNotNull(TEXT("타이틀 인스턴스"), Title))
	{
		return false;
	}
	// 반환된 Slate 참조를 붙들고 있어야 한다. 놓으면 그 자리에서 위젯이
	// 헐리며 NativeDestruct가 배선을 도로 끊는다.
	const TSharedRef<SWidget> TitleSlate = Title->TakeWidget();

	for (const TCHAR* RowName : { TEXT("StartButton"), TEXT("ContinueButton"),
		TEXT("SettingsButton"), TEXT("ExitButton") })
	{
		// 저작 자산은 프로필 접미사를 쓴다. 구형 자산의 접미사 없는 이름도
		// 같은 계약으로 인정한다 -- 둘 중 하나만 있으면 된다.
		UButton* Button = Cast<UButton>(Title->GetWidgetFromName(
			FName(*FString::Printf(TEXT("%s__base_16_9"), RowName))));
		if (Button == nullptr)
		{
			Button = Cast<UButton>(Title->GetWidgetFromName(FName(RowName)));
		}
		if (Button == nullptr)
		{
			AddError(FString::Printf(
				TEXT("Title: menu row '%s' has no button widget"), RowName));
			continue;
		}
		if (Button->OnClicked.IsBound() == false)
		{
			AddError(FString::Printf(
				TEXT("Title: menu row '%s' button is not bound"), RowName));
		}
	}

	return !HasAnyErrors();
}

/**
 * @brief 글자 배율 상자가 글리프를 세로로 자르지 않는지 본다.
 *
 * @details ``Xxx_AutoFit`` / ``Xxx_Fit`` 은 ScaleToFitX(또는 ScaleToFit) +
 * DownOnly 라 가로로는 넘칠 수 없다. 그런데도 ClipToBounds 가 걸리면 저작된
 * 칸 높이가 글꼴 줄 높이와 비슷할 때 아래꼬리와 외곽선이 잘린다(0824 검수:
 * "글자 짤리는거"). 실제 게임 경로(NativeConstruct)를 그대로 태워 검사한다.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommonTextClippingContractTest,
	"P_RD.UI.Common.AutoFitTextIsNotClipped",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommonTextClippingContractTest::RunTest(const FString& Parameters)
{
	UWorld* World = GEditor != nullptr
		? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("에디터 월드"), World))
	{
		return false;
	}

	int32 AuditedScaleBoxCount = 0;
	for (const FScreenContract& Screen : MainScreens)
	{
		UClass* WidgetClass = LoadClass<UUserWidget>(nullptr, Screen.ClassPath);
		if (WidgetClass == nullptr
			|| !WidgetClass->IsChildOf(URDUserWidget::StaticClass()))
		{
			continue;
		}
		URDUserWidget* Widget = CreateWidget<URDUserWidget>(World, WidgetClass);
		if (Widget == nullptr || Widget->WidgetTree == nullptr)
		{
			AddError(FString::Printf(TEXT("%s: instance creation failed"), Screen.Label));
			continue;
		}
		// Slate 참조를 붙들어야 NativeConstruct 결과가 살아 있다.
		const TSharedRef<SWidget> Slate = Widget->TakeWidget();
		Widget->NormalizeCommonUIContractForTest();

		Widget->WidgetTree->ForEachWidgetAndDescendants(
			[this, &AuditedScaleBoxCount, &Screen](UWidget* Child)
			{
				UScaleBox* Scale = Cast<UScaleBox>(Child);
				if (Scale == nullptr)
				{
					return;
				}
				const FString Name = Scale->GetName();
				if (!Name.EndsWith(TEXT("_AutoFit")) && !Name.EndsWith(TEXT("_Fit")))
				{
					return;
				}
				++AuditedScaleBoxCount;
				if (Scale->GetClipping() != EWidgetClipping::Inherit)
				{
					AddError(FString::Printf(
						TEXT("%s: text scale box '%s' still clips glyphs"),
						Screen.Label, *Name));
				}
			});
	}

	TestTrue(TEXT("글자 배율 상자를 실제로 검사함"), AuditedScaleBoxCount > 20);
	AddInfo(FString::Printf(TEXT("Audited %d text scale boxes"), AuditedScaleBoxCount));
	return !HasAnyErrors();
}

#endif
