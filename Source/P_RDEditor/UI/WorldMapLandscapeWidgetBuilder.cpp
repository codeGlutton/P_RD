#include "UI/WorldMapLandscapeWidgetBuilder.h"

#include "AssetToolsModule.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "HAL/IConsoleManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UI/FrontendMapLandscapeWidget.h"
#include "UI/UIFont.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintEditorUtils.h"
#include "WidgetBlueprintFactory.h"

namespace WorldMapLandscapeWidgetBuilder
{
	constexpr TCHAR PackagePath[] = TEXT("/Game/UI/WorldMapLandscape");
	constexpr TCHAR MainAssetName[] = TEXT("WBP_FrontendMapLandscape");
	constexpr TCHAR MainAssetPath[] = TEXT(
		"/Game/UI/WorldMapLandscape/WBP_FrontendMapLandscape.WBP_FrontendMapLandscape");
	constexpr TCHAR NodeAssetName[] = TEXT("WBP_FrontendMapLandscapeNode");
	constexpr TCHAR NodeAssetPath[] = TEXT(
		"/Game/UI/WorldMapLandscape/WBP_FrontendMapLandscapeNode.WBP_FrontendMapLandscapeNode");
	constexpr TCHAR LineAssetName[] = TEXT("WBP_FrontendMapLandscapeLine");
	constexpr TCHAR LineAssetPath[] = TEXT(
		"/Game/UI/WorldMapLandscape/WBP_FrontendMapLandscapeLine.WBP_FrontendMapLandscapeLine");

	TUniquePtr<FAutoConsoleCommand> BuildCommand;
	TUniquePtr<FAutoConsoleCommand> VerifyCommand;

	UTexture2D* Texture(const TCHAR* Path)
	{
		UTexture2D* Result = LoadObject<UTexture2D>(nullptr, Path);
		checkf(Result != nullptr, TEXT("Missing landscape world-map texture: %s"), Path);
		return Result;
	}

	FSlateBrush TextureBrush(UTexture2D* Source, const FLinearColor& Tint = FLinearColor::White)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Source);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.TintColor = FSlateColor(Tint);
		if (Source != nullptr)
		{
			const FIntPoint Size = Source->GetImportedSize();
			Brush.ImageSize = FVector2D(Size.X, Size.Y);
		}
		return Brush;
	}

	void Expose(UWidgetBlueprint* Blueprint, UWidget* Widget)
	{
		if (Blueprint != nullptr && Widget != nullptr
			&& Blueprint->WidgetVariableNameToGuidMap.Contains(Widget->GetFName()) == false)
		{
			Blueprint->OnVariableAdded(Widget->GetFName());
		}
	}

	void FillOverlay(UOverlay* Parent, UWidget* Child, int32 ZOrder = 0)
	{
		UOverlaySlot* Slot = Parent->AddChildToOverlay(Child);
		Slot->SetHorizontalAlignment(HAlign_Fill);
		Slot->SetVerticalAlignment(VAlign_Fill);
		Slot->SetPadding(FMargin(0.f));
		(void)ZOrder; // Overlay는 자식 추가 순서가 곧 그리기 순서다.
	}

	void Place(UCanvasPanel* Parent, UWidget* Child, const FVector2D& Position,
		const FVector2D& Size, int32 ZOrder)
	{
		UCanvasPanelSlot* Slot = Parent->AddChildToCanvas(Child);
		Slot->SetAnchors(FAnchors(0.f));
		Slot->SetAlignment(FVector2D::ZeroVector);
		Slot->SetAutoSize(false);
		Slot->SetPosition(Position);
		Slot->SetSize(Size);
		Slot->SetZOrder(ZOrder);
	}

	void Anchor(UCanvasPanel* Parent, UWidget* Child, const FAnchors& Anchors,
		const FMargin& Offsets, int32 ZOrder)
	{
		UCanvasPanelSlot* Slot = Parent->AddChildToCanvas(Child);
		Slot->SetAnchors(Anchors);
		Slot->SetAlignment(FVector2D::ZeroVector);
		Slot->SetAutoSize(false);
		Slot->SetOffsets(Offsets);
		Slot->SetZOrder(ZOrder);
	}

	void StyleText(UTextBlock* Text, int32 Size, const FLinearColor& Color = FLinearColor::White)
	{
		FSlateFontInfo Font = UIFont::MakeProjectExact(Text->GetFont(), Size);
		Font.OutlineSettings.OutlineSize = Size >= 28 ? 2 : 1;
		Font.OutlineSettings.OutlineColor = FLinearColor(0.02f, 0.01f, 0.f, 1.f);
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetShadowOffset(FVector2D(2.f, 2.f));
		Text->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, .8f));
		Text->SetJustification(ETextJustify::Center);
		Text->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	UTextBlock* AddText(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent, const FName Name,
		const TCHAR* Value, int32 FontSize, const FVector2D& Position,
		const FVector2D& Size, int32 ZOrder)
	{
		UTextBlock* Text = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), Name);
		Text->SetText(FText::FromString(Value));
		StyleText(Text, FontSize);
		Place(Parent, Text, Position, Size, ZOrder);
		return Text;
	}

	UImage* AddImage(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent, const FName Name,
		UTexture2D* Source, const FVector2D& Position, const FVector2D& Size, int32 ZOrder)
	{
		UImage* Image = Blueprint->WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
		Image->SetBrush(TextureBrush(Source));
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		Place(Parent, Image, Position, Size, ZOrder);
		return Image;
	}

	UWidgetBlueprint* EnsureBlueprint(const TCHAR* AssetPath, const TCHAR* AssetName,
		UClass* ParentClass)
	{
		if (UWidgetBlueprint* Existing = LoadObject<UWidgetBlueprint>(nullptr, AssetPath))
		{
			Existing->ParentClass = ParentClass;
			return Existing;
		}

		UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
		Factory->ParentClass = ParentClass;
		FAssetToolsModule& AssetTools =
			FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		return Cast<UWidgetBlueprint>(AssetTools.Get().CreateAsset(
			AssetName, PackagePath, UWidgetBlueprint::StaticClass(), Factory));
	}

	void ResetTree(UWidgetBlueprint* Blueprint, UClass* FinalParent)
	{
		Blueprint->Modify();
		Blueprint->WidgetTree->Modify();
		if (UWidget* PreviousRoot = Blueprint->WidgetTree->RootWidget)
		{
			Blueprint->ParentClass = UUserWidget::StaticClass();
			TSet<UWidget*> Widgets;
			Widgets.Add(PreviousRoot);
			FWidgetBlueprintEditorUtils::DeleteWidgets(Blueprint, MoveTemp(Widgets),
				FWidgetBlueprintEditorUtils::EDeleteWidgetWarningType::DeleteSilently);
		}
		Blueprint->ParentClass = FinalParent;
	}

	void CompileBlueprint(UWidgetBlueprint* Blueprint)
	{
		// UE 5.7은 변수로 노출하지 않은 장식 위젯에도 컴파일 GUID를 요구한다.
		Blueprint->WidgetTree->ForEachWidget([Blueprint](UWidget* Widget)
		{
			Expose(Blueprint, Widget);
		});
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
	}

	void SaveCompiledBlueprint(UWidgetBlueprint* Blueprint)
	{
		UPackage::SavePackage(Blueprint->GetPackage(), Blueprint,
			*FPackageName::LongPackageNameToFilename(
				Blueprint->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension()),
			FSavePackageArgs());
	}

	void SaveBlueprint(UWidgetBlueprint* Blueprint)
	{
		CompileBlueprint(Blueprint);
		SaveCompiledBlueprint(Blueprint);
	}

	FButtonStyle TransparentButtonStyle()
	{
		FSlateBrush Transparent;
		Transparent.DrawAs = ESlateBrushDrawType::NoDrawType;
		FButtonStyle Style;
		Style.SetNormal(Transparent);
		Style.SetHovered(Transparent);
		Style.SetPressed(Transparent);
		Style.SetDisabled(Transparent);
		Style.SetNormalPadding(FMargin(0.f));
		Style.SetPressedPadding(FMargin(0.f));
		return Style;
	}

	UClass* BuildNode()
	{
		UWidgetBlueprint* Blueprint = EnsureBlueprint(NodeAssetPath, NodeAssetName,
			UFrontendMapLandscapeNodeWidget::StaticClass());
		check(Blueprint != nullptr && Blueprint->WidgetTree != nullptr);
		ResetTree(Blueprint, UFrontendMapLandscapeNodeWidget::StaticClass());

		UButton* Root = Blueprint->WidgetTree->ConstructWidget<UButton>(
			UButton::StaticClass(), TEXT("NodeButton"));
		Root->SetStyle(TransparentButtonStyle());
		Root->SetClickMethod(EButtonClickMethod::MouseDown);
		Blueprint->WidgetTree->RootWidget = Root;
		Expose(Blueprint, Root);

		UOverlay* Stack = Blueprint->WidgetTree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(), TEXT("LandscapeNodeStack"));
		Root->SetContent(Stack);
		if (UButtonSlot* StackSlot = Cast<UButtonSlot>(Stack->Slot))
		{
			StackSlot->SetPadding(FMargin(0.f));
			StackSlot->SetHorizontalAlignment(HAlign_Fill);
			StackSlot->SetVerticalAlignment(VAlign_Fill);
		}

		UBorder* Icon = Blueprint->WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("NodePanel"));
		Icon->SetPadding(FMargin(0.f));
		Icon->SetBrushColor(FLinearColor::White);
		Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
		FillOverlay(Stack, Icon, 0);
		Expose(Blueprint, Icon);

		UImage* Ring = Blueprint->WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), TEXT("NodeRingImage"));
		Ring->SetVisibility(ESlateVisibility::HitTestInvisible);
		FillOverlay(Stack, Ring, 1);
		Expose(Blueprint, Ring);

		SaveBlueprint(Blueprint);
		check(Blueprint->GeneratedClass != nullptr);
		return Blueprint->GeneratedClass;
	}

	UClass* BuildLine()
	{
		UWidgetBlueprint* Blueprint = EnsureBlueprint(LineAssetPath, LineAssetName,
			UFrontendMapLandscapeLineWidget::StaticClass());
		check(Blueprint != nullptr && Blueprint->WidgetTree != nullptr);
		ResetTree(Blueprint, UFrontendMapLandscapeLineWidget::StaticClass());

		UOverlay* Root = Blueprint->WidgetTree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(), TEXT("LandscapeLineRoot"));
		Blueprint->WidgetTree->RootWidget = Root;

		UBorder* Fallback = Blueprint->WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("LinePanel"));
		Fallback->SetPadding(FMargin(0.f));
		Fallback->SetBrushColor(FLinearColor(0.96f, .66f, .15f, .8f));
		FillOverlay(Root, Fallback, 0);
		Expose(Blueprint, Fallback);

		UImage* Glow = Blueprint->WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), TEXT("LineGlowImage"));
		Glow->SetVisibility(ESlateVisibility::HitTestInvisible);
		FillOverlay(Root, Glow, 1);
		Expose(Blueprint, Glow);

		UImage* Image = Blueprint->WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), TEXT("LineImage"));
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		UOverlaySlot* CoreSlot = Root->AddChildToOverlay(Image);
		CoreSlot->SetHorizontalAlignment(HAlign_Fill);
		CoreSlot->SetVerticalAlignment(VAlign_Fill);
		CoreSlot->SetPadding(FMargin(0.f));
		Expose(Blueprint, Image);

		SaveBlueprint(Blueprint);
		check(Blueprint->GeneratedClass != nullptr);
		return Blueprint->GeneratedClass;
	}

	void AddLegend(UWidgetBlueprint* Blueprint, UCanvasPanel* MainCanvas)
	{
		UCanvasPanel* Legend = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("Map_LegendGroup"));
		Place(MainCanvas, Legend, FVector2D(78.f, 268.f), FVector2D(240.f, 448.f), 50);
		Expose(Blueprint, Legend);

		UImage* Frame = AddImage(Blueprint, Legend, TEXT("LandscapeLegendFrame"),
			Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Run/WorldMap/Panels/"
				"T_WorldMap_LegendPanel_Gen_20260812.T_WorldMap_LegendPanel_Gen_20260812")),
			FVector2D::ZeroVector, FVector2D(240.f, 448.f), 0);
		Frame->SetColorAndOpacity(FLinearColor::White);

		struct FLegendItem { const TCHAR* Label; const TCHAR* TexturePath; };
		const FLegendItem Items[] = {
			{ TEXT("전투"), TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Run/WorldMap/Nodes/T_WorldMap_Node_Battle_Gen_20260812.T_WorldMap_Node_Battle_Gen_20260812") },
			{ TEXT("엘리트"), TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Run/WorldMap/Nodes/T_WorldMap_Node_Elite_Gen_20260812.T_WorldMap_Node_Elite_Gen_20260812") },
			{ TEXT("보스"), TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Run/WorldMap/Nodes/T_WorldMap_Node_Boss_Gen_20260812.T_WorldMap_Node_Boss_Gen_20260812") },
			{ TEXT("상점"), TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Run/WorldMap/Nodes/T_WorldMap_Node_Shop_Gen_20260812.T_WorldMap_Node_Shop_Gen_20260812") },
			{ TEXT("보물"), TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Run/WorldMap/Nodes/T_WorldMap_Node_Treasure_Gen_20260812.T_WorldMap_Node_Treasure_Gen_20260812") },
		};

		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Items); ++Index)
		{
			const float Y = 42.f + Index * 72.f;
			AddImage(Blueprint, Legend, FName(*FString::Printf(TEXT("LegendIcon_%d"), Index)),
				Texture(Items[Index].TexturePath), FVector2D(34.f, Y), FVector2D(60.f, 60.f), 2);
			UTextBlock* Label = AddText(Blueprint, Legend,
				FName(*FString::Printf(TEXT("LegendText_%d"), Index)), Items[Index].Label,
				25, FVector2D(101.f, Y + 7.f), FVector2D(101.f, 46.f), 3);
			Label->SetJustification(ETextJustify::Left);

			if (Index + 1 < UE_ARRAY_COUNT(Items))
			{
				UBorder* Separator = Blueprint->WidgetTree->ConstructWidget<UBorder>(
					UBorder::StaticClass(),
					FName(*FString::Printf(TEXT("LegendSeparator_%d"), Index)));
				Separator->SetBrushColor(FLinearColor(.76f, .46f, .12f, .58f));
				Separator->SetPadding(FMargin(0.f));
				Separator->SetVisibility(ESlateVisibility::HitTestInvisible);
				Place(Legend, Separator, FVector2D(37.f, Y + 65.f), FVector2D(163.f, 1.5f), 2);
			}
		}
	}

	void BuildMain(UClass* LineWidgetClass, UClass* NodeWidgetClass)
	{
		check(LineWidgetClass != nullptr
			&& LineWidgetClass->IsChildOf(UFrontendMapLandscapeLineWidget::StaticClass()));
		check(NodeWidgetClass != nullptr
			&& NodeWidgetClass->IsChildOf(UFrontendMapLandscapeNodeWidget::StaticClass()));

		UTexture2D* Background = Texture(TEXT(
			"/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Run/WorldMap/Backgrounds/"
			"T_WorldMapLandscape_Base_20260811.T_WorldMapLandscape_Base_20260811"));
		UTexture2D* ButtonPlate = Texture(TEXT(
			"/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Combat/HUD/"
			"KK_HUD04_bottom_right_button.KK_HUD04_bottom_right_button"));
		UTexture2D* CurrentMarker = Texture(TEXT(
			"/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Run/WorldMap/Nodes/"
			"T_WorldMap_Node_CurrentParty_Gen_20260812.T_WorldMap_Node_CurrentParty_Gen_20260812"));
		UTexture2D* SelectGlow = Texture(TEXT(
			"/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Run/WorldMap/Markers/"
			"T_wm_marker_next_highlight.T_wm_marker_next_highlight"));

		UWidgetBlueprint* Blueprint = EnsureBlueprint(MainAssetPath, MainAssetName,
			UFrontendMapLandscapeWidget::StaticClass());
		check(Blueprint != nullptr && Blueprint->WidgetTree != nullptr);
		ResetTree(Blueprint, UFrontendMapLandscapeWidget::StaticClass());

		UOverlay* Root = Blueprint->WidgetTree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(), TEXT("LandscapeMapRoot"));
		Root->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		Blueprint->WidgetTree->RootWidget = Root;

		// 고정 비율 지도 밖에 남는 영역은 완전 불투명 단색 배경으로 가린다.
		// 게임 화면을 노출하지 않으면서도 BackgroundBlur 비용은 들지 않는다.
		UBorder* Backdrop = Blueprint->WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("LandscapeMapBackdrop"));
		Backdrop->SetBrushColor(FLinearColor::FromSRGBColor(FColor(0x05, 0x08, 0x0C, 0xFF)));
		Backdrop->SetPadding(FMargin(0.f));
		Backdrop->SetVisibility(ESlateVisibility::Visible);
		FillOverlay(Root, Backdrop);
		Expose(Blueprint, Backdrop);

		UScaleBox* MasterScale = Blueprint->WidgetTree->ConstructWidget<UScaleBox>(
			UScaleBox::StaticClass(), TEXT("LandscapeMapMasterScale"));
		MasterScale->SetStretch(EStretch::ScaleToFit);
		MasterScale->SetStretchDirection(EStretchDirection::Both);
		MasterScale->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		FillOverlay(Root, MasterScale);

		USizeBox* Design = Blueprint->WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("MapGraphSize"));
		Design->SetWidthOverride(1672.f);
		Design->SetHeightOverride(941.f);
		MasterScale->AddChild(Design);
		Expose(Blueprint, Design);

		UCanvasPanel* Canvas = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("MapGraphCanvas"));
		Canvas->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		Design->SetContent(Canvas);
		Expose(Blueprint, Canvas);

		UImage* BackgroundImage = AddImage(Blueprint, Canvas, TEXT("Map_ParchmentBody"),
			Background, FVector2D::ZeroVector, FVector2D(1672.f, 941.f), -200);
		Expose(Blueprint, BackgroundImage);

		UOverlay* TitleArea = Blueprint->WidgetTree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(), TEXT("MapTitleArea"));
		TitleArea->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Place(Canvas, TitleArea, FVector2D(505.f, 32.f), FVector2D(662.f, 86.f), 30);
		Expose(Blueprint, TitleArea);

		UTextBlock* Title = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("MapTitleText"));
		Title->SetText(FText::FromString(TEXT("지도")));
		StyleText(Title, 65);
		Title->SetColorAndOpacity(FSlateColor(FLinearColor(.96f, .91f, .74f, 1.f)));
		// 프로젝트 폰트의 line box는 실제 글리프보다 위쪽 여백이 커서 슬롯의
		// 수학적 중앙과 눈에 보이는 중앙이 다르다. Overlay 중앙정렬은 유지하고
		// 제목에만 글리프 기준 시각 보정을 적용한다.
		Title->SetRenderTranslation(FVector2D(0.f, -34.f));
		UOverlaySlot* TitleSlot = TitleArea->AddChildToOverlay(Title);
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
		TitleSlot->SetVerticalAlignment(VAlign_Center);
		TitleSlot->SetPadding(FMargin(0.f));
		Expose(Blueprint, Title);

		UBorder* NodeArea = Blueprint->WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("Map_NodeArea"));
		NodeArea->SetBrushColor(FLinearColor::Transparent);
		NodeArea->SetVisibility(ESlateVisibility::Collapsed);
		Anchor(Canvas, NodeArea, FAnchors(.22f, 0.f, .88f, 1.f),
			FMargin(0.f, 245.f, 0.f, 170.f), -100);
		Expose(Blueprint, NodeArea);

		USizeBox* NodeMetrics = Blueprint->WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("Map_NodeMetrics"));
		NodeMetrics->SetWidthOverride(88.f);
		NodeMetrics->SetHeightOverride(88.f);
		NodeMetrics->SetVisibility(ESlateVisibility::Collapsed);
		Place(Canvas, NodeMetrics, FVector2D::ZeroVector, FVector2D(88.f), -100);
		Expose(Blueprint, NodeMetrics);

		UImage* Glow = AddImage(Blueprint, Canvas, TEXT("Map_SelectGlow"), SelectGlow,
			FVector2D::ZeroVector, FVector2D(150.f), 1);
		Glow->SetVisibility(ESlateVisibility::Collapsed);
		Expose(Blueprint, Glow);
		UImage* Marker = AddImage(Blueprint, Canvas, TEXT("Map_CurrentMarker"), CurrentMarker,
			FVector2D::ZeroVector, FVector2D(126.f), 7);
		Marker->SetVisibility(ESlateVisibility::Collapsed);
		Expose(Blueprint, Marker);

		AddLegend(Blueprint, Canvas);

		UButton* Enter = Blueprint->WidgetTree->ConstructWidget<UButton>(
			UButton::StaticClass(), TEXT("EnterRoomButton"));
		FButtonStyle EnterStyle;
		EnterStyle.SetNormal(TextureBrush(ButtonPlate));
		EnterStyle.SetHovered(TextureBrush(ButtonPlate, FLinearColor(1.12f, 1.06f, .94f, 1.f)));
		EnterStyle.SetPressed(TextureBrush(ButtonPlate, FLinearColor(.72f, .72f, .72f, 1.f)));
		EnterStyle.SetDisabled(TextureBrush(ButtonPlate, FLinearColor(.62f, .58f, .50f, .82f)));
		EnterStyle.SetNormalPadding(FMargin(0.f));
		EnterStyle.SetPressedPadding(FMargin(2.f, 4.f, 0.f, 0.f));
		Enter->SetStyle(EnterStyle);
		Place(Canvas, Enter, FVector2D(1218.f, 758.f), FVector2D(356.f, 130.f), 80);
		Expose(Blueprint, Enter);
		UTextBlock* EnterText = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("EnterButtonText"));
		EnterText->SetText(FText::FromString(TEXT("입장")));
		StyleText(EnterText, 48, FLinearColor(.98f, .92f, .75f, 1.f));
		Enter->SetContent(EnterText);
		if (UButtonSlot* EnterSlot = Cast<UButtonSlot>(EnterText->Slot))
		{
			EnterSlot->SetHorizontalAlignment(HAlign_Center);
			EnterSlot->SetVerticalAlignment(VAlign_Center);
		}
		Expose(Blueprint, EnterText);

		UButton* Close = Blueprint->WidgetTree->ConstructWidget<UButton>(
			UButton::StaticClass(), TEXT("CloseButton"));
		FButtonStyle CloseStyle;
		CloseStyle.SetNormal(TextureBrush(ButtonPlate, FLinearColor(.82f, .76f, .62f, .92f)));
		CloseStyle.SetHovered(TextureBrush(ButtonPlate, FLinearColor(1.06f, 1.02f, .92f, 1.f)));
		CloseStyle.SetPressed(TextureBrush(ButtonPlate, FLinearColor(.66f, .62f, .55f, 1.f)));
		CloseStyle.SetDisabled(TextureBrush(ButtonPlate, FLinearColor(.42f, .42f, .42f, .65f)));
		CloseStyle.SetNormalPadding(FMargin(0.f));
		CloseStyle.SetPressedPadding(FMargin(1.f, 2.f, 0.f, 0.f));
		Close->SetStyle(CloseStyle);
		Place(Canvas, Close, FVector2D(78.f, 730.f), FVector2D(240.f, 78.f), 90);
		Expose(Blueprint, Close);
		UTextBlock* CloseText = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("CloseButtonText"));
		CloseText->SetText(FText::FromString(TEXT("뒤로")));
		StyleText(CloseText, 29, FLinearColor(.96f, .90f, .72f, 1.f));
		Close->SetContent(CloseText);
		if (UButtonSlot* CloseSlot = Cast<UButtonSlot>(CloseText->Slot))
		{
			CloseSlot->SetHorizontalAlignment(HAlign_Center);
			CloseSlot->SetVerticalAlignment(VAlign_Center);
		}
		Expose(Blueprint, CloseText);

		UTextBlock* Status = AddText(Blueprint, Canvas, TEXT("MapStatusText"), TEXT(""),
			18, FVector2D(600.f, 890.f), FVector2D(472.f, 30.f), 20);
		Status->SetVisibility(ESlateVisibility::Collapsed);
		Expose(Blueprint, Status);

		// Main을 먼저 컴파일해야 최종 GeneratedClass CDO가 생긴다. 그 CDO에 앞서
		// 컴파일·저장한 Line/Node 클래스를 넣으면 메인 패키지가 두 WBP를 하드 참조해
		// cooker가 별도 AlwaysCook 설정 없이 함께 수집한다.
		CompileBlueprint(Blueprint);
		UFrontendMapLandscapeWidget* Defaults = CastChecked<UFrontendMapLandscapeWidget>(
			Blueprint->GeneratedClass->GetDefaultObject());
		Defaults->Modify();
		Defaults->SetLandscapeGraphWidgetClassesForEditor(
			TSubclassOf<UFrontendMapLineWidget>(LineWidgetClass),
			TSubclassOf<UFrontendMapNodeWidget>(NodeWidgetClass));
		Blueprint->MarkPackageDirty();
		SaveCompiledBlueprint(Blueprint);
	}

	void Build()
	{
		UE_LOG(LogTemp, Display, TEXT("RD_WORLDMAP_LANDSCAPE_BUILD begin"));
		UClass* NodeWidgetClass = BuildNode();
		UClass* LineWidgetClass = BuildLine();
		BuildMain(LineWidgetClass, NodeWidgetClass);
		UE_LOG(LogTemp, Display, TEXT("RD_WORLDMAP_LANDSCAPE_BUILD success"));
	}

	void Verify()
	{
		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, MainAssetPath);
		checkf(Blueprint != nullptr && Blueprint->GeneratedClass != nullptr,
			TEXT("Landscape world-map WBP is missing"));
		checkf(Blueprint->GeneratedClass->IsChildOf(UFrontendMapLandscapeWidget::StaticClass()),
			TEXT("Landscape world-map WBP parent is wrong"));
		UWidgetBlueprint* LineBlueprint = LoadObject<UWidgetBlueprint>(nullptr, LineAssetPath);
		UWidgetBlueprint* NodeBlueprint = LoadObject<UWidgetBlueprint>(nullptr, NodeAssetPath);
		checkf(LineBlueprint != nullptr && LineBlueprint->GeneratedClass != nullptr,
			TEXT("Landscape world-map line WBP is missing"));
		checkf(NodeBlueprint != nullptr && NodeBlueprint->GeneratedClass != nullptr,
			TEXT("Landscape world-map node WBP is missing"));
		const UFrontendMapLandscapeWidget* Defaults =
			CastChecked<UFrontendMapLandscapeWidget>(Blueprint->GeneratedClass->GetDefaultObject());
		checkf(Defaults->GetLandscapeLineWidgetClass().Get() == LineBlueprint->GeneratedClass,
			TEXT("Main landscape WBP does not serialize the line WBP class"));
		checkf(Defaults->GetLandscapeNodeWidgetClass().Get() == NodeBlueprint->GeneratedClass,
			TEXT("Main landscape WBP does not serialize the node WBP class"));
		const FName Required[] = {
			TEXT("LandscapeMapBackdrop"), TEXT("MapGraphSize"), TEXT("MapGraphCanvas"),
			TEXT("Map_ParchmentBody"), TEXT("MapTitleArea"),
			TEXT("Map_NodeArea"), TEXT("Map_NodeMetrics"), TEXT("MapTitleText"),
			TEXT("EnterRoomButton"), TEXT("EnterButtonText"), TEXT("CloseButton"),
			TEXT("Map_CurrentMarker"),
			TEXT("Map_SelectGlow"), TEXT("Map_LegendGroup")
		};
		for (const FName Name : Required)
		{
			checkf(Blueprint->WidgetTree->FindWidget(Name) != nullptr,
				TEXT("Landscape world-map required widget missing: %s"), *Name.ToString());
		}
		UE_LOG(LogTemp, Display, TEXT("RD_WORLDMAP_LANDSCAPE_VERIFY success"));
	}
}

void RegisterWorldMapLandscapeWidgetBuilderCommands()
{
	using namespace WorldMapLandscapeWidgetBuilder;
	BuildCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.BuildWorldMapLandscape"),
		TEXT("Create the independent landscape world-map WBP set."),
		FConsoleCommandDelegate::CreateStatic(&Build));
	VerifyCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.VerifyWorldMapLandscape"),
		TEXT("Verify the independent landscape world-map WBP set."),
		FConsoleCommandDelegate::CreateStatic(&Verify));
}

void UnregisterWorldMapLandscapeWidgetBuilderCommands()
{
	WorldMapLandscapeWidgetBuilder::BuildCommand.Reset();
	WorldMapLandscapeWidgetBuilder::VerifyCommand.Reset();
}
