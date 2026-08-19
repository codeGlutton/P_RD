#include "UI/CombatHUDWidgetBuilder.h"
#include "UI/UIPartRects.h"
#include "UI/UIFont.h"

#include "AssetToolsModule.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "HAL/IConsoleManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/SavePackage.h"

namespace CombatHUDWidgetBuilder
{
	constexpr TCHAR AssetPath[] =
		TEXT("/Game/UI/CombatLayouts/WBP_CombatHUD04.WBP_CombatHUD04");
	constexpr TCHAR ActionButtonTexturePath[] =
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/HUD04/T_Combat_Button_Wood_SkillConfirm_20260811_v3.T_Combat_Button_Wood_SkillConfirm_20260811_v3");
	constexpr TCHAR OptionsRailFrameTexturePath[] =
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_OptionsRail_Frame.T_MB_OptionsRail_Frame");
	constexpr TCHAR MercenaryPortraitFrameTexturePath[] =
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_Portrait_Frame.T_KitA_Portrait_Frame");
	constexpr TCHAR DetailPortraitCellTexturePath[] =
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_Cell_Normal.T_KitA_Cell_Normal");
	// 보유 용병 패널은 자기 판을 쓴다. 왜 나눴는지는 BuildMercenaryPanel 참고.
	constexpr TCHAR MercenaryPackagePath[] = TEXT("/Game/UI/CombatLayouts");
	constexpr TCHAR MercenaryAssetName[] = TEXT("WBP_MercenaryPanel");
	constexpr TCHAR MercenaryAssetPath[] =
		TEXT("/Game/UI/CombatLayouts/WBP_MercenaryPanel.WBP_MercenaryPanel");
	TUniquePtr<FAutoConsoleCommand> BuildCommand;
	TUniquePtr<FAutoConsoleCommand> InventoryBuildCommand;
	TUniquePtr<FAutoConsoleCommand> RoundTurnRepairCommand;
	TUniquePtr<FAutoConsoleCommand> ActionButtonArtRepairCommand;
	TUniquePtr<FAutoConsoleCommand> RightHUDRepairCommand;
	TUniquePtr<FAutoConsoleCommand> MercenaryPortraitFrameRepairCommand;

	template <typename T>
	T* FindOrCreate(UWidgetBlueprint* Blueprint, const FName Name)
	{
		if (UWidget* Existing = Blueprint->WidgetTree->FindWidget(Name))
		{
			return CastChecked<T>(Existing);
		}
		T* NewWidget = Blueprint->WidgetTree->ConstructWidget<T>(T::StaticClass(), Name);
		Blueprint->OnVariableAdded(Name);
		return NewWidget;
	}

	/** @brief 판이 없으면 만들고 뿌리 캔버스를 보장한다. */
	UWidgetBlueprint* EnsureBlueprint(const TCHAR* Path, const TCHAR* PackagePath,
		const TCHAR* AssetName)
	{
		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, Path);
		if (Blueprint == nullptr)
		{
			UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
			Factory->ParentClass = UUserWidget::StaticClass();
			FAssetToolsModule& AssetTools =
				FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			Blueprint = Cast<UWidgetBlueprint>(AssetTools.Get().CreateAsset(
				AssetName, PackagePath, UWidgetBlueprint::StaticClass(), Factory));
		}
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
		{
			return nullptr;
		}
		if (Cast<UCanvasPanel>(Blueprint->WidgetTree->RootWidget) == nullptr)
		{
			UCanvasPanel* Canvas = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
				UCanvasPanel::StaticClass(), TEXT("MercenaryRootCanvas"));
			Blueprint->WidgetTree->RootWidget = Canvas;
		}
		return Blueprint;
	}

	/** @brief 이번 배치에 없는 옛 이름의 변수 GUID 를 걷는다. */
	void PruneStaleVariables(UWidgetBlueprint* Blueprint)
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

	void EnsureParent(UPanelWidget* Parent, UWidget* Child)
	{
		check(Parent != nullptr && Child != nullptr);
		if (Child->GetParent() == Parent)
		{
			return;
		}
		if (UPanelWidget* OldParent = Child->GetParent())
		{
			OldParent->RemoveChild(Child);
		}
		Parent->AddChild(Child);
	}

	void PlaceCanvas(UCanvasPanel* Parent, UWidget* Child, const FVector2D Position,
		const FVector2D Size, const int32 ZOrder)
	{
		EnsureParent(Parent, Child);
		UCanvasPanelSlot* Slot = CastChecked<UCanvasPanelSlot>(Child->Slot);
		Slot->SetAnchors(FAnchors(0.f));
		Slot->SetAlignment(FVector2D::ZeroVector);
		Slot->SetAutoSize(false);
		Slot->SetPosition(Position);
		Slot->SetSize(Size);
		Slot->SetZOrder(ZOrder);
	}

	void SetReadableFont(UTextBlock* Text, const FSlateFontInfo& Source, const int32 Size)
	{
		FSlateFontInfo Font = UIFont::Make(Source, Size);
		Font.OutlineSettings.OutlineSize = 2;
		Font.OutlineSettings.OutlineColor = FLinearColor(0.f, 0.f, 0.f, 0.9f);
		Text->SetFont(Font);
		Text->SetJustification(ETextJustify::Center);
		Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Text->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	void SetInvisibleButtonChrome(UButton* Button)
	{
		FButtonStyle Style = Button->GetStyle();
		Style.Normal.DrawAs = ESlateBrushDrawType::NoDrawType;
		Style.Hovered.DrawAs = ESlateBrushDrawType::NoDrawType;
		Style.Pressed.DrawAs = ESlateBrushDrawType::NoDrawType;
		Style.Disabled.DrawAs = ESlateBrushDrawType::NoDrawType;
		Button->SetStyle(Style);
	}

	/**
	 * @brief 0811에서 확정한 ROUND 배지와 턴 카드 줄을 WBP에 다시 굽는다.
	 *
	 * @details
	 * ROUND는 별도 HUD 구석 장식이 아니라 턴 카드 줄의 첫 칸이다. 둘을 같은
	 * 좌상단 기준으로 연달아 놓고, 카드 안의 중앙정렬 래퍼도 그대로 보존한다.
	 * 전체 빌더와 전용 복구 명령이 반드시 이 함수 하나를 써야 좌표가 다시
	 * 갈라지지 않는다.
	 */
	void RepairAuthoredRoundTurnLayout(UWidgetBlueprint* Blueprint,
		UTexture2D* RoundBadgeTexture, UTexture2D* TurnTokenFrameTexture,
		UTexture2D* SpeedTexture)
	{
		check(Blueprint != nullptr && Blueprint->WidgetTree != nullptr);
		check(RoundBadgeTexture != nullptr && TurnTokenFrameTexture != nullptr
			&& SpeedTexture != nullptr);

		UCanvasPanel* Root = CastChecked<UCanvasPanel>(
			Blueprint->WidgetTree->RootWidget);
		UCanvasPanel* RoundPanel = CastChecked<UCanvasPanel>(
			Blueprint->WidgetTree->FindWidget(TEXT("RoundPanel")));
		PlaceCanvas(Root, RoundPanel, FVector2D(8.f, 8.f),
			FVector2D(218.f, 166.f), 100);
		RoundPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UOverlay* RoundPlateMount = CastChecked<UOverlay>(
			Blueprint->WidgetTree->FindWidget(TEXT("RoundPlateMount")));
		PlaceCanvas(RoundPanel, RoundPlateMount, FVector2D::ZeroVector,
			FVector2D(218.f, 68.f), 5);
		RoundPlateMount->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UImage* RoundPlate = CastChecked<UImage>(
			Blueprint->WidgetTree->FindWidget(TEXT("RoundPlate")));
		EnsureParent(RoundPlateMount, RoundPlate);
		FSlateBrush RoundPlateBrush = RoundPlate->GetBrush();
		RoundPlateBrush.SetResourceObject(RoundBadgeTexture);
		RoundPlateBrush.DrawAs = ESlateBrushDrawType::Image;
		RoundPlateBrush.Margin = FMargin(0.f);
		RoundPlate->SetBrush(RoundPlateBrush);
		RoundPlate->SetColorAndOpacity(FLinearColor::White);
		RoundPlate->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UOverlaySlot* Slot = CastChecked<UOverlaySlot>(RoundPlate->Slot))
		{
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Fill);
			Slot->SetPadding(FMargin(0.f));
		}

		UOverlay* RoundTextCenter = CastChecked<UOverlay>(
			Blueprint->WidgetTree->FindWidget(TEXT("RoundText_Center")));
		EnsureParent(RoundPlateMount, RoundTextCenter);
		RoundTextCenter->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UOverlaySlot* Slot = CastChecked<UOverlaySlot>(RoundTextCenter->Slot))
		{
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Fill);
			Slot->SetPadding(FMargin(22.f, 8.f, 22.f, 8.f));
		}

		UTextBlock* RoundText = CastChecked<UTextBlock>(
			Blueprint->WidgetTree->FindWidget(TEXT("RoundText")));
		EnsureParent(RoundTextCenter, RoundText);
		if (UOverlaySlot* Slot = CastChecked<UOverlaySlot>(RoundText->Slot))
		{
			Slot->SetHorizontalAlignment(HAlign_Center);
			Slot->SetVerticalAlignment(VAlign_Center);
			Slot->SetPadding(FMargin(0.f, -5.f, 0.f, 0.f));
		}
		FSlateFontInfo RoundFont = UIFont::MakeProjectExact(RoundText->GetFont(), 30);
		RoundFont.LetterSpacing = 0;
		RoundFont.OutlineSettings.OutlineSize = 2;
		RoundFont.OutlineSettings.OutlineColor = FLinearColor::Black;
		RoundText->SetFont(RoundFont);
		RoundText->SetText(NSLOCTEXT("CombatHUD", "RoundPreview", "ROUND 1"));
		RoundText->SetJustification(ETextJustify::Center);
		RoundText->SetColorAndOpacity(FSlateColor(
			FLinearColor(.973f, .973f, .953f, 1.f)));
		RoundText->SetShadowOffset(FVector2D(2.f, 3.f));
		RoundText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, .55f));
		RoundText->SetAutoWrapText(false);
		RoundText->SetMinDesiredWidth(0.f);
		RoundText->SetVisibility(ESlateVisibility::HitTestInvisible);
		// 6f 자산에만 남은 구형 임무 문구다. 런타임 이름 계약은 유지하되,
		// canonical 0811에는 없는 행이므로 ROUND 배지 아래에서 그리지 않는다.
		if (UWidget* ObjectiveTextCenter = Blueprint->WidgetTree->FindWidget(
			TEXT("ObjectiveText_Center")))
		{
			ObjectiveTextCenter->SetVisibility(ESlateVisibility::Collapsed);
		}

		UCanvasPanel* TurnPanel = CastChecked<UCanvasPanel>(
			Blueprint->WidgetTree->FindWidget(TEXT("TurnPanel")));
		PlaceCanvas(Root, TurnPanel, FVector2D(236.f, 8.f),
			FVector2D(1090.f, 174.f), 90);
		TurnPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		for (int32 Index = 0; Index < 10; ++Index)
		{
			const float X = 5.f + 109.f * Index;
			const FString Tail = FString::Printf(TEXT("_%d"), Index);

			UCanvasPanel* Token = CastChecked<UCanvasPanel>(
				Blueprint->WidgetTree->FindWidget(FName(TEXT("TurnToken") + Tail)));
			PlaceCanvas(TurnPanel, Token, FVector2D(X, 30.f),
				FVector2D(108.f, 144.f), 10);
			Token->SetClipping(EWidgetClipping::ClipToBoundsAlways);
			Token->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			// 현재 런타임은 버튼을 RefreshTurnOrder에서 다시 켜지 않는다. donor의
			// 저장 Collapsed를 복제하면 카드 선택만 조용히 죽으므로 Visible을 보존한다.
			if (UButton* Button = Cast<UButton>(Blueprint->WidgetTree->FindWidget(
				FName(TEXT("TurnTokenButton") + Tail))))
			{
				PlaceCanvas(TurnPanel, Button, FVector2D(X, 30.f),
					FVector2D(108.f, 144.f), 40);
				Button->SetVisibility(ESlateVisibility::Visible);
			}

			UImage* Frame = FindOrCreate<UImage>(Blueprint,
				FName(TEXT("TurnFrame") + Tail));
			PlaceCanvas(Token, Frame, FVector2D::ZeroVector,
				FVector2D(108.f, 144.f), 5);
			Frame->SetBrushFromTexture(TurnTokenFrameTexture, false);
			Frame->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			UScaleBox* Crop = CastChecked<UScaleBox>(
				Blueprint->WidgetTree->FindWidget(FName(TEXT("TurnPortraitCrop") + Tail)));
			PlaceCanvas(Token, Crop, FVector2D(18.f, 21.f),
				FVector2D(72.f, 72.f), 10);
			Crop->SetStretch(EStretch::ScaleToFill);
			Crop->SetStretchDirection(EStretchDirection::Both);
			Crop->SetClipping(EWidgetClipping::ClipToBoundsAlways);
			Crop->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			UImage* Current = CastChecked<UImage>(
				Blueprint->WidgetTree->FindWidget(FName(TEXT("TurnCurrent") + Tail)));
			PlaceCanvas(Token, Current, FVector2D(11.25f, 15.f),
				FVector2D(85.f, 85.75f), 40);
			Current->SetVisibility(ESlateVisibility::Collapsed);

			UBorder* SpeedPlate = FindOrCreate<UBorder>(Blueprint,
				FName(TEXT("TurnSpeedPlate") + Tail));
			PlaceCanvas(Token, SpeedPlate, FVector2D(4.f, 73.f),
				FVector2D(70.f, 31.f), 20);
			SpeedPlate->SetBrushColor(FLinearColor(.055f, .04f, .025f, .92f));
			SpeedPlate->SetVisibility(ESlateVisibility::Collapsed);

			UImage* SpeedIcon = FindOrCreate<UImage>(Blueprint,
				FName(TEXT("TurnSpeedIcon") + Tail));
			PlaceCanvas(Token, SpeedIcon, FVector2D(20.f, 99.f),
				FVector2D(22.8f, 22.8f), 22);
			SpeedIcon->SetBrushFromTexture(SpeedTexture, false);
			SpeedIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			UOverlay* SpeedCenter = CastChecked<UOverlay>(
				Blueprint->WidgetTree->FindWidget(FName(TEXT("TurnSpeed") + Tail
					+ TEXT("_Center"))));
			PlaceCanvas(Token, SpeedCenter, FVector2D(40.f, 96.f),
				FVector2D(51.625301f, 30.299999f), 22);
			SpeedCenter->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			UTextBlock* Speed = CastChecked<UTextBlock>(
				Blueprint->WidgetTree->FindWidget(FName(TEXT("TurnSpeed") + Tail)));
			EnsureParent(SpeedCenter, Speed);
			if (UOverlaySlot* Slot = CastChecked<UOverlaySlot>(Speed->Slot))
			{
				Slot->SetHorizontalAlignment(HAlign_Center);
				Slot->SetVerticalAlignment(VAlign_Fill);
				Slot->SetPadding(FMargin(0.f, -1.613333f, 0.f, 0.f));
			}
			FSlateFontInfo SpeedFont = UIFont::MakeProjectExact(Speed->GetFont(), 16);
			SpeedFont.LetterSpacing = 0;
			SpeedFont.OutlineSettings.OutlineSize = 2;
			SpeedFont.OutlineSettings.OutlineColor = FLinearColor::Black;
			Speed->SetFont(SpeedFont);
			Speed->SetText(FText::FromString(TEXT("0")));
			Speed->SetJustification(ETextJustify::Left);
			Speed->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			Speed->SetShadowOffset(FVector2D(1.f, 1.f));
			Speed->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.f));
			Speed->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			UOverlay* DividerMount = CastChecked<UOverlay>(
				Blueprint->WidgetTree->FindWidget(FName(
					TEXT("TurnRoundDivider") + Tail + TEXT("Mount"))));
			PlaceCanvas(TurnPanel, DividerMount, FVector2D(X, 0.f),
				FVector2D(108.f, 34.f), 30);
			DividerMount->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			UBorder* Divider = CastChecked<UBorder>(
				Blueprint->WidgetTree->FindWidget(FName(TEXT("TurnRoundDivider") + Tail)));
			EnsureParent(DividerMount, Divider);
			FSlateBrush DividerBrush = Divider->Background;
			DividerBrush.SetResourceObject(RoundBadgeTexture);
			DividerBrush.DrawAs = ESlateBrushDrawType::Image;
			DividerBrush.Margin = FMargin(0.f);
			Divider->SetBrush(DividerBrush);
			Divider->SetBrushColor(FLinearColor::White);
			Divider->SetVisibility(ESlateVisibility::Collapsed);
			if (UOverlaySlot* Slot = CastChecked<UOverlaySlot>(Divider->Slot))
			{
				Slot->SetHorizontalAlignment(HAlign_Fill);
				Slot->SetVerticalAlignment(VAlign_Fill);
				Slot->SetPadding(FMargin(0.f));
			}

			UOverlay* LabelCenter = CastChecked<UOverlay>(
				Blueprint->WidgetTree->FindWidget(FName(
					TEXT("TurnRoundLabel") + Tail + TEXT("_Center"))));
			EnsureParent(DividerMount, LabelCenter);
			LabelCenter->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			if (UOverlaySlot* Slot = CastChecked<UOverlaySlot>(LabelCenter->Slot))
			{
				Slot->SetHorizontalAlignment(HAlign_Fill);
				Slot->SetVerticalAlignment(VAlign_Fill);
				Slot->SetPadding(FMargin(0.f, 2.f, 0.f, 2.f));
			}

			UTextBlock* Label = CastChecked<UTextBlock>(
				Blueprint->WidgetTree->FindWidget(FName(TEXT("TurnRoundLabel") + Tail)));
			EnsureParent(LabelCenter, Label);
			if (UOverlaySlot* Slot = CastChecked<UOverlaySlot>(Label->Slot))
			{
				Slot->SetHorizontalAlignment(HAlign_Fill);
				Slot->SetVerticalAlignment(VAlign_Top);
				Slot->SetPadding(FMargin(0.f, -6.5f, 0.f, 0.f));
			}
			FSlateFontInfo LabelFont = UIFont::MakeProjectExact(Label->GetFont(), 19);
			LabelFont.LetterSpacing = 0;
			LabelFont.OutlineSettings.OutlineSize = 2;
			LabelFont.OutlineSettings.OutlineColor = FLinearColor::Black;
			Label->SetFont(LabelFont);
			Label->SetText(FText::FromString(FString::Printf(TEXT("R%d"), Index + 1)));
			Label->SetJustification(ETextJustify::Center);
			Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			Label->SetShadowOffset(FVector2D(1.f, 1.f));
			Label->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, .9f));
			Label->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	/** @brief #519의 스킬/턴 종료 공용 목재 버튼 그림만 다시 연결한다. */
	void RepairAuthoredActionButtonArt(UWidgetBlueprint* Blueprint,
		UTexture2D* ActionButtonTexture)
	{
		check(Blueprint != nullptr && Blueprint->WidgetTree != nullptr);
		check(ActionButtonTexture != nullptr);

		const FName PlateNames[] = {
			TEXT("SkillTogglePlate"), TEXT("EndTurnPlate")
		};
		for (const FName& PlateName : PlateNames)
		{
			UImage* Plate = CastChecked<UImage>(
				Blueprint->WidgetTree->FindWidget(PlateName));
			FSlateBrush Brush = Plate->GetBrush();
			Brush.SetResourceObject(ActionButtonTexture);
			Brush.DrawAs = ESlateBrushDrawType::Image;
			Brush.Margin = FMargin(0.f);
			Plate->SetBrush(Brush);
			Plate->SetColorAndOpacity(FLinearColor::White);
		}
	}

	/** 실제 목재 면(원본 1862x845에서 잰 칸)에 라벨 겹을 맞춘다. */
	void FitAuthoredActionButtonLabel(UOverlay* LabelCenter,
		const FVector2D& Bounds)
	{
		if (LabelCenter == nullptr)
		{
			return;
		}
		// 장식 모서리와 바깥 들보를 제외한 중앙 목재 면. 전체 그림 중심보다
		// 약간 위라서 full bounds 중앙 정렬 때 글자가 아래로 처져 보였다.
		constexpr float Left = .082f;
		constexpr float Top = .178f;
		constexpr float Right = .082f;
		constexpr float Bottom = .202f;
		if (UOverlaySlot* Slot = Cast<UOverlaySlot>(LabelCenter->Slot))
		{
			Slot->SetPadding(FMargin(
				Bounds.X * Left, Bounds.Y * Top,
				Bounds.X * Right, Bounds.Y * Bottom));
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Fill);
		}
	}

	/**
	 * @brief 0811 확정본의 우측 메뉴, 요약판, 행동 단추 배치만 복구한다.
	 *
	 * @details ROUND/턴바와 용병 판은 이 함수에서 이름조차 찾지 않는다. 요약판
	 * 내부 역시 디자이너 자산을 그대로 두고, 화면에 붙는 루트 슬롯만 복구한다.
	 */
	void RepairAuthoredRightHUDLayout(UWidgetBlueprint* Blueprint,
		UTexture2D* OptionsRailFrameTexture)
	{
		check(Blueprint != nullptr && Blueprint->WidgetTree != nullptr);
		check(OptionsRailFrameTexture != nullptr);

		UCanvasPanel* Root = CastChecked<UCanvasPanel>(
			Blueprint->WidgetTree->RootWidget);
		auto SetOverlaySlot = [](UWidget* Widget, const FMargin Padding,
			const EHorizontalAlignment Horizontal,
			const EVerticalAlignment Vertical)
		{
			UOverlaySlot* Slot = CastChecked<UOverlaySlot>(Widget->Slot);
			Slot->SetPadding(Padding);
			Slot->SetHorizontalAlignment(Horizontal);
			Slot->SetVerticalAlignment(Vertical);
		};
		auto ResetVisualTransform = [](UWidget* Widget, const FVector2D Pivot)
		{
			Widget->SetRenderTransform(FWidgetTransform());
			Widget->SetRenderTransformPivot(Pivot);
			Widget->SetRenderOpacity(1.f);
			Widget->SetClipping(EWidgetClipping::Inherit);
		};

		// 우상단 설정 바. 0~2번 단추는 donor의 Overlay 여백을 쓰고,
		// 3번 단추만 ObjectivePanel의 Canvas 자식인 구조도 그대로 보존한다.
		UCanvasPanel* Objective = CastChecked<UCanvasPanel>(
			Blueprint->WidgetTree->FindWidget(TEXT("ObjectivePanel")));
		PlaceCanvas(Root, Objective, FVector2D(-470.f, 4.f),
			FVector2D(470.f, 173.f), 90);
		if (UCanvasPanelSlot* Slot = CastChecked<UCanvasPanelSlot>(Objective->Slot))
		{
			Slot->SetAnchors(FAnchors(1.f, 0.f));
			Slot->SetAlignment(FVector2D::ZeroVector);
		}
		Objective->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		FWidgetTransform ObjectiveTransform;
		ObjectiveTransform.Scale = FVector2D(.75f, .75f);
		Objective->SetRenderTransform(ObjectiveTransform);
		Objective->SetRenderTransformPivot(FVector2D(1.f, 0.f));
		Objective->SetRenderOpacity(1.f);
		Objective->SetClipping(EWidgetClipping::Inherit);
		// 0811 donor에는 없는 구형 임무 장식이다. 그래프/런타임 이름 호환을
		// 위해 객체는 남기되 우측 설정 바와 겹치지 않도록 항상 접어 둔다.
		for (const FName RetiredName : { FName(TEXT("ObjectivePlate")),
			FName(TEXT("ObjectiveText_Center")), FName(TEXT("ObjectiveText")) })
		{
			if (UWidget* Retired = Blueprint->WidgetTree->FindWidget(RetiredName))
			{
				Retired->SetVisibility(ESlateVisibility::Collapsed);
			}
		}

		UOverlay* OptionsMount = FindOrCreate<UOverlay>(Blueprint,
			TEXT("OptionsRailFrameMount"));
		PlaceCanvas(Objective, OptionsMount, FVector2D::ZeroVector,
			FVector2D(470.f, 173.f), 1);
		OptionsMount->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		ResetVisualTransform(OptionsMount, FVector2D(.5f, .5f));

		UImage* OptionsFrame = FindOrCreate<UImage>(Blueprint,
			TEXT("OptionsRailFrame"));
		EnsureParent(OptionsMount, OptionsFrame);
		OptionsFrame->SetBrushFromTexture(OptionsRailFrameTexture, false);
		OptionsFrame->SetColorAndOpacity(FLinearColor::White);
		OptionsFrame->SetVisibility(ESlateVisibility::HitTestInvisible);
		SetOverlaySlot(OptionsFrame, FMargin(0.f), HAlign_Fill, VAlign_Fill);

		const FMargin MenuButtonPaddings[] = {
			FMargin(37.f, 31.f, 339.f, 30.f),
			FMargin(138.f, 31.f, 238.f, 30.f),
			FMargin(242.f, 31.f, 140.f, 30.f),
		};
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(MenuButtonPaddings); ++Index)
		{
			UButton* Button = CastChecked<UButton>(Blueprint->WidgetTree->FindWidget(
				FName(*FString::Printf(TEXT("MenuButton_%d"), Index))));
			EnsureParent(OptionsMount, Button);
			SetOverlaySlot(Button, MenuButtonPaddings[Index], HAlign_Fill, VAlign_Fill);
			Button->SetVisibility(ESlateVisibility::Visible);
		}
		UButton* SettingsButton = CastChecked<UButton>(
			Blueprint->WidgetTree->FindWidget(TEXT("MenuButton_3")));
		PlaceCanvas(Objective, SettingsButton, FVector2D(344.f, 36.f),
			FVector2D(83.5f, 102.000008f), 0);
		SettingsButton->SetVisibility(ESlateVisibility::Visible);

		struct FIconLayout
		{
			FName Name;
			FVector2D Position;
			FVector2D Size;
		};
		const FIconLayout IconLayouts[] = {
			{ TEXT("MenuMapIcon"), FVector2D(45.f, 44.f), FVector2D(80.f, 81.f) },
			{ TEXT("MenuMercenaryIcon"), FVector2D(154.f, 36.f), FVector2D(63.f, 96.f) },
			{ TEXT("MenuMonsterIcon"), FVector2D(248.f, 48.f), FVector2D(77.f, 83.f) },
			{ TEXT("MenuSettingsIcon"), FVector2D(348.f, 48.f), FVector2D(77.f, 80.f) },
		};
		for (const FIconLayout& Layout : IconLayouts)
		{
			UImage* Icon = CastChecked<UImage>(
				Blueprint->WidgetTree->FindWidget(Layout.Name));
			PlaceCanvas(Objective, Icon, Layout.Position, Layout.Size, 31);
			Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}

		// 양 진영은 같은 우상단 요약판 자리를 번갈아 쓴다. 내부 authored
		// 위젯은 보존하고 루트 슬롯/피벗/저장 visibility만 canonical로 맞춘다.
		for (const FName PanelName : { FName(TEXT("EnemyPanel")),
			FName(TEXT("AllyPanel")) })
		{
			UCanvasPanel* Panel = CastChecked<UCanvasPanel>(
				Blueprint->WidgetTree->FindWidget(PanelName));
			PlaceCanvas(Root, Panel, FVector2D(0.f, 140.f),
				FVector2D(600.f, 430.f), 60);
			if (UCanvasPanelSlot* Slot = CastChecked<UCanvasPanelSlot>(Panel->Slot))
			{
				Slot->SetAnchors(FAnchors(1.f, 0.f));
				Slot->SetAlignment(FVector2D(1.f, 0.f));
			}
			Panel->SetVisibility(ESlateVisibility::Collapsed);
			ResetVisualTransform(Panel, FVector2D(1.f, 0.f));
			for (int32 Index = 0; Index < 3; ++Index)
			{
				if (UWidget* StatusButton = Blueprint->WidgetTree->FindWidget(FName(
					*FString::Printf(TEXT("%sStatusButton_%d"),
						*PanelName.ToString().LeftChop(5), Index))))
				{
					StatusButton->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
		}

		const FVector2D ActionSize(396.172241f, 181.435410f);
		auto PlaceActionPanel = [&](const FName PanelName,
			const FName MountName, const FVector2D Position)
		{
			UCanvasPanel* Panel = FindOrCreate<UCanvasPanel>(Blueprint, PanelName);
			PlaceCanvas(Root, Panel, Position, ActionSize, 60);
			if (UCanvasPanelSlot* Slot = CastChecked<UCanvasPanelSlot>(Panel->Slot))
			{
				Slot->SetAnchors(FAnchors(1.f, 1.f));
				Slot->SetAlignment(FVector2D(1.f, 1.f));
			}
			Panel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			ResetVisualTransform(Panel, FVector2D(.5f, .5f));

			UOverlay* Mount = FindOrCreate<UOverlay>(Blueprint, MountName);
			PlaceCanvas(Panel, Mount, FVector2D::ZeroVector, ActionSize, 0);
			Mount->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			ResetVisualTransform(Mount, FVector2D(.5f, .5f));
			return Mount;
		};

		UOverlay* SkillMount = PlaceActionPanel(TEXT("SkillTogglePanel"),
			TEXT("SkillTogglePlateMount"), FVector2D(-10.334961f, -209.789490f));
		UImage* SkillPlate = CastChecked<UImage>(
			Blueprint->WidgetTree->FindWidget(TEXT("SkillTogglePlate")));
		EnsureParent(SkillMount, SkillPlate);
		SkillPlate->SetVisibility(ESlateVisibility::HitTestInvisible);
		SetOverlaySlot(SkillPlate, FMargin(0.f), HAlign_Fill, VAlign_Fill);

		UOverlay* SkillLabelCenter = FindOrCreate<UOverlay>(Blueprint,
			TEXT("SkillToggleLabel_Center"));
		EnsureParent(SkillMount, SkillLabelCenter);
		SkillLabelCenter->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		SetOverlaySlot(SkillLabelCenter, FMargin(0.f), HAlign_Fill, VAlign_Fill);
		FitAuthoredActionButtonLabel(SkillLabelCenter, ActionSize);
		UTextBlock* SkillLabel = CastChecked<UTextBlock>(
			Blueprint->WidgetTree->FindWidget(TEXT("SkillToggleLabel")));
		EnsureParent(SkillLabelCenter, SkillLabel);
		SetOverlaySlot(SkillLabel, FMargin(0.f), HAlign_Center, VAlign_Center);
		FSlateFontInfo SkillFont = UIFont::MakeProjectExact(SkillLabel->GetFont(), 40);
		SkillFont.LetterSpacing = 0;
		SkillFont.OutlineSettings.OutlineSize = 2;
		SkillFont.OutlineSettings.OutlineColor = FLinearColor::Black;
		SkillLabel->SetFont(SkillFont);
		SkillLabel->SetText(NSLOCTEXT("CombatHUD", "SkillToggleLabel", "스킬"));
		SkillLabel->SetJustification(ETextJustify::Center);
		SkillLabel->SetColorAndOpacity(FSlateColor(
			FLinearColor(1.f, .921582f, .723055f, 1.f)));
		SkillLabel->SetShadowOffset(FVector2D(1.f, 1.f));
		SkillLabel->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.f));
		SkillLabel->SetAutoWrapText(false);
		SkillLabel->SetMinDesiredWidth(0.f);
		SkillLabel->SetVisibility(ESlateVisibility::HitTestInvisible);
		UButton* SkillButton = CastChecked<UButton>(
			Blueprint->WidgetTree->FindWidget(TEXT("SkillToggleButton")));
		EnsureParent(SkillMount, SkillButton);
		SetOverlaySlot(SkillButton, FMargin(0.f), HAlign_Fill, VAlign_Fill);
		SkillButton->SetVisibility(ESlateVisibility::Visible);

		UOverlay* EndMount = PlaceActionPanel(TEXT("EndTurnPanel"),
			TEXT("EndTurnPlateMount"), FVector2D(-10.334961f, -14.354064f));
		UImage* EndPlate = CastChecked<UImage>(
			Blueprint->WidgetTree->FindWidget(TEXT("EndTurnPlate")));
		EnsureParent(EndMount, EndPlate);
		EndPlate->SetVisibility(ESlateVisibility::Visible);
		SetOverlaySlot(EndPlate, FMargin(0.f), HAlign_Fill, VAlign_Fill);

		UOverlay* EndLabelCenter = CastChecked<UOverlay>(
			Blueprint->WidgetTree->FindWidget(TEXT("EndTurnLabel_Center")));
		EnsureParent(EndMount, EndLabelCenter);
		EndLabelCenter->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		SetOverlaySlot(EndLabelCenter, FMargin(0.f), HAlign_Fill, VAlign_Fill);
		FitAuthoredActionButtonLabel(EndLabelCenter, ActionSize);
		UTextBlock* EndLabel = CastChecked<UTextBlock>(
			Blueprint->WidgetTree->FindWidget(TEXT("EndTurnLabel")));
		EnsureParent(EndLabelCenter, EndLabel);
		SetOverlaySlot(EndLabel, FMargin(0.f), HAlign_Center, VAlign_Center);
		FSlateFontInfo EndFont = UIFont::MakeProjectExact(EndLabel->GetFont(), 40);
		EndFont.LetterSpacing = 0;
		EndFont.OutlineSettings.OutlineSize = 2;
		EndFont.OutlineSettings.OutlineColor = FLinearColor::Black;
		EndLabel->SetFont(EndFont);
		EndLabel->SetText(NSLOCTEXT("CombatHUD", "EndTurnLabel", "턴 종료"));
		EndLabel->SetJustification(ETextJustify::Center);
		EndLabel->SetColorAndOpacity(FSlateColor(
			FLinearColor(.973f, .973f, .953f, 1.f)));
		EndLabel->SetShadowOffset(FVector2D(2.f, 3.f));
		EndLabel->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, .55f));
		EndLabel->SetAutoWrapText(false);
		EndLabel->SetMinDesiredWidth(0.f);
		EndLabel->SetVisibility(ESlateVisibility::HitTestInvisible);
		UButton* EndButton = CastChecked<UButton>(
			Blueprint->WidgetTree->FindWidget(TEXT("EndTurnButton")));
		EnsureParent(EndMount, EndButton);
		SetOverlaySlot(EndButton, FMargin(0.f), HAlign_Fill, VAlign_Fill);
		EndButton->SetVisibility(ESlateVisibility::Visible);
	}

	void BuildEnemySummary(UWidgetBlueprint* Blueprint, const FSlateFontInfo& BaseFont,
		UTexture2D* PanelTexture, UTexture2D* PortraitFrameTexture,
		UTexture2D* StatusFrameTexture, UTexture2D* SpeedTexture)
	{
		UCanvasPanel* Root = CastChecked<UCanvasPanel>(Blueprint->WidgetTree->RootWidget);
		UCanvasPanel* Panel = FindOrCreate<UCanvasPanel>(Blueprint, TEXT("EnemyPanel"));
		PlaceCanvas(Root, Panel, FVector2D(-630.f, 178.f), FVector2D(600.f, 430.f), 60);
		if (UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(Panel->Slot))
		{
			PanelSlot->SetAnchors(FAnchors(1.f, 0.f));
		}
		Panel->SetVisibility(ESlateVisibility::Collapsed);

		UImage* Plate = FindOrCreate<UImage>(Blueprint, TEXT("EnemyPlate"));
		PlaceCanvas(Panel, Plate, FVector2D::ZeroVector, FVector2D(600.f, 430.f), 0);
		Plate->SetBrushFromTexture(PanelTexture, false);
		Plate->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UImage* PortraitFrame = FindOrCreate<UImage>(Blueprint, TEXT("EnemyPortraitFrame"));
		PlaceCanvas(Panel, PortraitFrame, FVector2D(38.f, 34.f), FVector2D(126.f, 126.f), 5);
		PortraitFrame->SetBrushFromTexture(PortraitFrameTexture, false);
		PortraitFrame->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UImage* Portrait = FindOrCreate<UImage>(Blueprint, TEXT("EnemyPortrait"));
		PlaceCanvas(Panel, Portrait, FVector2D(51.f, 47.f), FVector2D(100.f, 100.f), 6);
		Portrait->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		Portrait->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UBorder* BadgePlate = FindOrCreate<UBorder>(Blueprint, TEXT("EnemyBadgePlate"));
		PlaceCanvas(Panel, BadgePlate, FVector2D(180.f, 45.f), FVector2D(62.f, 42.f), 6);
		BadgePlate->SetBrushColor(FLinearColor(.68f, .07f, .035f, .96f));
		BadgePlate->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UTextBlock* Badge = FindOrCreate<UTextBlock>(Blueprint, TEXT("EnemyBadgeText"));
		PlaceCanvas(Panel, Badge, FVector2D(180.f, 46.f), FVector2D(62.f, 40.f), 7);
		Badge->SetText(NSLOCTEXT("CombatHUD", "EnemyBadge", "적"));
		SetReadableFont(Badge, BaseFont, 24);

		UTextBlock* Name = FindOrCreate<UTextBlock>(Blueprint, TEXT("EnemyName"));
		PlaceCanvas(Panel, Name, FVector2D(254.f, 37.f), FVector2D(300.f, 62.f), 7);
		Name->SetText(NSLOCTEXT("CombatHUD", "EnemyNamePreview", "독수리"));
		SetReadableFont(Name, BaseFont, 38);
		Name->SetJustification(ETextJustify::Left);

		UBorder* HPBack = FindOrCreate<UBorder>(Blueprint, TEXT("EnemyHPBack"));
		PlaceCanvas(Panel, HPBack, FVector2D(178.f, 105.f), FVector2D(378.f, 58.f), 5);
		HPBack->SetBrushColor(FLinearColor(.08f, .025f, .018f, .96f));
		HPBack->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UProgressBar* HPBar = FindOrCreate<UProgressBar>(Blueprint, TEXT("EnemyHPBar"));
		PlaceCanvas(Panel, HPBar, FVector2D(188.f, 115.f), FVector2D(358.f, 38.f), 6);
		HPBar->SetPercent(1.f);
		HPBar->SetFillColorAndOpacity(FLinearColor(.88f, .055f, .025f, 1.f));

		UTextBlock* HPText = FindOrCreate<UTextBlock>(Blueprint, TEXT("EnemyHPText"));
		PlaceCanvas(Panel, HPText, FVector2D(188.f, 112.f), FVector2D(358.f, 44.f), 7);
		HPText->SetText(NSLOCTEXT("CombatHUD", "EnemyHPPreview", "HP  64 / 64"));
		SetReadableFont(HPText, BaseFont, 27);

		UBorder* APPlate = FindOrCreate<UBorder>(Blueprint, TEXT("EnemyAPPlate"));
		PlaceCanvas(Panel, APPlate, FVector2D(54.f, 184.f), FVector2D(230.f, 58.f), 5);
		APPlate->SetBrushColor(FLinearColor(.025f, .17f, .27f, .97f));
		APPlate->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UTextBlock* APText = FindOrCreate<UTextBlock>(Blueprint, TEXT("EnemyAPText"));
		PlaceCanvas(Panel, APText, FVector2D(54.f, 188.f), FVector2D(230.f, 50.f), 6);
		APText->SetText(NSLOCTEXT("CombatHUD", "EnemyAPPreview", "AP  5 / 5"));
		SetReadableFont(APText, BaseFont, 29);

		UBorder* SpeedPlate = FindOrCreate<UBorder>(Blueprint, TEXT("EnemySpeedPlate"));
		PlaceCanvas(Panel, SpeedPlate, FVector2D(316.f, 184.f), FVector2D(230.f, 58.f), 5);
		SpeedPlate->SetBrushColor(FLinearColor(.21f, .105f, .035f, .97f));
		SpeedPlate->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UImage* SpeedIcon = FindOrCreate<UImage>(Blueprint, TEXT("EnemySpeedIcon"));
		PlaceCanvas(Panel, SpeedIcon, FVector2D(330.f, 191.f), FVector2D(44.f, 44.f), 6);
		SpeedIcon->SetBrushFromTexture(SpeedTexture, false);
		SpeedIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UTextBlock* SpeedText = FindOrCreate<UTextBlock>(Blueprint, TEXT("EnemySpeedText"));
		PlaceCanvas(Panel, SpeedText, FVector2D(376.f, 188.f), FVector2D(158.f, 50.f), 6);
		SpeedText->SetText(NSLOCTEXT("CombatHUD", "EnemySpeedPreview", "속도  4"));
		SetReadableFont(SpeedText, BaseFont, 29);

		UTextBlock* StatusLabel = FindOrCreate<UTextBlock>(Blueprint, TEXT("EnemyStatusLabel"));
		PlaceCanvas(Panel, StatusLabel, FVector2D(52.f, 267.f), FVector2D(94.f, 42.f), 6);
		StatusLabel->SetText(NSLOCTEXT("CombatHUD", "EnemyStatusLabel", "상태"));
		SetReadableFont(StatusLabel, BaseFont, 26);

		for (int32 Index = 0; Index < 3; ++Index)
		{
			const float X = 150.f + 92.f * Index;
			UImage* Frame = FindOrCreate<UImage>(Blueprint,
				FName(*FString::Printf(TEXT("EnemyStatusFrame_%d"), Index)));
			PlaceCanvas(Panel, Frame, FVector2D(X, 252.f), FVector2D(76.f, 76.f), 6);
			Frame->SetBrushFromTexture(StatusFrameTexture, false);
			Frame->SetVisibility(ESlateVisibility::Collapsed);

			UImage* Icon = FindOrCreate<UImage>(Blueprint,
				FName(*FString::Printf(TEXT("EnemyStatusIcon_%d"), Index)));
			PlaceCanvas(Panel, Icon, FVector2D(X + 8.f, 260.f), FVector2D(60.f, 60.f), 7);
			Icon->SetVisibility(ESlateVisibility::Collapsed);

			UTextBlock* Count = FindOrCreate<UTextBlock>(Blueprint,
				FName(*FString::Printf(TEXT("EnemyStatusCount_%d"), Index)));
			PlaceCanvas(Panel, Count, FVector2D(X + 43.f, 293.f), FVector2D(30.f, 30.f), 8);
			Count->SetText(FText::AsNumber(2));
			SetReadableFont(Count, BaseFont, 20);
			Count->SetVisibility(ESlateVisibility::Collapsed);
		}

		UTextBlock* StatusText = FindOrCreate<UTextBlock>(Blueprint, TEXT("EnemyStatus"));
		PlaceCanvas(Panel, StatusText, FVector2D(438.f, 263.f), FVector2D(118.f, 52.f), 7);
		StatusText->SetText(NSLOCTEXT("CombatHUD", "EnemyNoStatus", "상태 없음"));
		SetReadableFont(StatusText, BaseFont, 22);

		UTextBlock* Forecast = FindOrCreate<UTextBlock>(Blueprint, TEXT("EnemyForecast"));
		PlaceCanvas(Panel, Forecast, FVector2D(64.f, 356.f), FVector2D(472.f, 46.f), 7);
		Forecast->SetText(NSLOCTEXT("CombatHUD", "EnemyForecastPreview", "예상 피해  8~14"));
		SetReadableFont(Forecast, BaseFont, 27);

		if (UWidget* LegacyDefense = Blueprint->WidgetTree->FindWidget(TEXT("EnemyDefense")))
		{
			LegacyDefense->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	void BuildAllySummary(UWidgetBlueprint* Blueprint, const FSlateFontInfo& BaseFont,
		UTexture2D* PanelTexture, UTexture2D* PortraitFrameTexture,
		UTexture2D* StatusFrameTexture, UTexture2D* SpeedTexture)
	{
		UCanvasPanel* Root = CastChecked<UCanvasPanel>(Blueprint->WidgetTree->RootWidget);
		UCanvasPanel* Panel = FindOrCreate<UCanvasPanel>(Blueprint, TEXT("AllyPanel"));
		PlaceCanvas(Root, Panel, FVector2D(30.f, 178.f), FVector2D(600.f, 430.f), 60);
		Panel->SetVisibility(ESlateVisibility::Collapsed);

		UImage* Plate = FindOrCreate<UImage>(Blueprint, TEXT("AllyPlate"));
		PlaceCanvas(Panel, Plate, FVector2D::ZeroVector, FVector2D(600.f, 430.f), 0);
		Plate->SetBrushFromTexture(PanelTexture, false);
		Plate->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UImage* PortraitFrame = FindOrCreate<UImage>(Blueprint, TEXT("AllyPortraitFrame"));
		PlaceCanvas(Panel, PortraitFrame, FVector2D(38.f, 34.f), FVector2D(126.f, 126.f), 5);
		PortraitFrame->SetBrushFromTexture(PortraitFrameTexture, false);
		PortraitFrame->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UImage* Portrait = FindOrCreate<UImage>(Blueprint, TEXT("AllyPortrait"));
		PlaceCanvas(Panel, Portrait, FVector2D(51.f, 47.f), FVector2D(100.f, 100.f), 6);
		Portrait->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		Portrait->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UBorder* BadgePlate = FindOrCreate<UBorder>(Blueprint, TEXT("AllyBadgePlate"));
		PlaceCanvas(Panel, BadgePlate, FVector2D(180.f, 45.f), FVector2D(78.f, 42.f), 6);
		BadgePlate->SetBrushColor(FLinearColor(.025f, .35f, .58f, .96f));
		BadgePlate->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UTextBlock* Badge = FindOrCreate<UTextBlock>(Blueprint, TEXT("AllyBadgeText"));
		PlaceCanvas(Panel, Badge, FVector2D(180.f, 46.f), FVector2D(78.f, 40.f), 7);
		Badge->SetText(NSLOCTEXT("CombatHUD", "AllyBadge", "아군"));
		SetReadableFont(Badge, BaseFont, 22);

		UTextBlock* Name = FindOrCreate<UTextBlock>(Blueprint, TEXT("AllyName"));
		PlaceCanvas(Panel, Name, FVector2D(274.f, 37.f), FVector2D(280.f, 62.f), 7);
		Name->SetText(NSLOCTEXT("CombatHUD", "AllyNamePreview", "기사"));
		SetReadableFont(Name, BaseFont, 38);
		Name->SetJustification(ETextJustify::Left);

		UBorder* HPBack = FindOrCreate<UBorder>(Blueprint, TEXT("AllyHPBack"));
		PlaceCanvas(Panel, HPBack, FVector2D(178.f, 105.f), FVector2D(378.f, 58.f), 5);
		HPBack->SetBrushColor(FLinearColor(.035f, .09f, .035f, .96f));
		HPBack->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UProgressBar* HPBar = FindOrCreate<UProgressBar>(Blueprint, TEXT("AllyHPBar"));
		PlaceCanvas(Panel, HPBar, FVector2D(188.f, 115.f), FVector2D(358.f, 38.f), 6);
		HPBar->SetPercent(1.f);
		HPBar->SetFillColorAndOpacity(FLinearColor(.18f, .67f, .22f, 1.f));

		UTextBlock* HPText = FindOrCreate<UTextBlock>(Blueprint, TEXT("AllyHPText"));
		PlaceCanvas(Panel, HPText, FVector2D(188.f, 112.f), FVector2D(358.f, 44.f), 7);
		HPText->SetText(NSLOCTEXT("CombatHUD", "AllyHPPreview", "HP  100 / 100"));
		SetReadableFont(HPText, BaseFont, 27);

		UBorder* APPlate = FindOrCreate<UBorder>(Blueprint, TEXT("AllyAPPlate"));
		PlaceCanvas(Panel, APPlate, FVector2D(54.f, 184.f), FVector2D(230.f, 58.f), 5);
		APPlate->SetBrushColor(FLinearColor(.025f, .17f, .27f, .97f));
		APPlate->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UTextBlock* APText = FindOrCreate<UTextBlock>(Blueprint, TEXT("AllyAPText"));
		PlaceCanvas(Panel, APText, FVector2D(54.f, 188.f), FVector2D(230.f, 50.f), 6);
		APText->SetText(NSLOCTEXT("CombatHUD", "AllyAPPreview", "AP  10 / 10"));
		SetReadableFont(APText, BaseFont, 29);

		UBorder* SpeedPlate = FindOrCreate<UBorder>(Blueprint, TEXT("AllySpeedPlate"));
		PlaceCanvas(Panel, SpeedPlate, FVector2D(316.f, 184.f), FVector2D(230.f, 58.f), 5);
		SpeedPlate->SetBrushColor(FLinearColor(.21f, .105f, .035f, .97f));
		SpeedPlate->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UImage* SpeedIcon = FindOrCreate<UImage>(Blueprint, TEXT("AllySpeedIcon"));
		PlaceCanvas(Panel, SpeedIcon, FVector2D(330.f, 191.f), FVector2D(44.f, 44.f), 6);
		SpeedIcon->SetBrushFromTexture(SpeedTexture, false);
		SpeedIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UTextBlock* SpeedText = FindOrCreate<UTextBlock>(Blueprint, TEXT("AllySpeedText"));
		PlaceCanvas(Panel, SpeedText, FVector2D(376.f, 188.f), FVector2D(158.f, 50.f), 6);
		SpeedText->SetText(NSLOCTEXT("CombatHUD", "AllySpeedPreview", "속도  5"));
		SetReadableFont(SpeedText, BaseFont, 29);

		UTextBlock* StatusLabel = FindOrCreate<UTextBlock>(Blueprint, TEXT("AllyStatusLabel"));
		PlaceCanvas(Panel, StatusLabel, FVector2D(52.f, 267.f), FVector2D(94.f, 42.f), 6);
		StatusLabel->SetText(NSLOCTEXT("CombatHUD", "AllyStatusLabel", "상태"));
		SetReadableFont(StatusLabel, BaseFont, 26);

		for (int32 Index = 0; Index < 3; ++Index)
		{
			const float X = 150.f + 92.f * Index;
			UImage* Frame = FindOrCreate<UImage>(Blueprint,
				FName(*FString::Printf(TEXT("AllyStatusFrame_%d"), Index)));
			PlaceCanvas(Panel, Frame, FVector2D(X, 252.f), FVector2D(76.f, 76.f), 6);
			Frame->SetBrushFromTexture(StatusFrameTexture, false);
			Frame->SetVisibility(ESlateVisibility::Collapsed);

			UImage* Icon = FindOrCreate<UImage>(Blueprint,
				FName(*FString::Printf(TEXT("AllyStatusIcon_%d"), Index)));
			PlaceCanvas(Panel, Icon, FVector2D(X + 8.f, 260.f), FVector2D(60.f, 60.f), 7);
			Icon->SetVisibility(ESlateVisibility::Collapsed);

			UTextBlock* Count = FindOrCreate<UTextBlock>(Blueprint,
				FName(*FString::Printf(TEXT("AllyStatusCount_%d"), Index)));
			PlaceCanvas(Panel, Count, FVector2D(X + 43.f, 293.f), FVector2D(30.f, 30.f), 8);
			Count->SetText(FText::AsNumber(2));
			SetReadableFont(Count, BaseFont, 20);
			Count->SetVisibility(ESlateVisibility::Collapsed);
		}

		UTextBlock* StatusText = FindOrCreate<UTextBlock>(Blueprint, TEXT("AllyStatus"));
		PlaceCanvas(Panel, StatusText, FVector2D(438.f, 263.f), FVector2D(118.f, 52.f), 7);
		StatusText->SetText(NSLOCTEXT("CombatHUD", "AllyNoStatus", "상태 없음"));
		SetReadableFont(StatusText, BaseFont, 22);

		UTextBlock* Hint = FindOrCreate<UTextBlock>(Blueprint, TEXT("AllySummaryHint"));
		PlaceCanvas(Panel, Hint, FVector2D(64.f, 356.f), FVector2D(472.f, 46.f), 7);
		Hint->SetText(NSLOCTEXT("CombatHUD", "AllySummaryHint", "선택한 용병 요약"));
		SetReadableFont(Hint, BaseFont, 25);
	}

	/**
	 * @brief 보유 용병 패널. **자기 판(WBP_MercenaryPanel)에 짓는다.**
	 *
	 * @details
	 * 전에는 전투 HUD 판 안에 있었다. 화면 전체를 덮는 것이 셋(용병 패널 ·
	 * 커맨드 겹 · 파티 겹) 포개져 있어서, UMG 디자이너에서는 늘 맨 위 것만
	 * 잡히고 아래 것은 계층 목록으로만 고칠 수 있었다. 용병 판 하나만 51개
	 * 위젯이라 그 안을 손보려면 사실상 마우스를 못 썼다.
	 *
	 * 배선은 안 끊긴다 -- ``CombatLayoutHUDWidget`` 의 조회는 ``FindDeep`` 이라
	 * 자식 UserWidget 의 트리까지 훑는다. 다만 **비재귀** 조회를 쓰는 곳은
	 * 같이 고쳐야 한다(CombatLayoutCaptureTests).
	 */
	void BuildMercenaryInventoryTab(UWidgetBlueprint* Blueprint, UCanvasPanel* Board,
		const FSlateFontInfo& BaseFont, UTexture2D* InventoryIconTexture,
		UTexture2D* GoldIconTexture, UTexture2D* DescriptionPlateTexture,
		UTexture2D* ArtifactSlotTexture)
	{
		/*
		 * 3인 로스터 바로 아래의 파티 공용 인벤토리 탭.
		 *
		 * 런타임에서 ConstructWidget으로 덧붙이지 않는다. 판·아이콘·문구·입력면을
		 * 전부 WBP에 구워 두어 디자이너에서 위치와 크기, 그림을 직접 바꿀 수 있게
		 * 한다. C++은 이름으로 버튼을 찾아 의도만 연결한다.
		 */
		UCanvasPanel* Roster = Cast<UCanvasPanel>(
			Blueprint->WidgetTree->FindWidget(TEXT("MercRosterSection")));
		UWidget* SecondCard = Blueprint->WidgetTree->FindWidget(
			TEXT("MercenaryCardScale_1"));
		UWidget* ThirdCard = Blueprint->WidgetTree->FindWidget(
			TEXT("MercenaryCardScale_2"));
		UImage* ThirdPlate = Cast<UImage>(Blueprint->WidgetTree->FindWidget(
			TEXT("PartyPlate_2")));
		UCanvasPanelSlot* SecondSlot = SecondCard != nullptr
			? Cast<UCanvasPanelSlot>(SecondCard->Slot) : nullptr;
		UCanvasPanelSlot* ThirdSlot = ThirdCard != nullptr
			? Cast<UCanvasPanelSlot>(ThirdCard->Slot) : nullptr;
		UCanvasPanelSlot* ThirdPlateSlot = ThirdPlate != nullptr
			? Cast<UCanvasPanelSlot>(ThirdPlate->Slot) : nullptr;
		if (Roster == nullptr || SecondSlot == nullptr || ThirdSlot == nullptr
			|| ThirdPlateSlot == nullptr)
		{
			// 통합 HUD에는 카드가 nested WBP 인스턴스로 들어가 슬롯을 직접
			// 고칠 수 없지만, 인벤토리 판/버튼의 직렬화된 프록시는 남아 있다.
			// 그 경우에도 실제 화면에서 보이는 크기 차이는 이 프록시에서 바로
			// 보정한다. 독립 WBP는 아래 정상 경로에서 전체를 재구성한다.
			UImage* ExistingInventoryPlate = Cast<UImage>(
				Blueprint->WidgetTree->FindWidget(TEXT("MercenaryInventoryTabPlate")));
			UButton* ExistingInventoryButton = Cast<UButton>(
				Blueprint->WidgetTree->FindWidget(TEXT("MercenaryInventoryButton")));
			UImage* ExistingPartyPlate = Cast<UImage>(
				Blueprint->WidgetTree->FindWidget(TEXT("PartyPlate_2")));
			UCanvasPanelSlot* ExistingPlateSlot = ExistingInventoryPlate != nullptr
				? Cast<UCanvasPanelSlot>(ExistingInventoryPlate->Slot) : nullptr;
			UCanvasPanelSlot* ExistingButtonSlot = ExistingInventoryButton != nullptr
				? Cast<UCanvasPanelSlot>(ExistingInventoryButton->Slot) : nullptr;
			UCanvasPanelSlot* ExistingPartySlot = ExistingPartyPlate != nullptr
				? Cast<UCanvasPanelSlot>(ExistingPartyPlate->Slot) : nullptr;
			if (ExistingPlateSlot != nullptr && ExistingButtonSlot != nullptr)
			{
				const FVector2D DonorSize = ExistingPartySlot != nullptr
					? ExistingPartySlot->GetSize() : FVector2D(350.f, 128.f);
				const FVector2D VisibleSize(DonorSize.X * 1.075f, DonorSize.Y * .95f);
				const FVector2D VisiblePosition(0.f, (DonorSize.Y - VisibleSize.Y) * .5f);
				ExistingPlateSlot->SetPosition(VisiblePosition);
				ExistingPlateSlot->SetSize(VisibleSize);
				ExistingButtonSlot->SetPosition(VisiblePosition);
				ExistingButtonSlot->SetSize(VisibleSize);
				if (UPanelWidget* Parent = ExistingInventoryPlate->GetParent())
				{
					Parent->SetClipping(EWidgetClipping::Inherit);
				}
				UE_LOG(LogTemp, Display,
					TEXT("RD_COMBAT_HUD_INVENTORY_BUILD repaired nested proxy size=%s"),
					*VisibleSize.ToString());
			}
			else
			{
				UE_LOG(LogTemp, Error,
					TEXT("RD_COMBAT_HUD_INVENTORY_BUILD missing roster/proxy slots"));
			}
			return;
		}
		// 인벤토리 전용 재빌드도 세 로스터 행의 좌상단 앵커 계약을 복구한다.
		// 현재 자산은 Roster 자체가 Board (210,246)에 놓이고 카드는 그 안의
		// 로컬 좌표를 쓴다. 옛 자산처럼 Roster가 (0,0)인 경우도 있으므로
		// Board 절대 좌표에서 Roster 원점을 빼 두 구조를 모두 같은 화면에 둔다.
		UCanvasPanelSlot* RosterSlot = Cast<UCanvasPanelSlot>(Roster->Slot);
		if (RosterSlot != nullptr)
		{
			RosterSlot->SetPosition(FVector2D(240.f, 0.f));
			RosterSlot->SetSize(FVector2D(1680.f, 1080.f));
		}
		const FVector2D RosterOrigin = RosterSlot != nullptr
			? RosterSlot->GetPosition() : FVector2D::ZeroVector;
		const FVector2D AuthoredBoardCardPositions[] = {
			FVector2D(490.f, 280.f), FVector2D(490.f, 416.f), FVector2D(490.f, 552.f)
		};
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(AuthoredBoardCardPositions); ++Index)
		{
			if (UWidget* RosterCard = Blueprint->WidgetTree->FindWidget(
				FName(*FString::Printf(TEXT("MercenaryCardScale_%d"), Index))))
			{
				if (UScaleBox* RosterScale = Cast<UScaleBox>(RosterCard))
				{
					// 세 카드와 네 번째 탭이 같은 외곽 슬롯을 100% 채우게 한다.
					// ScaleToFit은 350:128과 390:142의 0.5% 비율차 때문에 좌우에
					// 서로 다른 빈 폭을 남겨 실기에서 버튼 크기가 달라 보였다.
					RosterScale->SetStretch(EStretch::Fill);
				}
				const UCanvasPanelSlot* ExistingSlot =
					Cast<UCanvasPanelSlot>(RosterCard->Slot);
				// Donor WBP의 실제 저장 크기를 보존한다. 화면에서 보이는 용병
				// 카드가 계약값과 다르면 상수로 덮는 순간 네 번째 탭과 어긋난다.
				const FVector2D Size = ExistingSlot != nullptr
					? ExistingSlot->GetSize() : FVector2D(390.f, 142.f);
				const int32 ZOrder = ExistingSlot != nullptr
					? ExistingSlot->GetZOrder() : 3;
				PlaceCanvas(Roster, RosterCard,
					AuthoredBoardCardPositions[Index] - RosterOrigin, Size, ZOrder);
			}
		}
		const FVector2D RowStep = ThirdSlot->GetPosition() - SecondSlot->GetPosition();
		const FVector2D TabPosition = ThirdSlot->GetPosition() + RowStep;
		// 수치 계약이 아니라 바로 위 세 번째 용병 카드와 내부 판을 donor로 쓴다.
		// 따라서 디자이너가 용병 카드 크기를 바꿔도 인벤토리는 같은 프레임에서
		// 같은 픽셀 크기를 복제한다.
		const FVector2D TabSize = ThirdSlot->GetSize();
		const FVector2D LocalTabSize = ThirdPlateSlot->GetSize();
		UE_LOG(LogTemp, Display,
			TEXT("RD_COMBAT_HUD_INVENTORY_DONOR outer=%s plate=%s third=%s"),
			*TabSize.ToString(), *LocalTabSize.ToString(), *ThirdCard->GetName());

		// 세 용병 카드와 똑같이 ScaleBox -> 로컬 카드 Canvas 구조를 쓴다.
		// 직접 작은 Canvas를 붙이면 같은 에셋이어도 네 번째만 작게 보인다.
		UScaleBox* InventoryScale = FindOrCreate<UScaleBox>(
			Blueprint, TEXT("MercenaryInventoryScale"));
		PlaceCanvas(Roster, InventoryScale, TabPosition, TabSize, 4);
		if (const UScaleBox* ThirdScale = Cast<UScaleBox>(ThirdCard))
		{
			InventoryScale->SetStretch(ThirdScale->GetStretch());
			InventoryScale->SetStretchDirection(ThirdScale->GetStretchDirection());
			InventoryScale->SetUserSpecifiedScale(ThirdScale->GetUserSpecifiedScale());
			InventoryScale->SetIgnoreInheritedScale(
				ThirdScale->IsIgnoreInheritedScale());
		}
		InventoryScale->SetStretch(EStretch::Fill);
		if (UCanvasPanelSlot* InventoryScaleSlot =
			Cast<UCanvasPanelSlot>(InventoryScale->Slot))
		{
			InventoryScaleSlot->SetAnchors(ThirdSlot->GetAnchors());
			InventoryScaleSlot->SetAlignment(ThirdSlot->GetAlignment());
		}
		UCanvasPanel* InventoryTab = FindOrCreate<UCanvasPanel>(
			Blueprint, TEXT("MercenaryInventoryTab"));
		EnsureParent(InventoryScale, InventoryTab);
		// 보정된 donor 외곽선이 Canvas의 350px desired width에서 잘리지 않게 한다.
		// 실제 입력면은 아래 VisibleTabSize로 명시되어 로스터 밖을 받지 않는다.
		InventoryTab->SetClipping(EWidgetClipping::Inherit);

		UImage* InventoryPlate = FindOrCreate<UImage>(
			Blueprint, TEXT("MercenaryInventoryTabPlate"));
		// donor 카드에는 레벨 배지/HP 자식들이 Canvas의 desired geometry를
		// 가로로 넓혀 주지만 인벤토리에는 그 자식이 없다. 같은 350x128 슬롯을
		// 복사해도 실제 Slate 결과가 6% 좁고 8% 높게 나왔다. 실물 캡처에서 잰
		// donor의 보이는 프레임 비율로 보정해 외곽선을 픽셀 단위로 맞춘다.
		const FVector2D VisibleTabSize(LocalTabSize.X * 1.075f, LocalTabSize.Y * .95f);
		const FVector2D VisibleTabPosition(0.f, (LocalTabSize.Y - VisibleTabSize.Y) * .5f);
		PlaceCanvas(InventoryTab, InventoryPlate, VisibleTabPosition,
			VisibleTabSize, 1);
		InventoryPlate->SetBrush(ThirdPlate->GetBrush());
		InventoryPlate->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UImage* InventoryIcon = FindOrCreate<UImage>(
			Blueprint, TEXT("MercenaryInventoryTabIcon"));
		// 네 번째 줄도 용병 카드의 초상화 칸을 그대로 쓴다. 별도 수치로
		// 줄이면 같은 프레임인데도 인벤토리만 다른 규격처럼 보인다.
		const float CardScaleX = LocalTabSize.X / 350.f;
		const float CardScaleY = LocalTabSize.Y / 128.f;
		const float IconSize = FMath::Min(96.f * CardScaleX, 96.f * CardScaleY);
		PlaceCanvas(InventoryTab, InventoryIcon,
			FVector2D(18.f * CardScaleX, 16.f * CardScaleY),
			FVector2D(IconSize, IconSize), 2);
		InventoryIcon->SetBrushFromTexture(InventoryIconTexture, false);
		InventoryIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UTextBlock* InventoryLabel = FindOrCreate<UTextBlock>(
			Blueprint, TEXT("MercenaryInventoryTabText"));
		PlaceCanvas(InventoryTab, InventoryLabel,
			FVector2D(128.f * CardScaleX, 21.f * CardScaleY),
			FVector2D(190.f * CardScaleX, 46.f * CardScaleY), 2);
		InventoryLabel->SetText(NSLOCTEXT(
			"CombatHUD", "MercenaryInventoryTab", "인벤토리"));
		SetReadableFont(InventoryLabel, BaseFont, 27);

		UButton* InventoryButton = FindOrCreate<UButton>(
			Blueprint, TEXT("MercenaryInventoryButton"));
		PlaceCanvas(InventoryTab, InventoryButton, VisibleTabPosition,
			VisibleTabSize, 10);
		SetInvisibleButtonChrome(InventoryButton);

		// 오른쪽 용병 상세 자리를 그대로 쓰는 인벤토리 페이지. 왼쪽 네 줄은
		// 유지되어 언제든 용병 상세로 돌아갈 수 있다.
		// MercDetailSection은 상세 부품을 묶은 전면 Canvas라 슬롯 자체는
		// 1920x1080이다. 그 크기를 복사하면 인벤토리가 새 화면처럼 왼쪽 탭까지
		// 덮는다. 같은 용병판의 오른쪽 내용 영역만 쓰도록 명시한다.
		// 1672x941 실물 캡처에서 외곽 셸의 검은 내용면을 재서 잡은 영역이다.
		// 헤더 금속 테두리와 아래 닫기 레일을 피하고 오른쪽 내용면 안에만 둔다.
		const FVector2D PagePosition(560.f, 194.f);
		// 실기 캡처에서 600 높이는 4x2 격자를 상단에 몰아 하단 검은 면이
		// 과하게 남았다. 셸의 실제 내부 하단까지 사용해 두 줄을 세로 중앙에 둔다.
		const FVector2D PageSize(1240.f, 610.f);
		UCanvasPanel* Page = FindOrCreate<UCanvasPanel>(
			Blueprint, TEXT("MercenaryInventoryPage"));
		PlaceCanvas(Board, Page, PagePosition, PageSize, 30);
		Page->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		Page->SetVisibility(ESlateVisibility::Collapsed);

		UBorder* Backdrop = FindOrCreate<UBorder>(
			Blueprint, TEXT("MercenaryInventoryPageBackdrop"));
		PlaceCanvas(Page, Backdrop, FVector2D::ZeroVector, PageSize, 0);
		// 상세 묶음 자체를 접으므로 별도 단색 덮개가 필요 없다. 덮개 색이 셸과
		// 조금만 달라도 새 팝업처럼 네모 경계가 생긴다. 위젯은 WBP 편집용으로
		// 남기되 기본값만 접어 기존 용병 셸의 오른쪽 면을 그대로 쓴다.
		Backdrop->SetBrushColor(FLinearColor::Transparent);
		Backdrop->SetVisibility(ESlateVisibility::Collapsed);

		UTextBlock* Title = FindOrCreate<UTextBlock>(
			Blueprint, TEXT("MercenaryInventoryTitle"));
		PlaceCanvas(Page, Title, FVector2D(250.f, 10.f),
			FVector2D(710.f, 72.f), 2);
		Title->SetText(NSLOCTEXT(
			"CombatHUD", "MercenaryInventoryTitle", "보유 아티팩트"));
		SetReadableFont(Title, BaseFont, 38);
		Title->SetJustification(ETextJustify::Center);
		Title->SetVisibility(ESlateVisibility::Collapsed);

		UTextBlock* GoldLabel = FindOrCreate<UTextBlock>(
			Blueprint, TEXT("MercenaryInventoryGoldLabel"));
		PlaceCanvas(Page, GoldLabel, FVector2D(980.f, 22.f), FVector2D(80.f, 54.f), 2);
		GoldLabel->SetText(NSLOCTEXT("CombatHUD", "MercenaryInventoryGold", "골드"));
		SetReadableFont(GoldLabel, BaseFont, 28);
		// 시안은 숫자 옆 실제 골드 아이콘만 쓴다. 라벨은 WBP에서 다시 켤 수
		// 있도록 삭제하지 않고 기본값만 접어 둔다.
		GoldLabel->SetVisibility(ESlateVisibility::Collapsed);
		UImage* GoldIcon = FindOrCreate<UImage>(
			Blueprint, TEXT("MercenaryInventoryGoldIcon"));
		// 슬롯 그림의 실물 내부는 전체 텍스처의 약 69%뿐이다
		// (UIPartRects::Inner 계약). 페이지 폭에 끝점만 맞춰 균등분배하면 투명
		// 외곽까지 간격으로 계산되어, 화면에서는 작은 칸들이 멀리 흩어져 보인다.
		// 그래서 프레임 자체를 키우고 그림의 가시 경계끼리 약 30px만 남는 고정
		// 피치를 쓴다. 4x2 전체가 오른쪽 검은 내용면 중앙에 한 덩어리로 놓인다.
		constexpr float FrameSize = 210.f;
		constexpr float ColumnGap = 24.f;
		constexpr float RowGap = 30.f;
		constexpr float ColumnPitch = FrameSize + ColumnGap;
		constexpr float RowPitch = FrameSize + RowGap;
		constexpr float GridWidth = FrameSize * 4.f + ColumnGap * 3.f;
		constexpr float GridHeight = FrameSize * 2.f + RowGap;
		// 왼쪽의 용병/인벤토리 탭 묶음은 오른쪽으로 옮겼지만, 오른쪽 내용면의
		// 슬롯은 반대로 왼쪽으로 당겨 프레임 안에서 시각 중심을 맞춘다.
		constexpr float GridShiftRight = -154.f;
		const FVector2D GridOrigin(
			(PageSize.X - GridWidth) * .5f + GridShiftRight,
			(PageSize.Y - GridHeight) * .5f);
		UImage* GoldFrame = FindOrCreate<UImage>(
			Blueprint, TEXT("MercenaryInventoryGoldFrame"));
		PlaceCanvas(Page, GoldFrame, GridOrigin, FVector2D(FrameSize, FrameSize), 2);
		GoldFrame->SetBrushFromTexture(ArtifactSlotTexture, false);
		GoldFrame->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		const FBox2D GoldInner = UIPartRects::Inner(TEXT("T_MB_ArtifactSlot_Frame"),
			GridOrigin, FVector2D(FrameSize, FrameSize), false);
		const FVector2D GoldContentSize = GoldInner.GetSize();
		const float GoldIconSize = FMath::Min(GoldContentSize.X,
			GoldContentSize.Y * .70f) * .76f;
		PlaceCanvas(Page, GoldIcon,
			FVector2D(GoldInner.GetCenter().X - GoldIconSize * .5f,
				GoldInner.Min.Y + GoldContentSize.Y * .03f),
			FVector2D(GoldIconSize, GoldIconSize), 3);
		GoldIcon->SetBrushFromTexture(GoldIconTexture, false);
		GoldIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UTextBlock* GoldText = FindOrCreate<UTextBlock>(
			Blueprint, TEXT("MercenaryInventoryGoldText"));
		// TextBlock의 글꼴 베이스라인이 슬롯 아래로 내려가므로 숫자 칸을
		// 프레임의 하단 경계에서 충분히 올린다. 실제 2176x1812 캡처에서
		// "100"이 프레임 밖으로 빠진 원인이었다.
		PlaceCanvas(Page, GoldText,
			FVector2D(GoldInner.Min.X, GoldInner.Min.Y + GoldContentSize.Y * .56f),
			FVector2D(GoldContentSize.X, GoldContentSize.Y * .28f), 4);
		GoldText->SetText(FText::AsNumber(100));
		SetReadableFont(GoldText, BaseFont, 24);
		GoldText->SetJustification(ETextJustify::Center);
		static const TCHAR* const PreviewArtifactPaths[6] = {
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Artifacts/T_Artifact_BloodChalice.T_Artifact_BloodChalice"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Artifacts/T_Artifact_FangAmulet.T_Artifact_FangAmulet"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Artifacts/T_Artifact_LuckyCoin.T_Artifact_LuckyCoin"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Artifacts/T_Artifact_ThornCrest.T_Artifact_ThornCrest"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Artifacts/T_Artifact_TravelersMap.T_Artifact_TravelersMap"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Artifacts/T_Artifact_WornShieldOrnament.T_Artifact_WornShieldOrnament") };
		static const TCHAR* const PreviewArtifactNames[6] = {
			TEXT("피의 성배"), TEXT("야수의 송곳니"), TEXT("행운의 주화"),
			TEXT("가시 문장"), TEXT("여행자의 지도"), TEXT("낡은 방패 장식") };
		for (int32 Index = 0; Index < 7; ++Index)
		{
			const int32 VisualIndex = Index + 1;
			const int32 Column = VisualIndex % 4;
			const int32 Row = VisualIndex / 4;
			const FVector2D CellOrigin = GridOrigin + FVector2D(
				Column * ColumnPitch, Row * RowPitch);

			// 조회 전용 격자라 선택 테두리를 두지 않는다. 터치하면 별도 상세가 뜬다.
			if (UWidget* Selection = Blueprint->WidgetTree->FindWidget(
				FName(*FString::Printf(
					TEXT("MercenaryInventoryArtifactSelection_%d"), Index))))
			{
				Blueprint->WidgetTree->RemoveWidget(Selection);
			}

			UImage* Frame = FindOrCreate<UImage>(Blueprint,
				FName(*FString::Printf(TEXT("MercenaryInventoryArtifactFrame_%d"), Index)));
			PlaceCanvas(Page, Frame, CellOrigin, FVector2D(FrameSize, FrameSize), 2);
			Frame->SetBrushFromTexture(ArtifactSlotTexture, false);
			Frame->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			UImage* Icon = FindOrCreate<UImage>(Blueprint,
				FName(*FString::Printf(TEXT("MercenaryInventoryArtifactIcon_%d"), Index)));
			const FBox2D CellInner = UIPartRects::Inner(
				TEXT("T_MB_ArtifactSlot_Frame"), CellOrigin,
				FVector2D(FrameSize, FrameSize), false);
			const float ArtifactIconSize = FMath::Min(
				CellInner.GetSize().X, CellInner.GetSize().Y) * .82f;
			PlaceCanvas(Page, Icon,
				CellInner.GetCenter() - FVector2D(ArtifactIconSize * .5f),
				FVector2D(ArtifactIconSize, ArtifactIconSize), 3);
			if (Index < 6)
			{
				if (UTexture2D* PreviewArtifact = LoadObject<UTexture2D>(
					nullptr, PreviewArtifactPaths[Index]))
				{
					Icon->SetBrushFromTexture(PreviewArtifact, false);
				}
			}
			Icon->SetVisibility(Index < 6
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed);

			UTextBlock* Name = FindOrCreate<UTextBlock>(Blueprint,
				FName(*FString::Printf(TEXT("MercenaryInventoryArtifactName_%d"), Index)));
			PlaceCanvas(Page, Name, CellOrigin + FVector2D(-24.f, FrameSize + 3.f),
				FVector2D(FrameSize + 48.f, 34.f), 4);
			Name->SetText(Index < 6
				? FText::FromString(PreviewArtifactNames[Index])
				: FText::GetEmpty());
			SetReadableFont(Name, BaseFont, 17);
			Name->SetJustification(ETextJustify::Center);
			// 슬롯 이름은 디자이너가 필요할 때 WBP에서 켠다. 런타임은 텍스트만
			// 갱신하고 Visibility를 덮어쓰지 않는다.
			Name->SetVisibility(ESlateVisibility::Collapsed);

			UButton* Button = FindOrCreate<UButton>(Blueprint,
				FName(*FString::Printf(TEXT("MercenaryInventoryArtifactButton_%d"), Index)));
			PlaceCanvas(Page, Button, CellOrigin, FVector2D(FrameSize, FrameSize), 5);
			SetInvisibleButtonChrome(Button);
		}
		// 확정 4x2(골드 1 + 아티팩트 7) 밖에 남은 옛 칸을 걷는다.
		for (const TCHAR* Prefix : { TEXT("MercenaryInventoryArtifactSelection"),
			TEXT("MercenaryInventoryArtifactFrame"), TEXT("MercenaryInventoryArtifactIcon"),
			TEXT("MercenaryInventoryArtifactName"), TEXT("MercenaryInventoryArtifactButton") })
		{
			for (int32 Index = 7; Index < 16; ++Index)
			{
				if (UWidget* Stale = Blueprint->WidgetTree->FindWidget(
					FName(*FString::Printf(TEXT("%s_%d"), Prefix, Index))))
				{
					Blueprint->WidgetTree->RemoveWidget(Stale);
				}
			}
		}

		UImage* DescriptionPlate = FindOrCreate<UImage>(
			Blueprint, TEXT("MercenaryInventoryDescriptionPlate"));
		PlaceCanvas(Page, DescriptionPlate, FVector2D(65.f, 585.f),
			FVector2D(1080.f, 72.f), 2);
		DescriptionPlate->SetBrushFromTexture(DescriptionPlateTexture, false);
		DescriptionPlate->SetVisibility(ESlateVisibility::Collapsed);

		UTextBlock* Description = FindOrCreate<UTextBlock>(
			Blueprint, TEXT("MercenaryInventoryDescriptionText"));
		PlaceCanvas(Page, Description, FVector2D(135.f, 596.f),
			FVector2D(940.f, 50.f), 3);
		Description->SetText(NSLOCTEXT("CombatHUD", "MercenaryInventoryPreview",
			"피의 성배 — 처치 시 체력을 5 회복한다."));
		SetReadableFont(Description, BaseFont, 20);
		Description->SetJustification(ETextJustify::Center);
		Description->SetVisibility(ESlateVisibility::Collapsed);
	}

	void BuildEnemyAPPips(UWidgetBlueprint* Blueprint)
	{
		UCanvasPanel* EnemyPanel = Cast<UCanvasPanel>(
			Blueprint->WidgetTree->FindWidget(TEXT("EnemyPanel")));
		UImage* TemplatePip = Cast<UImage>(
			Blueprint->WidgetTree->FindWidget(TEXT("TurnAPPip_0")));
		UImage* EmptyTemplatePip = Cast<UImage>(
			Blueprint->WidgetTree->FindWidget(TEXT("TurnAPPipUsed_0")));
		UWidget* Anchor = Blueprint->WidgetTree->FindWidget(TEXT("EnemyCritPlate"));
		UCanvasPanelSlot* AnchorSlot = Anchor != nullptr
			? Cast<UCanvasPanelSlot>(Anchor->Slot) : nullptr;
		UCanvasPanel* ExistingRow = Cast<UCanvasPanel>(
			Blueprint->WidgetTree->FindWidget(TEXT("EnemyAPPipRow")));
		UCanvasPanelSlot* ExistingRowSlot = ExistingRow != nullptr
			? Cast<UCanvasPanelSlot>(ExistingRow->Slot) : nullptr;
		if (EnemyPanel == nullptr || TemplatePip == nullptr
			|| EmptyTemplatePip == nullptr
			|| (ExistingRowSlot == nullptr && AnchorSlot == nullptr))
		{
			UE_LOG(LogTemp, Error,
				TEXT("RD_COMBAT_HUD_INVENTORY_BUILD missing enemy AP anchor"));
			return;
		}
		// 디자이너에서 옮긴 행 위치는 보존한다. 칸 구성만 30x30 열 칸으로 고친다.
		const FVector2D RowPosition = ExistingRowSlot != nullptr
			? ExistingRowSlot->GetPosition() : AnchorSlot->GetPosition();
		constexpr int32 PipCount = 10;
		constexpr float PipSize = 30.f;
		constexpr float PipGap = 4.f;
		const FVector2D RowSize(
			PipCount * PipSize + (PipCount - 1) * PipGap, PipSize);
		// 치명타 칸은 AP 보석 행의 임시 앵커였을 뿐, 보석으로 대체되는 정보가
		// 아니다. 용병 요약판과 동일하게 프레임/아이콘/값을 남긴다.
		for (const TCHAR* CriticalWidget : {
			TEXT("EnemyCritPlate"), TEXT("EnemyCritIcon"), TEXT("EnemyCritText") })
		{
			if (UWidget* Widget = Blueprint->WidgetTree->FindWidget(
				FName(CriticalWidget)))
			{
				Widget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}
		}
		// WBP 자체를 열거나 오프스크린으로 찍을 때도 아래 미리보기 적(4 AP)과
		// 숫자가 어긋나지 않게 한다. 실게임에서는 RefreshEnemy가 같은
		// FUnitUI::mActionPoints 값으로 이 문구와 보석을 함께 갱신한다.
		if (UTextBlock* APText = Cast<UTextBlock>(
			Blueprint->WidgetTree->FindWidget(TEXT("EnemyAPText"))))
		{
			APText->SetText(FText::FromString(TEXT("AP 4/4")));
		}

		UCanvasPanel* Row = FindOrCreate<UCanvasPanel>(
			Blueprint, TEXT("EnemyAPPipRow"));
		PlaceCanvas(EnemyPanel, Row, RowPosition, RowSize, 15);
		Row->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		for (int32 Index = 0; Index < PipCount; ++Index)
		{
			const FVector2D PipPosition(Index * (PipSize + PipGap), 0.f);
			UImage* UsedPip = FindOrCreate<UImage>(Blueprint,
				FName(*FString::Printf(TEXT("EnemyAPPipUsed_%d"), Index)));
			PlaceCanvas(Row, UsedPip, PipPosition,
				FVector2D(PipSize, PipSize), 0);
			UsedPip->SetBrush(EmptyTemplatePip->GetBrush());
			UsedPip->SetVisibility(ESlateVisibility::Collapsed);

			UImage* Pip = FindOrCreate<UImage>(Blueprint,
				FName(*FString::Printf(TEXT("EnemyAPPip_%d"), Index)));
			PlaceCanvas(Row, Pip, PipPosition,
				FVector2D(PipSize, PipSize), 1);
			Pip->SetBrush(TemplatePip->GetBrush());
			Pip->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	bool SaveCompiledBlueprint(UWidgetBlueprint* Blueprint)
	{
		if (Blueprint == nullptr)
		{
			return false;
		}
		PruneStaleVariables(Blueprint);
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		return UPackage::SavePackage(Blueprint->GetPackage(), Blueprint,
			*FPackageName::LongPackageNameToFilename(
				Blueprint->GetOutermost()->GetName(),
				FPackageName::GetAssetPackageExtension()),
			FSavePackageArgs());
	}

	bool RepairMercenaryPortraitFrame(UWidgetBlueprint* Blueprint,
		UTexture2D* PortraitFrameTexture)
	{
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr
			|| PortraitFrameTexture == nullptr)
		{
			return false;
		}
		UImage* PortraitFrame = Cast<UImage>(
			Blueprint->WidgetTree->FindWidget(TEXT("MercenaryPortraitFrame")));
		if (PortraitFrame == nullptr)
		{
			return false;
		}
		FSlateBrush Brush = PortraitFrame->GetBrush();
		Brush.SetResourceObject(PortraitFrameTexture);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.Margin = FMargin(0.f);
		PortraitFrame->SetBrush(Brush);
		PortraitFrame->SetColorAndOpacity(FLinearColor::White);
		PortraitFrame->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		if (UCanvasPanelSlot* FrameSlot = Cast<UCanvasPanelSlot>(PortraitFrame->Slot))
		{
			if (UWidget* Hero = Blueprint->WidgetTree->FindWidget(
				TEXT("MercenaryHeroPortrait")))
			{
				if (UCanvasPanelSlot* HeroSlot = Cast<UCanvasPanelSlot>(Hero->Slot))
				{
					// T_KitA_Cell_Normal은 중앙까지 불투명하다. 초상화 위에 그리면
					// 얼굴을 가리므로 셀은 매트처럼 아래, 크롭된 초상화는 위에 둔다.
					FrameSlot->SetZOrder(HeroSlot->GetZOrder() - 1);
					Hero->SetClipping(EWidgetClipping::ClipToBoundsAlways);
				}
			}
		}
		return true;
	}

	void RepairMercenaryPortraitFrameOnly()
	{
		UWidgetBlueprint* HudBlueprint =
			LoadObject<UWidgetBlueprint>(nullptr, AssetPath);
		UWidgetBlueprint* MercenaryBlueprint =
			LoadObject<UWidgetBlueprint>(nullptr, MercenaryAssetPath);
		UTexture2D* PortraitFrameTexture = LoadObject<UTexture2D>(
			nullptr, DetailPortraitCellTexturePath);
		if (HudBlueprint == nullptr || MercenaryBlueprint == nullptr
			|| PortraitFrameTexture == nullptr)
		{
			UE_LOG(LogTemp, Error,
				TEXT("RD_COMBAT_HUD_MERC_FRAME_REPAIR missing WBP or texture"));
			return;
		}

		bool bAllSaved = true;
		for (UWidgetBlueprint* Blueprint : { HudBlueprint, MercenaryBlueprint })
		{
			Blueprint->Modify();
			Blueprint->WidgetTree->Modify();
			bAllSaved = RepairMercenaryPortraitFrame(
				Blueprint, PortraitFrameTexture)
				&& SaveCompiledBlueprint(Blueprint) && bAllSaved;
		}
		if (bAllSaved == false)
		{
			UE_LOG(LogTemp, Error,
				TEXT("RD_COMBAT_HUD_MERC_FRAME_REPAIR failed"));
			return;
		}
		UE_LOG(LogTemp, Display,
			TEXT("RD_COMBAT_HUD_MERC_FRAME_REPAIR success assets=inline,modular texture=T_KitA_Cell_Normal"));
	}

	/**
	 * @brief 최신 전투 HUD 배치를 보존하고 인벤토리 탭만 WBP에 굽는다.
	 *
	 * 전체 HUD 빌더는 과거 좌표를 함께 가지고 있어 디자이너가 손본 요약판을
	 * 되돌릴 수 있다. 이 경로는 MercenaryBoard 아래 새 다섯 위젯만 만진다.
	 */
	void BuildInventoryTabOnly()
	{
		UWidgetBlueprint* HudBlueprint =
			LoadObject<UWidgetBlueprint>(nullptr, AssetPath);
		UWidgetBlueprint* MercenaryBlueprint =
			LoadObject<UWidgetBlueprint>(nullptr, MercenaryAssetPath);
		UTexture2D* InventoryIconTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/UI/KayKit/KK_Icon_Inventory.KK_Icon_Inventory"));
		UTexture2D* GoldIconTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Reward/T_Reward_GoldIcon_V1.T_Reward_GoldIcon_V1"));
		UTexture2D* DescriptionPlateTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_Row_Plate.T_KitA_Row_Plate"));
		UTexture2D* ArtifactSlotTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_Cell_Normal.T_KitA_Cell_Normal"));
		UTexture2D* PortraitFrameTexture = LoadObject<UTexture2D>(nullptr,
			DetailPortraitCellTexturePath);
		if (HudBlueprint == nullptr || MercenaryBlueprint == nullptr
			|| InventoryIconTexture == nullptr || GoldIconTexture == nullptr
			|| DescriptionPlateTexture == nullptr || ArtifactSlotTexture == nullptr
			|| PortraitFrameTexture == nullptr)
		{
			UE_LOG(LogTemp, Error,
				TEXT("RD_COMBAT_HUD_INVENTORY_BUILD missing WBP or texture"));
			return;
		}

		UTextBlock* RoundText = Cast<UTextBlock>(
			HudBlueprint->WidgetTree->FindWidget(TEXT("RoundText")));
		UCanvasPanel* InlineBoard = Cast<UCanvasPanel>(
			HudBlueprint->WidgetTree->FindWidget(TEXT("MercenaryBoard")));
		UCanvasPanel* ModularBoard = Cast<UCanvasPanel>(
			MercenaryBlueprint->WidgetTree->FindWidget(TEXT("MercenaryBoard")));
		if (RoundText == nullptr || InlineBoard == nullptr || ModularBoard == nullptr)
		{
			UE_LOG(LogTemp, Error,
				TEXT("RD_COMBAT_HUD_INVENTORY_BUILD missing authored board"));
			return;
		}

		const FSlateFontInfo BaseFont = RoundText->GetFont();
		for (const TPair<UWidgetBlueprint*, UCanvasPanel*> Target : {
			TPair<UWidgetBlueprint*, UCanvasPanel*>(HudBlueprint, InlineBoard),
			TPair<UWidgetBlueprint*, UCanvasPanel*>(MercenaryBlueprint, ModularBoard) })
		{
			Target.Key->Modify();
			Target.Key->WidgetTree->Modify();
			BuildMercenaryInventoryTab(Target.Key, Target.Value, BaseFont,
				InventoryIconTexture, GoldIconTexture, DescriptionPlateTexture,
				ArtifactSlotTexture);
			// 이 전용 빌더로 갱신하는 기존 WBP에는 스킬 그림만 있고 입력 버튼이
			// 없을 수 있다. 다른 HUD 배치는 건드리지 않고 각 프레임 슬롯만 복제한다.
			for (int32 Index = 0; Index < 6; ++Index)
			{
				UWidget* Frame = Target.Key->WidgetTree->FindWidget(FName(*FString::Printf(
					TEXT("MercenarySkillFrame_%d"), Index)));
				UCanvasPanel* SkillCanvasParent = Frame != nullptr
					? Cast<UCanvasPanel>(Frame->GetParent()) : nullptr;
				UOverlay* SkillOverlayParent = Frame != nullptr
					? Cast<UOverlay>(Frame->GetParent()) : nullptr;
				const UCanvasPanelSlot* FrameSlot = Frame != nullptr
					? Cast<UCanvasPanelSlot>(Frame->Slot) : nullptr;
				if (Frame == nullptr
					|| (SkillCanvasParent == nullptr && SkillOverlayParent == nullptr))
				{
					continue;
				}
				UButton* Button = FindOrCreate<UButton>(Target.Key,
					FName(*FString::Printf(TEXT("MercenarySkillButton_%d"), Index)));
				if (SkillCanvasParent != nullptr && FrameSlot != nullptr)
				{
					PlaceCanvas(SkillCanvasParent, Button, FrameSlot->GetPosition(),
						FrameSlot->GetSize(), FrameSlot->GetZOrder() + 20);
				}
				else if (SkillOverlayParent != nullptr)
				{
					EnsureParent(SkillOverlayParent, Button);
					if (UOverlaySlot* ButtonSlot = Cast<UOverlaySlot>(Button->Slot))
					{
						ButtonSlot->SetPadding(FMargin(0.f));
						ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
						ButtonSlot->SetVerticalAlignment(VAlign_Fill);
					}
				}
				SetInvisibleButtonChrome(Button);
			}
			RepairMercenaryPortraitFrame(Target.Key, PortraitFrameTexture);
		}
		if (UWidget* ArtifactStrip =
			HudBlueprint->WidgetTree->FindWidget(TEXT("ArtifactStrip")))
		{
			HudBlueprint->WidgetTree->RemoveWidget(ArtifactStrip);
		}
		BuildEnemyAPPips(HudBlueprint);
		PruneStaleVariables(HudBlueprint);
		PruneStaleVariables(MercenaryBlueprint);

		const bool bHudSaved = SaveCompiledBlueprint(HudBlueprint);
		const bool bMercenarySaved = SaveCompiledBlueprint(MercenaryBlueprint);
		if (bHudSaved == false || bMercenarySaved == false)
		{
			UE_LOG(LogTemp, Error, TEXT("RD_COMBAT_HUD_INVENTORY_BUILD save failed"));
			return;
		}
		UE_LOG(LogTemp, Display,
			TEXT("RD_COMBAT_HUD_INVENTORY_BUILD success assets=2"));
	}

	/** @brief 다른 HUD 구역은 건드리지 않고 0811 ROUND+턴바만 복구한다. */
	void RepairRoundTurnOnly()
	{
		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, AssetPath);
		UTexture2D* RoundBadgeTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_RoundBadge_Frame.T_MB_RoundBadge_Frame"));
		UTexture2D* TurnTokenFrameTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_TurnToken_Frame.T_MB_TurnToken_Frame"));
		UTexture2D* SpeedTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_Icon_Speed.T_MB_Icon_Speed"));
		UTexture2D* ActionButtonTexture = LoadObject<UTexture2D>(nullptr,
			ActionButtonTexturePath);
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr
			|| RoundBadgeTexture == nullptr || TurnTokenFrameTexture == nullptr
			|| SpeedTexture == nullptr || ActionButtonTexture == nullptr)
		{
			UE_LOG(LogTemp, Error,
				TEXT("RD_COMBAT_HUD_ROUND_TURN_REPAIR missing WBP or texture"));
			return;
		}

		Blueprint->Modify();
		Blueprint->WidgetTree->Modify();
		RepairAuthoredRoundTurnLayout(Blueprint, RoundBadgeTexture,
			TurnTokenFrameTexture, SpeedTexture);
		RepairAuthoredActionButtonArt(Blueprint, ActionButtonTexture);
		if (SaveCompiledBlueprint(Blueprint) == false)
		{
			UE_LOG(LogTemp, Error,
				TEXT("RD_COMBAT_HUD_ROUND_TURN_REPAIR save failed"));
			return;
		}
		UE_LOG(LogTemp, Display,
			TEXT("RD_COMBAT_HUD_ROUND_TURN_REPAIR success"));
	}

	/** @brief ROUND/용병 배치는 유지하고 #519 버튼 그림만 복구한다. */
	void RepairActionButtonArtOnly()
	{
		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, AssetPath);
		UTexture2D* ActionButtonTexture = LoadObject<UTexture2D>(nullptr,
			ActionButtonTexturePath);
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr
			|| ActionButtonTexture == nullptr)
		{
			UE_LOG(LogTemp, Error,
				TEXT("RD_COMBAT_HUD_ACTION_BUTTON_ART_REPAIR missing WBP or texture"));
			return;
		}

		Blueprint->Modify();
		Blueprint->WidgetTree->Modify();
		RepairAuthoredActionButtonArt(Blueprint, ActionButtonTexture);
		if (SaveCompiledBlueprint(Blueprint) == false)
		{
			UE_LOG(LogTemp, Error,
				TEXT("RD_COMBAT_HUD_ACTION_BUTTON_ART_REPAIR save failed"));
			return;
		}
		UE_LOG(LogTemp, Display,
			TEXT("RD_COMBAT_HUD_ACTION_BUTTON_ART_REPAIR success plates=2"));
	}

	/** @brief ROUND/턴바/용병 판을 보존하고 0811 우측 HUD만 복구한다. */
	void RepairRightHUDOnly()
	{
		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, AssetPath);
		UTexture2D* OptionsRailFrameTexture = LoadObject<UTexture2D>(nullptr,
			OptionsRailFrameTexturePath);
		UTexture2D* ActionButtonTexture = LoadObject<UTexture2D>(nullptr,
			ActionButtonTexturePath);
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr
			|| OptionsRailFrameTexture == nullptr || ActionButtonTexture == nullptr)
		{
			UE_LOG(LogTemp, Error,
				TEXT("RD_COMBAT_HUD_RIGHT_REPAIR missing WBP or texture"));
			return;
		}

		Blueprint->Modify();
		Blueprint->WidgetTree->Modify();
		RepairAuthoredRightHUDLayout(Blueprint, OptionsRailFrameTexture);
		RepairAuthoredActionButtonArt(Blueprint, ActionButtonTexture);
		if (SaveCompiledBlueprint(Blueprint) == false)
		{
			UE_LOG(LogTemp, Error, TEXT("RD_COMBAT_HUD_RIGHT_REPAIR save failed"));
			return;
		}
		UE_LOG(LogTemp, Display,
			TEXT("RD_COMBAT_HUD_RIGHT_REPAIR success protected=round,turn,mercenary"));
	}

	void BuildMercenaryPanel(UWidgetBlueprint* Blueprint, const FSlateFontInfo& BaseFont,
		UTexture2D* ShellTexture, UTexture2D* NormalCardTexture,
		UTexture2D* SelectedCardTexture, UTexture2D* BackButtonTexture,
		UTexture2D* SkillFrameTexture, UTexture2D* PortraitFrameTexture,
		UTexture2D* InventoryIconTexture,
		UTexture2D* GoldIconTexture, UTexture2D* DescriptionPlateTexture)
	{
		UCanvasPanel* Root = CastChecked<UCanvasPanel>(Blueprint->WidgetTree->RootWidget);
		UCanvasPanel* Panel = FindOrCreate<UCanvasPanel>(Blueprint, TEXT("MercenaryPanel"));
		PlaceCanvas(Root, Panel, FVector2D::ZeroVector, FVector2D(1920.f, 1080.f), 10000);
		if (UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(Panel->Slot))
		{
			// The old Python pass left this as a 1920x1080 rectangle attached to the
			// top-left corner.  On a small/wide mobile viewport the rectangle was
			// clipped and the live combat HUD leaked through on the right.  The modal
			// shell owns the whole viewport; only its authored contents are aspect-fit.
			PanelSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			PanelSlot->SetOffsets(FMargin(0.f));
		}
		Panel->SetVisibility(ESlateVisibility::Collapsed);

		UImage* Shell = FindOrCreate<UImage>(Blueprint, TEXT("RuntimeMercenaryRosterShell"));
		PlaceCanvas(Panel, Shell, FVector2D(90.f, 70.f), FVector2D(1740.f, 960.f), -100);
		Shell->SetBrushFromTexture(ShellTexture, false);
		Shell->SetVisibility(ESlateVisibility::HitTestInvisible);

		// 외곽 셸 자체가 배경을 제공한다. 별도의 불투명 검정 Well은 좁은
		// 화면에서 아래 16:9 Board와 서로 다른 배율을 받아 내용만 가렸다.
		// 예전 빌더가 만든 잔재도 지워 재빌드가 Photo1 회귀를 만들지 않게 한다.
		if (UWidget* ContentWell = Blueprint->WidgetTree->FindWidget(
			TEXT("MercenaryContentWell")))
		{
			Blueprint->WidgetTree->RemoveWidget(ContentWell);
		}

		UBorder* Scrim = FindOrCreate<UBorder>(Blueprint, TEXT("MercenaryScrim"));
		PlaceCanvas(Panel, Scrim, FVector2D::ZeroVector, FVector2D(1920.f, 1080.f), -90);
		if (UCanvasPanelSlot* ScrimSlot = Cast<UCanvasPanelSlot>(Scrim->Slot))
		{
			ScrimSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			ScrimSlot->SetOffsets(FMargin(0.f));
		}
		Scrim->SetVisibility(ESlateVisibility::Collapsed);

		UScaleBox* BoardScale = FindOrCreate<UScaleBox>(Blueprint,
			TEXT("MercenaryBoardScale"));
		PlaceCanvas(Panel, BoardScale, FVector2D::ZeroVector,
			FVector2D(1920.f, 1080.f), 1);
		if (UCanvasPanelSlot* ScaleSlot = Cast<UCanvasPanelSlot>(BoardScale->Slot))
		{
			ScaleSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			ScaleSlot->SetOffsets(FMargin(0.f));
		}
		BoardScale->SetStretch(EStretch::ScaleToFit);
		BoardScale->SetStretchDirection(EStretchDirection::Both);

		USizeBox* BoardSize = FindOrCreate<USizeBox>(Blueprint,
			TEXT("MercenaryBoardDesignSize"));
		BoardSize->SetWidthOverride(1920.f);
		BoardSize->SetHeightOverride(1080.f);
		EnsureParent(BoardScale, BoardSize);

		UCanvasPanel* Board = FindOrCreate<UCanvasPanel>(Blueprint, TEXT("MercenaryBoard"));
		EnsureParent(BoardSize, Board);
		UCanvasPanel* Roster = FindOrCreate<UCanvasPanel>(
			Blueprint, TEXT("MercRosterSection"));
		// 용병 세 행과 인벤토리 탭은 하나의 내비게이션 묶음이다. 개별 행을
		// 따로 밀지 않고 부모를 이동해야 네 줄의 좌우 정렬이 계속 유지된다.
		PlaceCanvas(Board, Roster, FVector2D(240.f, 0.f), FVector2D(1680.f, 1080.f), 2);
		UCanvasPanel* DetailSection = FindOrCreate<UCanvasPanel>(
			Blueprint, TEXT("MercDetailSection"));
		PlaceCanvas(Board, DetailSection, FVector2D::ZeroVector,
			FVector2D(1920.f, 1080.f), 3);

		for (const TCHAR* LegacyName : {
			TEXT("MercenaryHeaderPlate"), TEXT("MercenaryBoardPlate"),
			TEXT("MercenaryBoardShadow"), TEXT("MercenaryBoardInner"),
			TEXT("MercenaryClosePlate") })
		{
			UWidget* Legacy = Blueprint->WidgetTree->FindWidget(FName(LegacyName));
			if (Legacy == nullptr)
			{
				Legacy = Blueprint->WidgetTree->ConstructWidget<UImage>(
					UImage::StaticClass(), FName(LegacyName));
				Blueprint->OnVariableAdded(FName(LegacyName));
			}
			PlaceCanvas(Board, Legacy, FVector2D::ZeroVector, FVector2D(1.f, 1.f), -50);
			Legacy->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (UTexture2D* TitlePlateTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_Title_Plate.T_KitA_Title_Plate")))
		{
			UImage* HeaderPlate = FindOrCreate<UImage>(Blueprint,
				TEXT("MercenaryTitlePlate"));
			PlaceCanvas(Board, HeaderPlate, FVector2D(620.f, 80.f),
				FVector2D(680.f, 112.f), 3);
			HeaderPlate->SetBrushFromTexture(TitlePlateTexture, false);
			HeaderPlate->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}

		UTextBlock* Title = FindOrCreate<UTextBlock>(Blueprint, TEXT("MercenaryTitleText"));
		PlaceCanvas(Board, Title, FVector2D(689.f, 86.f), FVector2D(542.f, 88.f), 4);
		Title->SetText(NSLOCTEXT("CombatHUD", "MercenaryTabTitle", "용병"));
		SetReadableFont(Title, BaseFont, 54);

		UTextBlock* Subtitle = FindOrCreate<UTextBlock>(Blueprint,
			TEXT("MercenarySubtitleText"));
		PlaceCanvas(Board, Subtitle, FVector2D::ZeroVector, FVector2D(1.f, 1.f), 4);
		Subtitle->SetVisibility(ESlateVisibility::Collapsed);

		UTextBlock* GoldLabel = FindOrCreate<UTextBlock>(Blueprint,
			TEXT("MercenaryGoldLabel"));
		PlaceCanvas(Board, GoldLabel, FVector2D(48.f, 55.f), FVector2D(116.f, 68.f), 4);
		GoldLabel->SetText(NSLOCTEXT("CombatHUD", "MercenaryGoldLabel", "골드"));
		SetReadableFont(GoldLabel, BaseFont, 28);
		GoldLabel->SetVisibility(ESlateVisibility::Collapsed);

		UTextBlock* GoldText = FindOrCreate<UTextBlock>(Blueprint,
			TEXT("MercenaryGoldText"));
		PlaceCanvas(Board, GoldText, FVector2D(164.f, 50.f), FVector2D(180.f, 78.f), 4);
		GoldText->SetText(FText::AsNumber(0));
		SetReadableFont(GoldText, BaseFont, 42);
		GoldText->SetVisibility(ESlateVisibility::Collapsed);

		UImage* BackArt = FindOrCreate<UImage>(Blueprint, TEXT("MercenaryBackArt"));
		PlaceCanvas(Board, BackArt, FVector2D(825.f, 922.f), FVector2D(270.f, 96.f), 5);
		BackArt->SetBrushFromTexture(BackButtonTexture, false);
		if (BackButtonTexture != nullptr)
		{
			// 270x112 로 늘려 쓴다. 통짜로 늘리면 금 모서리까지 늘어나므로
			// 9-slice 로 그린다. 여백은 실측 35px + 잘라낼 때 남긴 8px 이다.
			// GetSizeX() 는 갓 LoadObject 한 텍스처에서 0 을 준다(플랫폼 데이터가
			// 아직 없다). 0 으로 나눠 마진이 inf 가 됐다 -- 실제로 그렇게 나왔다.
			// GetImportedSize() 는 원본 크기라 언제나 옳다.
			const FIntPoint Imported = BackButtonTexture->GetImportedSize();
			const FVector2D Texel(Imported.X, Imported.Y);
			FSlateBrush BackBrush = BackArt->GetBrush();
			BackBrush.DrawAs = ESlateBrushDrawType::Box;
			BackBrush.Margin = FMargin(
				(8.f + 44.f) / Texel.X, (8.f + 35.f) / Texel.Y,
				(8.f + 44.f) / Texel.X, (8.f + 35.f) / Texel.Y);
			BackArt->SetBrush(BackBrush);
		}
		BackArt->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		/*
		 * TextBlock에는 수직 정렬이 없어 캔버스 슬롯에 직접 두면 글리프가
		 * 위에 붙는다. 버튼 판과 같은 사각의 Overlay에 넣어 중앙 정렬한다.
		 */
		UOverlay* CloseCenter = FindOrCreate<UOverlay>(Blueprint,
			TEXT("MercenaryCloseText_Center"));
		PlaceCanvas(Board, CloseCenter, FVector2D(825.f, 922.f), FVector2D(270.f, 96.f), 6);
		CloseCenter->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UTextBlock* CloseText = FindOrCreate<UTextBlock>(Blueprint,
			TEXT("MercenaryCloseText"));
		EnsureParent(CloseCenter, CloseText);
		if (UOverlaySlot* CloseTextSlot = Cast<UOverlaySlot>(CloseText->Slot))
		{
			CloseTextSlot->SetPadding(FMargin(0.f));
			CloseTextSlot->SetHorizontalAlignment(HAlign_Center);
			CloseTextSlot->SetVerticalAlignment(VAlign_Center);
		}
		CloseText->SetText(NSLOCTEXT("CombatHUD", "MercenaryBack", "닫기"));
		CloseText->SetJustification(ETextJustify::Center);
		SetReadableFont(CloseText, BaseFont, 34);

		UButton* CloseButton = FindOrCreate<UButton>(Blueprint,
			TEXT("MercenaryCloseButton"));
		PlaceCanvas(Board, CloseButton, FVector2D(825.f, 922.f), FVector2D(270.f, 96.f), 7);
		SetInvisibleButtonChrome(CloseButton);

		const FVector2D LocalCardSize(350.f, 128.f);
		const FVector2D CardPositions[] = {
			FVector2D(250.f, 280.f), FVector2D(250.f, 416.f), FVector2D(250.f, 552.f)
		};
		for (int32 Index = 0; Index < 3; ++Index)
		{
			const FString Suffix = FString::Printf(TEXT("_%d"), Index);
			UScaleBox* Scale = FindOrCreate<UScaleBox>(Blueprint,
				FName(*FString::Printf(TEXT("MercenaryCardScale_%d"), Index)));
			PlaceCanvas(Roster, Scale, CardPositions[Index], FVector2D(390.f, 142.f), 3);
			Scale->SetStretch(EStretch::ScaleToFit);
			Scale->SetStretchDirection(EStretchDirection::Both);

			UCanvasPanel* Card = FindOrCreate<UCanvasPanel>(Blueprint,
				FName(TEXT("PartyCard") + Suffix));
			EnsureParent(Scale, Card);
			Card->SetClipping(EWidgetClipping::ClipToBoundsAlways);

			UImage* Plate = FindOrCreate<UImage>(Blueprint,
				FName(TEXT("PartyPlate") + Suffix));
			PlaceCanvas(Card, Plate, FVector2D::ZeroVector, LocalCardSize, 1);
			Plate->SetBrushFromTexture(Index == 0
				? SelectedCardTexture : NormalCardTexture, false);
			Plate->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			UCanvasPanel* Content = FindOrCreate<UCanvasPanel>(Blueprint,
				FName(TEXT("PartyContent") + Suffix));
			PlaceCanvas(Card, Content, FVector2D::ZeroVector, LocalCardSize, 2);

			UImage* Portrait = FindOrCreate<UImage>(Blueprint,
				FName(TEXT("PartyPortrait") + Suffix));
			// Marchbound list portraits are authored square.  Keeping the slot square
			// avoids the stretched faces produced by the old 112x150 rectangle.
			PlaceCanvas(Content, Portrait, FVector2D(18.f, 16.f), FVector2D(96.f, 96.f), 10);

			UTextBlock* Name = FindOrCreate<UTextBlock>(Blueprint,
				FName(TEXT("PartyName") + Suffix));
			PlaceCanvas(Content, Name, FVector2D(128.f, 21.f), FVector2D(190.f, 46.f), 15);
			Name->SetText(FText::FromString(FString::Printf(TEXT("용병 %d"), Index + 1)));
			SetReadableFont(Name, BaseFont, 27);

			UProgressBar* HPBar = FindOrCreate<UProgressBar>(Blueprint,
				FName(TEXT("PartyHPBar") + Suffix));
			PlaceCanvas(Content, HPBar, FVector2D(128.f, 76.f), FVector2D(190.f, 26.f), 10);
			HPBar->SetPercent(1.f);

			UTextBlock* HPText = FindOrCreate<UTextBlock>(Blueprint,
				FName(TEXT("PartyHPText") + Suffix));
			PlaceCanvas(Content, HPText, FVector2D(128.f, 73.f), FVector2D(190.f, 30.f), 15);
			HPText->SetText(FText::FromString(TEXT("100/100")));
			SetReadableFont(HPText, BaseFont, 20);

			UImage* APPlate = FindOrCreate<UImage>(Blueprint,
				FName(TEXT("PartyAPPlate") + Suffix));
			PlaceCanvas(Content, APPlate, FVector2D(128.f, 102.f), FVector2D(190.f, 24.f), 10);
			APPlate->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			UTextBlock* APText = FindOrCreate<UTextBlock>(Blueprint,
				FName(TEXT("PartyAPText") + Suffix));
			PlaceCanvas(Content, APText, FVector2D(128.f, 100.f), FVector2D(190.f, 26.f), 15);
			APText->SetText(FText::FromString(TEXT("AP 10/10")));
			SetReadableFont(APText, BaseFont, 21);

			UTextBlock* StatusText = FindOrCreate<UTextBlock>(Blueprint,
				FName(TEXT("PartyStatus") + Suffix));
			PlaceCanvas(Content, StatusText, FVector2D(128.f, 101.f), FVector2D(108.f, 24.f), 15);
			SetReadableFont(StatusText, BaseFont, 17);
			StatusText->SetJustification(ETextJustify::Left);
			StatusText->SetVisibility(ESlateVisibility::Collapsed);
			UImage* StatusIcon = FindOrCreate<UImage>(Blueprint,
				FName(TEXT("PartyStatusIcon") + Suffix));
			PlaceCanvas(Content, StatusIcon, FVector2D(291.f, 151.f), FVector2D(32.f, 32.f), 16);
			StatusIcon->SetVisibility(ESlateVisibility::Collapsed);

			for (int32 StatusIndex = 0; StatusIndex < 3; ++StatusIndex)
			{
				const float X = 288.f - 42.f * StatusIndex;
				UImage* Frame = FindOrCreate<UImage>(Blueprint, FName(*FString::Printf(
					TEXT("PartyStatusFrame_%d_%d"), Index, StatusIndex)));
				PlaceCanvas(Content, Frame, FVector2D(X, 146.f), FVector2D(38.f, 38.f), 18);
				Frame->SetVisibility(ESlateVisibility::Collapsed);
				UImage* Icon = FindOrCreate<UImage>(Blueprint, FName(*FString::Printf(
					TEXT("PartyStatusIcon_%d_%d"), Index, StatusIndex)));
				PlaceCanvas(Content, Icon, FVector2D(X + 4.f, 150.f), FVector2D(30.f, 30.f), 19);
				Icon->SetVisibility(ESlateVisibility::Collapsed);
			}

			UButton* Button = FindOrCreate<UButton>(Blueprint,
				FName(TEXT("PartyButton") + Suffix));
			PlaceCanvas(Card, Button, FVector2D::ZeroVector, LocalCardSize, 29);
			SetInvisibleButtonChrome(Button);
		}

		BuildMercenaryInventoryTab(Blueprint, Board, BaseFont,
			InventoryIconTexture, GoldIconTexture, DescriptionPlateTexture,
			SkillFrameTexture);

		UImage* PortraitFrame = FindOrCreate<UImage>(Blueprint,
			TEXT("MercenaryPortraitFrame"));
		PlaceCanvas(DetailSection, PortraitFrame, FVector2D(650.f, 292.f),
			FVector2D(360.f, 360.f), 4);
		PortraitFrame->SetBrushFromTexture(PortraitFrameTexture, false);
		PortraitFrame->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UImage* Hero = FindOrCreate<UImage>(Blueprint, TEXT("MercenaryHeroPortrait"));
		// The new hero illustrations are 1:1.  Show them as a large square rather
		// than turning a still image into a fake, stretched 3D standing model.
		PlaceCanvas(DetailSection, Hero, FVector2D(680.f, 322.f), FVector2D(300.f, 300.f), 5);
		Hero->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		if (UTexture2D* PreviewHero = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireHero_Knight.T_MB_HireHero_Knight")))
		{
			Hero->SetBrushFromTexture(PreviewHero, false);
		}

		struct FDetailText
		{
			const TCHAR* Name;
			FVector2D Position;
			FVector2D Size;
			const TCHAR* Preview;
			int32 FontSize;
		};
		/*
		 * 이름만 글자로 두고 수치 셋은 **칩**으로 옮긴다.
		 *
		 * "HP 100 / 100" 을 세 줄로 쌓아 놓으니 오른쪽 위가 글자 벽이 됐고,
		 * 정작 아래 스킬 칸과 결이 달랐다. 상세창·몬스터탭이 이미 칩을 쓰므로
		 * 같은 물건으로 맞춘다 -- 같은 값은 같은 모양으로 보여야 한다.
		 */
		const FDetailText Details[] = {
			{ TEXT("MercenaryDetailName"), FVector2D(1080.f, 230.f), FVector2D(620.f, 82.f), TEXT("용병"), 42 },
		};
		for (const FDetailText& Detail : Details)
		{
			UTextBlock* Text = FindOrCreate<UTextBlock>(Blueprint, FName(Detail.Name));
			PlaceCanvas(DetailSection, Text, Detail.Position, Detail.Size, 8);
			Text->SetText(FText::FromString(Detail.Preview));
			SetReadableFont(Text, BaseFont, Detail.FontSize);
			Text->SetJustification(ETextJustify::Left);
		}

		// 수치 칩 셋. 이름 바로 밑에 가로로 놓는다.
		const TCHAR* const ChipValueNames[3] = {
			TEXT("MercenaryDetailHP"), TEXT("MercenaryDetailAP"),
			TEXT("MercenaryDetailSpeed") };
		const TCHAR* const ChipLabels[3] = { TEXT("HP"), TEXT("AP"), TEXT("속도") };
		for (int32 Index = 0; Index < 3; ++Index)
		{
			const FVector2D ChipPos(1080.f, 330.f + 82.f * Index);
			const FVector2D ChipSize(620.f, 68.f);
			UImage* Ring = FindOrCreate<UImage>(Blueprint, FName(*FString::Printf(
				TEXT("MercenaryChip%dFrame"), Index)));
			PlaceCanvas(DetailSection, Ring, ChipPos, ChipSize, 8);
			Ring->SetBrushFromTexture(DescriptionPlateTexture, false);
			Ring->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			UTextBlock* Label = FindOrCreate<UTextBlock>(Blueprint, FName(*FString::Printf(
				TEXT("MercenaryChip%dLabel"), Index)));
			PlaceCanvas(DetailSection, Label, ChipPos + FVector2D(38.f, 4.f),
				FVector2D(145.f, 58.f), 9);
			Label->SetText(FText::FromString(ChipLabels[Index]));
			SetReadableFont(Label, BaseFont, 27);
			Label->SetJustification(ETextJustify::Left);

			UTextBlock* Value = FindOrCreate<UTextBlock>(Blueprint, FName(ChipValueNames[Index]));
			PlaceCanvas(DetailSection, Value, ChipPos + FVector2D(205.f, 4.f),
				FVector2D(360.f, 58.f), 9);
			Value->SetText(FText::FromString(TEXT("-")));
			SetReadableFont(Value, BaseFont, 30);
		}

		UImage* CritPlate = FindOrCreate<UImage>(Blueprint, TEXT("MercenaryCritPlate"));
		PlaceCanvas(DetailSection, CritPlate, FVector2D(1080.f, 576.f),
			FVector2D(620.f, 68.f), 8);
		CritPlate->SetBrushFromTexture(DescriptionPlateTexture, false);
		CritPlate->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UTextBlock* CritLabel = FindOrCreate<UTextBlock>(Blueprint, TEXT("MercenaryCritLabel"));
		PlaceCanvas(DetailSection, CritLabel, FVector2D(1118.f, 580.f),
			FVector2D(145.f, 58.f), 9);
		CritLabel->SetText(NSLOCTEXT("CombatHUD", "MercenaryCrit", "치명타"));
		SetReadableFont(CritLabel, BaseFont, 27);
		CritLabel->SetJustification(ETextJustify::Left);
		UTextBlock* CritValue = FindOrCreate<UTextBlock>(Blueprint, TEXT("MercenaryCritValue"));
		PlaceCanvas(DetailSection, CritValue, FVector2D(1285.f, 580.f),
			FVector2D(360.f, 58.f), 9);
		CritValue->SetText(FText::FromString(TEXT("-")));
		SetReadableFont(CritValue, BaseFont, 30);

		UTextBlock* SkillHeading = FindOrCreate<UTextBlock>(Blueprint,
			TEXT("MercenarySkillHeading"));
		PlaceCanvas(DetailSection, SkillHeading, FVector2D(650.f, 670.f),
			FVector2D(980.f, 46.f), 8);
		SkillHeading->SetText(NSLOCTEXT("CombatHUD", "MercenarySkills", "스킬"));
		SetReadableFont(SkillHeading, BaseFont, 32);
		SkillHeading->SetJustification(ETextJustify::Left);

		for (int32 Index = 0; Index < 6; ++Index)
		{
			const float X = 650.f + 164.f * Index;
			const float Y = 730.f;
			UImage* Frame = FindOrCreate<UImage>(Blueprint, FName(*FString::Printf(
				TEXT("MercenarySkillFrame_%d"), Index)));
			PlaceCanvas(DetailSection, Frame, FVector2D(X, Y), FVector2D(126.f, 126.f), 8);
			Frame->SetBrushFromTexture(SkillFrameTexture, false);
			Frame->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			// 그림 자리는 칸 그림에서 **잰 구멍**이다. (39,32)+108 은 눈대중이라
			// 아이콘이 칸 안에서 위로 치우쳐 있었다.
			const FBox2D CellInner = UIPartRects::Inner(TEXT("T_KitA_Cell_Normal"),
				FVector2D(X, Y), FVector2D(126.f, 126.f), false);
			UImage* Icon = FindOrCreate<UImage>(Blueprint, FName(*FString::Printf(
				TEXT("MercenarySkillIcon_%d"), Index)));
			PlaceCanvas(DetailSection, Icon, CellInner.Min, CellInner.GetSize(), 9);
			Icon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			UTextBlock* Name = FindOrCreate<UTextBlock>(Blueprint, FName(*FString::Printf(
				TEXT("MercenarySkillName_%d"), Index)));
			// 그림이 있으면 칸은 그림으로 말한다(런타임이 정한다). 이름은
			// 그림이 없을 때만 뜨므로 칸 밖이 아니라 칸 **안**에 겹쳐 둔다.
			PlaceCanvas(DetailSection, Name, CellInner.Min, CellInner.GetSize(), 10);
			Name->SetText(Index == 0
				? NSLOCTEXT("CombatHUD", "MercenaryMovePreview", "이동")
				: FText::FromString(FString::Printf(TEXT("스킬 %d"), Index)));
			SetReadableFont(Name, BaseFont, 18);

			UTextBlock* Cost = FindOrCreate<UTextBlock>(Blueprint, FName(*FString::Printf(
				TEXT("MercenarySkillCost_%d"), Index)));
			PlaceCanvas(DetailSection, Cost,
				FVector2D(CellInner.Max.X - 44.f, CellInner.Min.Y - 6.f),
				FVector2D(44.f, 44.f), 11);
			Cost->SetText(FText::AsNumber(Index == 0 ? 1 : 0));
			SetReadableFont(Cost, BaseFont, 20);

			// Frame/Icon만 있으면 런타임이 OnClicked를 연결할 대상이 없다.
			// 시각 부품 전체와 같은 크기의 투명 버튼을 가장 위에 둔다.
			UButton* Button = FindOrCreate<UButton>(Blueprint, FName(*FString::Printf(
				TEXT("MercenarySkillButton_%d"), Index)));
			PlaceCanvas(DetailSection, Button, FVector2D(X, Y),
				FVector2D(126.f, 126.f), 20);
			SetInvisibleButtonChrome(Button);
		}
	}

	void Build()
	{
		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, AssetPath);
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("RD_COMBAT_HUD_BUILD missing asset %s"), AssetPath);
			return;
		}
		Blueprint->Modify();
		Blueprint->WidgetTree->Modify();

		UCanvasPanel* Objective = CastChecked<UCanvasPanel>(
			Blueprint->WidgetTree->FindWidget(TEXT("ObjectivePanel")));
		UTextBlock* RoundText = CastChecked<UTextBlock>(
			Blueprint->WidgetTree->FindWidget(TEXT("RoundText")));
		const FSlateFontInfo BaseFont = RoundText->GetFont();

		UTexture2D* MercenaryTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_OptionsIcon_MercenaryGlyph.T_MB_OptionsIcon_MercenaryGlyph"));
		UTexture2D* MonsterTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_OptionsIcon_MonsterGlyph.T_MB_OptionsIcon_MonsterGlyph"));
		UTexture2D* SpeedTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_Icon_Speed.T_MB_Icon_Speed"));
		UTexture2D* TurnTokenFrameTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_TurnToken_Frame.T_MB_TurnToken_Frame"));
		UTexture2D* OptionsRailFrameTexture = LoadObject<UTexture2D>(nullptr,
			OptionsRailFrameTexturePath);
		UTexture2D* MapTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_OptionsIcon_Map.T_MB_OptionsIcon_Map"));
		UTexture2D* SettingsTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_OptionsIcon_Settings.T_MB_OptionsIcon_Settings"));
		UTexture2D* ArtifactSlotTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_ArtifactSlot_Frame.T_MB_ArtifactSlot_Frame"));
		UTexture2D* RoundBadgeTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_RoundBadge_Frame.T_MB_RoundBadge_Frame"));
		UTexture2D* ActionButtonTexture = LoadObject<UTexture2D>(nullptr,
			ActionButtonTexturePath);
		UTexture2D* MercenaryShellTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_Frame_Outer.T_KitA_Frame_Outer"));
		UTexture2D* MercenaryCardNormalTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_MercenaryCard_Normal.T_MB_MercenaryCard_Normal"));
		UTexture2D* MercenaryCardSelectedTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_MercenaryCard_Selected.T_MB_MercenaryCard_Selected"));
		// 뒤로 단추와 스킬 칸을 공용 KitA 부품으로 모은다. 같은 기능인데 화면마다
		// 다른 그림을 쓰고 있었다(0804 검수).
		UTexture2D* BackButtonTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_Button_Small_Normal.T_KitA_Button_Small_Normal"));
		UTexture2D* MercenarySkillFrameTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_Cell_Normal.T_KitA_Cell_Normal"));
		UTexture2D* MercenaryPortraitFrameTexture = LoadObject<UTexture2D>(nullptr,
			MercenaryPortraitFrameTexturePath);
		UTexture2D* MercenaryDetailPortraitCellTexture = LoadObject<UTexture2D>(
			nullptr, DetailPortraitCellTexturePath);
		UTexture2D* InventoryIconTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/UI/KayKit/KK_Icon_Inventory.KK_Icon_Inventory"));
		UTexture2D* MercenaryGoldIconTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Reward/T_Reward_GoldIcon_V1.T_Reward_GoldIcon_V1"));
		UTexture2D* MercenaryDescriptionPlateTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_Row_Plate.T_KitA_Row_Plate"));
		UTexture2D* EnemyPanelTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Common/T_MB_GenericDetailPanel.T_MB_GenericDetailPanel"));
		UTexture2D* StatusSlotTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_StatusSlot_Frame.T_MB_StatusSlot_Frame"));
		if (MercenaryTexture == nullptr || MonsterTexture == nullptr
			|| SpeedTexture == nullptr || TurnTokenFrameTexture == nullptr
			|| OptionsRailFrameTexture == nullptr || MapTexture == nullptr
			|| SettingsTexture == nullptr || ArtifactSlotTexture == nullptr
			|| RoundBadgeTexture == nullptr || ActionButtonTexture == nullptr
			|| MercenaryShellTexture == nullptr
			|| MercenaryCardNormalTexture == nullptr
			|| MercenaryCardSelectedTexture == nullptr || BackButtonTexture == nullptr
			|| MercenarySkillFrameTexture == nullptr
			|| MercenaryPortraitFrameTexture == nullptr
			|| MercenaryDetailPortraitCellTexture == nullptr
			|| InventoryIconTexture == nullptr
			|| MercenaryGoldIconTexture == nullptr
			|| MercenaryDescriptionPlateTexture == nullptr
			|| EnemyPanelTexture == nullptr || StatusSlotTexture == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("RD_COMBAT_HUD_BUILD HUD textures missing"));
			return;
		}

		if (UWidget* ObjectivePlate = Blueprint->WidgetTree->FindWidget(TEXT("ObjectivePlate")))
		{
			ObjectivePlate->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (UWidget* TurnPlate = Blueprint->WidgetTree->FindWidget(TEXT("TurnPlate")))
		{
			TurnPlate->SetVisibility(ESlateVisibility::Collapsed);
		}
		/*
		 * 용병 패널은 자기 판에 짓고, HUD 에는 그 판을 **하나 얹기만** 한다.
		 * HUD 판에 그대로 두면 화면 전체짜리가 셋 포개져 UMG 에서 못 고친다.
		 */
		if (UWidgetBlueprint* MercenaryBlueprint = EnsureBlueprint(
			MercenaryAssetPath, MercenaryPackagePath, MercenaryAssetName))
		{
			MercenaryBlueprint->Modify();
			MercenaryBlueprint->WidgetTree->Modify();
			BuildMercenaryPanel(MercenaryBlueprint, BaseFont, MercenaryShellTexture,
				MercenaryCardNormalTexture, MercenaryCardSelectedTexture,
				BackButtonTexture, MercenarySkillFrameTexture,
				MercenaryDetailPortraitCellTexture,
				InventoryIconTexture, MercenaryGoldIconTexture,
				MercenaryDescriptionPlateTexture);
			PruneStaleVariables(MercenaryBlueprint);
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(MercenaryBlueprint);
			FKismetEditorUtilities::CompileBlueprint(MercenaryBlueprint);
			UPackage::SavePackage(MercenaryBlueprint->GetPackage(), MercenaryBlueprint,
				*FPackageName::LongPackageNameToFilename(
					MercenaryBlueprint->GetOutermost()->GetName(),
					FPackageName::GetAssetPackageExtension()),
				FSavePackageArgs());

			// HUD 에는 그 판을 얹는다. 입력은 안 먹고, 안의 MercenaryPanel 캔버스가
			// 접혀 있다가 런타임에 펴진다.
			if (UClass* PanelClass = MercenaryBlueprint->GeneratedClass)
			{
				UUserWidget* Host = Cast<UUserWidget>(
					Blueprint->WidgetTree->FindWidget(TEXT("MercenaryPanelHost")));
				if (Host == nullptr)
				{
					Host = Blueprint->WidgetTree->ConstructWidget<UUserWidget>(
						PanelClass, TEXT("MercenaryPanelHost"));
					Blueprint->OnVariableAdded(TEXT("MercenaryPanelHost"));
				}
				UCanvasPanel* HudRoot =
					CastChecked<UCanvasPanel>(Blueprint->WidgetTree->RootWidget);
				PlaceCanvas(HudRoot, Host, FVector2D::ZeroVector,
					FVector2D(1920.f, 1080.f), 10000);
				if (UCanvasPanelSlot* HostSlot = Cast<UCanvasPanelSlot>(Host->Slot))
				{
					HostSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
					HostSlot->SetOffsets(FMargin(0.f));
				}
				Host->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}
		}
		// 인라인 판은 디자이너가 손으로 정렬한 최종본이다. 전체 빌더의 옛
		// 좌표로 다시 굽지 않는다. 판 자체가 없는 새 자산에서만 초기 생성한다.
		if (Blueprint->WidgetTree->FindWidget(TEXT("MercenaryPanel")) == nullptr)
		{
			BuildMercenaryPanel(Blueprint, BaseFont, MercenaryShellTexture,
				MercenaryCardNormalTexture, MercenaryCardSelectedTexture,
				BackButtonTexture, MercenarySkillFrameTexture,
				MercenaryDetailPortraitCellTexture,
				InventoryIconTexture, MercenaryGoldIconTexture,
				MercenaryDescriptionPlateTexture);
		}
		if (UWidget* Host = Blueprint->WidgetTree->FindWidget(TEXT("MercenaryPanelHost")))
		{
			// 중첩 WBP는 디자이너 비교용으로만 남기고 런타임에서는 인라인 판 하나만 쓴다.
			Host->SetVisibility(ESlateVisibility::Collapsed);
		}
		RepairAuthoredRoundTurnLayout(Blueprint, RoundBadgeTexture,
			TurnTokenFrameTexture, SpeedTexture);
		RepairAuthoredActionButtonArt(Blueprint, ActionButtonTexture);
		// EnemyPanel/AllyPanel은 현재 WBP에서 디자이너가 직접 관리한다. 이 함수의
		// Build*Summary 정의는 과거 배치라 실행하면 최신 요약판을 되돌린다.
		// 전체 빌더에서도 기존 WBP 속성을 보존한다.

		UImage* OptionsRailFrame = FindOrCreate<UImage>(Blueprint, TEXT("OptionsRailFrame"));
		PlaceCanvas(Objective, OptionsRailFrame, FVector2D::ZeroVector,
			FVector2D(470.f, 173.f), 1);
		OptionsRailFrame->SetBrushFromTexture(OptionsRailFrameTexture, false);
		OptionsRailFrame->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UImage* MapIcon = FindOrCreate<UImage>(Blueprint, TEXT("MenuMapIcon"));
		PlaceCanvas(Objective, MapIcon, FVector2D(47.f, 47.f), FVector2D(74.f, 74.f), 31);
		MapIcon->SetBrushFromTexture(MapTexture, false);
		MapIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UImage* MercenaryIcon = FindOrCreate<UImage>(Blueprint, TEXT("MenuMercenaryIcon"));
		// 세로형 원본의 종횡비를 유지해 헬멧이 찌그러지지 않게 한다.
		PlaceCanvas(Objective, MercenaryIcon, FVector2D(153.5f, 38.f), FVector2D(63.f, 96.f), 31);
		MercenaryIcon->SetBrushFromTexture(MercenaryTexture, false);
		MercenaryIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UImage* MonsterIcon = FindOrCreate<UImage>(Blueprint, TEXT("MenuMonsterIcon"));
		PlaceCanvas(Objective, MonsterIcon, FVector2D(244.f, 38.f), FVector2D(90.f, 96.f), 31);
		MonsterIcon->SetBrushFromTexture(MonsterTexture, false);
		MonsterIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UImage* SettingsIcon = FindOrCreate<UImage>(Blueprint, TEXT("MenuSettingsIcon"));
		PlaceCanvas(Objective, SettingsIcon, FVector2D(350.f, 47.f), FVector2D(74.f, 74.f), 31);
		SettingsIcon->SetBrushFromTexture(SettingsTexture, false);
		SettingsIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UTextBlock* MercenaryLabel = CastChecked<UTextBlock>(
			Blueprint->WidgetTree->FindWidget(TEXT("MenuMercenaryMaskLabel")));
		MercenaryLabel->SetVisibility(ESlateVisibility::Collapsed);

		UTextBlock* MonsterLabel = CastChecked<UTextBlock>(
			Blueprint->WidgetTree->FindWidget(TEXT("MenuEmptyMaskLabel")));
		MonsterLabel->SetVisibility(ESlateVisibility::Collapsed);
		if (UWidget* MercenaryMask = Blueprint->WidgetTree->FindWidget(TEXT("MenuMercenaryMask")))
		{
			MercenaryMask->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (UWidget* EmptyMask = Blueprint->WidgetTree->FindWidget(TEXT("MenuEmptyMask")))
		{
			EmptyMask->SetVisibility(ESlateVisibility::Collapsed);
		}

		if (UButton* MonsterButton = Cast<UButton>(
			Blueprint->WidgetTree->FindWidget(TEXT("MenuButton_2"))))
		{
			MonsterButton->SetIsEnabled(true);
		}
		const FVector2D MenuButtonPositions[] = {
			FVector2D(37.f, 31.f), FVector2D(138.f, 31.f),
			FVector2D(242.f, 31.f), FVector2D(343.f, 31.f),
		};
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(MenuButtonPositions); ++Index)
		{
			if (UButton* MenuButton = Cast<UButton>(Blueprint->WidgetTree->FindWidget(
				FName(*FString::Printf(TEXT("MenuButton_%d"), Index)))))
			{
				PlaceCanvas(Objective, MenuButton, MenuButtonPositions[Index],
					FVector2D(94.f, 112.f), 40);
			}
		}
		// 위의 옛 생성 블록이 Overlay 계보를 평탄화하므로 전체 빌드의 마지막
		// 값은 반드시 전용 surgical helper가 확정한다.
		RepairAuthoredRightHUDLayout(Blueprint, OptionsRailFrameTexture);

		if (UWidget* ArtifactTrayFrame =
			Blueprint->WidgetTree->FindWidget(TEXT("ArtifactTrayFrame")))
		{
			Blueprint->WidgetTree->RemoveWidget(ArtifactTrayFrame);
		}
		if (UCanvasPanel* ArtifactStrip = Cast<UCanvasPanel>(
			Blueprint->WidgetTree->FindWidget(TEXT("ArtifactStrip"))))
		{
			ArtifactStrip->SetVisibility(ESlateVisibility::Collapsed);
			if (UCanvasPanelSlot* ArtifactSlot = Cast<UCanvasPanelSlot>(ArtifactStrip->Slot))
			{
				ArtifactSlot->SetPosition(FVector2D(18.f, -286.f));
				ArtifactSlot->SetSize(FVector2D(680.f, 116.f));
			}
			for (int32 Index = 0; Index < 6; ++Index)
			{
				if (UImage* ArtifactFrame = Cast<UImage>(Blueprint->WidgetTree->FindWidget(
					FName(*FString::Printf(TEXT("ArtifactFrame_%d"), Index)))))
				{
					PlaceCanvas(ArtifactStrip, ArtifactFrame,
						FVector2D(4.f + 112.f * Index, 4.f), FVector2D(108.f, 108.f), 1);
					ArtifactFrame->SetBrushFromTexture(ArtifactSlotTexture, false);
					ArtifactFrame->SetVisibility(ESlateVisibility::Collapsed);
				}
				if (UImage* ArtifactIcon = Cast<UImage>(Blueprint->WidgetTree->FindWidget(
					FName(*FString::Printf(TEXT("ArtifactIcon_%d"), Index)))))
				{
					PlaceCanvas(ArtifactStrip, ArtifactIcon,
						FVector2D(13.f + 112.f * Index, 13.f), FVector2D(90.f, 90.f), 2);
				}
			}
			// 전투 화면의 독립 아티팩트 줄은 더 이상 사용하지 않는다.
			Blueprint->WidgetTree->RemoveWidget(ArtifactStrip);
		}

		PruneStaleVariables(Blueprint);
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		if (!UPackage::SavePackage(Blueprint->GetPackage(), Blueprint,
			*FPackageName::LongPackageNameToFilename(
				Blueprint->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension()),
			FSavePackageArgs()))
		{
			UE_LOG(LogTemp, Error, TEXT("RD_COMBAT_HUD_BUILD save failed"));
			return;
		}
		UE_LOG(LogTemp, Display,
			TEXT("RD_COMBAT_HUD_BUILD success menu_icons=4 turn_frames=10 speed_icons=10"));
	}
}

void RegisterCombatHUDWidgetBuilderCommands()
{
	using namespace CombatHUDWidgetBuilder;
	BuildCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.BuildCombatHUDAdditions"),
		TEXT("Add Marchbound combat tab icons and turn speed widgets."),
		FConsoleCommandDelegate::CreateStatic(&Build));
	InventoryBuildCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.BuildCombatHUDInventoryTab"),
		TEXT("Add only the WBP-authored inventory tab below the mercenary roster."),
		FConsoleCommandDelegate::CreateStatic(&BuildInventoryTabOnly));
	RoundTurnRepairCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.RepairCombatHUDRoundTurn"),
		TEXT("Restore the authored 0811 ROUND/turn bar and #519 action button art."),
		FConsoleCommandDelegate::CreateStatic(&RepairRoundTurnOnly));
	ActionButtonArtRepairCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.RepairCombatHUDActionButtonArt"),
		TEXT("Restore only the #519 Skill and End Turn button texture references."),
		FConsoleCommandDelegate::CreateStatic(&RepairActionButtonArtOnly));
	RightHUDRepairCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.RepairCombatHUDRightLayout"),
		TEXT("Restore only the authored 0811 options, summaries, and action buttons."),
		FConsoleCommandDelegate::CreateStatic(&RepairRightHUDOnly));
	MercenaryPortraitFrameRepairCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.RepairCombatHUDMercenaryPortraitFrame"),
		TEXT("Restore only the exact KitA portrait frame in inline and modular mercenary panels."),
		FConsoleCommandDelegate::CreateStatic(&RepairMercenaryPortraitFrameOnly));
}

void UnregisterCombatHUDWidgetBuilderCommands()
{
	CombatHUDWidgetBuilder::BuildCommand.Reset();
	CombatHUDWidgetBuilder::InventoryBuildCommand.Reset();
	CombatHUDWidgetBuilder::RoundTurnRepairCommand.Reset();
	CombatHUDWidgetBuilder::ActionButtonArtRepairCommand.Reset();
	CombatHUDWidgetBuilder::RightHUDRepairCommand.Reset();
	CombatHUDWidgetBuilder::MercenaryPortraitFrameRepairCommand.Reset();
}
