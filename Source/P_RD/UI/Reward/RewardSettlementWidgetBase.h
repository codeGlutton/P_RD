#pragma once

#include "RDMinimal.h"
#include "UI/RDUserWidget.h"
#include "UI/Reward/RewardUITypes.h"

#include "RewardSettlementWidgetBase.generated.h"

class UButton;
class UTextBlock;
class UTexture2D;
class UVerticalBox;
class URewardUIModel;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRewardSettlementClosed);

/**
 * @brief 신규 전투 보상 정산 WBP 전용 런타임 클래스.
 *
 * @details 구형 WBP_Reward의 동적 행/배경 보정 코드와 완전히 분리한다.
 * WBP는 반응형 영역과 이름 붙은 컨테이너만 소유하고, 실제 보상/용병 행은
 * RewardUIModel의 현재 데이터를 읽어 이 클래스가 생성한다.
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

	void UnbindUIModel();
	void RefreshView();
	void RebuildSummaryRows();
	void RebuildMercenaryRows();
	void AddSummaryRow(const FText& MainText, const FText& SubText, UTexture2D* Icon);
	void AddMercenaryRow(int32 RowIndex, const FRewardMercenaryExpUI& Mercenary, int32 ExpGained);

protected:
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UVerticalBox> mSummaryRowsBox;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UVerticalBox> mMercenaryRowsBox;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> mTitleText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> mGoldBalanceText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> mNextButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> mNextButtonText;

private:
	UPROPERTY(Transient) TObjectPtr<URewardUIModel> mUIModel;
	UPROPERTY() TObjectPtr<UTexture2D> mMercenaryRowTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mPortraitFrameTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mXPBadgeTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mExpTrackTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mExpFillTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mGoldIconTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mExpIconTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mEquipmentIconTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mSkillIconTexture;

	bool mContinueCommitted = false;
	int32 mDynamicBuildGeneration = 0;
};
