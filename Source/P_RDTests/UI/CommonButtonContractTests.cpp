#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Editor.h"
#include "Misc/AutomationTest.h"
#include "UI/RDUserWidget.h"
#include "UObject/UnrealType.h"
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
		{ TEXT("FrontendMap"), TEXT("/Game/UI/WorldMapLandscape/WBP_FrontendMapLandscape.WBP_FrontendMapLandscape_C") },
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

	bool IsCentered(const UTextBlock* Text)
	{
		const FByteProperty* Property = CastField<FByteProperty>(
			UTextLayoutWidget::StaticClass()->FindPropertyByName(TEXT("Justification")));
		return Text != nullptr && Property != nullptr
			&& Property->GetPropertyValue_InContainer(Text)
				== static_cast<uint8>(ETextJustify::Center);
	}

	void SplitProfiledButtonName(const FString& FullName, FString& OutBase,
		FString& OutSuffix)
	{
		OutBase = FullName;
		OutSuffix.Reset();
		const int32 Separator = FullName.Find(
			TEXT("__"), ESearchCase::CaseSensitive, ESearchDir::FromStart);
		if (Separator != INDEX_NONE)
		{
			OutBase = FullName.Left(Separator);
			OutSuffix = FullName.Mid(Separator);
		}
	}

	bool IsExactButtonLabelName(const FString& ButtonName, const FString& WidgetName)
	{
		FString Base;
		FString Suffix;
		SplitProfiledButtonName(ButtonName, Base, Suffix);
		return WidgetName == Base + TEXT("Text") + Suffix
			|| WidgetName == Base + TEXT("Label") + Suffix;
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
		int32 ScreenAuditedButtonCount = 0;
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
			[this, &AuditedButtonCount, &ScreenAuditedButtonCount, &Screen, Widget](UWidget* Child)
			{
				UButton* Button = Cast<UButton>(Child);
				if (Button == nullptr)
				{
					return;
				}
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
				++AuditedButtonCount;
				++ScreenAuditedButtonCount;

				// 일반 버튼은 Clicked, 상세용 길게 누르기 버튼은 Pressed+Released가
				// 실제 동작 계약이다. 둘 다 없으면 화면에 있어도 절대 반응하지 않는다.
				const bool bClickAction = Button->OnClicked.IsBound();
				const bool bPressAction = Button->OnPressed.IsBound()
					&& Button->OnReleased.IsBound();
				// Title/Settings는 공통 눌림 피드백이 Pressed+Released를 자동으로
				// 연결한다. 이 두 화면에서는 반드시 실제 Clicked 동작이 있어야 한다.
				const bool bRequiresClickAction = FCString::Strcmp(Screen.Label, TEXT("Title")) == 0
					|| FCString::Strcmp(Screen.Label, TEXT("Settings")) == 0;
				if (!bClickAction && (bRequiresClickAction || !bPressAction))
				{
					AddError(FString::Printf(TEXT("%s: button '%s' has no action delegate"),
						Screen.Label, *Name));
				}
			});
		TestTrue(*FString::Printf(TEXT("%s 기능 버튼을 실제로 검사함"), Screen.Label),
			ScreenAuditedButtonCount > 0);
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

		// 공통 정규화가 소유하는 정확한 이름의 라벨만 중앙 정렬됐는지 본다.
		// sibling Canvas 위치는 화면별 builder/responsive layout의 책임이다.
		TArray<UButton*> Buttons;
		Widget->WidgetTree->ForEachWidget([&Buttons](UWidget* Child)
			{
				if (UButton* Button = Cast<UButton>(Child))
				{
					Buttons.Add(Button);
				}
			});
		for (UButton* Button : Buttons)
		{
			Widget->WidgetTree->ForEachWidget(
				[this, &Screen, Button](UWidget* Child)
				{
					UTextBlock* Text = Cast<UTextBlock>(Child);
					if (Text == nullptr
						|| !IsExactButtonLabelName(Button->GetName(), Text->GetName()))
					{
						return;
					}
					if (!IsCentered(Text))
					{
						AddError(FString::Printf(TEXT("%s: paired label '%s' is not centered"),
							Screen.Label, *Text->GetName()));
					}
				});
		}
	}

	TestTrue(TEXT("메인 화면 장식 레이어를 실제로 검사함"),
		AuditedDecorationCount > 100);
	return !HasAnyErrors();
}

#endif
