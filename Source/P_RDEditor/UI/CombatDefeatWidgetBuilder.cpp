#include "UI/CombatDefeatWidgetBuilder.h"

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
#include "Engine/Texture2D.h"
#include "WidgetBlueprintFactory.h"
#include "HAL/IConsoleManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UI/CombatResultOverlayWidget.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"

namespace CombatDefeatWidgetBuilder
{
	constexpr TCHAR PackagePath[] = TEXT("/Game/UI/CombatResult");
	constexpr TCHAR AssetName[] = TEXT("WBP_CombatDefeat");
	constexpr TCHAR AssetPath[] = TEXT("/Game/UI/CombatResult/WBP_CombatDefeat.WBP_CombatDefeat");
	TUniquePtr<FAutoConsoleCommand> BuildCommand;

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
		Brush.ImageSize = Source != nullptr
			? FVector2D(Source->GetSizeX(), Source->GetSizeY()) : FVector2D::ZeroVector;
		Brush.SetUVRegion(UV);
		return Brush;
	}

	void StyleText(UTextBlock* Text, int32 Size, const FLinearColor& Color,
		ETextJustify::Type Justification = ETextJustify::Center)
	{
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
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

	UTextBlock* AddText(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent, const FName Name,
		const FText& Value, int32 FontSize, const FLinearColor& Color,
		const FVector2D Position, const FVector2D Size, int32 ZOrder,
		ETextJustify::Type Justification = ETextJustify::Center)
	{
		UTextBlock* Text = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Text->SetText(Value);
		StyleText(Text, FontSize, Color, Justification);
		Place(Parent, Text, Position, Size, ZOrder);
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
		UWidgetBlueprint* Blueprint = FindOrCreateBlueprint();
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("RD_COMBAT_DEFEAT_BUILD could not create %s"), AssetPath);
			return;
		}

		Blueprint->Modify();
		Blueprint->WidgetTree->Modify();
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

		UTexture2D* OuterFrame = Texture(TEXT("/Game/UI/Art/Marchbound/Defeat/T_MB_Defeat_OuterFrame.T_MB_Defeat_OuterFrame"));
		UTexture2D* TitleBanner = Texture(TEXT("/Game/UI/Art/Marchbound/Defeat/T_MB_Defeat_TitleBanner.T_MB_Defeat_TitleBanner"));
		UTexture2D* MercenaryCard = Texture(TEXT("/Game/UI/Art/Marchbound/Defeat/T_MB_Defeat_MercenaryCard.T_MB_Defeat_MercenaryCard"));
		UTexture2D* Summary = Texture(TEXT("/Game/UI/Art/Marchbound/Defeat/T_MB_Defeat_BattleSummary.T_MB_Defeat_BattleSummary"));
		UTexture2D* Secondary = Texture(TEXT("/Game/UI/Art/Marchbound/Defeat/T_MB_Defeat_ButtonSecondary.T_MB_Defeat_ButtonSecondary"));
		UTexture2D* Primary = Texture(TEXT("/Game/UI/Art/Marchbound/Defeat/T_MB_Defeat_ButtonPrimary.T_MB_Defeat_ButtonPrimary"));
		UTexture2D* Knight = Texture(TEXT("/Game/UI/Art/Marchbound/Mercenaries/T_MB_HireIcon_Knight.T_MB_HireIcon_Knight"));
		UTexture2D* Rogue = Texture(TEXT("/Game/UI/Art/Marchbound/Mercenaries/T_MB_HireIcon_Rogue.T_MB_HireIcon_Rogue"));
		UTexture2D* Mage = Texture(TEXT("/Game/UI/Art/Marchbound/Mercenaries/T_MB_HireIcon_Mage.T_MB_HireIcon_Mage"));

		const FBox2f OuterUV(FVector2f(.089319f, .064227f), FVector2f(.909761f, .928867f));
		const FBox2f TitleUV(FVector2f(.033298f, .282840f), FVector2f(.968851f, .665089f));
		const FBox2f CardUV(FVector2f(.146168f, .141227f), FVector2f(.852941f, .855920f));
		const FBox2f SummaryUV(FVector2f(.087919f, .190223f), FVector2f(.912081f, .809777f));
		const FBox2f SecondaryUV(FVector2f(.076722f, .203410f), FVector2f(.922756f, .799026f));
		const FBox2f PrimaryUV(FVector2f(.069193f, .232639f), FVector2f(.930807f, .769676f));
		const FBox2f FullUV(FVector2f::ZeroVector, FVector2f(1.f, 1.f));

		AddImage(Blueprint, Canvas, TEXT("DefeatOuterFrame"), OuterFrame, OuterUV,
			FVector2D(284.f, 18.f), FVector2D(968.f, 828.f), 0, true);
		AddImage(Blueprint, Canvas, TEXT("DefeatTitleBanner"), TitleBanner, TitleUV,
			FVector2D(398.f, 18.f), FVector2D(740.f, 137.f), 3);
		AddText(Blueprint, Canvas, TEXT("DefeatTitleText"), NSLOCTEXT("CombatDefeat", "Title", "원정 실패"),
			50, FLinearColor(1.f, .91f, .72f, 1.f), FVector2D(448.f, 42.f), FVector2D(640.f, 68.f), 4);
		AddText(Blueprint, Canvas, TEXT("DefeatSubtitleText"), NSLOCTEXT("CombatDefeat", "Subtitle", "용병단이 전투에서 패배했습니다"),
			23, FLinearColor(.16f, .075f, .025f, 1.f), FVector2D(420.f, 142.f), FVector2D(696.f, 38.f), 4);

		UTexture2D* PreviewPortraits[] = { Knight, Rogue, Mage };
		for (int32 Index = 0; Index < 3; ++Index)
		{
			const float X = 432.f + Index * 224.f;
			AddImage(Blueprint, Canvas, FName(*FString::Printf(TEXT("DefeatCardFrame_%d"), Index)),
				MercenaryCard, CardUV, FVector2D(X, 188.f), FVector2D(220.f, 260.f), 1);
			UImage* Portrait = AddImage(Blueprint, Canvas,
				FName(*FString::Printf(TEXT("mPartyPortrait%d"), Index)), PreviewPortraits[Index], FullUV,
				FVector2D(X + 31.f, 215.f), FVector2D(158.f, 154.f), 2);
			Portrait->SetColorAndOpacity(FLinearColor(.82f, .82f, .82f, 1.f));
			Blueprint->OnVariableAdded(Portrait->GetFName());
			AddText(Blueprint, Canvas, FName(*FString::Printf(TEXT("DefeatStatusText_%d"), Index)),
				NSLOCTEXT("CombatDefeat", "Incapacitated", "전투 불능"), 21,
				FLinearColor(.93f, .12f, .06f, 1.f), FVector2D(X + 24.f, 387.f), FVector2D(172.f, 42.f), 4);
		}

		AddImage(Blueprint, Canvas, TEXT("DefeatSummaryPanel"), Summary, SummaryUV,
			FVector2D(405.f, 458.f), FVector2D(726.f, 225.f), 1, true);
		const FLinearColor Ink(.17f, .08f, .025f, 1.f);
		UTextBlock* LocationText = AddText(Blueprint, Canvas, TEXT("mLocationText"),
			NSLOCTEXT("CombatDefeat", "LocationPreview", "도달 지점    현재 전투 지역"), 21, Ink,
			FVector2D(458.f, 476.f), FVector2D(620.f, 34.f), 3, ETextJustify::Left);
		UTextBlock* RoundText = AddText(Blueprint, Canvas, TEXT("mRoundText"),
			NSLOCTEXT("CombatDefeat", "RoundPreview", "진행 라운드    7 라운드"), 21, Ink,
			FVector2D(458.f, 515.f), FVector2D(620.f, 34.f), 3, ETextJustify::Left);
		UTextBlock* EnemyText = AddText(Blueprint, Canvas, TEXT("mEnemyText"),
			NSLOCTEXT("CombatDefeat", "EnemyPreview", "처치한 몬스터    12"), 21, Ink,
			FVector2D(458.f, 554.f), FVector2D(620.f, 34.f), 3, ETextJustify::Left);
		UTextBlock* GoldText = AddText(Blueprint, Canvas, TEXT("mGoldText"),
			NSLOCTEXT("CombatDefeat", "GoldPreview", "획득 골드    0"), 21, Ink,
			FVector2D(458.f, 593.f), FVector2D(620.f, 34.f), 3, ETextJustify::Left);
		UTextBlock* ExpText = AddText(Blueprint, Canvas, TEXT("mExpText"),
			NSLOCTEXT("CombatDefeat", "ExpPreview", "획득 경험치    +0"), 21, Ink,
			FVector2D(458.f, 632.f), FVector2D(620.f, 34.f), 3, ETextJustify::Left);
		Blueprint->OnVariableAdded(LocationText->GetFName());
		Blueprint->OnVariableAdded(RoundText->GetFName());
		Blueprint->OnVariableAdded(EnemyText->GetFName());
		Blueprint->OnVariableAdded(GoldText->GetFName());
		Blueprint->OnVariableAdded(ExpText->GetFName());

		AddImage(Blueprint, Canvas, TEXT("mTitleButtonArt"), Secondary, SecondaryUV,
			FVector2D(390.f, 708.f), FVector2D(360.f, 112.f), 2);
		AddText(Blueprint, Canvas, TEXT("mTitleButtonText"), NSLOCTEXT("CombatDefeat", "BackToTitle", "타이틀로 돌아가기"),
			30, FLinearColor(1.f, .91f, .72f, 1.f), FVector2D(420.f, 738.f), FVector2D(300.f, 50.f), 3);
		AddTransparentButton(Blueprint, Canvas, TEXT("mTitleButton"),
			FVector2D(390.f, 708.f), FVector2D(360.f, 112.f), 5);

		AddImage(Blueprint, Canvas, TEXT("mRetryButtonArt"), Primary, PrimaryUV,
			FVector2D(786.f, 708.f), FVector2D(360.f, 112.f), 2);
		AddText(Blueprint, Canvas, TEXT("mRetryButtonText"), NSLOCTEXT("CombatDefeat", "Retry", "재도전"),
			34, FLinearColor::White, FVector2D(816.f, 736.f), FVector2D(300.f, 54.f), 3);
		AddTransparentButton(Blueprint, Canvas, TEXT("mRetryButton"),
			FVector2D(786.f, 708.f), FVector2D(360.f, 112.f), 5);

		// UE 5.7 expects every designer-created widget to have a stable variable GUID,
		// including decorative widgets that are not exposed through BindWidget.
		Blueprint->WidgetTree->ForEachWidget([Blueprint](UWidget* Widget)
		{
			if (Widget != nullptr
				&& Blueprint->WidgetVariableNameToGuidMap.Contains(Widget->GetFName()) == false)
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
