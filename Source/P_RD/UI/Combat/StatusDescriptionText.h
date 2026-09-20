#pragma once

#include "Components/Widget.h"
#include "GameplayTagContainer.h"
#include "Styling/SlateTypes.h"
#include "StatusDescriptionText.generated.h"

class SRichTextBlock;
class FSlateStyleSet;

/** Skill descriptions retain their text while known status names open the shared glossary. */
UCLASS()
class P_RD_API UStatusDescriptionText : public UWidget
{
    GENERATED_BODY()
public:
    void SetDescription(const FText& Text, const FSlateFontInfo& Font, FSlateColor Color, float WrapWidth);
    static FString MakeStatusMarkup(const FString& Text);
    DECLARE_DELEGATE_OneParam(FOnStatusClicked, FGameplayTag);
    FOnStatusClicked OnStatusClicked;
    virtual void ReleaseSlateResources(bool bReleaseChildren) override;
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
private:
    FText Markup;
    FTextBlockStyle TextStyle;
    TSharedPtr<FSlateStyleSet> Styles;
    TSharedPtr<SRichTextBlock> RichText;
    float Width = 500.f;
};
