#include "UI/SkillTacticalDiagramWidgetBuilder.h"

#include "AssetRegistry/AssetRegistryModule.h"
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
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "Engine/Texture2D.h"
#include "HAL/IConsoleManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UI/Combat/SkillTacticalDiagramWidget.h"
#include "UI/UIFont.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintEditorUtils.h"
#include "WidgetBlueprintFactory.h"

namespace SkillTacticalDiagramWidgetBuilder
{
	constexpr TCHAR PackagePath[] = TEXT("/Game/UI/CombatDetail/SkillTactical");
	constexpr TCHAR ArtPackagePath[] = TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatDetail/SkillTactical");
	constexpr TCHAR FontPackagePath[] = TEXT("/Game/SVN/OutSideAsset/Fonts/GowunBatang");
	constexpr TCHAR FontAssetPackage[] = TEXT("/Game/SVN/OutSideAsset/Fonts/GowunBatang/F_GowunBatang");
	constexpr TCHAR FontAssetPath[] = TEXT("/Game/SVN/OutSideAsset/Fonts/GowunBatang/F_GowunBatang.F_GowunBatang");
	constexpr TCHAR AssetName[] = TEXT("WBP_SkillTacticalDiagram");
	constexpr TCHAR AssetPath[] =
		TEXT("/Game/UI/CombatDetail/SkillTactical/WBP_SkillTacticalDiagram.WBP_SkillTacticalDiagram");
	constexpr int32 RowCount = 9;
	constexpr int32 ColumnCount = 9;
	const FVector2D BoardOrigin(136.f, -18.5f);
	const FVector2D BoardSize(600.f, 600.f);
	const FVector2D GridOrigin(4.f, 4.f);
	const FVector2D TileSlotSize(66.f, 66.f);
	constexpr float CellPitch = 65.f;
	constexpr float BoardTiltY = .64f;
	TUniquePtr<FAutoConsoleCommand> BuildCommand;

	void SaveObject(UObject* Object)
	{
		check(Object != nullptr);
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Object->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
		checkf(UPackage::SavePackage(Object->GetPackage(), Object, *Filename,
			FSavePackageArgs()), TEXT("Could not save %s"), *Object->GetPathName());
	}

	UFont* EnsureDetailFont()
	{
		// 고운바탕 폰트는 SVN(OutSideAsset)에서 관리한다. 재임포트하지 않고
		// 이미 커밋된 에셋을 그대로 읽는다.
		UFont* Font = LoadObject<UFont>(nullptr, FontAssetPath);
		checkf(Font != nullptr,
			TEXT("Missing Gowun Batang font (SVN 업데이트 필요): %s"), FontAssetPath);
		return Font;
	}

	UTexture2D* ArtTexture(const TCHAR* Name)
	{
		// 전술판 아트도 SVN(OutSideAsset)에서 관리한다.
		const FString ObjectPath = FString::Printf(TEXT("%s/%s.%s"),
			ArtPackagePath, Name, Name);
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *ObjectPath);
		checkf(Texture != nullptr,
			TEXT("Missing tactical art texture (SVN 업데이트 필요): %s"), *ObjectPath);
		return Texture;
	}

	UTexture2D* Texture(const TCHAR* Path)
	{
		UTexture2D* Result = LoadObject<UTexture2D>(nullptr, Path);
		checkf(Result != nullptr, TEXT("Missing tactical WBP texture: %s"), Path);
		return Result;
	}

	FSlateBrush ImageBrush(UTexture2D* Source,
		const FLinearColor& Tint = FLinearColor::White)
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

	void StyleText(UTextBlock* Text, const int32 Size, const FLinearColor& Color)
	{
		FSlateFontInfo Font = UIFont::MakeProjectExact(Text->GetFont(), Size);
		Font.OutlineSettings.OutlineSize = 1;
		Font.OutlineSettings.OutlineColor = FLinearColor(.015f, .02f, .025f, 1.f);
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetShadowOffset(FVector2D(1.5f, 1.5f));
		Text->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, .82f));
		Text->SetJustification(ETextJustify::Center);
		Text->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	void AddRangeButton(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent,
		const TCHAR* Stem, const FText& TextValue, const FVector2D& Position,
		const FVector2D& Size, UTexture2D* ButtonTexture,
		UTexture2D* SelectedButtonTexture,
		const FLinearColor& BackColor,
		const FLinearColor& TextColor)
	{
		UButton* Button = Blueprint->WidgetTree->ConstructWidget<UButton>(
			UButton::StaticClass(), FName(*FString::Printf(TEXT("%sButton"), Stem)));
		FSlateBrush Normal = ImageBrush(ButtonTexture);
		Normal.DrawAs = ESlateBrushDrawType::Image;
		FSlateBrush Hovered = ImageBrush(SelectedButtonTexture);
		Hovered.DrawAs = ESlateBrushDrawType::Image;
		FSlateBrush Pressed = Hovered;
		Pressed.TintColor = FSlateColor(FLinearColor(.82f, .88f, .90f, 1.f));
		FButtonStyle Style;
		Style.SetNormal(Normal);
		Style.SetHovered(Hovered);
		Style.SetPressed(Pressed);
		Style.SetDisabled(Normal);
		Style.SetNormalPadding(FMargin(0.f));
		// 눌림 상태에서도 라벨의 중앙 좌표를 바꾸지 않는다.
		Style.SetPressedPadding(FMargin(0.f));
		Button->SetStyle(Style);
		Button->SetBackgroundColor(FLinearColor::White);
		Button->SetTouchMethod(EButtonTouchMethod::PreciseTap);
		Button->SetClickMethod(EButtonClickMethod::PreciseClick);
		Button->SetVisibility(ESlateVisibility::Visible);
		Place(Parent, Button, Position, Size, 300);

		UTextBlock* Text = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), FName(*FString::Printf(TEXT("%sText"), Stem)));
		Text->SetText(TextValue);
		StyleText(Text, 27, TextColor);
		Button->SetContent(Text);
		UButtonSlot* Slot = CastChecked<UButtonSlot>(Text->Slot);
		Slot->SetHorizontalAlignment(HAlign_Fill);
		Slot->SetVerticalAlignment(VAlign_Center);
		Slot->SetPadding(FMargin(0.f));
	}

	UWidgetBlueprint* EnsureBlueprint()
	{
		if (UWidgetBlueprint* Existing = LoadObject<UWidgetBlueprint>(nullptr, AssetPath))
		{
			return Existing;
		}
		UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
		Factory->ParentClass = USkillTacticalDiagramWidget::StaticClass();
		FAssetToolsModule& AssetTools =
			FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		return Cast<UWidgetBlueprint>(AssetTools.Get().CreateAsset(AssetName,
			PackagePath, UWidgetBlueprint::StaticClass(), Factory));
	}

	void Build()
	{
		EnsureDetailFont();
		UTexture2D* NeutralTile = Texture(TEXT(
			"/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/"
			"T_KitA_Cell_Disabled.T_KitA_Cell_Disabled"));
		UTexture2D* Ring = Texture(TEXT(
			"/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/"
			"T_KitA_StatChip_Ring.T_KitA_StatChip_Ring"));
		UTexture2D* Caster = Texture(TEXT(
			"/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/"
			"T_MB_OptionsIcon_MercenaryGlyph.T_MB_OptionsIcon_MercenaryGlyph"));
		UTexture2D* Target = Texture(TEXT(
			"/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/"
			"T_MB_OptionsIcon_MonsterGlyph.T_MB_OptionsIcon_MonsterGlyph"));
		UTexture2D* RangeButton = ArtTexture(
			TEXT("T_SkillRangeButton_Normal_v1"));
		UTexture2D* SelectedRangeButton = ArtTexture(
			TEXT("T_SkillRangeButton_Selected_v1"));
		// 피해/치명타는 작은 모바일 수치 행에서도 한 번에 읽히도록 단순화한
		// 전용 에셋을 런타임이 하드 참조한다. 존재 확인만 한다.
		ArtTexture(TEXT("T_SkillStat_Damage_Simple_v2"));
		ArtTexture(TEXT("T_SkillStat_Critical_Simple_v2"));

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
		Blueprint->ParentClass = USkillTacticalDiagramWidget::StaticClass();

		UCanvasPanel* Root = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("TacticalDiagramRoot"));
		Blueprint->WidgetTree->RootWidget = Root;

		UBorder* Backdrop = Blueprint->WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("TacticalDiagramBackdrop"));
		Backdrop->SetBrushColor(FLinearColor(.008f, .012f, .016f, .22f));
		Backdrop->SetPadding(FMargin(0.f));
		Backdrop->SetVisibility(ESlateVisibility::HitTestInvisible);
		Place(Root, Backdrop, FVector2D(0.f, 0.f), FVector2D(872.f, 563.f), 0);

		// 실제 사각 타일 좌표는 그대로 둔 채 판 전체를 45도 회전한 뒤
		// 세로만 압축한다. 두 단계 컨테이너를 나누면 회전 후 스케일이
		// 적용되어, 각 사각 셀이 인게임식으로 눕혀진 다이아몬드가 된다.
		UCanvasPanel* BoardTilt = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("TacticalBoardTilt"));
		BoardTilt->SetRenderTransformPivot(FVector2D(.5f, .5f));
		BoardTilt->SetRenderScale(FVector2D(1.f, BoardTiltY));
		BoardTilt->SetVisibility(ESlateVisibility::HitTestInvisible);
		Place(Root, BoardTilt, BoardOrigin, BoardSize, 10);

		UCanvasPanel* BoardRotate = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("TacticalBoardRotate"));
		BoardRotate->SetRenderTransformPivot(FVector2D(.5f, .5f));
		BoardRotate->SetRenderTransformAngle(45.f);
		BoardRotate->SetVisibility(ESlateVisibility::HitTestInvisible);
		Place(BoardTilt, BoardRotate, FVector2D::ZeroVector, BoardSize, 10);

		UBorder* BoardShadow = Blueprint->WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("TacticalBoardShadow"));
		BoardShadow->SetBrushColor(FLinearColor(.004f, .008f, .012f, .58f));
		BoardShadow->SetPadding(FMargin(0.f));
		BoardShadow->SetVisibility(ESlateVisibility::HitTestInvisible);
		Place(BoardRotate, BoardShadow, FVector2D(1.f), BoardSize - FVector2D(2.f), 0);

		for (int32 Row = 0; Row < RowCount; ++Row)
		{
			for (int32 Column = 0; Column < ColumnCount; ++Column)
			{
				// 실제 전투판과 동일한 9x9 직교 좌표계다. 셀은 판의 로컬
				// 공간에서 정사각형으로 배치되고 부모 변환으로만 투영된다.
				const FVector2D Position = GridOrigin + FVector2D(
					Column * CellPitch, Row * CellPitch);
				AddImage(Blueprint, BoardRotate, FName(*FString::Printf(
					TEXT("TacticalCell_R%dC%d"), Row, Column)), NeutralTile,
					Position, TileSlotSize,
					20 + (Row + Column) * 10 + Row,
					FLinearColor(1.f, 1.f, 1.f, .46f));
			}
		}

		AddImage(Blueprint, Root, TEXT("TacticalCasterRing"), Ring,
			FVector2D(417.f, 251.f), FVector2D(38.f, 38.f), 200,
			FLinearColor(.10f, .62f, 1.f, 1.f));
		AddImage(Blueprint, Root, TEXT("TacticalCasterMarker"), Caster,
			FVector2D(423.5f, 255.5f), FVector2D(25.f, 29.f), 220);
		AddImage(Blueprint, Root, TEXT("TacticalTargetRing"), Ring,
			FVector2D(482.f, 292.f), FVector2D(38.f, 38.f), 205,
			FLinearColor(1.f, .58f, .08f, 1.f));
		AddImage(Blueprint, Root, TEXT("TacticalTargetMarker"), Target,
			FVector2D(488.5f, 296.5f), FVector2D(25.f, 29.f), 225);

		for (int32 Index = 0; Index < 9; ++Index)
		{
			AddImage(Blueprint, Root, FName(*FString::Printf(
				TEXT("TacticalRouteDot_%d"), Index)), Ring,
				FVector2D(475.f + Index * 7.f, 156.f), FVector2D(6.f, 6.f),
				190, FLinearColor(1.f, .78f, .28f, 1.f));
		}

		UBorder* Rule = Blueprint->WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("TacticalLegendRule"));
		Rule->SetBrushColor(FLinearColor(.76f, .44f, .12f, .42f));
		Rule->SetPadding(FMargin(0.f));
		Rule->SetVisibility(ESlateVisibility::HitTestInvisible);
		Place(Root, Rule, FVector2D(45.f, 329.f), FVector2D(1030.f, 1.f), 280);

		AddRangeButton(Blueprint, Root, TEXT("TacticalSelectLegend"),
			FText::FromString(TEXT("사정 범위  2칸")), FVector2D(88.f, 342.f),
			FVector2D(310.f, 58.f), RangeButton, SelectedRangeButton,
			FLinearColor(.015f, .12f, .24f, .96f),
			FLinearColor(.30f, .70f, 1.f, 1.f));
		AddRangeButton(Blueprint, Root, TEXT("TacticalEffectLegend"),
			FText::FromString(TEXT("영향 범위  2칸")), FVector2D(722.f, 342.f),
			FVector2D(310.f, 58.f), RangeButton, SelectedRangeButton,
			FLinearColor(.25f, .045f, .018f, .96f),
			FLinearColor(1.f, .39f, .20f, 1.f));

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		SaveObject(Blueprint);
		UE_LOG(LogTemp, Display,
			TEXT("RD_SKILL_TACTICAL_BUILD success asset=%s cells=%dx%d worldCapture=false"),
			AssetPath, ColumnCount, RowCount);
	}
}

void RegisterSkillTacticalDiagramWidgetBuilderCommands()
{
	using namespace SkillTacticalDiagramWidgetBuilder;
	BuildCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.BuildSkillTacticalDiagram"),
		TEXT("Build the data-driven 9x9 square skill range WBP."),
		FConsoleCommandDelegate::CreateStatic(&Build));
}

void UnregisterSkillTacticalDiagramWidgetBuilderCommands()
{
	using namespace SkillTacticalDiagramWidgetBuilder;
	BuildCommand.Reset();
}
