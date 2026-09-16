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
	constexpr int32 DefeatButtonFontSize = 36;
	TUniquePtr<FAutoConsoleCommand> BuildCommand;

	const FLinearColor DefeatTextColor(.97f, .89f, .72f, 1.f);
	const FLinearColor InkColor(.055f, .023f, .004f, 1.f);

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
		const bool bLightText = Color.R > .5f;
		Font.OutlineSettings.OutlineSize = bLightText ? (Size >= 60 ? 3 : 2) : 0;
		Font.OutlineSettings.OutlineColor = FLinearColor(0.03f, 0.015f, 0.005f, 1.f);
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetShadowOffset(bLightText ? FVector2D(1.5f, 1.5f) : FVector2D::ZeroVector);
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
		UTexture2D* Artwork = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/DefeatClassic_20260916/T_DefeatClassicBlank_v1.T_DefeatClassicBlank_v1"));

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
		Dimmer->SetBrushColor(FLinearColor(.022f, .032f, .025f, 1.f));
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
		DesignSize->SetWidthOverride(1672.f);
		DesignSize->SetHeightOverride(941.f);
		Scale->AddChild(DesignSize);
		UCanvasPanel* Canvas = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("DefeatDesignCanvas"));
		DesignSize->SetContent(Canvas);

		const FBox2f FullUV(FVector2f::ZeroVector, FVector2f(1.f, 1.f));

		// Approved composition is a text-free art plate. All copy and values are native
		// widgets, so localized labels and live run data never become baked pixels.
		AddImage(Blueprint, Canvas, TEXT("DefeatArtwork"), Artwork, FullUV,
			FVector2D::ZeroVector, FVector2D(1672.f, 941.f), 0);
		AddText(Blueprint, Canvas, TEXT("DefeatTitleText"), NSLOCTEXT("CombatDefeat", "Title", "패배"),
			80, DefeatTextColor, FVector2D(590.f, 61.f), FVector2D(492.f, 126.f), 1);
		AddText(Blueprint, Canvas, TEXT("DefeatSubtitleText"),
			NSLOCTEXT("CombatDefeat", "ExpeditionEnded", "이번 원정이 종료되었습니다"),
			38, InkColor, FVector2D(376.f, 247.f), FVector2D(920.f, 67.f), 1);
		UTextBlock* LocationText = AddText(Blueprint, Canvas, TEXT("mLocationText"),
			NSLOCTEXT("CombatDefeat", "Location", "현재 전투 지역"), 31, DefeatTextColor,
			FVector2D(609.f, 469.f), FVector2D(454.f, 59.f), 1);
		AddText(Blueprint, Canvas, TEXT("DefeatRoundLabel"),
			NSLOCTEXT("CombatDefeat", "RoundLabel", "진행 라운드"), 25, InkColor,
			FVector2D(522.f, 558.f), FVector2D(271.f, 40.f), 1);
		AddText(Blueprint, Canvas, TEXT("DefeatEnemyLabel"),
			NSLOCTEXT("CombatDefeat", "EnemiesDefeated", "처치한 적"), 25, InkColor,
			FVector2D(878.f, 558.f), FVector2D(271.f, 40.f), 1);
		UTextBlock* RoundText = AddText(Blueprint, Canvas, TEXT("mRoundText"),
			FText::AsNumber(1), 46, InkColor, FVector2D(535.f, 595.f), FVector2D(245.f, 77.f), 1);
		UTextBlock* EnemyText = AddText(Blueprint, Canvas, TEXT("mEnemyText"),
			FText::AsNumber(0), 46, InkColor, FVector2D(891.f, 595.f), FVector2D(245.f, 77.f), 1);
		Blueprint->OnVariableAdded(LocationText->GetFName());
		Blueprint->OnVariableAdded(RoundText->GetFName());
		Blueprint->OnVariableAdded(EnemyText->GetFName());
		const FVector2D TitleButtonPosition(548.f, 734.f);
		const FVector2D TitleButtonBounds(576.f, 120.f);
		AddText(Blueprint, Canvas, TEXT("mTitleButtonText"),
			NSLOCTEXT("CombatDefeat", "BackToTitle", "타이틀로 돌아가기"),
			DefeatButtonFontSize, DefeatTextColor, TitleButtonPosition, TitleButtonBounds, 2);
		AddTransparentButton(Blueprint, Canvas, TEXT("mTitleButton"),
			TitleButtonPosition, TitleButtonBounds, 3);

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
		UE_LOG(LogTemp, Display, TEXT("RD_COMBAT_DEFEAT_BUILD success asset=%s responsive=1672x941"), AssetPath);
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
