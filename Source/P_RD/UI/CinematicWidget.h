/*****************************************************************//**
 * @file   CinematicWidget.h
 * @brief  시네마틱 UI 베이스
 * @date   2026-06-08
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "UI/ToggleableWidget.h"

#include "CinematicWidget.generated.h"

class UCinematicWidget;

DECLARE_DELEGATE_OneParam(FOnEndCinematicAnimation, UCinematicWidget*)

/**
 * @brief 시네마틱 표시, UI 열림/닫힘, 재생 완료 알림을 제공하는 위젯 베이스
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API UCinematicWidget : public UToggleableWidget
{
	GENERATED_BODY()

public:
	UCinematicWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void PlayCinematic(FOnEndCinematicAnimation Callback = FOnEndCinematicAnimation());

	UFUNCTION(BlueprintCallable, Category = "UI|Cinematic")
	void FinishCinematic();

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "UI|Cinematic")
	void PlayCinematicAnimation();
	virtual void PlayCinematicAnimation_Implementation();

private:
	FOnEndCinematicAnimation OnEndCinematicAnimation;
	bool bCinematicFinished = false;
};
