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
		UE_LOG(LogWorldWidget, Warning, TEXT("HUD 위젯 클래스가 등록되지 않음"));
		return;
	}

	UE_LOG(LogWorldWidget, Warning, TEXT("HUD 위젯 객체 생성"));
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
			UE_LOG(LogWorldWidget, Warning, TEXT("[%s] 월드 위젯 클래스가 지정되지 않음"), *EnumToString(WorldWidgetType));
			return;
		}

		UE_LOG(LogWorldWidget, Warning, TEXT("[%s] 월드 위젯 객체 생성"), *EnumToString(WorldWidgetType));
		mWorldWidgets[Index] = CreateWidget(GetWorld()->GetFirstPlayerController(), WorldWidgetClass);
	}
}

UUserWidget* UWorldWidgetSubsystem::GetWorldWidget(EWorldWidgetType Type) const
{
	return mWorldWidgets[StaticCast<uint8>(Type)];
}
