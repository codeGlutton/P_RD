#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

#include "Setting/GamePlaySettings.h"
#include "UI/MsgNotifyWidget.h"
#include "UI/WorldWidgetLifecycle.h"

DEFINE_LOG_CATEGORY(LogWorldWidget)

namespace
{
	bool IsValidWorldWidgetType(EWorldWidgetType WorldWidgetType)
	{
		const uint8 Index = StaticCast<uint8>(WorldWidgetType);
		return Index < StaticCast<uint8>(EWorldWidgetType::Count);
	}

	UUserWidget* CreateWorldUserWidget(UWorld* World, UClass* WidgetClass)
	{
		if (World == nullptr || WidgetClass == nullptr)
		{
			return nullptr;
		}

		if (APlayerController* PlayerController = World->GetFirstPlayerController())
		{
			return CreateWidget<UUserWidget>(PlayerController, WidgetClass);
		}

		return CreateWidget<UUserWidget>(World, WidgetClass);
	}
}

void UWorldWidgetSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(mMsgNotifyCloseTimerHandle);
	}

	if (mHUD != nullptr)
	{
		mHUD->RemoveFromParent();
		mHUD = nullptr;
	}

	for (TObjectPtr<UUserWidget>& WorldWidget : mWorldWidgets)
	{
		if (WorldWidget != nullptr)
		{
			WorldWidget->RemoveFromParent();
			WorldWidget = nullptr;
		}
	}

	Super::Deinitialize();
}

void UWorldWidgetSubsystem::InitWidgets(UClass* HUDClass, const TArray<EWorldWidgetType>& WorldWidgetTypes)
{
	InitHUD(HUDClass);
	for (const EWorldWidgetType WorldWidgetType : WorldWidgetTypes)
	{
		InitWorldWidget(WorldWidgetType);
	}
}

void UWorldWidgetSubsystem::InitWidgets(UClass* HUDClass, const TSet<EWorldWidgetType>& WorldWidgetTypes)
{
	InitHUD(HUDClass);
	for (const EWorldWidgetType WorldWidgetType : WorldWidgetTypes)
	{
		InitWorldWidget(WorldWidgetType);
	}
}

void UWorldWidgetSubsystem::InitHUD(UClass* HUDClass)
{
	if (mHUD != nullptr)
	{
		UE_LOG(LogWorldWidget, Display, TEXT("HUD 이미 초기화됨: %s"), *mHUD->GetName());
		return;
	}

	if (HUDClass == nullptr)
	{
		UE_LOG(LogWorldWidget, Warning, TEXT("HUD 위젯 클래스가 설정되지 않았습니다."));
		return;
	}

	mHUD = CreateWorldUserWidget(GetWorld(), HUDClass);
	if (mHUD != nullptr)
	{
		UE_LOG(LogWorldWidget, Display, TEXT("HUD 생성: %s"), *mHUD->GetName());
	}
}

void UWorldWidgetSubsystem::InitWorldWidget(EWorldWidgetType WorldWidgetType)
{
	if (!IsValidWorldWidgetType(WorldWidgetType))
	{
		UE_LOG(LogWorldWidget, Warning, TEXT("유효하지 않은 월드 위젯 타입입니다. Type: %d"), StaticCast<uint8>(WorldWidgetType));
		return;
	}

	const uint8 Index = StaticCast<uint8>(WorldWidgetType);
	if (mWorldWidgets[Index] != nullptr)
	{
		UE_LOG(LogWorldWidget, Display, TEXT("월드 위젯 이미 초기화됨. Type: %d, Widget: %s"), Index, *mWorldWidgets[Index]->GetName());
		return;
	}

	const TSubclassOf<UUserWidget> WorldWidgetClass = GetDefault<UGamePlaySettings>()->mWorldWidgetClasses[Index];
	if (WorldWidgetClass == nullptr)
	{
		UE_LOG(LogWorldWidget, Warning, TEXT("월드 위젯 클래스가 설정되지 않았습니다. Type: %d"), Index);
		return;
	}

	mWorldWidgets[Index] = CreateWorldUserWidget(GetWorld(), WorldWidgetClass);
	if (mWorldWidgets[Index] != nullptr)
	{
		UE_LOG(LogWorldWidget, Display, TEXT("월드 위젯 생성. Type: %d, Class: %s, Widget: %s"), Index, *WorldWidgetClass->GetName(), *mWorldWidgets[Index]->GetName());
	}
}

UUserWidget* UWorldWidgetSubsystem::GetWorldWidget(EWorldWidgetType Type) const
{
	if (!IsValidWorldWidgetType(Type))
	{
		return nullptr;
	}

	return mWorldWidgets[StaticCast<uint8>(Type)];
}

bool UWorldWidgetSubsystem::RegisterWorldWidget(EWorldWidgetType Type, UUserWidget* Widget)
{
	if (!IsValidWorldWidgetType(Type) || Widget == nullptr)
	{
		return false;
	}

	mWorldWidgets[StaticCast<uint8>(Type)] = Widget;
	return true;
}

bool UWorldWidgetSubsystem::AddHUDToViewport(EViewportZOrderType ZOrderType)
{
	if (mHUD == nullptr)
	{
		return false;
	}

	if (!mHUD->IsInViewport() && mHUD->GetParent() == nullptr)
	{
		mHUD->AddToViewport(FViewportZOrder::ToZOrder(ZOrderType));
		UE_LOG(LogWorldWidget, Display, TEXT("HUD 뷰포트 추가: %s, ZOrderType: %d"), *mHUD->GetName(), StaticCast<uint8>(ZOrderType));
	}

	return true;
}

bool UWorldWidgetSubsystem::OpenHUD(ESlateVisibility Visibility)
{
	if (!AddHUDToViewport())
	{
		return false;
	}

	return OpenWidget(mHUD, Visibility);
}

bool UWorldWidgetSubsystem::CloseHUD(ESlateVisibility Visibility)
{
	return CloseWidget(mHUD, Visibility);
}

bool UWorldWidgetSubsystem::RemoveHUD()
{
	if (mHUD == nullptr)
	{
		return false;
	}

	mHUD->RemoveFromParent();
	mHUD = nullptr;
	return true;
}

bool UWorldWidgetSubsystem::AddWorldWidgetToViewport(EWorldWidgetType Type)
{
	if (!IsValidWorldWidgetType(Type))
	{
		return false;
	}

	if (GetWorldWidget(Type) == nullptr)
	{
		InitWorldWidget(Type);
	}

	UUserWidget* WorldWidget = GetWorldWidget(Type);
	if (WorldWidget == nullptr)
	{
		return false;
	}

	if (!WorldWidget->IsInViewport() && WorldWidget->GetParent() == nullptr)
	{
		WorldWidget->AddToViewport(FViewportZOrder::ToZOrder(GetWorldWidgetZOrderType(Type)));
		UE_LOG(LogWorldWidget, Display, TEXT("월드 위젯 뷰포트 추가. Type: %d, Widget: %s"), StaticCast<uint8>(Type), *WorldWidget->GetName());
	}

	return true;
}

bool UWorldWidgetSubsystem::OpenWorldWidget(EWorldWidgetType Type, ESlateVisibility Visibility)
{
	if (!AddWorldWidgetToViewport(Type))
	{
		return false;
	}

	const bool bIsOpened = OpenWidget(GetWorldWidget(Type), Visibility);
	UE_LOG(LogWorldWidget, Display, TEXT("월드 위젯 열기. Type: %d, Result: %s"), StaticCast<uint8>(Type), bIsOpened ? TEXT("true") : TEXT("false"));
	return bIsOpened;
}

bool UWorldWidgetSubsystem::ShowMsgNotify(const FText& Message, float Duration)
{
	if (!AddWorldWidgetToViewport(EWorldWidgetType::MsgNotify))
	{
		return false;
	}

	if (UMsgNotifyWidget* MsgNotifyWidget = GetWorldWidget<UMsgNotifyWidget>(EWorldWidgetType::MsgNotify))
	{
		MsgNotifyWidget->SetMessage(Message);
	}

	const bool bIsOpened = OpenWorldWidget(EWorldWidgetType::MsgNotify, ESlateVisibility::HitTestInvisible);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(mMsgNotifyCloseTimerHandle);
		if (Duration > 0.f)
		{
			TWeakObjectPtr<UWorldWidgetSubsystem> WeakThis = this;
			World->GetTimerManager().SetTimer(mMsgNotifyCloseTimerHandle, FTimerDelegate::CreateLambda([WeakThis]() {
				if (UWorldWidgetSubsystem* WorldWidgetSubsystem = WeakThis.Get())
				{
					WorldWidgetSubsystem->CloseWorldWidget(EWorldWidgetType::MsgNotify);
				}
				}), Duration, false);
		}
	}

	return bIsOpened;
}

bool UWorldWidgetSubsystem::CloseWorldWidget(EWorldWidgetType Type, ESlateVisibility Visibility)
{
	const bool bIsClosed = CloseWidget(GetWorldWidget(Type), Visibility);
	UE_LOG(LogWorldWidget, Display, TEXT("월드 위젯 닫기. Type: %d, Result: %s"), StaticCast<uint8>(Type), bIsClosed ? TEXT("true") : TEXT("false"));
	return bIsClosed;
}

bool UWorldWidgetSubsystem::RemoveWorldWidget(EWorldWidgetType Type)
{
	if (!IsValidWorldWidgetType(Type))
	{
		return false;
	}

	const uint8 Index = StaticCast<uint8>(Type);
	if (mWorldWidgets[Index] == nullptr)
	{
		return false;
	}

	mWorldWidgets[Index]->RemoveFromParent();
	mWorldWidgets[Index] = nullptr;
	return true;
}

bool UWorldWidgetSubsystem::IsWorldWidgetInitialized(EWorldWidgetType Type) const
{
	return GetWorldWidget(Type) != nullptr;
}

bool UWorldWidgetSubsystem::IsWorldWidgetOpen(EWorldWidgetType Type) const
{
	const UUserWidget* WorldWidget = GetWorldWidget(Type);
	return WorldWidget != nullptr && WorldWidget->IsVisible();
}

EViewportZOrderType UWorldWidgetSubsystem::GetWorldWidgetZOrderType(EWorldWidgetType Type) const
{
	switch (Type)
	{
	case EWorldWidgetType::TopMenuBar:
		return EViewportZOrderType::HUD;
	case EWorldWidgetType::MsgNotify:
	case EWorldWidgetType::SaveNotify:
		return EViewportZOrderType::Notification;
	default:
		return EViewportZOrderType::Base;
	}
}

bool UWorldWidgetSubsystem::OpenWidget(UUserWidget* Widget, ESlateVisibility Visibility) const
{
	if (Widget == nullptr)
	{
		return false;
	}

	if (Widget->GetClass()->ImplementsInterface(UWorldWidgetLifecycle::StaticClass()))
	{
		IWorldWidgetLifecycle::Execute_OpenUI(Widget);
	}
	else
	{
		Widget->SetVisibility(Visibility);
	}

	return true;
}

bool UWorldWidgetSubsystem::CloseWidget(UUserWidget* Widget, ESlateVisibility Visibility) const
{
	if (Widget == nullptr)
	{
		return false;
	}

	if (Widget->GetClass()->ImplementsInterface(UWorldWidgetLifecycle::StaticClass()))
	{
		IWorldWidgetLifecycle::Execute_CloseUI(Widget);
	}
	else
	{
		Widget->SetVisibility(Visibility);
	}

	return true;
}
