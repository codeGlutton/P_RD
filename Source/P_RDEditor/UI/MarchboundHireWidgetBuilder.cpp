#include "UI/MarchboundHireWidgetBuilder.h"
#include "UI/UIPartRects.h"
#include "UI/UIFont.h"

#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "HAL/IConsoleManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprintEditorUtils.h"

namespace MarchboundHireWidgetBuilder
{
	constexpr TCHAR AssetPath[] =
		TEXT("/Game/UI/CombatLayouts/WBP_MercenaryHire_Marchbound.WBP_MercenaryHire_Marchbound");
	// Each screen region owns its own mobile-responsive scale box.
	const FVector2D DesignSize(1920.0f, 1080.0f);
	// LINESeedKR's metric line box is not the visible glyph box. These offsets are
	// measured per use because font size, slot height, and mixed scripts all matter.
	constexpr float TitleOpticalOffsetY = -31.5f;
	constexpr float ListNameOpticalOffsetY = -18.125f;
	constexpr float ListRoleOpticalOffsetY = -2.25f;
	constexpr float DetailNameOpticalOffsetY = -13.5f;
	constexpr float SkillLabelOpticalOffsetY = -2.5f;
	constexpr float ButtonLabelOpticalOffsetY = -20.0f;
	constexpr float PartyCountOpticalOffsetY = -26.5f;
	constexpr float PartySlotNameOpticalOffsetY = -16.25f;

	TUniquePtr<FAutoConsoleCommand> BuildCommand;

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

	void RemoveWidget(UWidgetBlueprint* Blueprint, const FName Name)
	{
		if (UWidget* Widget = Blueprint->WidgetTree->FindWidget(Name))
		{
			FWidgetBlueprintEditorUtils::DeleteWidgets(
				Blueprint, { Widget },
				FWidgetBlueprintEditorUtils::EDeleteWidgetWarningType::DeleteSilently);
		}
	}

	void PlaceCenteredText(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent,
		UTextBlock* Text, const FVector2D Position, const FVector2D Size,
		const int32 ZOrder)
	{
		const FName CenterName(*FString::Printf(TEXT("%s_Center"), *Text->GetName()));
		UOverlay* Center = FindOrCreate<UOverlay>(Blueprint, CenterName);
		PlaceCanvas(Parent, Center, Position, Size, ZOrder);
		EnsureParent(Center, Text);
		UOverlaySlot* TextSlot = CastChecked<UOverlaySlot>(Text->Slot);
		TextSlot->SetHorizontalAlignment(HAlign_Fill);
		TextSlot->SetVerticalAlignment(VAlign_Center);
		Text->SetJustification(ETextJustify::Center);
	}

	void ApplyTextOpticalCenter(UTextBlock* Text, const float OffsetY)
	{
		check(Text != nullptr);
		Text->SetRenderTranslation(FVector2D(0.0f, OffsetY));
	}

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
			Blueprint->OnVariableRemoved(Name);
		}
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
		Result->SetJustification(Justify == ETextJustify::Left
			? ETextJustify::Left : ETextJustify::Center);
		SetFont(Result, Font, FontSize);
		return Result;
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

		// 배경은 모든 화면비를 채우며 중앙 크롭한다. UI는 한 장짜리 1920x1080
		// 프레임으로 고정하지 않고 좌/중/우 영역이 각자 화면 폭에 맞춰 줄어든다.
		UOverlay* ViewportRoot = FindOrCreate<UOverlay>(Blueprint, TEXT("HireViewportRoot"));
		RemoveWidget(Blueprint, TEXT("HireUIScale"));
		EnsureParent(ViewportRoot, Root);
		Blueprint->WidgetTree->RootWidget = ViewportRoot;

		UTextBlock* FontSource = FindOrCreate<UTextBlock>(Blueprint, TEXT("HireName_0"));
		const FSlateFontInfo Font = FontSource->GetFont();

		UTexture2D* KnightBackground = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Characters/Mercenaries/T_MB_HireHero_Knight.T_MB_HireHero_Knight"));
		UTexture2D* ListFrame = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Run/Hire/T_MB_HireListFrame.T_MB_HireListFrame"));
		UTexture2D* RowNormal = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Run/Hire/T_MB_HireRowNormal.T_MB_HireRowNormal"));
		UTexture2D* RowSelected = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Run/Hire/T_MB_HireRowSelected.T_MB_HireRowSelected"));
		UTexture2D* BackPlate = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Run/Hire/T_MB_HireBackButton.T_MB_HireBackButton"));
		UTexture2D* TitlePlate = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Run/Hire/T_MB_HireTitlePlate.T_MB_HireTitlePlate"));
		UTexture2D* PartyFrame = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Run/Hire/T_MB_HirePartyFrame.T_MB_HirePartyFrame"));
		UTexture2D* PartyPlus = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Run/Hire/T_MB_HirePartyRowPlus.T_MB_HirePartyRowPlus"));
		UTexture2D* PartyEmpty = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Run/Hire/T_MB_HirePartyRowEmpty.T_MB_HirePartyRowEmpty"));
		UTexture2D* DepartPlate = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Run/Hire/T_MB_HireDepartButton.T_MB_HireDepartButton"));
		UTexture2D* NamePlate = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Run/Hire/T_MB_HireNamePlate.T_MB_HireNamePlate"));
		UTexture2D* StatsStrip = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Run/Hire/T_MB_HireStatsStrip.T_MB_HireStatsStrip"));
		UTexture2D* SkillFrame = Texture(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Run/Hire/T_MB_HireSkillButtonFrame.T_MB_HireSkillButtonFrame"));

		RemoveWidget(Blueprint, TEXT("Backdrop"));
		RemoveWidget(Blueprint, TEXT("Board"));
		UScaleBox* BackgroundScale = FindOrCreate<UScaleBox>(Blueprint, TEXT("HireBackgroundScale"));
		BackgroundScale->SetStretch(EStretch::ScaleToFill);
		BackgroundScale->SetStretchDirection(EStretchDirection::Both);
		BackgroundScale->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		UImage* BackgroundArt = FindOrCreate<UImage>(Blueprint, TEXT("Backdrop_Art"));
		EnsureParent(BackgroundScale, BackgroundArt);
		SetImage(BackgroundArt, KnightBackground);

		// Overlay의 자식 순서가 곧 그리기 순서다. 배경을 먼저, 반응형 UI 캔버스를
		// 나중에 다시 넣어 재빌드해도 항상 같은 계층과 Z 순서를 보장한다.
		if (UPanelWidget* Parent = BackgroundScale->GetParent())
		{
			Parent->RemoveChild(BackgroundScale);
		}
		if (UPanelWidget* Parent = Root->GetParent())
		{
			Parent->RemoveChild(Root);
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

		const FVector2D ListPos(55.0f, 100.0f);
		const FVector2D ListSize(500.0f, 850.0f);
		AddImage(Blueprint, LeftRegion, TEXT("HireListFrameArt"), ListFrame,
			ListPos, ListSize, 0);
		const FBox2D ListInner = UIPartRects::Inner(TEXT("T_MB_HireListFrame"),
			ListPos, ListSize, false);

		UCanvasPanel* TitlePanel = FindOrCreate<UCanvasPanel>(Blueprint, TEXT("HireTitlePanel"));
		PlaceCanvas(CenterRegion, TitlePanel, FVector2D(190.0f, 18.0f), FVector2D(430.0f, 106.0f), 30);
		AddImage(Blueprint, TitlePanel, TEXT("HireTitleArt"), TitlePlate,
			FVector2D::ZeroVector, FVector2D(430.0f, 106.0f), 0);
		UTextBlock* TitleText = AddText(Blueprint, TitlePanel, TEXT("HireTitleText"),
			NSLOCTEXT("MarchboundHire", "Title", "용병 선택"), Font, 42,
			FVector2D(30.0f, 20.0f), FVector2D(370.0f, 64.0f), 10);
		SetLightFont(TitleText, Font, 42);
		ApplyTextOpticalCenter(TitleText, TitleOpticalOffsetY);

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
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Characters/Mercenaries/T_MB_HireIcon_Knight.T_MB_HireIcon_Knight"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Characters/Mercenaries/T_MB_HireIcon_Mage.T_MB_HireIcon_Mage"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Characters/Mercenaries/T_MB_HireIcon_Ranger.T_MB_HireIcon_Ranger"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Characters/Mercenaries/T_MB_HireIcon_Rogue.T_MB_HireIcon_Rogue"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Characters/Mercenaries/T_MB_HireIcon_Barbarian.T_MB_HireIcon_Barbarian"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Characters/Mercenaries/T_MB_HireIcon_Druid.T_MB_HireIcon_Druid")
		};
		for (int32 Index = 0; Index < 6; ++Index)
		{
			UCanvasPanel* Card = FindOrCreate<UCanvasPanel>(Blueprint,
				FName(*FString::Printf(TEXT("HireCard_%d"), Index)));
			// 카드는 목록 틀의 **구멍 안**에 줄 세운다. 95,158 은 눈대중이었다.
			PlaceCanvas(LeftRegion, Card,
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
			ApplyTextOpticalCenter(Name, ListNameOpticalOffsetY);

			UTextBlock* Role = FindOrCreate<UTextBlock>(Blueprint,
				FName(*FString::Printf(TEXT("HireRole_%d"), Index)));
			PlaceCenteredText(Blueprint, Card, Role,
				CardInner.Min + FVector2D(FaceExtent + 14.f, CardSpan.Y * 0.58f),
				FVector2D(CardSpan.X - FaceExtent - 14.f, CardSpan.Y * 0.38f), 15);
			Role->SetText(FText::FromString(DefaultRoles[Index]));
			Role->SetJustification(ETextJustify::Center);
			SetFont(Role, Font, 16);
			ApplyTextOpticalCenter(Role, ListRoleOpticalOffsetY);

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
		PlaceCanvas(CenterRegion, NamePanel, FVector2D(180.0f, 590.0f), FVector2D(520.0f, 120.0f), 20);
		AddImage(Blueprint, NamePanel, TEXT("HireDetailNameArt"), NamePlate,
			FVector2D::ZeroVector, FVector2D(520.0f, 120.0f), 0);
		const FBox2D NameInner = UIPartRects::Inner(TEXT("T_MB_HireNamePlate"),
			FVector2D::ZeroVector, FVector2D(520.0f, 120.0f), false);
		UTextBlock* DetailName = AddText(Blueprint, NamePanel, TEXT("HireDetailName"),
			NSLOCTEXT("MarchboundHire", "Knight", "기사"), Font, 38,
			NameInner.Min, NameInner.GetSize(), 10);
		ApplyTextOpticalCenter(DetailName, DetailNameOpticalOffsetY);

		UCanvasPanel* StatsPanel = FindOrCreate<UCanvasPanel>(Blueprint, TEXT("HireDetailStatsPanel"));
		PlaceCanvas(CenterRegion, StatsPanel, FVector2D(95.0f, 710.0f), FVector2D(690.0f, 96.0f), 20);
		AddImage(Blueprint, StatsPanel, TEXT("HireDetailStatsArt"), StatsStrip,
			FVector2D::ZeroVector, FVector2D(690.0f, 96.0f), 0);
		// 이 띠에는 세로 칸막이가 둘 그려져 있다. 셋으로 나눠 쓴다.
		// 칸을 셋 그어 두면 그 셋을 그대로 쓰고, 하나만 그어져 있으면 그
		// 하나를 셋으로 나눈다 -- 어느 쪽이든 배치는 안 고쳐도 된다.
		const FVector2D StatsSize(690.0f, 96.0f);
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
				Font, 28, StatCell.Min, StatCell.GetSize(), 10);
		}

		const TCHAR* DefaultSkillLabels[6] = {
			TEXT("평타"), TEXT("이동"), TEXT("스킬 1"),
			TEXT("스킬 2"), TEXT("스킬 3"), TEXT("스킬 4")
		};
		for (int32 Index = 0; Index < 6; ++Index)
		{
			UCanvasPanel* SkillPanel = FindOrCreate<UCanvasPanel>(Blueprint,
				FName(*FString::Printf(TEXT("HireDetailSkill_%d"), Index)));
			PlaceCanvas(CenterRegion, SkillPanel, FVector2D(53.0f + 126.0f * Index, 820.0f),
				FVector2D(116.0f, 116.0f), 20);
			AddImage(Blueprint, SkillPanel,
				FName(*FString::Printf(TEXT("HireDetailSkillArt_%d"), Index)), SkillFrame,
				FVector2D::ZeroVector, FVector2D(116.0f, 116.0f), 0);
			const FBox2D SkillInner = UIPartRects::Inner(TEXT("T_MB_HireSkillButtonFrame"),
				FVector2D::ZeroVector, FVector2D(116.0f, 116.0f), false);
			UTextBlock* SkillText = AddText(Blueprint, SkillPanel,
				FName(*FString::Printf(TEXT("HireDetailSkillText_%d"), Index)),
				FText::FromString(DefaultSkillLabels[Index]), Font, 18,
				SkillInner.Min, SkillInner.GetSize(), 10);
			SetLightFont(SkillText, Font, 18);
			ApplyTextOpticalCenter(SkillText, SkillLabelOpticalOffsetY);
			UButton* SkillButton = FindOrCreate<UButton>(Blueprint,
				FName(*FString::Printf(TEXT("HireDetailSkillButton_%d"), Index)));
			PlaceCanvas(SkillPanel, SkillButton, FVector2D::ZeroVector,
				FVector2D(116.0f, 116.0f), 30);
			SetTransparentButton(SkillButton);
		}

		// 스킬을 검토한 뒤 편성하는 유일한 진입점. 목록 클릭은 상세만 바꾼다.
		UCanvasPanel* Add = FindOrCreate<UCanvasPanel>(Blueprint, TEXT("HireAddHolder"));
		PlaceCanvas(CenterRegion, Add, FVector2D(287.5f, 962.0f), FVector2D(270.0f, 106.0f), 30);
		AddImage(Blueprint, Add, TEXT("HireAddArt"), BackPlate,
			FVector2D::ZeroVector, FVector2D(270.0f, 106.0f), 0);
		UTextBlock* AddLabel = AddText(Blueprint, Add, TEXT("HireAddLabel"),
			NSLOCTEXT("MarchboundHire", "Add", "추가"), Font, 32,
			FVector2D(25.0f, 24.0f), FVector2D(220.0f, 58.0f), 15);
		SetLightFont(AddLabel, Font, 32);
		ApplyTextOpticalCenter(AddLabel, ButtonLabelOpticalOffsetY);
		UButton* AddButton = FindOrCreate<UButton>(Blueprint, TEXT("HireAddButton"));
		PlaceCanvas(Add, AddButton, FVector2D::ZeroVector, FVector2D(270.0f, 106.0f), 30);
		SetTransparentButton(AddButton);

		UCanvasPanel* PartyPanel = FindOrCreate<UCanvasPanel>(Blueprint, TEXT("HireBottomBar"));
		const FVector2D PartySize(420.0f, 627.0f); // 원본 640x956 비율 보존
		PlaceCanvas(RightRegion, PartyPanel, FVector2D(40.0f, 145.0f), PartySize, 20);
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
		PlaceCenteredText(Blueprint, PartyPanel, PartyCount,
			PartyHead.Min, PartyHead.GetSize(), 15);
		PartyCount->SetText(NSLOCTEXT("MarchboundHire", "PartyDefault", "파티 0/3"));
		PartyCount->SetJustification(ETextJustify::Center);
		SetLightFont(PartyCount, Font, 32);
		ApplyTextOpticalCenter(PartyCount, PartyCountOpticalOffsetY);

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
				SlotInner.Min + FVector2D(SlotFace + 12.f, SlotSpan.Y * 0.28f),
				FVector2D(SlotSpan.X - SlotFace - 12.f, SlotSpan.Y * 0.44f), 12);
			Name->SetJustification(ETextJustify::Center);
			SetFont(Name, Font, 26);
			ApplyTextOpticalCenter(Name, PartySlotNameOpticalOffsetY);
			Name->SetVisibility(ESlateVisibility::Collapsed);

			UButton* SlotButton = FindOrCreate<UButton>(Blueprint,
				FName(*FString::Printf(TEXT("PartySlotButton_%d"), Index)));
			PlaceCanvas(SlotPanel, SlotButton, FVector2D::ZeroVector, SlotSize, 30);
			SetTransparentButton(SlotButton);
		}

		RemoveWidget(Blueprint, TEXT("NoticeText"));
		RemoveWidget(Blueprint, TEXT("NoticeText_Center"));

		UCanvasPanel* Depart = FindOrCreate<UCanvasPanel>(Blueprint, TEXT("DepartHolder"));
		PlaceCanvas(RightRegion, Depart, FVector2D(148.0f, 962.0f), FVector2D(224.0f, 106.0f), 30);
		AddImage(Blueprint, Depart, TEXT("DepartArt"), DepartPlate,
			FVector2D::ZeroVector, FVector2D(224.0f, 106.0f), 0);
		UTextBlock* DepartLabel = FindOrCreate<UTextBlock>(Blueprint, TEXT("DepartLabel"));
		const FBox2D DepartInner = UIPartRects::Inner(TEXT("T_MB_HireDepartButton"),
			FVector2D::ZeroVector, FVector2D(224.0f, 106.0f), false);
		PlaceCenteredText(Blueprint, Depart, DepartLabel,
			DepartInner.Min, DepartInner.GetSize(), 15);
		DepartLabel->SetText(NSLOCTEXT("MarchboundHire", "Depart", "출발"));
		DepartLabel->SetJustification(ETextJustify::Center);
		SetLightFont(DepartLabel, Font, 32);
		ApplyTextOpticalCenter(DepartLabel, ButtonLabelOpticalOffsetY);
		UButton* DepartButton = FindOrCreate<UButton>(Blueprint, TEXT("DepartButton"));
		PlaceCanvas(Depart, DepartButton, FVector2D::ZeroVector, FVector2D(224.0f, 106.0f), 30);
		SetTransparentButton(DepartButton);

		UCanvasPanel* Back = FindOrCreate<UCanvasPanel>(Blueprint, TEXT("HireBackHolder"));
		PlaceCanvas(LeftRegion, Back, FVector2D(70.0f, 962.0f), FVector2D(270.0f, 106.0f), 30);
		AddImage(Blueprint, Back, TEXT("HireBackArt"), BackPlate,
			FVector2D::ZeroVector, FVector2D(270.0f, 106.0f), 0);
		UTextBlock* BackLabel = AddText(Blueprint, Back, TEXT("HireBackLabel"),
			NSLOCTEXT("MarchboundHire", "Back", "뒤로"), Font, 32,
			FVector2D(25.0f, 24.0f), FVector2D(220.0f, 58.0f), 15);
		SetLightFont(BackLabel, Font, 32);
		ApplyTextOpticalCenter(BackLabel, ButtonLabelOpticalOffsetY);
		UButton* BackButton = FindOrCreate<UButton>(Blueprint, TEXT("HireBackButton"));
		PlaceCanvas(Back, BackButton, FVector2D::ZeroVector, FVector2D(270.0f, 106.0f), 30);
		SetTransparentButton(BackButton);

		PruneStaleVariables(Blueprint);
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
}

void UnregisterMarchboundHireWidgetBuilderCommands()
{
	MarchboundHireWidgetBuilder::BuildCommand.Reset();
}
