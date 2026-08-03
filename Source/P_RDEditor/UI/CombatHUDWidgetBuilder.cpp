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

	void BuildMercenaryPanel(UWidgetBlueprint* Blueprint, const FSlateFontInfo& BaseFont,
		UTexture2D* ShellTexture, UTexture2D* NormalCardTexture,
		UTexture2D* SelectedCardTexture)
	{
		UCanvasPanel* Root = CastChecked<UCanvasPanel>(Blueprint->WidgetTree->RootWidget);
		UCanvasPanel* Panel = FindOrCreate<UCanvasPanel>(Blueprint, TEXT("MercenaryPanel"));
		PlaceCanvas(Root, Panel, FVector2D::ZeroVector, FVector2D(1920.f, 1080.f), 10000);
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
		Scrim->SetVisibility(ESlateVisibility::Collapsed);

		UCanvasPanel* Board = FindOrCreate<UCanvasPanel>(Blueprint, TEXT("MercenaryBoard"));
		PlaceCanvas(Panel, Board, FVector2D::ZeroVector, FVector2D(1920.f, 1080.f), 1);

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
		PlaceCanvas(Board, Title, FVector2D(689.f, 55.f), FVector2D(542.f, 87.f), 4);
		Title->SetText(NSLOCTEXT("CombatHUD", "MercenaryTabTitle", "용병"));
		SetReadableFont(Title, BaseFont, 46);

		UTextBlock* Subtitle = FindOrCreate<UTextBlock>(Blueprint,
			TEXT("MercenarySubtitleText"));
		PlaceCanvas(Board, Subtitle, FVector2D::ZeroVector, FVector2D(1.f, 1.f), 4);
		Subtitle->SetVisibility(ESlateVisibility::Collapsed);

		UTextBlock* GoldLabel = FindOrCreate<UTextBlock>(Blueprint,
			TEXT("MercenaryGoldLabel"));
		PlaceCanvas(Board, GoldLabel, FVector2D(41.f, 57.f), FVector2D(103.f, 60.f), 4);
		GoldLabel->SetText(NSLOCTEXT("CombatHUD", "MercenaryGoldLabel", "골드"));
		SetReadableFont(GoldLabel, BaseFont, 24);

		UTextBlock* GoldText = FindOrCreate<UTextBlock>(Blueprint,
			TEXT("MercenaryGoldText"));
		PlaceCanvas(Board, GoldText, FVector2D(147.f, 53.f), FVector2D(172.f, 69.f), 4);
		GoldText->SetText(FText::AsNumber(0));
		SetReadableFont(GoldText, BaseFont, 36);

		UTextBlock* CloseText = FindOrCreate<UTextBlock>(Blueprint,
			TEXT("MercenaryCloseText"));
		PlaceCanvas(Board, CloseText, FVector2D(1814.f, 55.f), FVector2D(67.f, 67.f), 5);
		CloseText->SetText(FText::FromString(TEXT("×")));
		SetReadableFont(CloseText, BaseFont, 42);

		UButton* CloseButton = FindOrCreate<UButton>(Blueprint,
			TEXT("MercenaryCloseButton"));
		PlaceCanvas(Board, CloseButton, FVector2D(1801.f, 41.f), FVector2D(94.f, 94.f), 6);
		SetInvisibleButtonChrome(CloseButton);

		const FVector2D LocalCardSize(350.f, 190.f);
		const FVector2D CardPositions[] = {
			FVector2D(28.f, 232.f), FVector2D(28.f, 491.f), FVector2D(28.f, 750.f)
		};
		for (int32 Index = 0; Index < 3; ++Index)
		{
			const FString Suffix = FString::Printf(TEXT("_%d"), Index);
			UScaleBox* Scale = FindOrCreate<UScaleBox>(Blueprint,
				FName(*FString::Printf(TEXT("MercenaryCardScale_%d"), Index)));
			PlaceCanvas(Board, Scale, CardPositions[Index], FVector2D(402.f, 218.f), 3);
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
			PlaceCanvas(Content, Portrait, FVector2D(24.f, 20.f), FVector2D(112.f, 150.f), 10);

			UTextBlock* Name = FindOrCreate<UTextBlock>(Blueprint,
				FName(TEXT("PartyName") + Suffix));
			PlaceCanvas(Content, Name, FVector2D(145.f, 24.f), FVector2D(176.f, 42.f), 15);
			Name->SetText(FText::FromString(FString::Printf(TEXT("용병 %d"), Index + 1)));
			SetReadableFont(Name, BaseFont, 24);

			UProgressBar* HPBar = FindOrCreate<UProgressBar>(Blueprint,
				FName(TEXT("PartyHPBar") + Suffix));
			PlaceCanvas(Content, HPBar, FVector2D(145.f, 77.f), FVector2D(176.f, 30.f), 10);
			HPBar->SetPercent(1.f);

			UTextBlock* HPText = FindOrCreate<UTextBlock>(Blueprint,
				FName(TEXT("PartyHPText") + Suffix));
			PlaceCanvas(Content, HPText, FVector2D(145.f, 76.f), FVector2D(176.f, 31.f), 15);
			HPText->SetText(FText::FromString(TEXT("100/100")));
			SetReadableFont(HPText, BaseFont, 18);

			UImage* APPlate = FindOrCreate<UImage>(Blueprint,
				FName(TEXT("PartyAPPlate") + Suffix));
			PlaceCanvas(Content, APPlate, FVector2D(145.f, 119.f), FVector2D(176.f, 32.f), 10);
			APPlate->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			UTextBlock* APText = FindOrCreate<UTextBlock>(Blueprint,
				FName(TEXT("PartyAPText") + Suffix));
			PlaceCanvas(Content, APText, FVector2D(145.f, 119.f), FVector2D(176.f, 32.f), 15);
			APText->SetText(FText::FromString(TEXT("AP 10/10")));
			SetReadableFont(APText, BaseFont, 19);

			UTextBlock* StatusText = FindOrCreate<UTextBlock>(Blueprint,
				FName(TEXT("PartyStatus") + Suffix));
			PlaceCanvas(Content, StatusText, FVector2D(145.f, 151.f), FVector2D(140.f, 30.f), 15);
			StatusText->SetVisibility(ESlateVisibility::Collapsed);
			UImage* StatusIcon = FindOrCreate<UImage>(Blueprint,
				FName(TEXT("PartyStatusIcon") + Suffix));
			PlaceCanvas(Content, StatusIcon, FVector2D(288.f, 151.f), FVector2D(30.f, 30.f), 16);
			StatusIcon->SetVisibility(ESlateVisibility::Collapsed);

			for (int32 StatusIndex = 0; StatusIndex < 3; ++StatusIndex)
			{
				const float X = 296.f - 45.f * StatusIndex;
				UImage* Frame = FindOrCreate<UImage>(Blueprint, FName(*FString::Printf(
					TEXT("PartyStatusFrame_%d_%d"), Index, StatusIndex)));
				PlaceCanvas(Content, Frame, FVector2D(X, 146.f), FVector2D(40.f, 40.f), 18);
				Frame->SetVisibility(ESlateVisibility::Collapsed);
				UImage* Icon = FindOrCreate<UImage>(Blueprint, FName(*FString::Printf(
					TEXT("PartyStatusIcon_%d_%d"), Index, StatusIndex)));
				PlaceCanvas(Content, Icon, FVector2D(X + 5.f, 151.f), FVector2D(30.f, 30.f), 19);
				Icon->SetVisibility(ESlateVisibility::Collapsed);
			}

			UButton* Button = FindOrCreate<UButton>(Blueprint,
				FName(TEXT("PartyButton") + Suffix));
			PlaceCanvas(Card, Button, FVector2D::ZeroVector, LocalCardSize, 29);
			SetInvisibleButtonChrome(Button);
		}

		UImage* Hero = FindOrCreate<UImage>(Blueprint, TEXT("MercenaryHeroPortrait"));
		PlaceCanvas(Board, Hero, FVector2D(480.f, 225.f), FVector2D(545.f, 735.f), 5);
		Hero->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		if (const UImage* FirstPortrait = Cast<UImage>(
			Blueprint->WidgetTree->FindWidget(TEXT("PartyPortrait_0"))))
		{
			if (UTexture2D* PreviewPortrait = Cast<UTexture2D>(
				FirstPortrait->GetBrush().GetResourceObject()))
			{
				Hero->SetBrushFromTexture(PreviewPortrait, false);
			}
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
			{ TEXT("MercenaryDetailName"), FVector2D(1110.f, 250.f), FVector2D(700.f, 76.f), TEXT("용병"), 44 },
			{ TEXT("MercenaryDetailHP"), FVector2D(1110.f, 370.f), FVector2D(540.f, 54.f), TEXT("HP  100 / 100"), 30 },
			{ TEXT("MercenaryDetailAP"), FVector2D(1110.f, 445.f), FVector2D(540.f, 54.f), TEXT("AP  10 / 10"), 30 },
			{ TEXT("MercenaryDetailSpeed"), FVector2D(1110.f, 520.f), FVector2D(540.f, 54.f), TEXT("속도  5"), 30 },
		};
		for (const FDetailText& Detail : Details)
		{
			UTextBlock* Text = FindOrCreate<UTextBlock>(Blueprint, FName(Detail.Name));
			PlaceCanvas(Board, Text, Detail.Position, Detail.Size, 8);
			Text->SetText(FText::FromString(Detail.Preview));
			SetReadableFont(Text, BaseFont, Detail.FontSize);
			Text->SetJustification(ETextJustify::Left);
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
		if (MercenaryTexture == nullptr || MonsterTexture == nullptr
			|| SpeedTexture == nullptr || TurnTokenFrameTexture == nullptr
			|| OptionsRailFrameTexture == nullptr || MapTexture == nullptr
			|| SettingsTexture == nullptr || ArtifactSlotTexture == nullptr
			|| RoundBadgeTexture == nullptr || MercenaryShellTexture == nullptr
			|| MercenaryCardNormalTexture == nullptr
			|| MercenaryCardSelectedTexture == nullptr)
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
			MercenaryCardNormalTexture, MercenaryCardSelectedTexture);
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
			ArtifactSlot->SetPosition(FVector2D(18.f, -244.f));
			ArtifactSlot->SetSize(FVector2D(500.f, 84.f));
		}
		UImage* ArtifactTrayFrame = FindOrCreate<UImage>(Blueprint, TEXT("ArtifactTrayFrame"));
		ArtifactTrayFrame->SetVisibility(ESlateVisibility::Collapsed);
		for (int32 Index = 0; Index < 6; ++Index)
		{
			if (UImage* ArtifactFrame = Cast<UImage>(Blueprint->WidgetTree->FindWidget(
				FName(*FString::Printf(TEXT("ArtifactFrame_%d"), Index)))))
			{
				PlaceCanvas(ArtifactStrip, ArtifactFrame,
					FVector2D(4.f + 80.f * Index, 4.f), FVector2D(76.f, 76.f), 1);
				ArtifactFrame->SetBrushFromTexture(ArtifactSlotTexture, false);
				ArtifactFrame->SetVisibility(ESlateVisibility::Collapsed);
			}
			if (UImage* ArtifactIcon = Cast<UImage>(Blueprint->WidgetTree->FindWidget(
				FName(*FString::Printf(TEXT("ArtifactIcon_%d"), Index)))))
			{
				PlaceCanvas(ArtifactStrip, ArtifactIcon,
					FVector2D(14.f + 80.f * Index, 14.f), FVector2D(56.f, 56.f), 2);
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
