#include "UI/RDUIInputSubsystem.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "Widgets/SViewport.h"

class FRDBackInputProcessor final : public IInputProcessor
{
public:
	explicit FRDBackInputProcessor(URDUIInputSubsystem* InOwner) : Owner(InOwner) {}
	void Tick(float, FSlateApplication&, TSharedRef<ICursor>) override {}
	bool HandleKeyDownEvent(FSlateApplication& App, const FKeyEvent& Event) override
	{
		if (!Owner.IsValid() || !URDUIInputSubsystem::IsBackKey(Event.GetKey())) return false;
		const auto Focused = App.GetKeyboardFocusedWidget();
		if (Focused && (Focused->GetType() == FName(TEXT("SEditableText"))
			|| Focused->GetType() == FName(TEXT("SMultiLineEditableText")))) return false;
		const auto* Client = Owner->GetGameInstance()->GetGameViewportClient();
		const auto Viewport = Client ? Client->GetGameViewportWidget() : nullptr;
		if (!Viewport || (!Viewport->HasKeyboardFocus() && !Viewport->HasFocusedDescendants())) return false;
		if (Event.IsRepeat()) return bConsumed;
		bConsumed = Owner->RouteBack();
		return bConsumed;
	}
	bool HandleKeyUpEvent(FSlateApplication&, const FKeyEvent& Event) override
	{
		if (!URDUIInputSubsystem::IsBackKey(Event.GetKey())) return false;
		const bool Result = bConsumed;
		bConsumed = false;
		return Result;
	}
private:
	TWeakObjectPtr<URDUIInputSubsystem> Owner;
	bool bConsumed = false;
};

void URDUIInputSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (FSlateApplication::IsInitialized())
	{
		Processor = MakeShared<FRDBackInputProcessor>(this);
		FSlateApplication::Get().RegisterInputPreProcessor(Processor);
	}
}

void URDUIInputSubsystem::Deinitialize()
{
	if (Processor && FSlateApplication::IsInitialized()) FSlateApplication::Get().UnregisterInputPreProcessor(Processor);
	Processor.Reset();
	Entries.Reset();
	Super::Deinitialize();
}

void URDUIInputSubsystem::Register(UUserWidget* Owner, TFunction<UUserWidget*()> Layer, TFunction<bool()> Handler)
{
	Unregister(Owner);
	Entries.Add({Owner, MoveTemp(Layer), MoveTemp(Handler)});
}

void URDUIInputSubsystem::Unregister(UUserWidget* Owner)
{
	Entries.RemoveAll([Owner](const FEntry& Entry) { return !Entry.Owner.IsValid() || Entry.Owner == Owner; });
}

bool URDUIInputSubsystem::IsBackKey(const FKey& Key)
{
	return Key == EKeys::Android_Back || Key == EKeys::Escape || Key == EKeys::Virtual_Gamepad_Back.GetVirtualKey();
}

bool URDUIInputSubsystem::RouteBack()
{
	UGameViewportSubsystem* Viewport = UGameViewportSubsystem::Get();
	if (!Viewport) return false;
	int32 TopIndex = INDEX_NONE, TopZ = MIN_int32;
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		const FEntry& Entry = Entries[Index];
		if (!Entry.Owner.IsValid()) continue;
		UUserWidget* Layer = Entry.Layer();
		if (!Layer || !Layer->IsInViewport() || !Layer->IsVisible() || !Layer->GetIsEnabled()) continue;
		const int32 Z = Viewport->GetWidgetSlot(Layer).ZOrder;
		if (Z >= TopZ) { TopIndex = Index; TopZ = Z; }
	}
	if (TopIndex == INDEX_NONE) return false;
	TArray<UUserWidget*> TopLevelWidgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetGameInstance(), TopLevelWidgets, UUserWidget::StaticClass(), true);
	for (UUserWidget* Widget : TopLevelWidgets)
		if (Widget && Widget->IsVisible() && Viewport->GetWidgetSlot(Widget).ZOrder > TopZ)
			return true; // A cinematic/loading layer owns the screen above our navigation owners.
	// The handler can remove/re-register widgets. Copy it before invoking it.
	const TFunction<bool()> Handler = Entries[TopIndex].Handler;
	Handler();
	// Never let an unclosable reward/transition fall through to the screen underneath.
	return true;
}
