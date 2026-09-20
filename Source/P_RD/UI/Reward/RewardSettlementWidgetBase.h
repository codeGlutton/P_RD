#pragma once

#include "RDMinimal.h"
#include "UI/RDUserWidget.h"
#include "UI/Reward/RewardUITypes.h"
#include "TimerManager.h"

#include "RewardSettlementWidgetBase.generated.h"

class UButton;
class UCanvasPanel;
class UImage;
class UTextBlock;
class UTexture2D;
class UVerticalBox;
class URewardUIModel;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRewardSettlementClosed);

/**
 * @brief 신규 전투 보상 정산 WBP 전용 런타임 클래스.
 *
 * @details 구형 WBP_Reward의 동적 행/배경 보정 코드와 완전히 분리한다.
 * WBP가 최대 3개의 경험치 행과 최대 3개의 선택 카드를 실제 위젯으로 소유한다.
 * 런타임은 RewardUIModel의 현재 데이터를 읽어 값/브러시/표시 여부만 갱신한다.
 * 따라서 디자이너에서 조정한 파츠의 위치와 크기가 그대로 인게임에 반영된다.
 */
UCLASS(Abstract)
class P_RD_API URewardSettlementWidgetBase : public URDUserWidget
{
	GENERATED_BODY()

public:
	URewardSettlementWidgetBase(const FObjectInitializer& ObjectInitializer);

	virtual void OpenUI(FOnEndUIOpenAnimation Callback = FOnEndUIOpenAnimation()) override;
	virtual void CloseUI(FOnEndUICloseAnimation Callback = FOnEndUICloseAnimation()) override;

	UFUNCTION(BlueprintCallable, Category = "Reward|Settlement")
	void BindUIModel(URewardUIModel* InUIModel);

	UFUNCTION(BlueprintCallable, Category = "Reward|Settlement")
	void ContinueToNext();

#if WITH_DEV_AUTOMATION_TESTS
	/** Drives the chest sequence synchronously and stops on the independent gold step. */
	void CompleteChestRevealForTest();
	/** Advances the gold step to the optional artifact step and completes its reveal. */
	void AdvanceGoldToArtifactForTest();
#endif

	UPROPERTY(BlueprintAssignable, Category = "Reward|Settlement")
	FOnRewardSettlementClosed OnClosed;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void PlayOpenUIAnimation_Implementation() override;
	virtual void PlayCloseUIAnimation_Implementation() override;

private:
	UFUNCTION() void HandleUIChanged();
	UFUNCTION() void HandleChoicesChanged();
	UFUNCTION() void HandleNextClicked();
	UFUNCTION() void HandleChoiceClicked_0();
	UFUNCTION() void HandleChoiceClicked_1();
	UFUNCTION() void HandleChoiceClicked_2();
	UFUNCTION() void HandleChestClicked();

	void UnbindUIModel();
	void RefreshView();
	void RebuildStep();
	void BuildResultStep();
	void BuildChestStep();
	void BuildGoldStep();
	void BuildChoiceStep();
	void ResetChestPresentation();
	void TickChestReveal();
	void AdvanceChestToOpen();
	void FinishChestReveal();
	void BeginChoiceReveal();
	void TickChoiceReveal();
	void FinishChoiceReveal();
	void RefreshMercenaryRow(int32 RowIndex,
		const FRewardMercenaryExpUI& Mercenary, int32 ExpGained);
	void SelectChoice(int32 ChoiceSlot);
	void RefreshStepCoins();

protected:
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UVerticalBox> mSummaryRowsBox;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UVerticalBox> mMercenaryRowsBox;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> mTitleText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> mGoldBalanceText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> mNextButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> mNextButtonText;

private:
	UPROPERTY(Transient) TObjectPtr<URewardUIModel> mUIModel;
	UPROPERTY() TObjectPtr<UTexture2D> mGoldIconTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mEquipmentIconTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mSkillIconTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mChestClosedTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mChestHalfOpenTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mChestOpenTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mChestRevealAuraTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mChoiceCardNormalTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mChoiceCardSelectedTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mStepCoinActiveTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mStepCoinInactiveTexture;

	bool mContinueCommitted = false;
	bool mChestOpening = false;
	bool mRewardRevealPlaying = false;
	/** @brief 1 = 경험치, 2 = 상자, 3 = 골드, 4 = 아티팩트 3중 1택(있을 때만). */
	int32 mCurrentStep = 1;
	int32 mSelectedChoice = INDEX_NONE;
	FTimerHandle mChestOpenTimer;
	FTimerHandle mChestRevealTimer;
	FTimerHandle mChestAnimationTimer;
	FTimerHandle mChoiceAnimationTimer;
	float mChestAnimationElapsed = 0.f;
	float mChoiceAnimationElapsed = 0.f;
};
