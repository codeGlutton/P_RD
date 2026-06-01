/*****************************************************************//**
 * @file   WorldWidgetSubsystem.h
 * @brief  특정한 소유자 없이 월드에 소속된 Widget 서브시스템 정의 헤더
 * @author 모호재
 * @date   2026-05-22
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Components/SlateWrapperTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"

#include "Singleton/WorldSubsystem/WorldWidgetType.h"
#include "UI/ViewportZOrderType.h"

#include "WorldWidgetSubsystem.generated.h"

class UUserWidget;

// World Widget 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogWorldWidget, Log, All)

/**
 * @brief  특정한 소유자 없이 월드에 소속된 Widget 서브시스템
 */
UCLASS()
class P_RD_API UWorldWidgetSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/* UWorldSubsystem 상속 */
	void Deinitialize() override;

	void InitWidgets(UClass* HUDClass, const TArray<EWorldWidgetType>& WorldWidgetTypes);
	void InitWidgets(UClass* HUDClass, const TSet<EWorldWidgetType>& WorldWidgetTypes);
	void InitHUD(UClass* HUDClass);
	void InitWorldWidget(EWorldWidgetType WorldWidgetType);

public:
	template<typename T = UUserWidget>
	T* GetHUD() const
	{
		static_assert(TIsDerivedFrom<T, UUserWidget>::IsDerived);

		return Cast<T>(mHUD);
	}

	UUserWidget* GetWorldWidget(EWorldWidgetType Type) const;

	template<typename T = UUserWidget>
	T* GetWorldWidget(EWorldWidgetType Type) const
	{
		static_assert(TIsDerivedFrom<T, UUserWidget>::IsDerived);

		return Cast<T>(GetWorldWidget(Type));
	}

	UFUNCTION(Category = UI, BlueprintCallable)
	bool RegisterWorldWidget(EWorldWidgetType Type, UUserWidget* Widget);

	UFUNCTION(Category = UI, BlueprintCallable)
	bool AddHUDToViewport(EViewportZOrderType ZOrderType = EViewportZOrderType::HUD);

	UFUNCTION(Category = UI, BlueprintCallable)
	bool OpenHUD(ESlateVisibility Visibility = ESlateVisibility::Visible);

	UFUNCTION(Category = UI, BlueprintCallable)
	bool CloseHUD(ESlateVisibility Visibility = ESlateVisibility::Collapsed);

	UFUNCTION(Category = UI, BlueprintCallable)
	bool RemoveHUD();

	UFUNCTION(Category = UI, BlueprintCallable)
	bool AddWorldWidgetToViewport(EWorldWidgetType Type);

	UFUNCTION(Category = UI, BlueprintCallable)
	bool OpenWorldWidget(EWorldWidgetType Type, ESlateVisibility Visibility = ESlateVisibility::Visible);

	UFUNCTION(Category = UI, BlueprintCallable)
	bool ShowMsgNotify(const FText& Message, float Duration = 2.f);

	UFUNCTION(Category = UI, BlueprintCallable)
	bool CloseWorldWidget(EWorldWidgetType Type, ESlateVisibility Visibility = ESlateVisibility::Collapsed);

	UFUNCTION(Category = UI, BlueprintCallable)
	bool RemoveWorldWidget(EWorldWidgetType Type);

	UFUNCTION(Category = UI, BlueprintPure)
	bool IsWorldWidgetInitialized(EWorldWidgetType Type) const;

	UFUNCTION(Category = UI, BlueprintPure)
	bool IsWorldWidgetOpen(EWorldWidgetType Type) const;

protected:
	EViewportZOrderType GetWorldWidgetZOrderType(EWorldWidgetType Type) const;
	bool OpenWidget(UUserWidget* Widget, ESlateVisibility Visibility) const;
	bool CloseWidget(UUserWidget* Widget, ESlateVisibility Visibility) const;

protected:
	UPROPERTY(Category = UI, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "HUD"))
	TObjectPtr<UUserWidget> mHUD;

	UPROPERTY(Category = UI, VisibleAnywhere, meta = (DisplayName = "WorldWidgets", ArraySizeEnum = "EWorldWidgetType"))
	TObjectPtr<UUserWidget> mWorldWidgets[static_cast<uint8>(EWorldWidgetType::Count)];

	FTimerHandle mMsgNotifyCloseTimerHandle;
};
