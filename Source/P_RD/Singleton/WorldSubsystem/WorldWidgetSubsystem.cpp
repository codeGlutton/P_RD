#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "Blueprint/UserWidget.h"

#include "Setting/GamePlaySettings.h"

DEFINE_LOG_CATEGORY(LogWorldWidget)

void UWorldWidgetSubsystem::InitWidgets(UClass* HUDClass, const TArray<EWorldWidgetType>& WorldWidgetTypes)
{
	InitHUD(HUDClass);
	for (auto& WorldWidgetType : WorldWidgetTypes)
	{
		PrepareWorldWidget(WorldWidgetType);
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

void UWorldWidgetSubsystem::PrepareWorldWidget(EWorldWidgetType WorldWidgetType)
{
	switch (WorldWidgetType)
	{
	case EWorldWidgetType::WorldMap:
	case EWorldWidgetType::InGameSettings:
	case EWorldWidgetType::DicePanel:
	case EWorldWidgetType::SkillPanel:
	case EWorldWidgetType::CharacterSelect:
		return;
	default:
		InitWorldWidget(WorldWidgetType);
		return;
	}
}

void UWorldWidgetSubsystem::InitWorldWidget(EWorldWidgetType WorldWidgetType)
{
	const uint8 Index = StaticCast<uint8>(WorldWidgetType);
	if (mWorldWidgets[Index] == nullptr)
	{
		const TSoftClassPtr<UUserWidget>& WorldWidgetClass = GetDefault<UGamePlaySettings>()->mWorldWidgetClasses[Index];
		if (WorldWidgetClass.IsNull())
		{
			UE_LOG(LogWorldWidget, Warning, TEXT("[%s] 월드 위젯 클래스가 지정되지 않음"), *EnumToString(WorldWidgetType));
			return;
		}

		UClass* LoadedWorldWidgetClass = WorldWidgetClass.LoadSynchronous();
		if (LoadedWorldWidgetClass == nullptr)
		{
			UE_LOG(LogWorldWidget, Error, TEXT("[%s] 월드 위젯 클래스 로드 실패"), *EnumToString(WorldWidgetType));
			return;
		}

		UE_LOG(LogWorldWidget, Warning, TEXT("[%s] 월드 위젯 객체 생성"), *EnumToString(WorldWidgetType));
		mWorldWidgets[Index] = CreateWidget(GetWorld()->GetFirstPlayerController(), LoadedWorldWidgetClass);
	}
}

UUserWidget* UWorldWidgetSubsystem::GetWorldWidget(EWorldWidgetType Type) const
{
	return mWorldWidgets[StaticCast<uint8>(Type)];
}

UUserWidget* UWorldWidgetSubsystem::GetOrCreateWorldWidget(EWorldWidgetType Type)
{
	InitWorldWidget(Type);
	return GetWorldWidget(Type);
}
