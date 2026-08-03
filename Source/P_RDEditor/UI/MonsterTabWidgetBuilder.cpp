#include "UI/MonsterTabWidgetBuilder.h"

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
#include "Components/ProgressBar.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "HAL/IConsoleManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"

namespace MonsterTabWidgetBuilder
{
	constexpr TCHAR PackagePath[] = TEXT("/Game/UI/MonsterTab");
	constexpr TCHAR AssetName[] = TEXT("WBP_MonsterTab_Marchbound");
	constexpr TCHAR AssetPath[] =
		TEXT("/Game/UI/MonsterTab/WBP_MonsterTab_Marchbound.WBP_MonsterTab_Marchbound");
	TUniquePtr<FAutoConsoleCommand> BuildCommand;

	UTexture2D* Texture(const TCHAR* Path)
	{
		UTexture2D* Result = LoadObject<UTexture2D>(nullptr, Path);
		checkf(Result != nullptr, TEXT("Missing monster-tab texture: %s"), Path);
		return Result;
	}

	void Place(UCanvasPanel* Parent, UWidget* Child, const FVector2D Position,
		const FVector2D Size, const int32 ZOrder)
	{
		UCanvasPanelSlot* Slot = Parent->AddChildToCanvas(Child);
		Slot->SetAnchors(FAnchors(0.0f));
		Slot->SetAlignment(FVector2D::ZeroVector);
		Slot->SetAutoSize(false);
		Slot->SetPosition(Position);
		Slot->SetSize(Size);
		Slot->SetZOrder(ZOrder);
	}

	void Expose(UWidgetBlueprint* Blueprint, UWidget* Widget)
	{
		check(Blueprint != nullptr && Widget != nullptr);
		Blueprint->OnVariableAdded(Widget->GetFName());
	}

	void StyleText(UTextBlock* Text, const int32 Size,
		const FLinearColor Color, const ETextJustify::Type Justification = ETextJustify::Center)
	{
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Font.OutlineSettings.OutlineSize = Size >= 27 ? 1 : 0;
		Font.OutlineSettings.OutlineColor = FLinearColor(0.02f, 0.01f, 0.0f, 0.95f);
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetJustification(Justification);
		Text->SetShadowOffset(FVector2D(1.5f, 1.5f));
		Text->SetShadowColorAndOpacity(FLinearColor(0.02f, 0.01f, 0.0f, 0.8f));
		Text->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	UTextBlock* AddText(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent,
		const FName Name, const FText Value, const int32 FontSize,
		const FVector2D Position, const FVector2D Size, const int32 ZOrder,
		const FLinearColor Color = FLinearColor(0.12f, 0.065f, 0.025f, 1.0f),
		const ETextJustify::Type Justification = ETextJustify::Center)
	{
		UTextBlock* Text = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), Name);
		Text->SetText(Value);
		StyleText(Text, FontSize, Color, Justification);
		Place(Parent, Text, Position, Size, ZOrder);
		Expose(Blueprint, Text);
		return Text;
	}

	UImage* AddImage(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent,
		const FName Name, UTexture2D* Source, const FVector2D Position,
		const FVector2D Size, const int32 ZOrder, const bool bExpose = false)
	{
		UImage* Image = Blueprint->WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), Name);
		Image->SetBrushFromTexture(Source, false);
		Image->SetColorAndOpacity(FLinearColor::White);
		Image->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Place(Parent, Image, Position, Size, ZOrder);
		if (bExpose)
		{
			Expose(Blueprint, Image);
		}
		return Image;
	}

	UButton* AddTransparentButton(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent,
		const FName Name, const FVector2D Position, const FVector2D Size, const int32 ZOrder)
	{
		UButton* Button = Blueprint->WidgetTree->ConstructWidget<UButton>(
			UButton::StaticClass(), Name);
		FButtonStyle Style;
		FSlateBrush Empty;
		Empty.DrawAs = ESlateBrushDrawType::NoDrawType;
		Style.SetNormal(Empty);
		Style.SetHovered(Empty);
		Style.SetPressed(Empty);
		Style.SetDisabled(Empty);
		Button->SetStyle(Style);
		Button->SetBackgroundColor(FLinearColor::Transparent);
		Button->SetColorAndOpacity(FLinearColor::White);
		Place(Parent, Button, Position, Size, ZOrder);
		Expose(Blueprint, Button);
		return Button;
	}

	UBorder* AddDarkWell(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent,
		const FName Name, const FVector2D Position, const FVector2D Size, const int32 ZOrder)
	{
		UBorder* Border = Blueprint->WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), Name);
		Border->SetBrushColor(FLinearColor(0.055f, 0.028f, 0.012f, 0.88f));
		Border->SetPadding(FMargin(10.0f));
		Border->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Place(Parent, Border, Position, Size, ZOrder);
		Expose(Blueprint, Border);
		return Border;
	}

	UWidgetBlueprint* FindOrCreateBlueprint()
	{
		if (UWidgetBlueprint* Existing = LoadObject<UWidgetBlueprint>(nullptr, AssetPath))
		{
			return Existing;
		}

		UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
		Factory->ParentClass = UUserWidget::StaticClass();
		FAssetToolsModule& AssetTools =
			FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		return Cast<UWidgetBlueprint>(AssetTools.Get().CreateAsset(
			AssetName, PackagePath, UWidgetBlueprint::StaticClass(), Factory));
	}

	void Build()
	{
		UWidgetBlueprint* Blueprint = FindOrCreateBlueprint();
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("RD_MONSTER_TAB_BUILD could not create %s"), AssetPath);
			return;
		}

		Blueprint->Modify();
		Blueprint->WidgetTree->Modify();
		Blueprint->ParentClass = UUserWidget::StaticClass();

		UOverlay* Root = Blueprint->WidgetTree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(), TEXT("MonsterTabViewportRoot"));
		Blueprint->WidgetTree->RootWidget = Root;

		UBorder* Dimmer = Blueprint->WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("MonsterTabWorldDimmer"));
		Dimmer->SetBrushColor(FLinearColor(0.015f, 0.008f, 0.003f, 0.64f));
		Root->AddChildToOverlay(Dimmer);
		CastChecked<UOverlaySlot>(Dimmer->Slot)->SetHorizontalAlignment(HAlign_Fill);
		CastChecked<UOverlaySlot>(Dimmer->Slot)->SetVerticalAlignment(VAlign_Fill);

		UScaleBox* ScreenScale = Blueprint->WidgetTree->ConstructWidget<UScaleBox>(
			UScaleBox::StaticClass(), TEXT("MonsterTabScale"));
		ScreenScale->SetStretch(EStretch::ScaleToFit);
		ScreenScale->SetStretchDirection(EStretchDirection::Both);
		ScreenScale->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		Root->AddChildToOverlay(ScreenScale);
		CastChecked<UOverlaySlot>(ScreenScale->Slot)->SetHorizontalAlignment(HAlign_Fill);
		CastChecked<UOverlaySlot>(ScreenScale->Slot)->SetVerticalAlignment(VAlign_Fill);

		USizeBox* DesignSize = Blueprint->WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("MonsterTabDesignSize"));
		DesignSize->SetWidthOverride(1920.0f);
		DesignSize->SetHeightOverride(1080.0f);
		ScreenScale->AddChild(DesignSize);

		UCanvasPanel* Canvas = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("MonsterTabCanvas"));
		DesignSize->SetContent(Canvas);

		UTexture2D* BaseFrame = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/MonsterTab/T_MT_BaseFrame.T_MT_BaseFrame"));
		UTexture2D* RowNormal = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/MonsterTab/T_MT_RowNormal.T_MT_RowNormal"));
		UTexture2D* RowSelected = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/MonsterTab/T_MT_RowSelected.T_MT_RowSelected"));
		UTexture2D* BackButtonTexture = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Hire/T_MB_HireBackButton.T_MB_HireBackButton"));
		AddImage(Blueprint, Canvas, TEXT("MonsterTabBaseFrame"), BaseFrame,
			FVector2D::ZeroVector, FVector2D(1920.0f, 1080.0f), 0, true);

		UTextBlock* Title = AddText(Blueprint, Canvas, TEXT("MonsterTabTitleText"),
			NSLOCTEXT("MarchboundMonsterTab", "Title", "몬스터"), 58,
			FVector2D(355.0f, 80.0f), FVector2D(620.0f, 92.0f), 10,
			FLinearColor(1.0f, 0.93f, 0.78f, 1.0f));
		Title->SetShadowOffset(FVector2D(3.0f, 3.0f));

		AddImage(Blueprint, Canvas, TEXT("MonsterBackArt"), BackButtonTexture,
			FVector2D(1600.0f, 41.0f), FVector2D(270.0f, 106.0f), 30, true);
		AddText(Blueprint, Canvas, TEXT("MonsterBackText"),
			NSLOCTEXT("MarchboundMonsterTab", "Back", "뒤로"), 32,
			FVector2D(1600.0f, 53.0f), FVector2D(270.0f, 76.0f), 31,
			FLinearColor(1.0f, 0.93f, 0.78f, 1.0f));
		AddTransparentButton(Blueprint, Canvas, TEXT("MonsterBackButton"),
			FVector2D(1600.0f, 41.0f), FVector2D(270.0f, 106.0f), 32);

		const TCHAR* MonsterNames[3] = { TEXT("독수리"), TEXT("늑대인간"), TEXT("골렘") };
		const TCHAR* HeadPaths[3] = {
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Portraits/KK_Face_Enemy_Eagle_HeadV2.KK_Face_Enemy_Eagle_HeadV2"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Portraits/KK_Face_Enemy_Werewolf_HeadV2.KK_Face_Enemy_Werewolf_HeadV2"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Portraits/KK_Face_Enemy_Golem_HeadV2.KK_Face_Enemy_Golem_HeadV2")
		};
		for (int32 Index = 0; Index < 3; ++Index)
		{
			UCanvasPanel* Row = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
				UCanvasPanel::StaticClass(),
				FName(*FString::Printf(TEXT("MonsterRow_%d"), Index)));
			Place(Canvas, Row, FVector2D(34.0f, 270.0f + 225.0f * Index),
				FVector2D(455.0f, 164.0f), 20);
			Expose(Blueprint, Row);

			AddImage(Blueprint, Row,
				FName(*FString::Printf(TEXT("MonsterRowNormal_%d"), Index)), RowNormal,
				FVector2D::ZeroVector, FVector2D(455.0f, 164.0f), 0, true);
			UImage* Selected = AddImage(Blueprint, Row,
				FName(*FString::Printf(TEXT("MonsterRowSelected_%d"), Index)), RowSelected,
				FVector2D::ZeroVector, FVector2D(455.0f, 164.0f), 2, true);
			Selected->SetVisibility(Index == 1
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed);

			UImage* Portrait = AddImage(Blueprint, Row,
				FName(*FString::Printf(TEXT("MonsterRowPortrait_%d"), Index)),
				Texture(HeadPaths[Index]), FVector2D(32.0f, 18.0f),
				FVector2D(126.0f, 126.0f), 6, true);
			Portrait->SetClipping(EWidgetClipping::ClipToBoundsAlways);
			AddText(Blueprint, Row,
				FName(*FString::Printf(TEXT("MonsterRowName_%d"), Index)),
				FText::FromString(MonsterNames[Index]), 35,
				FVector2D(172.0f, 48.0f), FVector2D(240.0f, 68.0f), 8);
			AddTransparentButton(Blueprint, Row,
				FName(*FString::Printf(TEXT("MonsterRowButton_%d"), Index)),
				FVector2D::ZeroVector, FVector2D(455.0f, 164.0f), 20);
		}

		UScaleBox* PortraitScale = Blueprint->WidgetTree->ConstructWidget<UScaleBox>(
			UScaleBox::StaticClass(), TEXT("MonsterDetailPortraitScale"));
		PortraitScale->SetStretch(EStretch::ScaleToFit);
		PortraitScale->SetStretchDirection(EStretchDirection::Both);
		PortraitScale->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		Place(Canvas, PortraitScale, FVector2D(555.0f, 235.0f),
			FVector2D(485.0f, 665.0f), 15);
		UImage* DetailPortrait = Blueprint->WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), TEXT("MonsterDetailPortrait"));
		DetailPortrait->SetBrushFromTexture(Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Portraits/KK_Face_Enemy_Werewolf_ActionV3.KK_Face_Enemy_Werewolf_ActionV3")), false);
		DetailPortrait->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		PortraitScale->AddChild(DetailPortrait);
		Expose(Blueprint, DetailPortrait);

		AddText(Blueprint, Canvas, TEXT("MonsterCenterNameText"),
			NSLOCTEXT("MarchboundMonsterTab", "CenterWerewolf", "늑대인간"), 40,
			FVector2D(570.0f, 900.0f), FVector2D(455.0f, 70.0f), 20);

		AddText(Blueprint, Canvas, TEXT("MonsterDetailNameText"),
			NSLOCTEXT("MarchboundMonsterTab", "DetailWerewolf", "늑대인간"), 48,
			FVector2D(1160.0f, 150.0f), FVector2D(650.0f, 72.0f), 20);
		AddText(Blueprint, Canvas, TEXT("MonsterDetailTypeText"),
			NSLOCTEXT("MarchboundMonsterTab", "DetailType", "야수 · 근접"), 27,
			FVector2D(1160.0f, 220.0f), FVector2D(650.0f, 48.0f), 20);

		AddText(Blueprint, Canvas, TEXT("MonsterDetailHPLabel"), FText::FromString(TEXT("HP")), 32,
			FVector2D(1165.0f, 302.0f), FVector2D(90.0f, 54.0f), 20,
			FLinearColor(0.36f, 0.04f, 0.025f, 1.0f), ETextJustify::Left);
		UProgressBar* HPBar = Blueprint->WidgetTree->ConstructWidget<UProgressBar>(
			UProgressBar::StaticClass(), TEXT("MonsterDetailHPBar"));
		HPBar->SetPercent(1.0f);
		HPBar->SetFillColorAndOpacity(FLinearColor(0.74f, 0.04f, 0.025f, 1.0f));
		Place(Canvas, HPBar, FVector2D(1250.0f, 307.0f), FVector2D(530.0f, 44.0f), 20);
		Expose(Blueprint, HPBar);
		AddText(Blueprint, Canvas, TEXT("MonsterDetailHPText"), FText::FromString(TEXT("120 / 120")), 26,
			FVector2D(1260.0f, 309.0f), FVector2D(510.0f, 39.0f), 22,
			FLinearColor::White);
		AddText(Blueprint, Canvas, TEXT("MonsterDetailAPText"), FText::FromString(TEXT("AP  5")), 31,
			FVector2D(1170.0f, 375.0f), FVector2D(285.0f, 54.0f), 20);
		AddText(Blueprint, Canvas, TEXT("MonsterDetailSpeedText"),
			NSLOCTEXT("MarchboundMonsterTab", "SpeedPreview", "속도  4"), 31,
			FVector2D(1465.0f, 375.0f), FVector2D(285.0f, 54.0f), 20);

		AddText(Blueprint, Canvas, TEXT("MonsterSkillHeading"),
			NSLOCTEXT("MarchboundMonsterTab", "SkillHeading", "스킬"), 31,
			FVector2D(1170.0f, 450.0f), FVector2D(600.0f, 50.0f), 20);
		const TCHAR* SkillNames[4] = {
			TEXT("야수의 발톱"), TEXT("도약"), TEXT("포효"), TEXT("사냥 본능")
		};
		for (int32 Index = 0; Index < 4; ++Index)
		{
			const float X = 1155.0f + 310.0f * (Index % 2);
			const float Y = 510.0f + 132.0f * (Index / 2);
			AddDarkWell(Blueprint, Canvas,
				FName(*FString::Printf(TEXT("MonsterSkillBox_%d"), Index)),
				FVector2D(X, Y), FVector2D(292.0f, 116.0f), 20);
			AddText(Blueprint, Canvas,
				FName(*FString::Printf(TEXT("MonsterSkillName_%d"), Index)),
				FText::FromString(SkillNames[Index]), 25,
				FVector2D(X + 18.0f, Y + 31.0f), FVector2D(256.0f, 54.0f), 22,
				FLinearColor(1.0f, 0.91f, 0.73f, 1.0f));
		}

		AddText(Blueprint, Canvas, TEXT("MonsterStatusHeading"),
			NSLOCTEXT("MarchboundMonsterTab", "StatusHeading", "상태"), 31,
			FVector2D(1170.0f, 790.0f), FVector2D(600.0f, 50.0f), 20);
		AddText(Blueprint, Canvas, TEXT("MonsterStatusText_0"),
			NSLOCTEXT("MarchboundMonsterTab", "StatusBleedImmune", "출혈 면역"), 27,
			FVector2D(1170.0f, 855.0f), FVector2D(285.0f, 62.0f), 20);
		AddText(Blueprint, Canvas, TEXT("MonsterStatusText_1"),
			NSLOCTEXT("MarchboundMonsterTab", "StatusAlert", "경계"), 27,
			FVector2D(1465.0f, 855.0f), FVector2D(285.0f, 62.0f), 20);

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Blueprint->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
		if (!UPackage::SavePackage(Blueprint->GetPackage(), Blueprint, *Filename,
			FSavePackageArgs()))
		{
			UE_LOG(LogTemp, Error, TEXT("RD_MONSTER_TAB_BUILD save failed"));
			return;
		}

		UE_LOG(LogTemp, Display,
			TEXT("RD_MONSTER_TAB_BUILD success asset=%s rows=3 skills=4 design=1920x1080"),
			AssetPath);
	}
}

void RegisterMonsterTabWidgetBuilderCommands()
{
	using namespace MonsterTabWidgetBuilder;
	BuildCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.BuildMonsterTab"),
		TEXT("Create the independent responsive Marchbound monster-tab WBP."),
		FConsoleCommandDelegate::CreateStatic(&Build));
}

void UnregisterMonsterTabWidgetBuilderCommands()
{
	MonsterTabWidgetBuilder::BuildCommand.Reset();
}
