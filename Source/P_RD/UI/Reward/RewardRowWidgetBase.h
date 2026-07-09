#pragma once

/** @brief 보상 화면의 한 줄을 표시하는 WBP 베이스입니다. 레이아웃은 WBP가 들고, C++은 값만 주입합니다. */

#include "RDMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Input/Reply.h"

#include "RewardRowWidgetBase.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRewardRowClicked, int32, RewardRowIndex);

UCLASS(Abstract)
class P_RD_API URewardRowWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Reward|Row")
	void SetRewardRow(const FText& MainText, const FText& SubText, UTexture2D* IconTexture);

	UFUNCTION(BlueprintCallable, Category = "Reward|Row")
	void SetRewardIndex(int32 RewardRowIndex);

	UFUNCTION(BlueprintCallable, Category = "Reward|Row")
	void SetClaimed(bool bClaimed);

	UPROPERTY(BlueprintAssignable, Category = "Reward|Row")
	FOnRewardRowClicked OnRewardRowClicked;

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> mRowIconFrame;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> mRewardIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mRewardSingleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mRewardMainText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mRewardSubText;

private:
	int32 mRewardRowIndex = INDEX_NONE;
	bool mIsClaimed = false;
	bool mIsPressed = false;
};
