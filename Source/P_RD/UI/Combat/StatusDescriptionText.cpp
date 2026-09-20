#include "UI/Combat/StatusDescriptionText.h"
#include "UI/Combat/CombatStatusPresentation.h"
#include "GameplayTagsManager.h"
#include "GameplayTagType.h"
#include "Framework/Text/TextDecorators.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateStyle.h"
#include "Widgets/Text/SRichTextBlock.h"

namespace
{
FString EscapeStatusText(FString Text)
{
    return Text.Replace(TEXT("&"), TEXT("&amp;")).Replace(TEXT("<"), TEXT("&lt;"))
        .Replace(TEXT(">"), TEXT("&gt;")).Replace(TEXT("\""), TEXT("&quot;"));
}
}

FString UStatusDescriptionText::MakeStatusMarkup(const FString& Text)
{
    struct FTerm { FString Name; FGameplayTag Tag; };
    TArray<FTerm> Terms;
    FGameplayTagContainer Tags = UGameplayTagsManager::Get().RequestGameplayTagChildren(EffectTags::GameplayEffect_StatusEffect);
    for (const FGameplayTag& Tag : Tags)
    {
        const auto Info = CombatStatusUI::Resolve(Tag);
        if (Info.mDisplayName.IsEmpty() || !CombatStatusUI::HasDescription(Tag)) continue;
        Terms.Add({Info.mDisplayName.ToString(), Tag});
    }
    Terms.Sort([](const FTerm& A, const FTerm& B) { return A.Name.Len() > B.Name.Len(); });
    FString Result;
    for (int32 Offset = 0; Offset < Text.Len();)
    {
        const FTerm* Match = Terms.FindByPredicate([&](const FTerm& Term)
        {
            if (!Text.Mid(Offset, Term.Name.Len()).Equals(Term.Name, ESearchCase::IgnoreCase)) return false;
            // English words must not match inside another word. Korean particles may follow a name.
            const bool bLatin = Term.Name[0] < 128;
            auto IsLatinLetter = [](TCHAR C) { return C < 128 && FChar::IsAlpha(C); };
            return !bLatin || ((Offset == 0 || !IsLatinLetter(Text[Offset - 1]))
                && (Offset + Term.Name.Len() == Text.Len() || !IsLatinLetter(Text[Offset + Term.Name.Len()])));
        });
        if (Match)
        {
            Result += FString::Printf(TEXT("<a id=\"status\" style=\"StatusLink\" tag=\"%s\">%s</>"),
                *Match->Tag.ToString(), *EscapeStatusText(Text.Mid(Offset, Match->Name.Len())));
            Offset += Match->Name.Len();
        }
        else
        {
            Result += EscapeStatusText(Text.Mid(Offset++, 1));
        }
    }
    return Result;
}

void UStatusDescriptionText::SetDescription(const FText& Text, const FSlateFontInfo& Font, FSlateColor Color, float WrapWidth)
{
    Markup = FText::FromString(MakeStatusMarkup(Text.ToString()));
    TextStyle.SetFont(Font).SetColorAndOpacity(Color);
    Width = WrapWidth;
    if (!Styles) Styles = MakeShared<FSlateStyleSet>(TEXT("SkillStatusGlossary"));
    FHyperlinkStyle Link = FCoreStyle::Get().GetWidgetStyle<FHyperlinkStyle>(TEXT("Hyperlink"));
    Link.SetTextStyle(FTextBlockStyle(TextStyle).SetColorAndOpacity(FLinearColor(.95f, .65f, .20f)));
    Styles->Set(TEXT("StatusLink"), Link);
    if (RichText)
    {
        RichText->SetTextStyle(TextStyle);
        RichText->SetWrapTextAt(Width);
        RichText->SetText(Markup);
    }
}

TSharedRef<SWidget> UStatusDescriptionText::RebuildWidget()
{
    if (!Styles) SetDescription(FText::GetEmpty(), FCoreStyle::GetDefaultFontStyle("Regular", 24), FLinearColor::White, Width);
    TArray<TSharedRef<ITextDecorator>> Decorators;
    Decorators.Add(FHyperlinkDecorator::Create(TEXT("status"), FSlateHyperlinkRun::FOnClick::CreateWeakLambda(this,
        [this](const TMap<FString, FString>& Metadata)
        {
            if (const FString* Tag = Metadata.Find(TEXT("tag")))
                OnStatusClicked.ExecuteIfBound(FGameplayTag::RequestGameplayTag(FName(**Tag), false));
        })));
    return SAssignNew(RichText, SRichTextBlock).Text(Markup).TextStyle(&TextStyle)
        .DecoratorStyleSet(Styles.Get()).Decorators(Decorators).AutoWrapText(true).WrapTextAt(Width)
        .LineHeightPercentage(1.32f);
}

void UStatusDescriptionText::ReleaseSlateResources(bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);
    RichText.Reset();
}
