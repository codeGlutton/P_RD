#include "UI/RewardSettlementWidgetBuilder.h"

#include "AssetToolsModule.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/Texture2D.h"
#include "WidgetBlueprintFactory.h"
#include "HAL/IConsoleManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UI/UIFont.h"
#include "UI/Reward/RewardSettlementWidgetBase.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintEditorUtils.h"

namespace RewardSettlementWidgetBuilder
{
	constexpr TCHAR PackagePath[] = TEXT("/Game/UI/RewardSettlement");
	constexpr TCHAR AssetName[] = TEXT("WBP_RewardSettlement_Runtime");
	constexpr TCHAR AssetPath[] = TEXT("/Game/UI/RewardSettlement/WBP_RewardSettlement_Runtime.WBP_RewardSettlement_Runtime");
	TUniquePtr<FAutoConsoleCommand> BuildCommand;
	TUniquePtr<FAutoConsoleCommand> VerifyCommand;

	FIntPoint NativeTextureSize(UTexture2D* Source)
	{
		if (Source == nullptr)
		{
			return FIntPoint::ZeroValue;
		}

		const FIntPoint ImportedSize = Source->GetImportedSize();
		if (ImportedSize.X > 0 && ImportedSize.Y > 0)
		{
			return ImportedSize;
		}
		return FIntPoint(Source->GetSizeX(), Source->GetSizeY());
	}

	UTexture2D* Texture(const TCHAR* Path)
	{
		UTexture2D* Result = LoadObject<UTexture2D>(nullptr, Path);
		checkf(Result != nullptr, TEXT("Missing reward settlement texture: %s"), Path);
		return Result;
	}

	FSlateBrush TextureBrush(UTexture2D* Source, const FBox2f* UV = nullptr)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Source);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		const FIntPoint NativeSize = NativeTextureSize(Source);
		if (NativeSize.X > 0 && NativeSize.Y > 0)
		{
			Brush.ImageSize = FVector2D(NativeSize);
		}
		if (UV != nullptr)
		{
			Brush.SetUVRegion(*UV);
		}
		return Brush;
	}

	void StyleText(UTextBlock* Text, int32 Size,
		ETextJustify::Type Justification = ETextJustify::Center)
	{
		FSlateFontInfo Font = UIFont::MakeProjectExact(Text->GetFont(), Size);
		Font.OutlineSettings.OutlineSize = Size >= 28 ? 1 : 0;
		Font.OutlineSettings.OutlineColor = FLinearColor::Black;
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Text->SetShadowOffset(FVector2D(1.5f, 1.5f));
		Text->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.7f));
		Text->SetJustification(Justification);
		Text->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	void Place(UCanvasPanel* Parent, UWidget* Child, const FVector2D Position,
		const FVector2D Size, int32 ZOrder)
	{
		UCanvasPanelSlot* Slot = Parent->AddChildToCanvas(Child);
		Slot->SetAnchors(FAnchors(0.f));
		Slot->SetAlignment(FVector2D::ZeroVector);
		Slot->SetAutoSize(false);
		Slot->SetPosition(Position);
		Slot->SetSize(Size);
		Slot->SetZOrder(ZOrder);
	}

	void Anchor(UCanvasPanel* Parent, UWidget* Child, const FAnchors Anchors,
		const FMargin& Offsets, int32 ZOrder)
	{
		UCanvasPanelSlot* Slot = Parent->AddChildToCanvas(Child);
		Slot->SetAnchors(Anchors);
		Slot->SetAlignment(FVector2D::ZeroVector);
		Slot->SetAutoSize(false);
		Slot->SetOffsets(Offsets);
		Slot->SetZOrder(ZOrder);
	}

	UCanvasPanel* AddFixedDesignRegion(UWidgetBlueprint* Blueprint, UCanvasPanel* Root,
		const FName ScaleName, const FName CanvasName, const FVector2D Position,
		const FVector2D DesignSize, int32 ZOrder)
	{
		UScaleBox* Scale = Blueprint->WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), ScaleName);
		Scale->SetStretch(EStretch::ScaleToFit);
		Scale->SetStretchDirection(EStretchDirection::Both);
		Scale->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		Place(Root, Scale, Position, DesignSize, ZOrder);

		USizeBox* Size = Blueprint->WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),
			FName(*(ScaleName.ToString() + TEXT("_DesignSize"))));
		Size->SetWidthOverride(DesignSize.X);
		Size->SetHeightOverride(DesignSize.Y);
		Scale->AddChild(Size);

		UCanvasPanel* Canvas = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), CanvasName);
		Size->SetContent(Canvas);
		return Canvas;
	}

	UImage* AddImage(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent, const FName Name,
		UTexture2D* Source, const FBox2f* UV, const FVector2D Position,
		const FVector2D Size, int32 ZOrder)
	{
		UImage* Image = Blueprint->WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
		Image->SetBrush(TextureBrush(Source, UV));
		Image->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Place(Parent, Image, Position, Size, ZOrder);
		return Image;
	}

	FVector2D AspectFitSize(UTexture2D* Source, const FVector2D Bounds)
	{
		const FIntPoint TextureSize = NativeTextureSize(Source);
		if (TextureSize.X <= 0 || TextureSize.Y <= 0)
		{
			return Bounds;
		}
		const FVector2D NativeSize(TextureSize);
		const double UniformScale = FMath::Min(Bounds.X / NativeSize.X, Bounds.Y / NativeSize.Y);
		return NativeSize * UniformScale;
	}

	UImage* AddAspectImage(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent, const FName Name,
		UTexture2D* Source, const FBox2f* UV, const FVector2D BoundsPosition,
		const FVector2D BoundsSize, int32 ZOrder)
	{
		const FVector2D FittedSize = AspectFitSize(Source, BoundsSize);
		const FVector2D FittedPosition = BoundsPosition + (BoundsSize - FittedSize) * 0.5f;
		return AddImage(Blueprint, Parent, Name, Source, UV, FittedPosition, FittedSize, ZOrder);
	}

	UScaleBox* AddScaleImage(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent,
		const FName ScaleName, const FName ImageName, UTexture2D* Source,
		const FVector2D Position, const FVector2D Size, int32 ZOrder)
	{
		UScaleBox* Scale = Blueprint->WidgetTree->ConstructWidget<UScaleBox>(
			UScaleBox::StaticClass(), ScaleName);
		Scale->SetStretch(EStretch::ScaleToFit);
		Scale->SetStretchDirection(EStretchDirection::Both);
		Scale->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		Scale->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Place(Parent, Scale, Position, Size, ZOrder);

		UImage* Image = Blueprint->WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), ImageName);
		Image->SetBrush(TextureBrush(Source));
		Image->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Scale->AddChild(Image);
		return Scale;
	}

	UTextBlock* AddText(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent, const FName Name,
		const FText& Value, int32 FontSize,
		const FVector2D Position, const FVector2D Size, int32 ZOrder)
	{
		UTextBlock* Text = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Text->SetText(Value);
		StyleText(Text, FontSize);
		Place(Parent, Text, Position, Size, ZOrder);
		return Text;
	}

	void AddPreviewRow(UWidgetBlueprint* Blueprint, UVerticalBox* Box, const FName Name,
		const FText& Label, float Height, const FLinearColor& Color,
		UTexture2D* Icon = nullptr, UTexture2D* IconFrame = nullptr)
	{
		USizeBox* Size = Blueprint->WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),
			FName(*(Name.ToString() + TEXT("_Size"))));
		Size->SetHeightOverride(Height);
		UOverlay* Overlay = Blueprint->WidgetTree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(), FName(*(Name.ToString() + TEXT("_Overlay"))));
		Size->SetContent(Overlay);
		UBorder* Border = Blueprint->WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
		Border->SetBrushColor(Color);
		Border->SetPadding(FMargin(0.f));
		Overlay->AddChildToOverlay(Border);
		UCanvasPanel* Canvas = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), FName(*(Name.ToString() + TEXT("_Canvas"))));
		Overlay->AddChildToOverlay(Canvas);
		const float IconExtent = FMath::Min(Height - 18.f, 92.f);
		if (IconFrame != nullptr)
		{
			AddAspectImage(Blueprint, Canvas, FName(*(Name.ToString() + TEXT("_IconFrame"))),
				IconFrame, nullptr, FVector2D(10.f, (Height - IconExtent) * .5f),
				FVector2D(IconExtent, IconExtent), 1);
		}
		if (Icon != nullptr)
		{
			const float Inset = IconFrame != nullptr ? IconExtent * .15f : 0.f;
			AddAspectImage(Blueprint, Canvas, FName(*(Name.ToString() + TEXT("_Icon"))),
				Icon, nullptr, FVector2D(10.f + Inset, (Height - IconExtent) * .5f + Inset),
				FVector2D(IconExtent - Inset * 2.f, IconExtent - Inset * 2.f), 2);
		}
		UTextBlock* Text = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
			FName(*(Name.ToString() + TEXT("_Text"))));
		Text->SetText(Label);
		StyleText(Text, 22, ETextJustify::Left);
		Place(Canvas, Text, FVector2D(Icon != nullptr ? 116.f : 16.f, 7.f),
			FVector2D(Icon != nullptr ? 900.f : 1000.f, Height - 14.f), 3);
		UVerticalBoxSlot* Slot = Box->AddChildToVerticalBox(Size);
		Slot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		Slot->SetHorizontalAlignment(HAlign_Fill);
	}

	UWidgetBlueprint* FindOrCreateBlueprint()
	{
		if (UWidgetBlueprint* Existing = LoadObject<UWidgetBlueprint>(nullptr, AssetPath))
		{
			return Existing;
		}

		UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
		Factory->ParentClass = URewardSettlementWidgetBase::StaticClass();
		FAssetToolsModule& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		return Cast<UWidgetBlueprint>(AssetTools.Get().CreateAsset(
			AssetName, PackagePath, UWidgetBlueprint::StaticClass(), Factory));
	}

	void Build()
	{
		UE_LOG(LogTemp, Display, TEXT("RD_REWARD_SETTLEMENT_BUILD begin"));
		// Resolve every new hard dependency before touching the existing WidgetTree.
		// A missing import must fail without leaving the currently open asset empty.
		UTexture2D* HeaderBlank = Texture(TEXT("/Game/UI/ResultBoards/Art/T_VR_HeaderBlank_0809.T_VR_HeaderBlank_0809"));
		UTexture2D* PanelBlank = Texture(TEXT("/Game/UI/ResultBoards/Art/T_VR_PanelBlank_0809.T_VR_PanelBlank_0809"));
		UTexture2D* TabBlank = Texture(TEXT("/Game/UI/ResultBoards/Art/T_VR_TabBlank_0809.T_VR_TabBlank_0809"));
		UTexture2D* PrimaryButtonBlank = Texture(TEXT("/Game/UI/ResultBoards/Art/T_UI_ButtonPrimaryBlank_0809.T_UI_ButtonPrimaryBlank_0809"));
		UTexture2D* PortraitFrame = Texture(TEXT("/Game/UI/ResultBoards/Art/T_VR_PortraitFrame_0809.T_VR_PortraitFrame_0809"));
		UTexture2D* ProgressTrack = Texture(TEXT("/Game/UI/ResultBoards/Art/T_VR_ProgressTrack_0809.T_VR_ProgressTrack_0809"));
		UTexture2D* ProgressFill = Texture(TEXT("/Game/UI/ResultBoards/Art/T_VR_ProgressFill_0809.T_VR_ProgressFill_0809"));
		UTexture2D* XPTicket = Texture(TEXT("/Game/UI/ResultBoards/Art/T_VR_XPTicketBlank_0809.T_VR_XPTicketBlank_0809"));
		UTexture2D* ChoiceCard = Texture(TEXT("/Game/UI/ResultBoards/Art/T_VR_RewardCardBlank_0809.T_VR_RewardCardBlank_0809"));
		UTexture2D* GoldCoin = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Common/Icons/T_Reward_GoldIcon_V1.T_Reward_GoldIcon_V1"));
		UTexture2D* EquipmentIcon = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Items/Equipment/T_equip_weapon_common.T_equip_weapon_common"));
		UTexture2D* ExpIcon = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Combat/Result/Rewards/T_reward_v4_exp_icon.T_reward_v4_exp_icon"));
		UTexture2D* ChoiceGoldIcon = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Combat/Result/Rewards/T_reward_v4_gold_icon.T_reward_v4_gold_icon"));
		UTexture2D* Knight = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Characters/Mercenaries/T_MB_HireIcon_Knight.T_MB_HireIcon_Knight"));
		UTexture2D* Mage = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Characters/Mercenaries/T_MB_HireIcon_Mage.T_MB_HireIcon_Mage"));
		UTexture2D* Rogue = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Characters/Mercenaries/T_MB_HireIcon_Rogue.T_MB_HireIcon_Rogue"));
		UE_LOG(LogTemp, Display, TEXT("RD_REWARD_SETTLEMENT_BUILD dependencies-ready"));

		UWidgetBlueprint* Blueprint = FindOrCreateBlueprint();
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("RD_REWARD_SETTLEMENT_BUILD could not create %s"), AssetPath);
			return;
		}

		Blueprint->Modify();
		Blueprint->WidgetTree->Modify();
		// This builder replaces the complete tree. Remove the previous root first so
		// widgets that keep a name but change class do not collide as UObject siblings.
		// DeleteWidgets structurally compiles immediately, so use the neutral UUserWidget
		// parent during that one transient compile to avoid false BindWidget errors.
		if (UWidget* PreviousRoot = Blueprint->WidgetTree->RootWidget)
		{
			UE_LOG(LogTemp, Display, TEXT("RD_REWARD_SETTLEMENT_BUILD deleting-old-tree"));
			Blueprint->ParentClass = UUserWidget::StaticClass();
			TSet<UWidget*> PreviousWidgets;
			PreviousWidgets.Add(PreviousRoot);
			FWidgetBlueprintEditorUtils::DeleteWidgets(
				Blueprint,
				MoveTemp(PreviousWidgets),
				FWidgetBlueprintEditorUtils::EDeleteWidgetWarningType::DeleteSilently);
		}
		Blueprint->ParentClass = URewardSettlementWidgetBase::StaticClass();
		UE_LOG(LogTemp, Display, TEXT("RD_REWARD_SETTLEMENT_BUILD old-tree-deleted"));

		UOverlay* Root = Blueprint->WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("SettlementViewportRoot"));
		Blueprint->WidgetTree->RootWidget = Root;

		UBorder* Dimmer = Blueprint->WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("WorldDimmer"));
		Dimmer->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, .30f));
		Root->AddChildToOverlay(Dimmer);
		CastChecked<UOverlaySlot>(Dimmer->Slot)->SetHorizontalAlignment(HAlign_Fill);
		CastChecked<UOverlaySlot>(Dimmer->Slot)->SetVerticalAlignment(VAlign_Fill);

		UCanvasPanel* Screen = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("SettlementResponsiveCanvas"));
		Root->AddChildToOverlay(Screen);
		CastChecked<UOverlaySlot>(Screen->Slot)->SetHorizontalAlignment(HAlign_Fill);
		CastChecked<UOverlaySlot>(Screen->Slot)->SetVerticalAlignment(VAlign_Fill);

		// 모든 영역을 하나의 1600x900 기준 캔버스에서 함께 스케일한다.
		// 헤더/요약/본문을 각각 화면 앵커에 맞춰 축소하면 화면비에 따라 서로 다른
		// 배율이 적용되어 간격과 글자 크기가 무너진다. 단일 ScaleBox는 모바일의
		// 16:9, 19.5:9, 4:3 모두에서 영역 간 비율을 동일하게 유지한다.
		UScaleBox* MasterScale = Blueprint->WidgetTree->ConstructWidget<UScaleBox>(
			UScaleBox::StaticClass(), TEXT("SettlementMasterScale"));
		MasterScale->SetStretch(EStretch::ScaleToFit);
		MasterScale->SetStretchDirection(EStretchDirection::Both);
		MasterScale->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		Anchor(Screen, MasterScale, FAnchors(0.f, 0.f, 1.f, 1.f), FMargin(0.f), 10);

		USizeBox* MasterDesignSize = Blueprint->WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("SettlementMasterDesignSize"));
		MasterDesignSize->SetWidthOverride(1600.f);
		MasterDesignSize->SetHeightOverride(900.f);
		MasterScale->AddChild(MasterDesignSize);

		UCanvasPanel* DesignCanvas = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("SettlementDesignCanvas"));
		MasterDesignSize->SetContent(DesignCanvas);

		// 0809 victory reference: every visible part shares the one 1600x900 master.
		// Header and tabs intentionally overlap, and the CTA hangs just below the panel.
		UCanvasPanel* Header = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("HeaderCanvas"));
		Place(DesignCanvas, Header, FVector2D(452.f, 20.f), FVector2D(696.f, 150.f), 20);
		AddAspectImage(Blueprint, Header, TEXT("HeaderBlankArt"), HeaderBlank, nullptr,
			FVector2D::ZeroVector, FVector2D(696.f, 150.f), 0);
		AddText(Blueprint, Header, TEXT("mTitleText"),
			NSLOCTEXT("RewardSettlement", "Title", "전투 보상"), 44,
			FVector2D(78.f, 30.f),
			FVector2D(540.f, 68.f), 2);

		AddAspectImage(Blueprint, DesignCanvas, TEXT("StepCoinArt_1"), TabBlank, nullptr,
			FVector2D(704.f, 145.f), FVector2D(76.f, 76.f), 22);
		AddAspectImage(Blueprint, DesignCanvas, TEXT("StepCoinArt_2"), TabBlank, nullptr,
			FVector2D(792.f, 145.f), FVector2D(76.f, 76.f), 22);
		AddText(Blueprint, DesignCanvas, TEXT("StepCoinNumber_1"), FText::AsNumber(1),
			31, FVector2D(704.f, 151.f),
			FVector2D(76.f, 58.f), 23);
		AddText(Blueprint, DesignCanvas, TEXT("StepCoinNumber_2"), FText::AsNumber(2),
			31, FVector2D(792.f, 151.f),
			FVector2D(76.f, 58.f), 23);

		AddAspectImage(Blueprint, DesignCanvas, TEXT("MainPanelArt"), PanelBlank, nullptr,
			FVector2D(324.f, 210.f), FVector2D(948.f, 520.f), 14);

		// ExpCanvas is the parchment's usable inset, not the outer framed panel.
		// Every functional result/choice part is authored below as a stable widget.
		// Designers can therefore move, resize and preview the real runtime nodes.
		UCanvasPanel* Exp = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("ExpCanvas"));
		Place(DesignCanvas, Exp, FVector2D(362.f, 249.f), FVector2D(872.f, 442.f), 16);

		UWidgetSwitcher* StepSwitcher = Blueprint->WidgetTree->ConstructWidget<UWidgetSwitcher>(
			UWidgetSwitcher::StaticClass(), TEXT("SettlementStepSwitcher"));
		Place(Exp, StepSwitcher, FVector2D::ZeroVector, FVector2D(872.f, 442.f), 0);

		UCanvasPanel* ResultStep = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("SettlementResultStep"));
		ResultStep->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		StepSwitcher->AddChild(ResultStep);

		// Gold occupies the centered top band. The live runtime only changes the
		// brush/text; its position and bounds remain owned by the WBP designer.
		AddAspectImage(Blueprint, ResultStep, TEXT("SettlementGoldCoin"), GoldCoin, nullptr,
			FVector2D(322.f, 2.f), FVector2D(66.f, 66.f), 3);
		AddText(Blueprint, ResultStep, TEXT("SettlementGoldGain"),
			NSLOCTEXT("RewardSettlement", "GoldPreview", "+22"), 31,
			FVector2D(398.f, 2.f), FVector2D(160.f, 66.f), 3);

		UTexture2D* PreviewPortraits[] = { Knight, Mage, Rogue };
		const TCHAR* PreviewLevels[] = { TEXT("Lv.1"), TEXT("Lv.2"), TEXT("Lv.3") };
		const TCHAR* PreviewProgress[] = { TEXT("50 / 250"), TEXT("150 / 250"), TEXT("200 / 250") };
		const float PreviewPercents[] = { .20f, .60f, .80f };
		for (int32 Index = 0; Index < 3; ++Index)
		{
			UCanvasPanel* Row = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
				UCanvasPanel::StaticClass(),
				*FString::Printf(TEXT("SettlementExpRow_%d"), Index));
			Row->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			Place(ResultStep, Row, FVector2D(18.f, 100.f + 114.f * Index),
				FVector2D(836.f, 102.f), 1);

			AddAspectImage(Blueprint, Row,
				*FString::Printf(TEXT("SettlementPortraitPlate_%d"), Index),
				PortraitFrame, nullptr, FVector2D::ZeroVector, FVector2D(102.f, 102.f), 1);
			AddScaleImage(Blueprint, Row,
				*FString::Printf(TEXT("SettlementPortraitFit_%d"), Index),
				*FString::Printf(TEXT("SettlementPortrait_%d"), Index),
				PreviewPortraits[Index], FVector2D(12.24f, 12.24f),
				FVector2D(77.52f, 77.52f), 2);
			AddText(Blueprint, Row,
				*FString::Printf(TEXT("SettlementMercenaryLevel_%d"), Index),
				FText::FromString(FString(PreviewLevels[Index])), 27,
				FVector2D(120.f, 0.f), FVector2D(90.f, 102.f), 3);

			const FVector2D TrackBounds(430.f, 58.f);
			const FVector2D TrackSize = AspectFitSize(ProgressTrack, TrackBounds);
			const FVector2D TrackPosition = FVector2D(222.f, 22.f)
				+ (TrackBounds - TrackSize) * .5f;
			AddImage(Blueprint, Row,
				*FString::Printf(TEXT("SettlementMercenaryTrack_%d"), Index),
				ProgressTrack, nullptr, TrackPosition, TrackSize, 2);

			// The fill image keeps its full native ratio. Progress is represented by
			// resizing this clipping mount, never by stretching the fill ornament.
			const FVector2D FillBounds(430.f, 40.f);
			const FVector2D FillSize = AspectFitSize(ProgressFill, FillBounds);
			const FVector2D FillPosition = FVector2D(222.f, 31.f)
				+ (FillBounds - FillSize) * .5f;
			UCanvasPanel* FillClip = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
				UCanvasPanel::StaticClass(),
				*FString::Printf(TEXT("SettlementMercenaryBarClip_%d"), Index));
			FillClip->SetClipping(EWidgetClipping::ClipToBoundsAlways);
			FillClip->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			Place(Row, FillClip, FillPosition,
				FVector2D(FillSize.X * PreviewPercents[Index], FillSize.Y), 3);
			AddImage(Blueprint, FillClip,
				*FString::Printf(TEXT("SettlementMercenaryBar_%d"), Index),
				ProgressFill, nullptr, FVector2D::ZeroVector, FillSize, 0);
			AddText(Blueprint, Row,
				*FString::Printf(TEXT("SettlementMercenaryBarText_%d"), Index),
				FText::FromString(FString(PreviewProgress[Index])), 20,
				FVector2D(222.f, 32.f), FVector2D(430.f, 36.f), 4);

			AddAspectImage(Blueprint, Row,
				*FString::Printf(TEXT("SettlementXPRibbon_%d"), Index),
				XPTicket, nullptr, FVector2D(670.f, 20.f), FVector2D(166.f, 62.f), 3);
			AddText(Blueprint, Row,
				*FString::Printf(TEXT("SettlementXPText_%d"), Index),
				NSLOCTEXT("RewardSettlement", "XPPreview", "+50 XP"), 22,
				FVector2D(670.f, 20.f), FVector2D(166.f, 62.f), 4);
		}

		UCanvasPanel* ChoiceStep = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("SettlementChoiceStep"));
		ChoiceStep->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		StepSwitcher->AddChild(ChoiceStep);
		StepSwitcher->SetActiveWidgetIndex(0);

		UTexture2D* PreviewChoiceIcons[] = { EquipmentIcon, ExpIcon, ChoiceGoldIcon };
		const FText PreviewChoiceNames[] = {
			NSLOCTEXT("RewardSettlement", "EquipmentPreview", "아티팩트"),
			NSLOCTEXT("RewardSettlement", "SkillPreview", "스킬"),
			NSLOCTEXT("RewardSettlement", "ChoiceGoldPreview", "골드")
		};
		const float ChoiceXs[] = { 38.f, 318.f, 598.f };
		for (int32 Index = 0; Index < 3; ++Index)
		{
			UCanvasPanel* Mount = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
				UCanvasPanel::StaticClass(),
				*FString::Printf(TEXT("SettlementChoiceMount_%d"), Index));
			Mount->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			Place(ChoiceStep, Mount, FVector2D(ChoiceXs[Index], 52.f),
				FVector2D(236.f, 338.f), 1);

			const FVector2D CardBounds(236.f, 338.f);
			const FVector2D CardArtSize = AspectFitSize(ChoiceCard, CardBounds);
			const FVector2D CardArtPosition = (CardBounds - CardArtSize) * .5f;
			AddImage(Blueprint, Mount,
				*FString::Printf(TEXT("SettlementChoiceCard_%d"), Index),
				ChoiceCard, nullptr, CardArtPosition, CardArtSize, 2);

			const FVector2D IconPosition = CardArtPosition
				+ FVector2D(CardArtSize.X * .20f, CardArtSize.Y * .17f);
			const FVector2D IconSize(CardArtSize.X * .60f, CardArtSize.X * .60f);
			AddScaleImage(Blueprint, Mount,
				*FString::Printf(TEXT("SettlementChoiceIconFit_%d"), Index),
				*FString::Printf(TEXT("SettlementChoiceIcon_%d"), Index),
				PreviewChoiceIcons[Index], IconPosition, IconSize, 3);
			UTextBlock* ChoiceName = AddText(Blueprint, Mount,
				*FString::Printf(TEXT("SettlementChoiceName_%d"), Index),
				PreviewChoiceNames[Index], 22,
				CardArtPosition + FVector2D(CardArtSize.X * .08f, CardArtSize.Y * .84f),
				FVector2D(CardArtSize.X * .84f, CardArtSize.Y * .12f), 4);
			FSlateFontInfo ChoiceNameFont = ChoiceName->GetFont();
			ChoiceNameFont.OutlineSettings.OutlineSize = 1;
			ChoiceNameFont.OutlineSettings.OutlineColor = FLinearColor::Black;
			ChoiceName->SetFont(ChoiceNameFont);

			UButton* PickButton = Blueprint->WidgetTree->ConstructWidget<UButton>(
				UButton::StaticClass(),
				*FString::Printf(TEXT("SettlementChoiceButton_%d"), Index));
			FButtonStyle PickStyle = PickButton->GetStyle();
			for (FSlateBrush* Brush : { &PickStyle.Normal, &PickStyle.Hovered,
				&PickStyle.Pressed, &PickStyle.Disabled })
			{
				Brush->DrawAs = ESlateBrushDrawType::NoDrawType;
			}
			PickButton->SetStyle(PickStyle);
			Place(Mount, PickButton, FVector2D::ZeroVector, FVector2D(236.f, 338.f), 5);
		}

		// The native base still has mandatory BindWidget fields from the previous WBP.
		// Keep type-correct placeholders, but never let the removed side column paint.
		UTextBlock* GoldBalance = AddText(Blueprint, DesignCanvas, TEXT("mGoldBalanceText"),
			FText::GetEmpty(), 1, FVector2D::ZeroVector,
			FVector2D(1.f, 1.f), 0);
		GoldBalance->SetVisibility(ESlateVisibility::Collapsed);
		UVerticalBox* SummaryRows = Blueprint->WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("mSummaryRowsBox"));
		SummaryRows->SetVisibility(ESlateVisibility::Collapsed);
		Place(DesignCanvas, SummaryRows, FVector2D::ZeroVector, FVector2D(1.f, 1.f), 0);
		UVerticalBox* MercenaryRows = Blueprint->WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("mMercenaryRowsBox"));
		MercenaryRows->SetVisibility(ESlateVisibility::Collapsed);
		Place(DesignCanvas, MercenaryRows, FVector2D::ZeroVector, FVector2D(1.f, 1.f), 0);

		UCanvasPanel* NextHolder = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("NextButtonHolder"));
		Place(DesignCanvas, NextHolder, FVector2D(946.f, 728.f), FVector2D(350.f, 110.f), 25);
		AddAspectImage(Blueprint, NextHolder, TEXT("NextButtonArt"), PrimaryButtonBlank, nullptr,
			FVector2D::ZeroVector, FVector2D(350.f, 110.f), 0);
		AddText(Blueprint, NextHolder, TEXT("mNextButtonText"),
			NSLOCTEXT("RewardSettlement", "Next", "다음"), 36,
			FVector2D(38.f, 23.f),
			FVector2D(274.f, 62.f), 2);
		UButton* NextButton = Blueprint->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("mNextButton"));
		FButtonStyle TransparentStyle;
		FSlateBrush Empty; Empty.DrawAs = ESlateBrushDrawType::NoDrawType;
		TransparentStyle.SetNormal(Empty); TransparentStyle.SetHovered(Empty);
		TransparentStyle.SetPressed(Empty); TransparentStyle.SetDisabled(Empty);
		NextButton->SetStyle(TransparentStyle);
		Place(NextHolder, NextButton, FVector2D::ZeroVector, FVector2D(350.f, 110.f), 5);

		// UE 5.7 컴파일러는 트리의 모든 위젯에 GUID가 있어야 한다. 이전 트리의
		// GUID는 DeleteWidgets가 위젯과 애니메이션 항목을 구분해 안전하게 제거한다.
		Blueprint->WidgetTree->ForEachWidget([Blueprint](UWidget* Widget)
		{
			if (Widget == nullptr)
			{
				return;
			}
			if (Blueprint->WidgetVariableNameToGuidMap.Contains(Widget->GetFName()) == false)
			{
				Blueprint->OnVariableAdded(Widget->GetFName());
			}
		});

		UE_LOG(LogTemp, Display, TEXT("RD_REWARD_SETTLEMENT_BUILD compiling"));
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		UE_LOG(LogTemp, Display, TEXT("RD_REWARD_SETTLEMENT_BUILD compiled"));
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Blueprint->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
		if (!UPackage::SavePackage(Blueprint->GetPackage(), Blueprint, *Filename, FSavePackageArgs()))
		{
			UE_LOG(LogTemp, Error, TEXT("RD_REWARD_SETTLEMENT_BUILD save failed"));
			return;
		}
		UE_LOG(LogTemp, Display, TEXT("RD_REWARD_SETTLEMENT_BUILD success asset=%s layout=0809-victory"), AssetPath);
	}

	void Verify()
	{
		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, AssetPath);
		checkf(Blueprint != nullptr && Blueprint->WidgetTree != nullptr,
			TEXT("Reward settlement WBP is missing"));
		checkf(Blueprint->ParentClass == URewardSettlementWidgetBase::StaticClass(),
			TEXT("Reward settlement WBP uses the wrong parent class"));

		int32 VerifiedWidgetCount = 0;
		auto RequireClass = [Blueprint, &VerifiedWidgetCount](const FName Name, UClass* ExpectedClass)
		{
			UWidget* Widget = Blueprint->WidgetTree->FindWidget(Name);
			checkf(Widget != nullptr, TEXT("Reward settlement WBP is missing widget %s"),
				*Name.ToString());
			checkf(Widget->IsA(ExpectedClass),
				TEXT("Reward settlement widget %s has type %s; expected %s"),
				*Name.ToString(), *Widget->GetClass()->GetName(), *ExpectedClass->GetName());
			++VerifiedWidgetCount;
		};

		RequireClass(TEXT("SettlementViewportRoot"), UOverlay::StaticClass());
		RequireClass(TEXT("WorldDimmer"), UBorder::StaticClass());
		RequireClass(TEXT("SettlementMasterScale"), UScaleBox::StaticClass());
		RequireClass(TEXT("SettlementMasterDesignSize"), USizeBox::StaticClass());
		RequireClass(TEXT("SettlementStepSwitcher"), UWidgetSwitcher::StaticClass());

		const FName CanvasNames[] = {
			TEXT("SettlementResponsiveCanvas"), TEXT("SettlementDesignCanvas"),
			TEXT("HeaderCanvas"), TEXT("ExpCanvas"), TEXT("SettlementResultStep"),
			TEXT("SettlementChoiceStep"), TEXT("NextButtonHolder")
		};
		for (const FName Name : CanvasNames)
		{
			RequireClass(Name, UCanvasPanel::StaticClass());
		}

		const FName ImageNames[] = {
			TEXT("HeaderBlankArt"), TEXT("MainPanelArt"),
			TEXT("StepCoinArt_1"), TEXT("StepCoinArt_2"),
			TEXT("SettlementGoldCoin"), TEXT("NextButtonArt")
		};
		for (const FName Name : ImageNames)
		{
			RequireClass(Name, UImage::StaticClass());
		}

		const FName TextNames[] = {
			TEXT("mTitleText"), TEXT("StepCoinNumber_1"), TEXT("StepCoinNumber_2"),
			TEXT("SettlementGoldGain"), TEXT("mGoldBalanceText"), TEXT("mNextButtonText")
		};
		for (const FName Name : TextNames)
		{
			RequireClass(Name, UTextBlock::StaticClass());
		}

		RequireClass(TEXT("mSummaryRowsBox"), UVerticalBox::StaticClass());
		RequireClass(TEXT("mMercenaryRowsBox"), UVerticalBox::StaticClass());
		RequireClass(TEXT("mNextButton"), UButton::StaticClass());

		for (int32 Index = 0; Index < 3; ++Index)
		{
			RequireClass(FName(*FString::Printf(TEXT("SettlementExpRow_%d"), Index)),
				UCanvasPanel::StaticClass());
			RequireClass(FName(*FString::Printf(TEXT("SettlementMercenaryBarClip_%d"), Index)),
				UCanvasPanel::StaticClass());
			RequireClass(FName(*FString::Printf(TEXT("SettlementChoiceMount_%d"), Index)),
				UCanvasPanel::StaticClass());

			RequireClass(FName(*FString::Printf(TEXT("SettlementPortraitFit_%d"), Index)),
				UScaleBox::StaticClass());
			RequireClass(FName(*FString::Printf(TEXT("SettlementChoiceIconFit_%d"), Index)),
				UScaleBox::StaticClass());

			const FName RowImageNames[] = {
				FName(*FString::Printf(TEXT("SettlementPortraitPlate_%d"), Index)),
				FName(*FString::Printf(TEXT("SettlementPortrait_%d"), Index)),
				FName(*FString::Printf(TEXT("SettlementMercenaryTrack_%d"), Index)),
				FName(*FString::Printf(TEXT("SettlementMercenaryBar_%d"), Index)),
				FName(*FString::Printf(TEXT("SettlementXPRibbon_%d"), Index)),
				FName(*FString::Printf(TEXT("SettlementChoiceCard_%d"), Index)),
				FName(*FString::Printf(TEXT("SettlementChoiceIcon_%d"), Index))
			};
			for (const FName Name : RowImageNames)
			{
				RequireClass(Name, UImage::StaticClass());
			}

			const FName RowTextNames[] = {
				FName(*FString::Printf(TEXT("SettlementMercenaryLevel_%d"), Index)),
				FName(*FString::Printf(TEXT("SettlementMercenaryBarText_%d"), Index)),
				FName(*FString::Printf(TEXT("SettlementXPText_%d"), Index)),
				FName(*FString::Printf(TEXT("SettlementChoiceName_%d"), Index))
			};
			for (const FName Name : RowTextNames)
			{
				RequireClass(Name, UTextBlock::StaticClass());
			}
			RequireClass(FName(*FString::Printf(TEXT("SettlementChoiceButton_%d"), Index)),
				UButton::StaticClass());
		}

		UCanvasPanel* ResultStep = CastChecked<UCanvasPanel>(
			Blueprint->WidgetTree->FindWidget(TEXT("SettlementResultStep")));
		UCanvasPanel* ChoiceStep = CastChecked<UCanvasPanel>(
			Blueprint->WidgetTree->FindWidget(TEXT("SettlementChoiceStep")));
		UWidgetSwitcher* StepSwitcher = CastChecked<UWidgetSwitcher>(
			Blueprint->WidgetTree->FindWidget(TEXT("SettlementStepSwitcher")));
		checkf(ResultStep->GetVisibility() == ESlateVisibility::SelfHitTestInvisible,
			TEXT("Reward settlement result step must be visible by default"));
		checkf(ChoiceStep->GetVisibility() == ESlateVisibility::SelfHitTestInvisible,
			TEXT("Reward settlement choice step must remain designer-visible"));
		checkf(StepSwitcher->GetActiveWidgetIndex() == 0
			&& StepSwitcher->GetWidgetAtIndex(0) == ResultStep
			&& StepSwitcher->GetWidgetAtIndex(1) == ChoiceStep,
			TEXT("Reward settlement step switcher has the wrong child order/default"));
		checkf(Blueprint->GeneratedClass != nullptr
			&& Blueprint->GeneratedClass->IsChildOf(URewardSettlementWidgetBase::StaticClass()),
			TEXT("Reward settlement generated class is invalid"));
		UE_LOG(LogTemp, Display,
			TEXT("RD_REWARD_SETTLEMENT_VERIFY success widgets=%d parent=%s"),
			VerifiedWidgetCount, *Blueprint->ParentClass->GetName());
	}
}

void RegisterRewardSettlementWidgetBuilderCommands()
{
	using namespace RewardSettlementWidgetBuilder;
	BuildCommand = MakeUnique<FAutoConsoleCommand>(TEXT("RD.Editor.BuildRewardSettlement"),
		TEXT("Create the independent responsive reward settlement WBP."),
		FConsoleCommandDelegate::CreateStatic(&Build));
	VerifyCommand = MakeUnique<FAutoConsoleCommand>(TEXT("RD.Editor.VerifyRewardSettlement"),
		TEXT("Verify the independent responsive reward settlement WBP."),
		FConsoleCommandDelegate::CreateStatic(&Verify));
}

void UnregisterRewardSettlementWidgetBuilderCommands()
{
	RewardSettlementWidgetBuilder::BuildCommand.Reset();
	RewardSettlementWidgetBuilder::VerifyCommand.Reset();
}
