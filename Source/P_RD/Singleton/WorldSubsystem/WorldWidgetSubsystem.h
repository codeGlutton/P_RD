/*****************************************************************//**
 * @file   WorldWidgetSubsystem.h
 * @brief  특정한 소유자 없이 월드에 소속된 Widget 서브시스템 정의 헤더
 * @author 모호재
 * @date   2026-05-22
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "Singleton/WorldSubsystem/WorldWidgetType.h"

#include "WorldWidgetSubsystem.generated.h"

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
	void InitWidgets(UClass* HUDClass, const TArray<EWorldWidgetType>& WorldWidgetTypes);
	void InitHUD(UClass* HUDClass);
	void InitWorldWidget(EWorldWidgetType WorldWidgetType);

	UFUNCTION(BlueprintCallable, Category = "UI|World Widget")
	void OpenWorldWidget(EWorldWidgetType WorldWidgetType);

	UFUNCTION(BlueprintCallable, Category = "UI|World Widget")
	void CloseWorldWidget(EWorldWidgetType WorldWidgetType);

	UFUNCTION(BlueprintPure, Category = "UI|World Widget")
	bool IsWorldWidgetOpen(EWorldWidgetType WorldWidgetType) const;

	void ShowWorldWidget(EWorldWidgetType WorldWidgetType);
	void HideWorldWidget(EWorldWidgetType WorldWidgetType);

public:
	template<typename T = UUserWidget>
	T* GetHUD() const
	{
		static_assert(TIsDerivedFrom<T, UUserWidget>::IsDerived);

		return Cast<T>(mHUD);
	}

	UUserWidget* GetWorldWidget(EWorldWidgetType Type) const;

protected:
	UPROPERTY(Category = UI, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "HUD"))
	TObjectPtr<UUserWidget> mHUD;

	UPROPERTY(Category = UI, VisibleAnywhere, meta = (DisplayName = "WorldWidgets", ArraySizeEnum = "EWorldWidgetType"))
	TObjectPtr<UUserWidget> mWorldWidgets[static_cast<uint8>(EWorldWidgetType::Count)];
};
