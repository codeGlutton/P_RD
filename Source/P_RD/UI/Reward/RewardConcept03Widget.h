#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "UI/Reward/RewardUITypes.h"

#include "RewardConcept03Widget.generated.h"

class UButton;
class UBackgroundBlur;
class UImage;
class UTextBlock;
class UWidget;
class UWidgetSwitcher;
class URewardUIModel;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FRewardConcept03StepChanged, int32, StepIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FRewardConcept03ArtifactSelected, int32, ArtifactIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FRewardConcept03Completed, int32, ArtifactIndex);

/**
 * Runtime interaction for the icon-free RewardConcept03 result screen.
 *
 * The generated WBP owns only layout and art. This class keeps the reward
 * sequence deterministic and exposes completion events to the actual reward
 * system without taking a dependency on the older settlement widget.
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API URewardConcept03Widget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Connects the generated WBP to the real combat reward payload/claim flow. */
	UFUNCTION(BlueprintCallable, Category = "Reward Concept 03")
	void BindUIModel(URewardUIModel* InUIModel);

	UFUNCTION(BlueprintCallable, Category = "Reward Concept 03")
	void ResetRewardFlow();

	UFUNCTION(BlueprintCallable, Category = "Reward Concept 03")
	void AdvanceRewardFlow();

	UFUNCTION(BlueprintCallable, Category = "Reward Concept 03")
	void OpenRewardChest();

	/** Advances only the presentation timeline. Useful for deterministic previews/tests. */
	UFUNCTION(BlueprintCallable, Category = "Reward Concept 03")
	void AdvanceRewardPresentation(float DeltaSeconds);

	/** Completes the active reveal phase while preserving the automatic flow order. */
	UFUNCTION(BlueprintCallable, Category = "Reward Concept 03")
	void SkipRewardPresentation();

	/** Disables NativeTick advancement so capture tools can step exact frame deltas. */
	UFUNCTION(BlueprintCallable, Category = "Reward Concept 03|Preview")
	void SetRewardPresentationManualTick(bool bManualTick)
	{
		bManualPresentationTick = bManualTick;
	}

	UFUNCTION(BlueprintCallable, Category = "Reward Concept 03")
	void SelectArtifact(int32 ArtifactIndex);

	/** Opens the same modal that a completed 0.5 second artifact press uses. */
	UFUNCTION(BlueprintCallable, Category = "Reward Concept 03|Artifact")
	void ShowArtifactDetails(int32 ArtifactIndex);

	UFUNCTION(BlueprintCallable, Category = "Reward Concept 03|Artifact")
	void HideArtifactDetails();

	/** Automation/read-only access to the shared combat detail WBP instance. */
	UUserWidget* GetArtifactDetailOverlayForTest() const
	{
		return ArtifactDetailOverlayWidget;
	}

	UFUNCTION(BlueprintPure, Category = "Reward Concept 03")
	int32 GetCurrentStepIndex() const { return CurrentStepIndex; }

	UFUNCTION(BlueprintPure, Category = "Reward Concept 03")
	bool IsRewardChestOpened() const { return bChestOpened; }

	UFUNCTION(BlueprintPure, Category = "Reward Concept 03")
	int32 GetSelectedArtifactIndex() const { return SelectedArtifactIndex; }

	UFUNCTION(BlueprintPure, Category = "Reward Concept 03")
	bool IsRewardFlowCompleted() const { return bFlowCompleted; }

	UFUNCTION(BlueprintPure, Category = "Reward Concept 03")
	bool HasArtifactReward() const { return UsesArtifactStep(); }

	UFUNCTION(BlueprintPure, Category = "Reward Concept 03")
	bool IsRewardPresentationPlaying() const;

	UPROPERTY(BlueprintAssignable, Category = "Reward Concept 03")
	FRewardConcept03StepChanged OnRewardStepChanged;

	UPROPERTY(BlueprintAssignable, Category = "Reward Concept 03")
	FRewardConcept03ArtifactSelected OnArtifactSelected;

	UPROPERTY(BlueprintAssignable, Category = "Reward Concept 03")
	FRewardConcept03Completed OnRewardFlowCompleted;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual bool UsesArtifactStep() const { return true; }

private:
	enum class EPresentationState : uint8
	{
		Idle,
		ChestAwaitInput,
		ChestOpening,
		GoldReveal,
		ArtifactReveal,
		AwaitArtifactChoice,
		AwaitConfirm,
		Completed
	};

	void ResolveWidgets();
	void BindInput();
	void UnbindInput();
	void SetCurrentStep(int32 StepIndex);
	void CompleteRewardFlow();
	void FinishRewardFlowAfterConfirmation();
	void StartChestOpening();
	void UpdateChestOpening(float NormalizedTime);
	void FinishChestOpening();
	void StartGoldReveal();
	void UpdateGoldReveal(float NormalizedTime);
	void FinishGoldReveal();
	void StartArtifactReveal();
	void UpdateArtifactReveal(float NormalizedTime);
	void FinishArtifactReveal();
	void ResetPresentationVisuals();
	void UnbindUIModel();
	void RefreshRewardData();
	void ClaimExperienceReward();
	void ClaimGoldReward();
	FText GetRewardChoiceTypeText(int32 ChoiceIndex) const;
	void ApplyVisualState();
	void ApplyArtifactSelection();
	void ApplyBottomActionVisual(bool bPressed = false);
	void BeginArtifactPress(int32 ArtifactIndex);
	void EndArtifactPress();
	void CancelArtifactPress();
	bool EnsureArtifactDetailOverlay();
	void ReleaseArtifactDetailOverlay();

	UFUNCTION()
	void HandleBottomActionClicked();

	UFUNCTION()
	void HandleChestClicked();

	UFUNCTION()
	void HandleArtifact0Clicked();

	UFUNCTION()
	void HandleArtifact1Clicked();

	UFUNCTION()
	void HandleArtifact2Clicked();

	UFUNCTION()
	void HandleArtifact0Pressed();

	UFUNCTION()
	void HandleArtifact1Pressed();

	UFUNCTION()
	void HandleArtifact2Pressed();

	UFUNCTION()
	void HandleArtifactReleased();

	UFUNCTION()
	void HandleArtifactDetailCloseClicked();

	UFUNCTION()
	void HandleBottomActionHovered();

	UFUNCTION()
	void HandleBottomActionUnhovered();

	UFUNCTION()
	void HandleBottomActionPressed();

	UFUNCTION()
	void HandleBottomActionReleased();

	UFUNCTION()
	void HandleRewardDataChanged();

	UFUNCTION()
	void HandleRewardSelectionConfirmed(FPrimaryAssetId RewardId);

	UFUNCTION()
	void HandleRewardGrantBundleConfirmed(FRewardGrantBundleResultUI Result);

	UPROPERTY(Transient)
	TObjectPtr<UWidgetSwitcher> StepSwitcher;

	UPROPERTY(Transient)
	TObjectPtr<UWidgetSwitcher> ProgressSwitcher;

	UPROPERTY(Transient)
	TObjectPtr<UWidgetSwitcher> TabSwitcher;

	UPROPERTY(Transient)
	TObjectPtr<UWidgetSwitcher> ButtonLabelSwitcher;

	UPROPERTY(Transient)
	TObjectPtr<UWidgetSwitcher> ChestVisualSwitcher;

	/**
	 * Frameless 33-frame sequence의 다음 프레임을 겹쳐 그리는 레이어.
	 * 한 장씩 즉시 교체하면 모바일에서 프레임별 노출 차이가 깜빡임처럼
	 * 보이므로, 현재 프레임과 다음 프레임을 연속적으로 크로스페이드한다.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UWidgetSwitcher> ChestBlendSwitcher;

	/** Single resident atlas sampled by UV; avoids mobile texture swaps/flicker. */
	UPROPERTY(Transient)
	TObjectPtr<UImage> ChestSequenceImage;

	/** Next atlas cell used only for a smooth crossfade. */
	UPROPERTY(Transient)
	TObjectPtr<UImage> ChestSequenceBlendImage;

	UPROPERTY(Transient)
	TObjectPtr<UButton> BottomActionButton;

	UPROPERTY(Transient)
	TObjectPtr<UImage> BottomButtonArt;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> BottomButtonPanel;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ChestButton;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> ChestVisualPanel;

	UPROPERTY(Transient)
	TObjectPtr<UImage> ChestBurstGlows[3];

	UPROPERTY(Transient)
	TObjectPtr<UImage> ChestBurstRings[3];

	UPROPERTY(Transient)
	TObjectPtr<UImage> ChestBurstRays[3];

	UPROPERTY(Transient)
	TObjectPtr<UImage> ChestBurstSparks[3];

	UPROPERTY(Transient)
	TObjectPtr<UImage> ChestBurstForegroundCoins[12];

	UPROPERTY(Transient)
	TObjectPtr<UWidget> ChestInfoPanel;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> GoldVisualPanel;

	/** Frameless gold step: keeps the opened, gold-filled chest behind the reward. */
	UPROPERTY(Transient)
	TObjectPtr<UImage> GoldBackgroundChestImage;

	/** Blurs the retained chest instead of removing it during the amount reveal. */
	UPROPERTY(Transient)
	TObjectPtr<UBackgroundBlur> GoldChestBlur;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> GoldInfoPanel;

	UPROPERTY(Transient)
	TObjectPtr<UImage> GoldCoinImage;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> GoldMainText;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> PresentationFlash;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ArtifactButtons[3];

	UPROPERTY(Transient)
	TObjectPtr<UWidget> ArtifactChoicePanels[3];

	/** Existing shared detail screen used by Combat HUD. */
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ArtifactDetailOverlayWidget;

	UPROPERTY(Transient)
	TObjectPtr<UImage> SelectionOutline;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ChestMainText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ChestHintText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ConfirmButtonText;

	UPROPERTY(Transient)
	TObjectPtr<URewardUIModel> UIModel;

	UPROPERTY(VisibleAnywhere, Category = "Reward Concept 03")
	int32 CurrentStepIndex = 0;

	UPROPERTY(VisibleAnywhere, Category = "Reward Concept 03")
	int32 SelectedArtifactIndex = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, Category = "Reward Concept 03")
	bool bChestOpened = false;

	UPROPERTY(VisibleAnywhere, Category = "Reward Concept 03")
	bool bFlowCompleted = false;

	EPresentationState PresentationState = EPresentationState::Idle;
	float PresentationElapsed = 0.f;
	bool bManualPresentationTick = false;

	bool bBottomActionHovered = false;
	bool bExperienceClaimRequested = false;
	bool bGoldClaimRequested = false;
	bool bRewardRequestPending = false;
	FPrimaryAssetId PendingRewardId;
	bool bSuppressNextArtifactClick = false;
	int32 PressedArtifactIndex = INDEX_NONE;
	FTimerHandle ArtifactLongPressTimer;
	int32 DisplayedGoldAmount = 350;
};

/** Three-step variant used when the reward payload has no artifact choice. */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API URewardConcept03NoArtifactWidget : public URewardConcept03Widget
{
	GENERATED_BODY()

protected:
	virtual bool UsesArtifactStep() const override { return false; }
};
