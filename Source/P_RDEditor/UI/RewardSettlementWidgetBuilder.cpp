#include "UI/RewardSettlementWidgetBuilder.h"

#include "AssetToolsModule.h"
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
#include "Engine/Texture2D.h"
#include "WidgetBlueprintFactory.h"
#include "HAL/IConsoleManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UI/Reward/RewardSettlementWidgetBase.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"

namespace RewardSettlementWidgetBuilder
{
	constexpr TCHAR PackagePath[] = TEXT("/Game/UI/RewardSettlement");
	constexpr TCHAR AssetName[] = TEXT("WBP_RewardSettlement_Runtime");
	constexpr TCHAR AssetPath[] = TEXT("/Game/UI/RewardSettlement/WBP_RewardSettlement_Runtime.WBP_RewardSettlement_Runtime");
	TUniquePtr<FAutoConsoleCommand> BuildCommand;
	TUniquePtr<FAutoConsoleCommand> VerifyCommand;

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
		if (Source != nullptr)
		{
			Brush.ImageSize = FVector2D(Source->GetSizeX(), Source->GetSizeY());
		}
		if (UV != nullptr)
		{
			Brush.SetUVRegion(*UV);
		}
		return Brush;
	}

	void StyleText(UTextBlock* Text, int32 Size, const FLinearColor& Color,
		ETextJustify::Type Justification = ETextJustify::Center)
	{
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Font.OutlineSettings.OutlineSize = Size >= 28 ? 1 : 0;
		Font.OutlineSettings.OutlineColor = FLinearColor::Black;
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));
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

	UTextBlock* AddText(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent, const FName Name,
		const FText& Value, int32 FontSize, const FLinearColor& Color,
		const FVector2D Position, const FVector2D Size, int32 ZOrder)
	{
		UTextBlock* Text = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Text->SetText(Value);
		StyleText(Text, FontSize, Color);
		Place(Parent, Text, Position, Size, ZOrder);
		return Text;
	}

	void AddPreviewRow(UWidgetBlueprint* Blueprint, UVerticalBox* Box, const FName Name,
		const FText& Label, float Height, const FLinearColor& Color)
	{
		USizeBox* Size = Blueprint->WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),
			FName(*(Name.ToString() + TEXT("_Size"))));
		Size->SetHeightOverride(Height);
		UBorder* Border = Blueprint->WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
		Border->SetBrushColor(Color);
		Border->SetPadding(FMargin(16.f, 7.f));
		Size->SetContent(Border);
		UTextBlock* Text = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
			FName(*(Name.ToString() + TEXT("_Text"))));
		Text->SetText(Label);
		StyleText(Text, 22, FLinearColor(1.f, .91f, .73f, 1.f), ETextJustify::Left);
		Border->SetContent(Text);
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
		UWidgetBlueprint* Blueprint = FindOrCreateBlueprint();
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("RD_REWARD_SETTLEMENT_BUILD could not create %s"), AssetPath);
			return;
		}

		Blueprint->Modify();
		Blueprint->WidgetTree->Modify();
		Blueprint->ParentClass = URewardSettlementWidgetBase::StaticClass();

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

		UTexture2D* TitlePlate = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/RewardSettlement/T_RS_TitlePlate.T_RS_TitlePlate"));
		UTexture2D* StepPlate = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/RewardSettlement/T_RS_Step1Active.T_RS_Step1Active"));
		UTexture2D* VictoryPanel = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/RewardSettlement/T_RS_VictoryPanelNeutral.T_RS_VictoryPanelNeutral"));
		UTexture2D* ExpFrame = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/RewardSettlement/T_RS_ExpFrame.T_RS_ExpFrame"));
		UTexture2D* NextPlate = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/RewardSettlement/T_RS_NextButton.T_RS_NextButton"));

		const FBox2f TitleUV(FVector2f(46.f / 2149.f, 127.f / 732.f), FVector2f(2119.f / 2149.f, 506.f / 732.f));
		const FBox2f StepUV(FVector2f(34.f / 2172.f, 248.f / 724.f), FVector2f(2137.f / 2172.f, 472.f / 724.f));
		const FBox2f VictoryUV(FVector2f(33.f / 1081.f, 52.f / 1455.f), FVector2f(1050.f / 1081.f, 1421.f / 1455.f));
		const FBox2f ExpUV(FVector2f(25.f / 1650.f, 20.f / 953.f), FVector2f(1628.f / 1650.f, 937.f / 953.f));
		const FBox2f NextUV(FVector2f(88.f / 1649.f, 177.f / 954.f), FVector2f(1561.f / 1649.f, 781.f / 954.f));

		UCanvasPanel* Header = AddFixedDesignRegion(Blueprint, DesignCanvas,
			TEXT("HeaderScale"), TEXT("HeaderCanvas"), FVector2D(350.f, 18.f),
			FVector2D(900.f, 172.f), 20);
		AddImage(Blueprint, Header, TEXT("TitlePlateArt"), TitlePlate, &TitleUV,
			FVector2D(155.f, 0.f), FVector2D(590.f, 100.f), 0);
		UTextBlock* TitleText = AddText(Blueprint, Header, TEXT("mTitleText"), NSLOCTEXT("RewardSettlement", "Title", "전투 보상"),
			44, FLinearColor(1.f, .93f, .78f, 1.f), FVector2D(190.f, 18.f), FVector2D(520.f, 62.f), 2);
		AddImage(Blueprint, Header, TEXT("StepProgressArt"), StepPlate, &StepUV,
			FVector2D(55.f, 103.f), FVector2D(790.f, 66.f), 0);
		AddText(Blueprint, Header, TEXT("StepOneNumber"), FText::AsNumber(1),
			30, FLinearColor::White, FVector2D(64.f, 107.f), FVector2D(82.f, 48.f), 2);
		AddText(Blueprint, Header, TEXT("StepOneLabel"),
			NSLOCTEXT("RewardSettlement", "StepOne", "경험치 정산"), 25,
			FLinearColor::White, FVector2D(145.f, 108.f), FVector2D(300.f, 46.f), 2);
		AddText(Blueprint, Header, TEXT("StepTwoNumber"), FText::AsNumber(2),
			29, FLinearColor(.72f, .72f, .72f, 1.f), FVector2D(500.f, 107.f), FVector2D(82.f, 48.f), 2);
		AddText(Blueprint, Header, TEXT("StepTwoLabel"),
			NSLOCTEXT("RewardSettlement", "StepTwo", "골드 · 아티팩트"), 24,
			FLinearColor(.72f, .72f, .72f, 1.f), FVector2D(580.f, 108.f), FVector2D(250.f, 46.f), 2);

		UCanvasPanel* Summary = AddFixedDesignRegion(Blueprint, DesignCanvas,
			TEXT("SummaryScale"), TEXT("SummaryCanvas"), FVector2D(35.f, 195.f),
			FVector2D(360.f, 650.f), 15);
		AddImage(Blueprint, Summary, TEXT("VictoryPanelArt"), VictoryPanel, &VictoryUV,
			FVector2D(0.f, 0.f), FVector2D(360.f, 650.f), 0);
		AddText(Blueprint, Summary, TEXT("SummaryTitle"), NSLOCTEXT("RewardSettlement", "VictoryReward", "승리 보상"),
			31, FLinearColor(1.f, .78f, .25f, 1.f), FVector2D(35.f, 42.f), FVector2D(290.f, 48.f), 2);
		UTextBlock* GoldBalance = AddText(Blueprint, Summary, TEXT("mGoldBalanceText"), NSLOCTEXT("RewardSettlement", "GoldPreview", "보유 골드 245"),
			18, FLinearColor(1.f, .91f, .73f, 1.f), FVector2D(35.f, 96.f), FVector2D(290.f, 35.f), 2);
		UVerticalBox* SummaryRows = Blueprint->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("mSummaryRowsBox"));
		Place(Summary, SummaryRows, FVector2D(20.f, 150.f), FVector2D(320.f, 400.f), 3);
		AddPreviewRow(Blueprint, SummaryRows, TEXT("PreviewGoldRow"), NSLOCTEXT("RewardSettlement", "PreviewGold", "골드 +120"), 88.f, FLinearColor(.11f, .05f, .015f, .78f));
		AddPreviewRow(Blueprint, SummaryRows, TEXT("PreviewExpRow"), NSLOCTEXT("RewardSettlement", "PreviewExp", "경험치 +40"), 88.f, FLinearColor(.11f, .05f, .015f, .78f));
		AddPreviewRow(Blueprint, SummaryRows, TEXT("PreviewArtifactRow"), NSLOCTEXT("RewardSettlement", "PreviewArtifact", "아티팩트 1개"), 88.f, FLinearColor(.11f, .05f, .015f, .78f));

		UCanvasPanel* Exp = AddFixedDesignRegion(Blueprint, DesignCanvas,
			TEXT("ExpScale"), TEXT("ExpCanvas"), FVector2D(405.f, 195.f),
			FVector2D(1160.f, 650.f), 16);
		AddImage(Blueprint, Exp, TEXT("ExpFrameArt"), ExpFrame, &ExpUV,
			FVector2D(0.f, 0.f), FVector2D(1160.f, 650.f), 0);
		AddText(Blueprint, Exp, TEXT("ExpHeading"), NSLOCTEXT("RewardSettlement", "ExpSettlement", "경험치 정산"),
			40, FLinearColor(.12f, .065f, .025f, 1.f), FVector2D(260.f, 28.f), FVector2D(640.f, 58.f), 2);
		UVerticalBox* MercenaryRows = Blueprint->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("mMercenaryRowsBox"));
		Place(Exp, MercenaryRows, FVector2D(55.f, 108.f), FVector2D(1050.f, 420.f), 3);
		AddPreviewRow(Blueprint, MercenaryRows, TEXT("PreviewMercenary1"), NSLOCTEXT("RewardSettlement", "PreviewKnight", "기사   Lv.3       120 → 160       160 / 200       +40 XP"), 128.f, FLinearColor(.84f, .67f, .39f, .32f));
		AddPreviewRow(Blueprint, MercenaryRows, TEXT("PreviewMercenary2"), NSLOCTEXT("RewardSettlement", "PreviewMage", "마법사 Lv.2        70 → 110       110 / 150       +40 XP"), 128.f, FLinearColor(.84f, .67f, .39f, .32f));
		AddPreviewRow(Blueprint, MercenaryRows, TEXT("PreviewMercenary3"), NSLOCTEXT("RewardSettlement", "PreviewRanger", "레인저 Lv.2 → Lv.3  140 → 30       30 / 180        +40 XP"), 128.f, FLinearColor(.84f, .67f, .39f, .32f));
		AddText(Blueprint, Exp, TEXT("SettlementFooterText"),
			NSLOCTEXT("RewardSettlement", "Footer", "모든 용병이 경험치를 획득했습니다."),
			22, FLinearColor(1.f, .91f, .73f, 1.f), FVector2D(250.f, 571.f),
			FVector2D(560.f, 42.f), 4);

		UCanvasPanel* NextHolder = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("NextButtonHolder"));
		Place(Exp, NextHolder, FVector2D(845.f, 548.f), FVector2D(280.f, 92.f), 10);
		AddImage(Blueprint, NextHolder, TEXT("NextButtonArt"), NextPlate, &NextUV,
			FVector2D::ZeroVector, FVector2D(280.f, 92.f), 0);
		UTextBlock* NextText = AddText(Blueprint, NextHolder, TEXT("mNextButtonText"), NSLOCTEXT("RewardSettlement", "Next", "다음"),
			34, FLinearColor::White, FVector2D(25.f, 19.f), FVector2D(230.f, 52.f), 2);
		UButton* NextButton = Blueprint->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("mNextButton"));
		FButtonStyle TransparentStyle;
		FSlateBrush Empty; Empty.DrawAs = ESlateBrushDrawType::NoDrawType;
		TransparentStyle.SetNormal(Empty); TransparentStyle.SetHovered(Empty);
		TransparentStyle.SetPressed(Empty); TransparentStyle.SetDisabled(Empty);
		NextButton->SetStyle(TransparentStyle);
		Place(NextHolder, NextButton, FVector2D::ZeroVector, FVector2D(280.f, 92.f), 5);

		// UE 5.7 컴파일러는 트리의 모든 위젯에 GUID가 있어야 한다.
		// 런타임 BindWidget 대상뿐 아니라 장식 위젯도 한 번씩 등록한다.
		Blueprint->WidgetTree->ForEachWidget([Blueprint](UWidget* Widget)
		{
			if (Blueprint->WidgetVariableNameToGuidMap.Contains(Widget->GetFName()) == false)
			{
				Blueprint->OnVariableAdded(Widget->GetFName());
			}
		});

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Blueprint->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
		if (!UPackage::SavePackage(Blueprint->GetPackage(), Blueprint, *Filename, FSavePackageArgs()))
		{
			UE_LOG(LogTemp, Error, TEXT("RD_REWARD_SETTLEMENT_BUILD save failed"));
			return;
		}
		UE_LOG(LogTemp, Display, TEXT("RD_REWARD_SETTLEMENT_BUILD success asset=%s responsive=header+summary+exp"), AssetPath);
	}

	void Verify()
	{
		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, AssetPath);
		checkf(Blueprint != nullptr && Blueprint->WidgetTree != nullptr,
			TEXT("Reward settlement WBP is missing"));
		checkf(Blueprint->ParentClass == URewardSettlementWidgetBase::StaticClass(),
			TEXT("Reward settlement WBP uses the wrong parent class"));
		const FName Required[] = {
			TEXT("SettlementViewportRoot"), TEXT("SettlementMasterScale"),
			TEXT("SettlementDesignCanvas"), TEXT("HeaderScale"), TEXT("SummaryScale"),
			TEXT("ExpScale"), TEXT("mTitleText"), TEXT("mGoldBalanceText"),
			TEXT("mSummaryRowsBox"), TEXT("mMercenaryRowsBox"),
			TEXT("mNextButton"), TEXT("mNextButtonText")
		};
		for (const FName Name : Required)
		{
			checkf(Blueprint->WidgetTree->FindWidget(Name) != nullptr,
				TEXT("Reward settlement WBP is missing widget %s"), *Name.ToString());
		}
		checkf(Blueprint->GeneratedClass != nullptr
			&& Blueprint->GeneratedClass->IsChildOf(URewardSettlementWidgetBase::StaticClass()),
			TEXT("Reward settlement generated class is invalid"));
		UE_LOG(LogTemp, Display,
			TEXT("RD_REWARD_SETTLEMENT_VERIFY success widgets=%d parent=%s"),
			UE_ARRAY_COUNT(Required), *Blueprint->ParentClass->GetName());
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
