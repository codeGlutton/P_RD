#pragma once

#include "UI/RDUserWidget.h"
#include "CreditsPanelWidget.generated.h"

class UFont;
class UTexture2D;

/** Offline credits and license reader, shared by title and in-game settings. */
UCLASS()
class P_RD_API UCreditsPanelWidget : public URDUserWidget
{
	GENERATED_BODY()

public:
	UCreditsPanelWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	void ShowPage(bool bLicenses, bool bKorean);
	void ReturnToSettings();
	virtual bool HandleBackNavigation() override { ReturnToSettings(); return true; }
	FSimpleDelegate OnReturnToSettings;

	UFont* GetReaderFont() const { return mReaderFont; }
	UTexture2D* GetBookTexture() const { return mBookTexture; }
	UTexture2D* GetRibbonTexture() const { return mRibbonTexture; }
	UTexture2D* GetChoiceTexture() const { return mChoiceTexture; }
	UTexture2D* GetSelectedChoiceTexture() const { return mSelectedChoiceTexture; }
	UTexture2D* GetNavigationTexture() const { return mNavigationTexture; }
	UTexture2D* GetBackTexture() const { return mBackTexture; }
	UTexture2D* GetDividerTexture() const { return mDividerTexture; }
	bool IsLicensePage() const { return mLicensePage; }
	bool IsKorean() const { return mKorean; }
	static FString LegalDirectory();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
	UPROPERTY()
	TObjectPtr<UFont> mReaderFont;
	UPROPERTY()
	TObjectPtr<UTexture2D> mBookTexture;
	UPROPERTY()
	TObjectPtr<UTexture2D> mRibbonTexture;
	UPROPERTY()
	TObjectPtr<UTexture2D> mChoiceTexture;
	UPROPERTY()
	TObjectPtr<UTexture2D> mSelectedChoiceTexture;
	UPROPERTY()
	TObjectPtr<UTexture2D> mNavigationTexture;
	UPROPERTY()
	TObjectPtr<UTexture2D> mBackTexture;
	UPROPERTY()
	TObjectPtr<UTexture2D> mDividerTexture;
	TSharedPtr<class SCreditsReader> mReader;
	bool mLicensePage = false;
	bool mKorean = true;
};
