#include "UI/SkillDetailContentWidgetBuilder.h"

#include "AssetToolsModule.h"
#include "Blueprint/UserWidget.h"
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
#include "Components/ScaleBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/Texture2D.h"
#include "HAL/IConsoleManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UI/UIFont.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintEditorUtils.h"
#include "WidgetBlueprintFactory.h"

namespace SkillDetailContentWidgetBuilder
{
	constexpr TCHAR PackagePath[] = TEXT("/Game/UI/CombatDetail");
	constexpr TCHAR AssetName[] = TEXT("WBP_SkillDetailContent");
	constexpr TCHAR AssetPath[] =
		TEXT("/Game/UI/CombatDetail/WBP_SkillDetailContent.WBP_SkillDetailContent");
	constexpr TCHAR RangeNormalPath[] =
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatDetail/SkillTactical/"
			"T_SkillRangeButton_Normal_v1.T_SkillRangeButton_Normal_v1");
	constexpr TCHAR RangeSelectedPath[] =
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatDetail/SkillTactical/"
			"T_SkillRangeButton_Selected_v1.T_SkillRangeButton_Selected_v1");
	constexpr TCHAR APPath[] =
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/HUD04/"
			"KK_HUD04_zone_cost_badge.KK_HUD04_zone_cost_badge");
	constexpr TCHAR DamagePath[] =
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatDetail/SkillTactical/"
			"T_SkillStat_Damage_Simple_v2.T_SkillStat_Damage_Simple_v2");
	constexpr TCHAR CooldownPath[] =
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/HUD04/"
			"KK_HUD04_zone_cooldown_badge.KK_HUD04_zone_cooldown_badge");
	constexpr TCHAR CriticalPath[] =
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatDetail/SkillTactical/"
			"T_SkillStat_Critical_Clear_v1.T_SkillStat_Critical_Clear_v1");

	const FVector2D ContentSize(1230.f, 563.f);
	// 투명 캔버스는 브러시 UV에서 제거한다. 글자 상자는 보이는 판을 기준으로
	// 좌우 같은 여백을 사용해야 라벨의 중심과 판의 중심이 일치한다.
	const FMargin RangeTextSafePadding(40.f, 0.f, 40.f, 0.f);
	const FVector2D StatIconPositions[] = {
		FVector2D(42.f, 272.f), FVector2D(42.f, 314.f),
		FVector2D(42.f, 356.f), FVector2D(42.f, 398.f) };
	const FVector2D StatTextPositions[] = {
		FVector2D(86.f, 271.f), FVector2D(86.f, 313.f),
		FVector2D(86.f, 355.f), FVector2D(86.f, 397.f) };
	TUniquePtr<FAutoConsoleCommand> BuildCommand;

	void SaveObject(UObject* Object)
	{
		check(Object != nullptr);
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Object->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
		checkf(UPackage::SavePackage(Object->GetPackage(), Object, *Filename,
			FSavePackageArgs()), TEXT("Could not save %s"), *Object->GetPathName());
	}

	UTexture2D* Texture(const TCHAR* Path)
	{
		UTexture2D* Result = LoadObject<UTexture2D>(nullptr, Path);
		checkf(Result != nullptr, TEXT("Missing skill detail texture: %s"), Path);
		return Result;
	}

	FSlateBrush ImageBrush(UTexture2D* Source,
		const FLinearColor& Tint = FLinearColor::White)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Source);
		Brush.DrawAs = Source != nullptr
			? ESlateBrushDrawType::Image : ESlateBrushDrawType::NoDrawType;
		Brush.Margin = FMargin(0.f);
		Brush.TintColor = FSlateColor(Tint);
		if (Source != nullptr)
		{
			const FIntPoint Imported = Source->GetImportedSize();
			Brush.ImageSize = FVector2D(Imported.X, Imported.Y);
		}
		return Brush;
	}

	FSlateBrush RangeButtonBrush(UTexture2D* Source)
	{
		FSlateBrush Brush = ImageBrush(Source);
		if (Source == nullptr)
		{
			return Brush;
		}
		// 원본 소스 알파 실측값. Normal 1024x128=(32,0)-(994,118),
		// Selected=(33,0)-(992,121). 투명 캔버스를 Slate 배치에 남기지 않는다.
		const bool bSelected = Source->GetName().Contains(TEXT("Selected"));
		const FVector2f Min = bSelected
			? FVector2f(33.f / 1024.f, 0.f)
			: FVector2f(32.f / 1024.f, 0.f);
		const FVector2f Max = bSelected
			? FVector2f(992.f / 1024.f, 121.f / 128.f)
			: FVector2f(994.f / 1024.f, 118.f / 128.f);
		Brush.SetUVRegion(FBox2f(Min, Max));
		return Brush;
	}

	void Place(UCanvasPanel* Parent, UWidget* Child, const FVector2D& Position,
		const FVector2D& Size, const int32 ZOrder)
	{
		UCanvasPanelSlot* Slot = Parent->AddChildToCanvas(Child);
		Slot->SetAnchors(FAnchors(0.f));
		Slot->SetAlignment(FVector2D::ZeroVector);
		Slot->SetAutoSize(false);
		Slot->SetPosition(Position);
		Slot->SetSize(Size);
		Slot->SetZOrder(ZOrder);
	}

	void StyleText(UTextBlock* Text, const int32 FontSize,
		const FLinearColor& Color, const ETextJustify::Type Justification)
	{
		FSlateFontInfo Font = UIFont::MakeProjectExact(Text->GetFont(), FontSize);
		Font.OutlineSettings.OutlineSize = 1;
		Font.OutlineSettings.OutlineColor = FLinearColor(.015f, .02f, .025f, 1.f);
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetJustification(Justification);
		Text->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, .84f));
		Text->SetShadowOffset(FVector2D(1.5f, 1.5f));
		Text->SetMargin(FMargin(0.f));
		Text->SetRenderTransform(FWidgetTransform());
		Text->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	UImage* AddImage(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent,
		const FName Name, UTexture2D* Source, const FVector2D& Position,
		const FVector2D& Size, const int32 ZOrder,
		const FLinearColor& Tint = FLinearColor::White)
	{
		UImage* Image = Blueprint->WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), Name);
		Image->SetBrush(ImageBrush(Source, Tint));
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		Place(Parent, Image, Position, Size, ZOrder);
		return Image;
	}

	UTextBlock* AddText(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent,
		const FName Name, const FText& Value, const FVector2D& Position,
		const FVector2D& Size, const int32 FontSize, const int32 ZOrder,
		const ETextJustify::Type Justification = ETextJustify::Left)
	{
		UTextBlock* Text = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), Name);
		Text->SetText(Value);
		StyleText(Text, FontSize, FLinearColor(.94f, .88f, .76f, 1.f),
			Justification);
		Place(Parent, Text, Position, Size, ZOrder);
		return Text;
	}

	void AddRule(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent,
		const FName Name, const FVector2D& Position, const FVector2D& Size,
		const FLinearColor& Color, const int32 ZOrder)
	{
		UBorder* Rule = Blueprint->WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), Name);
		Rule->SetBrushColor(Color);
		Rule->SetPadding(FMargin(0.f));
		Rule->SetVisibility(ESlateVisibility::HitTestInvisible);
		Place(Parent, Rule, Position, Size, ZOrder);
	}

	void AddRangeControl(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent,
		const TCHAR* Stem, const FText& Label, const FVector2D& Position,
		UTexture2D* NormalTexture)
	{
		// PNG는 8:1인데 터치 표면은 316x56이다. 그림만 316x39.5로
		// Aspect Fit하고 투명 Button/라벨은 316x56 전체를 사용한다.
		const FVector2D ButtonSize(316.f, 56.f);
		const float PlateHeight = ButtonSize.X / 8.f;
		const FVector2D PlatePosition(Position.X,
			Position.Y + (ButtonSize.Y - PlateHeight) * .5f);
		UImage* Plate = AddImage(Blueprint, Parent,
			FName(*FString::Printf(TEXT("%sPlate"), Stem)), NormalTexture,
			PlatePosition, FVector2D(ButtonSize.X, PlateHeight), 20);
		Plate->SetBrush(RangeButtonBrush(NormalTexture));

		UButton* Button = Blueprint->WidgetTree->ConstructWidget<UButton>(
			UButton::StaticClass(), FName(*FString::Printf(TEXT("%sButton"), Stem)));
		FButtonStyle InvisibleStyle;
		InvisibleStyle.Normal.DrawAs = ESlateBrushDrawType::NoDrawType;
		InvisibleStyle.Hovered.DrawAs = ESlateBrushDrawType::NoDrawType;
		InvisibleStyle.Pressed.DrawAs = ESlateBrushDrawType::NoDrawType;
		InvisibleStyle.Disabled.DrawAs = ESlateBrushDrawType::NoDrawType;
		InvisibleStyle.SetNormalPadding(FMargin(0.f));
		InvisibleStyle.SetPressedPadding(FMargin(0.f));
		Button->SetStyle(InvisibleStyle);
		Button->SetTouchMethod(EButtonTouchMethod::PreciseTap);
		Button->SetClickMethod(EButtonClickMethod::PreciseClick);
		Button->SetVisibility(ESlateVisibility::Visible);
		Place(Parent, Button, Position, ButtonSize, 30);

		UScaleBox* AutoFit = Blueprint->WidgetTree->ConstructWidget<UScaleBox>(
			UScaleBox::StaticClass(),
			FName(*FString::Printf(TEXT("%sText_AutoFit"), Stem)));
		AutoFit->SetStretch(EStretch::ScaleToFitX);
		AutoFit->SetStretchDirection(EStretchDirection::DownOnly);
		AutoFit->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		AutoFit->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Button->SetContent(AutoFit);
		if (UButtonSlot* Slot = CastChecked<UButtonSlot>(AutoFit->Slot))
		{
			Slot->SetPadding(RangeTextSafePadding);
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Fill);
		}

		UTextBlock* Text = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			FName(*FString::Printf(TEXT("%sText"), Stem)));
		Text->SetText(Label);
		StyleText(Text, 24, FLinearColor(.96f, .91f, .80f, 1.f),
			ETextJustify::Center);
		AutoFit->SetContent(Text);
		if (UScaleBoxSlot* Slot = CastChecked<UScaleBoxSlot>(Text->Slot))
		{
			Slot->SetHorizontalAlignment(HAlign_Center);
			Slot->SetVerticalAlignment(VAlign_Center);
		}
	}

	UWidgetBlueprint* EnsureBlueprint()
	{
		if (UWidgetBlueprint* Existing = LoadObject<UWidgetBlueprint>(nullptr, AssetPath))
		{
			return Existing;
		}
		UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
		Factory->ParentClass = UUserWidget::StaticClass();
		FAssetToolsModule& AssetTools =
			FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		return Cast<UWidgetBlueprint>(AssetTools.Get().CreateAsset(AssetName,
			PackagePath, UWidgetBlueprint::StaticClass(), Factory));
	}

	void Build()
	{
		UTexture2D* RangeNormal = Texture(RangeNormalPath);
		Texture(RangeSelectedPath);
		UTexture2D* StatTextures[] = {
			Texture(APPath), Texture(DamagePath), Texture(CooldownPath),
			Texture(CriticalPath) };

		UWidgetBlueprint* Blueprint = EnsureBlueprint();
		check(Blueprint != nullptr && Blueprint->WidgetTree != nullptr);
		Blueprint->Modify();
		Blueprint->WidgetTree->Modify();
		if (UWidget* PreviousRoot = Blueprint->WidgetTree->RootWidget)
		{
			Blueprint->ParentClass = UUserWidget::StaticClass();
			TSet<UWidget*> PreviousWidgets;
			PreviousWidgets.Add(PreviousRoot);
			FWidgetBlueprintEditorUtils::DeleteWidgets(Blueprint,
				MoveTemp(PreviousWidgets),
				FWidgetBlueprintEditorUtils::EDeleteWidgetWarningType::DeleteSilently);
		}
		Blueprint->ParentClass = UUserWidget::StaticClass();

		USizeBox* DesignSize = Blueprint->WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("SkillDetailContentSize"));
		DesignSize->SetWidthOverride(ContentSize.X);
		DesignSize->SetHeightOverride(ContentSize.Y);
		DesignSize->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Blueprint->WidgetTree->RootWidget = DesignSize;

		UCanvasPanel* Root = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("SkillDetailContentRoot"));
		Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		DesignSize->SetContent(Root);

		UBorder* Backdrop = Blueprint->WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("SkillContentBackdrop"));
		Backdrop->SetBrushColor(FLinearColor(.008f, .012f, .016f, .72f));
		Backdrop->SetPadding(FMargin(0.f));
		Backdrop->SetVisibility(ESlateVisibility::HitTestInvisible);
		Place(Root, Backdrop, FVector2D::ZeroVector, ContentSize, 0);

		// 스킬 아이콘은 원본 실루엣만 보여 준다. 아이콘별 외곽 형태를 가리는
		// 공용 원형 프레임은 두지 않는다.
		AddImage(Blueprint, Root, TEXT("SkillIconImage"), nullptr,
			FVector2D(72.f, 52.f), FVector2D(180.f, 180.f), 6);
		AddRule(Blueprint, Root, TEXT("SkillColumnDivider"),
			FVector2D(350.f, 20.f), FVector2D(2.f, 510.f),
			FLinearColor(.72f, .46f, .18f, .72f), 5);

		const FText StatDefaults[] = {
			FText::FromString(TEXT("AP 4")), FText::FromString(TEXT("피해 24~32")),
			FText::FromString(TEXT("쿨타임 2턴")), FText::FromString(TEXT("치명타 48")) };
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(StatTextures); ++Index)
		{
			AddImage(Blueprint, Root, FName(*FString::Printf(
				TEXT("SkillStatIcon_%d"), Index)), StatTextures[Index],
				StatIconPositions[Index], FVector2D(34.f), 8);
			AddText(Blueprint, Root, FName(*FString::Printf(
				TEXT("SkillStatText_%d"), Index)), StatDefaults[Index],
				StatTextPositions[Index], FVector2D(232.f, 36.f), 24, 8);
			AddRule(Blueprint, Root, FName(*FString::Printf(
				TEXT("SkillStatRule_%d"), Index)),
				FVector2D(38.f, StatTextPositions[Index].Y + 38.f),
				FVector2D(286.f, 1.f), FLinearColor(.58f, .37f, .15f, .66f), 6);
		}

		AddRangeControl(Blueprint, Root, TEXT("SkillSelectRange"),
			FText::FromString(TEXT("사정 범위  2칸")), FVector2D(24.f, 436.f),
			RangeNormal);
		AddRangeControl(Blueprint, Root, TEXT("SkillEffectRange"),
			FText::FromString(TEXT("영향 범위  2칸")), FVector2D(24.f, 498.f),
			RangeNormal);

		UWidgetSwitcher* Switcher =
			Blueprint->WidgetTree->ConstructWidget<UWidgetSwitcher>(
				UWidgetSwitcher::StaticClass(), TEXT("SkillContentSwitcher"));
		Switcher->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Place(Root, Switcher, FVector2D(354.f, 0.f), FVector2D(872.f, 563.f), 10);

		UCanvasPanel* DescriptionPage =
			Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
				UCanvasPanel::StaticClass(), TEXT("SkillDescriptionPage"));
		DescriptionPage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Switcher->AddChild(DescriptionPage);

		UScrollBox* DescriptionScroll =
			Blueprint->WidgetTree->ConstructWidget<UScrollBox>(
				UScrollBox::StaticClass(), TEXT("SkillDescriptionScroll"));
		DescriptionScroll->SetScrollBarVisibility(ESlateVisibility::Visible);
		DescriptionScroll->SetScrollbarThickness(FVector2D(5.f));
		DescriptionScroll->SetAlwaysShowScrollbar(false);
		DescriptionScroll->SetAlwaysShowScrollbarTrack(false);
		DescriptionScroll->SetAnimateWheelScrolling(true);
		DescriptionScroll->SetAllowOverscroll(true);
		DescriptionScroll->SetConsumeMouseWheel(
			EConsumeMouseWheel::WhenScrollingPossible);
		DescriptionScroll->SetVisibility(ESlateVisibility::Visible);
		Place(DescriptionPage, DescriptionScroll, FVector2D(26.f, 24.f),
			FVector2D(778.f, 494.f), 1);

		UTextBlock* DescriptionText =
			Blueprint->WidgetTree->ConstructWidget<UTextBlock>(
				UTextBlock::StaticClass(), TEXT("SkillDescriptionText"));
		DescriptionText->SetText(FText::FromString(
			TEXT("검을 크게 휘둘러 주변의 모든 적을 연속으로 공격합니다.\n\n"
				"중심에 가까운 적은 더 큰 피해를 받습니다.")));
		StyleText(DescriptionText, 29, FLinearColor(.94f, .88f, .76f, 1.f),
			ETextJustify::Left);
		DescriptionText->SetAutoWrapText(true);
		DescriptionText->SetWrapTextAt(736.f);
		DescriptionText->SetLineHeightPercentage(1.32f);
		DescriptionScroll->AddChild(DescriptionText);
		if (UScrollBoxSlot* Slot = CastChecked<UScrollBoxSlot>(DescriptionText->Slot))
		{
			Slot->SetPadding(FMargin(8.f, 4.f, 18.f, 18.f));
			Slot->SetHorizontalAlignment(HAlign_Fill);
		}

		UOverlay* TacticalPage = Blueprint->WidgetTree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(), TEXT("SkillTacticalPage"));
		TacticalPage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Switcher->AddChild(TacticalPage);
		UOverlay* TacticalHost = Blueprint->WidgetTree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(), TEXT("SkillTacticalHost"));
		TacticalHost->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		TacticalPage->AddChildToOverlay(TacticalHost);
		if (UOverlaySlot* Slot = CastChecked<UOverlaySlot>(TacticalHost->Slot))
		{
			Slot->SetPadding(FMargin(0.f));
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Fill);
		}
		Switcher->SetActiveWidgetIndex(0);

		UImage* WorldPreview = AddImage(Blueprint, Root,
			TEXT("SkillWorldPreview"), nullptr, FVector2D(354.f, 281.f),
			FVector2D(820.f, 255.f), 25);
		WorldPreview->SetVisibility(ESlateVisibility::Collapsed);

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		SaveObject(Blueprint);
		UE_LOG(LogTemp, Display,
			TEXT("RD_SKILL_DETAIL_CONTENT_BUILD success asset=%s size=%.0fx%.0f runtime_children=diagram-only"),
			AssetPath, ContentSize.X, ContentSize.Y);
	}
}

void RegisterSkillDetailContentWidgetBuilderCommands()
{
	using namespace SkillDetailContentWidgetBuilder;
	BuildCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.BuildSkillDetailContent"),
		TEXT("Build the WBP-authored skill detail information surface."),
		FConsoleCommandDelegate::CreateStatic(&Build));
}

void UnregisterSkillDetailContentWidgetBuilderCommands()
{
	using namespace SkillDetailContentWidgetBuilder;
	BuildCommand.Reset();
}
