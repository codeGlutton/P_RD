#pragma once

#include "RDMinimal.h"
#include "UI/RDUserWidget.h"
#include "UI/Combat/CombatUITypes.h"
#include "UI/Reward/RewardUITypes.h"

#include "CombatResultOverlayWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;

UENUM()
enum class ECombatResultOverlayMode : uint8
{
	None,
	VictoryReward,
	DefeatContinue
};

/** @brief 전투 결과 영상 위에 띄우는 최소 C++ 오버레이. */
UCLASS()
class P_RD_API UCombatResultOverlayWidget : public URDUserWidget
{
	GENERATED_BODY()

public:
	UCombatResultOverlayWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void ShowVictoryReward(const FRewardUI& Reward, FSimpleDelegate ConfirmCallback);
	void ShowDefeatResult(
		const FCombatResultUI& Result,
		FSimpleDelegate TitleCallback);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION() void HandleTitleClicked();
	void BindButtons();
	void RefreshWidget();

private:
	ECombatResultOverlayMode mMode = ECombatResultOverlayMode::None;
	FRewardUI mReward;
	FCombatResultUI mCombatResult;
	FSimpleDelegate mTitleCallback;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> mTitleButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> mLocationText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> mRoundText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> mEnemyText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> mGoldText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> mExpText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> mPartyPortrait0;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> mPartyPortrait1;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> mPartyPortrait2;
};
