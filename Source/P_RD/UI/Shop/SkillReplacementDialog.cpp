#include "UI/Shop/SkillReplacementDialog.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Brushes/SlateColorBrush.h"

#define LOCTEXT_NAMESPACE "SkillReplacementDialog"

USkillReplacementDialog::USkillReplacementDialog(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mViewportZOrder = 10020;
}

bool USkillReplacementDialog::Initialize()
{
	const bool Result = Super::Initialize();
	BuildContent();
	return Result;
}

void USkillReplacementDialog::BuildContent()
{
	if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	if (WidgetTree->RootWidget) return;
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("ReplaceRoot"));
	WidgetTree->RootWidget = Root;
	UButton* Shield = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ReplaceInputShield"));
	FButtonStyle ShieldStyle;
	// Keep the modal input shield without dimming the screen behind the dialog.
	ShieldStyle.SetNormal(FSlateColorBrush(FLinearColor::Transparent));
	ShieldStyle.SetHovered(ShieldStyle.Normal);
	ShieldStyle.SetPressed(ShieldStyle.Normal);
	ShieldStyle.SetDisabled(ShieldStyle.Normal);
	Shield->SetStyle(ShieldStyle);
	UOverlaySlot* ShieldSlot = Root->AddChildToOverlay(Shield);
	ShieldSlot->SetHorizontalAlignment(HAlign_Fill);
	ShieldSlot->SetVerticalAlignment(VAlign_Fill);

	UScaleBox* Fit = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("ReplaceFit"));
	Fit->SetStretch(EStretch::ScaleToFit);
	Fit->SetStretchDirection(EStretchDirection::DownOnly);
	UOverlaySlot* FitSlot = Root->AddChildToOverlay(Fit);
	FitSlot->SetHorizontalAlignment(HAlign_Fill);
	FitSlot->SetVerticalAlignment(VAlign_Fill);
	FitSlot->SetPadding(FMargin(24.f));
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	Size->SetWidthOverride(840.f);
	Size->SetHeightOverride(440.f);
	Fit->SetContent(Size);
	UCanvasPanel* Panel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	Size->SetContent(Panel);
	auto Place = [Panel](UWidget* Widget, FVector2D Position, FVector2D Dimensions)
	{
		UCanvasPanelSlot* Slot = Panel->AddChildToCanvas(Widget);
		Slot->SetPosition(Position);
		Slot->SetSize(Dimensions);
	};
	UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Frame->SetBrush(FSlateColorBrush(FLinearColor(.62f, .38f, .12f)));
	Place(Frame, FVector2D::ZeroVector, FVector2D(840.f, 440.f));
	UBorder* Surface = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Surface->SetBrush(FSlateColorBrush(FLinearColor(.065f, .037f, .02f)));
	Place(Surface, FVector2D(4.f, 4.f), FVector2D(832.f, 432.f));
	auto Text = [this, &Place](FName Name, const FText& Value, FVector2D Position, FVector2D Dimensions)
	{
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Label->SetText(Value);
		Label->SetJustification(ETextJustify::Center);
		Label->SetColorAndOpacity(FSlateColor(FLinearColor(.98f, .91f, .74f)));
		Label->SetAutoWrapText(true);
		Label->SetVisibility(ESlateVisibility::HitTestInvisible);
		Place(Label, Position, Dimensions);
		return Label;
	};
	mTitle = Text(TEXT("ReplaceTitle"), LOCTEXT("Title", "스킬을 교체하시겠습니까?"), FVector2D(40.f, 42.f), FVector2D(760.f, 64.f));
	mBody = Text(TEXT("ReplaceBody"), FText::GetEmpty(), FVector2D(54.f, 132.f), FVector2D(732.f, 160.f));
	auto Button = [this, &Place](FName Name, FVector2D Position, FLinearColor Color)
	{
		UButton* Result = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		FButtonStyle Style;
		Style.SetNormal(FSlateColorBrush(Color));
		Style.SetHovered(FSlateColorBrush(Color * 1.2f));
		Style.SetPressed(FSlateColorBrush(Color * .8f));
		Style.NormalPadding = FMargin(8.f);
		Style.PressedPadding = FMargin(8.f);
		Result->SetStyle(Style);
		Place(Result, Position, FVector2D(300.f, 76.f));
		return Result;
	};
	UButton* Cancel = Button(TEXT("ReplaceCancel"), FVector2D(90.f, 318.f), FLinearColor(.15f, .18f, .20f));
	UButton* Confirm = Button(TEXT("ReplaceConfirm"), FVector2D(450.f, 318.f), FLinearColor(.48f, .22f, .045f));
	mCancelLabel = Text(TEXT("ReplaceCancelLabel"), LOCTEXT("Cancel", "취소"), FVector2D::ZeroVector, FVector2D::ZeroVector);
	mConfirmLabel = Text(TEXT("ReplaceConfirmLabel"), LOCTEXT("Confirm", "교체"), FVector2D::ZeroVector, FVector2D::ZeroVector);
	Cancel->SetContent(mCancelLabel);
	Confirm->SetContent(mConfirmLabel);
	Cancel->OnClicked.AddDynamic(this, &USkillReplacementDialog::HandleCancel);
	Confirm->OnClicked.AddDynamic(this, &USkillReplacementDialog::HandleConfirm);
}

void USkillReplacementDialog::SetSkills(const FText& OldSkill, const FText& NewSkill, const FSlateFontInfo& Font)
{
	BuildContent();
	mBody->SetText(FText::Format(LOCTEXT("Body", "기존 스킬 ‘{0}’이 삭제됩니다.\n새 스킬 ‘{1}’을 장착합니다."), OldSkill, NewSkill));
	for (UTextBlock* Label : { mTitle.Get(), mBody.Get(), mConfirmLabel.Get(), mCancelLabel.Get() })
	{
		FSlateFontInfo LabelFont = Font;
		LabelFont.Size = Label == mTitle ? 34 : Label == mBody ? 26 : 30;
		Label->SetFont(LabelFont);
	}
}

void USkillReplacementDialog::ApplyOpenUI()
{
	Super::ApplyOpenUI();
	if (GetOwningPlayer())
		if (UButton* Cancel = Cast<UButton>(GetWidgetFromName(TEXT("ReplaceCancel"))))
			Cancel->SetUserFocus(GetOwningPlayer());
}

void USkillReplacementDialog::HandleConfirm() { const FSimpleDelegate Callback = OnConfirmed; Callback.ExecuteIfBound(); }
void USkillReplacementDialog::HandleCancel() { const FSimpleDelegate Callback = OnCancelled; Callback.ExecuteIfBound(); }
bool USkillReplacementDialog::HandleBackNavigation() { HandleCancel(); return true; }

#undef LOCTEXT_NAMESPACE
