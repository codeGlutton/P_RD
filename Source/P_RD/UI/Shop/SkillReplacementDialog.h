#pragma once

#include "UI/RDUserWidget.h"
#include "Fonts/SlateFontInfo.h"
#include "SkillReplacementDialog.generated.h"

class UTextBlock;

/** Confirmation only; the shop remains responsible for the pending transaction. */
UCLASS()
class P_RD_API USkillReplacementDialog : public URDUserWidget
{
	GENERATED_BODY()
public:
	USkillReplacementDialog(const FObjectInitializer& ObjectInitializer);
	void SetSkills(const FText& OldSkill, const FText& NewSkill, const FSlateFontInfo& Font);
	FSimpleDelegate OnConfirmed;
	FSimpleDelegate OnCancelled;
	bool HandleBackNavigation() override;
protected:
	bool Initialize() override;
	void ApplyOpenUI() override;
	FReply NativeOnMouseButtonDown(const FGeometry&, const FPointerEvent&) override { return FReply::Handled(); }
	FReply NativeOnMouseButtonUp(const FGeometry&, const FPointerEvent&) override { return FReply::Handled(); }
	FReply NativeOnMouseWheel(const FGeometry&, const FPointerEvent&) override { return FReply::Handled(); }
	FReply NativeOnTouchStarted(const FGeometry&, const FPointerEvent&) override { return FReply::Handled(); }
	FReply NativeOnTouchMoved(const FGeometry&, const FPointerEvent&) override { return FReply::Handled(); }
	FReply NativeOnTouchEnded(const FGeometry&, const FPointerEvent&) override { return FReply::Handled(); }
private:
	void BuildContent();
	UFUNCTION() void HandleConfirm();
	UFUNCTION() void HandleCancel();
	UPROPERTY() TObjectPtr<UTextBlock> mTitle;
	UPROPERTY() TObjectPtr<UTextBlock> mBody;
	UPROPERTY() TObjectPtr<UTextBlock> mConfirmLabel;
	UPROPERTY() TObjectPtr<UTextBlock> mCancelLabel;
};
