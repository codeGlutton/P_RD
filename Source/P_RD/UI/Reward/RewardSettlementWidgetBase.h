#pragma once

#include "RDMinimal.h"
#include "UI/RDUserWidget.h"
#include "UI/Reward/RewardUITypes.h"

#include "RewardSettlementWidgetBase.generated.h"

class UButton;
class UCanvasPanel;
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

	void UnbindUIModel();
	void RefreshView();
	void RebuildStep();
	void BuildResultStep();
	void BuildChoiceStep();
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

	bool mContinueCommitted = false;
	/** @brief 1 = 경험치·골드 정산, 2 = 아티팩트 3중 1택. */
	int32 mCurrentStep = 1;
	int32 mSelectedChoice = INDEX_NONE;
};
