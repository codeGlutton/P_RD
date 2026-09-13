#include "UI/CreditsPanelWidget.h"
#include "UI/SCenteredSafeZone.h"

#include "Engine/Font.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateNoResource.h"
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformProcess.h"
#include "InputCoreTypes.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"
#include "UObject/ConstructorHelpers.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace RDCredits
{
	struct FPage
	{
		const TCHAR* Korean;
		const TCHAR* English;
		const TCHAR* File;
		const TCHAR* KoreanFile = nullptr;
	};
	const TArray<FPage> Credits = {
		{ TEXT("모델 · 애니메이션"), TEXT("Models & Animation"), TEXT("Credits/Models.txt") },
		{ TEXT("AI 제작"), TEXT("AI Generated"), TEXT("Credits/AI.txt"), TEXT("Credits/AI_KO.txt") },
		{ TEXT("음악 · 효과음"), TEXT("Music & Sound"), TEXT("Credits/Audio.txt") },
		{ TEXT("시각 효과"), TEXT("Visual Effects"), TEXT("Credits/VFX.txt") },
		{ TEXT("글꼴"), TEXT("Fonts"), TEXT("Credits/Fonts.txt") },
		{ TEXT("게임 엔진"), TEXT("Game Engine"), TEXT("Credits/Engine.txt") }
	};
	const TArray<FPage> Licenses = {
		{ TEXT("개인정보처리방침"), TEXT("Privacy Policy"), TEXT("Policies/Privacy.txt"), TEXT("Policies/Privacy_KO.txt") },
		{ TEXT("고운바탕"), TEXT("Gowun Batang"), TEXT("Licenses/GowunBatang.txt") },
		{ TEXT("LINE Seed Sans KR"), TEXT("LINE Seed Sans KR"), TEXT("Licenses/LINESeedKR.txt") },
		{ TEXT("Oswald"), TEXT("Oswald"), TEXT("Licenses/Oswald.txt") },
		{ TEXT("Roboto"), TEXT("Roboto"), TEXT("Licenses/Roboto.txt") },
		{ TEXT("Noto Arabic / Thai"), TEXT("Noto Arabic / Thai"), TEXT("Licenses/Noto.txt") },
		{ TEXT("Droid Sans"), TEXT("Droid Sans"), TEXT("Licenses/DroidSans.txt") },
		{ TEXT("Last Resort"), TEXT("Last Resort"), TEXT("Licenses/LastResort.txt") }
	};
	const FLinearColor Ink(.12f, .055f, .018f, 1.f);
	const FLinearColor MutedInk(.24f, .13f, .045f, 1.f);
	const FLinearColor Blue(.015f, .065f, .12f, 1.f);
	const FLinearColor Cream(1.f, .87f, .60f, 1.f);
}

/** Uses the same authored wood, gold and ribbon pieces as the settings ledger. */
class SCreditsReader : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCreditsReader) {}
		SLATE_ARGUMENT(UCreditsPanelWidget*, Owner)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args)
	{
		Owner = Args._Owner;
		BookBrush = Art(Owner->GetBookTexture());
		RibbonBrush = Art(Owner->GetRibbonTexture());
		DividerBrush = Art(Owner->GetDividerTexture());
		TabStyle = PlateStyle(Art(Owner->GetChoiceTexture()), FMargin(30.f, 16.f, 30.f, 22.f));
		SelectedTabStyle = PlateStyle(Art(Owner->GetSelectedChoiceTexture()), FMargin(30.f, 16.f, 30.f, 22.f));
		NavigationStyle = PlateStyle(Art(Owner->GetNavigationTexture()), FMargin(48.f, 12.f, 48.f, 15.f));
		SelectedNavigationStyle = PlateStyle(RibbonBrush, FMargin(65.f, 12.f, 65.f, 22.f));
		BackStyle = PlateStyle(Art(Owner->GetBackTexture()), FMargin(63.f, 25.f, 63.f, 49.f));
		LinkStyle = PlateStyle(FSlateNoResource(), FMargin(0.f, 3.f));
		ScrollStyle = FCoreStyle::Get().GetWidgetStyle<FScrollBoxStyle>("ScrollBox");
		ScrollStyle.TopShadowBrush = FSlateNoResource();
		ScrollStyle.BottomShadowBrush = FSlateNoResource();
		ScrollStyle.LeftShadowBrush = FSlateNoResource();
		ScrollStyle.RightShadowBrush = FSlateNoResource();
		ScrollBarStyle = FCoreStyle::Get().GetWidgetStyle<FScrollBarStyle>("ScrollBar");
		ScrollBarStyle.NormalThumbImage = FSlateColorBrush(FLinearColor(.38f, .20f, .065f, .65f));
		ScrollBarStyle.HoveredThumbImage = FSlateColorBrush(FLinearColor(.52f, .30f, .08f, 1.f));
		ScrollBarStyle.DraggedThumbImage = ScrollBarStyle.HoveredThumbImage;
		ScrollBarStyle.VerticalBackgroundImage = FSlateNoResource();

		ChildSlot
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FLinearColor(.026f, .018f, .012f, .94f))
			]
			+ SOverlay::Slot().Padding(20.f)
			[
				SNew(SCenteredSafeZone).IsTitleSafe(true)
				[
					SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
					[
						SNew(SBox).WidthOverride(1920.f).HeightOverride(1080.f)
						[
							SNew(SOverlay)
							+ SOverlay::Slot().Padding(175.f, 28.f, 175.f, 51.f)
							[ SNew(SImage).Image(&BookBrush) ]
							+ SOverlay::Slot().Padding(665.f, 62.f, 665.f, 932.f)
							[ SAssignNew(MainTitle, STextBlock).Font(Font(48, true)).ColorAndOpacity(RDCredits::Ink).Justification(ETextJustify::Center) ]
							+ SOverlay::Slot().Padding(327.f, 218.f, 1043.f, 762.f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 18.f, 0.f)[ Tab(false) ]
								+ SHorizontalBox::Slot().FillWidth(1.f)[ Tab(true) ]
							]
							+ SOverlay::Slot().Padding(345.f, 328.f, 1060.f, 706.f)
							[ SAssignNew(ContentsCaption, STextBlock).Font(Font(22)).ColorAndOpacity(RDCredits::MutedInk).Justification(ETextJustify::Center) ]
							+ SOverlay::Slot().Padding(329.f, 389.f, 1040.f, 230.f)
							[
								SAssignNew(Navigation, SScrollBox).Style(&ScrollStyle).ScrollBarStyle(&ScrollBarStyle)
								.ScrollBarThickness(FVector2D(5.f, 5.f)).ConsumeMouseWheel(EConsumeMouseWheel::Always)
							]
							+ SOverlay::Slot().Padding(1018.f, 196.f, 302.f, 744.f)
							[
								SNew(SOverlay)
								+ SOverlay::Slot()[ SNew(SImage).Image(&RibbonBrush) ]
								+ SOverlay::Slot().Padding(82.f, 23.f, 82.f, 34.f).VAlign(VAlign_Center)
								[ SAssignNew(PageHeading, STextBlock).Font(PlateFont(29)).ColorAndOpacity(RDCredits::Cream).Justification(ETextJustify::Center) ]
							]
							+ SOverlay::Slot().Padding(1045.f, 342.f, 338.f, 692.f)
							[ SAssignNew(PageHint, STextBlock).Font(Font(19)).ColorAndOpacity(RDCredits::MutedInk).Justification(ETextJustify::Center) ]
							+ SOverlay::Slot().Padding(1080.f, 379.f, 377.f, 689.f)
							[ SNew(SImage).Image(&DividerBrush).ColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, .65f)) ]
							+ SOverlay::Slot().Padding(1046.f, 412.f, 328.f, 290.f)
							[
								SAssignNew(Document, SScrollBox).Style(&ScrollStyle).ScrollBarStyle(&ScrollBarStyle)
								.ScrollBarAlwaysVisible(true).ScrollBarThickness(FVector2D(6.f, 6.f)).ConsumeMouseWheel(EConsumeMouseWheel::Always)
							]
							+ SOverlay::Slot().Padding(387.f, 893.f, 1173.f, 57.f)
							[
								SNew(SButton).ButtonStyle(&BackStyle).HAlign(HAlign_Center).VAlign(VAlign_Center)
								.OnClicked_Lambda([this]() { return Return(); })
								[ SNew(STextBlock).Text(Label(TEXT("뒤로가기"), TEXT("Back"))).Font(PlateFont(28)).ColorAndOpacity(RDCredits::Cream) ]
							]
							+ SOverlay::Slot().Padding(1070.f, 812.f, 349.f, 213.f)
							[ Text(Label(TEXT("스크롤하여 전체 보기"), TEXT("Scroll to read more")), 18, RDCredits::MutedInk, ETextJustify::Center) ]
						]
					]
				]
			]
		];
		SetPage(Owner->IsLicensePage());
	}

	void SetPage(bool bShowLicenses)
	{
		bLicenses = bShowLicenses;
		MainTitle->SetText(bLicenses ? Label(TEXT("정책 · 라이선스"), TEXT("POLICIES & LICENSES")) : Label(TEXT("크레딧"), TEXT("CREDITS")));
		MainTitle->SetFont(Font(bLicenses && !Owner->IsKorean() ? 32 : 48, true));
		ContentsCaption->SetText(bLicenses ? Label(TEXT("개인정보 보호와 저작권 고지"), TEXT("Privacy & copyright notices")) : Label(TEXT("이 모험을 함께 만든 이들"), TEXT("The creators behind this adventure")));
		CreditsTab->SetButtonStyle(bLicenses ? &TabStyle : &SelectedTabStyle);
		LicensesTab->SetButtonStyle(bLicenses ? &SelectedTabStyle : &TabStyle);
		Navigation->ClearChildren();
		NavigationButtons.Reset();
		const auto& Pages = bLicenses ? RDCredits::Licenses : RDCredits::Credits;
		for (int32 Index = 0; Index < Pages.Num(); ++Index)
		{
			const auto& Page = Pages[Index];
			TSharedPtr<SButton> Button;
			Navigation->AddSlot().Padding(0.f, 0.f, 12.f, 4.f)
			[
				SNew(SBox).HeightOverride(86.f)
				[
					SAssignNew(Button, SButton).ButtonStyle(&NavigationStyle).HAlign(HAlign_Center).VAlign(VAlign_Center)
					.TouchMethod(EButtonTouchMethod::PreciseTap).ClickMethod(EButtonClickMethod::PreciseClick)
					.OnClicked_Lambda([this, Index]() { Select(Index); return FReply::Handled(); })
					[
						SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly)
						[ SNew(STextBlock).Text(Label(Page.Korean, Page.English)).Font(PlateFont(26)).ColorAndOpacity(RDCredits::Cream) ]
					]
				]
			];
			NavigationButtons.Add(Button);
		}
		Navigation->ScrollToStart();
		Select(0);
	}

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry&, const FKeyEvent& Event) override
	{
		if (Event.GetKey() == EKeys::Escape || Event.GetKey() == EKeys::Android_Back || Event.GetKey() == EKeys::Gamepad_FaceButton_Right) { return Return(); }
		return FReply::Handled();
	}
	virtual FReply OnMouseButtonDown(const FGeometry&, const FPointerEvent&) override { return FReply::Handled(); }
	virtual FReply OnMouseButtonUp(const FGeometry&, const FPointerEvent&) override { return FReply::Handled(); }
	virtual FReply OnTouchStarted(const FGeometry&, const FPointerEvent&) override { return FReply::Handled(); }
	virtual FReply OnTouchEnded(const FGeometry&, const FPointerEvent&) override { return FReply::Handled(); }
	virtual FReply OnMouseWheel(const FGeometry&, const FPointerEvent&) override { return FReply::Handled(); }

private:
	FSlateBrush Art(UTexture2D* Texture) const
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Brush.ImageSize = Texture ? FVector2D(Texture->GetSizeX(), Texture->GetSizeY()) : FVector2D(1.f, 1.f);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		return Brush;
	}
	FButtonStyle PlateStyle(const FSlateBrush& Brush, const FMargin& Padding) const
	{
		FButtonStyle Style = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
		FSlateBrush Hovered = Brush;
		Hovered.TintColor = FLinearColor(1.14f, 1.14f, 1.14f, 1.f);
		FSlateBrush Pressed = Brush;
		Pressed.TintColor = FLinearColor(.72f, .72f, .72f, 1.f);
		Style.SetNormal(Brush).SetHovered(Hovered).SetPressed(Pressed);
		Style.SetNormalPadding(Padding).SetPressedPadding(Padding);
		return Style;
	}
	FText Label(const TCHAR* Korean, const TCHAR* English) const { return FText::FromString(Owner->IsKorean() ? Korean : English); }
	FSlateFontInfo Font(int32 Size, bool bBold = false) const { return FSlateFontInfo(Owner->GetReaderFont(), Size, FName(bBold ? "Bold" : "Regular")); }
	FSlateFontInfo PlateFont(int32 Size) const
	{
		FSlateFontInfo Result = Font(Size, true);
		Result.OutlineSettings.OutlineSize = 1;
		Result.OutlineSettings.OutlineColor = FLinearColor(.025f, .012f, .004f, 1.f);
		return Result;
	}
	TSharedRef<SWidget> Text(const FText& Value, int32 Size, FLinearColor Color, ETextJustify::Type Align = ETextJustify::Left, bool bBold = false)
	{
		return SNew(STextBlock).Text(Value).Font(Font(Size, bBold)).ColorAndOpacity(Color).AutoWrapText(true)
			.WrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping).Justification(Align);
	}
	TSharedRef<SWidget> Tab(bool bLicenseTab)
	{
		TSharedPtr<SButton>& Button = bLicenseTab ? LicensesTab : CreditsTab;
		return SAssignNew(Button, SButton).ButtonStyle(&TabStyle).HAlign(HAlign_Center).VAlign(VAlign_Center)
			.OnClicked_Lambda([this, bLicenseTab]() { SetPage(bLicenseTab); return FReply::Handled(); })
			[ SNew(STextBlock).Text(bLicenseTab ? Label(TEXT("라이선스"), TEXT("Licenses")) : Label(TEXT("크레딧"), TEXT("Credits"))).Font(PlateFont(27)).ColorAndOpacity(RDCredits::Cream) ];
	}
	void Select(int32 Index)
	{
		for (int32 ButtonIndex = 0; ButtonIndex < NavigationButtons.Num(); ++ButtonIndex)
		{
			NavigationButtons[ButtonIndex]->SetButtonStyle(ButtonIndex == Index ? &SelectedNavigationStyle : &NavigationStyle);
		}
		const auto& Page = (bLicenses ? RDCredits::Licenses : RDCredits::Credits)[Index];
		PageHeading->SetText(Label(Page.Korean, Page.English));
		PageHint->SetText(bLicenses && Index == 0
			? Label(TEXT("게임 데이터와 이용자 문의"), TEXT("Game data & support enquiries"))
			: (bLicenses ? Label(TEXT("저작권 고지 · 라이선스 원문"), TEXT("Copyright notice · Original license")) : Label(TEXT("사용된 작품과 제작자"), TEXT("Featured works & creators"))));
		FString Content;
		const TCHAR* DocumentFile = Owner->IsKorean() && Page.KoreanFile ? Page.KoreanFile : Page.File;
		const FString Path = FPaths::Combine(UCreditsPanelWidget::LegalDirectory(), DocumentFile);
		if (!FFileHelper::LoadFileToString(Content, *Path))
		{
			UE_LOG(LogTemp, Error, TEXT("Credits: unable to read %s"), *Path);
			Content = Label(TEXT("문서를 불러오지 못했습니다. 앱을 다시 실행해 주세요."), TEXT("This document could not be loaded. Please restart the app.")).ToString();
		}
		Content.ReplaceInline(TEXT("\r"), TEXT(""));
		TArray<FString> Paragraphs;
		Content.ParseIntoArray(Paragraphs, TEXT("\n\n"), true);
		Document->ClearChildren();
		for (const FString& RawParagraph : Paragraphs)
		{
			const FString Paragraph = RawParagraph.TrimStartAndEnd();
			if (Paragraph.IsEmpty()) { continue; }
			if (bLicenses)
			{
				Document->AddSlot().Padding(0.f, 0.f, 22.f, 20.f)[ Text(FText::FromString(Paragraph), 20, RDCredits::Ink) ];
				continue;
			}
			TSharedRef<SVerticalBox> Entry = SNew(SVerticalBox);
			TArray<FString> Lines;
			Paragraph.ParseIntoArrayLines(Lines, true);
			for (int32 LineIndex = 0; LineIndex < Lines.Num(); ++LineIndex)
			{
				const FString& Line = Lines[LineIndex];
				if (Line.StartsWith(TEXT("https://")))
				{
					Entry->AddSlot().AutoHeight().HAlign(HAlign_Left).Padding(0.f, 6.f, 0.f, 0.f)
					[
						SNew(SButton).ButtonStyle(&LinkStyle).ToolTipText(FText::FromString(Line))
						.TouchMethod(EButtonTouchMethod::PreciseTap).ClickMethod(EButtonClickMethod::PreciseClick)
						.OnClicked_Lambda([Line]() { FPlatformProcess::LaunchURL(*Line, nullptr, nullptr); return FReply::Handled(); })
						[ SNew(STextBlock).Text(Label(TEXT("원본 보기"), TEXT("View source"))).Font(Font(18, true)).ColorAndOpacity(RDCredits::Blue) ]
					];
					continue;
				}
				FString Author, Work;
				if (LineIndex == 0 && Line.Split(TEXT(" — "), &Author, &Work))
				{
					Entry->AddSlot().AutoHeight()[ Text(FText::FromString(Author), 26, RDCredits::Ink, ETextJustify::Left, true) ];
					Entry->AddSlot().AutoHeight().Padding(0.f, 5.f, 0.f, 0.f)[ Text(FText::FromString(Work), 21, RDCredits::MutedInk) ];
				}
				else { Entry->AddSlot().AutoHeight()[ Text(FText::FromString(Line), 21, RDCredits::Ink) ]; }
			}
			Document->AddSlot().Padding(0.f, 0.f, 22.f, 20.f)[ Entry ];
		}
		Document->ScrollToStart();
	}
	FReply Return() { if (Owner.IsValid()) { Owner->ReturnToSettings(); } return FReply::Handled(); }
	TWeakObjectPtr<UCreditsPanelWidget> Owner;
	FSlateBrush BookBrush, RibbonBrush, DividerBrush;
	FButtonStyle TabStyle, SelectedTabStyle, NavigationStyle, SelectedNavigationStyle, BackStyle, LinkStyle;
	FScrollBoxStyle ScrollStyle;
	FScrollBarStyle ScrollBarStyle;
	TSharedPtr<SScrollBox> Navigation, Document;
	TSharedPtr<STextBlock> MainTitle, ContentsCaption, PageHeading, PageHint;
	TSharedPtr<SButton> CreditsTab, LicensesTab;
	TArray<TSharedPtr<SButton>> NavigationButtons;
	bool bLicenses = false;
};

UCreditsPanelWidget::UCreditsPanelWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	mViewportZOrder = 11;
	mRemoveFromParentOnClose = true;
	SetIsFocusable(true);
	static ConstructorHelpers::FObjectFinder<UFont> Font(TEXT("/Game/SVN/OutSideAsset/Fonts/F_HUD_LINESeedKR.F_HUD_LINESeedKR"));
	mReaderFont = Font.Object;
	static ConstructorHelpers::FObjectFinder<UTexture2D> Book(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/SettingsLedger/T_MB_SettingsLedger_BookBase.T_MB_SettingsLedger_BookBase"));
	mBookTexture = Book.Object;
	static ConstructorHelpers::FObjectFinder<UTexture2D> Ribbon(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/SettingsLedger/T_MB_SettingsLedger_SectionRibbon.T_MB_SettingsLedger_SectionRibbon"));
	mRibbonTexture = Ribbon.Object;
	static ConstructorHelpers::FObjectFinder<UTexture2D> Choice(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/SettingsLedger/T_MB_SettingsLedger_ChoiceButton.T_MB_SettingsLedger_ChoiceButton"));
	mChoiceTexture = Choice.Object;
	static ConstructorHelpers::FObjectFinder<UTexture2D> SelectedChoice(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/SettingsLedger/T_MB_SettingsLedger_ChoiceButton_Selected.T_MB_SettingsLedger_ChoiceButton_Selected"));
	mSelectedChoiceTexture = SelectedChoice.Object;
	static ConstructorHelpers::FObjectFinder<UTexture2D> NavigationArt(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_Button_Wide_Normal.T_KitA_Button_Wide_Normal"));
	mNavigationTexture = NavigationArt.Object;
	static ConstructorHelpers::FObjectFinder<UTexture2D> Back(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/SettingsLedger/T_MB_SettingsLedger_ActionButton.T_MB_SettingsLedger_ActionButton"));
	mBackTexture = Back.Object;
	static ConstructorHelpers::FObjectFinder<UTexture2D> Divider(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Hire/T_Hire_Divider_V11.T_Hire_Divider_V11"));
	mDividerTexture = Divider.Object;
}

FString UCreditsPanelWidget::LegalDirectory() { return FPaths::ProjectContentDir() / TEXT("Legal"); }

void UCreditsPanelWidget::ShowPage(bool bLicenses, bool bKorean)
{
	mLicensePage = bLicenses;
	mKorean = bKorean;
	OpenUI();
	if (mReader.IsValid())
	{
		mReader->SetPage(bLicenses);
		FSlateApplication::Get().SetKeyboardFocus(mReader, EFocusCause::SetDirectly);
	}
}

void UCreditsPanelWidget::ReturnToSettings()
{
	CloseUI();
	OnReturnToSettings.ExecuteIfBound();
}

TSharedRef<SWidget> UCreditsPanelWidget::RebuildWidget()
{
	return SAssignNew(mReader, SCreditsReader).Owner(this);
}

void UCreditsPanelWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	mReader.Reset();
}
