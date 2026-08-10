#include "UI/CombatDefeatWidgetBuilder.h"

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
#include "Engine/Texture2D.h"
#include "WidgetBlueprintFactory.h"
#include "HAL/IConsoleManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UI/UIFont.h"
#include "UI/CombatResultOverlayWidget.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintEditorUtils.h"

namespace CombatDefeatWidgetBuilder
{
	constexpr TCHAR PackagePath[] = TEXT("/Game/UI/CombatResult");
	constexpr TCHAR AssetName[] = TEXT("WBP_CombatDefeat");
	constexpr TCHAR AssetPath[] = TEXT("/Game/UI/CombatResult/WBP_CombatDefeat.WBP_CombatDefeat");
	constexpr int32 DefeatButtonFontSize = 30;
	TUniquePtr<FAutoConsoleCommand> BuildCommand;

	const FLinearColor DefeatTextColor = FLinearColor::White;

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
		checkf(Result != nullptr, TEXT("Missing defeat texture: %s"), Path);
		return Result;
	}

	FSlateBrush TextureBrush(UTexture2D* Source, const FBox2f& UV, bool bBox = false)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Source);
		Brush.DrawAs = bBox ? ESlateBrushDrawType::Box : ESlateBrushDrawType::Image;
		Brush.Margin = bBox ? FMargin(.16f) : FMargin(0.f);
		const FIntPoint NativeSize = NativeTextureSize(Source);
		Brush.ImageSize = NativeSize.X > 0 && NativeSize.Y > 0
			? FVector2D(NativeSize) : FVector2D::ZeroVector;
		Brush.SetUVRegion(UV);
		return Brush;
	}

	void StyleText(UTextBlock* Text, int32 Size, const FLinearColor& Color,
		ETextJustify::Type Justification = ETextJustify::Center)
	{
		FSlateFontInfo Font = UIFont::MakeProjectExact(Text->GetFont(), Size);
		Font.OutlineSettings.OutlineSize = Size >= 26 ? 1 : 0;
		Font.OutlineSettings.OutlineColor = FLinearColor(0.03f, 0.015f, 0.005f, 1.f);
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetShadowOffset(FVector2D(1.5f, 1.5f));
		Text->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, .62f));
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

	UImage* AddImage(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent, const FName Name,
		UTexture2D* Source, const FBox2f& UV, const FVector2D Position,
		const FVector2D Size, int32 ZOrder, bool bBox = false)
	{
		UImage* Image = Blueprint->WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
		Image->SetBrush(TextureBrush(Source, UV, bBox));
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
		UTexture2D* Source, const FBox2f& UV, const FVector2D BoundsPosition,
		const FVector2D BoundsSize, int32 ZOrder)
	{
		const FVector2D FittedSize = AspectFitSize(Source, BoundsSize);
		const FVector2D FittedPosition = BoundsPosition + (BoundsSize - FittedSize) * 0.5f;
		return AddImage(Blueprint, Parent, Name, Source, UV, FittedPosition, FittedSize, ZOrder);
	}

	UTextBlock* AddText(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent, const FName Name,
		const FText& Value, int32 FontSize, const FLinearColor& Color,
		const FVector2D Position, const FVector2D Size, int32 ZOrder,
		ETextJustify::Type Justification = ETextJustify::Center)
	{
		// UTextBlock does not expose a vertical justification option. Giving the text
		// the full Canvas slot therefore leaves glyphs top-aligned inside taller rows.
		// A dedicated overlay keeps the semantic TextBlock name intact while centering
		// its desired height inside the exact design rectangle at every ScaleBox size.
		const FName MountName(*FString::Printf(TEXT("%s_CenterMount"), *Name.ToString()));
		UOverlay* Mount = Blueprint->WidgetTree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(), MountName);
		Mount->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Place(Parent, Mount, Position, Size, ZOrder);

		UTextBlock* Text = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Text->SetText(Value);
		StyleText(Text, FontSize, Color, Justification);
		Mount->AddChildToOverlay(Text);
		UOverlaySlot* TextSlot = CastChecked<UOverlaySlot>(Text->Slot);
		TextSlot->SetHorizontalAlignment(HAlign_Fill);
		TextSlot->SetVerticalAlignment(VAlign_Center);
		return Text;
	}

	UBorder* AddRule(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent, const FName Name,
		const FVector2D Position, const FVector2D Size, int32 ZOrder)
	{
		UBorder* Rule = Blueprint->WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
		Rule->SetBrushColor(FLinearColor(.31f, .15f, .045f, .5f));
		Rule->SetPadding(FMargin(0.f));
		Rule->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Place(Parent, Rule, Position, Size, ZOrder);
		return Rule;
	}

	UButton* AddTransparentButton(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent,
		const FName Name, const FVector2D Position, const FVector2D Size, int32 ZOrder)
	{
		UButton* Button = Blueprint->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		FButtonStyle Style;
		FSlateBrush Empty;
		Empty.DrawAs = ESlateBrushDrawType::NoDrawType;
		Style.SetNormal(Empty);
		Style.SetHovered(Empty);
		Style.SetPressed(Empty);
		Style.SetDisabled(Empty);
		Button->SetStyle(Style);
		Place(Parent, Button, Position, Size, ZOrder);
		Blueprint->OnVariableAdded(Button->GetFName());
		return Button;
	}

	UWidgetBlueprint* FindOrCreateBlueprint()
	{
		if (UWidgetBlueprint* Existing = LoadObject<UWidgetBlueprint>(nullptr, AssetPath))
		{
			return Existing;
		}

		UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
		Factory->ParentClass = UCombatResultOverlayWidget::StaticClass();
		FAssetToolsModule& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		return Cast<UWidgetBlueprint>(AssetTools.Get().CreateAsset(
			AssetName, PackagePath, UWidgetBlueprint::StaticClass(), Factory));
	}

	void Build()
	{
		// Resolve every new hard dependency before touching the existing WidgetTree.
		// A missing import must fail without leaving the currently open asset empty.
		UTexture2D* OuterFrame = Texture(TEXT("/Game/UI/ResultBoards/Art/T_DF_BoardBlank_0809.T_DF_BoardBlank_0809"));
		UTexture2D* TitleBanner = Texture(TEXT("/Game/UI/ResultBoards/Art/T_DF_RibbonBlank_0809.T_DF_RibbonBlank_0809"));
		UTexture2D* MercenaryCard = Texture(TEXT("/Game/UI/ResultBoards/Art/T_DF_PortraitCardBlank_0809.T_DF_PortraitCardBlank_0809"));
		UTexture2D* Secondary = Texture(TEXT("/Game/UI/ResultBoards/Art/T_UI_ButtonSecondaryBlank_0809.T_UI_ButtonSecondaryBlank_0809"));
		UTexture2D* Knight = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Knight.T_MB_HireIcon_Knight"));
		UTexture2D* Rogue = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Rogue.T_MB_HireIcon_Rogue"));
		UTexture2D* Mage = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Mage.T_MB_HireIcon_Mage"));

		UWidgetBlueprint* Blueprint = FindOrCreateBlueprint();
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("RD_COMBAT_DEFEAT_BUILD could not create %s"), AssetPath);
			return;
		}

		Blueprint->Modify();
		Blueprint->WidgetTree->Modify();
		// The 0809 board changes some legacy mount classes. Removing the old root
		// releases all prior widget objects before names are reused by another class.
		// DeleteWidgets structurally compiles immediately, so use the neutral UUserWidget
		// parent during that one transient compile to avoid false BindWidget errors.
		if (UWidget* PreviousRoot = Blueprint->WidgetTree->RootWidget)
		{
			Blueprint->ParentClass = UUserWidget::StaticClass();
			TSet<UWidget*> PreviousWidgets;
			PreviousWidgets.Add(PreviousRoot);
			FWidgetBlueprintEditorUtils::DeleteWidgets(
				Blueprint,
				MoveTemp(PreviousWidgets),
				FWidgetBlueprintEditorUtils::EDeleteWidgetWarningType::DeleteSilently);
		}
		Blueprint->ParentClass = UCombatResultOverlayWidget::StaticClass();

		UOverlay* Root = Blueprint->WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("DefeatViewportRoot"));
		Blueprint->WidgetTree->RootWidget = Root;

		UBorder* Dimmer = Blueprint->WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BattlefieldDimmer"));
		Dimmer->SetBrushColor(FLinearColor(0.005f, 0.008f, 0.012f, .78f));
		Root->AddChildToOverlay(Dimmer);
		CastChecked<UOverlaySlot>(Dimmer->Slot)->SetHorizontalAlignment(HAlign_Fill);
		CastChecked<UOverlaySlot>(Dimmer->Slot)->SetVerticalAlignment(VAlign_Fill);

		UScaleBox* Scale = Blueprint->WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("DefeatResponsiveScale"));
		Scale->SetStretch(EStretch::ScaleToFit);
		Scale->SetStretchDirection(EStretchDirection::Both);
		Scale->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		Root->AddChildToOverlay(Scale);
		CastChecked<UOverlaySlot>(Scale->Slot)->SetHorizontalAlignment(HAlign_Fill);
		CastChecked<UOverlaySlot>(Scale->Slot)->SetVerticalAlignment(VAlign_Fill);

		USizeBox* DesignSize = Blueprint->WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DefeatDesignSize"));
		DesignSize->SetWidthOverride(1536.f);
		DesignSize->SetHeightOverride(864.f);
		Scale->AddChild(DesignSize);
		UCanvasPanel* Canvas = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("DefeatDesignCanvas"));
		DesignSize->SetContent(Canvas);

		const FBox2f FullUV(FVector2f::ZeroVector, FVector2f(1.f, 1.f));

		// 0809 확정 시안은 한 장의 나무/양피지 보드를 그대로 사용한다. 생성 원본은
		// 이미 투명 여백까지 정리됐으므로 atlas UV crop이나 9-slice를 적용하지 않는다.
		AddAspectImage(Blueprint, Canvas, TEXT("DefeatOuterFrame"), OuterFrame, FullUV,
			FVector2D(284.f, 24.f), FVector2D(968.f, 760.f), 0);
		AddAspectImage(Blueprint, Canvas, TEXT("DefeatTitleBanner"), TitleBanner, FullUV,
			FVector2D(520.f, 0.f), FVector2D(496.f, 148.f), 3);
		AddText(Blueprint, Canvas, TEXT("DefeatTitleText"), NSLOCTEXT("CombatDefeat", "Title", "패배"),
			50, DefeatTextColor, FVector2D(520.f, 0.f), FVector2D(496.f, 148.f), 4);
		UTextBlock* SubtitleText = AddText(Blueprint, Canvas, TEXT("DefeatSubtitleText"),
			NSLOCTEXT("CombatDefeat", "Subtitle", "용병단이 전투에서 패배했습니다"),
			23, DefeatTextColor, FVector2D(420.f, 112.f), FVector2D(696.f, 34.f), 4);
		// 이름은 기존 자동화/디자이너 호환을 위해 유지하되, 확정 시안에는 부제가 없다.
		SubtitleText->SetVisibility(ESlateVisibility::Collapsed);

		UTexture2D* PreviewPortraits[] = { Knight, Rogue, Mage };
		for (int32 Index = 0; Index < 3; ++Index)
		{
			const float X = 449.f + Index * 213.f;
			UCanvasPanel* CardMount = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
				UCanvasPanel::StaticClass(),
				FName(*FString::Printf(TEXT("DefeatCardFrame_%dMount"), Index)));
			CardMount->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			Place(Canvas, CardMount, FVector2D(X, 155.f), FVector2D(212.f, 250.f), 1);

			const FVector2D CardBounds(212.f, 250.f);
			const FVector2D CardArtSize = AspectFitSize(MercenaryCard, CardBounds);
			const FVector2D CardArtOffset = (CardBounds - CardArtSize) * 0.5f;
			AddImage(Blueprint, CardMount, FName(*FString::Printf(TEXT("DefeatCardFrame_%d"), Index)),
				MercenaryCard, FullUV, CardArtOffset, CardArtSize, 0);
			UImage* Portrait = AddAspectImage(Blueprint, CardMount,
				FName(*FString::Printf(TEXT("mPartyPortrait%d"), Index)), PreviewPortraits[Index], FullUV,
				CardArtOffset + FVector2D(38.f, 29.f), FVector2D(136.f, 136.f), 1);
			Portrait->SetColorAndOpacity(FLinearColor(.82f, .82f, .82f, 1.f));
			Blueprint->OnVariableAdded(Portrait->GetFName());
			UTextBlock* StatusText = AddText(Blueprint, CardMount,
				FName(*FString::Printf(TEXT("DefeatStatusText_%d"), Index)),
				NSLOCTEXT("CombatDefeat", "Incapacitated", "전투 불능"), 20,
				DefeatTextColor, CardArtOffset + FVector2D(17.f, 194.f),
				FVector2D(178.f, 36.f), 2);
			// 레퍼런스 카드에는 상태 문구가 없다. 이름은 기존 테스트/디자이너
			// 호환을 위해 남기되, 카드 위에 임의 문구를 추가로 노출하지 않는다.
			StatusText->SetVisibility(ESlateVisibility::Collapsed);
		}

		// 예전 Summary 이미지를 다시 그리면 보드 안에 양피지가 한 겹 더 생긴다. 기존
		// 위젯 이름만 무그림 마운트로 보존하고, 정보는 보드 양피지에 직접 놓는다.
		UImage* SummaryPanel = Blueprint->WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), TEXT("DefeatSummaryPanel"));
		FSlateBrush EmptySummaryBrush;
		EmptySummaryBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
		SummaryPanel->SetBrush(EmptySummaryBrush);
		SummaryPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Place(Canvas, SummaryPanel, FVector2D(432.f, 418.f), FVector2D(672.f, 188.f), 1);

		UTextBlock* LocationText = AddText(Blueprint, Canvas, TEXT("mLocationText"),
			NSLOCTEXT("CombatDefeat", "LocationPreview", "현재 전투 지역 · 2 라운드"), 21, DefeatTextColor,
			FVector2D(449.f, 424.f), FVector2D(638.f, 34.f), 3);
		AddRule(Blueprint, Canvas, TEXT("DefeatSummaryTopRule"),
			FVector2D(449.f, 462.f), FVector2D(638.f, 2.f), 2);

		AddText(Blueprint, Canvas, TEXT("DefeatEnemyLabel"),
			NSLOCTEXT("CombatDefeat", "EnemyLabel", "처치"), 20, DefeatTextColor,
			FVector2D(449.f, 480.f), FVector2D(212.f, 30.f), 3);
		AddText(Blueprint, Canvas, TEXT("DefeatGoldLabel"),
			NSLOCTEXT("CombatDefeat", "GoldLabel", "획득 골드"), 20, DefeatTextColor,
			FVector2D(662.f, 480.f), FVector2D(212.f, 30.f), 3);
		AddText(Blueprint, Canvas, TEXT("DefeatSurvivorLabel"),
			NSLOCTEXT("CombatDefeat", "SurvivorLabel", "생존"), 20, DefeatTextColor,
			FVector2D(875.f, 480.f), FVector2D(212.f, 30.f), 3);
		AddRule(Blueprint, Canvas, TEXT("DefeatSummaryLeftDivider"),
			FVector2D(661.f, 479.f), FVector2D(1.f, 84.f), 2);
		AddRule(Blueprint, Canvas, TEXT("DefeatSummaryRightDivider"),
			FVector2D(874.f, 479.f), FVector2D(1.f, 84.f), 2);

		UTextBlock* RoundText = AddText(Blueprint, Canvas, TEXT("mRoundText"),
			NSLOCTEXT("CombatDefeat", "RoundPreview", "2 라운드"), 21, DefeatTextColor,
			FVector2D(449.f, 424.f), FVector2D(638.f, 34.f), 3);
		UTextBlock* EnemyText = AddText(Blueprint, Canvas, TEXT("mEnemyText"),
			NSLOCTEXT("CombatDefeat", "EnemyPreview", "12"), 34, DefeatTextColor,
			FVector2D(449.f, 517.f), FVector2D(212.f, 46.f), 3);
		UTextBlock* GoldText = AddText(Blueprint, Canvas, TEXT("mGoldText"),
			NSLOCTEXT("CombatDefeat", "GoldPreview", "0"), 34, DefeatTextColor,
			FVector2D(662.f, 517.f), FVector2D(212.f, 46.f), 3);
		AddText(Blueprint, Canvas, TEXT("DefeatSurvivorValue"),
			NSLOCTEXT("CombatDefeat", "SurvivorPreview", "0 / 3"), 34, DefeatTextColor,
			FVector2D(875.f, 517.f), FVector2D(212.f, 46.f), 3);
		AddRule(Blueprint, Canvas, TEXT("DefeatSummaryBottomRule"),
			FVector2D(449.f, 575.f), FVector2D(638.f, 2.f), 2);

		UTextBlock* ExpText = AddText(Blueprint, Canvas, TEXT("mExpText"),
			NSLOCTEXT("CombatDefeat", "ExpPreview", "+0"), 21, DefeatTextColor,
			FVector2D(449.f, 580.f), FVector2D(638.f, 34.f), 3);
		RoundText->SetVisibility(ESlateVisibility::Collapsed);
		ExpText->SetVisibility(ESlateVisibility::Collapsed);
		Blueprint->OnVariableAdded(LocationText->GetFName());
		Blueprint->OnVariableAdded(RoundText->GetFName());
		Blueprint->OnVariableAdded(EnemyText->GetFName());
		Blueprint->OnVariableAdded(GoldText->GetFName());
		Blueprint->OnVariableAdded(ExpText->GetFName());

		// Defeat is terminal for this roguelike run. Keep a single, unambiguous CTA
		// centered below the board; the art is center-fitted at native ratio while
		// the text and transparent hit target share the full, generous rectangle.
		const FVector2D TitleButtonPosition(558.f, 704.f);
		const FVector2D TitleButtonBounds(420.f, 160.f);
		AddAspectImage(Blueprint, Canvas, TEXT("mTitleButtonArt"), Secondary, FullUV,
			TitleButtonPosition, TitleButtonBounds, 4);
		AddText(Blueprint, Canvas, TEXT("mTitleButtonText"), NSLOCTEXT("CombatDefeat", "BackToTitle", "타이틀로 돌아가기"),
			DefeatButtonFontSize, DefeatTextColor,
			TitleButtonPosition, TitleButtonBounds, 5);
		AddTransparentButton(Blueprint, Canvas, TEXT("mTitleButton"),
			TitleButtonPosition, TitleButtonBounds, 6);

		// UE 5.7 expects every live widget to have a stable variable GUID. Previous
		// widget GUIDs are removed by DeleteWidgets without touching animation GUIDs.
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

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Blueprint->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
		if (!UPackage::SavePackage(Blueprint->GetPackage(), Blueprint, *Filename, FSavePackageArgs()))
		{
			UE_LOG(LogTemp, Error, TEXT("RD_COMBAT_DEFEAT_BUILD save failed"));
			return;
		}
		UE_LOG(LogTemp, Display, TEXT("RD_COMBAT_DEFEAT_BUILD success asset=%s responsive=1536x864"), AssetPath);
	}
}

void RegisterCombatDefeatWidgetBuilderCommands()
{
	using namespace CombatDefeatWidgetBuilder;
	BuildCommand = MakeUnique<FAutoConsoleCommand>(TEXT("RD.Editor.BuildCombatDefeat"),
		TEXT("Create the responsive MARCHBOUND combat defeat WBP."),
		FConsoleCommandDelegate::CreateStatic(&Build));
}

void UnregisterCombatDefeatWidgetBuilderCommands()
{
	using namespace CombatDefeatWidgetBuilder;
	BuildCommand.Reset();
}
