#include "UI/MarchboundHireWidgetBuilder.h"
#include "UI/UIPartRects.h"
#include "UI/UIFont.h"

#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "HAL/IConsoleManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "WidgetBlueprintEditorUtils.h"
#include "Brushes/SlateColorBrush.h"
#include "UObject/SavePackage.h"

namespace MarchboundHireWidgetBuilder
{
	constexpr TCHAR AssetPath[] =
		TEXT("/Game/UI/CombatLayouts/WBP_MercenaryHire_Marchbound.WBP_MercenaryHire_Marchbound");
	// Each screen region owns its own mobile-responsive scale box.
	const FVector2D DesignSize(1920.0f, 1080.0f);
	TUniquePtr<FAutoConsoleCommand> BuildCommand;
	TUniquePtr<FAutoConsoleCommand> ButtonLabelRepairCommand;
	TUniquePtr<FAutoConsoleCommand> TextTranslationRepairCommand;

	template <typename T>
	T* FindOrCreate(UWidgetBlueprint* Blueprint, const FName Name)
	{
		if (UWidget* Existing = Blueprint->WidgetTree->FindWidget(Name))
		{
			T* Typed = Cast<T>(Existing);
			checkf(Typed != nullptr, TEXT("%s is not %s"),
				*Name.ToString(), *T::StaticClass()->GetName());
			return Typed;
		}
		T* NewWidget = Blueprint->WidgetTree->ConstructWidget<T>(T::StaticClass(), Name);
		Blueprint->OnVariableAdded(Name);
		return NewWidget;
	}

	void EnsureParent(UPanelWidget* Parent, UWidget* Child)
	{
		check(Parent != nullptr);
		check(Child != nullptr);
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

	void DeleteWidgetIfPresent(UWidgetBlueprint* Blueprint, const FName Name)
	{
		if (UWidget* Widget = Blueprint->WidgetTree->FindWidget(Name))
		{
			TSet<UWidget*> Widgets;
			Widgets.Add(Widget);
			FWidgetBlueprintEditorUtils::DeleteWidgets(Blueprint, MoveTemp(Widgets),
				FWidgetBlueprintEditorUtils::EDeleteWidgetWarningType::DeleteSilently);
		}
	}

	/** 삭제한 옛 배경 위젯의 변수 GUID가 컴파일러 맵에 남지 않게 정리한다. */
	void PruneStaleVariables(UWidgetBlueprint* Blueprint)
	{
		TSet<FName> LiveNames;
		Blueprint->WidgetTree->ForEachWidget([&LiveNames](UWidget* Widget)
		{
			if (Widget != nullptr)
			{
				LiveNames.Add(Widget->GetFName());
			}
		});
		TArray<FName> StaleNames;
		for (const TPair<FName, FGuid>& Entry : Blueprint->WidgetVariableNameToGuidMap)
		{
			if (!LiveNames.Contains(Entry.Key))
			{
				StaleNames.Add(Entry.Key);
			}
		}
		for (const FName& StaleName : StaleNames)
		{
			Blueprint->WidgetVariableNameToGuidMap.Remove(StaleName);
			Blueprint->OnVariableRemoved(StaleName);
		}
	}

	/** 재사용한 기존 위젯도 컴파일러 변수 GUID를 반드시 갖게 한다. */
	void RepairLiveVariableGuids(UWidgetBlueprint* Blueprint)
	{
		Blueprint->WidgetTree->ForEachWidget([Blueprint](UWidget* Widget)
		{
			if (Widget != nullptr && Widget->bIsVariable
				&& !Blueprint->WidgetVariableNameToGuidMap.Contains(Widget->GetFName()))
			{
				Blueprint->OnVariableAdded(Widget->GetFName());
			}
		});
	}

	void PlaceCanvas(UPanelWidget* Parent, UWidget* Child,
		const FVector2D Position, const FVector2D Size, const int32 ZOrder)
	{
		EnsureParent(Parent, Child);
		UCanvasPanelSlot* Slot = CastChecked<UCanvasPanelSlot>(Child->Slot);
		Slot->SetAnchors(FAnchors(0.0f));
		Slot->SetAlignment(FVector2D::ZeroVector);
		Slot->SetAutoSize(false);
		Slot->SetPosition(Position);
		Slot->SetSize(Size);
		Slot->SetZOrder(ZOrder);
	}

	void PlaceCenteredText(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent,
		UTextBlock* Text, const FVector2D Position, const FVector2D Size,
		const int32 ZOrder)
	{
		const FName CenterName(*FString::Printf(TEXT("%s_Center"), *Text->GetName()));
		UOverlay* Center = FindOrCreate<UOverlay>(Blueprint, CenterName);
		PlaceCanvas(Parent, Center, Position, Size, ZOrder);
		const FName FitName(*FString::Printf(TEXT("%s_AutoFit"), *Text->GetName()));
		UScaleBox* Fit = FindOrCreate<UScaleBox>(Blueprint, FitName);
		Fit->SetStretch(EStretch::ScaleToFitX);
		Fit->SetStretchDirection(EStretchDirection::DownOnly);
		Fit->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		Fit->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		EnsureParent(Center, Fit);
		UOverlaySlot* FitSlot = CastChecked<UOverlaySlot>(Fit->Slot);
		FitSlot->SetPadding(FMargin(0.0f));
		FitSlot->SetHorizontalAlignment(HAlign_Fill);
		FitSlot->SetVerticalAlignment(VAlign_Fill);

		EnsureParent(Fit, Text);
		UScaleBoxSlot* TextSlot = CastChecked<UScaleBoxSlot>(Text->Slot);
		TextSlot->SetHorizontalAlignment(HAlign_Center);
		TextSlot->SetVerticalAlignment(VAlign_Center);
		Text->SetJustification(ETextJustify::Center);
		// 이 빌더는 기존 WBP를 재사용한다. 옛 시안에서 넣은 여백과 렌더
		// 이동을 초기화하지 않으면 재빌드할 때마다 수학적 중앙과 글리프가
		// 서로 다른 위치에 남는다. 아래 optical offset만 유일한 보정값이다.
		Text->SetMargin(FMargin(0.0f));
		Text->SetRenderTransform(FWidgetTransform());
		Text->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	}

	void PlaceFittedCenteredText(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent,
		UTextBlock* Text, const FVector2D Position, const FVector2D Size,
		const int32 ZOrder)
	{
		const FName CenterName(*FString::Printf(TEXT("%s_Center"), *Text->GetName()));
		UOverlay* Center = FindOrCreate<UOverlay>(Blueprint, CenterName);
		PlaceCanvas(Parent, Center, Position, Size, ZOrder);
		// 세로로는 자르지 않는다. 배율이 이미 칸 안에 맞춰 주므로, 자르기가
		// 깎는 것은 아래꼬리와 외곽선뿐이다(0824 검수: "글자 짤리는거").
		Center->SetClipping(EWidgetClipping::Inherit);

		const FName FitName(*FString::Printf(TEXT("%s_Fit"), *Text->GetName()));
		UScaleBox* Fit = FindOrCreate<UScaleBox>(Blueprint, FitName);
		Fit->SetStretch(EStretch::ScaleToFitX);
		Fit->SetStretchDirection(EStretchDirection::DownOnly);
		Fit->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		Fit->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		EnsureParent(Center, Fit);
		UOverlaySlot* FitSlot = CastChecked<UOverlaySlot>(Fit->Slot);
		FitSlot->SetPadding(FMargin(0.0f));
		FitSlot->SetHorizontalAlignment(HAlign_Fill);
		FitSlot->SetVerticalAlignment(VAlign_Fill);

		EnsureParent(Fit, Text);
		UScaleBoxSlot* TextSlot = CastChecked<UScaleBoxSlot>(Text->Slot);
		TextSlot->SetHorizontalAlignment(HAlign_Center);
		TextSlot->SetVerticalAlignment(VAlign_Center);
		Text->SetJustification(ETextJustify::Center);
		Text->SetMargin(FMargin(0.0f));
		Text->SetRenderTransform(FWidgetTransform());
		Text->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	}

	void SetTransparentButton(UButton* Button);

	/**
	 * 단일 문구 버튼은 아트, 라벨 레이아웃, 클릭 영역이 같은 사각을 쓴다.
	 * 글꼴의 ascent/descent를 좌표로 상쇄하지 않고 Overlay가 문구를 중앙에
	 * 배치하게 하므로 문자열과 언어가 바뀌어도 별도 Y 보정이 필요 없다.
	 */
	void FillSingleLabelButton(UWidgetBlueprint* Blueprint, UCanvasPanel* Holder,
		UTextBlock* Label, UButton* Button, const FVector2D Size)
	{
		PlaceCenteredText(Blueprint, Holder, Label,
			FVector2D::ZeroVector, Size, 15);
		PlaceCanvas(Holder, Button, FVector2D::ZeroVector, Size, 30);
		SetTransparentButton(Button);
	}

	void StretchCanvas(UCanvasPanel* Parent, UWidget* Child,
		const FAnchors Anchors, const int32 ZOrder)
	{
		EnsureParent(Parent, Child);
		UCanvasPanelSlot* Slot = CastChecked<UCanvasPanelSlot>(Child->Slot);
		Slot->SetAnchors(Anchors);
		Slot->SetAlignment(FVector2D::ZeroVector);
		Slot->SetAutoSize(false);
		Slot->SetOffsets(FMargin(0.0f));
		Slot->SetZOrder(ZOrder);
	}

	UTexture2D* Texture(const TCHAR* Path)
	{
		UTexture2D* Result = LoadObject<UTexture2D>(nullptr, Path);
		checkf(Result != nullptr, TEXT("Missing Marchbound UI texture: %s"), Path);
		return Result;
	}

	void SetImage(UImage* Image, UTexture2D* Source)
	{
		Image->SetBrushFromTexture(Source, false);
		Image->SetColorAndOpacity(FLinearColor::White);
		Image->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	void SetFont(UTextBlock* Text, const FSlateFontInfo& Template, const int32 Size)
	{
		Text->SetFont(UIFont::Make(Template, Size));
		Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.12f, 0.065f, 0.025f, 1.0f)));
		Text->SetShadowOffset(FVector2D(1.0f, 1.0f));
		Text->SetShadowColorAndOpacity(FLinearColor(1.0f, 0.86f, 0.60f, 0.35f));
		Text->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	void SetLightFont(UTextBlock* Text, const FSlateFontInfo& Template, const int32 Size)
	{
		Text->SetFont(UIFont::Make(Template, Size));
		Text->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.91f, 0.73f, 1.0f)));
		Text->SetShadowOffset(FVector2D(2.0f, 2.0f));
		Text->SetShadowColorAndOpacity(FLinearColor(0.03f, 0.012f, 0.004f, 0.95f));
		Text->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	void SetTransparentButton(UButton* Button)
	{
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
		Button->SetVisibility(ESlateVisibility::Visible);
	}

	UImage* AddImage(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent,
		const FName Name, UTexture2D* Source, const FVector2D Position,
		const FVector2D Size, const int32 ZOrder)
	{
		UImage* Result = FindOrCreate<UImage>(Blueprint, Name);
		PlaceCanvas(Parent, Result, Position, Size, ZOrder);
		SetImage(Result, Source);
		return Result;
	}

	UTextBlock* AddText(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent,
		const FName Name, const FText& Value, const FSlateFontInfo& Font,
		const int32 FontSize, const FVector2D Position, const FVector2D Size,
		const int32 ZOrder, const ETextJustify::Type Justify = ETextJustify::Center)
	{
		UTextBlock* Result = FindOrCreate<UTextBlock>(Blueprint, Name);
		PlaceCenteredText(Blueprint, Parent, Result, Position, Size, ZOrder);
		Result->SetText(Value);
		Result->SetJustification(Justify);
		SetFont(Result, Font, FontSize);
		return Result;
	}

	bool SaveCompiledBlueprint(UWidgetBlueprint* Blueprint)
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		return UPackage::SavePackage(Blueprint->GetPackage(), Blueprint,
			*FPackageName::LongPackageNameToFilename(
				Blueprint->GetOutermost()->GetName(),
				FPackageName::GetAssetPackageExtension()), FSavePackageArgs());
	}

	/** 기존 WBP의 단일 문구 버튼만 수술해 디자이너의 다른 배치를 보존한다. */
	void RepairSingleLabelButtonsOnly()
	{
		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, AssetPath);
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("RD_MB_HIRE_BUTTON_LABEL_REPAIR missing %s"),
				AssetPath);
			return;
		}

		struct FButtonContract
		{
			FName Holder;
			FName Label;
			FName Button;
			FVector2D Size;
		};
		const FButtonContract Contracts[] = {
			{ TEXT("HireAddHolder"), TEXT("HireAddLabel"),
				TEXT("HireAddButton"), FVector2D(270.f, 106.f) },
			{ TEXT("DepartHolder"), TEXT("DepartLabel"),
				TEXT("DepartButton"), FVector2D(224.f, 106.f) },
			{ TEXT("HireBackHolder"), TEXT("HireBackLabel"),
				TEXT("HireBackButton"), FVector2D(270.f, 106.f) },
		};

		Blueprint->Modify();
		Blueprint->WidgetTree->Modify();
		for (const FButtonContract& Contract : Contracts)
		{
			UCanvasPanel* Holder = CastChecked<UCanvasPanel>(
				Blueprint->WidgetTree->FindWidget(Contract.Holder));
			UTextBlock* Label = CastChecked<UTextBlock>(
				Blueprint->WidgetTree->FindWidget(Contract.Label));
			UButton* Button = CastChecked<UButton>(
				Blueprint->WidgetTree->FindWidget(Contract.Button));
			FillSingleLabelButton(Blueprint, Holder, Label, Button, Contract.Size);
		}

		if (SaveCompiledBlueprint(Blueprint) == false)
		{
			UE_LOG(LogTemp, Error, TEXT("RD_MB_HIRE_BUTTON_LABEL_REPAIR save failed"));
			return;
		}
		UE_LOG(LogTemp, Display,
			TEXT("RD_MB_HIRE_BUTTON_LABEL_REPAIR success buttons=3"));
	}

	/** 수동 광학 보정값을 지우고 각 Center/AutoFit의 정렬만 사용한다. */
	void RepairTextRenderTranslationsOnly()
	{
		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, AssetPath);
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
		{
			UE_LOG(LogTemp, Error,
				TEXT("RD_MB_HIRE_TEXT_TRANSLATION_REPAIR missing %s"), AssetPath);
			return;
		}

		Blueprint->Modify();
		Blueprint->WidgetTree->Modify();
		int32 ResetCount = 0;
		Blueprint->WidgetTree->ForEachWidget([&ResetCount](UWidget* Widget)
		{
			if (UTextBlock* Text = Cast<UTextBlock>(Widget))
			{
				if (!Text->GetRenderTransform().Translation.IsNearlyZero())
				{
					++ResetCount;
				}
				Text->SetRenderTranslation(FVector2D::ZeroVector);
			}
		});

		if (SaveCompiledBlueprint(Blueprint) == false)
		{
			UE_LOG(LogTemp, Error,
				TEXT("RD_MB_HIRE_TEXT_TRANSLATION_REPAIR save failed"));
			return;
		}
		UE_LOG(LogTemp, Display,
			TEXT("RD_MB_HIRE_TEXT_TRANSLATION_REPAIR success reset=%d"),
			ResetCount);
	}

	void Build()
	{
		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, AssetPath);
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("RD_MB_HIRE_BUILD missing %s"), AssetPath);
			return;
		}

		Blueprint->Modify();
		Blueprint->WidgetTree->Modify();
		UCanvasPanel* Root = Cast<UCanvasPanel>(
			Blueprint->WidgetTree->FindWidget(TEXT("RootCanvas")));
		if (Root == nullptr)
		{
			Root = Cast<UCanvasPanel>(Blueprint->WidgetTree->RootWidget);
		}
		if (Root == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("RD_MB_HIRE_BUILD RootCanvas is not a CanvasPanel"));
			return;
		}

		// 배경을 제거한 전경 일러스트는 전신이 잘리지 않도록 화면 높이에 맞춘다.
		// 초광폭 화면의 좌우는 직업마다 따로 제작한 색상 배경으로 채운다.
		// UI는 한 장짜리 1920x1080 프레임으로 고정하지 않고 좌/중/우 영역이
		// 각자 화면 폭에 맞춰 줄어든다.
		UOverlay* ViewportRoot = FindOrCreate<UOverlay>(Blueprint, TEXT("HireViewportRoot"));
		if (UWidget* LegacyUIScale = Blueprint->WidgetTree->FindWidget(TEXT("HireUIScale")))
		{
			if (UPanelWidget* Parent = LegacyUIScale->GetParent())
			{
				Parent->RemoveChild(LegacyUIScale);
			}
			LegacyUIScale->SetVisibility(ESlateVisibility::Collapsed);
		}
		EnsureParent(ViewportRoot, Root);
		Blueprint->WidgetTree->RootWidget = ViewportRoot;

		UTextBlock* FontSource = FindOrCreate<UTextBlock>(Blueprint, TEXT("HireName_0"));
		const FSlateFontInfo Font = FontSource->GetFont();

		UTexture2D* KnightHeroCutout = Texture(
			TEXT("/Game/UI/MercenaryHire/HeroCutouts/T_HireHeroCutout_Knight_v1.T_HireHeroCutout_Knight_v1"));
		UTexture2D* KnightGeneratedBackground = Texture(
			TEXT("/Game/UI/MercenaryHire/GeneratedBackgrounds/T_HireGeneratedBG_Knight_v1.T_HireGeneratedBG_Knight_v1"));
		UTexture2D* ListFrame = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Hire/T_MB_HireListFrame.T_MB_HireListFrame"));
		UTexture2D* RowNormal = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Hire/T_MB_HireRowNormal.T_MB_HireRowNormal"));
		UTexture2D* RowSelected = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Hire/T_MB_HireRowSelected.T_MB_HireRowSelected"));
		UTexture2D* BackPlate = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Hire/T_MB_HireBackButton.T_MB_HireBackButton"));
		UTexture2D* TitlePlate = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Hire/T_MB_HireTitlePlate.T_MB_HireTitlePlate"));
		UTexture2D* PartyFrame = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Hire/T_MB_HirePartyFrame.T_MB_HirePartyFrame"));
		UTexture2D* PartyPlus = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Hire/T_MB_HirePartyRowPlus.T_MB_HirePartyRowPlus"));
		UTexture2D* PartyEmpty = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Hire/T_MB_HirePartyRowEmpty.T_MB_HirePartyRowEmpty"));
		UTexture2D* DepartPlate = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Hire/T_MB_HireDepartButton.T_MB_HireDepartButton"));
		UTexture2D* NamePlate = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Hire/T_MB_HireNamePlate.T_MB_HireNamePlate"));
		UTexture2D* StatsStrip = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Hire/T_MB_HireStatsStrip.T_MB_HireStatsStrip"));
		UTexture2D* SkillFrame = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Hire/T_MB_HireSkillButtonFrame.T_MB_HireSkillButtonFrame"));

		UCanvasPanel* OldBackdrop = FindOrCreate<UCanvasPanel>(Blueprint, TEXT("Backdrop"));
		OldBackdrop->SetVisibility(ESlateVisibility::Collapsed);
		if (UWidget* LegacyBoard = Blueprint->WidgetTree->FindWidget(TEXT("Board")))
		{
			LegacyBoard->SetVisibility(ESlateVisibility::Collapsed);
		}
		// 텍스처가 아직 없거나 로드에 실패한 한두 프레임에만 보이는 안전색.
		// 정상 경로에서는 아래의 전용 생성 배경이 화면 전체를 덮는다.
		const FLinearColor MissingArtFallbackColor(0.008f, 0.016f, 0.027f, 1.0f);
		UBorder* BackgroundFill = FindOrCreate<UBorder>(
			Blueprint, TEXT("HireBackgroundLetterboxFill"));
		BackgroundFill->SetBrush(FSlateColorBrush(MissingArtFallbackColor));
		BackgroundFill->SetBrushColor(FLinearColor::White);
		BackgroundFill->SetPadding(FMargin(0.0f));
		BackgroundFill->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		// 이전의 같은 원화 확대/단계 페더 레이어는 삭제한다. 숨기기만 하면 옛
		// 하위 위젯과 하드 레퍼런스가 WBP에 남아 다시 cook될 수 있다.
		DeleteWidgetIfPresent(Blueprint, TEXT("HireBackgroundAmbientScale"));
		DeleteWidgetIfPresent(Blueprint, TEXT("HireBackgroundEdgeFade"));
		UScaleBox* GeneratedScale = FindOrCreate<UScaleBox>(
			Blueprint, TEXT("HireGeneratedBackgroundScale"));
		GeneratedScale->SetStretch(EStretch::ScaleToFill);
		GeneratedScale->SetStretchDirection(EStretchDirection::Both);
		GeneratedScale->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		UImage* GeneratedArt = FindOrCreate<UImage>(
			Blueprint, TEXT("HireGeneratedBackgroundArt"));
		EnsureParent(GeneratedScale, GeneratedArt);
		SetImage(GeneratedArt, KnightGeneratedBackground);

		UScaleBox* BackgroundScale = FindOrCreate<UScaleBox>(Blueprint, TEXT("HireBackgroundScale"));
		BackgroundScale->SetStretch(EStretch::ScaleToFit);
		BackgroundScale->SetStretchDirection(EStretchDirection::Both);
		BackgroundScale->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		UImage* BackgroundArt = FindOrCreate<UImage>(Blueprint, TEXT("Backdrop_Art"));
		EnsureParent(BackgroundScale, BackgroundArt);
		SetImage(BackgroundArt, KnightHeroCutout);
		// Overlay의 자식 순서가 곧 그리기 순서다. 배경을 먼저, 반응형 UI 캔버스를
		// 나중에 다시 넣어 재빌드해도 항상 같은 계층과 Z 순서를 보장한다.
		if (UPanelWidget* Parent = BackgroundFill->GetParent())
		{
			Parent->RemoveChild(BackgroundFill);
		}
		if (UPanelWidget* Parent = BackgroundScale->GetParent())
		{
			Parent->RemoveChild(BackgroundScale);
		}
		if (UPanelWidget* Parent = GeneratedScale->GetParent())
		{
			Parent->RemoveChild(GeneratedScale);
		}
		if (UPanelWidget* Parent = Root->GetParent())
		{
			Parent->RemoveChild(Root);
		}
		ViewportRoot->AddChild(BackgroundFill);
		if (UOverlaySlot* Slot = Cast<UOverlaySlot>(BackgroundFill->Slot))
		{
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Fill);
		}
		ViewportRoot->AddChild(GeneratedScale);
		if (UOverlaySlot* Slot = Cast<UOverlaySlot>(GeneratedScale->Slot))
		{
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Fill);
		}
		ViewportRoot->AddChild(BackgroundScale);
		if (UOverlaySlot* Slot = Cast<UOverlaySlot>(BackgroundScale->Slot))
		{
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Fill);
		}
		ViewportRoot->AddChild(Root);
		if (UOverlaySlot* Slot = Cast<UOverlaySlot>(Root->Slot))
		{
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Fill);
		}

		// 각 영역은 자기 디자인 크기를 유지하면서 할당된 화면 비율 안에서만
		// 축소/확대된다. 16:9, 20:9, 정사각형 창에서도 서로를 밀어내지 않는다.
		auto MakeRegion = [Blueprint, Root](const FName ScaleName,
			const FName SizeName, const FName CanvasName, const FVector2D RegionDesignSize,
			const FAnchors RegionAnchors, const int32 ZOrder)
		{
			UScaleBox* Scale = FindOrCreate<UScaleBox>(Blueprint, ScaleName);
			Scale->SetStretch(EStretch::ScaleToFit);
			Scale->SetStretchDirection(EStretchDirection::Both);
			USizeBox* Size = FindOrCreate<USizeBox>(Blueprint, SizeName);
			Size->SetWidthOverride(RegionDesignSize.X);
			Size->SetHeightOverride(RegionDesignSize.Y);
			UCanvasPanel* Canvas = FindOrCreate<UCanvasPanel>(Blueprint, CanvasName);
			EnsureParent(Size, Canvas);
			EnsureParent(Scale, Size);
			StretchCanvas(Root, Scale, RegionAnchors, ZOrder);
			return Canvas;
		};

		UCanvasPanel* LeftRegion = MakeRegion(TEXT("HireLeftScale"), TEXT("HireLeftSize"),
			TEXT("HireLeftRegion"), FVector2D(555.0f, 1080.0f),
			FAnchors(0.0f, 0.0f, 0.30f, 1.0f), 10);
		UCanvasPanel* CenterRegion = MakeRegion(TEXT("HireCenterScale"), TEXT("HireCenterSize"),
			TEXT("HireCenterRegion"), FVector2D(845.0f, 1080.0f),
			FAnchors(0.27f, 0.0f, 0.73f, 1.0f), 20);
		UCanvasPanel* RightRegion = MakeRegion(TEXT("HireRightScale"), TEXT("HireRightSize"),
			TEXT("HireRightRegion"), FVector2D(520.0f, 1080.0f),
			FAnchors(0.70f, 0.0f, 1.0f, 1.0f), 10);

		// Each regional ScaleBox centers its 1080-high design board when the viewport
		// is taller than the allocated region. Keep semantic groups separate so the
		// runtime can leave the two side columns centered, move central stats/skills
		// above the bottom action, and pin actions without rewriting child coordinates.
		UCanvasPanel* LeftTopGroup = FindOrCreate<UCanvasPanel>(
			Blueprint, TEXT("HireLeftTopGroup"));
		PlaceCanvas(LeftRegion, LeftTopGroup, FVector2D::ZeroVector,
			FVector2D(555.0f, 1080.0f), 5);
		LeftTopGroup->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UCanvasPanel* CenterTopGroup = FindOrCreate<UCanvasPanel>(
			Blueprint, TEXT("HireCenterTopGroup"));
		PlaceCanvas(CenterRegion, CenterTopGroup, FVector2D::ZeroVector,
			FVector2D(845.0f, 1080.0f), 5);
		CenterTopGroup->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UCanvasPanel* CenterMiddleGroup = FindOrCreate<UCanvasPanel>(
			Blueprint, TEXT("HireCenterMiddleGroup"));
		PlaceCanvas(CenterRegion, CenterMiddleGroup, FVector2D::ZeroVector,
			FVector2D(845.0f, 1080.0f), 6);
		CenterMiddleGroup->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UCanvasPanel* RightTopGroup = FindOrCreate<UCanvasPanel>(
			Blueprint, TEXT("HireRightTopGroup"));
		PlaceCanvas(RightRegion, RightTopGroup, FVector2D::ZeroVector,
			FVector2D(520.0f, 1080.0f), 5);
		RightTopGroup->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		const FVector2D ListPos(55.0f, 100.0f);
		const FVector2D ListSize(500.0f, 850.0f);
		AddImage(Blueprint, LeftTopGroup, TEXT("HireListFrameArt"), ListFrame,
			ListPos, ListSize, 0);
		const FBox2D ListInner = UIPartRects::Inner(TEXT("T_MB_HireListFrame"),
			ListPos, ListSize, false);

		UCanvasPanel* TitlePanel = FindOrCreate<UCanvasPanel>(Blueprint, TEXT("HireTitlePanel"));
		PlaceCanvas(CenterTopGroup, TitlePanel, FVector2D(190.0f, 18.0f), FVector2D(430.0f, 106.0f), 30);
		AddImage(Blueprint, TitlePanel, TEXT("HireTitleArt"), TitlePlate,
			FVector2D::ZeroVector, FVector2D(430.0f, 106.0f), 0);
		UTextBlock* TitleText = AddText(Blueprint, TitlePanel, TEXT("HireTitleText"),
			NSLOCTEXT("MarchboundHire", "Title", "용병 선택"), Font, 42,
			FVector2D(30.0f, 20.0f), FVector2D(370.0f, 64.0f), 10);
		SetLightFont(TitleText, Font, 42);
		// 화면의 용도가 좌우 후보/파티로 이미 분명하고, 이 판은 영웅 머리만
		// 가린다. 자식 계약은 보존하되 패널 전체를 표시하지 않는다.
		TitlePanel->SetVisibility(ESlateVisibility::Collapsed);

		const FVector2D CardSize(420.0f, 116.0f);
		const TCHAR* DefaultNames[6] = {
			TEXT("기사"), TEXT("마법사"), TEXT("레인저"),
			TEXT("도적"), TEXT("야만전사"), TEXT("드루이드")
		};
		const TCHAR* DefaultRoles[6] = {
			TEXT("방패 탱커 · 근접"), TEXT("주문 술사 · 원거리"), TEXT("명사수 · 원거리"),
			TEXT("기습 암살자 · 근접"), TEXT("광전사 · 근접"), TEXT("자연 술사 · 지원")
		};
		const TCHAR* PortraitPaths[6] = {
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Knight.T_MB_HireIcon_Knight"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Mage.T_MB_HireIcon_Mage"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Ranger.T_MB_HireIcon_Ranger"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Rogue.T_MB_HireIcon_Rogue"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Barbarian.T_MB_HireIcon_Barbarian"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Druid.T_MB_HireIcon_Druid")
		};
		for (int32 Index = 0; Index < 6; ++Index)
		{
			UCanvasPanel* Card = FindOrCreate<UCanvasPanel>(Blueprint,
				FName(*FString::Printf(TEXT("HireCard_%d"), Index)));
			// 카드는 목록 틀의 **구멍 안**에 줄 세운다. 95,158 은 눈대중이었다.
			PlaceCanvas(LeftTopGroup, Card,
				FVector2D(ListInner.Min.X + (ListInner.GetSize().X - CardSize.X) * 0.5f,
					ListInner.Min.Y + 12.f + (CardSize.Y + 8.f) * Index),
				CardSize, 10);
			Card->SetClipping(EWidgetClipping::ClipToBoundsAlways);

			UImage* Art = FindOrCreate<UImage>(Blueprint,
				FName(*FString::Printf(TEXT("HireCard_%d_Art"), Index)));
			PlaceCanvas(Card, Art, FVector2D::ZeroVector, CardSize, 0);
			SetImage(Art, RowNormal);

			UImage* Selected = FindOrCreate<UImage>(Blueprint,
				FName(*FString::Printf(TEXT("HireSelected_%d"), Index)));
			PlaceCanvas(Card, Selected, FVector2D::ZeroVector, CardSize, 5);
			SetImage(Selected, RowSelected);
			Selected->SetVisibility(Index == 0
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed);

			// 카드 그림은 통짜로 그린다(비율 그대로). 안쪽 자리도 비율로 구한다.
			// 24,11 · 126,19 는 눈대중이었고, 사람이 맞춘 칸은 3.6%/8.8% 다.
			const FBox2D CardInner = UIPartRects::Inner(TEXT("T_MB_HireRowNormal"),
				FVector2D::ZeroVector, CardSize, false);
			const FVector2D CardSpan = CardInner.GetSize();
			const float FaceExtent = CardSpan.Y;

			UImage* Portrait = FindOrCreate<UImage>(Blueprint,
				FName(*FString::Printf(TEXT("HirePortrait_%d"), Index)));
			PlaceCanvas(Card, Portrait, CardInner.Min,
				FVector2D(FaceExtent, FaceExtent), 12);
			SetImage(Portrait, Texture(PortraitPaths[Index]));
			Portrait->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			UTextBlock* Name = FindOrCreate<UTextBlock>(Blueprint,
				FName(*FString::Printf(TEXT("HireName_%d"), Index)));
			PlaceCenteredText(Blueprint, Card, Name,
				CardInner.Min + FVector2D(FaceExtent + 14.f, 0.f),
				FVector2D(CardSpan.X - FaceExtent - 14.f, CardSpan.Y * 0.62f), 15);
			Name->SetText(FText::FromString(DefaultNames[Index]));
			Name->SetJustification(ETextJustify::Center);
			SetFont(Name, Font, 29);

			UTextBlock* Role = FindOrCreate<UTextBlock>(Blueprint,
				FName(*FString::Printf(TEXT("HireRole_%d"), Index)));
			PlaceCenteredText(Blueprint, Card, Role,
				CardInner.Min + FVector2D(FaceExtent + 14.f, CardSpan.Y * 0.58f),
				FVector2D(CardSpan.X - FaceExtent - 14.f, CardSpan.Y * 0.38f), 15);
			Role->SetText(FText::FromString(DefaultRoles[Index]));
			Role->SetJustification(ETextJustify::Center);
			SetFont(Role, Font, 16);

			for (const TCHAR* Prefix : {TEXT("HireHP"), TEXT("HireBadge"), TEXT("HireTrait")})
			{
				FindOrCreate<UTextBlock>(Blueprint,
					FName(*FString::Printf(TEXT("%s_%d"), Prefix, Index)))->SetVisibility(
						ESlateVisibility::Collapsed);
			}
			for (int32 SkillLine = 0; SkillLine < 2; ++SkillLine)
			{
				FindOrCreate<UTextBlock>(Blueprint,
					FName(*FString::Printf(TEXT("HireSkill_%d_%d"), Index, SkillLine)))->SetVisibility(
						ESlateVisibility::Collapsed);
			}

			UImage* Seal = FindOrCreate<UImage>(Blueprint,
				FName(*FString::Printf(TEXT("HireSeal_%d"), Index)));
			PlaceCanvas(Card, Seal, FVector2D(350.0f, 24.0f), FVector2D(58.0f, 68.0f), 20);
			Seal->SetVisibility(ESlateVisibility::Collapsed);

			UButton* Button = FindOrCreate<UButton>(Blueprint,
				FName(*FString::Printf(TEXT("HireButton_%d"), Index)));
			PlaceCanvas(Card, Button, FVector2D::ZeroVector, CardSize, 40);
			SetTransparentButton(Button);
		}

		UCanvasPanel* NamePanel = FindOrCreate<UCanvasPanel>(Blueprint, TEXT("HireDetailNamePanel"));
		PlaceCanvas(CenterMiddleGroup, NamePanel, FVector2D(180.0f, 590.0f), FVector2D(520.0f, 120.0f), 20);
		AddImage(Blueprint, NamePanel, TEXT("HireDetailNameArt"), NamePlate,
			FVector2D::ZeroVector, FVector2D(520.0f, 120.0f), 0);
		const FBox2D NameInner = UIPartRects::Inner(TEXT("T_MB_HireNamePlate"),
			FVector2D::ZeroVector, FVector2D(520.0f, 120.0f), false);
		UTextBlock* DetailName = AddText(Blueprint, NamePanel, TEXT("HireDetailName"),
			NSLOCTEXT("MarchboundHire", "Knight", "기사"), Font, 38,
			NameInner.Min, NameInner.GetSize(), 10);
		// 후보 목록에 클래스명이 이미 있으므로 중앙에서 반복하지 않는다.
		NamePanel->SetVisibility(ESlateVisibility::Collapsed);

		UCanvasPanel* StatsPanel = FindOrCreate<UCanvasPanel>(Blueprint, TEXT("HireDetailStatsPanel"));
		const FVector2D StatsSize(600.0f, 84.0f);
		PlaceCanvas(CenterMiddleGroup, StatsPanel, FVector2D(122.5f, 728.0f), StatsSize, 20);
		AddImage(Blueprint, StatsPanel, TEXT("HireDetailStatsArt"), StatsStrip,
			FVector2D::ZeroVector, StatsSize, 0);
		// 이 띠에는 세로 칸막이가 둘 그려져 있다. 셋으로 나눠 쓴다.
		// 칸을 셋 그어 두면 그 셋을 그대로 쓰고, 하나만 그어져 있으면 그
		// 하나를 셋으로 나눈다 -- 어느 쪽이든 배치는 안 고쳐도 된다.
		const TCHAR* const StatNames[3] = {
			TEXT("HireDetailHP"), TEXT("HireDetailAP"), TEXT("HireDetailSpeed") };
		const FText StatDefaults[3] = {
			FText::FromString(TEXT("HP 100")), FText::FromString(TEXT("AP 7")),
			NSLOCTEXT("MarchboundHire", "SpeedDefault", "SPEED 3") };
		for (int32 Index = 0; Index < 3; ++Index)
		{
			const FBox2D StatCell = UIPartRects::Cell(TEXT("T_MB_HireStatsStrip"),
				FVector2D::ZeroVector, StatsSize, false, Index, 3);
			AddText(Blueprint, StatsPanel, StatNames[Index], StatDefaults[Index],
				Font, 24, StatCell.Min, StatCell.GetSize(), 10);
		}

		const TCHAR* DefaultSkillLabels[6] = {
			TEXT("평타"), TEXT("이동"), TEXT("스킬 1"),
			TEXT("스킬 2"), TEXT("스킬 3"), TEXT("스킬 4")
		};
		for (int32 Index = 0; Index < 6; ++Index)
		{
			UCanvasPanel* SkillPanel = FindOrCreate<UCanvasPanel>(Blueprint,
				FName(*FString::Printf(TEXT("HireDetailSkill_%d"), Index)));
			PlaceCanvas(CenterMiddleGroup, SkillPanel, FVector2D(53.0f + 126.0f * Index, 820.0f),
				FVector2D(116.0f, 116.0f), 20);
			SkillPanel->SetClipping(EWidgetClipping::ClipToBoundsAlways);
			AddImage(Blueprint, SkillPanel,
				FName(*FString::Printf(TEXT("HireDetailSkillArt_%d"), Index)), SkillFrame,
				FVector2D::ZeroVector, FVector2D(116.0f, 116.0f), 0);

			// 런타임 UMercenaryHireWidget이 이 이름으로 실제 스킬 DA 아이콘을
			// 찾는다. 기존 자산에 우연히 남은 위젯에 의존하지 않고 빌더가 구조를
			// 완전히 소유해야 새 WBP에서도 같은 결과가 나온다.
			UOverlay* IconMount = FindOrCreate<UOverlay>(Blueprint,
				FName(*FString::Printf(TEXT("HireDetailSkillIconMount_%d"), Index)));
			PlaceCanvas(SkillPanel, IconMount, FVector2D::ZeroVector,
				FVector2D(116.0f, 116.0f), 5);
			IconMount->SetClipping(EWidgetClipping::ClipToBoundsAlways);
			if (UWidget* LegacyIconMount = Blueprint->WidgetTree->FindWidget(
				FName(*FString::Printf(TEXT("HireDetailSkillArt_%dMount"), Index))))
			{
				LegacyIconMount->SetVisibility(ESlateVisibility::Collapsed);
			}
			UImage* SkillIcon = FindOrCreate<UImage>(Blueprint,
				FName(*FString::Printf(TEXT("HireDetailSkillIcon_%d"), Index)));
			EnsureParent(IconMount, SkillIcon);
			UOverlaySlot* IconSlot = CastChecked<UOverlaySlot>(SkillIcon->Slot);
			IconSlot->SetPadding(FMargin(22.0f));
			IconSlot->SetHorizontalAlignment(HAlign_Fill);
			IconSlot->SetVerticalAlignment(VAlign_Fill);
			FSlateBrush EmptyIconBrush;
			EmptyIconBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
			SkillIcon->SetBrush(EmptyIconBrush);
			SkillIcon->SetColorAndOpacity(FLinearColor::White);
			SkillIcon->SetVisibility(ESlateVisibility::Collapsed);

			const FBox2D SkillInner = UIPartRects::Inner(TEXT("T_MB_HireSkillButtonFrame"),
				FVector2D::ZeroVector, FVector2D(116.0f, 116.0f), false);
			UTextBlock* SkillText = FindOrCreate<UTextBlock>(Blueprint,
				FName(*FString::Printf(TEXT("HireDetailSkillText_%d"), Index)));
			PlaceFittedCenteredText(Blueprint, SkillPanel, SkillText,
				SkillInner.Min, SkillInner.GetSize(), 10);
			SkillText->SetText(FText::FromString(DefaultSkillLabels[Index]));
			SetLightFont(SkillText, Font, 18);
			UButton* SkillButton = FindOrCreate<UButton>(Blueprint,
				FName(*FString::Printf(TEXT("HireDetailSkillButton_%d"), Index)));
			PlaceCanvas(SkillPanel, SkillButton, FVector2D::ZeroVector,
				FVector2D(116.0f, 116.0f), 30);
			SetTransparentButton(SkillButton);
		}

		// 목록은 상세만 바꾸고, 이 버튼만 현재 후보를 파티에 넣는다.
		UCanvasPanel* Add = FindOrCreate<UCanvasPanel>(Blueprint, TEXT("HireAddHolder"));
		PlaceCanvas(CenterRegion, Add, FVector2D(287.5f, 962.0f), FVector2D(270.0f, 106.0f), 30);
		AddImage(Blueprint, Add, TEXT("HireAddArt"), BackPlate,
			FVector2D::ZeroVector, FVector2D(270.0f, 106.0f), 0);
		UTextBlock* AddLabel = AddText(Blueprint, Add, TEXT("HireAddLabel"),
			NSLOCTEXT("MarchboundHire", "Add", "추가"), Font, 32,
			FVector2D::ZeroVector, FVector2D(270.0f, 106.0f), 15);
		SetLightFont(AddLabel, Font, 32);
		UButton* AddButton = FindOrCreate<UButton>(Blueprint, TEXT("HireAddButton"));
		FillSingleLabelButton(Blueprint, Add, AddLabel, AddButton,
			FVector2D(270.0f, 106.0f));

		UCanvasPanel* PartyPanel = FindOrCreate<UCanvasPanel>(Blueprint, TEXT("HireBottomBar"));
		const FVector2D PartySize(420.0f, 627.0f);
		PlaceCanvas(RightTopGroup, PartyPanel, FVector2D(40.0f, 145.0f), PartySize, 20);
		UImage* PartyFrameArt = FindOrCreate<UImage>(Blueprint, TEXT("HireBottomBar_Art"));
		PlaceCanvas(PartyPanel, PartyFrameArt, FVector2D::ZeroVector, PartySize, 0);
		SetImage(PartyFrameArt, PartyFrame);

		// 이 틀 그림에는 칸이 둘이다 -- 사람이 맞춰 둔 몸통(0)과 머리칸(1).
		// 인원수는 머리칸에, 자리들은 몸통에 넣는다.
		const FBox2D PartyBody = UIPartRects::Inner(TEXT("T_MB_HirePartyFrame"),
			FVector2D::ZeroVector, PartySize, false, 0);
		const FBox2D PartyHead = UIPartRects::Inner(TEXT("T_MB_HirePartyFrame"),
			FVector2D::ZeroVector, PartySize, false, 1);

		UTextBlock* PartyCount = FindOrCreate<UTextBlock>(Blueprint, TEXT("PartyCountText"));
		// 머리칸 검출 사각형은 약 45px라 영문 y의 아래꼬리와 외곽선이
		// 잘린다. 시각적 중심은 유지하면서 위아래만 4px씩 더 허용한다.
		const FVector2D PartyCountBleed(0.0f, 4.0f);
		PlaceCenteredText(Blueprint, PartyPanel, PartyCount,
			PartyHead.Min - PartyCountBleed,
			PartyHead.GetSize() + PartyCountBleed * 2.0f, 15);
		if (UWidget* PartyCountCenter = Blueprint->WidgetTree->FindWidget(
			TEXT("PartyCountText_Center")))
		{
			PartyCountCenter->SetClipping(EWidgetClipping::Inherit);
		}
		PartyCount->SetClipping(EWidgetClipping::Inherit);
		PartyCount->SetText(NSLOCTEXT("MarchboundHire", "PartyDefault", "파티 0/3"));
		PartyCount->SetJustification(ETextJustify::Center);
		SetLightFont(PartyCount, Font, 32);

		for (int32 Index = 0; Index < 3; ++Index)
		{
			UCanvasPanel* SlotPanel = FindOrCreate<UCanvasPanel>(Blueprint,
				FName(*FString::Printf(TEXT("PartySlot_%d"), Index)));
			// 자리 셋을 몸통 칸에 고르게 나눠 넣는다. 40,112 · 150 간격은
			// 눈대중이었고, 그 값들은 틀 그림이 바뀌면 같이 안 따라왔다.
			const FVector2D BodySpan = PartyBody.GetSize();
			const float SlotHeight = BodySpan.X * (420.0f / 1024.0f);
			const float SlotGap = (BodySpan.Y - SlotHeight * 3.0f) / 4.0f;
			const FVector2D SlotSize(BodySpan.X, SlotHeight);
			PlaceCanvas(PartyPanel, SlotPanel,
				PartyBody.Min + FVector2D(0.f,
					SlotGap + (SlotHeight + SlotGap) * Index),
				SlotSize, 10);

			AddImage(Blueprint, SlotPanel,
				FName(*FString::Printf(TEXT("PartySlotArt_%d"), Index)), PartyEmpty,
				FVector2D::ZeroVector, SlotSize, 0);
			AddImage(Blueprint, SlotPanel,
				FName(*FString::Printf(TEXT("PartySlotPlus_%d"), Index)), PartyPlus,
				FVector2D::ZeroVector, SlotSize, 2);

			UImage* Face = FindOrCreate<UImage>(Blueprint,
				FName(*FString::Printf(TEXT("PartySlotFace_%d"), Index)));
			// 얼굴과 이름은 자리 그림에서 잰 칸 안에 넣는다.
			const FBox2D SlotInner = UIPartRects::Inner(TEXT("T_MB_HirePartyRowEmpty"),
				FVector2D::ZeroVector, SlotSize, false);
			const FVector2D SlotSpan = SlotInner.GetSize();
			const float SlotFace = SlotSpan.Y;
			PlaceCanvas(SlotPanel, Face, SlotInner.Min,
				FVector2D(SlotFace, SlotFace), 10);
			Face->SetVisibility(ESlateVisibility::Collapsed);

			UTextBlock* Name = FindOrCreate<UTextBlock>(Blueprint,
				FName(*FString::Printf(TEXT("PartySlotName_%d"), Index)));
			PlaceCenteredText(Blueprint, SlotPanel, Name,
				SlotInner.Min + FVector2D(SlotFace + 12.f, 0.f),
				FVector2D(SlotSpan.X - SlotFace - 12.f, SlotSpan.Y * 0.62f), 12);
			Name->SetJustification(ETextJustify::Center);
			SetFont(Name, Font, 26);
			Name->SetVisibility(ESlateVisibility::Collapsed);

			// 교체 대상도 왼쪽 후보 카드처럼 직업과 레벨을 두 줄로 나눈다.
			UTextBlock* Level = FindOrCreate<UTextBlock>(Blueprint,
				FName(*FString::Printf(TEXT("PartySlotLevel_%d"), Index)));
			PlaceCenteredText(Blueprint, SlotPanel, Level,
				SlotInner.Min + FVector2D(SlotFace + 12.f, SlotSpan.Y * 0.58f),
				FVector2D(SlotSpan.X - SlotFace - 12.f, SlotSpan.Y * 0.38f), 13);
			Level->SetJustification(ETextJustify::Center);
			SetFont(Level, Font, 16);
			Level->SetVisibility(ESlateVisibility::Collapsed);

			UButton* SlotButton = FindOrCreate<UButton>(Blueprint,
				FName(*FString::Printf(TEXT("PartySlotButton_%d"), Index)));
			PlaceCanvas(SlotPanel, SlotButton, FVector2D::ZeroVector, SlotSize, 30);
			SetTransparentButton(SlotButton);
		}

		// 옛 안내 문구는 어느 패널에도 붙지 않은 고아 변수였다. 숨긴 채 다시
		// 생성하면 컴파일러 GUID 맵에만 남으므로 재빌드 때 제거한다.
		DeleteWidgetIfPresent(Blueprint, TEXT("NoticeText"));

		UCanvasPanel* Depart = FindOrCreate<UCanvasPanel>(Blueprint, TEXT("DepartHolder"));
		PlaceCanvas(RightRegion, Depart, FVector2D(148.0f, 962.0f), FVector2D(224.0f, 106.0f), 30);
		AddImage(Blueprint, Depart, TEXT("DepartArt"), DepartPlate,
			FVector2D::ZeroVector, FVector2D(224.0f, 106.0f), 0);
		UTextBlock* DepartLabel = FindOrCreate<UTextBlock>(Blueprint, TEXT("DepartLabel"));
		DepartLabel->SetText(NSLOCTEXT("MarchboundHire", "Depart", "출발"));
		DepartLabel->SetJustification(ETextJustify::Center);
		SetLightFont(DepartLabel, Font, 32);
		UButton* DepartButton = FindOrCreate<UButton>(Blueprint, TEXT("DepartButton"));
		FillSingleLabelButton(Blueprint, Depart, DepartLabel, DepartButton,
			FVector2D(224.0f, 106.0f));

		UCanvasPanel* Back = FindOrCreate<UCanvasPanel>(Blueprint, TEXT("HireBackHolder"));
		PlaceCanvas(LeftRegion, Back, FVector2D(70.0f, 962.0f), FVector2D(270.0f, 106.0f), 30);
		AddImage(Blueprint, Back, TEXT("HireBackArt"), BackPlate,
			FVector2D::ZeroVector, FVector2D(270.0f, 106.0f), 0);
		UTextBlock* BackLabel = AddText(Blueprint, Back, TEXT("HireBackLabel"),
			NSLOCTEXT("MarchboundHire", "Back", "뒤로"), Font, 32,
			FVector2D::ZeroVector, FVector2D(270.0f, 106.0f), 15);
		SetLightFont(BackLabel, Font, 32);
		UButton* BackButton = FindOrCreate<UButton>(Blueprint, TEXT("HireBackButton"));
		FillSingleLabelButton(Blueprint, Back, BackLabel, BackButton,
			FVector2D(270.0f, 106.0f));

		// 상점에서 찬 파티 칸을 교체할 때 사용하는 명시적 확인창. 기존 용병
		// 선택 WBP를 재사용하되, 교체가 즉시 실행되어 실수로 파티가 바뀌는 흐름은
		// 막는다. 빈 칸 고용에는 이 창을 띄우지 않는다.
		UCanvasPanel* ReplaceConfirmLayer = FindOrCreate<UCanvasPanel>(
			Blueprint, TEXT("HireReplaceConfirmLayer"));
		PlaceCanvas(Root, ReplaceConfirmLayer, FVector2D::ZeroVector, DesignSize, 200);
		if (UCanvasPanelSlot* LayerSlot = Cast<UCanvasPanelSlot>(ReplaceConfirmLayer->Slot))
		{
			LayerSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			LayerSlot->SetOffsets(FMargin(0.f));
		}
		ReplaceConfirmLayer->SetVisibility(ESlateVisibility::Collapsed);
		UBorder* ReplaceDim = FindOrCreate<UBorder>(Blueprint, TEXT("HireReplaceConfirmDim"));
		ReplaceDim->SetBrush(FSlateColorBrush(FLinearColor(0.f, 0.f, 0.f, .68f)));
		PlaceCanvas(ReplaceConfirmLayer, ReplaceDim, FVector2D::ZeroVector, DesignSize, 0);
		UCanvasPanel* ReplacePanel = FindOrCreate<UCanvasPanel>(
			Blueprint, TEXT("HireReplaceConfirmPanel"));
		PlaceCanvas(ReplaceConfirmLayer, ReplacePanel,
			FVector2D(610.f, 360.f), FVector2D(700.f, 340.f), 10);
		AddImage(Blueprint, ReplacePanel, TEXT("HireReplaceConfirmPlate"), TitlePlate,
			FVector2D::ZeroVector, FVector2D(700.f, 340.f), 0);
		UTextBlock* ReplaceTitle = AddText(Blueprint, ReplacePanel,
			TEXT("HireReplaceConfirmTitle"),
			NSLOCTEXT("MarchboundHire", "ReplaceTitle", "용병 교체"), Font, 38,
			FVector2D(80.f, 48.f), FVector2D(540.f, 64.f), 15);
		SetLightFont(ReplaceTitle, Font, 38);
		UTextBlock* ReplaceBody = AddText(Blueprint, ReplacePanel,
			TEXT("HireReplaceConfirmBody"),
			NSLOCTEXT("MarchboundHire", "ReplaceBody",
				"선택한 파티 용병을 새 용병으로 교체할까요?"), Font, 26,
			FVector2D(55.f, 122.f), FVector2D(590.f, 70.f), 15);
		SetLightFont(ReplaceBody, Font, 26);
		UCanvasPanel* ReplaceAcceptHolder = FindOrCreate<UCanvasPanel>(
			Blueprint, TEXT("HireReplaceAcceptHolder"));
		PlaceCanvas(ReplacePanel, ReplaceAcceptHolder,
			FVector2D(70.f, 220.f), FVector2D(250.f, 92.f), 20);
		AddImage(Blueprint, ReplaceAcceptHolder, TEXT("HireReplaceAcceptArt"),
			DepartPlate, FVector2D::ZeroVector, FVector2D(250.f, 92.f), 0);
		UTextBlock* ReplaceAcceptLabel = AddText(Blueprint, ReplaceAcceptHolder,
			TEXT("HireReplaceAcceptLabel"), NSLOCTEXT("MarchboundHire", "Replace", "교체"),
			Font, 30, FVector2D::ZeroVector, FVector2D(250.f, 92.f), 15);
		SetLightFont(ReplaceAcceptLabel, Font, 30);
		UButton* ReplaceAcceptButton = FindOrCreate<UButton>(
			Blueprint, TEXT("HireReplaceAcceptButton"));
		FillSingleLabelButton(Blueprint, ReplaceAcceptHolder, ReplaceAcceptLabel,
			ReplaceAcceptButton, FVector2D(250.f, 92.f));
		UCanvasPanel* ReplaceCancelHolder = FindOrCreate<UCanvasPanel>(
			Blueprint, TEXT("HireReplaceCancelHolder"));
		PlaceCanvas(ReplacePanel, ReplaceCancelHolder,
			FVector2D(380.f, 220.f), FVector2D(250.f, 92.f), 20);
		AddImage(Blueprint, ReplaceCancelHolder, TEXT("HireReplaceCancelArt"),
			BackPlate, FVector2D::ZeroVector, FVector2D(250.f, 92.f), 0);
		UTextBlock* ReplaceCancelLabel = AddText(Blueprint, ReplaceCancelHolder,
			TEXT("HireReplaceCancelLabel"), NSLOCTEXT("MarchboundHire", "Cancel", "취소"),
			Font, 30, FVector2D::ZeroVector, FVector2D(250.f, 92.f), 15);
		SetLightFont(ReplaceCancelLabel, Font, 30);
		UButton* ReplaceCancelButton = FindOrCreate<UButton>(
			Blueprint, TEXT("HireReplaceCancelButton"));
		FillSingleLabelButton(Blueprint, ReplaceCancelHolder, ReplaceCancelLabel,
			ReplaceCancelButton, FVector2D(250.f, 92.f));

		PruneStaleVariables(Blueprint);
		RepairLiveVariableGuids(Blueprint);
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		if (!UPackage::SavePackage(Blueprint->GetPackage(), Blueprint,
			*FPackageName::LongPackageNameToFilename(
				Blueprint->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension()),
			FSavePackageArgs()))
		{
			UE_LOG(LogTemp, Error, TEXT("RD_MB_HIRE_BUILD save failed"));
			return;
		}

		UE_LOG(LogTemp, Display,
			TEXT("RD_MB_HIRE_BUILD success asset=%s cards=6 party_slots=3 skills=6 design=1920x1080"),
			AssetPath);
	}
}

void RegisterMarchboundHireWidgetBuilderCommands()
{
	using namespace MarchboundHireWidgetBuilder;
	BuildCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.BuildMercenaryHire"),
		TEXT("Rebuild WBP_MercenaryHire_Marchbound with the Marchbound split UI parts."),
		FConsoleCommandDelegate::CreateStatic(&Build));
	ButtonLabelRepairCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.RepairMercenaryHireButtonLabels"),
		TEXT("Make the Add, Depart, and Back label bounds match their buttons."),
		FConsoleCommandDelegate::CreateStatic(&RepairSingleLabelButtonsOnly));
	TextTranslationRepairCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.ResetMercenaryHireTextTranslations"),
		TEXT("Remove every manual TextBlock render translation from the hire WBP."),
		FConsoleCommandDelegate::CreateStatic(&RepairTextRenderTranslationsOnly));
}

void UnregisterMarchboundHireWidgetBuilderCommands()
{
	MarchboundHireWidgetBuilder::BuildCommand.Reset();
	MarchboundHireWidgetBuilder::ButtonLabelRepairCommand.Reset();
	MarchboundHireWidgetBuilder::TextTranslationRepairCommand.Reset();
}
