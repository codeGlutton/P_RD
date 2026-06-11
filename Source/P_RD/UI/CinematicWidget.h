/*****************************************************************//**
 * @file   CinematicWidget.h
 * @brief  시네마틱 UI 베이스
 * @date   2026-06-08
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "UI/RDUserWidget.h"

#include "CinematicWidget.generated.h"

class UCinematicWidget;

DECLARE_DELEGATE_OneParam(FOnEndCinematicAnimation, UCinematicWidget*)

/**
 * @brief 시네마틱 표시, UI 열림/닫힘, 재생 완료 알림을 제공하는 위젯 베이스
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API UCinematicWidget : public URDUserWidget
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

protected:
	/**
	 * @brief WBP가 별도 애니메이션을 구현하지 않았을 때 시네마틱을 유지할 최소 시간
	 */
	UPROPERTY(Category = "UI|Cinematic", EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", DisplayName = "Default Cinematic Duration"))
	float mDefaultCinematicDuration = 1.5f;

private:
	FOnEndCinematicAnimation OnEndCinematicAnimation;
	bool mCinematicFinished = false;
};
