#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Widgets/SWidget.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

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
	const TSharedRef<SWidget> TitleSlate = Title->TakeWidget();

	for (const TCHAR* RowName : { TEXT("StartButton"), TEXT("ContinueButton"),
		TEXT("SettingsButton"), TEXT("ExitButton") })
	{
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

#endif
