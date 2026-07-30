// @file InventoryUIWidgetBase.h
// @brief 파티 공용 골드와 아티팩트만 표시하는 인벤토리 WBP 베이스입니다.

#pragma once

#include "RDMinimal.h"
#include "UI/RDUserWidget.h"
#include "UI/Inventory/InventoryUITypes.h"
#include "InventoryUIWidgetBase.generated.h"

class UButton;
class UTextBlock;
class UTexture2D;
class UWidget;
class UWrapBox;
class UInventoryUIModel;

/**
 * @brief 파티 공용 인벤토리 화면.
 *
 * @details 용병 성장, 스킬, 장비는 각 용병 화면의 책임이다. 이 위젯은
 *          공용 골드와 파티 전체에 적용되는 아티팩트만 표시한다.
 */
UCLASS(Abstract)
class P_RD_API UInventoryUIWidgetBase : public URDUserWidget
{
	GENERATED_BODY()

public:
	UInventoryUIWidgetBase(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
	void BindUIModel(UInventoryUIModel* InUIModel);

	UFUNCTION(BlueprintPure, Category = "Inventory|UI")
	UInventoryUIModel* GetUIModel() const { return mUIModel; }

	UFUNCTION(BlueprintPure, Category = "Inventory|Read")
	int32 GetDisplayedGold() const;

	UFUNCTION(BlueprintPure, Category = "Inventory|Read")
	int32 GetArtifactCount() const;

	UFUNCTION(BlueprintPure, Category = "Inventory|Read")
	bool HasBackgroundArt() const;

	UFUNCTION(BlueprintPure, Category = "Inventory|Read")
	bool HasFallbackArtifactIcon() const;

	UFUNCTION(BlueprintPure, Category = "Inventory|Read")
	bool HasArtifactCardFrame() const;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|UI")
	void OnInventoryRefreshed();

	virtual void NativeConstruct() override;
	virtual void ApplyOpenUI() override;
	virtual void ApplyCloseUI() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleCloseClicked();

	UFUNCTION()
	void HandleUIChanged();

	void UnbindUIModel();
	void RefreshView();
	void RefreshFromCurrentRoom();
	void EnsureFallbackLayout();
	void FillArtifacts(UWrapBox* Box, const TArray<FInventoryArtifactUI>& Artifacts);
	UTexture2D* ResolveArtifactIcon(const FInventoryArtifactUI& Artifact) const;

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mTitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mMetaText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mArtifactLabel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> mArtifactBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> mCloseButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mCloseButtonText;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInventoryUIModel> mUIModel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Art", meta = (AllowPrivateAccess = "true"))
	TSoftObjectPtr<UTexture2D> mBackgroundArt;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Art", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTexture2D> mDefaultBackgroundArt;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Art", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTexture2D> mDefaultArtifactIcon;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Art", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTexture2D> mArtifactCardFrame;

	UPROPERTY(Transient)
	TObjectPtr<UInventoryUIModel> mRuntimeUIModel;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> mFallbackLayoutRoot;
};
