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
	constexpr TCHAR AssetName[] = TEXT("WBP_RewardSettlement_V3");
	constexpr TCHAR AssetPath[] = TEXT("/Game/UI/RewardSettlement/WBP_RewardSettlement_V3.WBP_RewardSettlement_V3");
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

	UImage* AddColorImage(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent, const FName Name,
		const FLinearColor& Color, ESlateBrushDrawType::Type DrawAs,
		const FVector2D Position, const FVector2D Size, int32 ZOrder,
		const FMargin& Margin = FMargin(0.f))
	{
		UImage* Image = Blueprint->WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
		FSlateBrush Brush;
		Brush.DrawAs = DrawAs;
		Brush.TintColor = FSlateColor(Color);
		Brush.ImageSize = Size;
		Brush.Margin = Margin;
		Image->SetBrush(Brush);
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

	void BuildLegacy()
	{
		UE_LOG(LogTemp, Display, TEXT("RD_REWARD_SETTLEMENT_BUILD begin"));
		// Resolve every new hard dependency before touching the existing WidgetTree.
		// A missing import must fail without leaving the currently open asset empty.
		UTexture2D* HeaderBlank = Texture(TEXT("/Game/UI/ResultBoards/Art/T_VR_HeaderBlank_0809.T_VR_HeaderBlank_0809"));
		UTexture2D* PanelBlank = Texture(TEXT("/Game/UI/ResultBoards/Art/T_VR_PanelBlank_0809.T_VR_PanelBlank_0809"));
		UTexture2D* TabBlank = Texture(TEXT("/Game/UI/ResultBoards/Art/T_VR_TabBlank_0809.T_VR_TabBlank_0809"));
		UTexture2D* PrimaryButtonBlank = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/T_RS_Generated_PrimaryButton.T_RS_Generated_PrimaryButton"));
		UTexture2D* PortraitFrame = Texture(TEXT("/Game/UI/ResultBoards/Art/T_VR_PortraitFrame_0809.T_VR_PortraitFrame_0809"));
		UTexture2D* ProgressTrack = Texture(TEXT("/Game/UI/ResultBoards/Art/T_VR_ProgressTrack_0809.T_VR_ProgressTrack_0809"));
		UTexture2D* ProgressFill = Texture(TEXT("/Game/UI/ResultBoards/Art/T_VR_ProgressFill_0809.T_VR_ProgressFill_0809"));
		UTexture2D* XPTicket = Texture(TEXT("/Game/UI/ResultBoards/Art/T_VR_XPTicketBlank_0809.T_VR_XPTicketBlank_0809"));
		UTexture2D* ChoiceCard = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/T_RS_Generated_ArtifactCardSelected.T_RS_Generated_ArtifactCardSelected"));
		UTexture2D* ChestClosed = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/T_RS_Generated_ChestClosed.T_RS_Generated_ChestClosed"));
		UTexture2D* ChestRevealBurst = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/T_RS_Generated_ChestRevealBurst.T_RS_Generated_ChestRevealBurst"));
		UTexture2D* GoldCoin = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Reward/T_Reward_GoldIcon_V1.T_Reward_GoldIcon_V1"));
		UTexture2D* EquipmentIcon = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Equipment/T_equip_weapon_common.T_equip_weapon_common"));
		UTexture2D* ExpIcon = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/RewardV4_11/Tex/T_reward_v4_exp_icon.T_reward_v4_exp_icon"));
		UTexture2D* ChoiceGoldIcon = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/RewardV4_11/Tex/T_reward_v4_gold_icon.T_reward_v4_gold_icon"));
		UTexture2D* Knight = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Knight.T_MB_HireIcon_Knight"));
		UTexture2D* Mage = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Mage.T_MB_HireIcon_Mage"));
		UTexture2D* Rogue = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Rogue.T_MB_HireIcon_Rogue"));
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
		AddAspectImage(Blueprint, DesignCanvas, TEXT("StepCoinArt_3"), TabBlank, nullptr,
			FVector2D(880.f, 145.f), FVector2D(76.f, 76.f), 22);
		AddText(Blueprint, DesignCanvas, TEXT("StepCoinNumber_1"), FText::AsNumber(1),
			31, FVector2D(704.f, 151.f),
			FVector2D(76.f, 58.f), 23);
		AddText(Blueprint, DesignCanvas, TEXT("StepCoinNumber_2"), FText::AsNumber(2),
			31, FVector2D(792.f, 151.f),
			FVector2D(76.f, 58.f), 23);
		AddText(Blueprint, DesignCanvas, TEXT("StepCoinNumber_3"), FText::AsNumber(3),
			31, FVector2D(880.f, 151.f),
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
			Place(ResultStep, Row, FVector2D(18.f, 44.f + 122.f * Index),
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

		UCanvasPanel* ChestStep = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("SettlementChestStep"));
		ChestStep->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		StepSwitcher->AddChild(ChestStep);

		UImage* ChestBurst = AddAspectImage(Blueprint, ChestStep,
			TEXT("SettlementChestBurst"), ChestRevealBurst, nullptr,
			FVector2D(236.f, -8.f), FVector2D(400.f, 400.f), 1);
		ChestBurst->SetVisibility(ESlateVisibility::Collapsed);
		AddScaleImage(Blueprint, ChestStep, TEXT("SettlementChestFit"),
			TEXT("SettlementChestArt"), ChestClosed,
			FVector2D(211.f, 28.f), FVector2D(450.f, 320.f), 2);
		AddText(Blueprint, ChestStep, TEXT("SettlementChestHint"),
			NSLOCTEXT("RewardSettlement", "TouchChest", "상자를 터치하세요"), 31,
			FVector2D(236.f, 354.f), FVector2D(400.f, 60.f), 3);
		UButton* ChestButton = Blueprint->WidgetTree->ConstructWidget<UButton>(
			UButton::StaticClass(), TEXT("SettlementChestButton"));
		FButtonStyle ChestButtonStyle;
		FSlateBrush ChestButtonEmpty;
		ChestButtonEmpty.DrawAs = ESlateBrushDrawType::NoDrawType;
		ChestButtonStyle.SetNormal(ChestButtonEmpty);
		ChestButtonStyle.SetHovered(ChestButtonEmpty);
		ChestButtonStyle.SetPressed(ChestButtonEmpty);
		ChestButtonStyle.SetDisabled(ChestButtonEmpty);
		ChestButton->SetStyle(ChestButtonStyle);
		Place(ChestStep, ChestButton, FVector2D(211.f, 28.f),
			FVector2D(450.f, 380.f), 5);

		UCanvasPanel* ChoiceStep = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("SettlementChoiceStep"));
		ChoiceStep->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		StepSwitcher->AddChild(ChoiceStep);
		StepSwitcher->SetActiveWidgetIndex(0);

		AddAspectImage(Blueprint, ChoiceStep, TEXT("SettlementGoldCoin"), GoldCoin, nullptr,
			FVector2D(322.f, -8.f), FVector2D(66.f, 66.f), 3);
		AddText(Blueprint, ChoiceStep, TEXT("SettlementGoldGain"),
			NSLOCTEXT("RewardSettlement", "GoldPreview", "+22"), 31,
			FVector2D(398.f, -8.f), FVector2D(160.f, 66.f), 3);

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
			Place(ChoiceStep, Mount, FVector2D(ChoiceXs[Index], 62.f),
				FVector2D(236.f, 330.f), 1);

			const FVector2D CardBounds(236.f, 330.f);
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
			Place(Mount, PickButton, FVector2D::ZeroVector, FVector2D(236.f, 330.f), 5);
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

	void VerifyLegacy()
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
			TEXT("SettlementChestStep"), TEXT("SettlementChoiceStep"), TEXT("NextButtonHolder")
		};
		for (const FName Name : CanvasNames)
		{
			RequireClass(Name, UCanvasPanel::StaticClass());
		}

		const FName ImageNames[] = {
			TEXT("HeaderBlankArt"), TEXT("MainPanelArt"),
			TEXT("StepCoinArt_1"), TEXT("StepCoinArt_2"), TEXT("StepCoinArt_3"),
			TEXT("SettlementChestArt"), TEXT("SettlementChestBurst"),
			TEXT("SettlementGoldCoin"), TEXT("NextButtonArt")
		};
		for (const FName Name : ImageNames)
		{
			RequireClass(Name, UImage::StaticClass());
		}

		const FName TextNames[] = {
			TEXT("mTitleText"), TEXT("StepCoinNumber_1"), TEXT("StepCoinNumber_2"), TEXT("StepCoinNumber_3"),
			TEXT("SettlementChestHint"),
			TEXT("SettlementGoldGain"), TEXT("mGoldBalanceText"), TEXT("mNextButtonText")
		};
		for (const FName Name : TextNames)
		{
			RequireClass(Name, UTextBlock::StaticClass());
		}

		RequireClass(TEXT("mSummaryRowsBox"), UVerticalBox::StaticClass());
		RequireClass(TEXT("mMercenaryRowsBox"), UVerticalBox::StaticClass());
		RequireClass(TEXT("mNextButton"), UButton::StaticClass());
		RequireClass(TEXT("SettlementChestButton"), UButton::StaticClass());
		RequireClass(TEXT("SettlementChestFit"), UScaleBox::StaticClass());

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
		UCanvasPanel* ChestStep = CastChecked<UCanvasPanel>(
			Blueprint->WidgetTree->FindWidget(TEXT("SettlementChestStep")));
		UWidgetSwitcher* StepSwitcher = CastChecked<UWidgetSwitcher>(
			Blueprint->WidgetTree->FindWidget(TEXT("SettlementStepSwitcher")));
		checkf(ResultStep->GetVisibility() == ESlateVisibility::SelfHitTestInvisible,
			TEXT("Reward settlement result step must be visible by default"));
		checkf(ChoiceStep->GetVisibility() == ESlateVisibility::SelfHitTestInvisible,
			TEXT("Reward settlement choice step must remain designer-visible"));
		checkf(ChestStep->GetVisibility() == ESlateVisibility::SelfHitTestInvisible,
			TEXT("Reward settlement chest step must remain designer-visible"));
		checkf(StepSwitcher->GetActiveWidgetIndex() == 0
			&& StepSwitcher->GetWidgetAtIndex(0) == ResultStep
			&& StepSwitcher->GetWidgetAtIndex(1) == ChestStep
			&& StepSwitcher->GetWidgetAtIndex(2) == ChoiceStep,
			TEXT("Reward settlement step switcher has the wrong child order/default"));
		checkf(Blueprint->GeneratedClass != nullptr
			&& Blueprint->GeneratedClass->IsChildOf(URewardSettlementWidgetBase::StaticClass()),
			TEXT("Reward settlement generated class is invalid"));
		UE_LOG(LogTemp, Display,
			TEXT("RD_REWARD_SETTLEMENT_VERIFY success widgets=%d parent=%s"),
			VerifiedWidgetCount, *Blueprint->ParentClass->GetName());
	}

	void ApplyTransparentButtonStyle(UButton* Button)
	{
		FButtonStyle Style;
		FSlateBrush Empty;
		Empty.DrawAs = ESlateBrushDrawType::NoDrawType;
		Style.SetNormal(Empty);
		Style.SetHovered(Empty);
		Style.SetPressed(Empty);
		Style.SetDisabled(Empty);
		Button->SetStyle(Style);
	}

	/**
	 * 완전히 새로 만든 V3 보상판. 외곽/본문/행/배지/카드 상태를 독립 파츠로
	 * 조립해 UMG가 각 내부 영역과 상태를 직접 제어한다.
	 */
	void Build()
	{
		UE_LOG(LogTemp, Display, TEXT("RD_REWARD_SETTLEMENT_V3_BUILD begin"));
		// concept03 시안에서 직접 분해·정제한 목재/황동/철판 파츠다. 글자와
		// 값은 이미지에 굽지 않고 UMG 위젯으로 유지한다.
		UTexture2D* BoardInterior = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_BoardInterior.T_C03_BoardInterior"));
		UTexture2D* RailH = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_RailH.T_C03_RailH"));
		UTexture2D* RailVLeft = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_RailVLeft.T_C03_RailVLeft"));
		UTexture2D* RailVRight = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_RailVRight.T_C03_RailVRight"));
		UTexture2D* CornerTL = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_CornerTL.T_C03_CornerTL"));
		UTexture2D* CornerTR = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_CornerTR.T_C03_CornerTR"));
		UTexture2D* CornerBL = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_CornerBL.T_C03_CornerBL"));
		UTexture2D* CornerBR = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_CornerBR.T_C03_CornerBR"));
		UTexture2D* HeaderBackground = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_TitlePlate.T_C03_TitlePlate"));
		UTexture2D* StepBackground = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_StageTab.T_C03_StageTab"));
		UTexture2D* StepTrack = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_StepBarTrack.T_C03_StepBarTrack"));
		UTexture2D* StepFill = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_StepBarFill.T_C03_StepBarFill"));
		UTexture2D* StepCoinActive = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_StepCoinActive.T_C03_StepCoinActive"));
		UTexture2D* StepCoinInactive = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_StepCoinInactive.T_C03_StepCoinInactive"));
		UTexture2D* CtaBackground = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_CtaPlate.T_C03_CtaPlate"));
		UTexture2D* ParchmentWindow = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_ParchWindow.T_C03_ParchWindow"));
		UTexture2D* ExpProgressTrack = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_TrackPlate.T_C03_TrackPlate"));
		UTexture2D* ProgressFill = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_TrackFill.T_C03_TrackFill"));
		UTexture2D* ExpXpBadge = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_XpBadge.T_C03_XpBadge"));
		UTexture2D* CardFrame = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_CardBlank.T_C03_CardBlank"));
		UTexture2D* CardSelectedOverlay = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_SelectionGlow.T_C03_SelectionGlow"));
		UTexture2D* ChestClosed = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/V8PartsV2/T_RS_V8_ChestClosed.T_RS_V8_ChestClosed"));
		UTexture2D* ChestAura = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/V8PartsV2/T_RS_V8_ChestAura.T_RS_V8_ChestAura"));
		UTexture2D* ChestBurst = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/V8PartsV2/T_RS_V8_ChestBurst.T_RS_V8_ChestBurst"));
		UTexture2D* GoldCoin = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_GoldCoinVisualGenerated.T_C03_GoldCoinVisualGenerated"));
		UTexture2D* Equipment = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Equipment/T_equip_weapon_common.T_equip_weapon_common"));
		UTexture2D* PreviewArtifacts[] = {
			Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Artifacts/T_Artifact_BloodChalice.T_Artifact_BloodChalice")),
			Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Artifacts/T_Artifact_FangAmulet.T_Artifact_FangAmulet")),
			Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Artifacts/T_Artifact_LuckyCoin.T_Artifact_LuckyCoin"))
		};
		UTexture2D* Portraits[] = {
			Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Knight.T_MB_HireIcon_Knight")),
			Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Mage.T_MB_HireIcon_Mage")),
			Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Rogue.T_MB_HireIcon_Rogue"))
		};

		UWidgetBlueprint* Blueprint = FindOrCreateBlueprint();
		checkf(Blueprint != nullptr && Blueprint->WidgetTree != nullptr,
			TEXT("Could not create V3 reward WBP"));
		Blueprint->Modify();
		Blueprint->WidgetTree->Modify();
		if (UWidget* PreviousRoot = Blueprint->WidgetTree->RootWidget)
		{
			Blueprint->ParentClass = UUserWidget::StaticClass();
			TSet<UWidget*> PreviousWidgets;
			PreviousWidgets.Add(PreviousRoot);
			FWidgetBlueprintEditorUtils::DeleteWidgets(Blueprint, MoveTemp(PreviousWidgets),
				FWidgetBlueprintEditorUtils::EDeleteWidgetWarningType::DeleteSilently);
		}
		Blueprint->ParentClass = URewardSettlementWidgetBase::StaticClass();

		UOverlay* Root = Blueprint->WidgetTree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(), TEXT("SettlementViewportRoot"));
		Blueprint->WidgetTree->RootWidget = Root;
		UBorder* Dimmer = Blueprint->WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("WorldDimmer"));
		Dimmer->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, .58f));
		Root->AddChildToOverlay(Dimmer);
		CastChecked<UOverlaySlot>(Dimmer->Slot)->SetHorizontalAlignment(HAlign_Fill);
		CastChecked<UOverlaySlot>(Dimmer->Slot)->SetVerticalAlignment(VAlign_Fill);

		UCanvasPanel* Screen = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("SettlementResponsiveCanvas"));
		Root->AddChildToOverlay(Screen);
		CastChecked<UOverlaySlot>(Screen->Slot)->SetHorizontalAlignment(HAlign_Fill);
		CastChecked<UOverlaySlot>(Screen->Slot)->SetVerticalAlignment(VAlign_Fill);
		UScaleBox* Scale = Blueprint->WidgetTree->ConstructWidget<UScaleBox>(
			UScaleBox::StaticClass(), TEXT("SettlementMasterScale"));
		Scale->SetStretch(EStretch::ScaleToFit);
		Scale->SetStretchDirection(EStretchDirection::Both);
		Scale->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		Anchor(Screen, Scale, FAnchors(0.f, 0.f, 1.f, 1.f), FMargin(0.f), 1);
		USizeBox* DesignSize = Blueprint->WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("SettlementMasterDesignSize"));
		DesignSize->SetWidthOverride(1536.f);
		DesignSize->SetHeightOverride(864.f);
		Scale->AddChild(DesignSize);
		UCanvasPanel* Design = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("SettlementDesignCanvas"));
		DesignSize->SetContent(Design);

		// concept03은 전투 화면 자체를 배경으로 쓰고 그 위에 보상판만 띄운다.
		// WorldDimmer 아래의 실제 전투 장면은 그대로 보존하며, 셸에는 시안의
		// 목재/황동 프레임만 조립한다.
		UCanvasPanel* Shell = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("SettlementModalShell"));
		Place(Design, Shell, FVector2D::ZeroVector, FVector2D(1536.f, 864.f), 0);
		UImage* ModalBackgroundContract = AddImage(Blueprint, Shell,
			TEXT("SettlementModalBackgroundArt"), BoardInterior, nullptr,
			FVector2D::ZeroVector, FVector2D(1.f), 0);
		ModalBackgroundContract->SetVisibility(ESlateVisibility::Collapsed);
		UImage* ModalFrameContract = AddImage(Blueprint, Shell,
			TEXT("SettlementModalOuterFrameArt"), HeaderBackground, nullptr,
			FVector2D::ZeroVector, FVector2D(566.f, 136.f), 0);
		ModalFrameContract->SetVisibility(ESlateVisibility::Collapsed);

		UCanvasPanel* HeaderSection = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("SettlementHeaderSection"));
		Place(Shell, HeaderSection, FVector2D(485.f, 26.f), FVector2D(566.f, 136.f), 5);
		AddImage(Blueprint, HeaderSection, TEXT("SettlementHeaderBackgroundArt"), HeaderBackground,
			nullptr, FVector2D::ZeroVector, FVector2D(566.f, 136.f), 0);
		UImage* HeaderFrameArt = AddImage(Blueprint, HeaderSection,
			TEXT("SettlementHeaderFrameArt"), HeaderBackground,
			nullptr, FVector2D::ZeroVector, FVector2D(566.f, 136.f), 1);
		HeaderFrameArt->SetVisibility(ESlateVisibility::Collapsed);
		AddText(Blueprint, HeaderSection, TEXT("mTitleText"),
			NSLOCTEXT("RewardSettlement", "TitleC03", "전투 보상"), 42,
			FVector2D(60.f, 40.f), FVector2D(446.f, 60.f), 2);

		UCanvasPanel* BodySection = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("SettlementBodySection"));
		Place(Shell, BodySection, FVector2D(98.f, 258.f), FVector2D(1326.f, 482.f), 1);
		AddImage(Blueprint, BodySection, TEXT("SettlementBodyBackgroundArt"), BoardInterior,
			nullptr, FVector2D(20.f, 20.f), FVector2D(1282.f, 442.f), 0);
		UImage* BodyFrameArt = AddImage(Blueprint, BodySection,
			TEXT("SettlementBodyFrameArt"), BoardInterior,
			nullptr, FVector2D::ZeroVector, FVector2D(1.f), 1);
		BodyFrameArt->SetVisibility(ESlateVisibility::Collapsed);
		AddImage(Blueprint, BodySection, TEXT("SettlementRailTop"), RailH, nullptr,
			FVector2D::ZeroVector, FVector2D(1326.f, 44.f), 1);
		AddImage(Blueprint, BodySection, TEXT("SettlementRailBottom"), RailH, nullptr,
			FVector2D(0.f, 438.f), FVector2D(1326.f, 44.f), 1);
		AddImage(Blueprint, BodySection, TEXT("SettlementRailLeft"), RailVLeft, nullptr,
			FVector2D::ZeroVector, FVector2D(44.f, 482.f), 1);
		AddImage(Blueprint, BodySection, TEXT("SettlementRailRight"), RailVRight, nullptr,
			FVector2D(1282.f, 0.f), FVector2D(44.f, 482.f), 1);
		AddImage(Blueprint, BodySection, TEXT("SettlementCornerTL"), CornerTL, nullptr,
			FVector2D::ZeroVector, FVector2D(92.f, 92.f), 2);
		AddImage(Blueprint, BodySection, TEXT("SettlementCornerTR"), CornerTR, nullptr,
			FVector2D(1234.f, 0.f), FVector2D(92.f, 92.f), 2);
		AddImage(Blueprint, BodySection, TEXT("SettlementCornerBL"), CornerBL, nullptr,
			FVector2D(0.f, 390.f), FVector2D(92.f, 92.f), 2);
		AddImage(Blueprint, BodySection, TEXT("SettlementCornerBR"), CornerBR, nullptr,
			FVector2D(1234.f, 390.f), FVector2D(92.f, 92.f), 2);

		UCanvasPanel* StepBadge = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("SettlementStepBadgeMount"));
		Place(Shell, StepBadge, FVector2D(98.f, 276.f), FVector2D(196.f, 48.f), 6);
		AddImage(Blueprint, StepBadge, TEXT("SettlementStepBackgroundArt"),
			StepBackground, nullptr, FVector2D::ZeroVector, FVector2D(196.f, 48.f), 0);
		UImage* StepFrameArt = AddImage(Blueprint, StepBadge, TEXT("SettlementStepFrameArt"),
			StepBackground, nullptr, FVector2D::ZeroVector, FVector2D(196.f, 48.f), 1);
		StepFrameArt->SetVisibility(ESlateVisibility::Collapsed);
		AddText(Blueprint, StepBadge, TEXT("SettlementStepText"),
			NSLOCTEXT("RewardSettlement", "ExpStepC03", "경험치"), 20,
			FVector2D(10.f, 5.f), FVector2D(176.f, 38.f), 2);

		AddImage(Blueprint, Shell, TEXT("SettlementStepTrackArt"), StepTrack, nullptr,
			FVector2D(423.f, 258.f), FVector2D(690.f, 22.f), 4);
		UCanvasPanel* StepFillClip = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("SettlementStepFillClip"));
		StepFillClip->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		Place(Shell, StepFillClip, FVector2D(430.f, 261.f), FVector2D(100.f, 16.f), 5);
		AddImage(Blueprint, StepFillClip, TEXT("SettlementStepFillArt"), StepFill, nullptr,
			FVector2D::ZeroVector, FVector2D(676.f, 16.f), 0);
		const float StepCenters[] = { 530.f, 706.f, 882.f, 1058.f };
		for (int32 Index = 0; Index < 4; ++Index)
		{
			const bool bActive = Index == 0;
			const float Extent = bActive ? 92.f : 64.f;
			const FVector2D CoinPosition(StepCenters[Index] - Extent * .5f,
				269.f - Extent * .5f);
			AddImage(Blueprint, Shell,
				*FString::Printf(TEXT("SettlementStepCoin_%d"), Index),
				bActive ? StepCoinActive : StepCoinInactive, nullptr,
				CoinPosition, FVector2D(Extent, Extent), 7);
			AddText(Blueprint, Shell,
				*FString::Printf(TEXT("SettlementStepCoinText_%d"), Index),
				FText::AsNumber(Index + 1), bActive ? 28 : 24,
				FVector2D(StepCenters[Index] - 30.f, 245.f), FVector2D(60.f, 48.f), 8);
		}

		UCanvasPanel* Content = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("ExpCanvas"));
		Place(BodySection, Content, FVector2D(60.f, 72.f), FVector2D(1206.f, 350.f), 3);
		UWidgetSwitcher* Switcher = Blueprint->WidgetTree->ConstructWidget<UWidgetSwitcher>(
			UWidgetSwitcher::StaticClass(), TEXT("SettlementStepSwitcher"));
		Place(Content, Switcher, FVector2D::ZeroVector, FVector2D(1206.f, 350.f), 0);

		UCanvasPanel* Result = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("SettlementResultStep"));
		Result->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Switcher->AddChild(Result);
		for (int32 Index = 0; Index < 3; ++Index)
		{
			UCanvasPanel* Row = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
				UCanvasPanel::StaticClass(), *FString::Printf(TEXT("SettlementExpRow_%d"), Index));
			Row->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			Place(Result, Row, FVector2D(38.f, 4.f + Index * 110.f),
				FVector2D(620.f, 104.f), 1);
			UImage* RowPlateContract = AddImage(Blueprint, Row,
				*FString::Printf(TEXT("SettlementExpRowPlate_%d"), Index), BoardInterior,
				nullptr, FVector2D::ZeroVector, FVector2D(1.f), 0);
			RowPlateContract->SetVisibility(ESlateVisibility::Collapsed);
			UImage* RingContract = AddImage(Blueprint, Row,
				*FString::Printf(TEXT("SettlementExpPortraitRing_%d"), Index), StepCoinInactive,
				nullptr, FVector2D::ZeroVector, FVector2D(1.f), 0);
			RingContract->SetVisibility(ESlateVisibility::Collapsed);
			UImage* LevelContract = AddImage(Blueprint, Row,
				*FString::Printf(TEXT("SettlementExpLevelWindow_%d"), Index), StepBackground,
				nullptr, FVector2D::ZeroVector, FVector2D(1.f), 0);
			LevelContract->SetVisibility(ESlateVisibility::Collapsed);
			AddImage(Blueprint, Row, *FString::Printf(TEXT("SettlementExpProgressTrack_%d"), Index),
				ExpProgressTrack, nullptr, FVector2D(126.f, 30.f), FVector2D(322.f, 44.f), 1);
			UImage* BadgeContract = AddImage(Blueprint, Row,
				*FString::Printf(TEXT("SettlementExpXpBadge_%d"), Index), ExpXpBadge,
				nullptr, FVector2D::ZeroVector, FVector2D(1.f), 0);
			BadgeContract->SetVisibility(ESlateVisibility::Collapsed);
			AddScaleImage(Blueprint, Row,
				*FString::Printf(TEXT("SettlementPortraitFit_%d"), Index),
				*FString::Printf(TEXT("SettlementPortrait_%d"), Index), Portraits[Index],
				FVector2D(8.f, 4.f), FVector2D(96.f, 96.f), 2);
			AddText(Blueprint, Row,
				*FString::Printf(TEXT("SettlementMercenaryLevel_%d"), Index),
				FText::FromString(FString::Printf(TEXT("Lv.%d"), Index + 1)), 18,
				FVector2D(456.f, 12.f), FVector2D(72.f, 34.f), 3);
			UCanvasPanel* Clip = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
				UCanvasPanel::StaticClass(),
				*FString::Printf(TEXT("SettlementMercenaryBarClip_%d"), Index));
			Clip->SetClipping(EWidgetClipping::ClipToBoundsAlways);
			Clip->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			Place(Row, Clip, FVector2D(134.f, 38.f), FVector2D(300.f, 28.f), 2);
			AddImage(Blueprint, Clip,
				*FString::Printf(TEXT("SettlementMercenaryBar_%d"), Index),
				ProgressFill, nullptr, FVector2D::ZeroVector, FVector2D(300.f, 28.f), 0);
			AddText(Blueprint, Row,
				*FString::Printf(TEXT("SettlementMercenaryBarText_%d"), Index),
				FText::FromString(TEXT("50 / 250")), 16,
				FVector2D(134.f, 34.f), FVector2D(300.f, 36.f), 4);
			AddText(Blueprint, Row,
				*FString::Printf(TEXT("SettlementXPText_%d"), Index),
				NSLOCTEXT("RewardSettlement", "XPPreviewC03", "+50 XP"), 18,
				FVector2D(456.f, 52.f), FVector2D(112.f, 38.f), 4);
		}
		AddImage(Blueprint, Result, TEXT("SettlementXpSummaryWindow"), ParchmentWindow,
			nullptr, FVector2D(730.f, 24.f), FVector2D(446.f, 286.f), 1);
		AddAspectImage(Blueprint, Result, TEXT("SettlementXpSummaryBadge"), ExpXpBadge,
			nullptr, FVector2D(865.f, 42.f), FVector2D(176.f, 150.f), 2);
		AddText(Blueprint, Result, TEXT("SettlementXpSummaryAmount"),
			NSLOCTEXT("RewardSettlement", "XPSummaryC03", "+50 XP"), 38,
			FVector2D(808.f, 184.f), FVector2D(290.f, 52.f), 3);
		AddText(Blueprint, Result, TEXT("SettlementXpSummaryLevel"),
			NSLOCTEXT("RewardSettlement", "XPLevelC03", "Lv.5  ->  Lv.6"), 24,
			FVector2D(808.f, 234.f), FVector2D(290.f, 42.f), 3);

		UCanvasPanel* Chest = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("SettlementChestStep"));
		Chest->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Switcher->AddChild(Chest);
		UImage* Aura = AddAspectImage(Blueprint, Chest, TEXT("SettlementChestAura"),
			ChestAura, nullptr, FVector2D(80.f, -28.f), FVector2D(440.f, 420.f), 0);
		Aura->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Aura->SetRenderOpacity(.12f);
		UImage* Burst = AddAspectImage(Blueprint, Chest, TEXT("SettlementChestBurst"),
			ChestBurst, nullptr, FVector2D(110.f, -8.f), FVector2D(390.f, 390.f), 1);
		Burst->SetVisibility(ESlateVisibility::Collapsed);
		AddScaleImage(Blueprint, Chest, TEXT("SettlementChestFit"), TEXT("SettlementChestArt"),
			ChestClosed, FVector2D(130.f, 22.f), FVector2D(350.f, 286.f), 2);
		AddImage(Blueprint, Chest, TEXT("SettlementChestHintWindow"), ParchmentWindow,
			nullptr, FVector2D(694.f, 32.f), FVector2D(446.f, 286.f), 1);
		AddText(Blueprint, Chest, TEXT("SettlementChestHint"),
			NSLOCTEXT("RewardSettlement", "TouchChestC03", "상자를 눌러 여세요"), 28,
			FVector2D(738.f, 138.f), FVector2D(358.f, 58.f), 3);
		UButton* ChestButton = Blueprint->WidgetTree->ConstructWidget<UButton>(
			UButton::StaticClass(), TEXT("SettlementChestButton"));
		ApplyTransparentButtonStyle(ChestButton);
		Place(Chest, ChestButton, FVector2D(92.f, 0.f), FVector2D(440.f, 330.f), 5);

		// 단계 3은 골드 전용 화면이다. 아티팩트가 없는 방은 이 화면에서 끝나고,
		// 아티팩트가 있는 방만 별도 단계 4로 넘어간다.
		UCanvasPanel* Gold = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("SettlementGoldStep"));
		Gold->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Switcher->AddChild(Gold);
		UTextBlock* GoldTitle = AddText(Blueprint, Gold, TEXT("SettlementGoldRewardTitle"),
			FText::GetEmpty(), 1, FVector2D::ZeroVector, FVector2D(1.f), 0);
		GoldTitle->SetVisibility(ESlateVisibility::Collapsed);
		UCanvasPanel* GoldPanel = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("SettlementGuaranteedGoldPanel"));
		GoldPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Place(Gold, GoldPanel, FVector2D::ZeroVector, FVector2D(1206.f, 350.f), 2);
		UImage* GoldPanelContract = AddImage(Blueprint, GoldPanel,
			TEXT("SettlementGoldPanelPlateArt"), BoardInterior, nullptr,
			FVector2D::ZeroVector, FVector2D(1.f), 0);
		GoldPanelContract->SetVisibility(ESlateVisibility::Collapsed);
		UImage* GoldRingContract = AddImage(Blueprint, GoldPanel,
			TEXT("SettlementGoldCoinRingArt"), StepCoinActive, nullptr,
			FVector2D::ZeroVector, FVector2D(1.f), 0);
		GoldRingContract->SetVisibility(ESlateVisibility::Collapsed);
		AddImage(Blueprint, GoldPanel, TEXT("SettlementGoldAmountWindowArt"),
			ParchmentWindow, nullptr, FVector2D(704.f, 32.f), FVector2D(446.f, 286.f), 1);
		AddAspectImage(Blueprint, GoldPanel, TEXT("SettlementGoldCoin"), GoldCoin, nullptr,
			FVector2D(150.f, 24.f), FVector2D(300.f, 300.f), 2);
		AddText(Blueprint, GoldPanel, TEXT("SettlementGuaranteedGoldLabel"),
			NSLOCTEXT("RewardSettlement", "GuaranteedGoldC03", "획득 골드"), 30,
			FVector2D(754.f, 92.f), FVector2D(346.f, 54.f), 3);
		AddText(Blueprint, GoldPanel, TEXT("SettlementGoldGain"),
			NSLOCTEXT("RewardSettlement", "GoldPreviewC03", "+350 G"), 48,
			FVector2D(754.f, 158.f), FVector2D(346.f, 76.f), 3);
		AddText(Blueprint, Gold, TEXT("SettlementGoldContinueHint"),
			FText::GetEmpty(), 1, FVector2D::ZeroVector, FVector2D(1.f), 0)->
			SetVisibility(ESlateVisibility::Collapsed);

		// 단계 4는 룸 데이터가 가진 아티팩트 후보만 표시한다. 골드 위젯은
		// 부모부터 완전히 다른 switcher 자식이므로 이 화면에 섞일 수 없다.
		UCanvasPanel* Choice = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("SettlementChoiceStep"));
		Choice->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Switcher->AddChild(Choice);
		Switcher->SetActiveWidgetIndex(0);
		UTextBlock* ChoiceTitle = AddText(Blueprint, Choice,
			TEXT("SettlementArtifactChoiceTitle"), FText::GetEmpty(), 1,
			FVector2D::ZeroVector, FVector2D(1.f), 0);
		ChoiceTitle->SetVisibility(ESlateVisibility::Collapsed);
		UTexture2D* ChoiceIcons[] = {
			PreviewArtifacts[0] != nullptr ? PreviewArtifacts[0] : Equipment,
			PreviewArtifacts[1] != nullptr ? PreviewArtifacts[1] : Equipment,
			PreviewArtifacts[2] != nullptr ? PreviewArtifacts[2] : Equipment
		};
		const FText ChoiceNames[] = {
			NSLOCTEXT("RewardSettlement", "ArtifactA_V5", "피의 성배"),
			NSLOCTEXT("RewardSettlement", "ArtifactB_V5", "야수의 송곳니"),
			NSLOCTEXT("RewardSettlement", "ArtifactC_V5", "행운의 주화") };
		const float ChoiceX[] = { 98.f, 458.f, 818.f };
		for (int32 Index = 0; Index < 3; ++Index)
		{
			UCanvasPanel* Mount = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
				UCanvasPanel::StaticClass(), *FString::Printf(TEXT("SettlementChoiceMount_%d"), Index));
			Mount->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			Place(Choice, Mount, FVector2D(ChoiceX[Index], 10.f), FVector2D(302.f, 338.f), 1);
			UImage* BackingContract = AddImage(Blueprint, Mount,
				*FString::Printf(TEXT("SettlementCardBackground_%d"), Index), CardFrame,
				nullptr, FVector2D::ZeroVector, FVector2D(1.f), 0);
			BackingContract->SetVisibility(ESlateVisibility::Collapsed);
			AddImage(Blueprint, Mount,
				*FString::Printf(TEXT("SettlementCardFrame_%d"), Index), CardFrame, nullptr,
				FVector2D(6.f, 6.f), FVector2D(290.f, 326.f), 1);
			AddScaleImage(Blueprint, Mount,
				*FString::Printf(TEXT("SettlementChoiceIconFit_%d"), Index),
				*FString::Printf(TEXT("SettlementChoiceIcon_%d"), Index), ChoiceIcons[Index],
				FVector2D(64.f, 104.f), FVector2D(174.f, 174.f), 2);
			UImage* NamePlateContract = AddImage(Blueprint, Mount,
				*FString::Printf(TEXT("SettlementCardNamePlate_%d"), Index), StepBackground,
				nullptr, FVector2D::ZeroVector, FVector2D(1.f), 0);
			NamePlateContract->SetVisibility(ESlateVisibility::Collapsed);
			AddText(Blueprint, Mount,
				*FString::Printf(TEXT("SettlementChoiceName_%d"), Index), ChoiceNames[Index], 20,
				FVector2D(30.f, 34.f), FVector2D(242.f, 52.f), 3);
			UImage* SelectedOverlay = AddImage(Blueprint, Mount,
				*FString::Printf(TEXT("SettlementCardSelectedOverlay_%d"), Index),
				CardSelectedOverlay, nullptr, FVector2D::ZeroVector, FVector2D(302.f, 338.f), 4);
			SelectedOverlay->SetVisibility(ESlateVisibility::Collapsed);
			UButton* Pick = Blueprint->WidgetTree->ConstructWidget<UButton>(
				UButton::StaticClass(), *FString::Printf(TEXT("SettlementChoiceButton_%d"), Index));
			ApplyTransparentButtonStyle(Pick);
			Place(Mount, Pick, FVector2D::ZeroVector, FVector2D(302.f, 338.f), 5);
		}

		// 기존 native BindWidget 계약은 숨은 빈 위젯으로만 유지한다.
		UTextBlock* GoldBalance = AddText(Blueprint, Design, TEXT("mGoldBalanceText"),
			FText::GetEmpty(), 1, FVector2D::ZeroVector, FVector2D(1.f), 0);
		GoldBalance->SetVisibility(ESlateVisibility::Collapsed);
		for (const FName Name : { FName(TEXT("mSummaryRowsBox")), FName(TEXT("mMercenaryRowsBox")) })
		{
			UVerticalBox* Box = Blueprint->WidgetTree->ConstructWidget<UVerticalBox>(
				UVerticalBox::StaticClass(), Name);
			Box->SetVisibility(ESlateVisibility::Collapsed);
			Place(Design, Box, FVector2D::ZeroVector, FVector2D(1.f), 0);
		}

		UCanvasPanel* NextHolder = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("NextButtonHolder"));
		// V9 원자 파츠 계약: CTA의 바탕과 장식 프레임도 별도 레이어다.
		Place(Shell, NextHolder, FVector2D(568.f, 696.f), FVector2D(400.f, 94.f), 6);
		AddImage(Blueprint, NextHolder, TEXT("NextButtonBackgroundArt"), CtaBackground, nullptr,
			FVector2D::ZeroVector, FVector2D(400.f, 94.f), 0);
		UImage* CtaFrameArt = AddImage(Blueprint, NextHolder, TEXT("NextButtonFrameArt"), CtaBackground,
			nullptr, FVector2D::ZeroVector, FVector2D(400.f, 94.f), 1);
		CtaFrameArt->SetVisibility(ESlateVisibility::Collapsed);
		AddText(Blueprint, NextHolder, TEXT("mNextButtonText"),
			NSLOCTEXT("RewardSettlement", "NextC03", "다음"), 30,
			FVector2D(40.f, 18.f), FVector2D(320.f, 58.f), 2);
		UButton* Next = Blueprint->WidgetTree->ConstructWidget<UButton>(
			UButton::StaticClass(), TEXT("mNextButton"));
		ApplyTransparentButtonStyle(Next);
		Place(NextHolder, Next, FVector2D::ZeroVector, FVector2D(400.f, 94.f), 5);

		Blueprint->WidgetTree->ForEachWidget([Blueprint](UWidget* Widget)
		{
			if (Widget != nullptr
				&& !Blueprint->WidgetVariableNameToGuidMap.Contains(Widget->GetFName()))
			{
				Blueprint->OnVariableAdded(Widget->GetFName());
			}
		});
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Blueprint->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
		checkf(UPackage::SavePackage(Blueprint->GetPackage(), Blueprint, *Filename,
			FSavePackageArgs()), TEXT("Could not save V3 reward WBP"));
		UE_LOG(LogTemp, Display, TEXT("RD_REWARD_SETTLEMENT_V3_BUILD success asset=%s"), AssetPath);
	}

	void Verify()
	{
		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, AssetPath);
		checkf(Blueprint != nullptr && Blueprint->WidgetTree != nullptr,
			TEXT("V3 reward settlement WBP is missing"));
		auto Require = [Blueprint](const FName Name, UClass* Expected)
		{
			UWidget* Widget = Blueprint->WidgetTree->FindWidget(Name);
			checkf(Widget != nullptr && Widget->IsA(Expected),
				TEXT("V3 reward widget %s is missing or has wrong type"), *Name.ToString());
		};
		Require(TEXT("SettlementModalBackgroundArt"), UImage::StaticClass());
		Require(TEXT("SettlementModalOuterFrameArt"), UImage::StaticClass());
		Require(TEXT("SettlementHeaderSection"), UCanvasPanel::StaticClass());
		Require(TEXT("SettlementHeaderBackgroundArt"), UImage::StaticClass());
		Require(TEXT("SettlementHeaderFrameArt"), UImage::StaticClass());
		Require(TEXT("SettlementBodySection"), UCanvasPanel::StaticClass());
		Require(TEXT("SettlementBodyBackgroundArt"), UImage::StaticClass());
		Require(TEXT("SettlementBodyFrameArt"), UImage::StaticClass());
		Require(TEXT("SettlementStepBadgeMount"), UCanvasPanel::StaticClass());
		Require(TEXT("SettlementStepBackgroundArt"), UImage::StaticClass());
		Require(TEXT("SettlementStepFrameArt"), UImage::StaticClass());
		Require(TEXT("SettlementStepText"), UTextBlock::StaticClass());
		Require(TEXT("SettlementStepSwitcher"), UWidgetSwitcher::StaticClass());
		Require(TEXT("SettlementChestAura"), UImage::StaticClass());
		Require(TEXT("SettlementChestArt"), UImage::StaticClass());
		Require(TEXT("SettlementChestButton"), UButton::StaticClass());
		Require(TEXT("SettlementGoldStep"), UCanvasPanel::StaticClass());
		Require(TEXT("SettlementGoldRewardTitle"), UTextBlock::StaticClass());
		Require(TEXT("SettlementGuaranteedGoldPanel"), UCanvasPanel::StaticClass());
		Require(TEXT("SettlementGoldPanelPlateArt"), UImage::StaticClass());
		Require(TEXT("SettlementGoldCoinRingArt"), UImage::StaticClass());
		Require(TEXT("SettlementGoldAmountWindowArt"), UImage::StaticClass());
		Require(TEXT("SettlementGuaranteedGoldLabel"), UTextBlock::StaticClass());
		Require(TEXT("SettlementGoldContinueHint"), UTextBlock::StaticClass());
		Require(TEXT("SettlementChoiceStep"), UCanvasPanel::StaticClass());
		Require(TEXT("SettlementArtifactChoiceTitle"), UTextBlock::StaticClass());
		Require(TEXT("NextButtonBackgroundArt"), UImage::StaticClass());
		Require(TEXT("NextButtonFrameArt"), UImage::StaticClass());
		Require(TEXT("mNextButton"), UButton::StaticClass());
		for (int32 Index = 0; Index < 3; ++Index)
		{
			Require(*FString::Printf(TEXT("SettlementExpRowPlate_%d"), Index), UImage::StaticClass());
			Require(*FString::Printf(TEXT("SettlementExpPortraitRing_%d"), Index), UImage::StaticClass());
			Require(*FString::Printf(TEXT("SettlementExpLevelWindow_%d"), Index), UImage::StaticClass());
			Require(*FString::Printf(TEXT("SettlementExpProgressTrack_%d"), Index), UImage::StaticClass());
			Require(*FString::Printf(TEXT("SettlementExpXpBadge_%d"), Index), UImage::StaticClass());
			Require(*FString::Printf(TEXT("SettlementCardBackground_%d"), Index), UImage::StaticClass());
			Require(*FString::Printf(TEXT("SettlementCardFrame_%d"), Index), UImage::StaticClass());
			Require(*FString::Printf(TEXT("SettlementCardNamePlate_%d"), Index), UImage::StaticClass());
			Require(*FString::Printf(TEXT("SettlementCardSelectedOverlay_%d"), Index), UImage::StaticClass());
		}
		auto RequireCanvasPlacement = [Blueprint](const FName Name,
			const FVector2D ExpectedPosition, const FVector2D ExpectedSize)
		{
			UWidget* Widget = Blueprint->WidgetTree->FindWidget(Name);
			UCanvasPanelSlot* Slot = Widget != nullptr
				? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr;
			checkf(Slot != nullptr
				&& Slot->GetPosition().Equals(ExpectedPosition, .01f)
				&& Slot->GetSize().Equals(ExpectedSize, .01f),
				TEXT("V10 single-master placement mismatch: %s"), *Name.ToString());
		};
		RequireCanvasPlacement(TEXT("SettlementHeaderSection"),
			FVector2D(485.f, 26.f), FVector2D(566.f, 136.f));
		RequireCanvasPlacement(TEXT("SettlementStepBadgeMount"),
			FVector2D(98.f, 276.f), FVector2D(196.f, 48.f));
		RequireCanvasPlacement(TEXT("SettlementBodySection"),
			FVector2D(98.f, 258.f), FVector2D(1326.f, 482.f));
		RequireCanvasPlacement(TEXT("NextButtonHolder"),
			FVector2D(568.f, 696.f), FVector2D(400.f, 94.f));
		checkf(Blueprint->WidgetTree->FindWidget(TEXT("HeaderBlankArt")) == nullptr
			&& Blueprint->WidgetTree->FindWidget(TEXT("MainPanelArt")) == nullptr
			&& Blueprint->WidgetTree->FindWidget(TEXT("StepCoinArt_1")) == nullptr
			&& Blueprint->WidgetTree->FindWidget(TEXT("SettlementFrameArt")) == nullptr
			&& Blueprint->WidgetTree->FindWidget(TEXT("SettlementHeaderPlateArt")) == nullptr
			&& Blueprint->WidgetTree->FindWidget(TEXT("SettlementContentPanelArt")) == nullptr
			&& Blueprint->WidgetTree->FindWidget(TEXT("SettlementStepBadgeArt")) == nullptr
			&& Blueprint->WidgetTree->FindWidget(TEXT("SettlementExpRowArt_0")) == nullptr
			&& Blueprint->WidgetTree->FindWidget(TEXT("SettlementGuaranteedGoldPanelArt")) == nullptr
			&& Blueprint->WidgetTree->FindWidget(TEXT("SettlementChoiceCard_0")) == nullptr
			&& Blueprint->WidgetTree->FindWidget(TEXT("NextButtonArt")) == nullptr,
			TEXT("V10 reward WBP still contains retired composite UI chrome"));
		UWidgetSwitcher* Switcher = CastChecked<UWidgetSwitcher>(
			Blueprint->WidgetTree->FindWidget(TEXT("SettlementStepSwitcher")));
		checkf(Switcher->GetChildrenCount() == 4 && Switcher->GetActiveWidgetIndex() == 0,
			TEXT("V3 reward switcher contract failed"));
		checkf(Blueprint->GeneratedClass != nullptr
			&& Blueprint->GeneratedClass->IsChildOf(URewardSettlementWidgetBase::StaticClass()),
			TEXT("V3 reward generated class is invalid"));
		UE_LOG(LogTemp, Display, TEXT("RD_REWARD_SETTLEMENT_V3_VERIFY success"));
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
