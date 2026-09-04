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
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "HAL/IConsoleManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UI/RunOptionsRailWidget.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprintEditorUtils.h"

namespace CombatHUDWidgetBuilder
{
	constexpr TCHAR AssetPath[] =
		TEXT("/Game/UI/CombatLayouts/WBP_CombatHUD04.WBP_CombatHUD04");
	constexpr TCHAR DetailOverlayAssetPath[] =
		TEXT("/Game/UI/CombatDetail/WBP_CombatDetailOverlay.WBP_CombatDetailOverlay");
	constexpr TCHAR ActionButtonTexturePath[] =
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/HUD04/T_Combat_Button_Wood_SkillConfirm_20260811_v3.T_Combat_Button_Wood_SkillConfirm_20260811_v3");
	constexpr TCHAR OptionsRailFrameTexturePath[] =
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_OptionsRail_Frame.T_MB_OptionsRail_Frame");
	constexpr TCHAR MercenaryPortraitFrameTexturePath[] =
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_Portrait_Frame.T_KitA_Portrait_Frame");
	constexpr TCHAR DetailPortraitCellTexturePath[] =
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_Cell_Normal.T_KitA_Cell_Normal");
	constexpr TCHAR SummaryVerticalTexturePath[] =
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/HUD04/T_SummaryVerticalReferenceStyle_v5.T_SummaryVerticalReferenceStyle_v5");
	constexpr TCHAR CriticalIconTexturePath[] =
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatDetail/SkillTactical/T_SkillStat_Critical_Clear_v1.T_SkillStat_Critical_Clear_v1");
	constexpr TCHAR APGemFlashMaterialPath[] =
		TEXT("/Game/UI/CombatLayouts/M_APGemFlash.M_APGemFlash");
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
	TUniquePtr<FAutoConsoleCommand> DetailResponsiveRepairCommand;
	TUniquePtr<FAutoConsoleCommand> WidgetTreeContractRepairCommand;
	TUniquePtr<FAutoConsoleCommand> APPlacementRepairCommand;
	TUniquePtr<FAutoConsoleCommand> StatusScrollRepairCommand;
	TUniquePtr<FAutoConsoleCommand> SlotDumpCommand;

	UTexture2D* EnsureSummaryVerticalTexture()
	{
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, SummaryVerticalTexturePath);
		checkf(Texture != nullptr, TEXT("Missing SVN vertical summary texture: %s"),
			SummaryVerticalTexturePath);
		return Texture;
	}

	UTexture2D* EnsureCriticalIconTexture()
	{
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, CriticalIconTexturePath);
		checkf(Texture != nullptr, TEXT("Missing SVN critical icon texture: %s"),
			CriticalIconTexturePath);
		return Texture;
	}

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

	/** WidgetTree에서 떼는 데 그치지 않고 변수/GUID와 UObject까지 완전히 삭제한다. */
	int32 DeleteWidgetsCompletely(UWidgetBlueprint* Blueprint,
		const TSet<UWidget*>& Widgets)
	{
		if (Blueprint == nullptr || Widgets.IsEmpty())
		{
			return 0;
		}

		// 부모를 지우면 자식까지 정리되므로 선택 집합에서 가장 바깥 뿌리만
		// 넘긴다. 이미 부모를 잃은 이전 빌더의 orphan도 각각 뿌리로 정리된다.
		TSet<UWidget*> Roots;
		for (UWidget* Widget : Widgets)
		{
			if (Widget == nullptr)
			{
				continue;
			}
			bool bHasSelectedAncestor = false;
			for (UWidget* Parent = Widget->GetParent(); Parent != nullptr;
				Parent = Parent->GetParent())
			{
				if (Widgets.Contains(Parent))
				{
					bHasSelectedAncestor = true;
					break;
				}
			}
			if (bHasSelectedAncestor == false)
			{
				Roots.Add(Widget);
			}
		}

		FWidgetBlueprintEditorUtils::DeleteWidgets(Blueprint, MoveTemp(Roots),
			FWidgetBlueprintEditorUtils::EDeleteWidgetWarningType::DeleteSilently);
		return Widgets.Num();
	}

	/** @brief 합의에서 빠진 턴 속도 행은 숨기지 않고 WBP 트리에서 완전히 지운다. */
	int32 RemoveTurnSpeedWidgets(UWidgetBlueprint* Blueprint)
	{
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
		{
			return 0;
		}
		int32 Removed = 0;
		for (int32 Index = 0; Index < 10; ++Index)
		{
			const FString Tail = FString::Printf(TEXT("_%d"), Index);
			// 자식을 먼저 지워 부모 제거 시 고아 위젯이 남지 않게 한다.
			for (const FName Name : {
				FName(TEXT("TurnSpeed") + Tail),
				FName(TEXT("TurnSpeed") + Tail + TEXT("_Center")),
				FName(TEXT("TurnSpeedIcon") + Tail),
				FName(TEXT("TurnSpeedPlate") + Tail) })
			{
				if (UWidget* Widget = Blueprint->WidgetTree->FindWidget(Name))
				{
					Blueprint->WidgetTree->RemoveWidget(Widget);
					++Removed;
				}
			}
		}
		return Removed;
	}

	/** @brief 채택되지 않은 중앙 하단 퀵 스킬 바를 트리와 변수 계약에서 지운다. */
	int32 RemoveRetiredQuickSkillWidgets(UWidgetBlueprint* Blueprint)
	{
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
		{
			return 0;
		}

		TSet<UWidget*> RetiredWidgets;
		// root traversal에 잡히지 않는 이전 빌더 orphan도 함께 걷는다.
		for (UWidget* Widget : Blueprint->GetAllSourceWidgets())
		{
			if (Widget == nullptr)
			{
				continue;
			}
			const FString WidgetName = Widget->GetName();
			if (WidgetName == TEXT("QuickSkillBar"))
			{
				RetiredWidgets.Add(Widget);
				continue;
			}
			for (const TCHAR* Prefix : { TEXT("QuickSkillButton_"),
				TEXT("QuickSkillCooldown_"), TEXT("QuickSkillIcon_"),
				TEXT("QuickSkillFrame_"), TEXT("QuickSkillSlot_") })
			{
				if (WidgetName.StartsWith(Prefix))
				{
					RetiredWidgets.Add(Widget);
					break;
				}
			}
		}
		return DeleteWidgetsCompletely(Blueprint, RetiredWidgets);
	}

	/**
	 * @brief Xxx_Center를 채우는 AutoFit 안에서 동명 TextBlock을 중앙 정렬한다.
	 *
	 * TextBlock 자체를 세로로 Fill하면 폰트의 ascent/descent 기준선 때문에 실제
	 * 글리프가 아래로 처져 보인다. 대신 Xxx_AutoFit ScaleBox가 Center 전체를
	 * 차지하고 TextBlock은 desired size로 중앙에 둔다. 가로가 넘칠 때만
	 * ScaleToFitX/DownOnly가 작동하므로 짧은 글자는 원래 크기를 유지한다.
	 * 버튼별 Padding이나 RenderTranslation 같은 좌표 보정은 사용하지 않는다.
	 */
	int32 NormalizeCenteredTextBounds(UWidgetBlueprint* Blueprint)
	{
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
		{
			return 0;
		}
		TArray<UTextBlock*> TextBlocks;
		Blueprint->WidgetTree->ForEachWidget([&TextBlocks](UWidget* Widget)
		{
			if (UTextBlock* Text = Cast<UTextBlock>(Widget))
			{
				TextBlocks.Add(Text);
			}
		});

		int32 Normalized = 0;
		for (UTextBlock* Text : TextBlocks)
		{
			const FName CenterName(*FString::Printf(TEXT("%s_Center"),
				*Text->GetName()));
			UOverlay* Center = Cast<UOverlay>(
				Blueprint->WidgetTree->FindWidget(CenterName));
			if (Center == nullptr)
			{
				continue;
			}

			UWidget* CenterChild = Text;
			while (CenterChild != nullptr && CenterChild->GetParent() != Center)
			{
				CenterChild = CenterChild->GetParent();
			}
			if (CenterChild == nullptr)
			{
				continue;
			}

			UScaleBox* AutoFit = Cast<UScaleBox>(CenterChild);
			if (AutoFit == nullptr && CenterChild == Text)
			{
				const int32 ChildIndex = Center->GetChildIndex(Text);
				UPanelSlot* SlotTemplate = Text->Slot != nullptr
					? DuplicateObject<UPanelSlot>(Text->Slot, GetTransientPackage())
					: nullptr;
				if (ChildIndex == INDEX_NONE || SlotTemplate == nullptr)
				{
					continue;
				}

				const FName AutoFitName(*FString::Printf(TEXT("%s_AutoFit"),
					*Text->GetName()));
				AutoFit = Cast<UScaleBox>(
					Blueprint->WidgetTree->FindWidget(AutoFitName));
				if (AutoFit == nullptr)
				{
					AutoFit = Blueprint->WidgetTree->ConstructWidget<UScaleBox>(
						UScaleBox::StaticClass(), AutoFitName);
					Blueprint->OnVariableAdded(AutoFitName);
				}
				else if (UPanelWidget* OldParent = AutoFit->GetParent())
				{
					OldParent->RemoveChild(AutoFit);
				}

				Center->RemoveChildAt(ChildIndex);
				if (Center->InsertChildAt(ChildIndex, AutoFit, SlotTemplate) == nullptr)
				{
					continue;
				}
				AutoFit->SetContent(Text);
				CenterChild = AutoFit;
			}
			if (AutoFit == nullptr)
			{
				// 예상하지 못한 중간 래퍼는 트리를 훼손하지 않는다.
				continue;
			}

			AutoFit->SetStretch(EStretch::ScaleToFitX);
			AutoFit->SetStretchDirection(EStretchDirection::DownOnly);
			// 세로로는 자르지 않는다. 저작된 칸 높이가 글꼴 줄 높이와 비슷하면
			// 아래꼬리와 외곽선이 잘렸다. 가로는 ScaleToFitX 가 이미 막는다.
			AutoFit->SetClipping(EWidgetClipping::Inherit);
			AutoFit->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			if (UOverlaySlot* Slot = Cast<UOverlaySlot>(CenterChild->Slot))
			{
				Slot->SetPadding(FMargin(0.f));
				Slot->SetHorizontalAlignment(HAlign_Fill);
				Slot->SetVerticalAlignment(VAlign_Fill);
			}
			if (UScaleBoxSlot* ScaleTextSlot = Cast<UScaleBoxSlot>(Text->Slot))
			{
				ScaleTextSlot->SetHorizontalAlignment(HAlign_Center);
				ScaleTextSlot->SetVerticalAlignment(VAlign_Center);
			}
			Text->SetMargin(FMargin(0.f));
			Text->SetJustification(ETextJustify::Center);
			Text->SetRenderTransform(FWidgetTransform());
			Text->SetRenderTransformPivot(FVector2D(.5f));
			++Normalized;
		}
		return Normalized;
	}

	/** @brief 공용 상세 WBP의 장식/내용 전체를 한 개 1920x1080 ScaleBox에 묶는다. */
	bool RepairResponsiveDetailTree(UWidgetBlueprint* Blueprint)
	{
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
		{
			return false;
		}
		UCanvasPanel* Root = Cast<UCanvasPanel>(
			Blueprint->WidgetTree->FindWidget(TEXT("DetailPanelRoot")));
		if (Root == nullptr)
		{
			return false;
		}

		if (UScaleBox* ExistingScale = Cast<UScaleBox>(
			Blueprint->WidgetTree->FindWidget(TEXT("DetailResponsiveScale"))))
		{
			ExistingScale->SetStretch(EStretch::ScaleToFit);
			ExistingScale->SetStretchDirection(EStretchDirection::Both);
			ExistingScale->SetClipping(EWidgetClipping::ClipToBoundsAlways);
			if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(ExistingScale->Slot))
			{
				Slot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
				Slot->SetOffsets(FMargin(0.f));
				Slot->SetAlignment(FVector2D::ZeroVector);
				Slot->SetZOrder(10);
			}
			USizeBox* DesignSize = Cast<USizeBox>(Blueprint->WidgetTree->FindWidget(
				TEXT("DetailResponsiveDesignSize")));
			UCanvasPanel* DesignCanvas = Cast<UCanvasPanel>(
				Blueprint->WidgetTree->FindWidget(TEXT("DetailResponsiveCanvas")));
			if (DesignSize == nullptr || DesignCanvas == nullptr)
			{
				return false;
			}
			DesignSize->SetWidthOverride(1920.f);
			DesignSize->SetHeightOverride(1080.f);
			// 배경 암막은 디자인 판과 함께 축소하면 레터박스가 투명해진다.
			// 이미 1차 수리된 에셋도 다시 실행하면 전 화면 루트로 복구한다.
			for (const FName ScrimName : { FName(TEXT("DetailScrimBg")),
				FName(TEXT("DetailScrimImage")) })
			{
				UWidget* Scrim = Blueprint->WidgetTree->FindWidget(ScrimName);
				if (Scrim == nullptr || Scrim->GetParent() == Root)
				{
					continue;
				}
				Scrim->GetParent()->RemoveChild(Scrim);
				if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(Scrim))
				{
					Slot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
					Slot->SetOffsets(FMargin(0.f));
					Slot->SetAlignment(FVector2D::ZeroVector);
					Slot->SetZOrder(0);
				}
			}
			return true;
		}

		struct FAuthoredCanvasLayout
		{
			UWidget* Widget = nullptr;
			FAnchors Anchors;
			FMargin Offsets;
			FVector2D Alignment = FVector2D::ZeroVector;
			bool bAutoSize = false;
			int32 ZOrder = 0;
		};
		TArray<FAuthoredCanvasLayout> ToMove;
		for (int32 Index = 0; Index < Root->GetChildrenCount(); ++Index)
		{
			UWidget* Child = Root->GetChildAt(Index);
			if (Child == nullptr || Child->GetFName() == TEXT("DetailScrimBg")
				|| Child->GetFName() == TEXT("DetailScrimImage")
				|| Child->GetFName() == TEXT("DetailCloseCatch"))
			{
				continue;
			}
			UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Child->Slot);
			if (Slot == nullptr)
			{
				continue;
			}
			FAuthoredCanvasLayout& Layout = ToMove.AddDefaulted_GetRef();
			Layout.Widget = Child;
			Layout.Anchors = Slot->GetAnchors();
			Layout.Offsets = Slot->GetOffsets();
			Layout.Alignment = Slot->GetAlignment();
			Layout.bAutoSize = Slot->GetAutoSize();
			Layout.ZOrder = Slot->GetZOrder();
		}
		for (const FAuthoredCanvasLayout& Layout : ToMove)
		{
			Root->RemoveChild(Layout.Widget);
		}

		UScaleBox* ResponsiveScale = Blueprint->WidgetTree->ConstructWidget<UScaleBox>(
			UScaleBox::StaticClass(), TEXT("DetailResponsiveScale"));
		ResponsiveScale->SetStretch(EStretch::ScaleToFit);
		ResponsiveScale->SetStretchDirection(EStretchDirection::Both);
		ResponsiveScale->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		ResponsiveScale->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		if (UCanvasPanelSlot* ScaleSlot = Root->AddChildToCanvas(ResponsiveScale))
		{
			ScaleSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			ScaleSlot->SetOffsets(FMargin(0.f));
			ScaleSlot->SetAlignment(FVector2D::ZeroVector);
			ScaleSlot->SetZOrder(10);
		}

		USizeBox* DesignSize = Blueprint->WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("DetailResponsiveDesignSize"));
		DesignSize->SetWidthOverride(1920.f);
		DesignSize->SetHeightOverride(1080.f);
		ResponsiveScale->AddChild(DesignSize);
		if (UScaleBoxSlot* Slot = Cast<UScaleBoxSlot>(DesignSize->Slot))
		{
			Slot->SetHorizontalAlignment(HAlign_Center);
			Slot->SetVerticalAlignment(VAlign_Center);
		}

		UCanvasPanel* DesignCanvas = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("DetailResponsiveCanvas"));
		DesignSize->SetContent(DesignCanvas);
		for (const FAuthoredCanvasLayout& Layout : ToMove)
		{
			if (UCanvasPanelSlot* Slot = DesignCanvas->AddChildToCanvas(Layout.Widget))
			{
				Slot->SetAnchors(Layout.Anchors);
				Slot->SetOffsets(Layout.Offsets);
				Slot->SetAlignment(Layout.Alignment);
				Slot->SetAutoSize(Layout.bAutoSize);
				Slot->SetZOrder(Layout.ZOrder);
			}
		}
		return ToMove.Num() > 0;
	}

	void SetOverlayLayout(UWidget* Widget, const FMargin Padding,
		const EHorizontalAlignment Horizontal, const EVerticalAlignment Vertical)
	{
		UOverlaySlot* Slot = CastChecked<UOverlaySlot>(Widget->Slot);
		Slot->SetPadding(Padding);
		Slot->SetHorizontalAlignment(Horizontal);
		Slot->SetVerticalAlignment(Vertical);
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
	 * @brief #567 크기 조정본의 ROUND 배지와 턴 카드 줄을 WBP에 다시 굽는다.
	 *
	 * @details
	 * ROUND는 별도 HUD 구석 장식이 아니라 턴 카드 줄의 첫 칸이다. 둘을 같은
	 * 좌상단 기준으로 연달아 놓고, 카드 안의 중앙정렬 래퍼도 그대로 보존한다.
	 * 전체 빌더와 전용 복구 명령이 반드시 이 함수 하나를 써야 좌표가 다시
	 * 갈라지지 않는다.
	 */
	void RepairAuthoredRoundTurnLayout(UWidgetBlueprint* Blueprint,
		UTexture2D* RoundBadgeTexture, UTexture2D* TurnTokenFrameTexture)
	{
		check(Blueprint != nullptr && Blueprint->WidgetTree != nullptr);
		check(RoundBadgeTexture != nullptr && TurnTokenFrameTexture != nullptr);
		RemoveTurnSpeedWidgets(Blueprint);

		UCanvasPanel* Root = CastChecked<UCanvasPanel>(
			Blueprint->WidgetTree->RootWidget);
		UCanvasPanel* RoundPanel = CastChecked<UCanvasPanel>(
			Blueprint->WidgetTree->FindWidget(TEXT("RoundPanel")));
		PlaceCanvas(Root, RoundPanel, FVector2D(18.f, 10.f),
			FVector2D(218.f, 136.f), 100);
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
		// 0823 확정: 배지는 "ROUND" 글자만. 라운드 수는 아래 숫자 칸이 맡는다.
		RoundText->SetText(NSLOCTEXT("CombatHUD", "RoundPreview", "ROUND"));
		RoundText->SetJustification(ETextJustify::Center);
		RoundText->SetColorAndOpacity(FSlateColor(
			FLinearColor(.973f, .973f, .953f, 1.f)));
		RoundText->SetShadowOffset(FVector2D(2.f, 3.f));
		RoundText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, .55f));
		RoundText->SetAutoWrapText(false);
		RoundText->SetMinDesiredWidth(0.f);
		RoundText->SetVisibility(ESlateVisibility::HitTestInvisible);
		// 같은 배지 에셋을 바로 아래에 한 장 더 붙이고 두 자리 라운드를 넣는다.
		// 런타임 RefreshTurnOrder 의 %02d 와 짝이다.
		UOverlay* RoundNumberPlateMount = FindOrCreate<UOverlay>(
			Blueprint, TEXT("RoundNumberPlateMount"));
		PlaceCanvas(RoundPanel, RoundNumberPlateMount, FVector2D(0.f, 68.f),
			FVector2D(218.f, 68.f), 5);
		RoundNumberPlateMount->SetVisibility(
			ESlateVisibility::SelfHitTestInvisible);

		UImage* RoundNumberPlate = FindOrCreate<UImage>(
			Blueprint, TEXT("RoundNumberPlate"));
		EnsureParent(RoundNumberPlateMount, RoundNumberPlate);
		FSlateBrush RoundNumberBrush = RoundNumberPlate->GetBrush();
		RoundNumberBrush.SetResourceObject(RoundBadgeTexture);
		RoundNumberBrush.DrawAs = ESlateBrushDrawType::Image;
		RoundNumberBrush.Margin = FMargin(0.f);
		RoundNumberPlate->SetBrush(RoundNumberBrush);
		RoundNumberPlate->SetColorAndOpacity(FLinearColor::White);
		RoundNumberPlate->SetVisibility(ESlateVisibility::HitTestInvisible);
		SetOverlayLayout(RoundNumberPlate, FMargin(0.f), HAlign_Fill, VAlign_Fill);

		UOverlay* RoundNumberTextCenter = FindOrCreate<UOverlay>(
			Blueprint, TEXT("RoundNumberText_Center"));
		EnsureParent(RoundNumberPlateMount, RoundNumberTextCenter);
		RoundNumberTextCenter->SetVisibility(ESlateVisibility::HitTestInvisible);
		SetOverlayLayout(RoundNumberTextCenter, FMargin(22.f, 8.f, 22.f, 8.f),
			HAlign_Fill, VAlign_Fill);

		UTextBlock* RoundNumberText = FindOrCreate<UTextBlock>(
			Blueprint, TEXT("RoundNumberText"));
		EnsureParent(RoundNumberTextCenter, RoundNumberText);
		SetOverlayLayout(RoundNumberText, FMargin(0.f, -3.f, 0.f, 0.f),
			HAlign_Center, VAlign_Center);
		FSlateFontInfo RoundNumberFont
			= UIFont::MakeProjectExact(RoundNumberText->GetFont(), 36);
		RoundNumberFont.LetterSpacing = 0;
		RoundNumberFont.OutlineSettings.OutlineSize = 2;
		RoundNumberFont.OutlineSettings.OutlineColor = FLinearColor::Black;
		RoundNumberText->SetFont(RoundNumberFont);
		RoundNumberText->SetText(
			NSLOCTEXT("CombatHUD", "RoundNumberPreview", "01"));
		RoundNumberText->SetJustification(ETextJustify::Center);
		RoundNumberText->SetColorAndOpacity(FSlateColor(
			FLinearColor(.973f, .973f, .953f, 1.f)));
		RoundNumberText->SetShadowOffset(FVector2D(2.f, 3.f));
		RoundNumberText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, .55f));
		RoundNumberText->SetVisibility(ESlateVisibility::HitTestInvisible);

		// 6f 자산에만 남은 구형 임무 문구다. 런타임 이름 계약은 유지하되,
		// canonical 0811에는 없는 행이므로 ROUND 배지 아래에서 그리지 않는다.
		if (UWidget* ObjectiveTextCenter = Blueprint->WidgetTree->FindWidget(
			TEXT("ObjectiveText_Center")))
		{
			ObjectiveTextCenter->SetVisibility(ESlateVisibility::Collapsed);
		}

		UCanvasPanel* TurnPanel = CastChecked<UCanvasPanel>(
			Blueprint->WidgetTree->FindWidget(TEXT("TurnPanel")));
		PlaceCanvas(Root, TurnPanel, FVector2D(246.f, 10.f),
			FVector2D(1090.f, 150.f), 90);
		TurnPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		for (int32 Index = 0; Index < 10; ++Index)
		{
			const float X = 5.f + 109.f * Index;
			const FString Tail = FString::Printf(TEXT("_%d"), Index);

			UCanvasPanel* Token = CastChecked<UCanvasPanel>(
				Blueprint->WidgetTree->FindWidget(FName(TEXT("TurnToken") + Tail)));
			PlaceCanvas(TurnPanel, Token, FVector2D(X, 30.f),
				FVector2D(108.f, 120.f), 10);
			Token->SetClipping(EWidgetClipping::ClipToBoundsAlways);
			Token->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			// 현재 런타임은 버튼을 RefreshTurnOrder에서 다시 켜지 않는다. donor의
			// 저장 Collapsed를 복제하면 카드 선택만 조용히 죽으므로 Visible을 보존한다.
			if (UButton* Button = Cast<UButton>(Blueprint->WidgetTree->FindWidget(
				FName(TEXT("TurnTokenButton") + Tail))))
			{
				PlaceCanvas(TurnPanel, Button, FVector2D(X, 30.f),
					FVector2D(108.f, 120.f), 40);
				Button->SetVisibility(ESlateVisibility::Visible);
			}

			UImage* Frame = FindOrCreate<UImage>(Blueprint,
				FName(TEXT("TurnFrame") + Tail));
			PlaceCanvas(Token, Frame, FVector2D::ZeroVector,
				FVector2D(108.f, 120.f), 5);
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
		NormalizeCenteredTextBounds(Blueprint);
	}

	/** @brief #519의 스킬/턴 종료 공용 목재 버튼 그림만 다시 연결한다. */
	void RepairAuthoredActionButtonArt(UWidgetBlueprint* Blueprint,
		UTexture2D* ActionButtonTexture)
	{
		check(Blueprint != nullptr && Blueprint->WidgetTree != nullptr);
		check(ActionButtonTexture != nullptr);

		const FName PlateNames[] = {
			TEXT("SkillTogglePlate"), TEXT("EndTurnPlate"), TEXT("CancelPlate")
		};
		for (const FName& PlateName : PlateNames)
		{
			UImage* Plate = Cast<UImage>(
				Blueprint->WidgetTree->FindWidget(PlateName));
			if (Plate == nullptr)
			{
				continue;
			}
			FSlateBrush Brush = Plate->GetBrush();
			Brush.SetResourceObject(ActionButtonTexture);
			Brush.DrawAs = ESlateBrushDrawType::Image;
			Brush.Margin = FMargin(0.f);
			Plate->SetBrush(Brush);
			Plate->SetColorAndOpacity(PlateName == TEXT("CancelPlate")
				? FLinearColor(.72f, .22f, .16f, 1.f)
				: FLinearColor::White);
		}
	}

	/** 단일 문구 행동 버튼은 라벨 겹을 아트/클릭 영역 전체에 맞춘다. */
	void FillAuthoredActionButtonLabel(UOverlay* LabelCenter)
	{
		if (LabelCenter == nullptr)
		{
			return;
		}
		if (UOverlaySlot* Slot = Cast<UOverlaySlot>(LabelCenter->Slot))
		{
			Slot->SetPadding(FMargin(0.f));
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Fill);
		}
	}

	/** 범용 세로 프레임 위에 기존 정보 위젯만 다시 놓는다. */
	void RepairAuthoredSummaryFrames(UWidgetBlueprint* Blueprint,
		UTexture2D* SummaryPanelTexture)
	{
		check(Blueprint != nullptr && Blueprint->WidgetTree != nullptr);
		check(SummaryPanelTexture != nullptr);
		for (const TCHAR* Prefix : { TEXT("Enemy"), TEXT("Ally") })
		{
			UCanvasPanel* Panel = CastChecked<UCanvasPanel>(
				Blueprint->WidgetTree->FindWidget(FName(FString(Prefix) + TEXT("Panel"))));
			UImage* Plate = CastChecked<UImage>(Blueprint->WidgetTree->FindWidget(
				FName(FString(Prefix) + TEXT("Plate"))));
			PlaceCanvas(Panel, Plate, FVector2D::ZeroVector,
				FVector2D(168.f, 550.f), -100);
			Plate->SetBrushFromTexture(SummaryPanelTexture, false);
			Plate->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			auto PlaceNamed = [&](const TCHAR* Suffix, const FVector2D Position,
				const FVector2D Size, const int32 ZOrder)
			{
				if (UWidget* Widget = Blueprint->WidgetTree->FindWidget(
					FName(FString(Prefix) + Suffix)))
				{
					PlaceCanvas(Panel, Widget, Position, Size, ZOrder);
				}
			};

			// 안쪽에는 새 장식을 만들지 않는다. 기존 이름판/초상/수치/상태 소켓을
			// 범용 바탕 위에 얹기만 하므로 이후 구성이 바뀌어도 프레임을 재생성할
			// 필요가 없다.
			PlaceNamed(TEXT("BadgePlateMount"), FVector2D(16.f, 16.f),
				FVector2D(136.f, 56.f), 10);
			PlaceNamed(TEXT("Name_Center"), FVector2D(16.f, 18.f),
				FVector2D(136.f, 52.f), 12);
			PlaceNamed(TEXT("PortraitFrame"), FVector2D(26.f, 70.f),
				FVector2D(116.f, 122.f), 10);
			PlaceNamed(TEXT("Portrait"), FVector2D(43.f, 85.f),
				FVector2D(82.f, 88.f), 11);
			PlaceNamed(TEXT("HPBackMount"), FVector2D(14.f, 204.f),
				FVector2D(140.f, 40.f), 10);
			PlaceNamed(TEXT("APPlateMount"), FVector2D(20.f, 255.f),
				FVector2D(128.f, 42.f), 10);
			PlaceNamed(TEXT("APText_Center"), FVector2D(20.f, 255.f),
				FVector2D(128.f, 42.f), 12);
			PlaceNamed(TEXT("SpeedPlateMount"), FVector2D(20.f, 303.f),
				FVector2D(128.f, 42.f), 10);
			PlaceNamed(TEXT("SpeedIcon"), FVector2D(30.f, 313.f),
				FVector2D(22.f, 22.f), 12);
			PlaceNamed(TEXT("SpeedText_Center"), FVector2D(52.f, 303.f),
				FVector2D(96.f, 42.f), 12);

			const float StatusSizes[3] = { 48.f, 58.f, 72.f };
			const float StatusY[3] = { 357.f, 412.f, 474.f };
			for (int32 Index = 0; Index < 3; ++Index)
			{
				const float StatusSize = StatusSizes[Index];
				const float X = (168.f - StatusSize) * .5f;
				const float Y = StatusY[Index];
				PlaceNamed(*FString::Printf(TEXT("StatusFrame_%dMount"), Index),
					FVector2D(X, Y), FVector2D(StatusSize, StatusSize), 10);
				PlaceNamed(*FString::Printf(TEXT("StatusIcon_%d"), Index),
					FVector2D(X + 6.f, Y + 6.f),
					FVector2D(StatusSize - 12.f, StatusSize - 12.f), 11);
				const FString CountBase = FString::Printf(
					TEXT("%sStatusCount_%d"), Prefix, Index);
				UOverlay* CountCenter = Cast<UOverlay>(Blueprint->WidgetTree->FindWidget(
					FName(CountBase + TEXT("_Center"))));
				UScaleBox* CountAutoFit = Cast<UScaleBox>(Blueprint->WidgetTree->FindWidget(
					FName(CountBase + TEXT("_AutoFit"))));
				UTextBlock* Count = Cast<UTextBlock>(Blueprint->WidgetTree->FindWidget(
					FName(CountBase)));
				if (CountCenter != nullptr && CountAutoFit != nullptr && Count != nullptr)
				{
					EnsureParent(CountCenter, CountAutoFit);
					if (UOverlaySlot* Slot = Cast<UOverlaySlot>(CountAutoFit->Slot))
					{
						Slot->SetPadding(FMargin(0.f));
						Slot->SetHorizontalAlignment(HAlign_Fill);
						Slot->SetVerticalAlignment(VAlign_Fill);
					}
					EnsureParent(CountAutoFit, Count);
					if (UScaleBoxSlot* Slot = Cast<UScaleBoxSlot>(Count->Slot))
					{
						Slot->SetHorizontalAlignment(HAlign_Fill);
						Slot->SetVerticalAlignment(VAlign_Fill);
					}
					PlaceCanvas(Panel, CountCenter,
						FVector2D(X + StatusSize - 18.f, Y + StatusSize - 18.f),
						FVector2D(18.f, 18.f), 12);
				}
				PlaceNamed(*FString::Printf(TEXT("StatusButton_%d"), Index),
					FVector2D(X, Y), FVector2D(StatusSize, StatusSize), 20);
			}

			for (const TCHAR* RetiredSuffix : { TEXT("CritPlate"), TEXT("CritIcon"),
				TEXT("CritText"), TEXT("CritText_Center"), TEXT("APPipRow") })
			{
				if (UWidget* Retired = Blueprint->WidgetTree->FindWidget(
					FName(FString(Prefix) + RetiredSuffix)))
				{
					Retired->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
		}
		// 다음 스킬은 데이터 계약만 남기고 이 요약판에서는 어떤 경우에도 그리지 않는다.
		for (const FName NextSkillName : { FName(TEXT("EnemyNextSkillFrame")),
			FName(TEXT("EnemyNextSkillIcon")), FName(TEXT("EnemyNextSkillButton")) })
		{
			if (UWidget* NextSkill = Blueprint->WidgetTree->FindWidget(NextSkillName))
			{
				NextSkill->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
		// 별도 설명 줄도 세로 요약판에서는 쓰지 않는다.
		for (const FName FooterName : { FName(TEXT("EnemyForecast")),
			FName(TEXT("EnemyForecast_Center")), FName(TEXT("AllySummaryHint")),
			FName(TEXT("AllySummaryHint_Center")) })
		{
			if (UWidget* Footer = Blueprint->WidgetTree->FindWidget(FooterName))
			{
				Footer->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}

	/**
	 * @brief 세로 요약판의 고정 3칸 상태 소켓을 런타임 스크롤 자리로 교체한다.
	 *
	 * 행은 실제 상태 수만큼 HUD가 동적으로 만든다. WBP에는 클리핑/입력 영역인
	 * ScrollBox만 굽고, 과거 소켓은 이름 호환을 위해 접어 둔다.
	 */
	void RepairAuthoredSummaryStatusScrolls(UWidgetBlueprint* Blueprint)
	{
		check(Blueprint != nullptr && Blueprint->WidgetTree != nullptr);
		for (const TCHAR* Prefix : { TEXT("Enemy"), TEXT("Ally") })
		{
			UCanvasPanel* Panel = CastChecked<UCanvasPanel>(
				Blueprint->WidgetTree->FindWidget(FName(FString(Prefix) + TEXT("Panel"))));
			UScrollBox* Scroll = FindOrCreate<UScrollBox>(Blueprint,
				FName(FString(Prefix) + TEXT("StatusScroll")));
			PlaceCanvas(Panel, Scroll, FVector2D(14.f, 352.f),
				FVector2D(140.f, 190.f), 15);
			Scroll->SetOrientation(EOrientation::Orient_Vertical);
			Scroll->SetClipping(EWidgetClipping::ClipToBoundsAlways);
			Scroll->SetScrollBarVisibility(ESlateVisibility::Collapsed);
			Scroll->SetScrollbarThickness(FVector2D(4.f, 4.f));
			Scroll->SetConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible);
			Scroll->SetAnimateWheelScrolling(true);
			Scroll->SetAllowOverscroll(false);
			Scroll->SetAlwaysShowScrollbar(false);
			Scroll->SetVisibility(ESlateVisibility::Visible);

			// 카드가 펼쳐져도 체력을 읽을 수 있어야 하므로 요약판 HP도 남긴다.
			for (const TCHAR* Suffix : { TEXT("HPBackMount"), TEXT("HPBack"),
				TEXT("HPBar"), TEXT("HPText_Center"), TEXT("HPText") })
			{
				if (UWidget* Widget = Blueprint->WidgetTree->FindWidget(
					FName(FString(Prefix) + Suffix)))
				{
					Widget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				}
			}
			// HP→금속 프레임→글자 순으로 그리면 HP는 넓게 차오르되 금속
			// 테두리를 덮지 않는다. 프레임을 시각적 마스크로 사용한다.
			if (UProgressBar* HPBar = Cast<UProgressBar>(
				Blueprint->WidgetTree->FindWidget(
					FName(FString(Prefix) + TEXT("HPBar")))))
			{
				if (UOverlaySlot* HPSlot = Cast<UOverlaySlot>(HPBar->Slot))
				{
					HPSlot->SetPadding(FMargin(2.f));
					if (UOverlay* Mount = Cast<UOverlay>(HPBar->GetParent()))
					{
						UWidget* Frame = Blueprint->WidgetTree->FindWidget(
							FName(FString(Prefix) + TEXT("HPBack")));
						const int32 FrameIndex = Mount->GetChildIndex(Frame);
						const int32 BarIndex = Mount->GetChildIndex(HPBar);
						if (Frame != nullptr && FrameIndex != INDEX_NONE
							&& BarIndex != INDEX_NONE && FrameIndex < BarIndex)
						{
							UPanelSlot* FrameSlot = Frame->Slot;
							Mount->RemoveChildAt(FrameIndex);
							Mount->InsertChildAt(
								Mount->GetChildIndex(HPBar) + 1, Frame, FrameSlot);
						}
					}
				}
			}

			// HP를 없앴던 판이 AP/속도 행을 위로 당겨 저장했을 수 있다.
			for (const TCHAR* Suffix : { TEXT("APPlateMount"), TEXT("APPlate"),
				TEXT("APText_Center"), TEXT("APText"), TEXT("SpeedPlateMount"),
				TEXT("SpeedPlate"), TEXT("SpeedIcon"), TEXT("SpeedText_Center"),
				TEXT("SpeedText") })
			{
				if (UWidget* Widget = Blueprint->WidgetTree->FindWidget(
					FName(FString(Prefix) + Suffix)))
				{
					FWidgetTransform Transform;
					if (FString(Suffix).EndsWith(TEXT("Icon")))
					{
						Transform.Scale = FVector2D(1.2f, 1.2f);
					}
					Widget->SetRenderTransform(Transform);
					Widget->SetRenderTransformPivot(FVector2D(.5f, .5f));
				}
			}

			for (const TCHAR* Suffix : { TEXT("StatusLabel"),
				TEXT("StatusLabel_Center"), TEXT("Status"), TEXT("Status_Center") })
			{
				if (UWidget* Legacy = Blueprint->WidgetTree->FindWidget(
					FName(FString(Prefix) + Suffix)))
				{
					Legacy->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
			for (int32 Index = 0; Index < 3; ++Index)
			{
				const FString Base = FString::Printf(TEXT("%sStatus"), Prefix);
				const FName LegacyNames[] = {
					FName(*FString::Printf(TEXT("%sFrame_%dMount"), *Base, Index)),
					FName(*FString::Printf(TEXT("%sFrame_%d"), *Base, Index)),
					FName(*FString::Printf(TEXT("%sIcon_%d"), *Base, Index)),
					FName(*FString::Printf(TEXT("%sCount_%d_Center"), *Base, Index)),
					FName(*FString::Printf(TEXT("%sCount_%d_AutoFit"), *Base, Index)),
					FName(*FString::Printf(TEXT("%sCount_%d"), *Base, Index)),
					FName(*FString::Printf(TEXT("%sButton_%d"), *Base, Index)),
				};
				for (const FName LegacyName : LegacyNames)
				{
					if (UWidget* Legacy = Blueprint->WidgetTree->FindWidget(LegacyName))
					{
						Legacy->SetRenderTransform(FWidgetTransform());
						Legacy->SetVisibility(ESlateVisibility::Collapsed);
					}
				}
			}
		}
	}

	/**
	 * 디자이너가 관리하는 인라인 용병 상세판에도 치명타 아이콘을 직접 굽는다.
	 * 별도 WBP_MercenaryPanel만 수정하면 실제 HUD에서는 그 Host가 Collapsed라
	 * 화면에 아무 변화가 없다.
	 */
	void RepairAuthoredMercenaryCriticalRow(UWidgetBlueprint* Blueprint,
		UTexture2D* DescriptionPlateTexture)
	{
		check(Blueprint != nullptr && Blueprint->WidgetTree != nullptr);
		check(DescriptionPlateTexture != nullptr);
		UCanvasPanel* DetailSection = Cast<UCanvasPanel>(
			Blueprint->WidgetTree->FindWidget(TEXT("MercDetailSection")));
		if (DetailSection == nullptr)
		{
			return;
		}

		// 앞의 세 수치 행은 FrameMount 안에서 원화를 AspectFit 한다. 치명타만
		// 이미지를 캔버스에 직접 놓으면 같은 620x68 슬롯이어도 브러시 희망
		// 크기로 그려져 오른쪽으로 길게 튀어나온다. 마지막 수치 행의 마운트
		// 좌표/크기를 복제하고 한 행 아래에 같은 계보로 배치한다.
		FVector2D RowPosition(1080.f, 576.f);
		FVector2D RowSize(620.f, 68.f);
		UOverlay* ReferenceMount = Cast<UOverlay>(
			Blueprint->WidgetTree->FindWidget(TEXT("MercenaryChip2FrameMount")));
		if (const UCanvasPanelSlot* ReferenceSlot = ReferenceMount != nullptr
			? Cast<UCanvasPanelSlot>(ReferenceMount->Slot) : nullptr)
		{
			RowPosition = ReferenceSlot->GetPosition() + FVector2D(0.f, 82.f);
			RowSize = ReferenceSlot->GetSize();
		}
		const FVector2D CritIconOffset(26.f, 10.f);
		const FVector2D ExistingStatIconOffset(36.f, 10.f);
		const FVector2D StatIconSize(46.f, 46.f);

		// 네 위젯의 박스 좌표만 같게 두면 치명타 원화의 왼쪽 투명 여백 때문에
		// 실제 그림은 치명타만 오른쪽에 보인다. HP/AP/속도 원화는 그 여백이
		// 없으므로 10px 오른쪽으로 보정해 화면상의 불투명 그림 중심을 맞춘다.
		for (int32 Index = 0; Index < 3; ++Index)
		{
			UWidget* StatMount = Blueprint->WidgetTree->FindWidget(
				FName(*FString::Printf(TEXT("MercenaryChip%dFrameMount"), Index)));
			UImage* StatIcon = Cast<UImage>(Blueprint->WidgetTree->FindWidget(
				FName(*FString::Printf(TEXT("MercenaryStatIcon_%d"), Index))));
			const UCanvasPanelSlot* StatMountSlot = StatMount != nullptr
				? Cast<UCanvasPanelSlot>(StatMount->Slot) : nullptr;
			if (StatIcon != nullptr && StatMountSlot != nullptr)
			{
				PlaceCanvas(DetailSection, StatIcon,
					StatMountSlot->GetPosition() + ExistingStatIconOffset,
					StatIconSize, 9);
				StatIcon->SetRenderTransform(FWidgetTransform());
				StatIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}
		}

		UOverlay* PlateMount = FindOrCreate<UOverlay>(Blueprint,
			TEXT("MercenaryCritPlateMount"));
		PlaceCanvas(DetailSection, PlateMount, RowPosition, RowSize, 8);
		PlateMount->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UImage* Plate = FindOrCreate<UImage>(Blueprint, TEXT("MercenaryCritPlate"));
		EnsureParent(PlateMount, Plate);
		if (const UImage* ReferencePlate = Cast<UImage>(
			Blueprint->WidgetTree->FindWidget(TEXT("MercenaryChip2Frame"))))
		{
			Plate->SetBrush(ReferencePlate->GetBrush());
			if (const UOverlaySlot* ReferencePlateSlot =
				Cast<UOverlaySlot>(ReferencePlate->Slot))
			{
				SetOverlayLayout(Plate, ReferencePlateSlot->GetPadding(),
					ReferencePlateSlot->GetHorizontalAlignment(),
					ReferencePlateSlot->GetVerticalAlignment());
			}
		}
		else
		{
			SetOverlayLayout(Plate, FMargin(0.f), HAlign_Fill, VAlign_Fill);
		}
		// 구형 행 배경에는 노란 마름모가 이미지에 박혀 있었다. 범용 무지 행으로
		// 갈아 끼운 뒤 별도 아이콘을 얹어야 실제 에셋 교체가 보인다.
		// 인라인 최종본의 수치 행은 빌더 기본 텍스처와 다른 브러시를 쓴다.
		// 바로 위 HP 행을 복사해야 구형 치명타 판에 박힌 노란 마름모까지 빠지고
		// 네 행의 모양도 정확히 같아진다.
		if (const UImage* ReferencePlate = Cast<UImage>(
			Blueprint->WidgetTree->FindWidget(TEXT("MercenaryChip0Frame"))))
		{
			Plate->SetBrush(ReferencePlate->GetBrush());
		}
		else
		{
			Plate->SetBrushFromTexture(DescriptionPlateTexture, false);
		}
		Plate->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UImage* Icon = FindOrCreate<UImage>(Blueprint, TEXT("MercenaryCritIcon"));
		PlaceCanvas(DetailSection, Icon, RowPosition + CritIconOffset,
			StatIconSize, 9);
		Icon->SetBrushFromTexture(EnsureCriticalIconTexture(), false);
		Icon->SetColorAndOpacity(FLinearColor::White);
		Icon->SetRenderTransform(FWidgetTransform());
		Icon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		// 디자이너 최종본의 옛 노란 마름모는 MercenaryCrit*가 아니라 공용
		// 네 번째 스탯 아이콘 이름으로 남아 있었다.
		if (UWidget* LegacyIcon = Blueprint->WidgetTree->FindWidget(
			TEXT("MercenaryStatIcon_3")))
		{
			LegacyIcon->SetVisibility(ESlateVisibility::Collapsed);
		}

		if (UTextBlock* Label = Cast<UTextBlock>(
			Blueprint->WidgetTree->FindWidget(TEXT("MercenaryCritLabel"))))
		{
			PlaceCanvas(DetailSection, Label, RowPosition + FVector2D(82.f, 4.f),
				FVector2D(145.f, 58.f), 9);
		}
		if (UTextBlock* Value = Cast<UTextBlock>(
			Blueprint->WidgetTree->FindWidget(TEXT("MercenaryCritValue"))))
		{
			PlaceCanvas(DetailSection, Value, RowPosition + FVector2D(205.f, 4.f),
				FVector2D(360.f, 58.f), 9);
		}
	}

	/** WBP에 이미 있는 스킬/확정/턴 종료 라벨의 계보와 변환만 수술한다. */
	void RepairAuthoredActionButtonLabels(UWidgetBlueprint* Blueprint)
	{
		check(Blueprint != nullptr && Blueprint->WidgetTree != nullptr);
		struct FActionButtonNames
		{
			FName Mount;
			FName LabelCenter;
			FName Label;
			FName Button;
		};
		const FActionButtonNames Buttons[] = {
			{ TEXT("SkillTogglePlateMount"), TEXT("SkillToggleLabel_Center"),
				TEXT("SkillToggleLabel"), TEXT("SkillToggleButton") },
			{ TEXT("ConfirmPlateMount"), TEXT("ConfirmLabel_Center"),
				TEXT("ConfirmLabel"), TEXT("ConfirmButton") },
			{ TEXT("EndTurnPlateMount"), TEXT("EndTurnLabel_Center"),
				TEXT("EndTurnLabel"), TEXT("EndTurnButton") },
		};
		for (const FActionButtonNames& Names : Buttons)
		{
			UOverlay* Mount = CastChecked<UOverlay>(
				Blueprint->WidgetTree->FindWidget(Names.Mount));
			UOverlay* Center = CastChecked<UOverlay>(
				Blueprint->WidgetTree->FindWidget(Names.LabelCenter));
			UTextBlock* Label = CastChecked<UTextBlock>(
				Blueprint->WidgetTree->FindWidget(Names.Label));
			UButton* Button = CastChecked<UButton>(
				Blueprint->WidgetTree->FindWidget(Names.Button));

			EnsureParent(Mount, Center);
			SetOverlayLayout(Center, FMargin(0.f), HAlign_Fill, VAlign_Fill);
			FillAuthoredActionButtonLabel(Center);
			EnsureParent(Center, Label);
			SetOverlayLayout(Label, FMargin(0.f), HAlign_Fill, VAlign_Fill);
			Label->SetMargin(FMargin(0.f));
			Label->SetRenderTransform(FWidgetTransform());
			Label->SetRenderTransformPivot(FVector2D(.5f, .5f));
			EnsureParent(Mount, Button);
			SetOverlayLayout(Button, FMargin(0.f), HAlign_Fill, VAlign_Fill);
		}
	}

	/**
	 * @brief #567 크기 조정본의 우측 메뉴, 요약판, 행동 단추 배치만 복구한다.
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

		// 우상단 설정 바. 네 단추를 모두 같은 Overlay 입력 계층에 둔다.
		// 설정 단추만 Canvas 자식으로 남으면 프레임 위 다른 자식의 hit test에
		// 가려져 모바일에서 눌리지 않는 회귀가 생긴다.
		UCanvasPanel* Objective = CastChecked<UCanvasPanel>(
			Blueprint->WidgetTree->FindWidget(TEXT("ObjectivePanel")));
		PlaceCanvas(Root, Objective, FVector2D(-570.f, 2.f),
			FVector2D(RunOptionsRail::Width, RunOptionsRail::Height), 90);
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
			FVector2D::ZeroVector, 1);
		if (UCanvasPanelSlot* Slot = CastChecked<UCanvasPanelSlot>(OptionsMount->Slot))
		{
			Slot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			Slot->SetOffsets(FMargin(0.f));
		}
		OptionsMount->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		ResetVisualTransform(OptionsMount, FVector2D(.5f, .5f));

		UImage* OptionsFrame = FindOrCreate<UImage>(Blueprint,
			TEXT("OptionsRailFrame"));
		EnsureParent(OptionsMount, OptionsFrame);
		OptionsFrame->SetBrushFromTexture(OptionsRailFrameTexture, false);
		OptionsFrame->SetColorAndOpacity(FLinearColor::White);
		OptionsFrame->SetVisibility(ESlateVisibility::HitTestInvisible);
		SetOverlaySlot(OptionsFrame, FMargin(0.f), HAlign_Fill, VAlign_Fill);

		// 눌리는 사각형은 아이콘과 같은 배율로 함께 움직여야 한다. 레일을
		// 1.2배로 키우면서 프레임과 아이콘만 옮기고 이 사각형은 옛 좌표로
		// 남겨 둔 탓에, 네 칸이 서로 겹쳐 아이콘 위에 커서를 올려도 옆 칸이
		// 잡혔다(0824 검수 4번). 좌표는 RunOptionsRail 한 곳에서만 온다.
		for (int32 Index = 0; Index < RunOptionsRail::SlotCount; ++Index)
		{
			UButton* Button = CastChecked<UButton>(Blueprint->WidgetTree->FindWidget(
				FName(*FString::Printf(TEXT("MenuButton_%d"), Index))));
			EnsureParent(OptionsMount, Button);
			// Overlay 안의 여백은 (왼쪽, 위, 오른쪽, 아래)다. 레일 크기에서
			// 사각형을 빼면 남는 쪽 여백이 된다.
			const FVector2D Position = RunOptionsRail::ButtonPosition(Index);
			const FVector2D Size = RunOptionsRail::ButtonSize();
			SetOverlaySlot(Button, FMargin(Position.X, Position.Y,
				RunOptionsRail::Width - Position.X - Size.X,
				RunOptionsRail::Height - Position.Y - Size.Y),
				HAlign_Fill, VAlign_Fill);
			Button->SetVisibility(ESlateVisibility::Visible);
		}

		struct FIconLayout
		{
			FName Name;
			FVector2D Position;
			FVector2D Size;
		};
		const FName IconNames[RunOptionsRail::SlotCount] = {
			TEXT("MenuMapIcon"), TEXT("MenuMercenaryIcon"),
			TEXT("MenuMonsterIcon"), TEXT("MenuSettingsIcon") };
		// 이 아이콘들은 판 한가운데를 기준으로 놓인다. 공용 좌표는 좌상단
		// 기준이므로 레일 중심을 빼서 옮긴다.
		const FVector2D RailCenter(
			RunOptionsRail::Width * .5f, RunOptionsRail::Height * .5f);
		for (int32 Index = 0; Index < RunOptionsRail::SlotCount; ++Index)
		{
			UImage* Icon = CastChecked<UImage>(
				Blueprint->WidgetTree->FindWidget(IconNames[Index]));
			PlaceCanvas(Objective, Icon,
				RunOptionsRail::IconPosition(Index) - RailCenter,
				RunOptionsRail::IconSize(Index), 31);
			if (UCanvasPanelSlot* Slot = CastChecked<UCanvasPanelSlot>(Icon->Slot))
			{
				Slot->SetAnchors(FAnchors(.5f, .5f));
			}
			Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}

		// 설정 바와 하단 행동 단추 사이의 좁은 세로 공간을 쓴다. 양 진영은
		// 같은 자리를 번갈아 사용한다.
		for (const FName PanelName : { FName(TEXT("EnemyPanel")),
			FName(TEXT("AllyPanel")) })
		{
			UCanvasPanel* Panel = CastChecked<UCanvasPanel>(
				Blueprint->WidgetTree->FindWidget(PanelName));
			PlaceCanvas(Root, Panel, FVector2D(0.f, 150.f),
				FVector2D(168.f, 550.f), 60);
			if (UCanvasPanelSlot* Slot = CastChecked<UCanvasPanelSlot>(Panel->Slot))
			{
				Slot->SetAnchors(FAnchors(1.f, 0.f));
				Slot->SetAlignment(FVector2D(1.f, 0.f));
			}
			Panel->SetVisibility(ESlateVisibility::Collapsed);
			ResetVisualTransform(Panel, FVector2D(1.f, 0.f));
			const FString Prefix = PanelName.ToString().LeftChop(5);
			// 스킬 카드가 열린 동안에도 체력을 읽을 수 있도록 요약판 HP 행을
			// 유지한다. 상태 목록은 아래 전용 스크롤이 차지한다.
			for (const TCHAR* Suffix : { TEXT("HPBackMount"), TEXT("HPBack"),
				TEXT("HPBar"), TEXT("HPText_Center"), TEXT("HPText") })
			{
				if (UWidget* HPWidget = Blueprint->WidgetTree->FindWidget(FName(
					*FString::Printf(TEXT("%s%s"), *Prefix, Suffix))))
				{
					HPWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				}
			}
			for (const TCHAR* Suffix : { TEXT("PortraitFrame"), TEXT("Portrait") })
			{
				if (UWidget* Widget = Blueprint->WidgetTree->FindWidget(FName(
					*FString::Printf(TEXT("%s%s"), *Prefix, Suffix))))
				{
					FWidgetTransform Transform;
					Transform.Scale = FVector2D(1.22f, 1.22f);
					Widget->SetRenderTransform(Transform);
					Widget->SetRenderTransformPivot(FVector2D(.5f, .5f));
				}
			}
			for (const TCHAR* Suffix : { TEXT("APPlateMount"), TEXT("APPlate"),
				TEXT("APText_Center"), TEXT("APText"), TEXT("SpeedPlateMount"),
				TEXT("SpeedPlate"), TEXT("SpeedIcon"), TEXT("SpeedText_Center"),
				TEXT("SpeedText") })
			{
				if (UWidget* Widget = Blueprint->WidgetTree->FindWidget(FName(
					*FString::Printf(TEXT("%s%s"), *Prefix, Suffix))))
				{
					FWidgetTransform Transform;
					if (FString(Suffix).EndsWith(TEXT("Icon")))
					{
						Transform.Scale = FVector2D(1.2f, 1.2f);
					}
					Widget->SetRenderTransform(Transform);
					Widget->SetRenderTransformPivot(FVector2D(.5f, .5f));
				}
			}
			for (int32 Index = 0; Index < 3; ++Index)
			{
				for (const TCHAR* Kind : { TEXT("Frame"), TEXT("Icon"),
					TEXT("Count"), TEXT("Button") })
				{
					if (UWidget* StatusWidget = Blueprint->WidgetTree->FindWidget(FName(
						*FString::Printf(TEXT("%sStatus%s_%d"),
							*Prefix, Kind, Index))))
					{
						StatusWidget->SetRenderTransform(FWidgetTransform());
						StatusWidget->SetVisibility(ESlateVisibility::Collapsed);
					}
				}
				if (UWidget* StatusButton = Blueprint->WidgetTree->FindWidget(FName(
					*FString::Printf(TEXT("%sStatusButton_%d"),
						*Prefix, Index))))
				{
					StatusButton->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
		}
		RepairAuthoredSummaryStatusScrolls(Blueprint);

		// 버튼 그림의 authored 크기는 보존하고 화면 모서리 배치만 새 계약으로
		// 정한다. 스킬/확정은 좌하단, 턴 종료는 우하단이다.
		const FVector2D ActionSize(396.172241f, 150.f);
		auto PlaceActionPanel = [&](const FName PanelName,
			const FName MountName, const FVector2D Position,
			const FAnchors Anchors, const FVector2D Alignment)
		{
			UCanvasPanel* Panel = FindOrCreate<UCanvasPanel>(Blueprint, PanelName);
			PlaceCanvas(Root, Panel, Position, ActionSize, 60);
			if (UCanvasPanelSlot* Slot = CastChecked<UCanvasPanelSlot>(Panel->Slot))
			{
				Slot->SetAnchors(Anchors);
				Slot->SetAlignment(Alignment);
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
			TEXT("SkillTogglePlateMount"), FVector2D(18.f, -26.f),
			FAnchors(0.f, 1.f), FVector2D(0.f, 1.f));
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
		FillAuthoredActionButtonLabel(SkillLabelCenter);
		UTextBlock* SkillLabel = CastChecked<UTextBlock>(
			Blueprint->WidgetTree->FindWidget(TEXT("SkillToggleLabel")));
		EnsureParent(SkillLabelCenter, SkillLabel);
		SetOverlaySlot(SkillLabel, FMargin(0.f), HAlign_Fill, VAlign_Fill);
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

		// 조준 중 스킬 단추 자리를 대신하는 확정 단추도 같은 외곽 크기와
		// 동일한 Overlay 계약을 쓴다. 기존 자산은 별도 350x112 판이라 전환 시
		// 버튼이 갑자기 작아 보였다.
		UOverlay* ConfirmMount = PlaceActionPanel(TEXT("ConfirmPanel"),
			TEXT("ConfirmPlateMount"), FVector2D(18.f, -26.f),
			FAnchors(0.f, 1.f), FVector2D(0.f, 1.f));
		UImage* ConfirmPlate = CastChecked<UImage>(
			Blueprint->WidgetTree->FindWidget(TEXT("ConfirmPlate")));
		EnsureParent(ConfirmMount, ConfirmPlate);
		ConfirmPlate->SetVisibility(ESlateVisibility::HitTestInvisible);
		SetOverlaySlot(ConfirmPlate, FMargin(0.f), HAlign_Fill, VAlign_Fill);
		UOverlay* ConfirmLabelCenter = FindOrCreate<UOverlay>(Blueprint,
			TEXT("ConfirmLabel_Center"));
		EnsureParent(ConfirmMount, ConfirmLabelCenter);
		ConfirmLabelCenter->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		SetOverlaySlot(ConfirmLabelCenter, FMargin(0.f), HAlign_Fill, VAlign_Fill);
		FillAuthoredActionButtonLabel(ConfirmLabelCenter);
		UTextBlock* ConfirmLabel = CastChecked<UTextBlock>(
			Blueprint->WidgetTree->FindWidget(TEXT("ConfirmLabel")));
		EnsureParent(ConfirmLabelCenter, ConfirmLabel);
		SetOverlaySlot(ConfirmLabel, FMargin(0.f), HAlign_Fill, VAlign_Fill);
		ConfirmLabel->SetJustification(ETextJustify::Center);
		UButton* ConfirmButton = CastChecked<UButton>(
			Blueprint->WidgetTree->FindWidget(TEXT("ConfirmButton")));
		EnsureParent(ConfirmMount, ConfirmButton);
		SetOverlaySlot(ConfirmButton, FMargin(0.f), HAlign_Fill, VAlign_Fill);
		ConfirmButton->SetVisibility(ESlateVisibility::Visible);

		UOverlay* EndMount = PlaceActionPanel(TEXT("EndTurnPanel"),
			TEXT("EndTurnPlateMount"), FVector2D(-10.334961f, -26.f),
			FAnchors(1.f, 1.f), FVector2D(1.f, 1.f));
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
		FillAuthoredActionButtonLabel(EndLabelCenter);
		UTextBlock* EndLabel = CastChecked<UTextBlock>(
			Blueprint->WidgetTree->FindWidget(TEXT("EndTurnLabel")));
		EnsureParent(EndLabelCenter, EndLabel);
		SetOverlaySlot(EndLabel, FMargin(0.f), HAlign_Fill, VAlign_Fill);
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

		// 취소는 턴 종료의 라벨만 바꿔 재사용하지 않는다. 좌하단 확정과도
		// 겹치지 않는 오른쪽 자리에 작고 붉은 전용 단추로 둔다.
		const FVector2D CancelSize(190.f, 112.f);
		UCanvasPanel* CancelPanel = FindOrCreate<UCanvasPanel>(
			Blueprint, TEXT("CancelPanel"));
		PlaceCanvas(Root, CancelPanel, FVector2D(430.f, -45.f),
			CancelSize, 61);
		if (UCanvasPanelSlot* CancelPanelSlot = CastChecked<UCanvasPanelSlot>(
			CancelPanel->Slot))
		{
			CancelPanelSlot->SetAnchors(FAnchors(0.f, 1.f));
			CancelPanelSlot->SetAlignment(FVector2D(0.f, 1.f));
		}
		CancelPanel->SetVisibility(ESlateVisibility::Collapsed);
		ResetVisualTransform(CancelPanel, FVector2D(.5f, .5f));

		UOverlay* CancelMount = FindOrCreate<UOverlay>(Blueprint,
			TEXT("CancelPlateMount"));
		PlaceCanvas(CancelPanel, CancelMount, FVector2D::ZeroVector,
			CancelSize, 0);
		CancelMount->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UImage* CancelPlate = FindOrCreate<UImage>(Blueprint,
			TEXT("CancelPlate"));
		EnsureParent(CancelMount, CancelPlate);
		SetOverlaySlot(CancelPlate, FMargin(0.f), HAlign_Fill, VAlign_Fill);
		CancelPlate->SetBrush(EndPlate->GetBrush());
		CancelPlate->SetColorAndOpacity(FLinearColor(.72f, .22f, .16f, 1.f));
		CancelPlate->SetVisibility(ESlateVisibility::HitTestInvisible);

		UOverlay* CancelCenter = FindOrCreate<UOverlay>(Blueprint,
			TEXT("CancelLabel_Center"));
		EnsureParent(CancelMount, CancelCenter);
		SetOverlaySlot(CancelCenter, FMargin(0.f), HAlign_Fill, VAlign_Fill);
		CancelCenter->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UTextBlock* CancelLabel = FindOrCreate<UTextBlock>(Blueprint,
			TEXT("CancelLabel"));
		EnsureParent(CancelCenter, CancelLabel);
		SetOverlaySlot(CancelLabel, FMargin(0.f), HAlign_Fill, VAlign_Fill);
		FSlateFontInfo CancelFont = UIFont::MakeProjectExact(
			EndLabel->GetFont(), 34);
		CancelFont.OutlineSettings.OutlineSize = 2;
		CancelFont.OutlineSettings.OutlineColor = FLinearColor::Black;
		CancelLabel->SetFont(CancelFont);
		CancelLabel->SetText(NSLOCTEXT("CombatHUD", "CancelAim", "취소"));
		CancelLabel->SetJustification(ETextJustify::Center);
		CancelLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		CancelLabel->SetVisibility(ESlateVisibility::HitTestInvisible);

		UButton* CancelButton = FindOrCreate<UButton>(Blueprint,
			TEXT("CancelButton"));
		EnsureParent(CancelMount, CancelButton);
		SetOverlaySlot(CancelButton, FMargin(0.f), HAlign_Fill, VAlign_Fill);
		SetInvisibleButtonChrome(CancelButton);
		RepairAuthoredActionButtonLabels(Blueprint);
		NormalizeCenteredTextBounds(Blueprint);
	}

	/** @brief AP 바를 해상도에 따라 움직이는 파티 레이어에서 떼어 좌상단에 고정한다. */
	bool RepairTurnAPPlacement(UWidgetBlueprint* Blueprint)
	{
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
		{
			return false;
		}
		UCanvasPanel* Root = Cast<UCanvasPanel>(Blueprint->WidgetTree->RootWidget);
		UScaleBox* TurnAPScale = Cast<UScaleBox>(
			Blueprint->WidgetTree->FindWidget(TEXT("TurnAPScale")));
		if (Root == nullptr || TurnAPScale == nullptr)
		{
			return false;
		}

		TurnAPScale->SetStretch(EStretch::ScaleToFit);
		TurnAPScale->SetStretchDirection(EStretchDirection::Both);
		PlaceCanvas(Root, TurnAPScale, FVector2D(18.f, 164.f),
			FVector2D(800.f, 97.f), 2);
		return true;
	}

	/** @brief 좌상단 15칸 AP 바와 원본 스킬 카드 배치를 복구한다. */
	void RepairAuthoredPrimaryCombatControls(UWidgetBlueprint* Blueprint)
	{
		check(Blueprint != nullptr && Blueprint->WidgetTree != nullptr);
		UCanvasPanel* Root = CastChecked<UCanvasPanel>(
			Blueprint->WidgetTree->RootWidget);
		RemoveRetiredQuickSkillWidgets(Blueprint);

		// ROUND/턴 바 바로 아래의 좌상단에 고정한다. 왼쪽 AP 전용 배지와
		// 오른쪽 15칸 레일을 800x97 안에 함께 넣어 중앙 Move 카드와 겹치지 않는다.
		check(RepairTurnAPPlacement(Blueprint));
		if (UTextBlock* TurnAPText = Cast<UTextBlock>(
			Blueprint->WidgetTree->FindWidget(TEXT("TurnAPText"))))
		{
			TurnAPText->SetMargin(FMargin(0.f));
			TurnAPText->SetFont(UIFont::MakeProjectExact(
				TurnAPText->GetFont(), 19));
			TurnAPText->SetRenderTransform(FWidgetTransform());
		}

		// 15칸을 5 | 5 | 5로 읽을 수 있도록 각 묶음 사이에 11px 여백과
		// 밝은 금속 구분선을 둔다. 보석과 점등 레이어는 같은 고정 좌표를 쓴다.
		if (UCanvasPanel* PipRow = Cast<UCanvasPanel>(
			Blueprint->WidgetTree->FindWidget(TEXT("TurnAPPipRow"))))
		{
			constexpr int32 PipCount = 15;
			constexpr int32 PipsPerGroup = 5;
			constexpr float PipStep = 27.f;
			constexpr float GroupGap = 9.f;
			const FVector2D PipOrigin(.5f, 1.f);
			const FVector2D PipSize(25.f, 32.f);
			constexpr float PipRowWidth = 424.f;
			constexpr float PlateWidth = 568.421f;
			constexpr float RailX = 116.f;
			constexpr float RailY = 5.5f;
			constexpr float RailHeight = 58.f;
			constexpr float BadgeWidth = 132.f;
			constexpr float BadgeHeight = 68.8995f;

			// 수동 편집이나 이전 시안에서 16칸 이상이 남아 있어도 빌더 한 번으로
			// 정확히 15칸 계약으로 돌아오게 한다.
			TSet<UWidget*> OverflowPips;
			for (UWidget* Widget : Blueprint->GetAllSourceWidgets())
			{
				if (Widget == nullptr)
				{
					continue;
				}
				const FString WidgetName = Widget->GetName();
				for (const TCHAR* Prefix : { TEXT("TurnAPPip"),
					TEXT("TurnAPPipUsed"), TEXT("TurnAPPipGlow") })
				{
					const FString PrefixWithSeparator = FString(Prefix) + TEXT("_");
					if (WidgetName.StartsWith(PrefixWithSeparator) == false)
					{
						continue;
					}
					const FString Suffix = WidgetName.RightChop(
						PrefixWithSeparator.Len());
					if (Suffix.IsNumeric() && FCString::Atoi(*Suffix) >= PipCount)
					{
						OverflowPips.Add(Widget);
					}
				}
			}
			DeleteWidgetsCompletely(Blueprint, OverflowPips);

			// 참고안은 하나의 연속 레일에 선만 둔다. 이전 시안의 어두운
			// 그룹 홈은 모두 지우고 구분선은 정확히 2개만 남긴다.
			TSet<UWidget*> OverflowGroups;
			for (UWidget* Widget : Blueprint->GetAllSourceWidgets())
			{
				if (Widget == nullptr)
				{
					continue;
				}
				const FString WidgetName = Widget->GetName();
				for (const TCHAR* Prefix : { TEXT("TurnAPGroupWell"),
					TEXT("TurnAPGroupSeparatorShadow"),
					TEXT("TurnAPGroupSeparator") })
				{
					const FString PrefixWithSeparator = FString(Prefix) + TEXT("_");
					if (WidgetName.StartsWith(PrefixWithSeparator) == false)
					{
						continue;
					}
					const FString Suffix = WidgetName.RightChop(
						PrefixWithSeparator.Len());
					const int32 MaxCount = FString(Prefix) == TEXT("TurnAPGroupWell")
						? 0 : 2;
					if (Suffix.IsNumeric() && FCString::Atoi(*Suffix) >= MaxCount)
					{
						OverflowGroups.Add(Widget);
					}
				}
			}
			DeleteWidgetsCompletely(Blueprint, OverflowGroups);

			if (UCanvasPanelSlot* RowSlot = Cast<UCanvasPanelSlot>(PipRow->Slot))
			{
				RowSlot->SetPosition(FVector2D(136.f, 17.f));
				RowSlot->SetSize(FVector2D(PipRowWidth, 34.f));
			}
			UOverlay* PlateMount = Cast<UOverlay>(Blueprint->WidgetTree->FindWidget(
				TEXT("TurnAPPlateMount")));
			UCanvasPanel* TurnAPPanel = Cast<UCanvasPanel>(
				Blueprint->WidgetTree->FindWidget(TEXT("TurnAPPanel")));
			if (PlateMount != nullptr && TurnAPPanel != nullptr)
			{
				if (UCanvasPanelSlot* PlateSlot = Cast<UCanvasPanelSlot>(PlateMount->Slot))
				{
					PlateSlot->SetPosition(FVector2D(RailX, RailY));
					PlateSlot->SetSize(FVector2D(PlateWidth - RailX, RailHeight));
				}

				// ROUND의 좌우 금속 캡과 주황 테두리 전체를 재사용해
				// AP 전용 배지를 만든다. 긴 AP 원화를 배지로 찌그러뜨리지 않는다.
				UOverlay* BadgeMount = FindOrCreate<UOverlay>(Blueprint,
					TEXT("TurnAPBadgeMount"));
				PlaceCanvas(TurnAPPanel, BadgeMount, FVector2D::ZeroVector,
					FVector2D(BadgeWidth, BadgeHeight), 20);
				BadgeMount->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

				UImage* BadgePlate = FindOrCreate<UImage>(Blueprint,
					TEXT("TurnAPBadgePlate"));
				EnsureParent(BadgeMount, BadgePlate);
				SetOverlayLayout(BadgePlate, FMargin(0.f), HAlign_Fill, VAlign_Fill);
				if (UImage* RoundPlate = Cast<UImage>(
					Blueprint->WidgetTree->FindWidget(TEXT("RoundPlate"))))
				{
					FSlateBrush BadgeBrush = RoundPlate->GetBrush();
					// ROUND 원본(1839x573)의 장식 전체를 작은 AP 배지 안에 보인다.
					// Box 9-slice는 실제 원본 크기로 테두리를 계산해 작은 배지의
					// 중앙을 접으므로 이 전용 배지만 전체 이미지 축소를 사용한다.
					BadgeBrush.DrawAs = ESlateBrushDrawType::Image;
					BadgeBrush.Margin = FMargin(0.f);
					BadgePlate->SetBrush(BadgeBrush);
				}
				BadgePlate->SetVisibility(ESlateVisibility::HitTestInvisible);

				UOverlay* LabelCenter = FindOrCreate<UOverlay>(Blueprint,
					TEXT("TurnAPLabel_Center"));
				EnsureParent(BadgeMount, LabelCenter);
				SetOverlayLayout(LabelCenter, FMargin(12.f, 6.f, 12.f, 34.f),
					HAlign_Fill, VAlign_Fill);
				UTextBlock* APLabel = FindOrCreate<UTextBlock>(Blueprint,
					TEXT("TurnAPLabel"));
				EnsureParent(LabelCenter, APLabel);
				SetOverlayLayout(APLabel, FMargin(0.f), HAlign_Fill, VAlign_Fill);
				APLabel->SetText(FText::FromString(TEXT("AP")));
				FSlateFontInfo LabelFont = UIFont::MakeProjectExact(
					APLabel->GetFont(), 14);
				LabelFont.OutlineSettings.OutlineSize = 1;
				LabelFont.OutlineSettings.OutlineColor = FLinearColor::Black;
				APLabel->SetFont(LabelFont);
				APLabel->SetJustification(ETextJustify::Center);
				APLabel->SetColorAndOpacity(FSlateColor(
					FLinearColor(.82f, .58f, .31f, 1.f)));
				APLabel->SetVisibility(ESlateVisibility::HitTestInvisible);

				if (UWidget* TextCenter = Blueprint->WidgetTree->FindWidget(
					TEXT("TurnAPText_Center")))
				{
					EnsureParent(BadgeMount, TextCenter);
					SetOverlayLayout(TextCenter, FMargin(10.f, 26.f, 10.f, 5.f),
						HAlign_Fill, VAlign_Fill);
				}
			}
			// 위에서 새로 만든 AP 라벨도 다른 HUD 문구와 같은
			// Center -> AutoFit -> Text 계약에 편입한다.
			NormalizeCenteredTextBounds(Blueprint);

			UMaterialInterface* APGemFlashMaterial = LoadObject<UMaterialInterface>(
				nullptr, APGemFlashMaterialPath);
			UImage* PipTemplate = Cast<UImage>(Blueprint->WidgetTree->FindWidget(
				TEXT("TurnAPPip_0")));
			UImage* UsedTemplate = Cast<UImage>(Blueprint->WidgetTree->FindWidget(
				TEXT("TurnAPPipUsed_0")));
			check(PipTemplate != nullptr && UsedTemplate != nullptr);

			for (int32 SeparatorIndex = 0; SeparatorIndex < 2; ++SeparatorIndex)
			{
				const int32 NextPipIndex = (SeparatorIndex + 1) * PipsPerGroup;
				const float NextPipX = PipOrigin.X + PipStep * NextPipIndex
					+ GroupGap * (SeparatorIndex + 1);
				const float PreviousPipRight = PipOrigin.X
					+ PipStep * (NextPipIndex - 1)
					+ GroupGap * SeparatorIndex + PipSize.X;
				const float SeparatorCenter = (PreviousPipRight + NextPipX) * .5f;

				UBorder* SeparatorShadow = FindOrCreate<UBorder>(Blueprint,
					FName(*FString::Printf(
						TEXT("TurnAPGroupSeparatorShadow_%d"), SeparatorIndex)));
				PlaceCanvas(PipRow, SeparatorShadow,
					FVector2D(SeparatorCenter - 3.5f, .5f), FVector2D(7.f, 33.f), 14);
				SeparatorShadow->SetBrushColor(
					FLinearColor(.025f, .012f, .004f, .95f));
				SeparatorShadow->SetVisibility(ESlateVisibility::HitTestInvisible);

				UBorder* Separator = FindOrCreate<UBorder>(Blueprint,
					FName(*FString::Printf(
						TEXT("TurnAPGroupSeparator_%d"), SeparatorIndex)));
				PlaceCanvas(PipRow, Separator,
					FVector2D(SeparatorCenter - 2.f, 2.f), FVector2D(4.f, 30.f), 15);
				Separator->SetBrushColor(FLinearColor(.78f, .72f, .61f, 1.f));
				Separator->SetVisibility(ESlateVisibility::HitTestInvisible);
			}

			for (int32 PipIndex = 0; PipIndex < PipCount; ++PipIndex)
			{
				const FVector2D Position = PipOrigin
					+ FVector2D(PipStep * PipIndex
						+ GroupGap * (PipIndex / PipsPerGroup), 0.f);
				UImage* UsedPip = FindOrCreate<UImage>(Blueprint,
					FName(*FString::Printf(TEXT("TurnAPPipUsed_%d"), PipIndex)));
				PlaceCanvas(PipRow, UsedPip, Position, PipSize, 10);
				UsedPip->SetBrush(UsedTemplate->GetBrush());
				UsedPip->SetVisibility(ESlateVisibility::Collapsed);

				UImage* Pip = FindOrCreate<UImage>(Blueprint,
					FName(*FString::Printf(TEXT("TurnAPPip_%d"), PipIndex)));
				PlaceCanvas(PipRow, Pip, Position, PipSize, 11);
				Pip->SetBrush(PipTemplate->GetBrush());
				Pip->SetVisibility(ESlateVisibility::Collapsed);

				UImage* Glow = FindOrCreate<UImage>(Blueprint,
					FName(*FString::Printf(TEXT("TurnAPPipGlow_%d"), PipIndex)));
				PlaceCanvas(PipRow, Glow, Position, PipSize, 20);
				if (APGemFlashMaterial != nullptr)
				{
					Glow->SetBrushFromMaterial(APGemFlashMaterial);
				}
				Glow->SetColorAndOpacity(FLinearColor::White);
				Glow->SetRenderOpacity(0.f);
				Glow->SetRenderTransformPivot(FVector2D(.5f));
				Glow->SetRenderTransformAngle(0.f);
				Glow->SetVisibility(ESlateVisibility::Collapsed);
			}
		}

		const FVector2D CardSize(200.f, 228.f);
		const FVector2D CardPositions[] = {
			FVector2D(-95.9809f, -327.8756f),
			FVector2D(-376.1722f, -184.3349f),
			FVector2D(168.1340f, -184.3349f),
			FVector2D(-376.1722f, 90.1148f),
			FVector2D(168.1340f, 90.1148f),
			FVector2D(-95.9809f, 232.5072f),
		};
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(CardPositions); ++Index)
		{
			UCanvasPanel* Card = Cast<UCanvasPanel>(Blueprint->WidgetTree->FindWidget(
				FName(*FString::Printf(TEXT("CommandCard_%d"), Index))));
			if (Card == nullptr)
			{
				continue;
			}

			PlaceCanvas(Root, Card, CardPositions[Index], CardSize, 0);
			if (UCanvasPanelSlot* CardSlot = CastChecked<UCanvasPanelSlot>(Card->Slot))
			{
				CardSlot->SetAnchors(FAnchors(.5f, .5f));
			}

			// 펼친 스킬 카드의 아이콘 정중앙에
			// 남은 쿨타임을 보여 준다. 기존 우하단 배지는 총 쿨타임처럼
			// 읽히므로 런타임에서 숨기고 이 겹만 사용한다.
			if (Index > 0)
			{
				UImage* CommandIcon = Cast<UImage>(Blueprint->WidgetTree->FindWidget(
					FName(*FString::Printf(TEXT("CommandIcon_%d"), Index))));
				const UCanvasPanelSlot* IconSlot = CommandIcon != nullptr
					? Cast<UCanvasPanelSlot>(CommandIcon->Slot) : nullptr;
				if (IconSlot != nullptr)
				{
					UOverlay* CooldownRoot = FindOrCreate<UOverlay>(Blueprint,
						FName(*FString::Printf(
							TEXT("CommandCooldownOverlayRoot_%d"), Index)));
					PlaceCanvas(Card, CooldownRoot, IconSlot->GetPosition(),
						IconSlot->GetSize(), 40);
					CooldownRoot->SetVisibility(ESlateVisibility::Collapsed);

					UTextBlock* CooldownText = FindOrCreate<UTextBlock>(Blueprint,
						FName(*FString::Printf(
							TEXT("CommandCooldownOverlay_%d"), Index)));
					EnsureParent(CooldownRoot, CooldownText);
					SetOverlayLayout(CooldownText, FMargin(0.f),
						HAlign_Center, VAlign_Center);
					FSlateFontInfo CooldownFont = UIFont::MakeProjectExact(
						CooldownText->GetFont(), 42);
					CooldownFont.OutlineSettings.OutlineSize = 4;
					CooldownFont.OutlineSettings.OutlineColor = FLinearColor::Black;
					CooldownText->SetFont(CooldownFont);
					CooldownText->SetJustification(ETextJustify::Center);
					CooldownText->SetColorAndOpacity(
						FSlateColor(FLinearColor::White));
					CooldownText->SetVisibility(ESlateVisibility::HitTestInvisible);
				}
			}

			// 크기 실험에서 추가한 바깥 래퍼는 남겨 두지 않는다. 카드 자체의
			// 200x228 저작 슬롯만 존재해야 해상도/DPI가 한 번만 적용된다.
			for (const FName RetiredName : {
				FName(*FString::Printf(TEXT("CommandCardScale_%d"), Index)),
				FName(*FString::Printf(TEXT("CommandCardDesignSize_%d"), Index)) })
			{
				if (UWidget* Retired = Blueprint->WidgetTree->FindWidget(RetiredName))
				{
					Blueprint->WidgetTree->RemoveWidget(Retired);
					Blueprint->WidgetVariableNameToGuidMap.Remove(RetiredName);
					Blueprint->OnVariableRemoved(RetiredName);
				}
			}
		}

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

		// 0823 확정: 요약판 AP 는 문구로만 보여 준다(보석 행만 걷는다).
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

		// 0823 확정: 요약판 AP 는 문구로만 보여 준다(보석 행만 걷는다).
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
				if (UCanvasPanel* Parent = Cast<UCanvasPanel>(
					ExistingInventoryPlate->GetParent()))
				{
					Parent->SetClipping(EWidgetClipping::Inherit);
					UImage* ExistingIcon = Cast<UImage>(
						Blueprint->WidgetTree->FindWidget(
							TEXT("MercenaryInventoryTabIcon")));
					UTextBlock* ExistingLabel = Cast<UTextBlock>(
						Blueprint->WidgetTree->FindWidget(
							TEXT("MercenaryInventoryTabText")));
					if (ExistingIcon != nullptr && ExistingLabel != nullptr)
					{
						// 통합 HUD에 남은 직렬화 프록시도 독립 용병 WBP와 같은
						// 구조로 만든다. 문구만 판 중앙에 두면 가방이 왼쪽에
						// 추가되어 두 요소 전체가 오른쪽으로 치우쳐 보인다.
						UOverlay* Center = FindOrCreate<UOverlay>(Blueprint,
							TEXT("MercenaryInventoryTabText_Center"));
						PlaceCanvas(Parent, Center, VisiblePosition, VisibleSize, 2);
						UHorizontalBox* Content = FindOrCreate<UHorizontalBox>(Blueprint,
							TEXT("MercenaryInventoryTabContent"));
						EnsureParent(Center, Content);
						SetOverlayLayout(Content, FMargin(0.f), HAlign_Center,
							VAlign_Center);
						USizeBox* IconSizeBox = FindOrCreate<USizeBox>(Blueprint,
							TEXT("MercenaryInventoryTabIconSize"));
						const float CardScaleX = DonorSize.X / 350.f;
						const float CardScaleY = DonorSize.Y / 128.f;
						const float IconSize = FMath::Min(
							96.f * CardScaleX, 96.f * CardScaleY);
						IconSizeBox->SetWidthOverride(IconSize);
						IconSizeBox->SetHeightOverride(IconSize);
						EnsureParent(IconSizeBox, ExistingIcon);
						EnsureParent(Content, IconSizeBox);
						if (UHorizontalBoxSlot* IconSlot =
							Cast<UHorizontalBoxSlot>(IconSizeBox->Slot))
						{
							IconSlot->SetPadding(FMargin(
								0.f, 0.f, 12.f * CardScaleX, 0.f));
							IconSlot->SetHorizontalAlignment(HAlign_Center);
							IconSlot->SetVerticalAlignment(VAlign_Center);
						}
						EnsureParent(Content, ExistingLabel);
						if (UHorizontalBoxSlot* LabelSlot =
							Cast<UHorizontalBoxSlot>(ExistingLabel->Slot))
						{
							LabelSlot->SetPadding(FMargin(0.f));
							LabelSlot->SetHorizontalAlignment(HAlign_Left);
							LabelSlot->SetVerticalAlignment(VAlign_Center);
						}
						ExistingLabel->SetJustification(ETextJustify::Left);
						ExistingLabel->SetRenderTransform(FWidgetTransform());
					}
				}
				UWidget* ExistingGoldFrame = Blueprint->WidgetTree->FindWidget(
					TEXT("MercenaryInventoryGoldFrame"));
				UImage* ExistingGoldIcon = Cast<UImage>(
					Blueprint->WidgetTree->FindWidget(TEXT("MercenaryInventoryGoldIcon")));
				UCanvasPanelSlot* GoldFrameSlot = ExistingGoldFrame != nullptr
					? Cast<UCanvasPanelSlot>(ExistingGoldFrame->Slot) : nullptr;
				UCanvasPanelSlot* GoldIconSlot = ExistingGoldIcon != nullptr
					? Cast<UCanvasPanelSlot>(ExistingGoldIcon->Slot) : nullptr;
				UCanvasPanel* GoldParent = ExistingGoldFrame != nullptr
					? Cast<UCanvasPanel>(ExistingGoldFrame->GetParent()) : nullptr;
				if (GoldFrameSlot != nullptr && GoldIconSlot != nullptr
					&& GoldParent != nullptr)
				{
					const FVector2D FramePosition = GoldFrameSlot->GetPosition();
					const FVector2D FrameSize = GoldFrameSlot->GetSize();
					const float IconSize = FMath::Min(FrameSize.X, FrameSize.Y) * .38f;
					PlaceCanvas(GoldParent, ExistingGoldIcon,
						FramePosition + (FrameSize - FVector2D(IconSize, IconSize)) * .5f,
						FVector2D(IconSize, IconSize), 3);
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
		InventoryIcon->SetBrushFromTexture(InventoryIconTexture, false);
		InventoryIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UTextBlock* InventoryLabel = FindOrCreate<UTextBlock>(
			Blueprint, TEXT("MercenaryInventoryTabText"));
		// 아이콘과 문구를 따로 판 중앙에 맞추면 둘을 합친 덩어리는 왼쪽으로
		// 치우친다. 둘을 하나의 가로 행으로 묶고 그 행 자체를 판 중앙에 둔다.
		UOverlay* InventoryLabelCenter = FindOrCreate<UOverlay>(
			Blueprint, TEXT("MercenaryInventoryTabText_Center"));
		PlaceCanvas(InventoryTab, InventoryLabelCenter, VisibleTabPosition,
			VisibleTabSize, 2);
		UHorizontalBox* InventoryContent = FindOrCreate<UHorizontalBox>(
			Blueprint, TEXT("MercenaryInventoryTabContent"));
		EnsureParent(InventoryLabelCenter, InventoryContent);
		SetOverlayLayout(InventoryContent, FMargin(0.f), HAlign_Center, VAlign_Center);
		USizeBox* InventoryIconSize = FindOrCreate<USizeBox>(
			Blueprint, TEXT("MercenaryInventoryTabIconSize"));
		InventoryIconSize->SetWidthOverride(IconSize);
		InventoryIconSize->SetHeightOverride(IconSize);
		EnsureParent(InventoryIconSize, InventoryIcon);
		EnsureParent(InventoryContent, InventoryIconSize);
		if (UHorizontalBoxSlot* IconSlot = Cast<UHorizontalBoxSlot>(
			InventoryIconSize->Slot))
		{
			IconSlot->SetPadding(FMargin(0.f, 0.f, 12.f * CardScaleX, 0.f));
			IconSlot->SetHorizontalAlignment(HAlign_Center);
			IconSlot->SetVerticalAlignment(VAlign_Center);
		}
		EnsureParent(InventoryContent, InventoryLabel);
		if (UHorizontalBoxSlot* LabelSlot = Cast<UHorizontalBoxSlot>(
			InventoryLabel->Slot))
		{
			LabelSlot->SetPadding(FMargin(0.f));
			LabelSlot->SetHorizontalAlignment(HAlign_Left);
			LabelSlot->SetVerticalAlignment(VAlign_Center);
		}
		InventoryLabel->SetText(NSLOCTEXT(
			"CombatHUD", "MercenaryInventoryTab", "인벤토리"));
		SetReadableFont(InventoryLabel, BaseFont, 27);
		InventoryLabel->SetJustification(ETextJustify::Left);
		InventoryLabel->SetRenderTransform(FWidgetTransform());

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
			GoldContentSize.Y) * .55f;
		PlaceCanvas(Page, GoldIcon,
			FVector2D(GridOrigin.X + (FrameSize - GoldIconSize) * .5f,
				GridOrigin.Y + (FrameSize - GoldIconSize) * .5f),
			FVector2D(GoldIconSize, GoldIconSize), 3);
		GoldIcon->SetBrushFromTexture(GoldIconTexture, false);
		GoldIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UTextBlock* GoldText = FindOrCreate<UTextBlock>(
			Blueprint, TEXT("MercenaryInventoryGoldText"));
		// TextBlock의 글꼴 베이스라인이 슬롯 아래로 내려가므로 숫자 칸을
		// 프레임의 하단 경계에서 충분히 올린다. 실제 2176x1812 캡처에서
		// "100"이 프레임 밖으로 빠진 원인이었다.
		PlaceCanvas(Page, GoldText,
			FVector2D(GoldInner.Min.X + GoldContentSize.X * .48f,
				GoldInner.Min.Y + GoldContentSize.Y * .70f),
			FVector2D(GoldContentSize.X * .47f, GoldContentSize.Y * .24f), 4);
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
		static const FText PreviewArtifactNames[6] = {
			NSLOCTEXT("CombatHUD", "ArtifactPreviewChalice", "피의 성배"),
			NSLOCTEXT("CombatHUD", "ArtifactPreviewFang", "야수의 송곳니"),
			NSLOCTEXT("CombatHUD", "ArtifactPreviewCoin", "행운의 주화"),
			NSLOCTEXT("CombatHUD", "ArtifactPreviewThorn", "가시 문장"),
			NSLOCTEXT("CombatHUD", "ArtifactPreviewMap", "여행자의 지도"),
			NSLOCTEXT("CombatHUD", "ArtifactPreviewShield", "낡은 방패 장식") };
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
				? FText(PreviewArtifactNames[Index])
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
		// 세로 요약판은 AP 문구와 상태이상만 쓴다. 치명타 칸은 옛 AP 보석
		// 행의 위치를 읽기 위한 앵커일 뿐이며, 인벤토리 재빌드가 다시 켜면 안 된다.
		for (const TCHAR* CriticalWidget : {
			TEXT("EnemyCritPlate"), TEXT("EnemyCritIcon"), TEXT("EnemyCritText") })
		{
			if (UWidget* Widget = Blueprint->WidgetTree->FindWidget(
				FName(CriticalWidget)))
			{
				Widget->SetVisibility(ESlateVisibility::Collapsed);
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
		// 0822 확정: 요약판 AP 보석 행도 걷는다. 이름 계약만 남긴다.
		Row->SetVisibility(ESlateVisibility::Collapsed);
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

	/** @brief 다른 HUD 구역은 건드리지 않고 #567 ROUND+턴바만 복구한다. */
	void RepairRoundTurnOnly()
	{
		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, AssetPath);
		UTexture2D* RoundBadgeTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_RoundBadge_Frame.T_MB_RoundBadge_Frame"));
		UTexture2D* TurnTokenFrameTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/UI/Generated/CombatHUD/T_MB_TurnToken_Frame_NoSpeed_v1.T_MB_TurnToken_Frame_NoSpeed_v1"));
		UTexture2D* SummaryPanelTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/UI/Generated/CombatHUD/T_MB_GenericDetailPanel_NoFooter_v1.T_MB_GenericDetailPanel_NoFooter_v1"));
		UTexture2D* ActionButtonTexture = LoadObject<UTexture2D>(nullptr,
			ActionButtonTexturePath);
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr
			|| RoundBadgeTexture == nullptr || TurnTokenFrameTexture == nullptr
			|| SummaryPanelTexture == nullptr || ActionButtonTexture == nullptr)
		{
			UE_LOG(LogTemp, Error,
				TEXT("RD_COMBAT_HUD_ROUND_TURN_REPAIR missing WBP or texture"));
			return;
		}

		Blueprint->Modify();
		Blueprint->WidgetTree->Modify();
		RepairAuthoredRoundTurnLayout(Blueprint, RoundBadgeTexture,
			TurnTokenFrameTexture);
		RepairAuthoredSummaryFrames(Blueprint, SummaryPanelTexture);
		RepairAuthoredSummaryStatusScrolls(Blueprint);
		RepairAuthoredActionButtonArt(Blueprint, ActionButtonTexture);
		PruneStaleVariables(Blueprint);
		if (SaveCompiledBlueprint(Blueprint) == false)
		{
			UE_LOG(LogTemp, Error,
				TEXT("RD_COMBAT_HUD_ROUND_TURN_REPAIR save failed"));
			return;
		}
		UE_LOG(LogTemp, Display,
			TEXT("RD_COMBAT_HUD_ROUND_TURN_REPAIR success"));
	}

	/**
	 * @brief 저작된 Canvas 슬롯 값을 그대로 로그에 찍는다.
	 *
	 * @details 크기를 옛 판(예: #567)으로 되돌리라는 요청이 오면 그 판의 실제
	 * 저작값이 필요하다. 화면 캡처에서 재면 캡처 배율(0.87) 때문에 1~2px씩
	 * 어긋나, 되돌린 값이 원본과 미묘하게 다른 채로 굳는다. 에셋에서 직접
	 * 읽어 찍는다.
	 *
	 * 사용: ``RD.Editor.DumpWidgetSlots [이름조각]`` -- 조각을 주면 이름에
	 * 그 문자열이 든 위젯만 찍는다.
	 */
	void DumpWidgetSlots(const TArray<FString>& Args)
	{
		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, AssetPath);
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("RD_SLOT_DUMP missing WBP"));
			return;
		}
		const FString Filter = Args.IsEmpty() ? FString() : Args[0];
		int32 Printed = 0;
		Blueprint->WidgetTree->ForEachWidget([&Filter, &Printed](UWidget* Widget)
		{
			if (Widget == nullptr)
			{
				return;
			}
			const FString Name = Widget->GetName();
			if (Filter.IsEmpty() == false && Name.Contains(Filter) == false)
			{
				return;
			}
			const UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Widget->Slot);
			if (Slot == nullptr)
			{
				return;
			}
			const FAnchors Anchors = Slot->GetAnchors();
			UE_LOG(LogTemp, Display,
				TEXT("RD_SLOT %s|%s|pos=%.4f,%.4f|size=%.4f,%.4f|")
				TEXT("anchor=%.3f,%.3f,%.3f,%.3f|align=%.3f,%.3f|z=%d|auto=%d"),
				*Name, *Widget->GetClass()->GetName(),
				Slot->GetPosition().X, Slot->GetPosition().Y,
				Slot->GetSize().X, Slot->GetSize().Y,
				Anchors.Minimum.X, Anchors.Minimum.Y,
				Anchors.Maximum.X, Anchors.Maximum.Y,
				Slot->GetAlignment().X, Slot->GetAlignment().Y,
				Slot->GetZOrder(), Slot->GetAutoSize() ? 1 : 0);
			++Printed;
		});
		UE_LOG(LogTemp, Display, TEXT("RD_SLOT_DUMP done count=%d"), Printed);
	}

	/** @brief 다른 HUD 저작값은 보존하고 AP 바의 부모와 좌표만 복구한다. */
	void RepairTurnAPPlacementOnly()
	{
		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, AssetPath);
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("RD_COMBAT_HUD_AP_PLACEMENT missing WBP"));
			return;
		}
		Blueprint->Modify();
		Blueprint->WidgetTree->Modify();
		if (RepairTurnAPPlacement(Blueprint) == false
			|| SaveCompiledBlueprint(Blueprint) == false)
		{
			UE_LOG(LogTemp, Error, TEXT("RD_COMBAT_HUD_AP_PLACEMENT save failed"));
			return;
		}
		UE_LOG(LogTemp, Display,
			TEXT("RD_COMBAT_HUD_AP_PLACEMENT success parent=RootCanvas pos=18,164 size=800,97"));
	}

	/** @brief 다른 WBP 편집은 보존하고 요약판 HP/상태 스크롤 계약만 굽는다. */
	void RepairSummaryStatusScrollOnly()
	{
		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, AssetPath);
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
		{
			UE_LOG(LogTemp, Error,
				TEXT("RD_COMBAT_HUD_STATUS_SCROLL missing WBP"));
			return;
		}
		Blueprint->Modify();
		Blueprint->WidgetTree->Modify();
		RepairAuthoredSummaryStatusScrolls(Blueprint);
		if (SaveCompiledBlueprint(Blueprint) == false)
		{
			UE_LOG(LogTemp, Error,
				TEXT("RD_COMBAT_HUD_STATUS_SCROLL save failed"));
			return;
		}
		UE_LOG(LogTemp, Display,
			TEXT("RD_COMBAT_HUD_STATUS_SCROLL success rows=runtime viewport=140x190"));
	}

	/** @brief 전투 HUD의 불필요한 속도 위젯과 모든 Xxx_Center/Text 범위를 정리한다. */
	void RepairCombatHUDWidgetTreeContracts()
	{
		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, AssetPath);
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
		{
			UE_LOG(LogTemp, Error,
				TEXT("RD_COMBAT_HUD_TREE_CONTRACT missing WBP"));
			return;
		}
		Blueprint->Modify();
		Blueprint->WidgetTree->Modify();
		const int32 Removed = RemoveTurnSpeedWidgets(Blueprint);
		const int32 Normalized = NormalizeCenteredTextBounds(Blueprint);
		PruneStaleVariables(Blueprint);
		if (SaveCompiledBlueprint(Blueprint) == false)
		{
			UE_LOG(LogTemp, Error,
				TEXT("RD_COMBAT_HUD_TREE_CONTRACT save failed"));
			return;
		}
		UE_LOG(LogTemp, Display,
			TEXT("RD_COMBAT_HUD_TREE_CONTRACT success removed=%d normalized=%d"),
			Removed, Normalized);
	}

	/** @brief 공용 상세판을 모든 호출 화면에서 동일하게 반응형으로 만든다. */
	void RepairDetailResponsiveOnly()
	{
		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(
			nullptr, DetailOverlayAssetPath);
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
		{
			UE_LOG(LogTemp, Error,
				TEXT("RD_COMBAT_DETAIL_RESPONSIVE_REPAIR missing WBP"));
			return;
		}
		Blueprint->Modify();
		Blueprint->WidgetTree->Modify();
		if (RepairResponsiveDetailTree(Blueprint) == false
			|| SaveCompiledBlueprint(Blueprint) == false)
		{
			UE_LOG(LogTemp, Error,
				TEXT("RD_COMBAT_DETAIL_RESPONSIVE_REPAIR save failed"));
			return;
		}
		UE_LOG(LogTemp, Display,
			TEXT("RD_COMBAT_DETAIL_RESPONSIVE_REPAIR success"));
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
		RepairAuthoredActionButtonLabels(Blueprint);
		NormalizeCenteredTextBounds(Blueprint);
		if (SaveCompiledBlueprint(Blueprint) == false)
		{
			UE_LOG(LogTemp, Error,
				TEXT("RD_COMBAT_HUD_ACTION_BUTTON_ART_REPAIR save failed"));
			return;
		}
		UE_LOG(LogTemp, Display,
			TEXT("RD_COMBAT_HUD_ACTION_BUTTON_ART_REPAIR success plates=2 labels=3"));
	}

	/** @brief ROUND/턴바/용병 판을 보존하고 #567 우측 HUD만 복구한다. */
	void RepairRightHUDOnly()
	{
		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, AssetPath);
		UTexture2D* SummaryVerticalTexture = EnsureSummaryVerticalTexture();
		UTexture2D* OptionsRailFrameTexture = LoadObject<UTexture2D>(nullptr,
			OptionsRailFrameTexturePath);
		UTexture2D* ActionButtonTexture = LoadObject<UTexture2D>(nullptr,
			ActionButtonTexturePath);
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr
			|| SummaryVerticalTexture == nullptr
			|| OptionsRailFrameTexture == nullptr || ActionButtonTexture == nullptr)
		{
			UE_LOG(LogTemp, Error,
				TEXT("RD_COMBAT_HUD_RIGHT_REPAIR missing WBP or texture"));
			return;
		}

		Blueprint->Modify();
		Blueprint->WidgetTree->Modify();
		RepairAuthoredSummaryFrames(Blueprint, SummaryVerticalTexture);
		RepairAuthoredRightHUDLayout(Blueprint, OptionsRailFrameTexture);
		RepairAuthoredPrimaryCombatControls(Blueprint);
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
			Name->SetText(FText::Format(
				NSLOCTEXT("CombatHUD", "MercenaryPreviewName", "용병 {0}"), Index + 1));
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
			{ TEXT("MercenaryDetailName"), FVector2D(1080.f, 230.f), FVector2D(620.f, 82.f), TEXT(""), 42 },
		};
		for (const FDetailText& Detail : Details)
		{
			UTextBlock* Text = FindOrCreate<UTextBlock>(Blueprint, FName(Detail.Name));
			PlaceCanvas(DetailSection, Text, Detail.Position, Detail.Size, 8);
			Text->SetText(FCString::Strlen(Detail.Preview) > 0
				? FText::FromString(Detail.Preview)
				: NSLOCTEXT("CombatHUD", "MercenaryDetailPreview", "용병"));
			SetReadableFont(Text, BaseFont, Detail.FontSize);
			Text->SetJustification(ETextJustify::Left);
		}

		// 수치 칩 셋. 이름 바로 밑에 가로로 놓는다.
		const TCHAR* const ChipValueNames[3] = {
			TEXT("MercenaryDetailHP"), TEXT("MercenaryDetailAP"),
			TEXT("MercenaryDetailSpeed") };
		const FText ChipLabels[3] = { FText::FromString(TEXT("HP")),
			FText::FromString(TEXT("AP")),
			NSLOCTEXT("CombatHUD", "ChipSpeed", "속도") };
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
			Label->SetText(ChipLabels[Index]);
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
		UImage* CritIcon = FindOrCreate<UImage>(Blueprint, TEXT("MercenaryCritIcon"));
		PlaceCanvas(DetailSection, CritIcon, FVector2D(1106.f, 586.f),
			FVector2D(46.f, 46.f), 9);
		CritIcon->SetBrushFromTexture(EnsureCriticalIconTexture(), false);
		CritIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UTextBlock* CritLabel = FindOrCreate<UTextBlock>(Blueprint, TEXT("MercenaryCritLabel"));
		PlaceCanvas(DetailSection, CritLabel, FVector2D(1162.f, 580.f),
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
				: FText::Format(
					NSLOCTEXT("CombatHUD", "MercenarySkillPreview", "스킬 {0}"), Index));
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
		UTexture2D* TurnTokenFrameTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/UI/Generated/CombatHUD/T_MB_TurnToken_Frame_NoSpeed_v1.T_MB_TurnToken_Frame_NoSpeed_v1"));
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
		UTexture2D* EnemyPanelTexture = EnsureSummaryVerticalTexture();
		UTexture2D* StatusSlotTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_StatusSlot_Frame.T_MB_StatusSlot_Frame"));
		if (MercenaryTexture == nullptr || MonsterTexture == nullptr
			|| TurnTokenFrameTexture == nullptr
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
			TurnTokenFrameTexture);
		RepairAuthoredActionButtonArt(Blueprint, ActionButtonTexture);
		RepairAuthoredSummaryFrames(Blueprint, EnemyPanelTexture);
		RepairAuthoredMercenaryCriticalRow(Blueprint,
			MercenaryDescriptionPlateTexture);
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
		RepairAuthoredPrimaryCombatControls(Blueprint);

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
			TEXT("RD_COMBAT_HUD_BUILD success menu_icons=4 turn_frames=10 speed_rows=0"));
	}
}

void RegisterCombatHUDWidgetBuilderCommands()
{
	using namespace CombatHUDWidgetBuilder;
	BuildCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.BuildCombatHUDAdditions"),
		TEXT("Build Marchbound combat HUD additions without obsolete turn-speed widgets."),
		FConsoleCommandDelegate::CreateStatic(&Build));
	InventoryBuildCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.BuildCombatHUDInventoryTab"),
		TEXT("Add only the WBP-authored inventory tab below the mercenary roster."),
		FConsoleCommandDelegate::CreateStatic(&BuildInventoryTabOnly));
	RoundTurnRepairCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.RepairCombatHUDRoundTurn"),
		TEXT("Apply the double ROUND badge, no-speed turn cards, and no-footer summaries."),
		FConsoleCommandDelegate::CreateStatic(&RepairRoundTurnOnly));
	ActionButtonArtRepairCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.RepairCombatHUDActionButtonArt"),
		TEXT("Restore action art and fill the Skill/Confirm/End Turn label bounds."),
		FConsoleCommandDelegate::CreateStatic(&RepairActionButtonArtOnly));
	RightHUDRepairCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.RepairCombatHUDRightLayout"),
		TEXT("Restore only the #567-sized options, summaries, and action buttons."),
		FConsoleCommandDelegate::CreateStatic(&RepairRightHUDOnly));
	MercenaryPortraitFrameRepairCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.RepairCombatHUDMercenaryPortraitFrame"),
		TEXT("Restore only the exact KitA portrait frame in inline and modular mercenary panels."),
		FConsoleCommandDelegate::CreateStatic(&RepairMercenaryPortraitFrameOnly));
	DetailResponsiveRepairCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.RepairCombatDetailResponsive"),
		TEXT("Wrap the shared combat detail board in one responsive 1920x1080 ScaleBox."),
		FConsoleCommandDelegate::CreateStatic(&RepairDetailResponsiveOnly));
	WidgetTreeContractRepairCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.RepairCombatHUDWidgetTreeContracts"),
		TEXT("Remove obsolete turn-speed widgets and make every Xxx text fill Xxx_Center."),
		FConsoleCommandDelegate::CreateStatic(&RepairCombatHUDWidgetTreeContracts));
	APPlacementRepairCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.RepairCombatHUDAPPlacement"),
		TEXT("Reparent the combat AP bar to RootCanvas and restore its authored bounds."),
		FConsoleCommandDelegate::CreateStatic(&RepairTurnAPPlacementOnly));
	StatusScrollRepairCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.RepairCombatHUDStatusScroll"),
		TEXT("Bake only the summary HP visibility and runtime status scroll viewport."),
		FConsoleCommandDelegate::CreateStatic(&RepairSummaryStatusScrollOnly));
	SlotDumpCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.DumpWidgetSlots"),
		TEXT("Log authored canvas-slot values of combat HUD widgets (optional name filter)."),
		FConsoleCommandWithArgsDelegate::CreateStatic(&DumpWidgetSlots));
}

void UnregisterCombatHUDWidgetBuilderCommands()
{
	CombatHUDWidgetBuilder::BuildCommand.Reset();
	CombatHUDWidgetBuilder::InventoryBuildCommand.Reset();
	CombatHUDWidgetBuilder::RoundTurnRepairCommand.Reset();
	CombatHUDWidgetBuilder::ActionButtonArtRepairCommand.Reset();
	CombatHUDWidgetBuilder::RightHUDRepairCommand.Reset();
	CombatHUDWidgetBuilder::MercenaryPortraitFrameRepairCommand.Reset();
	CombatHUDWidgetBuilder::DetailResponsiveRepairCommand.Reset();
	CombatHUDWidgetBuilder::WidgetTreeContractRepairCommand.Reset();
	CombatHUDWidgetBuilder::APPlacementRepairCommand.Reset();
	CombatHUDWidgetBuilder::StatusScrollRepairCommand.Reset();
	CombatHUDWidgetBuilder::SlotDumpCommand.Reset();
}
