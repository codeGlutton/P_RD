#include "UI/RunOptionsRailWidget.h"
#include "UI/DetailOverlayInputShield.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameMode/RoomGameModeBase.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "UI/FrontendMapWidget.h"
#include "UI/Combat/CombatLayoutHUDWidget.h"
#include "UI/Combat/CombatUITypes.h"
#include "UI/Combat/SkillDetailOverlayPresenter.h"
#include "UI/Combat/SkillTacticalDiagramWidget.h"
#include "UI/SettingsPanelWidget.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "RunOptionsRailWidget"

namespace
{
	template <typename T>
	T* FindWidget(UUserWidget* Owner, const TCHAR* Name)
	{
		return Owner != nullptr ? Cast<T>(Owner->GetWidgetFromName(Name)) : nullptr;
	}

	void SetShown(UWidget* Widget, const bool bShown,
		const bool bInteractive = false)
	{
		if (Widget != nullptr)
		{
			Widget->SetVisibility(bShown
				? (bInteractive ? ESlateVisibility::Visible
					: ESlateVisibility::SelfHitTestInvisible)
				: ESlateVisibility::Collapsed);
		}
	}

	void SetText(UUserWidget* Owner, const TCHAR* Name, const FText& Value)
	{
		if (UTextBlock* Text = FindWidget<UTextBlock>(Owner, Name))
		{
			Text->SetText(Value);
		}
	}

	void SetCenteredText(UUserWidget* Owner, const TCHAR* Name,
		const FText& Value)
	{
		if (UTextBlock* Text = FindWidget<UTextBlock>(Owner, Name))
		{
			Text->SetText(Value);
			Text->SetJustification(ETextJustify::Center);
		}
	}

	/*
	 * 직업 표시명(MercenaryJobName)과 T_MB_HireIcon_* 초상 해석은 생산자
	 * (ARoomGameModeBase::GetPartyRosterView)로 옮겼다. 위젯은 View 값만 그린다.
	 */

	void SetImage(UImage* Image, UTexture2D* Texture)
	{
		if (Image == nullptr)
		{
			return;
		}
		Image->SetVisibility(Texture != nullptr
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (Texture != nullptr)
		{
			Image->SetBrushFromTexture(Texture, false);
			FSlateBrush Brush = Image->GetBrush();
			Brush.DrawAs = ESlateBrushDrawType::Image;
			Image->SetBrush(Brush);
		}
	}

	FButtonStyle TransparentButtonStyle()
	{
		FButtonStyle Style;
		FSlateBrush Empty;
		Empty.DrawAs = ESlateBrushDrawType::NoDrawType;
		Style.SetNormal(Empty);
		Style.SetHovered(Empty);
		Style.SetPressed(Empty);
		Style.SetDisabled(Empty);
		Style.SetNormalPadding(FMargin(0.f));
		Style.SetPressedPadding(FMargin(0.f));
		return Style;
	}
}

URunOptionsRailWidget::URunOptionsRailWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> FrameFinder(
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_OptionsRail_Frame.T_MB_OptionsRail_Frame"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> MapFinder(
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_OptionsIcon_Map.T_MB_OptionsIcon_Map"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> MercenaryFinder(
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_OptionsIcon_MercenaryGlyph.T_MB_OptionsIcon_MercenaryGlyph"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> MonsterFinder(
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_OptionsIcon_MonsterGlyph.T_MB_OptionsIcon_MonsterGlyph"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> SettingsFinder(
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_OptionsIcon_Settings.T_MB_OptionsIcon_Settings"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> CardNormalFinder(
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Hire/T_MB_HireRowNormal.T_MB_HireRowNormal"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> CardSelectedFinder(
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Hire/T_MB_HireRowSelected.T_MB_HireRowSelected"));
	OptionsFrameTexture = FrameFinder.Object;
	MapIconTexture = MapFinder.Object;
	MercenaryIconTexture = MercenaryFinder.Object;
	MonsterIconTexture = MonsterFinder.Object;
	SettingsIconTexture = SettingsFinder.Object;
	MercenaryCardNormalTexture = CardNormalFinder.Object;
	MercenaryCardSelectedTexture = CardSelectedFinder.Object;
}

TSharedRef<SWidget> URunOptionsRailWidget::RebuildWidget()
{
	BuildRuntimeTree();
	return Super::RebuildWidget();
}

void URunOptionsRailWidget::BuildRuntimeTree()
{
	if (WidgetTree == nullptr || WidgetTree->RootWidget != nullptr)
	{
		return;
	}
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("RunOptionsRailRoot"));
	// 이 위젯은 별도 Viewport 레이어라 루트 Canvas가 화면 전체를 차지한다.
	// Visible이면 우상단 버튼 바깥의 지도 노드와 BACK 입력까지 가로챈다.
	// Canvas 자신만 hit-test에서 빼고 실제 UButton 자식 입력은 유지한다.
	Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = Root;
	UCanvasPanel* Rail = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("RunOptionsRailCanvas"));
	Rail->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (UCanvasPanelSlot* RailSlot = Root->AddChildToCanvas(Rail))
	{
		RailSlot->SetAnchors(FAnchors(1.f, 0.f));
		RailSlot->SetAlignment(FVector2D(1.f, 0.f));
		RailSlot->SetPosition(FVector2D(-8.f, 4.f));
		RailSlot->SetSize(FVector2D(
			RunOptionsRail::Width, RunOptionsRail::Height));
		RailSlot->SetZOrder(1);
	}
	FWidgetTransform Transform;
	Transform.Scale = FVector2D(.75f, .75f);
	Rail->SetRenderTransform(Transform);
	Rail->SetRenderTransformPivot(FVector2D(1.f, 0.f));

	auto AddImage = [&](const TCHAR* Name, UTexture2D* Texture,
		const FVector2D Position, const FVector2D Size, const int32 Z) -> UImage*
	{
		UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
		SetImage(Image, Texture);
		if (UCanvasPanelSlot* Slot = Rail->AddChildToCanvas(Image))
		{
			Slot->SetPosition(Position);
			Slot->SetSize(Size);
			Slot->SetZOrder(Z);
		}
		return Image;
	};
	auto AddButton = [&](const TCHAR* Name, const FVector2D Position,
		const FVector2D Size) -> UButton*
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		Button->SetStyle(TransparentButtonStyle());
		if (UCanvasPanelSlot* Slot = Rail->AddChildToCanvas(Button))
		{
			Slot->SetPosition(Position);
			Slot->SetSize(Size);
			Slot->SetZOrder(40);
		}
		return Button;
	};

	// 좌표는 RunOptionsRail 한 곳에서만 온다 -- 전투 HUD를 굽는 편집기 빌더도
	// 같은 값을 쓴다. 여기에 숫자를 다시 적으면 두 화면이 또 갈라진다.
	AddImage(TEXT("OptionsRailFrame"), OptionsFrameTexture,
		FVector2D::ZeroVector,
		FVector2D(RunOptionsRail::Width, RunOptionsRail::Height), 1);
	MapIcon = AddImage(TEXT("MenuMapIcon"), MapIconTexture,
		RunOptionsRail::IconPosition(0), RunOptionsRail::IconSize(0), 31);
	AddImage(TEXT("MenuMercenaryIcon"), MercenaryIconTexture,
		RunOptionsRail::IconPosition(1), RunOptionsRail::IconSize(1), 31);
	UImage* MonsterIcon = AddImage(TEXT("MenuMonsterIcon"), MonsterIconTexture,
		RunOptionsRail::IconPosition(2), RunOptionsRail::IconSize(2), 31);
	AddImage(TEXT("MenuSettingsIcon"), SettingsIconTexture,
		RunOptionsRail::IconPosition(3), RunOptionsRail::IconSize(3), 31);
	MapButton = AddButton(TEXT("MenuButton_0"),
		RunOptionsRail::ButtonPosition(0), RunOptionsRail::ButtonSize());
	MercenaryButton = AddButton(TEXT("MenuButton_1"),
		RunOptionsRail::ButtonPosition(1), RunOptionsRail::ButtonSize());
	MonsterButton = AddButton(TEXT("MenuButton_2"),
		RunOptionsRail::ButtonPosition(2), RunOptionsRail::ButtonSize());
	SettingsButton = AddButton(TEXT("MenuButton_3"),
		RunOptionsRail::ButtonPosition(3), RunOptionsRail::ButtonSize());
	// 비전투 방에는 생존 몬스터 데이터가 없다. 자리/아이콘은 같은 레일 계약을
	// 유지하되 가짜 상세를 열지 않도록 명시적으로 잠근다.
	MonsterButton->SetIsEnabled(false);
	MonsterIcon->SetRenderOpacity(.42f);
}

void URunOptionsRailWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindRailInputs();
	SetMapContext(bMapContext);
}

void URunOptionsRailWidget::BindRailInputs()
{
	if (MapButton) MapButton->OnClicked.AddUniqueDynamic(this, &URunOptionsRailWidget::HandleMapClicked);
	if (MercenaryButton) MercenaryButton->OnClicked.AddUniqueDynamic(this, &URunOptionsRailWidget::HandleMercenaryClicked);
	if (MonsterButton) MonsterButton->OnClicked.AddUniqueDynamic(this, &URunOptionsRailWidget::HandleMonsterClicked);
	if (SettingsButton) SettingsButton->OnClicked.AddUniqueDynamic(this, &URunOptionsRailWidget::HandleSettingsClicked);
}

void URunOptionsRailWidget::SetMapContext(const bool bInMapContext)
{
	bMapContext = bInMapContext;
	if (MapButton != nullptr)
	{
		MapButton->SetIsEnabled(!bMapContext);
	}
	if (MapIcon != nullptr)
	{
		MapIcon->SetRenderOpacity(bMapContext ? 1.f : .86f);
		MapIcon->SetColorAndOpacity(bMapContext
			? FLinearColor(1.f, .82f, .32f, 1.f) : FLinearColor::White);
	}
}

void URunOptionsRailWidget::HandleMapClicked()
{
	if (bMapContext || GetWorld() == nullptr)
	{
		return;
	}
	UWorldWidgetSubsystem* Subsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>();
	if (Subsystem == nullptr) return;
	UFrontendMapWidget* Map = Subsystem->GetWorldWidget<UFrontendMapWidget>(EWorldWidgetType::WorldMap);
	if (Map == nullptr)
	{
		Subsystem->InitWorldWidget(EWorldWidgetType::WorldMap);
		Map = Subsystem->GetWorldWidget<UFrontendMapWidget>(EWorldWidgetType::WorldMap);
	}
	if (Map != nullptr)
	{
		if (OpenedMapWidget != nullptr && OpenedMapWidget != Map)
		{
			OpenedMapWidget->OnCloseRequested.RemoveDynamic(
				this, &URunOptionsRailWidget::HandleMapCloseRequested);
		}
		OpenedMapWidget = Map;
		Map->OnCloseRequested.AddUniqueDynamic(
			this, &URunOptionsRailWidget::HandleMapCloseRequested);
		Map->SetRoomSelectionEnabled(false);
		Map->RefreshMap();
		Map->OpenUI();
	}
}

void URunOptionsRailWidget::HandleMapCloseRequested()
{
	UFrontendMapWidget* Map = OpenedMapWidget;
	OpenedMapWidget = nullptr;
	if (Map == nullptr)
	{
		return;
	}
	Map->OnCloseRequested.RemoveDynamic(
		this, &URunOptionsRailWidget::HandleMapCloseRequested);
	Map->CloseUI();
}

void URunOptionsRailWidget::HandleMercenaryClicked()
{
	OpenMercenaryPanel();
}

void URunOptionsRailWidget::HandleMonsterClicked()
{
	// 비전투 방에서는 버튼이 비활성이다. UFUNCTION은 WBP 호환을 위해 남긴다.
}

void URunOptionsRailWidget::HandleSettingsClicked()
{
	if (GetWorld() == nullptr) return;
	UWorldWidgetSubsystem* Subsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>();
	if (Subsystem == nullptr) return;
	USettingsPanelWidget* Settings = Subsystem->GetWorldWidget<USettingsPanelWidget>(
		EWorldWidgetType::InGameSettings);
	if (Settings == nullptr)
	{
		Subsystem->InitWorldWidget(EWorldWidgetType::InGameSettings);
		Settings = Subsystem->GetWorldWidget<USettingsPanelWidget>(EWorldWidgetType::InGameSettings);
	}
	if (Settings != nullptr)
	{
		Settings->SetPanelMode(ESettingsPanelMode::InGame);
		Settings->RefreshPanelState(true, true);
		Settings->HideAbandonConfirm();
		Settings->SetStatusText(FText::GetEmpty());
		Settings->OpenUI();
	}
}

void URunOptionsRailWidget::OpenMercenaryPanel()
{
	if (MercenaryPanelWidget != nullptr)
	{
		RefreshMercenaryPanel();
		return;
	}
	// 모듈러 WBP_MercenaryPanel은 전투 HUD에서 더 이상 쓰는 최종 판이 아니다.
	// 전투 HUD는 WBP_CombatHUD04 안의 반응형 MercenaryPanel을 사용한다. 상점과
	// 지도에서도 정확히 그 판을 호스트해 폴드 화면의 ScaleBox 계보까지 공유한다.
	UClass* PanelClass = LoadClass<UUserWidget>(nullptr,
		TEXT("/Game/UI/CombatLayouts/WBP_CombatHUD04.WBP_CombatHUD04_C"));
	if (PanelClass == nullptr) return;
	if (APlayerController* Owner = GetOwningPlayer())
	{
		MercenaryPanelWidget = CreateWidget<UUserWidget>(Owner, PanelClass);
	}
	else if (UWorld* World = GetWorld())
	{
		MercenaryPanelWidget = CreateWidget<UUserWidget>(World, PanelClass);
	}
	if (MercenaryPanelWidget == nullptr) return;
	if (UCombatLayoutHUDWidget* CombatHost =
		Cast<UCombatLayoutHUDWidget>(MercenaryPanelWidget))
	{
		CombatHost->mUsePreviewData = false;
	}
	MercenaryPanelWidget->AddToViewport(10010);
	MercenaryPanelWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UWidget* MercenaryPanel = FindWidget<UWidget>(
		MercenaryPanelWidget, TEXT("MercenaryPanel"));
	// 전체 전투 HUD WBP에서 공용 용병 판만 남긴다. 이렇게 해야 외형뿐 아니라
	// 전투 HUD가 검증한 1920x1080 -> viewport ScaleBox 구조를 그대로 사용한다.
	if (MercenaryPanelWidget->WidgetTree != nullptr)
	{
		if (UPanelWidget* Root = Cast<UPanelWidget>(
			MercenaryPanelWidget->WidgetTree->RootWidget))
		{
			for (UWidget* Child : Root->GetAllChildren())
			{
				if (Child != MercenaryPanel)
				{
					Child->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
		}
	}
	SetShown(MercenaryPanel, true, true);
	if (UButton* Close = FindWidget<UButton>(MercenaryPanelWidget, TEXT("MercenaryCloseButton")))
	{
		Close->OnClicked.AddUniqueDynamic(this, &URunOptionsRailWidget::HandleMercenaryCloseClicked);
	}
	if (UButton* Inventory = FindWidget<UButton>(MercenaryPanelWidget, TEXT("MercenaryInventoryButton")))
	{
		Inventory->OnClicked.AddUniqueDynamic(this, &URunOptionsRailWidget::HandleInventoryClicked);
	}
	const FName PartyHandlers[3] = {
		GET_FUNCTION_NAME_CHECKED(URunOptionsRailWidget, HandlePartyClicked0),
		GET_FUNCTION_NAME_CHECKED(URunOptionsRailWidget, HandlePartyClicked1),
		GET_FUNCTION_NAME_CHECKED(URunOptionsRailWidget, HandlePartyClicked2) };
	for (int32 Index = 0; Index < 3; ++Index)
	{
		if (UButton* Button = FindWidget<UButton>(MercenaryPanelWidget,
			*FString::Printf(TEXT("PartyButton_%d"), Index)))
		{
			FScriptDelegate Delegate;
			Delegate.BindUFunction(this, PartyHandlers[Index]);
			Button->OnClicked.AddUnique(Delegate);
		}
	}
	const FName SkillHandlers[6] = {
		GET_FUNCTION_NAME_CHECKED(URunOptionsRailWidget, HandleSkillClicked0),
		GET_FUNCTION_NAME_CHECKED(URunOptionsRailWidget, HandleSkillClicked1),
		GET_FUNCTION_NAME_CHECKED(URunOptionsRailWidget, HandleSkillClicked2),
		GET_FUNCTION_NAME_CHECKED(URunOptionsRailWidget, HandleSkillClicked3),
		GET_FUNCTION_NAME_CHECKED(URunOptionsRailWidget, HandleSkillClicked4),
		GET_FUNCTION_NAME_CHECKED(URunOptionsRailWidget, HandleSkillClicked5) };
	for (int32 Index = 0; Index < 6; ++Index)
	{
		if (UButton* Button = FindWidget<UButton>(MercenaryPanelWidget,
			*FString::Printf(TEXT("MercenarySkillButton_%d"), Index)))
		{
			FScriptDelegate Delegate;
			Delegate.BindUFunction(this, SkillHandlers[Index]);
			Button->OnClicked.AddUnique(Delegate);
		}
	}
	const FName SkillPressedHandlers[6] = {
		GET_FUNCTION_NAME_CHECKED(URunOptionsRailWidget, HandleSkillPressed0),
		GET_FUNCTION_NAME_CHECKED(URunOptionsRailWidget, HandleSkillPressed1),
		GET_FUNCTION_NAME_CHECKED(URunOptionsRailWidget, HandleSkillPressed2),
		GET_FUNCTION_NAME_CHECKED(URunOptionsRailWidget, HandleSkillPressed3),
		GET_FUNCTION_NAME_CHECKED(URunOptionsRailWidget, HandleSkillPressed4),
		GET_FUNCTION_NAME_CHECKED(URunOptionsRailWidget, HandleSkillPressed5) };
	for (int32 Index = 0; Index < 6; ++Index)
	{
		if (UButton* Button = FindWidget<UButton>(MercenaryPanelWidget,
			*FString::Printf(TEXT("MercenarySkillButton_%d"), Index)))
		{
			FScriptDelegate Pressed;
			Pressed.BindUFunction(this, SkillPressedHandlers[Index]);
			Button->OnPressed.AddUnique(Pressed);
			Button->OnReleased.AddUniqueDynamic(
				this, &URunOptionsRailWidget::HandleDetailReleased);
		}
	}
	const FName ArtifactHandlers[7] = {
		GET_FUNCTION_NAME_CHECKED(URunOptionsRailWidget, HandleArtifactClicked0),
		GET_FUNCTION_NAME_CHECKED(URunOptionsRailWidget, HandleArtifactClicked1),
		GET_FUNCTION_NAME_CHECKED(URunOptionsRailWidget, HandleArtifactClicked2),
		GET_FUNCTION_NAME_CHECKED(URunOptionsRailWidget, HandleArtifactClicked3),
		GET_FUNCTION_NAME_CHECKED(URunOptionsRailWidget, HandleArtifactClicked4),
		GET_FUNCTION_NAME_CHECKED(URunOptionsRailWidget, HandleArtifactClicked5),
		GET_FUNCTION_NAME_CHECKED(URunOptionsRailWidget, HandleArtifactClicked6) };
	for (int32 Index = 0; Index < 7; ++Index)
	{
		if (UButton* Button = FindWidget<UButton>(MercenaryPanelWidget,
			*FString::Printf(TEXT("MercenaryInventoryArtifactButton_%d"), Index)))
		{
			FScriptDelegate Delegate;
			Delegate.BindUFunction(this, ArtifactHandlers[Index]);
			Button->OnClicked.AddUnique(Delegate);
		}
	}
	const FName ArtifactPressedHandlers[7] = {
		GET_FUNCTION_NAME_CHECKED(URunOptionsRailWidget, HandleArtifactPressed0),
		GET_FUNCTION_NAME_CHECKED(URunOptionsRailWidget, HandleArtifactPressed1),
		GET_FUNCTION_NAME_CHECKED(URunOptionsRailWidget, HandleArtifactPressed2),
		GET_FUNCTION_NAME_CHECKED(URunOptionsRailWidget, HandleArtifactPressed3),
		GET_FUNCTION_NAME_CHECKED(URunOptionsRailWidget, HandleArtifactPressed4),
		GET_FUNCTION_NAME_CHECKED(URunOptionsRailWidget, HandleArtifactPressed5),
		GET_FUNCTION_NAME_CHECKED(URunOptionsRailWidget, HandleArtifactPressed6) };
	for (int32 Index = 0; Index < 7; ++Index)
	{
		if (UButton* Button = FindWidget<UButton>(MercenaryPanelWidget,
			*FString::Printf(TEXT("MercenaryInventoryArtifactButton_%d"), Index)))
		{
			FScriptDelegate Pressed;
			Pressed.BindUFunction(this, ArtifactPressedHandlers[Index]);
			Button->OnPressed.AddUnique(Pressed);
			Button->OnReleased.AddUniqueDynamic(
				this, &URunOptionsRailWidget::HandleDetailReleased);
		}
	}
	SelectedPartyMember = 0;
	bInventoryPageShown = false;
	RefreshMercenaryPanel();
}

void URunOptionsRailWidget::CloseMercenaryPanel()
{
	EndDetailPress();
	CloseDetailOverlay();
	if (MercenaryPanelWidget != nullptr)
	{
		MercenaryPanelWidget->RemoveFromParent();
		MercenaryPanelWidget = nullptr;
	}
	PartyRoster = FPartyRosterView();
	ShownSkillViews.Reset();
	ShownSkillSlotIndices.Reset();
	ShownArtifactViews.Reset();
	ShownArtifactIndices.Reset();
}

void URunOptionsRailWidget::HandleMercenaryCloseClicked() { CloseMercenaryPanel(); }
void URunOptionsRailWidget::HandleInventoryClicked()
{
	SetInventoryPageShown(!bInventoryPageShown);
}

void URunOptionsRailWidget::SetInventoryPageShown(const bool bShown)
{
	bInventoryPageShown = bShown;
	// 네 번째 행도 전투 HUD의 세 용병 행과 같은 양피지 판을 사용한다.
	// WBP에 저장된 과거 남색 판을 그대로 두면 비전투 방 어댑터에서만
	// 인벤토리가 다른 버튼처럼 보이므로 페이지 상태에 맞춰 명시적으로 교체한다.
	SetImage(FindWidget<UImage>(MercenaryPanelWidget,
		TEXT("MercenaryInventoryTabPlate")),
		bShown ? MercenaryCardSelectedTexture : MercenaryCardNormalTexture);
	SetShown(FindWidget<UWidget>(MercenaryPanelWidget, TEXT("MercenaryInventoryPage")), bShown);
	SetShown(FindWidget<UWidget>(MercenaryPanelWidget, TEXT("MercDetailSection")), !bShown);
	SetText(MercenaryPanelWidget, TEXT("MercenaryTitleText"), bShown
		? LOCTEXT("InventoryTitle", "인벤토리") : LOCTEXT("MercenaryTitle", "용병"));
	static const TCHAR* DetailNames[] = {
		TEXT("MercenaryHeroPortrait"), TEXT("MercenaryPortraitFrame"),
		TEXT("MercenaryNamePlate"), TEXT("MercenaryDetailName"),
		TEXT("MercenaryDetailHP"), TEXT("MercenaryDetailAP"),
		TEXT("MercenaryDetailSpeed"), TEXT("MercenaryCritPlate"),
		TEXT("MercenaryCritIcon"),
		TEXT("MercenaryCritLabel"), TEXT("MercenaryCritValue"),
		TEXT("MercenarySkillHeading"), TEXT("MercenarySkillDivider") };
	for (const TCHAR* Name : DetailNames)
	{
		SetShown(FindWidget<UWidget>(MercenaryPanelWidget, Name), !bShown);
	}
	if (UWidget* Plate = FindWidget<UWidget>(MercenaryPanelWidget,
		TEXT("MercenaryInventoryTabPlate")))
	{
		UWidget* Center = FindWidget<UWidget>(MercenaryPanelWidget,
			TEXT("MercenaryInventoryTabText_Center"));
		if (UCanvasPanelSlot* PlateSlot = Cast<UCanvasPanelSlot>(Plate->Slot))
		{
			if (UCanvasPanelSlot* CenterSlot = Center != nullptr
				? Cast<UCanvasPanelSlot>(Center->Slot) : nullptr)
			{
				constexpr float LabelHeight = 46.f;
				CenterSlot->SetPosition(PlateSlot->GetPosition() + FVector2D(0.f,
					(PlateSlot->GetSize().Y - LabelHeight) * .5f));
				CenterSlot->SetSize(FVector2D(PlateSlot->GetSize().X, LabelHeight));
				Center->SetRenderTransform(FWidgetTransform());
			}
		}
	}
	if (UTextBlock* InventoryText = FindWidget<UTextBlock>(MercenaryPanelWidget,
		TEXT("MercenaryInventoryTabText")))
	{
		InventoryText->SetJustification(ETextJustify::Center);
		InventoryText->SetRenderTransform(FWidgetTransform());
	}
	if (bShown)
	{
		for (int32 Index = 0; Index < 6; ++Index)
		{
			for (const TCHAR* Prefix : { TEXT("MercenarySkillFrame"), TEXT("MercenarySkillIcon"),
				TEXT("MercenarySkillName"), TEXT("MercenarySkillCost") })
			{
				SetShown(FindWidget<UWidget>(MercenaryPanelWidget,
					*FString::Printf(TEXT("%s_%d"), Prefix, Index)), false);
			}
			SetShown(FindWidget<UButton>(MercenaryPanelWidget,
				*FString::Printf(TEXT("MercenarySkillButton_%d"), Index)), false, true);
		}
	}
	else
	{
		/*
		 * 일괄 재표시하면 브러시 없는 아이콘 Image가 흰 사각으로 뜬다.
		 * 슬롯별 상태(스킬 유무·아이콘 유무)를 다시 계산해서 편다.
		 */
		ApplyMercenarySkillSlots();
	}
	RefreshInventoryPage();
}

void URunOptionsRailWidget::ApplyMercenarySkillSlots()
{
	if (MercenaryPanelWidget == nullptr) return;
	for (int32 Index = 0; Index < 6; ++Index)
	{
		const FPartyRosterSkillView* Skill = ShownSkillViews.IsValidIndex(Index)
			? &ShownSkillViews[Index] : nullptr;
		UTexture2D* IconTexture = Skill != nullptr ? Skill->mIcon.Get() : nullptr;
		SetShown(FindWidget<UWidget>(MercenaryPanelWidget,
			*FString::Printf(TEXT("MercenarySkillFrame_%d"), Index)), Skill != nullptr);
		SetImage(FindWidget<UImage>(MercenaryPanelWidget,
			*FString::Printf(TEXT("MercenarySkillIcon_%d"), Index)), IconTexture);
		SetText(MercenaryPanelWidget, *FString::Printf(TEXT("MercenarySkillName_%d"), Index),
			Skill != nullptr ? Skill->mName : FText::GetEmpty());
		/* 아이콘 없는 스킬은 이름 텍스트로 대신한다 -- 고용 화면과 같은 규칙. */
		SetShown(FindWidget<UWidget>(MercenaryPanelWidget,
			*FString::Printf(TEXT("MercenarySkillName_%d"), Index)),
			Skill != nullptr && IconTexture == nullptr);
		/* 비용 미상(mActionPointCost < 0)은 유닛 스킬이 아닌 것 -- 표기 생략. */
		SetText(MercenaryPanelWidget, *FString::Printf(TEXT("MercenarySkillCost_%d"), Index),
			Skill != nullptr && Skill->mActionPointCost >= 0
				? FText::AsNumber(Skill->mActionPointCost) : FText::GetEmpty());
		SetShown(FindWidget<UWidget>(MercenaryPanelWidget,
			*FString::Printf(TEXT("MercenarySkillCost_%d"), Index)), Skill != nullptr);
		if (UButton* Button = FindWidget<UButton>(MercenaryPanelWidget,
			*FString::Printf(TEXT("MercenarySkillButton_%d"), Index)))
		{
			Button->SetIsEnabled(Skill != nullptr);
			SetShown(Button, Skill != nullptr, true);
		}
	}
}

void URunOptionsRailWidget::RefreshMercenaryPanel()
{
	ARoomGameModeBase* GameMode = GetWorld() != nullptr
		? GetWorld()->GetAuthGameMode<ARoomGameModeBase>() : nullptr;
	if (GameMode == nullptr || MercenaryPanelWidget == nullptr) return;
	// 위젯은 모델을 직접 읽지 않는다 -- GameMode의 pull API 로 View 만 받는다
	// (codeGlutton PR #426 규칙).
	PartyRoster = FPartyRosterView();
	if (GameMode->GetPartyRosterView(OUT PartyRoster) == false)
	{
		return;
	}
	SelectedPartyMember = FMath::Clamp(SelectedPartyMember, 0,
		FMath::Max(PartyRoster.mMembers.Num() - 1, 0));

	for (int32 Index = 0; Index < 3; ++Index)
	{
		const FPartyRosterMemberView* Member = PartyRoster.mMembers.IsValidIndex(Index)
			? &PartyRoster.mMembers[Index] : nullptr;
		// 전투 HUD 확정 규격과 동일하게 목록 줄에는 초상·이름·Lv만 남긴다.
		// WBP에는 과거 HP/AP 표시 부품도 보존돼 있으므로 비전투 방에서 별도
		// View 를 사용할 때도 전투 HUD의 표시 정책을 명시적으로 적용한다.
		for (const TCHAR* Prefix : {
			TEXT("PartyHPBar"), TEXT("PartyHPText"), TEXT("PartyAPText"),
			TEXT("PartyAPPlate"), TEXT("PartyStatus"), TEXT("PartyStatusIcon"),
			TEXT("PartyAPPipRow"), TEXT("PartyStatusRow") })
		{
			SetShown(FindWidget<UWidget>(MercenaryPanelWidget,
				*FString::Printf(TEXT("%s_%d"), Prefix, Index)), false);
		}
		UWidget* Content = FindWidget<UWidget>(MercenaryPanelWidget,
			*FString::Printf(TEXT("PartyContent_%d"), Index));
		SetShown(Content, Member != nullptr);
		if (UButton* Button = FindWidget<UButton>(MercenaryPanelWidget,
			*FString::Printf(TEXT("PartyButton_%d"), Index)))
		{
			Button->SetIsEnabled(Member != nullptr);
		}
		if (UImage* Plate = FindWidget<UImage>(MercenaryPanelWidget,
			*FString::Printf(TEXT("PartyPlate_%d"), Index)))
		{
			SetImage(Plate, Index == SelectedPartyMember
				? MercenaryCardSelectedTexture : MercenaryCardNormalTexture);
			Plate->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		if (Member == nullptr)
		{
			SetText(MercenaryPanelWidget,
				*FString::Printf(TEXT("PartyName_%d"), Index), LOCTEXT("EmptyPartySlot", "빈 자리"));
			continue;
		}
		SetCenteredText(MercenaryPanelWidget,
			*FString::Printf(TEXT("PartyName_%d"), Index),
			Member->mJobName);
		SetCenteredText(MercenaryPanelWidget,
			*FString::Printf(TEXT("PartyLevel_%d"), Index),
			FText::FromString(FString::Printf(TEXT("Lv %d"), Member->mLevel)));
		SetImage(FindWidget<UImage>(MercenaryPanelWidget,
			*FString::Printf(TEXT("PartyPortrait_%d"), Index)),
			Member->mPortrait);
	}

	const FPartyRosterMemberView* Selected =
		PartyRoster.mMembers.IsValidIndex(SelectedPartyMember)
		? &PartyRoster.mMembers[SelectedPartyMember] : nullptr;
	if (Selected != nullptr)
	{
		SetCenteredText(MercenaryPanelWidget, TEXT("MercenaryDetailName"),
			Selected->mJobName);
		SetImage(FindWidget<UImage>(MercenaryPanelWidget, TEXT("MercenaryHeroPortrait")),
			Selected->mPortrait);
		SetText(MercenaryPanelWidget, TEXT("MercenaryDetailHP"),
			FText::FromString(FString::Printf(TEXT("%d/%d"),
				Selected->mHP, Selected->mMaxHP)));
		SetText(MercenaryPanelWidget, TEXT("MercenaryDetailAP"),
			FText::FromString(FString::Printf(TEXT("%d/%d"),
				Selected->mAP, Selected->mMaxAP)));
		SetText(MercenaryPanelWidget, TEXT("MercenaryDetailSpeed"),
			FText::AsNumber(Selected->mSpeed));
	}

	ShownSkillViews.Reset();
	ShownSkillSlotIndices.Reset();
	if (Selected != nullptr)
	{
		for (const FPartyRosterSkillView& Skill : Selected->mSkills)
		{
			if (ShownSkillViews.Num() >= 6)
			{
				break;
			}
			ShownSkillViews.Add(Skill);
			/* 상세 조회는 컴포넌트 원본 슬롯 index로 해야 한다. */
			ShownSkillSlotIndices.Add(Skill.mSlotIndex);
		}
	}
	SetInventoryPageShown(bInventoryPageShown);
}

void URunOptionsRailWidget::RefreshInventoryPage()
{
	if (!bInventoryPageShown || MercenaryPanelWidget == nullptr) return;
	ARoomGameModeBase* GameMode = GetWorld() != nullptr
		? GetWorld()->GetAuthGameMode<ARoomGameModeBase>() : nullptr;
	// 가방도 밀지 않고 물어본다 -- 골드/아티팩트는 파티 공용 소유라 명단
	// View(GetPartyRosterView) 한 판에 실려 온다. 별도 인벤토리 조회는 없다.
	FPartyRosterView Roster;
	if (GameMode == nullptr || GameMode->GetPartyRosterView(OUT Roster) == false)
	{
		return;
	}
	SetText(MercenaryPanelWidget, TEXT("MercenaryInventoryGoldText"),
		FText::AsNumber(Roster.mGold));
	ShownArtifactViews.Reset();
	ShownArtifactIndices.Reset();
	for (const FPartyRosterArtifactView& Artifact : Roster.mArtifacts)
	{
		if (ShownArtifactViews.Num() >= 7)
		{
			break;
		}
		ShownArtifactViews.Add(Artifact);
		/* 상세 조립은 파티 목록 원본 index로 해야 한다 -- 스킬과 같은 규칙. */
		ShownArtifactIndices.Add(Artifact.mArtifactIndex);
	}
	for (int32 Index = 0; Index < 7; ++Index)
	{
		const FPartyRosterArtifactView* Artifact = ShownArtifactViews.IsValidIndex(Index)
			? &ShownArtifactViews[Index] : nullptr;
		SetShown(FindWidget<UWidget>(MercenaryPanelWidget,
			*FString::Printf(TEXT("MercenaryInventoryArtifactFrame_%d"), Index)), true);
		SetImage(FindWidget<UImage>(MercenaryPanelWidget,
			*FString::Printf(TEXT("MercenaryInventoryArtifactIcon_%d"), Index)),
			Artifact != nullptr ? Artifact->mIcon.Get() : nullptr);
		SetText(MercenaryPanelWidget,
			*FString::Printf(TEXT("MercenaryInventoryArtifactName_%d"), Index),
			Artifact != nullptr ? Artifact->mName : FText::GetEmpty());
		if (UButton* Button = FindWidget<UButton>(MercenaryPanelWidget,
			*FString::Printf(TEXT("MercenaryInventoryArtifactButton_%d"), Index)))
		{
			Button->SetIsEnabled(Artifact != nullptr);
		}
	}
}

void URunOptionsRailWidget::SelectPartyMember(const int32 MemberIndex)
{
	if (PartyRoster.mMembers.IsValidIndex(MemberIndex))
	{
		SelectedPartyMember = MemberIndex;
		bInventoryPageShown = false;
		RefreshMercenaryPanel();
	}
}

bool URunOptionsRailWidget::EnsureDetailOverlay()
{
	if (DetailOverlayWidget != nullptr) return true;
	UClass* DetailClass = LoadClass<UUserWidget>(nullptr,
		TEXT("/Game/UI/CombatDetail/WBP_CombatDetailOverlay.WBP_CombatDetailOverlay_C"));
	if (DetailClass == nullptr) return false;
	if (APlayerController* Owner = GetOwningPlayer())
	{
		DetailOverlayWidget = CreateWidget<UUserWidget>(Owner, DetailClass);
	}
	else if (UWorld* World = GetWorld())
	{
		DetailOverlayWidget = CreateWidget<UUserWidget>(World, DetailClass);
	}
	if (DetailOverlayWidget == nullptr) return false;
	DetailOverlayWidget->AddToViewport(10020);
	DetailOverlayWidget->SetVisibility(ESlateVisibility::Collapsed);
	RDDetailOverlay::EnsureModalInputShield(DetailOverlayWidget);
	if (UButton* CloseButton = FindWidget<UButton>(
		DetailOverlayWidget, TEXT("DetailCloseButton")))
	{
		CloseButton->OnClicked.AddUniqueDynamic(
			this, &URunOptionsRailWidget::HandleDetailCloseClicked);
	}
	return true;
}

void URunOptionsRailWidget::PresentDetail(const FText& Name, const FText& Subtitle,
	const FText& Description, UTexture2D* Icon, const bool bArtifact)
{
	if (!EnsureDetailOverlay()) return;
	SetText(DetailOverlayWidget, TEXT("DetailTitleText"), Name);
	SetText(DetailOverlayWidget, TEXT("DetailSubtitleText"), Subtitle);
	SetText(DetailOverlayWidget, TEXT("DetailExtraHeading"),
		bArtifact ? LOCTEXT("ArtifactEffects", "효과") : LOCTEXT("SkillEffects", "스킬 효과"));
	SetText(DetailOverlayWidget, TEXT("DetailExtraText"), Description);
	SetText(DetailOverlayWidget, TEXT("DetailBodyText"), FText::Format(
		bArtifact ? LOCTEXT("ArtifactBody", "효과\n{0}")
			: LOCTEXT("SkillBody", "설명\n{0}"), Description));
	SetImage(FindWidget<UImage>(DetailOverlayWidget, TEXT("DetailIconImage")), Icon);
	auto Show = [this](const TCHAR* WidgetName, const bool bShown)
	{
		SetShown(FindWidget<UWidget>(DetailOverlayWidget, WidgetName), bShown);
	};
	Show(TEXT("DetailIdentityColumn"), true);
	Show(TEXT("DetailStatColumn"), false);
	Show(TEXT("DetailRightColumn"), false);
	Show(TEXT("DetailWideColumn"), true);
	Show(TEXT("DetailStatBlock"), false);
	Show(TEXT("DetailTargetBlock"), false);
	Show(TEXT("DetailSkillBlock"), false);
	Show(TEXT("DetailExtraBlock"), false);
	Show(TEXT("DetailFreePlate"), false);
	Show(TEXT("DetailBodyText"), true);
	Show(TEXT("DetailSkillRowHost"), false);
	// 공용 WBP를 직전에 어떤 상세가 사용했는지와 무관하게 아티팩트/스킬
	// 상세의 중앙 구분선은 항상 제거한다.
	Show(TEXT("DetailDivider_0"), false);
	Show(TEXT("DetailDivider_1"), false);
	DetailOverlayWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void URunOptionsRailWidget::ShowSkillDetail(const int32 SlotIndex)
{
	if (!ShownSkillSlotIndices.IsValidIndex(SlotIndex)) return;

	/*
	 * 전투 HUD와 같은 리치 상세를 쓴다. DTO 조립은 GameMode 몫이다 --
	 * 위젯이 모델을 직접 읽던 구식 경로(PR #426 위반)를 걷어냈다.
	 */
	ARoomGameModeBase* GameMode = GetWorld() != nullptr
		? GetWorld()->GetAuthGameMode<ARoomGameModeBase>() : nullptr;
	const int32 MemberIndex = PartyRoster.mMembers.IsValidIndex(SelectedPartyMember)
		? PartyRoster.mMembers[SelectedPartyMember].mMemberIndex
		: SelectedPartyMember;
	FSkillDetailUI Detail;
	if (GameMode == nullptr || GameMode->BuildPartyUnitSkillDetailUI(
		MemberIndex, ShownSkillSlotIndices[SlotIndex], OUT Detail) == false)
	{
		// 방 전환 직후 모델 상세가 아직 준비되지 않았더라도 빈 창/무반응 대신
		// 이미 그려진 로스터 View의 기본 정보를 즉시 보여 준다.
		if (!ShownSkillViews.IsValidIndex(SlotIndex)) return;
		const FPartyRosterSkillView& Skill = ShownSkillViews[SlotIndex];
		Detail.mSkillIndex = Skill.mSlotIndex;
		Detail.mName = Skill.mName;
		Detail.mIcon = Skill.mIcon;
		Detail.mActionPointCost = FMath::Max(Skill.mActionPointCost, 0);
		Detail.mDescription = LOCTEXT("SkillDetailFallback",
			"보유 용병이 장착한 스킬입니다.");
	}
	if (EnsureSkillDetailPresenter() == false
		|| SkillDetailPresenter->EnsureOverlayWidget(GetOwningPlayer()) == false)
	{
		return;
	}
	SkillDetailPresenter->Present(Detail);
}

bool URunOptionsRailWidget::EnsureSkillDetailPresenter()
{
	if (SkillDetailPresenter != nullptr) return true;
	UClass* OverlayClass = LoadClass<UUserWidget>(nullptr,
		TEXT("/Game/UI/CombatDetail/WBP_CombatDetailOverlay.WBP_CombatDetailOverlay_C"));
	UClass* DiagramClass = LoadClass<USkillTacticalDiagramWidget>(nullptr,
		TEXT("/Game/UI/CombatDetail/SkillTactical/WBP_SkillTacticalDiagram.WBP_SkillTacticalDiagram_C"));
	if (OverlayClass == nullptr) return false;
	SkillDetailPresenter = NewObject<USkillDetailOverlayPresenter>(this);
	/* 레일의 플레인 겹과 같은 층(10020)에 얹어 지도/상점 팝업 위에 오게 한다. */
	SkillDetailPresenter->Initialize(GetWorld(), OverlayClass, DiagramClass, 10020);
	return true;
}

void URunOptionsRailWidget::ShowArtifactDetail(const int32 SlotIndex)
{
	if (!ShownArtifactViews.IsValidIndex(SlotIndex)) return;

	/*
	 * 전투 HUD와 같은 리치 아티팩트 상세를 쓴다. DTO 조립은 GameMode 몫이다 --
	 * 스킬 상세(ShowSkillDetail)와 같은 규칙 (PR #426).
	 */
	ARoomGameModeBase* GameMode = GetWorld() != nullptr
		? GetWorld()->GetAuthGameMode<ARoomGameModeBase>() : nullptr;
	FCombatArtifactUI Detail;
	if (GameMode != nullptr && ShownArtifactIndices.IsValidIndex(SlotIndex)
		&& GameMode->BuildPartyArtifactDetailUI(
			ShownArtifactIndices[SlotIndex], OUT Detail)
		&& EnsureSkillDetailPresenter()
		&& SkillDetailPresenter->EnsureOverlayWidget(GetOwningPlayer()))
	{
		SkillDetailPresenter->PresentArtifact(Detail);
		return;
	}

	/*
	 * 폴백: 리치 겹 WBP 부재(자동화 등)에서는 기존 플레인 겹으로 보여 준다.
	 * 위젯은 DA 를 다시 읽지 않고 View 값(이름/아이콘)만 쓴다.
	 */
	const FPartyRosterArtifactView& Artifact = ShownArtifactViews[SlotIndex];
	PresentDetail(Artifact.mName, LOCTEXT("ArtifactSubtitle", "아티팩트 · 파티 공용"),
		LOCTEXT("ArtifactFallback", "파티 전체에 적용됩니다."),
		Artifact.mIcon.Get(), true);
}

void URunOptionsRailWidget::BeginDetailPress(
	const bool bArtifact, const int32 SlotIndex)
{
	if (GetWorld() == nullptr)
	{
		return;
	}
	GetWorld()->GetTimerManager().ClearTimer(DetailLongPressTimer);
	PressedDetailSlot = SlotIndex;
	bPressedDetailIsArtifact = bArtifact;
	bDetailLongPressTriggered = false;
	GetWorld()->GetTimerManager().SetTimer(DetailLongPressTimer,
		this, &URunOptionsRailWidget::FireHeldDetail,
		DetailLongPressSeconds, false);
}

void URunOptionsRailWidget::EndDetailPress()
{
	if (GetWorld() != nullptr)
	{
		GetWorld()->GetTimerManager().ClearTimer(DetailLongPressTimer);
	}
	PressedDetailSlot = INDEX_NONE;
	bDetailLongPressTriggered = false;
}

void URunOptionsRailWidget::FireHeldDetail()
{
	if (PressedDetailSlot == INDEX_NONE)
	{
		return;
	}
	bDetailLongPressTriggered = true;
	if (bPressedDetailIsArtifact)
	{
		ShowArtifactDetail(PressedDetailSlot);
	}
	else
	{
		ShowSkillDetail(PressedDetailSlot);
	}
}

void URunOptionsRailWidget::CloseDetailOverlay()
{
	if (DetailOverlayWidget != nullptr)
	{
		DetailOverlayWidget->RemoveFromParent();
		DetailOverlayWidget = nullptr;
	}
}

void URunOptionsRailWidget::HandleDetailCloseClicked()
{
	if (DetailOverlayWidget != nullptr)
	{
		DetailOverlayWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void URunOptionsRailWidget::NativeDestruct()
{
	EndDetailPress();
	if (OpenedMapWidget != nullptr)
	{
		OpenedMapWidget->OnCloseRequested.RemoveDynamic(
			this, &URunOptionsRailWidget::HandleMapCloseRequested);
		OpenedMapWidget = nullptr;
	}
	if (SkillDetailPresenter != nullptr)
	{
		SkillDetailPresenter->Teardown();
		SkillDetailPresenter = nullptr;
	}
	CloseMercenaryPanel();
	Super::NativeDestruct();
}

void URunOptionsRailWidget::HandlePartyClicked0() { SelectPartyMember(0); }
void URunOptionsRailWidget::HandlePartyClicked1() { SelectPartyMember(1); }
void URunOptionsRailWidget::HandlePartyClicked2() { SelectPartyMember(2); }
void URunOptionsRailWidget::HandleSkillClicked0() { ShowSkillDetail(0); }
void URunOptionsRailWidget::HandleSkillClicked1() { ShowSkillDetail(1); }
void URunOptionsRailWidget::HandleSkillClicked2() { ShowSkillDetail(2); }
void URunOptionsRailWidget::HandleSkillClicked3() { ShowSkillDetail(3); }
void URunOptionsRailWidget::HandleSkillClicked4() { ShowSkillDetail(4); }
void URunOptionsRailWidget::HandleSkillClicked5() { ShowSkillDetail(5); }
void URunOptionsRailWidget::HandleSkillPressed0() { BeginDetailPress(false, 0); }
void URunOptionsRailWidget::HandleSkillPressed1() { BeginDetailPress(false, 1); }
void URunOptionsRailWidget::HandleSkillPressed2() { BeginDetailPress(false, 2); }
void URunOptionsRailWidget::HandleSkillPressed3() { BeginDetailPress(false, 3); }
void URunOptionsRailWidget::HandleSkillPressed4() { BeginDetailPress(false, 4); }
void URunOptionsRailWidget::HandleSkillPressed5() { BeginDetailPress(false, 5); }
void URunOptionsRailWidget::HandleArtifactClicked0() { ShowArtifactDetail(0); }
void URunOptionsRailWidget::HandleArtifactClicked1() { ShowArtifactDetail(1); }
void URunOptionsRailWidget::HandleArtifactClicked2() { ShowArtifactDetail(2); }
void URunOptionsRailWidget::HandleArtifactClicked3() { ShowArtifactDetail(3); }
void URunOptionsRailWidget::HandleArtifactClicked4() { ShowArtifactDetail(4); }
void URunOptionsRailWidget::HandleArtifactClicked5() { ShowArtifactDetail(5); }
void URunOptionsRailWidget::HandleArtifactClicked6() { ShowArtifactDetail(6); }
void URunOptionsRailWidget::HandleArtifactPressed0() { BeginDetailPress(true, 0); }
void URunOptionsRailWidget::HandleArtifactPressed1() { BeginDetailPress(true, 1); }
void URunOptionsRailWidget::HandleArtifactPressed2() { BeginDetailPress(true, 2); }
void URunOptionsRailWidget::HandleArtifactPressed3() { BeginDetailPress(true, 3); }
void URunOptionsRailWidget::HandleArtifactPressed4() { BeginDetailPress(true, 4); }
void URunOptionsRailWidget::HandleArtifactPressed5() { BeginDetailPress(true, 5); }
void URunOptionsRailWidget::HandleArtifactPressed6() { BeginDetailPress(true, 6); }
void URunOptionsRailWidget::HandleDetailReleased() { EndDetailPress(); }

#undef LOCTEXT_NAMESPACE
