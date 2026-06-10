/**
 * @file LoadingNotifyWidget.h
 * @brief 룸 전환 중 로딩 상태를 최소 시간 동안 보여주는 알림 위젯.
 *
 * @details
 * Preload가 매우 빨리 끝나면 사용자는 화면 전환이 눌렸는지 알기 어렵다. 이 위젯은 로딩중 상태를
 * 최소 표시 시간 동안 유지한 뒤, CloseUI에서 로딩완료 상태와 닫힘 애니메이션을 끝내고 콜백을 실행한다.
 */

#pragma once

#include "RDMinimal.h"
#include "UI/RDUserWidget.h"

#include "LoadingNotifyWidget.generated.h"

class UTextBlock;
class UWidget;

/**
 * @brief 최소 표시 시간, 로딩중/로딩완료 문구, 점멸 인디케이터를 관리한다.
 *
 * 전환 흐름의 실제 완료 여부는 GameMode/RoomTransitionSubsystem이 결정하고, 이 위젯은 사용자가 볼
 * 로딩 표현과 닫힘 완료 콜백만 담당한다.
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API ULoadingNotifyWidget : public URDUserWidget
{
	GENERATED_BODY()

public:
	ULoadingNotifyWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	bool Initialize() override;
	void NativePreConstruct() override;
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	void PlayOpenUIAnimation_Implementation() override;
	void PlayCloseUIAnimation_Implementation() override;

private:
	enum class ELoadingNotifyState : uint8
	{
		None,
		Loading,
		Completed,
	};

	void EnsureDefaultVisual();
	void SetLoadingState(ELoadingNotifyState NewState);
	void ApplyIndicatorAlpha(float Alpha) const;
	void ShowCompletedState();
	void FinishCompletedState();

private:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> mLoadingIndicator;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mLoadingStatusText;

	UPROPERTY(Category = UI, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "MinimumVisibleSeconds", AllowPrivateAccess = true))
	float mMinimumVisibleSeconds = 1.0f;

	UPROPERTY(Category = UI, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "CompletedVisibleSeconds", AllowPrivateAccess = true))
	float mCompletedVisibleSeconds = 0.35f;

	UPROPERTY(Category = UI, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "CloseAnimationSeconds", AllowPrivateAccess = true))
	float mCloseAnimationSeconds = 0.25f;

	UPROPERTY(Category = UI, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "IndicatorBlinkSpeed", AllowPrivateAccess = true))
	float mIndicatorBlinkSpeed = 2.0f;

	FTimerHandle mMinimumVisibleTimerHandle;
	FTimerHandle mCompletedVisibleTimerHandle;
	FTimerHandle mCloseAnimationTimerHandle;
	double mOpenedTimeSeconds = 0.0;
	float mIndicatorElapsedSeconds = 0.0f;
	ELoadingNotifyState mLoadingState = ELoadingNotifyState::None;
};
