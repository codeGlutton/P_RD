#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "Blueprint/UserWidget.h"

#include "Setting/GamePlaySettings.h"

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

void UWorldWidgetSubsystem::OpenWorldWidget(EWorldWidgetType WorldWidgetType, FOnEndUIOpenAnimation Callback)
{
	InitWorldWidget(WorldWidgetType);

	UUserWidget* WorldWidget = GetWorldWidget(WorldWidgetType);
	if (WorldWidget == nullptr)
	{
		UE_LOG(LogWorldWidget, Warning, TEXT("World widget is not available. Type: %d"), StaticCast<uint8>(WorldWidgetType));
		return;
	}

	IToggleableWidgetInterface* ToggleableWidget = Cast<IToggleableWidgetInterface>(WorldWidget);
	if (ToggleableWidget == nullptr)
	{
		UE_LOG(LogWorldWidget, Warning, TEXT("World widget does not implement ToggleableWidgetInterface. Type: %d"), StaticCast<uint8>(WorldWidgetType));
		return;
	}

	ToggleableWidget->OpenUI(MoveTemp(Callback));
}

void UWorldWidgetSubsystem::CloseWorldWidget(EWorldWidgetType WorldWidgetType, FOnEndUICloseAnimation Callback)
{
	UUserWidget* WorldWidget = GetWorldWidget(WorldWidgetType);
	if (WorldWidget == nullptr)
	{
		return;
	}

	IToggleableWidgetInterface* ToggleableWidget = Cast<IToggleableWidgetInterface>(WorldWidget);
	if (ToggleableWidget == nullptr)
	{
		UE_LOG(LogWorldWidget, Warning, TEXT("World widget does not implement ToggleableWidgetInterface. Type: %d"), StaticCast<uint8>(WorldWidgetType));
		return;
	}

	ToggleableWidget->CloseUI(MoveTemp(Callback));
}

bool UWorldWidgetSubsystem::IsWorldWidgetOpen(EWorldWidgetType WorldWidgetType) const
{
	const UUserWidget* WorldWidget = GetWorldWidget(WorldWidgetType);
	const IToggleableWidgetInterface* ToggleableWidget = Cast<IToggleableWidgetInterface>(WorldWidget);
	return ToggleableWidget != nullptr && ToggleableWidget->IsOpened();
}

void UWorldWidgetSubsystem::ShowWorldWidget(EWorldWidgetType WorldWidgetType, int32 ZOrder)
{
	(void)ZOrder;
	OpenWorldWidget(WorldWidgetType);
}

void UWorldWidgetSubsystem::HideWorldWidget(EWorldWidgetType WorldWidgetType)
{
	CloseWorldWidget(WorldWidgetType);
}

UUserWidget* UWorldWidgetSubsystem::GetWorldWidget(EWorldWidgetType Type) const
{
	return mWorldWidgets[StaticCast<uint8>(Type)];
}
