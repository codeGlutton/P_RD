#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "Blueprint/UserWidget.h"

#include "Setting/GamePlaySettings.h"
#include "Singleton/WorldSubsystem/WorldWidgetTransitionInterface.h"

DEFINE_LOG_CATEGORY(LogWorldWidget)

void UWorldWidgetSubsystem::InitWidgets(UClass* HUDClass, const TArray<EWorldWidgetType>& WorldWidgetTypes)
{
	InitHUD(HUDClass);
	for (auto& WorldWidgetType : WorldWidgetTypes)
	{
		InitWorldWidget(WorldWidgetType);
	}
}

void UWorldWidgetSubsystem::InitHUD(UClass* HUDClass)
{
	if (HUDClass == nullptr)
	{
		UE_LOG(LogWorldWidget, Warning, TEXT("HUD widget class is not configured."));
		return;
	}

	mHUD = CreateWidget(GetWorld()->GetFirstPlayerController(), HUDClass);
}

void UWorldWidgetSubsystem::InitWorldWidget(EWorldWidgetType WorldWidgetType)
{
	const uint8 Index = StaticCast<uint8>(WorldWidgetType);
	if (mWorldWidgets[Index] == nullptr)
	{
		const TSubclassOf<UUserWidget> WorldWidgetClass = GetDefault<UGamePlaySettings>()->mWorldWidgetClasses[Index];
		if (WorldWidgetClass == nullptr)
		{
			UE_LOG(LogWorldWidget, Warning, TEXT("World widget class is not configured. Type: %d"), Index);
			return;
		}

		mWorldWidgets[Index] = CreateWidget(GetWorld()->GetFirstPlayerController(), WorldWidgetClass);
	}
}

void UWorldWidgetSubsystem::OpenWorldWidget(EWorldWidgetType WorldWidgetType, int32 ZOrder)
{
	InitWorldWidget(WorldWidgetType);

	UUserWidget* WorldWidget = GetWorldWidget(WorldWidgetType);
	if (WorldWidget == nullptr)
	{
		UE_LOG(LogWorldWidget, Warning, TEXT("World widget is not available. Type: %d"), StaticCast<uint8>(WorldWidgetType));
		return;
	}

	const uint8 Index = StaticCast<uint8>(WorldWidgetType);
	EWorldWidgetLifecycleState& WidgetState = mWorldWidgetStates[Index];
	if (WidgetState == EWorldWidgetLifecycleState::Open && WorldWidget->IsInViewport() && WorldWidget->IsVisible())
	{
		return;
	}

	if (WorldWidget->IsInViewport() == false)
	{
		WorldWidget->AddToViewport(ZOrder);
	}

	WidgetState = EWorldWidgetLifecycleState::Open;
	WorldWidget->SetVisibility(ESlateVisibility::Visible);

	if (WorldWidget->GetClass()->ImplementsInterface(UWorldWidgetTransitionInterface::StaticClass()))
	{
		IWorldWidgetTransitionInterface::Execute_HandleWorldWidgetOpened(WorldWidget, WorldWidgetType);
	}
}

void UWorldWidgetSubsystem::CloseWorldWidget(EWorldWidgetType WorldWidgetType)
{
	UUserWidget* WorldWidget = GetWorldWidget(WorldWidgetType);
	if (WorldWidget == nullptr)
	{
		return;
	}

	EWorldWidgetLifecycleState& WidgetState = mWorldWidgetStates[StaticCast<uint8>(WorldWidgetType)];
	if (WidgetState == EWorldWidgetLifecycleState::Closed || WidgetState == EWorldWidgetLifecycleState::Closing)
	{
		return;
	}

	WidgetState = EWorldWidgetLifecycleState::Closing;

	if (WorldWidget->GetClass()->ImplementsInterface(UWorldWidgetTransitionInterface::StaticClass())
		&& IWorldWidgetTransitionInterface::Execute_HandleWorldWidgetCloseRequested(WorldWidget, WorldWidgetType))
	{
		return;
	}

	CompleteCloseWorldWidget(WorldWidgetType);
}

void UWorldWidgetSubsystem::CompleteCloseWorldWidget(EWorldWidgetType WorldWidgetType)
{
	UUserWidget* WorldWidget = GetWorldWidget(WorldWidgetType);
	if (WorldWidget == nullptr)
	{
		return;
	}

	EWorldWidgetLifecycleState& WidgetState = mWorldWidgetStates[StaticCast<uint8>(WorldWidgetType)];
	if (WidgetState != EWorldWidgetLifecycleState::Closing)
	{
		return;
	}

	WorldWidget->SetVisibility(ESlateVisibility::Collapsed);
	WidgetState = EWorldWidgetLifecycleState::Closed;

	if (WorldWidget->GetClass()->ImplementsInterface(UWorldWidgetTransitionInterface::StaticClass()))
	{
		IWorldWidgetTransitionInterface::Execute_HandleWorldWidgetClosed(WorldWidget, WorldWidgetType);
	}
}

bool UWorldWidgetSubsystem::IsWorldWidgetOpen(EWorldWidgetType WorldWidgetType) const
{
	const UUserWidget* WorldWidget = GetWorldWidget(WorldWidgetType);
	const EWorldWidgetLifecycleState WidgetState = mWorldWidgetStates[StaticCast<uint8>(WorldWidgetType)];
	return WidgetState == EWorldWidgetLifecycleState::Open
		&& WorldWidget != nullptr
		&& WorldWidget->IsInViewport()
		&& WorldWidget->IsVisible();
}

void UWorldWidgetSubsystem::ShowWorldWidget(EWorldWidgetType WorldWidgetType, int32 ZOrder)
{
	OpenWorldWidget(WorldWidgetType, ZOrder);
}

void UWorldWidgetSubsystem::HideWorldWidget(EWorldWidgetType WorldWidgetType)
{
	CloseWorldWidget(WorldWidgetType);
}

UUserWidget* UWorldWidgetSubsystem::GetWorldWidget(EWorldWidgetType Type) const
{
	return mWorldWidgets[StaticCast<uint8>(Type)];
}
