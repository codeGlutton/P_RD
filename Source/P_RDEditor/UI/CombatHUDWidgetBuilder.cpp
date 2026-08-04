#include "UI/CombatHUDWidgetBuilder.h"

#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "HAL/IConsoleManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/SavePackage.h"

namespace CombatHUDWidgetBuilder
{
	constexpr TCHAR AssetPath[] =
		TEXT("/Game/UI/CombatLayouts/WBP_CombatHUD04.WBP_CombatHUD04");
	TUniquePtr<FAutoConsoleCommand> BuildCommand;

	template <typename T>
	T* FindOrCreate(UWidgetBlueprint* Blueprint, const FName Name)
	{
		if (UWidget* Existing = Blueprint->WidgetTree->FindWidget(Name))
		{
			return CastChecked<T>(Existing);
		}
		T* NewWidget = Blueprint->WidgetTree->ConstructWidget<T>(T::StaticClass(), Name);
		Blueprint->OnVariableAdded(Name);
		return NewWidget;
	}

	void EnsureParent(UPanelWidget* Parent, UWidget* Child)
	{
		check(Parent != nullptr && Child != nullptr);
		if (Child->GetParent() == Parent)
		{
			return;
		}
		if (UPanelWidget* OldParent = Child->GetParent())
		{
			OldParent->RemoveChild(Child);
		}
		Parent->AddChild(Child);
	}

	void PlaceCanvas(UCanvasPanel* Parent, UWidget* Child, const FVector2D Position,
		const FVector2D Size, const int32 ZOrder)
	{
		EnsureParent(Parent, Child);
		UCanvasPanelSlot* Slot = CastChecked<UCanvasPanelSlot>(Child->Slot);
		Slot->SetAnchors(FAnchors(0.f));
		Slot->SetAlignment(FVector2D::ZeroVector);
		Slot->SetAutoSize(false);
		Slot->SetPosition(Position);
		Slot->SetSize(Size);
		Slot->SetZOrder(ZOrder);
	}

	void SetReadableFont(UTextBlock* Text, const FSlateFontInfo& Source, const int32 Size)
	{
		FSlateFontInfo Font = Source;
		Font.Size = Size;
		Font.OutlineSettings.OutlineSize = 2;
		Font.OutlineSettings.OutlineColor = FLinearColor(0.f, 0.f, 0.f, 0.9f);
		Text->SetFont(Font);
		Text->SetJustification(ETextJustify::Center);
		Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Text->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	void SetInvisibleButtonChrome(UButton* Button)
	{
		FButtonStyle Style = Button->GetStyle();
		Style.Normal.DrawAs = ESlateBrushDrawType::NoDrawType;
		Style.Hovered.DrawAs = ESlateBrushDrawType::NoDrawType;
		Style.Pressed.DrawAs = ESlateBrushDrawType::NoDrawType;
		Style.Disabled.DrawAs = ESlateBrushDrawType::NoDrawType;
		Button->SetStyle(Style);
	}

	void BuildEnemySummary(UWidgetBlueprint* Blueprint, const FSlateFontInfo& BaseFont,
		UTexture2D* PanelTexture, UTexture2D* PortraitFrameTexture,
		UTexture2D* StatusFrameTexture, UTexture2D* SpeedTexture)
	{
		UCanvasPanel* Root = CastChecked<UCanvasPanel>(Blueprint->WidgetTree->RootWidget);
		UCanvasPanel* Panel = FindOrCreate<UCanvasPanel>(Blueprint, TEXT("EnemyPanel"));
		PlaceCanvas(Root, Panel, FVector2D(-630.f, 178.f), FVector2D(600.f, 430.f), 60);
		if (UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(Panel->Slot))
		{
			PanelSlot->SetAnchors(FAnchors(1.f, 0.f));
		}
		Panel->SetVisibility(ESlateVisibility::Collapsed);

		UImage* Plate = FindOrCreate<UImage>(Blueprint, TEXT("EnemyPlate"));
		PlaceCanvas(Panel, Plate, FVector2D::ZeroVector, FVector2D(600.f, 430.f), 0);
		Plate->SetBrushFromTexture(PanelTexture, false);
		Plate->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UImage* PortraitFrame = FindOrCreate<UImage>(Blueprint, TEXT("EnemyPortraitFrame"));
		PlaceCanvas(Panel, PortraitFrame, FVector2D(38.f, 34.f), FVector2D(126.f, 126.f), 5);
		PortraitFrame->SetBrushFromTexture(PortraitFrameTexture, false);
		PortraitFrame->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UImage* Portrait = FindOrCreate<UImage>(Blueprint, TEXT("EnemyPortrait"));
		PlaceCanvas(Panel, Portrait, FVector2D(51.f, 47.f), FVector2D(100.f, 100.f), 6);
		Portrait->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		Portrait->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UBorder* BadgePlate = FindOrCreate<UBorder>(Blueprint, TEXT("EnemyBadgePlate"));
		PlaceCanvas(Panel, BadgePlate, FVector2D(180.f, 45.f), FVector2D(62.f, 42.f), 6);
		BadgePlate->SetBrushColor(FLinearColor(.68f, .07f, .035f, .96f));
		BadgePlate->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UTextBlock* Badge = FindOrCreate<UTextBlock>(Blueprint, TEXT("EnemyBadgeText"));
		PlaceCanvas(Panel, Badge, FVector2D(180.f, 46.f), FVector2D(62.f, 40.f), 7);
		Badge->SetText(NSLOCTEXT("CombatHUD", "EnemyBadge", "적"));
		SetReadableFont(Badge, BaseFont, 24);

		UTextBlock* Name = FindOrCreate<UTextBlock>(Blueprint, TEXT("EnemyName"));
		PlaceCanvas(Panel, Name, FVector2D(254.f, 37.f), FVector2D(300.f, 62.f), 7);
		Name->SetText(NSLOCTEXT("CombatHUD", "EnemyNamePreview", "독수리"));
		SetReadableFont(Name, BaseFont, 38);
		Name->SetJustification(ETextJustify::Left);

		UBorder* HPBack = FindOrCreate<UBorder>(Blueprint, TEXT("EnemyHPBack"));
		PlaceCanvas(Panel, HPBack, FVector2D(178.f, 105.f), FVector2D(378.f, 58.f), 5);
		HPBack->SetBrushColor(FLinearColor(.08f, .025f, .018f, .96f));
		HPBack->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UProgressBar* HPBar = FindOrCreate<UProgressBar>(Blueprint, TEXT("EnemyHPBar"));
		PlaceCanvas(Panel, HPBar, FVector2D(188.f, 115.f), FVector2D(358.f, 38.f), 6);
		HPBar->SetPercent(1.f);
		HPBar->SetFillColorAndOpacity(FLinearColor(.88f, .055f, .025f, 1.f));

		UTextBlock* HPText = FindOrCreate<UTextBlock>(Blueprint, TEXT("EnemyHPText"));
		PlaceCanvas(Panel, HPText, FVector2D(188.f, 112.f), FVector2D(358.f, 44.f), 7);
		HPText->SetText(NSLOCTEXT("CombatHUD", "EnemyHPPreview", "HP  64 / 64"));
		SetReadableFont(HPText, BaseFont, 27);

		UBorder* APPlate = FindOrCreate<UBorder>(Blueprint, TEXT("EnemyAPPlate"));
		PlaceCanvas(Panel, APPlate, FVector2D(54.f, 184.f), FVector2D(230.f, 58.f), 5);
		APPlate->SetBrushColor(FLinearColor(.025f, .17f, .27f, .97f));
		APPlate->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UTextBlock* APText = FindOrCreate<UTextBlock>(Blueprint, TEXT("EnemyAPText"));
		PlaceCanvas(Panel, APText, FVector2D(54.f, 188.f), FVector2D(230.f, 50.f), 6);
		APText->SetText(NSLOCTEXT("CombatHUD", "EnemyAPPreview", "AP  5 / 5"));
		SetReadableFont(APText, BaseFont, 29);

		UBorder* SpeedPlate = FindOrCreate<UBorder>(Blueprint, TEXT("EnemySpeedPlate"));
		PlaceCanvas(Panel, SpeedPlate, FVector2D(316.f, 184.f), FVector2D(230.f, 58.f), 5);
		SpeedPlate->SetBrushColor(FLinearColor(.21f, .105f, .035f, .97f));
		SpeedPlate->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UImage* SpeedIcon = FindOrCreate<UImage>(Blueprint, TEXT("EnemySpeedIcon"));
		PlaceCanvas(Panel, SpeedIcon, FVector2D(330.f, 191.f), FVector2D(44.f, 44.f), 6);
		SpeedIcon->SetBrushFromTexture(SpeedTexture, false);
		SpeedIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UTextBlock* SpeedText = FindOrCreate<UTextBlock>(Blueprint, TEXT("EnemySpeedText"));
		PlaceCanvas(Panel, SpeedText, FVector2D(376.f, 188.f), FVector2D(158.f, 50.f), 6);
		SpeedText->SetText(NSLOCTEXT("CombatHUD", "EnemySpeedPreview", "속도  4"));
		SetReadableFont(SpeedText, BaseFont, 29);

		UTextBlock* StatusLabel = FindOrCreate<UTextBlock>(Blueprint, TEXT("EnemyStatusLabel"));
		PlaceCanvas(Panel, StatusLabel, FVector2D(52.f, 267.f), FVector2D(94.f, 42.f), 6);
		StatusLabel->SetText(NSLOCTEXT("CombatHUD", "EnemyStatusLabel", "상태"));
		SetReadableFont(StatusLabel, BaseFont, 26);

		for (int32 Index = 0; Index < 3; ++Index)
		{
			const float X = 150.f + 92.f * Index;
			UImage* Frame = FindOrCreate<UImage>(Blueprint,
				FName(*FString::Printf(TEXT("EnemyStatusFrame_%d"), Index)));
			PlaceCanvas(Panel, Frame, FVector2D(X, 252.f), FVector2D(76.f, 76.f), 6);
			Frame->SetBrushFromTexture(StatusFrameTexture, false);
			Frame->SetVisibility(ESlateVisibility::Collapsed);

			UImage* Icon = FindOrCreate<UImage>(Blueprint,
				FName(*FString::Printf(TEXT("EnemyStatusIcon_%d"), Index)));
			PlaceCanvas(Panel, Icon, FVector2D(X + 8.f, 260.f), FVector2D(60.f, 60.f), 7);
			Icon->SetVisibility(ESlateVisibility::Collapsed);

			UTextBlock* Count = FindOrCreate<UTextBlock>(Blueprint,
				FName(*FString::Printf(TEXT("EnemyStatusCount_%d"), Index)));
			PlaceCanvas(Panel, Count, FVector2D(X + 43.f, 293.f), FVector2D(30.f, 30.f), 8);
			Count->SetText(FText::AsNumber(2));
			SetReadableFont(Count, BaseFont, 20);
			Count->SetVisibility(ESlateVisibility::Collapsed);
		}

		UTextBlock* StatusText = FindOrCreate<UTextBlock>(Blueprint, TEXT("EnemyStatus"));
		PlaceCanvas(Panel, StatusText, FVector2D(438.f, 263.f), FVector2D(118.f, 52.f), 7);
		StatusText->SetText(NSLOCTEXT("CombatHUD", "EnemyNoStatus", "상태 없음"));
		SetReadableFont(StatusText, BaseFont, 22);

		UTextBlock* Forecast = FindOrCreate<UTextBlock>(Blueprint, TEXT("EnemyForecast"));
		PlaceCanvas(Panel, Forecast, FVector2D(64.f, 356.f), FVector2D(472.f, 46.f), 7);
		Forecast->SetText(NSLOCTEXT("CombatHUD", "EnemyForecastPreview", "예상 피해  8~14"));
		SetReadableFont(Forecast, BaseFont, 27);

		if (UWidget* LegacyDefense = Blueprint->WidgetTree->FindWidget(TEXT("EnemyDefense")))
		{
			LegacyDefense->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	void BuildAllySummary(UWidgetBlueprint* Blueprint, const FSlateFontInfo& BaseFont,
		UTexture2D* PanelTexture, UTexture2D* PortraitFrameTexture,
		UTexture2D* StatusFrameTexture, UTexture2D* SpeedTexture)
	{
		UCanvasPanel* Root = CastChecked<UCanvasPanel>(Blueprint->WidgetTree->RootWidget);
		UCanvasPanel* Panel = FindOrCreate<UCanvasPanel>(Blueprint, TEXT("AllyPanel"));
		PlaceCanvas(Root, Panel, FVector2D(30.f, 178.f), FVector2D(600.f, 430.f), 60);
		Panel->SetVisibility(ESlateVisibility::Collapsed);

		UImage* Plate = FindOrCreate<UImage>(Blueprint, TEXT("AllyPlate"));
		PlaceCanvas(Panel, Plate, FVector2D::ZeroVector, FVector2D(600.f, 430.f), 0);
		Plate->SetBrushFromTexture(PanelTexture, false);
		Plate->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UImage* PortraitFrame = FindOrCreate<UImage>(Blueprint, TEXT("AllyPortraitFrame"));
		PlaceCanvas(Panel, PortraitFrame, FVector2D(38.f, 34.f), FVector2D(126.f, 126.f), 5);
		PortraitFrame->SetBrushFromTexture(PortraitFrameTexture, false);
		PortraitFrame->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UImage* Portrait = FindOrCreate<UImage>(Blueprint, TEXT("AllyPortrait"));
		PlaceCanvas(Panel, Portrait, FVector2D(51.f, 47.f), FVector2D(100.f, 100.f), 6);
		Portrait->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		Portrait->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UBorder* BadgePlate = FindOrCreate<UBorder>(Blueprint, TEXT("AllyBadgePlate"));
		PlaceCanvas(Panel, BadgePlate, FVector2D(180.f, 45.f), FVector2D(78.f, 42.f), 6);
		BadgePlate->SetBrushColor(FLinearColor(.025f, .35f, .58f, .96f));
		BadgePlate->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UTextBlock* Badge = FindOrCreate<UTextBlock>(Blueprint, TEXT("AllyBadgeText"));
		PlaceCanvas(Panel, Badge, FVector2D(180.f, 46.f), FVector2D(78.f, 40.f), 7);
		Badge->SetText(NSLOCTEXT("CombatHUD", "AllyBadge", "아군"));
		SetReadableFont(Badge, BaseFont, 22);

		UTextBlock* Name = FindOrCreate<UTextBlock>(Blueprint, TEXT("AllyName"));
		PlaceCanvas(Panel, Name, FVector2D(274.f, 37.f), FVector2D(280.f, 62.f), 7);
		Name->SetText(NSLOCTEXT("CombatHUD", "AllyNamePreview", "기사"));
		SetReadableFont(Name, BaseFont, 38);
		Name->SetJustification(ETextJustify::Left);

		UBorder* HPBack = FindOrCreate<UBorder>(Blueprint, TEXT("AllyHPBack"));
		PlaceCanvas(Panel, HPBack, FVector2D(178.f, 105.f), FVector2D(378.f, 58.f), 5);
		HPBack->SetBrushColor(FLinearColor(.035f, .09f, .035f, .96f));
		HPBack->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UProgressBar* HPBar = FindOrCreate<UProgressBar>(Blueprint, TEXT("AllyHPBar"));
		PlaceCanvas(Panel, HPBar, FVector2D(188.f, 115.f), FVector2D(358.f, 38.f), 6);
		HPBar->SetPercent(1.f);
		HPBar->SetFillColorAndOpacity(FLinearColor(.18f, .67f, .22f, 1.f));

		UTextBlock* HPText = FindOrCreate<UTextBlock>(Blueprint, TEXT("AllyHPText"));
		PlaceCanvas(Panel, HPText, FVector2D(188.f, 112.f), FVector2D(358.f, 44.f), 7);
		HPText->SetText(NSLOCTEXT("CombatHUD", "AllyHPPreview", "HP  100 / 100"));
		SetReadableFont(HPText, BaseFont, 27);

		UBorder* APPlate = FindOrCreate<UBorder>(Blueprint, TEXT("AllyAPPlate"));
		PlaceCanvas(Panel, APPlate, FVector2D(54.f, 184.f), FVector2D(230.f, 58.f), 5);
		APPlate->SetBrushColor(FLinearColor(.025f, .17f, .27f, .97f));
		APPlate->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UTextBlock* APText = FindOrCreate<UTextBlock>(Blueprint, TEXT("AllyAPText"));
		PlaceCanvas(Panel, APText, FVector2D(54.f, 188.f), FVector2D(230.f, 50.f), 6);
		APText->SetText(NSLOCTEXT("CombatHUD", "AllyAPPreview", "AP  10 / 10"));
		SetReadableFont(APText, BaseFont, 29);

		UBorder* SpeedPlate = FindOrCreate<UBorder>(Blueprint, TEXT("AllySpeedPlate"));
		PlaceCanvas(Panel, SpeedPlate, FVector2D(316.f, 184.f), FVector2D(230.f, 58.f), 5);
		SpeedPlate->SetBrushColor(FLinearColor(.21f, .105f, .035f, .97f));
		SpeedPlate->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UImage* SpeedIcon = FindOrCreate<UImage>(Blueprint, TEXT("AllySpeedIcon"));
		PlaceCanvas(Panel, SpeedIcon, FVector2D(330.f, 191.f), FVector2D(44.f, 44.f), 6);
		SpeedIcon->SetBrushFromTexture(SpeedTexture, false);
		SpeedIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UTextBlock* SpeedText = FindOrCreate<UTextBlock>(Blueprint, TEXT("AllySpeedText"));
		PlaceCanvas(Panel, SpeedText, FVector2D(376.f, 188.f), FVector2D(158.f, 50.f), 6);
		SpeedText->SetText(NSLOCTEXT("CombatHUD", "AllySpeedPreview", "속도  5"));
		SetReadableFont(SpeedText, BaseFont, 29);

		UTextBlock* StatusLabel = FindOrCreate<UTextBlock>(Blueprint, TEXT("AllyStatusLabel"));
		PlaceCanvas(Panel, StatusLabel, FVector2D(52.f, 267.f), FVector2D(94.f, 42.f), 6);
		StatusLabel->SetText(NSLOCTEXT("CombatHUD", "AllyStatusLabel", "상태"));
		SetReadableFont(StatusLabel, BaseFont, 26);

		for (int32 Index = 0; Index < 3; ++Index)
		{
			const float X = 150.f + 92.f * Index;
			UImage* Frame = FindOrCreate<UImage>(Blueprint,
				FName(*FString::Printf(TEXT("AllyStatusFrame_%d"), Index)));
			PlaceCanvas(Panel, Frame, FVector2D(X, 252.f), FVector2D(76.f, 76.f), 6);
			Frame->SetBrushFromTexture(StatusFrameTexture, false);
			Frame->SetVisibility(ESlateVisibility::Collapsed);

			UImage* Icon = FindOrCreate<UImage>(Blueprint,
				FName(*FString::Printf(TEXT("AllyStatusIcon_%d"), Index)));
			PlaceCanvas(Panel, Icon, FVector2D(X + 8.f, 260.f), FVector2D(60.f, 60.f), 7);
			Icon->SetVisibility(ESlateVisibility::Collapsed);

			UTextBlock* Count = FindOrCreate<UTextBlock>(Blueprint,
				FName(*FString::Printf(TEXT("AllyStatusCount_%d"), Index)));
			PlaceCanvas(Panel, Count, FVector2D(X + 43.f, 293.f), FVector2D(30.f, 30.f), 8);
			Count->SetText(FText::AsNumber(2));
			SetReadableFont(Count, BaseFont, 20);
			Count->SetVisibility(ESlateVisibility::Collapsed);
		}

		UTextBlock* StatusText = FindOrCreate<UTextBlock>(Blueprint, TEXT("AllyStatus"));
		PlaceCanvas(Panel, StatusText, FVector2D(438.f, 263.f), FVector2D(118.f, 52.f), 7);
		StatusText->SetText(NSLOCTEXT("CombatHUD", "AllyNoStatus", "상태 없음"));
		SetReadableFont(StatusText, BaseFont, 22);

		UTextBlock* Hint = FindOrCreate<UTextBlock>(Blueprint, TEXT("AllySummaryHint"));
		PlaceCanvas(Panel, Hint, FVector2D(64.f, 356.f), FVector2D(472.f, 46.f), 7);
		Hint->SetText(NSLOCTEXT("CombatHUD", "AllySummaryHint", "선택한 용병 요약"));
		SetReadableFont(Hint, BaseFont, 25);
	}

	void BuildMercenaryPanel(UWidgetBlueprint* Blueprint, const FSlateFontInfo& BaseFont,
		UTexture2D* ShellTexture, UTexture2D* NormalCardTexture,
		UTexture2D* SelectedCardTexture, UTexture2D* BackButtonTexture,
		UTexture2D* SkillFrameTexture)
	{
		UCanvasPanel* Root = CastChecked<UCanvasPanel>(Blueprint->WidgetTree->RootWidget);
		UCanvasPanel* Panel = FindOrCreate<UCanvasPanel>(Blueprint, TEXT("MercenaryPanel"));
		PlaceCanvas(Root, Panel, FVector2D::ZeroVector, FVector2D(1920.f, 1080.f), 10000);
		if (UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(Panel->Slot))
		{
			// The old Python pass left this as a 1920x1080 rectangle attached to the
			// top-left corner.  On a small/wide mobile viewport the rectangle was
			// clipped and the live combat HUD leaked through on the right.  The modal
			// shell owns the whole viewport; only its authored contents are aspect-fit.
			PanelSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			PanelSlot->SetOffsets(FMargin(0.f));
		}
		Panel->SetVisibility(ESlateVisibility::Collapsed);

		UImage* Shell = FindOrCreate<UImage>(Blueprint, TEXT("RuntimeMercenaryRosterShell"));
		PlaceCanvas(Panel, Shell, FVector2D::ZeroVector, FVector2D(1920.f, 1080.f), -100);
		if (UCanvasPanelSlot* ShellSlot = Cast<UCanvasPanelSlot>(Shell->Slot))
		{
			ShellSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			ShellSlot->SetOffsets(FMargin(0.f));
		}
		Shell->SetBrushFromTexture(ShellTexture, false);
		Shell->SetVisibility(ESlateVisibility::HitTestInvisible);

		UBorder* Scrim = FindOrCreate<UBorder>(Blueprint, TEXT("MercenaryScrim"));
		PlaceCanvas(Panel, Scrim, FVector2D::ZeroVector, FVector2D(1920.f, 1080.f), -90);
		if (UCanvasPanelSlot* ScrimSlot = Cast<UCanvasPanelSlot>(Scrim->Slot))
		{
			ScrimSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			ScrimSlot->SetOffsets(FMargin(0.f));
		}
		Scrim->SetVisibility(ESlateVisibility::Collapsed);

		UScaleBox* BoardScale = FindOrCreate<UScaleBox>(Blueprint,
			TEXT("MercenaryBoardScale"));
		PlaceCanvas(Panel, BoardScale, FVector2D::ZeroVector,
			FVector2D(1920.f, 1080.f), 1);
		if (UCanvasPanelSlot* ScaleSlot = Cast<UCanvasPanelSlot>(BoardScale->Slot))
		{
			ScaleSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			ScaleSlot->SetOffsets(FMargin(0.f));
		}
		BoardScale->SetStretch(EStretch::ScaleToFit);
		BoardScale->SetStretchDirection(EStretchDirection::Both);

		USizeBox* BoardSize = FindOrCreate<USizeBox>(Blueprint,
			TEXT("MercenaryBoardDesignSize"));
		BoardSize->SetWidthOverride(1920.f);
		BoardSize->SetHeightOverride(1080.f);
		EnsureParent(BoardScale, BoardSize);

		UCanvasPanel* Board = FindOrCreate<UCanvasPanel>(Blueprint, TEXT("MercenaryBoard"));
		EnsureParent(BoardSize, Board);

		for (const TCHAR* LegacyName : {
			TEXT("MercenaryHeaderPlate"), TEXT("MercenaryBoardPlate"),
			TEXT("MercenaryBoardShadow"), TEXT("MercenaryBoardInner"),
			TEXT("MercenaryClosePlate") })
		{
			UWidget* Legacy = Blueprint->WidgetTree->FindWidget(FName(LegacyName));
			if (Legacy == nullptr)
			{
				Legacy = Blueprint->WidgetTree->ConstructWidget<UImage>(
					UImage::StaticClass(), FName(LegacyName));
				Blueprint->OnVariableAdded(FName(LegacyName));
			}
			PlaceCanvas(Board, Legacy, FVector2D::ZeroVector, FVector2D(1.f, 1.f), -50);
			Legacy->SetVisibility(ESlateVisibility::Collapsed);
		}

		UTextBlock* Title = FindOrCreate<UTextBlock>(Blueprint, TEXT("MercenaryTitleText"));
		PlaceCanvas(Board, Title, FVector2D(689.f, 45.f), FVector2D(542.f, 102.f), 4);
		Title->SetText(NSLOCTEXT("CombatHUD", "MercenaryTabTitle", "용병"));
		SetReadableFont(Title, BaseFont, 54);

		UTextBlock* Subtitle = FindOrCreate<UTextBlock>(Blueprint,
			TEXT("MercenarySubtitleText"));
		PlaceCanvas(Board, Subtitle, FVector2D::ZeroVector, FVector2D(1.f, 1.f), 4);
		Subtitle->SetVisibility(ESlateVisibility::Collapsed);

		UTextBlock* GoldLabel = FindOrCreate<UTextBlock>(Blueprint,
			TEXT("MercenaryGoldLabel"));
		PlaceCanvas(Board, GoldLabel, FVector2D(48.f, 55.f), FVector2D(116.f, 68.f), 4);
		GoldLabel->SetText(NSLOCTEXT("CombatHUD", "MercenaryGoldLabel", "골드"));
		SetReadableFont(GoldLabel, BaseFont, 28);

		UTextBlock* GoldText = FindOrCreate<UTextBlock>(Blueprint,
			TEXT("MercenaryGoldText"));
		PlaceCanvas(Board, GoldText, FVector2D(164.f, 50.f), FVector2D(180.f, 78.f), 4);
		GoldText->SetText(FText::AsNumber(0));
		SetReadableFont(GoldText, BaseFont, 42);

		UImage* BackArt = FindOrCreate<UImage>(Blueprint, TEXT("MercenaryBackArt"));
		PlaceCanvas(Board, BackArt, FVector2D(1600.f, 36.f), FVector2D(270.f, 112.f), 5);
		BackArt->SetBrushFromTexture(BackButtonTexture, false);
		BackArt->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UTextBlock* CloseText = FindOrCreate<UTextBlock>(Blueprint,
			TEXT("MercenaryCloseText"));
		PlaceCanvas(Board, CloseText, FVector2D(1600.f, 50.f), FVector2D(270.f, 82.f), 6);
		CloseText->SetText(NSLOCTEXT("CombatHUD", "MercenaryBack", "뒤로"));
		SetReadableFont(CloseText, BaseFont, 38);

		UButton* CloseButton = FindOrCreate<UButton>(Blueprint,
			TEXT("MercenaryCloseButton"));
		PlaceCanvas(Board, CloseButton, FVector2D(1600.f, 36.f), FVector2D(270.f, 112.f), 7);
		SetInvisibleButtonChrome(CloseButton);

		const FVector2D LocalCardSize(350.f, 190.f);
		const FVector2D CardPositions[] = {
			FVector2D(18.f, 205.f), FVector2D(18.f, 470.f), FVector2D(18.f, 735.f)
		};
		for (int32 Index = 0; Index < 3; ++Index)
		{
			const FString Suffix = FString::Printf(TEXT("_%d"), Index);
			UScaleBox* Scale = FindOrCreate<UScaleBox>(Blueprint,
				FName(*FString::Printf(TEXT("MercenaryCardScale_%d"), Index)));
			PlaceCanvas(Board, Scale, CardPositions[Index], FVector2D(432.f, 235.f), 3);
			Scale->SetStretch(EStretch::ScaleToFit);
			Scale->SetStretchDirection(EStretchDirection::Both);

			UCanvasPanel* Card = FindOrCreate<UCanvasPanel>(Blueprint,
				FName(TEXT("PartyCard") + Suffix));
			EnsureParent(Scale, Card);
			Card->SetClipping(EWidgetClipping::ClipToBoundsAlways);

			UImage* Plate = FindOrCreate<UImage>(Blueprint,
				FName(TEXT("PartyPlate") + Suffix));
			PlaceCanvas(Card, Plate, FVector2D::ZeroVector, LocalCardSize, 1);
			Plate->SetBrushFromTexture(Index == 0
				? SelectedCardTexture : NormalCardTexture, false);
			Plate->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			UCanvasPanel* Content = FindOrCreate<UCanvasPanel>(Blueprint,
				FName(TEXT("PartyContent") + Suffix));
			PlaceCanvas(Card, Content, FVector2D::ZeroVector, LocalCardSize, 2);

			UImage* Portrait = FindOrCreate<UImage>(Blueprint,
				FName(TEXT("PartyPortrait") + Suffix));
			// Marchbound list portraits are authored square.  Keeping the slot square
			// avoids the stretched faces produced by the old 112x150 rectangle.
			PlaceCanvas(Content, Portrait, FVector2D(21.f, 24.f), FVector2D(142.f, 142.f), 10);

			UTextBlock* Name = FindOrCreate<UTextBlock>(Blueprint,
				FName(TEXT("PartyName") + Suffix));
			PlaceCanvas(Content, Name, FVector2D(171.f, 22.f), FVector2D(153.f, 44.f), 15);
			Name->SetText(FText::FromString(FString::Printf(TEXT("용병 %d"), Index + 1)));
			SetReadableFont(Name, BaseFont, 27);

			UProgressBar* HPBar = FindOrCreate<UProgressBar>(Blueprint,
				FName(TEXT("PartyHPBar") + Suffix));
			PlaceCanvas(Content, HPBar, FVector2D(171.f, 76.f), FVector2D(153.f, 32.f), 10);
			HPBar->SetPercent(1.f);

			UTextBlock* HPText = FindOrCreate<UTextBlock>(Blueprint,
				FName(TEXT("PartyHPText") + Suffix));
			PlaceCanvas(Content, HPText, FVector2D(171.f, 75.f), FVector2D(153.f, 34.f), 15);
			HPText->SetText(FText::FromString(TEXT("100/100")));
			SetReadableFont(HPText, BaseFont, 20);

			UImage* APPlate = FindOrCreate<UImage>(Blueprint,
				FName(TEXT("PartyAPPlate") + Suffix));
			PlaceCanvas(Content, APPlate, FVector2D(171.f, 118.f), FVector2D(153.f, 34.f), 10);
			APPlate->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			UTextBlock* APText = FindOrCreate<UTextBlock>(Blueprint,
				FName(TEXT("PartyAPText") + Suffix));
			PlaceCanvas(Content, APText, FVector2D(171.f, 118.f), FVector2D(153.f, 34.f), 15);
			APText->SetText(FText::FromString(TEXT("AP 10/10")));
			SetReadableFont(APText, BaseFont, 21);

			UTextBlock* StatusText = FindOrCreate<UTextBlock>(Blueprint,
				FName(TEXT("PartyStatus") + Suffix));
			PlaceCanvas(Content, StatusText, FVector2D(171.f, 153.f), FVector2D(108.f, 30.f), 15);
			SetReadableFont(StatusText, BaseFont, 17);
			StatusText->SetJustification(ETextJustify::Left);
			StatusText->SetVisibility(ESlateVisibility::Collapsed);
			UImage* StatusIcon = FindOrCreate<UImage>(Blueprint,
				FName(TEXT("PartyStatusIcon") + Suffix));
			PlaceCanvas(Content, StatusIcon, FVector2D(291.f, 151.f), FVector2D(32.f, 32.f), 16);
			StatusIcon->SetVisibility(ESlateVisibility::Collapsed);

			for (int32 StatusIndex = 0; StatusIndex < 3; ++StatusIndex)
			{
				const float X = 288.f - 42.f * StatusIndex;
				UImage* Frame = FindOrCreate<UImage>(Blueprint, FName(*FString::Printf(
					TEXT("PartyStatusFrame_%d_%d"), Index, StatusIndex)));
				PlaceCanvas(Content, Frame, FVector2D(X, 146.f), FVector2D(38.f, 38.f), 18);
				Frame->SetVisibility(ESlateVisibility::Collapsed);
				UImage* Icon = FindOrCreate<UImage>(Blueprint, FName(*FString::Printf(
					TEXT("PartyStatusIcon_%d_%d"), Index, StatusIndex)));
				PlaceCanvas(Content, Icon, FVector2D(X + 4.f, 150.f), FVector2D(30.f, 30.f), 19);
				Icon->SetVisibility(ESlateVisibility::Collapsed);
			}

			UButton* Button = FindOrCreate<UButton>(Blueprint,
				FName(TEXT("PartyButton") + Suffix));
			PlaceCanvas(Card, Button, FVector2D::ZeroVector, LocalCardSize, 29);
			SetInvisibleButtonChrome(Button);
		}

		UImage* Hero = FindOrCreate<UImage>(Blueprint, TEXT("MercenaryHeroPortrait"));
		// The new hero illustrations are 1:1.  Show them as a large square rather
		// than turning a still image into a fake, stretched 3D standing model.
		PlaceCanvas(Board, Hero, FVector2D(482.f, 248.f), FVector2D(548.f, 548.f), 5);
		Hero->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		if (UTexture2D* PreviewHero = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireHero_Knight.T_MB_HireHero_Knight")))
		{
			Hero->SetBrushFromTexture(PreviewHero, false);
		}

		struct FDetailText
		{
			const TCHAR* Name;
			FVector2D Position;
			FVector2D Size;
			const TCHAR* Preview;
			int32 FontSize;
		};
		const FDetailText Details[] = {
			{ TEXT("MercenaryDetailName"), FVector2D(1110.f, 235.f), FVector2D(700.f, 88.f), TEXT("용병"), 52 },
			{ TEXT("MercenaryDetailHP"), FVector2D(1110.f, 365.f), FVector2D(620.f, 68.f), TEXT("HP  100 / 100"), 38 },
			{ TEXT("MercenaryDetailAP"), FVector2D(1110.f, 465.f), FVector2D(620.f, 68.f), TEXT("AP  10 / 10"), 38 },
			{ TEXT("MercenaryDetailSpeed"), FVector2D(1110.f, 565.f), FVector2D(620.f, 68.f), TEXT("속도  5"), 38 },
		};
		for (const FDetailText& Detail : Details)
		{
			UTextBlock* Text = FindOrCreate<UTextBlock>(Blueprint, FName(Detail.Name));
			PlaceCanvas(Board, Text, Detail.Position, Detail.Size, 8);
			Text->SetText(FText::FromString(Detail.Preview));
			SetReadableFont(Text, BaseFont, Detail.FontSize);
			Text->SetJustification(ETextJustify::Left);
		}

		UTextBlock* SkillHeading = FindOrCreate<UTextBlock>(Blueprint,
			TEXT("MercenarySkillHeading"));
		PlaceCanvas(Board, SkillHeading, FVector2D(1110.f, 642.f),
			FVector2D(620.f, 52.f), 8);
		SkillHeading->SetText(NSLOCTEXT("CombatHUD", "MercenarySkills", "스킬"));
		SetReadableFont(SkillHeading, BaseFont, 32);
		SkillHeading->SetJustification(ETextJustify::Left);

		for (int32 Index = 0; Index < 6; ++Index)
		{
			const float X = 1110.f + 205.f * (Index % 3);
			const float Y = 690.f + 160.f * (Index / 3);
			UImage* Frame = FindOrCreate<UImage>(Blueprint, FName(*FString::Printf(
				TEXT("MercenarySkillFrame_%d"), Index)));
			PlaceCanvas(Board, Frame, FVector2D(X, Y), FVector2D(156.f, 156.f), 8);
			Frame->SetBrushFromTexture(SkillFrameTexture, false);
			Frame->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			UImage* Icon = FindOrCreate<UImage>(Blueprint, FName(*FString::Printf(
				TEXT("MercenarySkillIcon_%d"), Index)));
			PlaceCanvas(Board, Icon, FVector2D(X + 33.f, Y + 27.f),
				FVector2D(90.f, 90.f), 9);
			Icon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			UTextBlock* Name = FindOrCreate<UTextBlock>(Blueprint, FName(*FString::Printf(
				TEXT("MercenarySkillName_%d"), Index)));
			PlaceCanvas(Board, Name, FVector2D(X + 8.f, Y + 108.f),
				FVector2D(140.f, 38.f), 10);
			Name->SetText(Index == 0
				? NSLOCTEXT("CombatHUD", "MercenaryMovePreview", "이동")
				: FText::FromString(FString::Printf(TEXT("스킬 %d"), Index)));
			SetReadableFont(Name, BaseFont, 22);

			UTextBlock* Cost = FindOrCreate<UTextBlock>(Blueprint, FName(*FString::Printf(
				TEXT("MercenarySkillCost_%d"), Index)));
			PlaceCanvas(Board, Cost, FVector2D(X + 112.f, Y + 8.f),
				FVector2D(38.f, 38.f), 11);
			Cost->SetText(FText::AsNumber(Index == 0 ? 1 : 0));
			SetReadableFont(Cost, BaseFont, 20);
		}
	}

	void Build()
	{
		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, AssetPath);
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("RD_COMBAT_HUD_BUILD missing asset %s"), AssetPath);
			return;
		}

		UCanvasPanel* Objective = CastChecked<UCanvasPanel>(
			Blueprint->WidgetTree->FindWidget(TEXT("ObjectivePanel")));
		UTextBlock* RoundText = CastChecked<UTextBlock>(
			Blueprint->WidgetTree->FindWidget(TEXT("RoundText")));
		const FSlateFontInfo BaseFont = RoundText->GetFont();

		UTexture2D* MercenaryTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_OptionsIcon_MercenaryGlyph.T_MB_OptionsIcon_MercenaryGlyph"));
		UTexture2D* MonsterTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_OptionsIcon_MonsterGlyph.T_MB_OptionsIcon_MonsterGlyph"));
		UTexture2D* SpeedTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_Icon_Speed.T_MB_Icon_Speed"));
		UTexture2D* TurnTokenFrameTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_TurnToken_Frame.T_MB_TurnToken_Frame"));
		UTexture2D* OptionsRailFrameTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_OptionsRail_Frame.T_MB_OptionsRail_Frame"));
		UTexture2D* MapTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_OptionsIcon_Map.T_MB_OptionsIcon_Map"));
		UTexture2D* SettingsTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_OptionsIcon_Settings.T_MB_OptionsIcon_Settings"));
		UTexture2D* ArtifactSlotTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_ArtifactSlot_Frame.T_MB_ArtifactSlot_Frame"));
		UTexture2D* RoundBadgeTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_RoundBadge_Frame.T_MB_RoundBadge_Frame"));
		UTexture2D* MercenaryShellTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MercenaryRoster_Shell.T_MercenaryRoster_Shell"));
		UTexture2D* MercenaryCardNormalTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_MercenaryCard_Normal.T_MB_MercenaryCard_Normal"));
		UTexture2D* MercenaryCardSelectedTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_MercenaryCard_Selected.T_MB_MercenaryCard_Selected"));
		UTexture2D* BackButtonTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Hire/T_MB_HireBackButton.T_MB_HireBackButton"));
		UTexture2D* MercenarySkillFrameTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Hire/T_MB_HireSkillButtonFrame.T_MB_HireSkillButtonFrame"));
		UTexture2D* EnemyPanelTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Common/T_MB_GenericDetailPanel.T_MB_GenericDetailPanel"));
		UTexture2D* StatusSlotTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_StatusSlot_Frame.T_MB_StatusSlot_Frame"));
		if (MercenaryTexture == nullptr || MonsterTexture == nullptr
			|| SpeedTexture == nullptr || TurnTokenFrameTexture == nullptr
			|| OptionsRailFrameTexture == nullptr || MapTexture == nullptr
			|| SettingsTexture == nullptr || ArtifactSlotTexture == nullptr
			|| RoundBadgeTexture == nullptr || MercenaryShellTexture == nullptr
			|| MercenaryCardNormalTexture == nullptr
			|| MercenaryCardSelectedTexture == nullptr || BackButtonTexture == nullptr
			|| MercenarySkillFrameTexture == nullptr
			|| EnemyPanelTexture == nullptr || StatusSlotTexture == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("RD_COMBAT_HUD_BUILD HUD textures missing"));
			return;
		}

		if (UWidget* ObjectivePlate = Blueprint->WidgetTree->FindWidget(TEXT("ObjectivePlate")))
		{
			ObjectivePlate->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (UWidget* TurnPlate = Blueprint->WidgetTree->FindWidget(TEXT("TurnPlate")))
		{
			TurnPlate->SetVisibility(ESlateVisibility::Collapsed);
		}

		BuildMercenaryPanel(Blueprint, BaseFont, MercenaryShellTexture,
			MercenaryCardNormalTexture, MercenaryCardSelectedTexture, BackButtonTexture,
			MercenarySkillFrameTexture);
		BuildEnemySummary(Blueprint, BaseFont, EnemyPanelTexture,
			ArtifactSlotTexture, StatusSlotTexture, SpeedTexture);
		BuildAllySummary(Blueprint, BaseFont, EnemyPanelTexture,
			ArtifactSlotTexture, StatusSlotTexture, SpeedTexture);
		UCanvasPanel* TurnPanel = CastChecked<UCanvasPanel>(
			Blueprint->WidgetTree->FindWidget(TEXT("TurnPanel")));
		if (UCanvasPanelSlot* TurnPanelSlot = Cast<UCanvasPanelSlot>(TurnPanel->Slot))
		{
			TurnPanelSlot->SetPosition(FVector2D(-580.f, 8.f));
			TurnPanelSlot->SetSize(FVector2D(1090.f, 174.f));
		}

		UImage* OptionsRailFrame = FindOrCreate<UImage>(Blueprint, TEXT("OptionsRailFrame"));
		PlaceCanvas(Objective, OptionsRailFrame, FVector2D::ZeroVector,
			FVector2D(470.f, 173.f), 1);
		OptionsRailFrame->SetBrushFromTexture(OptionsRailFrameTexture, false);
		OptionsRailFrame->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UImage* MapIcon = FindOrCreate<UImage>(Blueprint, TEXT("MenuMapIcon"));
		PlaceCanvas(Objective, MapIcon, FVector2D(47.f, 47.f), FVector2D(74.f, 74.f), 31);
		MapIcon->SetBrushFromTexture(MapTexture, false);
		MapIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UImage* MercenaryIcon = FindOrCreate<UImage>(Blueprint, TEXT("MenuMercenaryIcon"));
		// 세로형 원본의 종횡비를 유지해 헬멧이 찌그러지지 않게 한다.
		PlaceCanvas(Objective, MercenaryIcon, FVector2D(153.5f, 38.f), FVector2D(63.f, 96.f), 31);
		MercenaryIcon->SetBrushFromTexture(MercenaryTexture, false);
		MercenaryIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UImage* MonsterIcon = FindOrCreate<UImage>(Blueprint, TEXT("MenuMonsterIcon"));
		PlaceCanvas(Objective, MonsterIcon, FVector2D(244.f, 38.f), FVector2D(90.f, 96.f), 31);
		MonsterIcon->SetBrushFromTexture(MonsterTexture, false);
		MonsterIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UImage* SettingsIcon = FindOrCreate<UImage>(Blueprint, TEXT("MenuSettingsIcon"));
		PlaceCanvas(Objective, SettingsIcon, FVector2D(350.f, 47.f), FVector2D(74.f, 74.f), 31);
		SettingsIcon->SetBrushFromTexture(SettingsTexture, false);
		SettingsIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UTextBlock* MercenaryLabel = CastChecked<UTextBlock>(
			Blueprint->WidgetTree->FindWidget(TEXT("MenuMercenaryMaskLabel")));
		MercenaryLabel->SetVisibility(ESlateVisibility::Collapsed);

		UTextBlock* MonsterLabel = CastChecked<UTextBlock>(
			Blueprint->WidgetTree->FindWidget(TEXT("MenuEmptyMaskLabel")));
		MonsterLabel->SetVisibility(ESlateVisibility::Collapsed);
		if (UWidget* MercenaryMask = Blueprint->WidgetTree->FindWidget(TEXT("MenuMercenaryMask")))
		{
			MercenaryMask->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (UWidget* EmptyMask = Blueprint->WidgetTree->FindWidget(TEXT("MenuEmptyMask")))
		{
			EmptyMask->SetVisibility(ESlateVisibility::Collapsed);
		}

		if (UButton* MonsterButton = Cast<UButton>(
			Blueprint->WidgetTree->FindWidget(TEXT("MenuButton_2"))))
		{
			MonsterButton->SetIsEnabled(true);
		}
		const FVector2D MenuButtonPositions[] = {
			FVector2D(37.f, 31.f), FVector2D(138.f, 31.f),
			FVector2D(242.f, 31.f), FVector2D(343.f, 31.f),
		};
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(MenuButtonPositions); ++Index)
		{
			if (UButton* MenuButton = Cast<UButton>(Blueprint->WidgetTree->FindWidget(
				FName(*FString::Printf(TEXT("MenuButton_%d"), Index)))))
			{
				PlaceCanvas(Objective, MenuButton, MenuButtonPositions[Index],
					FVector2D(94.f, 112.f), 40);
			}
		}

		UCanvasPanel* ArtifactStrip = CastChecked<UCanvasPanel>(
			Blueprint->WidgetTree->FindWidget(TEXT("ArtifactStrip")));
		if (UCanvasPanelSlot* ArtifactSlot = Cast<UCanvasPanelSlot>(ArtifactStrip->Slot))
		{
			// 모바일에서는 76px 원본 슬롯이 최종 화면에서 20px 안팎까지 줄어든다.
			// 개별 원형 프레임과 아이콘을 1.5배 이상 키우고 AP바 위 여백도 같이 확보한다.
			ArtifactSlot->SetPosition(FVector2D(18.f, -286.f));
			ArtifactSlot->SetSize(FVector2D(680.f, 116.f));
		}
		UImage* ArtifactTrayFrame = FindOrCreate<UImage>(Blueprint, TEXT("ArtifactTrayFrame"));
		ArtifactTrayFrame->SetVisibility(ESlateVisibility::Collapsed);
		for (int32 Index = 0; Index < 6; ++Index)
		{
			if (UImage* ArtifactFrame = Cast<UImage>(Blueprint->WidgetTree->FindWidget(
				FName(*FString::Printf(TEXT("ArtifactFrame_%d"), Index)))))
			{
				PlaceCanvas(ArtifactStrip, ArtifactFrame,
					FVector2D(4.f + 112.f * Index, 4.f), FVector2D(108.f, 108.f), 1);
				ArtifactFrame->SetBrushFromTexture(ArtifactSlotTexture, false);
				ArtifactFrame->SetVisibility(ESlateVisibility::Collapsed);
			}
			if (UImage* ArtifactIcon = Cast<UImage>(Blueprint->WidgetTree->FindWidget(
				FName(*FString::Printf(TEXT("ArtifactIcon_%d"), Index)))))
			{
				PlaceCanvas(ArtifactStrip, ArtifactIcon,
					FVector2D(13.f + 112.f * Index, 13.f), FVector2D(90.f, 90.f), 2);
			}
		}

		for (int32 Index = 0; Index < 10; ++Index)
		{
			UCanvasPanel* Token = CastChecked<UCanvasPanel>(Blueprint->WidgetTree->FindWidget(
				FName(*FString::Printf(TEXT("TurnToken_%d"), Index))));
			PlaceCanvas(TurnPanel, Token, FVector2D(5.f + 109.f * Index, 30.f),
				FVector2D(108.f, 144.f), 10);
			Token->SetClipping(EWidgetClipping::ClipToBoundsAlways);

			UImage* TurnFrame = FindOrCreate<UImage>(Blueprint,
				FName(*FString::Printf(TEXT("TurnFrame_%d"), Index)));
			PlaceCanvas(Token, TurnFrame, FVector2D::ZeroVector, FVector2D(108.f, 144.f), 5);
			TurnFrame->SetBrushFromTexture(TurnTokenFrameTexture, false);
			TurnFrame->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			UScaleBox* Crop = CastChecked<UScaleBox>(Blueprint->WidgetTree->FindWidget(
				FName(*FString::Printf(TEXT("TurnPortraitCrop_%d"), Index))));
			PlaceCanvas(Token, Crop, FVector2D(14.f, 17.f), FVector2D(80.f, 86.f), 10);
			Crop->SetStretch(EStretch::ScaleToFill);
			Crop->SetClipping(EWidgetClipping::ClipToBoundsAlways);

			UImage* Current = CastChecked<UImage>(Blueprint->WidgetTree->FindWidget(
				FName(*FString::Printf(TEXT("TurnCurrent_%d"), Index))));
			PlaceCanvas(Token, Current, FVector2D(9.f, 11.f), FVector2D(90.f, 97.f), 40);

			UBorder* SpeedPlate = FindOrCreate<UBorder>(Blueprint,
				FName(*FString::Printf(TEXT("TurnSpeedPlate_%d"), Index)));
			SpeedPlate->SetVisibility(ESlateVisibility::Collapsed);

			UImage* SpeedIcon = FindOrCreate<UImage>(Blueprint,
				FName(*FString::Printf(TEXT("TurnSpeedIcon_%d"), Index)));
			PlaceCanvas(Token, SpeedIcon, FVector2D(13.f, 103.f), FVector2D(36.f, 36.f), 21);
			SpeedIcon->SetBrushFromTexture(SpeedTexture, false);
			SpeedIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			UTextBlock* Speed = FindOrCreate<UTextBlock>(Blueprint,
				FName(*FString::Printf(TEXT("TurnSpeed_%d"), Index)));
			PlaceCanvas(Token, Speed, FVector2D(52.f, 103.f), FVector2D(42.f, 34.f), 22);
			Speed->SetText(NSLOCTEXT("CombatHUD", "SpeedPreview", "0"));
			SetReadableFont(Speed, BaseFont, 22);

			UBorder* RoundBadge = CastChecked<UBorder>(Blueprint->WidgetTree->FindWidget(
				FName(*FString::Printf(TEXT("TurnRoundDivider_%d"), Index))));
			PlaceCanvas(TurnPanel, RoundBadge, FVector2D(5.f + 109.f * Index, 0.f),
				FVector2D(108.f, 34.f), 30);
			FSlateBrush RoundBadgeBrush;
			RoundBadgeBrush.SetResourceObject(RoundBadgeTexture);
			RoundBadgeBrush.ImageSize = FVector2D(108.f, 34.f);
			RoundBadge->SetBrush(RoundBadgeBrush);
			RoundBadge->SetBrushColor(FLinearColor::White);
			RoundBadge->SetVisibility(ESlateVisibility::Collapsed);

			UTextBlock* RoundLabel = CastChecked<UTextBlock>(Blueprint->WidgetTree->FindWidget(
				FName(*FString::Printf(TEXT("TurnRoundLabel_%d"), Index))));
			PlaceCanvas(TurnPanel, RoundLabel, FVector2D(5.f + 109.f * Index, 2.f),
				FVector2D(108.f, 30.f), 31);
			SetReadableFont(RoundLabel, BaseFont, 19);
			RoundLabel->SetVisibility(ESlateVisibility::Collapsed);
		}

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		if (!UPackage::SavePackage(Blueprint->GetPackage(), Blueprint,
			*FPackageName::LongPackageNameToFilename(
				Blueprint->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension()),
			FSavePackageArgs()))
		{
			UE_LOG(LogTemp, Error, TEXT("RD_COMBAT_HUD_BUILD save failed"));
			return;
		}
		UE_LOG(LogTemp, Display,
			TEXT("RD_COMBAT_HUD_BUILD success menu_icons=4 turn_frames=10 speed_icons=10"));
	}
}

void RegisterCombatHUDWidgetBuilderCommands()
{
	using namespace CombatHUDWidgetBuilder;
	BuildCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.BuildCombatHUDAdditions"),
		TEXT("Add Marchbound combat tab icons and turn speed widgets."),
		FConsoleCommandDelegate::CreateStatic(&Build));
}

void UnregisterCombatHUDWidgetBuilderCommands()
{
	CombatHUDWidgetBuilder::BuildCommand.Reset();
}
