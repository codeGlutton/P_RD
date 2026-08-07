#include "UI/MonsterTabWidgetBuilder.h"
#include "UI/UIPartRects.h"
#include "UI/UIFont.h"

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
		// 이미 GUID 가 있는 이름을 또 등록하면 ensure 가 뜬다. 이 빌더는 이미
		// 만들어진 판 위에 다시 도는 것이 정상이라 두 번째부터는 늘 그랬다.
		if (Blueprint->WidgetVariableNameToGuidMap.Contains(Widget->GetFName()))
		{
			return;
		}
		Blueprint->OnVariableAdded(Widget->GetFName());
	}

	void StyleText(UTextBlock* Text, const int32 Size,
		const FLinearColor Color, const ETextJustify::Type Justification = ETextJustify::Center)
	{
		FSlateFontInfo Font = UIFont::Make(Text->GetFont(), Size);
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

	/**
	 * @brief 늘려도 모서리가 안 뭉개지게 9-slice 로 그리는 그림.
	 *
	 * KitA 부품은 나무 틀에 금 모서리가 박혀 있어 통짜로 늘리면 장식까지 늘어난다.
	 * 여백은 실측값(Saved/UIKit/ConceptA/_nineslice.txt)에 잘라낼 때 남긴 투명
	 * 여백 8px 을 더한 값이고, Box 브러시 마진은 그림 크기 대비 비율이다.
	 */
	UImage* AddSlicedImage(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent,
		const FName Name, UTexture2D* Source, const FVector2D Position,
		const FVector2D Size, const int32 ZOrder, const FVector2D BorderPx,
		const float PadPx = 8.f)
	{
		UImage* Image = AddImage(Blueprint, Parent, Name, Source, Position, Size, ZOrder);
		if (Source == nullptr)
		{
			return Image;
		}
		// GetSizeX() 는 갓 LoadObject 한 텍스처에서 0 을 준다(플랫폼 데이터가
		// 아직 없다). 0 으로 나눠 마진이 inf 가 됐다 -- 실제로 그렇게 나왔다.
		const FIntPoint Imported = Source->GetImportedSize();
		if (Imported.X <= 0 || Imported.Y <= 0)
		{
			return Image;
		}
		const FVector2D Texel(Imported.X, Imported.Y);
		FSlateBrush Brush = Image->GetBrush();
		Brush.DrawAs = ESlateBrushDrawType::Box;
		// 투명 여백은 부품 시트에서 잘라낸 것만 8px 이다. 틀 그림은 원본에서
		// 바로 떠서 0 이다 -- 무조건 8 을 더하고 있어 테두리가 8px 씩 밀렸다.
		Brush.Margin = FMargin(
			(PadPx + BorderPx.X) / Texel.X, (PadPx + BorderPx.Y) / Texel.Y,
			(PadPx + BorderPx.X) / Texel.X, (PadPx + BorderPx.Y) / Texel.Y);
		Image->SetBrush(Brush);
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

		/*
		 * 세 칸의 자리. 프레임이 조립식이 되면서 이 값이 **원인**이 됐다 --
		 * 전에는 그림에 그려진 분할선을 따라 맞추는 결과였지만, 이제는 이 값이
		 * 기둥을 어디 세울지 정한다. 열 비율을 바꾸려면 여기만 고치면 된다.
		 */
		// 칸 자리는 **틀 그림에서 잰 구멍**에서 나온다. 전에는 (58,150) 처럼
		// 손으로 적어 두었는데, 잰 구멍은 x70 부터라 칸이 12px 씩 나무 밑으로
		// 들어가 있었다. 재 놓은 값이 있는데 짐작할 이유가 없다.
		const FBox2D Window = UIPartRects::Inner(TEXT("T_KitA_Frame_Outer"),
			FVector2D::ZeroVector, FVector2D(1920.f, 1080.f));
		const float DividerWidth = 58.0f;
		const float HeadRoom = 70.f;          // 제목 명패가 먹는 위쪽
		const float ColumnTop = Window.Min.Y + HeadRoom;
		const float ColumnHeight = Window.Max.Y - ColumnTop;
		const float Usable = Window.GetSize().X - DividerWidth * 2.f;
		// 열 비율. 상세창(frame_registry.SCREEN_COLUMNS)과 같은 값이다.
		const FVector2D LeftCell(Window.Min.X, ColumnTop);
		const FVector2D LeftSize(Usable * 0.24f, ColumnHeight);
		const FVector2D MidCell(LeftCell.X + LeftSize.X + DividerWidth, ColumnTop);
		const FVector2D MidSize(Usable * 0.29f, ColumnHeight);
		const FVector2D RightCell(MidCell.X + MidSize.X + DividerWidth, ColumnTop);
		const FVector2D RightSize(Usable * 0.47f, ColumnHeight);
		// 열 안 여백. 나무 두께는 위에서 이미 뺐으므로 숨통만 준다.
		const float Gutter = 14.f;

		// 옛 통짜 3열 프레임은 지웠다. 열 경계가 그림에 박혀 있어
		// 열 비율을 못 바꾸고, 16:9 가 아닌 폰에서 나무가 늘어났다.
		// 이제 바깥 틀 한 장 + 기둥 두 개를 코드가 놓는다.
		UTexture2D* BaseFrame = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_Frame_Outer.T_KitA_Frame_Outer"));
		UTexture2D* FrameDivider = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_Frame_Divider.T_KitA_Frame_Divider"));
		UTexture2D* RowNormal = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/MonsterTab/T_MT_RowNormal.T_MT_RowNormal"));
		UTexture2D* RowSelected = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/MonsterTab/T_MT_RowSelected.T_MT_RowSelected"));
		// 같은 기능인데 지도·탭·설정이 서로 다른 단추를 쓰고 있었다(0804 검수).
		// 공용 KitA 단추 하나로 모은다.
		UTexture2D* BackButtonTexture = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_Button_Small_Normal.T_KitA_Button_Small_Normal"));
		// 바깥 틀은 9-slice 로 편다. 모서리 장식이 x91 · y98 까지라 그 안쪽만 늘어난다
		// (Saved/UIKit/_slice_margins.txt 실측).
		AddSlicedImage(Blueprint, Canvas, TEXT("MonsterTabBaseFrame"), BaseFrame,
			FVector2D::ZeroVector, FVector2D(1920.0f, 1080.0f), 0,
			FVector2D(91.0f, 98.0f), 0.f);
		// 기둥은 칸과 칸 사이에 놓는다. 열 좌표를 바꾸면 여기 두 줄만 따라 옮기면 된다.
		const float DividerGaps[] = {
			(LeftCell.X + LeftSize.X + MidCell.X - DividerWidth) * 0.5f,
			(MidCell.X + MidSize.X + RightCell.X - DividerWidth) * 0.5f };
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(DividerGaps); ++Index)
		{
			AddSlicedImage(Blueprint, Canvas,
				FName(*FString::Printf(TEXT("MonsterTabDivider_%d"), Index)), FrameDivider,
				FVector2D(DividerGaps[Index], 73.0f),
				FVector2D(DividerWidth, 939.0f), 1, FVector2D(0.0f, 94.0f), 0.f);
		}

		UTextBlock* Title = AddText(Blueprint, Canvas, TEXT("MonsterTabTitleText"),
			NSLOCTEXT("MarchboundMonsterTab", "Title", "몬스터"), 62,
			FVector2D(340.0f, 69.0f), FVector2D(640.0f, 104.0f), 10,
			FLinearColor(1.0f, 0.93f, 0.78f, 1.0f));
		Title->SetShadowOffset(FVector2D(3.0f, 3.0f));

		/*
		 * 배치는 위에서 정한 세 칸을 쓴다. 기둥은 그 칸 사이에 세운다.
		 */
		const FLinearColor Accent(0.95f, 0.78f, 0.42f, 1.0f);
		const FLinearColor Ink(0.12f, 0.065f, 0.025f, 1.0f);

		// 뒤로는 그림의 위쪽 장식 띠(y<130)에 둔다. 칸을 침범하지 않는 자리다.
		AddSlicedImage(Blueprint, Canvas, TEXT("MonsterBackArt"), BackButtonTexture,
			FVector2D(1650.0f, 40.0f), FVector2D(190.0f, 76.0f), 30, FVector2D(44.0f, 35.0f));
		AddText(Blueprint, Canvas, TEXT("MonsterBackText"),
			NSLOCTEXT("MarchboundMonsterTab", "Back", "뒤로"), 29,
			FVector2D(1650.0f, 56.0f), FVector2D(190.0f, 52.0f), 31,
			FLinearColor(1.0f, 0.93f, 0.78f, 1.0f));
		AddTransparentButton(Blueprint, Canvas, TEXT("MonsterBackButton"),
			FVector2D(1650.0f, 40.0f), FVector2D(190.0f, 76.0f), 32);

		// ── 왼쪽 칸: 나온 순서대로 채우는 몬스터 명단 ──────────────────
		AddText(Blueprint, Canvas, TEXT("MonsterListHeading"),
			NSLOCTEXT("MarchboundMonsterTab", "ListHeading", "출현 몬스터"), 30,
			FVector2D(LeftCell.X + Gutter, LeftCell.Y + 14.0f),
			FVector2D(LeftSize.X - Gutter * 2.f, 44.0f), 20, Accent, ETextJustify::Left);

		const TCHAR* MonsterNames[3] = { TEXT("독수리"), TEXT("늑대인간"), TEXT("골렘") };
		const TCHAR* HeadPaths[3] = {
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Portraits/KK_Face_Enemy_Eagle_HeadV2.KK_Face_Enemy_Eagle_HeadV2"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Portraits/KK_Face_Enemy_Werewolf_HeadV2.KK_Face_Enemy_Werewolf_HeadV2"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Portraits/KK_Face_Enemy_Golem_HeadV2.KK_Face_Enemy_Golem_HeadV2")
		};
		// 행 그림은 1903x681(2.79:1)이다. 폭을 칸에 맞추고 높이를 비율로 얻어야
		// 테두리가 늘어지지 않는다.
		const float RowWidth = LeftSize.X - Gutter * 2.f;
		const float RowHeight = RowWidth / 2.794f;
		for (int32 Index = 0; Index < 3; ++Index)
		{
			UCanvasPanel* Row = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
				UCanvasPanel::StaticClass(),
				FName(*FString::Printf(TEXT("MonsterRow_%d"), Index)));
			Place(Canvas, Row,
				FVector2D(LeftCell.X + Gutter, LeftCell.Y + 70.0f + (RowHeight + 18.0f) * Index),
				FVector2D(RowWidth, RowHeight), 20);
			Expose(Blueprint, Row);

			AddImage(Blueprint, Row,
				FName(*FString::Printf(TEXT("MonsterRowNormal_%d"), Index)), RowNormal,
				FVector2D::ZeroVector, FVector2D(RowWidth, RowHeight), 0, true);
			UImage* Selected = AddImage(Blueprint, Row,
				FName(*FString::Printf(TEXT("MonsterRowSelected_%d"), Index)), RowSelected,
				FVector2D::ZeroVector, FVector2D(RowWidth, RowHeight), 2, true);
			Selected->SetVisibility(Index == 0
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed);

			// 줄 그림은 통짜(Image)로 그린다 -- 원본 비율 그대로 놓았으므로
			// 테두리도 같은 배율로 줄어든다. 그래서 안쪽 자리는 비율로 구한다.
			const FBox2D RowInner = UIPartRects::Inner(TEXT("T_MT_RowNormal"),
				FVector2D::ZeroVector, FVector2D(RowWidth, RowHeight), false);
			const FVector2D RowSpan = RowInner.GetSize();
			const float PortraitExtent = RowSpan.Y;
			UImage* Portrait = AddImage(Blueprint, Row,
				FName(*FString::Printf(TEXT("MonsterRowPortrait_%d"), Index)),
				Texture(HeadPaths[Index]), RowInner.Min,
				FVector2D(PortraitExtent, PortraitExtent), 6, true);
			Portrait->SetClipping(EWidgetClipping::ClipToBoundsAlways);
			AddText(Blueprint, Row,
				FName(*FString::Printf(TEXT("MonsterRowName_%d"), Index)),
				FText::FromString(MonsterNames[Index]), 32,
				FVector2D(RowInner.Min.X + PortraitExtent + 16.f,
					RowInner.Min.Y + RowSpan.Y * 0.28f),
				FVector2D(RowSpan.X - PortraitExtent - 16.f, RowSpan.Y * 0.48f), 8,
				Ink, ETextJustify::Left);
			AddTransparentButton(Blueprint, Row,
				FName(*FString::Printf(TEXT("MonsterRowButton_%d"), Index)),
				FVector2D::ZeroVector, FVector2D(RowWidth, RowHeight), 20);
		}

		// ── 가운데 칸: 고른 몬스터의 모습과 이름 ────────────────────────
		UScaleBox* PortraitScale = Blueprint->WidgetTree->ConstructWidget<UScaleBox>(
			UScaleBox::StaticClass(), TEXT("MonsterDetailPortraitScale"));
		PortraitScale->SetStretch(EStretch::ScaleToFit);
		PortraitScale->SetStretchDirection(EStretchDirection::Both);
		PortraitScale->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		Place(Canvas, PortraitScale, FVector2D(MidCell.X + Gutter, MidCell.Y + 46.0f),
			FVector2D(MidSize.X - Gutter * 2.f, 618.0f), 15);
		UImage* DetailPortrait = Blueprint->WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), TEXT("MonsterDetailPortrait"));
		DetailPortrait->SetBrushFromTexture(Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Portraits/KK_Face_Enemy_Werewolf_ActionV3.KK_Face_Enemy_Werewolf_ActionV3")), false);
		DetailPortrait->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		PortraitScale->AddChild(DetailPortrait);
		Expose(Blueprint, DetailPortrait);

		AddText(Blueprint, Canvas, TEXT("MonsterCenterNameText"),
			NSLOCTEXT("MarchboundMonsterTab", "CenterWerewolf", "늑대인간"), 44,
			FVector2D(MidCell.X + Gutter, MidCell.Y + 684.0f),
			FVector2D(MidSize.X - Gutter * 2.f, 76.0f), 20);
		AddText(Blueprint, Canvas, TEXT("MonsterDetailTypeText"),
			NSLOCTEXT("MarchboundMonsterTab", "DetailType", "야수 · 근접"), 28,
			FVector2D(MidCell.X + Gutter, MidCell.Y + 766.0f),
			FVector2D(MidSize.X - Gutter * 2.f, 48.0f), 20);

		/*
		 * ── 오른쪽 칸: 고른 몬스터의 값 ──────────────────────────────
		 *
		 * 왼쪽은 **고르는 곳**, 오른쪽은 **고른 결과**다. 전에는 수치와 상태가
		 * 왼쪽 목록 바로 밑에 붙어 있어서, 줄을 눌러도 무엇이 바뀌었는지
		 * 눈이 못 따라갔다.
		 *
		 *     이름
		 *     수치 칩 넷      HP · AP · 방어 · 속도
		 *     HP 막대
		 *     스킬 칸 넷      아이콘 한 줄
		 *     상태
		 */
		const float RightInner = RightSize.X - Gutter * 2.f;
		const float RightLeft = RightCell.X + Gutter;
		float Cursor = RightCell.Y + 20.f;

		AddText(Blueprint, Canvas, TEXT("MonsterDetailNameText"),
			NSLOCTEXT("MarchboundMonsterTab", "DetailWerewolf", "늑대인간"), 54,
			FVector2D(RightLeft, Cursor), FVector2D(RightInner, 82.0f), 20, Ink);
		Cursor += 96.f;

		UTexture2D* ChipRing = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_StatChip_Ring.T_KitA_StatChip_Ring"));
		const TCHAR* const ChipValueNames[4] = {
			TEXT("MonsterDetailHPText"), TEXT("MonsterDetailAPText"),
			TEXT("MonsterDetailDefenseText"), TEXT("MonsterDetailSpeedText") };
		const TCHAR* const ChipLabels[4] = {
			TEXT("HP"), TEXT("AP"), TEXT("방어"), TEXT("속도") };
		const float ChipRoom = RightInner / 4.f;
		const float ChipSide = FMath::Min(ChipRoom - 14.f, 152.f);
		for (int32 Index = 0; Index < 4; ++Index)
		{
			const FVector2D ChipPos(
				RightLeft + ChipRoom * Index + (ChipRoom - ChipSide) * 0.5f, Cursor);
			const FVector2D ChipSize(ChipSide, ChipSide);
			AddImage(Blueprint, Canvas, FName(*FString::Printf(
				TEXT("MonsterChip%dFrame"), Index)), ChipRing, ChipPos, ChipSize, 19, true);
			// 글자는 링 **안쪽 구멍**에만 넣는다. 링을 밟으면 안 읽힌다.
			const FBox2D Ring = UIPartRects::Inner(TEXT("T_KitA_StatChip_Ring"),
				ChipPos, ChipSize, false);
			const FVector2D RingSpan = Ring.GetSize();
			AddText(Blueprint, Canvas, FName(*FString::Printf(
				TEXT("MonsterChip%dLabel"), Index)), FText::FromString(ChipLabels[Index]),
				FMath::RoundToInt(RingSpan.Y * 0.26f), Ring.Min,
				FVector2D(RingSpan.X, RingSpan.Y * 0.38f), 20, Accent);
			AddText(Blueprint, Canvas, ChipValueNames[Index], FText::FromString(TEXT("-")),
				FMath::RoundToInt(RingSpan.Y * 0.42f),
				Ring.Min + FVector2D(0.f, RingSpan.Y * 0.38f),
				FVector2D(RingSpan.X, RingSpan.Y * 0.62f), 20, Ink);
		}
		Cursor += ChipSide + 18.f;

		// HP 막대. 칩의 숫자와 같은 값이지만 남은 양은 막대가 한눈에 보인다.
		UProgressBar* HPBar = Blueprint->WidgetTree->ConstructWidget<UProgressBar>(
			UProgressBar::StaticClass(), TEXT("MonsterDetailHPBar"));
		HPBar->SetPercent(1.0f);
		HPBar->SetFillColorAndOpacity(FLinearColor(0.74f, 0.04f, 0.025f, 1.0f));
		Place(Canvas, HPBar, FVector2D(RightLeft, Cursor), FVector2D(RightInner, 30.f), 20);
		Expose(Blueprint, HPBar);
		Cursor += 52.f;

		AddText(Blueprint, Canvas, TEXT("MonsterSkillHeading"),
			NSLOCTEXT("MarchboundMonsterTab", "SkillHeading", "스킬"), 32,
			FVector2D(RightLeft, Cursor), FVector2D(RightInner, 48.0f), 20, Accent);
		Cursor += 56.f;

		// 스킬은 아이콘 한 줄. 이름은 아이콘이 없을 때만 런타임이 켠다.
		UTexture2D* SkillSlotFrame = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_Cell_Normal.T_KitA_Cell_Normal"));
		const float SkillRoom = RightInner / 4.f;
		const float SkillSide = FMath::Min(SkillRoom - 14.f, 168.f);
		for (int32 Index = 0; Index < 4; ++Index)
		{
			const FVector2D SlotPos(
				RightLeft + SkillRoom * Index + (SkillRoom - SkillSide) * 0.5f, Cursor);
			const FVector2D SlotSize(SkillSide, SkillSide);
			AddImage(Blueprint, Canvas, FName(*FString::Printf(
				TEXT("MonsterSkillSlot_%d"), Index)), SkillSlotFrame, SlotPos, SlotSize, 21, true);
			const FBox2D SlotInner = UIPartRects::Inner(TEXT("T_KitA_Cell_Normal"),
				SlotPos, SlotSize, false);
			AddImage(Blueprint, Canvas, FName(*FString::Printf(
				TEXT("MonsterSkillIcon_%d"), Index)), nullptr,
				SlotInner.Min, SlotInner.GetSize(), 22, true);
			AddText(Blueprint, Canvas, FName(*FString::Printf(
				TEXT("MonsterSkillName_%d"), Index)), FText::FromString(TEXT("")), 24,
				SlotInner.Min, SlotInner.GetSize(), 23, Ink);
		}
		Cursor += SkillSide + 28.f;

		AddText(Blueprint, Canvas, TEXT("MonsterStatusHeading"),
			NSLOCTEXT("MarchboundMonsterTab", "StatusHeading", "상태"), 32,
			FVector2D(RightLeft, Cursor), FVector2D(RightInner, 48.0f), 20, Accent);
		Cursor += 56.f;
		for (int32 Index = 0; Index < 2; ++Index)
		{
			AddText(Blueprint, Canvas, FName(*FString::Printf(
				TEXT("MonsterStatusText_%d"), Index)), FText::FromString(TEXT("")), 27,
				FVector2D(RightLeft + RightInner * 0.5f * Index, Cursor),
				FVector2D(RightInner * 0.5f, 52.0f), 20, Ink);
		}

		/*
		 * 트리를 통째로 다시 만드는 빌더라, 지난 배치에만 있던 이름은 변수
		 * GUID 만 남는다. 그대로 두면 컴파일마다 "was deleted but still has a
		 * GUID" ensure 가 뜬다. 살아 있는 이름을 모아 두고 나머지를 걷는다.
		 */
		{
			TSet<FName> Live;
			Blueprint->WidgetTree->ForEachWidget([&Live](UWidget* Widget)
			{
				if (Widget != nullptr)
				{
					Live.Add(Widget->GetFName());
				}
			});
			TArray<FName> Stale;
			for (const TPair<FName, FGuid>& Entry : Blueprint->WidgetVariableNameToGuidMap)
			{
				if (Live.Contains(Entry.Key) == false)
				{
					Stale.Add(Entry.Key);
				}
			}
			for (const FName& Name : Stale)
			{
				Blueprint->WidgetVariableNameToGuidMap.Remove(Name);
				Blueprint->OnVariableRemoved(Name);
			}
		}

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
			TEXT("RD_MONSTER_TAB_BUILD success asset=%s rows=3 skills=4 design=1920x1080 layout=mobile-v2"),
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
