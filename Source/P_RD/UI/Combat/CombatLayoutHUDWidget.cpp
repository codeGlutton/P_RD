#include "UI/Combat/CombatLayoutHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/Combat/MockCombatDriver.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "UI/Reward/RewardUIModel.h"
#include "UI/Reward/RewardSettlementWidgetBase.h"
#include "UI/CombatResultOverlayWidget.h"

#define LOCTEXT_NAMESPACE "CombatLayoutHUD"

/**
 * @brief 머리 위 HP 바가 쓰는 그림들을 미리 물어 둔다.
 *
 * @details
 * ini 강제 쿡 대신 생성자 하드 레퍼런스로 잡는다(#300 관행). 옛 HUD 가
 * 하던 것 중 HP 바에 필요한 것만 옮겼다 -- 소리와 상세창은 그 기능을
 * 옮길 때 같이 온다.
 */
UCombatLayoutHUDWidget::UCombatLayoutHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mMercenaryRosterShellTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MercenaryRoster_Shell.T_MercenaryRoster_Shell")));
	mMercenaryCardNormalTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_MercenaryCard_Normal.T_MB_MercenaryCard_Normal")));
	mMercenaryCardSelectedTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_MercenaryCard_Selected.T_MB_MercenaryCard_Selected")));

	static ConstructorHelpers::FClassFinder<UUserWidget> UnitHpBarClassFinder(
		TEXT("/Game/UI/CombatHUD/UnitHpBar/WBP_CombatUnitHpBar"));
	if (UnitHpBarClassFinder.Succeeded())
	{
		mUnitHpBarWidgetClass = UnitHpBarClassFinder.Class;
	}

#define RD_LOAD_TEX(Member, Path) 	{ static ConstructorHelpers::FObjectFinder<UTexture2D> Finder(TEXT(Path)); if (Finder.Succeeded()) { Member = Finder.Object; } }
	RD_LOAD_TEX(mUnitHpFillRedTexture,     "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/UnitHpBar/T_CombatHUD_UnitHpBar_Fill_Red.T_CombatHUD_UnitHpBar_Fill_Red");
	RD_LOAD_TEX(mUnitHpFillGreenTexture,   "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/UnitHpBar/T_CombatHUD_UnitHpBar_Fill_Green.T_CombatHUD_UnitHpBar_Fill_Green");
	RD_LOAD_TEX(mUnitDefenseIconTexture,   "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/UnitHpBar/T_UnitHpBar_Defense_Icon.T_UnitHpBar_Defense_Icon");
	RD_LOAD_TEX(mUnitStatusSlotTexture,    "/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_StatusSlot_Frame.T_MB_StatusSlot_Frame");
	RD_LOAD_TEX(mLogIconHpDamage,      "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/StatusIcons/T_Status_HP_Damage.T_Status_HP_Damage");
	RD_LOAD_TEX(mLogIconHpRecovery,    "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/StatusIcons/T_Status_HP_Recovery.T_Status_HP_Recovery");
	RD_LOAD_TEX(mLogIconGetMove,       "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/StatusIcons/T_Status_GetMove.T_Status_GetMove");
	RD_LOAD_TEX(mLogIconGetDefense,    "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/StatusIcons/T_Status_GetDefense.T_Status_GetDefense");
	RD_LOAD_TEX(mLogIconAgility,       "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/StatusIcons/T_Status_Agility.T_Status_Agility");
	RD_LOAD_TEX(mLogIconFortification, "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/StatusIcons/T_Status_Fortification.T_Status_Fortification");
	RD_LOAD_TEX(mLogIconVulnerability, "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/StatusIcons/T_Status_Vulnerability.T_Status_Vulnerability");
	RD_LOAD_TEX(mLogIconWeakness,      "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/StatusIcons/T_Status_Weakness.T_Status_Weakness");
#undef RD_LOAD_TEX

	static ConstructorHelpers::FClassFinder<URewardSettlementWidgetBase> RewardWidgetClassFinder(
		TEXT("/Game/UI/RewardSettlement/WBP_RewardSettlement_Runtime"));
	if (RewardWidgetClassFinder.Succeeded())
	{
		mRewardWidgetClass = RewardWidgetClassFinder.Class;
	}

	// 패배 WBP도 하드 레퍼런스로 든다. 문자열 LoadClass만 있으면 Always Cook
	// 목록에 없는 /Game/UI/CombatResult가 패키징에서 빠진다.
	static ConstructorHelpers::FClassFinder<UCombatResultOverlayWidget> DefeatWidgetClassFinder(
		TEXT("/Game/UI/CombatResult/WBP_CombatDefeat"));
	if (DefeatWidgetClassFinder.Succeeded())
	{
		mDefeatWidgetClass = DefeatWidgetClassFinder.Class;
	}

	static ConstructorHelpers::FClassFinder<UUserWidget> DetailOverlayClassFinder(
		TEXT("/Game/UI/CombatDetail/WBP_CombatDetailOverlay"));
	if (DetailOverlayClassFinder.Succeeded())
	{
		mDetailOverlayWidgetClass = DetailOverlayClassFinder.Class;
	}

	// 몬스터 탭도 보상창처럼 하드 레퍼런스로 든다 -- 문자열 LoadClass만 있으면
	// Always Cook 목록에 없는 이 WBP가 패키징에서 빠진다.
	static ConstructorHelpers::FClassFinder<UUserWidget> MonsterTabClassFinder(
		TEXT("/Game/UI/MonsterTab/WBP_MonsterTab_Marchbound"));
	if (MonsterTabClassFinder.Succeeded())
	{
		mMonsterTabWidgetClass = MonsterTabClassFinder.Class;
	}

	static ConstructorHelpers::FClassFinder<UUserWidget> WorldMapClassFinder(
		TEXT("/Game/UI/WBP_FrontendMap"));
	if (WorldMapClassFinder.Succeeded())
	{
		mWorldMapWidgetClass = WorldMapClassFinder.Class;
	}

#define RD_LOAD_SOUND(Member, Path) 	{ static ConstructorHelpers::FObjectFinder<USoundBase> Finder(TEXT(Path)); if (Finder.Succeeded()) { Member = Finder.Object; } }
	RD_LOAD_SOUND(mVictoryJingleSound, "/Game/SVN/OutSideAsset/Music/OpenGameArt/Jingle/BGM_Jingle_Victory_Fupi_CC0.BGM_Jingle_Victory_Fupi_CC0");
	RD_LOAD_SOUND(mDefeatJingleSound,  "/Game/SVN/OutSideAsset/Music/OpenGameArt/Jingle/BGM_Jingle_Defeat_Spuispuin_CCBY4.BGM_Jingle_Defeat_Spuispuin_CCBY4");
	RD_LOAD_SOUND(mExpGainSound,       "/Game/SVN/OutSideAsset/SFX/OpenGameArt_CC0/UI/SFX_XPGain_OGA_CC0_Rise03.SFX_XPGain_OGA_CC0_Rise03");
#undef RD_LOAD_SOUND
}

namespace
{
	/**
	 * @brief 초상을 칸에 맞춰 윗부분만 잘라 그린다.
	 *
	 * @details
	 * 초상은 972x1619 로 길쭉한데 칸은 정사각에 가깝다. 그냥 넣으면 가로로
	 * 눌려 얼굴이 찌그러진다.
	 *
	 * 여백을 두면 칸이 작아지고, 정사각본을 따로 만들면 자산이 배로 는다.
	 * 그래서 **UV 로 윗부분만** 쓴다 -- 얼굴이 위쪽에 있어 잘라도 다 나온다.
	 *
	 * @param Image  그릴 곳
	 * @param Texture 초상. 널이면 아무 일도 안 한다
	 * @param Aspect 칸의 가로/세로 비. 1 이면 정사각
	 */
	void SetPortraitCropped(UImage* Image, UTexture2D* Texture, const float Aspect = 1.f)
	{
		if (Image == nullptr || Texture == nullptr)
		{
			return;
		}
		const float Width = static_cast<float>(Texture->GetSizeX());
		const float Height = static_cast<float>(Texture->GetSizeY());
		if (Width <= 0.f || Height <= 0.f || Aspect <= 0.f)
		{
			Image->SetBrushFromTexture(Texture, false);
			return;
		}

		FSlateBrush Brush = Image->GetBrush();
		Brush.SetResourceObject(Texture);
		Brush.DrawAs = ESlateBrushDrawType::Image;

		// 칸보다 세로로 길면 위에서 잘라 쓰고, 가로로 길면 가운데를 쓴다.
		const float WantHeight = Width / Aspect;
		if (WantHeight < Height)
		{
			// UV만 정사각으로 잘라도 Brush의 원본 desired size가 세로로 길면
			// ScaleBox가 다시 폭을 눌러 버린다. 잘라 쓴 영역의 비율도 함께
			// 알려 얼굴이 정사각 crop wrapper 안에서 늘어나지 않게 한다.
			Brush.ImageSize = FVector2D(Width, WantHeight);
			Brush.SetUVRegion(FBox2f(FVector2f(0.f, 0.f),
				FVector2f(1.f, WantHeight / Height)));
		}
		else
		{
			const float WantWidth = Height * Aspect;
			const float Margin = (1.f - WantWidth / Width) * 0.5f;
			Brush.ImageSize = FVector2D(WantWidth, Height);
			Brush.SetUVRegion(FBox2f(FVector2f(Margin, 0.f),
				FVector2f(1.f - Margin, 1.f)));
		}
		Image->SetBrush(Brush);
	}

	/**
	 * @brief 턴바가 실제로 투영할 순서 수.
	 *
	 * 이번 라운드 잔여분 뒤에 다음 라운드 한 바퀴를 반드시 붙이고, 유닛이
	 * 적어도 열 칸은 미래 순서로 채운다.
	 */
	int32 ProjectedTurnCount(const FTurnUI& Turn, const int32 SlotRoom)
	{
		const int32 CycleCount = Turn.mTurnOrderUnitIds.Num();
		if (CycleCount <= 0)
		{
			return 0;
		}
		const int32 RemainingThisRound = FMath::Clamp(
			Turn.mCurrentRoundRemainingTurnCount > 0
				? Turn.mCurrentRoundRemainingTurnCount
				: CycleCount,
			1, CycleCount);
		return FMath::Max(SlotRoom, RemainingThisRound + CycleCount);
	}

	/** @brief 이름으로 위젯을 찾되 중첩 UserWidget 안까지 내려간다. */
	UWidget* FindDeep(const UWidgetTree* Tree, const FName Name)
	{
		if (Tree == nullptr)
		{
			return nullptr;
		}
		if (UWidget* Direct = Tree->FindWidget(Name))
		{
			return Direct;
		}
		UWidget* Result = nullptr;
		Tree->ForEachWidget([&Result, Name](UWidget* Candidate)
		{
			if (Result != nullptr)
			{
				return;
			}
			if (const UUserWidget* Nested = Cast<UUserWidget>(Candidate))
			{
				Result = FindDeep(Nested->WidgetTree, Name);
			}
		});
		return Result;
	}

	template <typename T>
	T* Find(const UWidgetTree* Tree, const FString& Name)
	{
		return Cast<T>(FindDeep(Tree, FName(*Name)));
	}

	/** @brief 있으면 보이고 없으면 접는다. 배치안마다 요소를 빼도 되게 하는 핵심. */
	void SetShown(UWidget* Widget, const bool bShown)
	{
		if (Widget != nullptr)
		{
			Widget->SetVisibility(bShown
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed);
		}
	}

	/** @brief 실제로 눌러야 하는 버튼은 표시 중 hit test를 유지한다. */
	void SetInteractiveShown(UButton* Button, const bool bShown)
	{
		if (Button != nullptr)
		{
			Button->SetVisibility(bShown
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed);
		}
	}

	void SetTextIfPresent(UTextBlock* Text, const FText& Value)
	{
		if (Text != nullptr)
		{
			Text->SetText(Value);
		}
	}

	/**
	 * @brief 상태이상 태그를 카드에 적을 짧은 이름으로.
	 *
	 * 태그 이름을 그대로 찍으면 "GameplayEffect.StatusEffect.TurnDuration.
	 * Debuff.Weakness" 가 아군 카드 한 줄에 들어간다. 잎 이름만 떼고, 아는
	 * 것은 우리말로 바꾼다. 모르는 태그는 잎 이름 그대로 -- 게임플레이가
	 * 새 상태를 추가해도 빈칸이 되지는 않는다.
	 */
	FString StatusDisplayName(const FGameplayTag& Tag)
	{
		FString Full = Tag.GetTagName().ToString();
		if (Full.IsEmpty())
		{
			return TEXT("이상");
		}

		FString Leaf = Full;
		int32 Dot = INDEX_NONE;
		if (Full.FindLastChar(TEXT('.'), Dot))
		{
			Leaf = Full.Mid(Dot + 1);
		}

		static const TMap<FString, FString> Korean = {
			{ TEXT("Weakness"),      TEXT("약화") },
			{ TEXT("Vulnerability"), TEXT("취약") },
			{ TEXT("Agility"),       TEXT("민첩") },
			{ TEXT("Fortification"), TEXT("강화") },
			{ TEXT("Dead"),          TEXT("전투불능") },
		};
		if (const FString* Found = Korean.Find(Leaf))
		{
			return *Found;
		}
		return Leaf;
	}
}

void UCombatLayoutHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CacheAuthoredWidgets();
	WireCommands();
	StartPreviewIfUnbound();
	// 실제 전투에서는 모델이 이미 채워진 뒤에 붙으므로, 붙자마자 한 번 그린다.
	// BindUIModel의 All 갱신은 위젯을 찾기 전에 올 수도 있다.
	NativeOnUIRefreshed(ECombatUIDomain::All);
}

/**
 * @brief WBP가 제공하는 위젯을 이름으로 모아 둔다.
 *
 * @details
 * 없는 위젯은 null로 남기고 그리기 단계에서 건너뛴다. 배치안마다 화면에 두는
 * 요소가 달라지므로 -- 어떤 안은 턴 순서를 아예 빼고, 어떤 안은 적 패널을
 * 탭했을 때만 띄운다 -- 필수 위젯을 두면 그 탐색이 막힌다.
 */
void UCombatLayoutHUDWidget::CacheAuthoredWidgets()
{
	if (mCached || WidgetTree == nullptr)
	{
		return;
	}

	// 런타임에 짓는 것들 -- 머리 위 HP 바, 플로팅 로그, 라운드 배너 -- 이 다
	// 여기에 붙는다. 옛 HUD 는 통짜 런타임 생성기가 잡아 주던 자리다.
	mRootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);

	mCached = true;

	mRoundText = Find<UTextBlock>(WidgetTree, TEXT("RoundText"));
	mObjectiveText = Find<UTextBlock>(WidgetTree, TEXT("ObjectiveText"));

	mPartySlots.SetNum(PartySlotCount);
	for (int32 Index = 0; Index < PartySlotCount; ++Index)
	{
		FPartySlotWidgets& Widgets = mPartySlots[Index];
		const FString Suffix = FString::Printf(TEXT("_%d"), Index);
		Widgets.Root = Find<UWidget>(WidgetTree, TEXT("PartyCard") + Suffix);
		Widgets.Plate = Find<UImage>(WidgetTree, TEXT("PartyPlate") + Suffix);
		Widgets.Content = Find<UWidget>(WidgetTree, TEXT("PartyContent") + Suffix);
		Widgets.Portrait = Find<UImage>(WidgetTree, TEXT("PartyPortrait") + Suffix);
		Widgets.Name = Find<UTextBlock>(WidgetTree, TEXT("PartyName") + Suffix);
		Widgets.HPBar = Find<UProgressBar>(WidgetTree, TEXT("PartyHPBar") + Suffix);
		Widgets.HPText = Find<UTextBlock>(WidgetTree, TEXT("PartyHPText") + Suffix);
		Widgets.APText = Find<UTextBlock>(WidgetTree, TEXT("PartyAPText") + Suffix);
		Widgets.APPlate = Find<UWidget>(WidgetTree, TEXT("PartyAPPlate") + Suffix);
		Widgets.StatusText = Find<UTextBlock>(WidgetTree, TEXT("PartyStatus") + Suffix);
		Widgets.StatusIcon = Find<UWidget>(WidgetTree, TEXT("PartyStatusIcon") + Suffix);
		// 홈과 그림은 굽는 쪽이 몇 개를 놓았는지 모른 채 센다. 개수를 여기
		// 적어 두면 구역을 늘렸을 때 조용히 못 찾는다.
		Widgets.StatusFrames.Reset();
		Widgets.StatusIcons.Reset();
		for (int32 SlotNo = 0; ; ++SlotNo)
		{
			UWidget* Frame = Find<UWidget>(WidgetTree,
				FString::Printf(TEXT("PartyStatusFrame_%d_%d"), Index, SlotNo));
			UImage* Icon = Find<UImage>(WidgetTree,
				FString::Printf(TEXT("PartyStatusIcon_%d_%d"), Index, SlotNo));
			if (Frame == nullptr && Icon == nullptr)
			{
				break;
			}
			Widgets.StatusFrames.Add(Frame);
			Widgets.StatusIcons.Add(Icon);
		}
		// 낱개는 굽는 쪽이 몇 개를 놓았는지 모른 채 센다. 개수를 여기 적어
		// 두면 굽는 쪽에서 늘렸을 때 조용히 못 찾는다.
		Widgets.APPips.Reset();
		Widgets.APPipsUsed.Reset();
		for (int32 Pip = 0; ; ++Pip)
		{
			UWidget* Found = Find<UWidget>(WidgetTree,
				FString::Printf(TEXT("PartyAPPip_%d_%d"), Index, Pip));
			if (Found == nullptr)
			{
				break;
			}
			Widgets.APPips.Add(Found);
			Widgets.APPipsUsed.Add(Find<UWidget>(WidgetTree,
				FString::Printf(TEXT("PartyAPPipUsed_%d_%d"), Index, Pip)));
		}
	}

	mCommandSlots.SetNum(CommandSlotCount);
	for (int32 Index = 0; Index < CommandSlotCount; ++Index)
	{
		FCommandSlotWidgets& Widgets = mCommandSlots[Index];
		const FString Suffix = FString::Printf(TEXT("_%d"), Index);
		Widgets.Root = Find<UWidget>(WidgetTree, TEXT("CommandCard") + Suffix);
		Widgets.Button = Find<UButton>(WidgetTree, TEXT("CommandButton") + Suffix);
		Widgets.Icon = Find<UImage>(WidgetTree, TEXT("CommandIcon") + Suffix);
		Widgets.Name = Find<UTextBlock>(WidgetTree, TEXT("CommandName") + Suffix);
		Widgets.Cost = Find<UTextBlock>(WidgetTree, TEXT("CommandCost") + Suffix);
		Widgets.CostLine = Find<UTextBlock>(WidgetTree, TEXT("CommandCostLine") + Suffix);
		Widgets.Cooldown = Find<UTextBlock>(WidgetTree, TEXT("CommandCooldown") + Suffix);
		Widgets.CooldownIcon = Find<UWidget>(WidgetTree, TEXT("CommandCooldownIcon") + Suffix);
		Widgets.Damage = Find<UTextBlock>(WidgetTree, TEXT("CommandDamage") + Suffix);
		Widgets.Disabled = Find<UWidget>(WidgetTree, TEXT("CommandDisabled") + Suffix);
	}

	mTurnSlots.SetNum(TurnSlotCount);
	for (int32 Index = 0; Index < TurnSlotCount; ++Index)
	{
		FTurnSlotWidgets& Widgets = mTurnSlots[Index];
		const FString Suffix = FString::Printf(TEXT("_%d"), Index);
		Widgets.Root = Find<UWidget>(WidgetTree, TEXT("TurnToken") + Suffix);
		Widgets.Portrait = Find<UImage>(WidgetTree, TEXT("TurnPortrait") + Suffix);
		Widgets.Name = Find<UTextBlock>(WidgetTree, TEXT("TurnName") + Suffix);
		Widgets.SpeedIcon = Find<UWidget>(WidgetTree, TEXT("TurnSpeedIcon") + Suffix);
		Widgets.Speed = Find<UTextBlock>(WidgetTree, TEXT("TurnSpeed") + Suffix);
		Widgets.Current = Find<UWidget>(WidgetTree, TEXT("TurnCurrent") + Suffix);
		Widgets.RoundDivider = Find<UWidget>(
			WidgetTree, TEXT("TurnRoundDivider") + Suffix);
		Widgets.RoundLabel = Find<UTextBlock>(
			WidgetTree, TEXT("TurnRoundLabel") + Suffix);
	}

	mEnemyPanel = Find<UWidget>(WidgetTree, TEXT("EnemyPanel"));
	mEnemyPortrait = Find<UImage>(WidgetTree, TEXT("EnemyPortrait"));
	mEnemyName = Find<UTextBlock>(WidgetTree, TEXT("EnemyName"));
	mEnemyHPBar = Find<UProgressBar>(WidgetTree, TEXT("EnemyHPBar"));
	mEnemyHPText = Find<UTextBlock>(WidgetTree, TEXT("EnemyHPText"));
	mEnemyDefenseText = Find<UTextBlock>(WidgetTree, TEXT("EnemyDefense"));
	mEnemyStatusText = Find<UTextBlock>(WidgetTree, TEXT("EnemyStatus"));
	mEnemyForecastText = Find<UTextBlock>(WidgetTree, TEXT("EnemyForecast"));

	mEndTurnButton = Find<UButton>(WidgetTree, TEXT("EndTurnButton"));

	mCommandLayer = Find<UScaleBox>(WidgetTree, TEXT("CommandLayerScale"));
	mPartyLayer = Find<UScaleBox>(WidgetTree, TEXT("PartyLayerScale"));
	const bool bFirstMercenaryPanelCache = mMercenaryPanel == nullptr;
	mMercenaryPanel = Find<UWidget>(WidgetTree, TEXT("MercenaryPanel"));
	EnsureMercenaryRosterShell();
	mMercenaryGoldText = Find<UTextBlock>(WidgetTree, TEXT("MercenaryGoldText"));
	mMercenaryCloseButton = Find<UButton>(WidgetTree, TEXT("MercenaryCloseButton"));
	mMercenaryHeroPortrait = Find<UImage>(WidgetTree, TEXT("MercenaryHeroPortrait"));
	mMercenaryDetailName = Find<UTextBlock>(WidgetTree, TEXT("MercenaryDetailName"));
	mMercenaryDetailHP = Find<UTextBlock>(WidgetTree, TEXT("MercenaryDetailHP"));
	mMercenaryDetailAP = Find<UTextBlock>(WidgetTree, TEXT("MercenaryDetailAP"));
	mMercenaryDetailSpeed = Find<UTextBlock>(WidgetTree, TEXT("MercenaryDetailSpeed"));
	// WBP 기본값이 잘못 저장되어도 전투 진입 때는 닫힌 상태로 시작한다.
	// 이후 도메인 갱신에서도 CacheAuthoredWidgets가 다시 불린다. 그때까지
	// 접으면, 골드가 갱신되는 순간 사용자가 열어 둔 탭이 닫혀 버린다.
	if (bFirstMercenaryPanelCache == true && mMercenaryPanel != nullptr)
	{
		mMercenaryPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	mConfirmPanel = Find<UWidget>(WidgetTree, TEXT("ConfirmPanel"));
	mConfirmButton = Find<UButton>(WidgetTree, TEXT("ConfirmButton"));
	mEndTurnLabel = Find<UTextBlock>(WidgetTree, TEXT("EndTurnLabel"));
	mTurnAPText = Find<UTextBlock>(WidgetTree, TEXT("TurnAPText"));
	mTurnAPPips.Reset();
	mTurnAPPipsUsed.Reset();
	for (int32 Pip = 0; ; ++Pip)
	{
		UWidget* Found = Find<UWidget>(WidgetTree,
			FString::Printf(TEXT("TurnAPPip_%d"), Pip));
		if (Found == nullptr)
		{
			break;
		}
		mTurnAPPips.Add(Found);
		mTurnAPPipsUsed.Add(Find<UWidget>(WidgetTree,
			FString::Printf(TEXT("TurnAPPipUsed_%d"), Pip)));
	}
	mArtifactIcons.SetNum(ArtifactSlotCount);
	mArtifactFrames.SetNum(ArtifactSlotCount);
	for (int32 Index = 0; Index < ArtifactSlotCount; ++Index)
	{
		mArtifactIcons[Index] = Find<UImage>(WidgetTree,
			FString::Printf(TEXT("ArtifactIcon_%d"), Index));
		mArtifactFrames[Index] = Find<UWidget>(WidgetTree,
			FString::Printf(TEXT("ArtifactFrame_%d"), Index));
		SetShown(mArtifactFrames[Index], false);
	}
	SetShown(Find<UWidget>(WidgetTree, TEXT("ArtifactStripPlate")), false);
	SetShown(Find<UWidget>(WidgetTree, TEXT("ArtifactStripLabel")), false);

	mTurnPageLeft = Find<UButton>(WidgetTree, TEXT("TurnPageLeft"));
	mTurnPageRight = Find<UButton>(WidgetTree, TEXT("TurnPageRight"));
	mTurnPageLeftText = Find<UTextBlock>(WidgetTree, TEXT("TurnPageLeftText"));
	mTurnPageRightText = Find<UTextBlock>(WidgetTree, TEXT("TurnPageRightText"));

	// 눌림을 삼킬 묶음들. 카드는 안 넣는다 -- 카드는 제 버튼이 가져간다.
	//
	// 묶음(Canvas) 을 잡는다. 그 안의 판과 글자는 SelfHitTestInvisible 이라
	// 눌림이 그대로 뿌리까지 내려오는데, 묶음의 자리를 재면 그 안 아무 데나
	// 눌러도 걸린다.
	mChromeWidgets.Reset();
	for (const TCHAR* Name : { TEXT("RoundPanel"), TEXT("TurnPanel"),
		TEXT("ObjectivePanel"), TEXT("EnemyPanel"), TEXT("EndTurnPanel"),
		TEXT("PartyCard_0"), TEXT("PartyCard_1"), TEXT("PartyCard_2"),
		TEXT("MercenaryPanel") })
	{
		if (UWidget* Found = Find<UWidget>(WidgetTree, Name))
		{
			mChromeWidgets.Add(Found);
		}
	}
}

void UCombatLayoutHUDWidget::EnsureMercenaryRosterShell()
{
	UCanvasPanel* Panel = Cast<UCanvasPanel>(mMercenaryPanel);
	if (Panel == nullptr || WidgetTree == nullptr)
	{
		return;
	}

	if (mMercenaryRosterShellImage == nullptr)
	{
		mMercenaryRosterShellImage = Find<UImage>(
			WidgetTree, TEXT("RuntimeMercenaryRosterShell"));
	}
	if (mMercenaryRosterShellImage == nullptr)
	{
		mMercenaryRosterShellImage = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), TEXT("RuntimeMercenaryRosterShell"));
		if (UCanvasPanelSlot* ShellSlot =
			Panel->AddChildToCanvas(mMercenaryRosterShellImage))
		{
			ShellSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			ShellSlot->SetOffsets(FMargin(0.f));
			ShellSlot->SetAlignment(FVector2D::ZeroVector);
			ShellSlot->SetZOrder(-100);
		}
	}

	// 그림 자체는 버튼 입력을 절대 받지 않는다. 제목·골드·닫기·용병 카드가
	// 같은 Canvas의 더 높은 z-order에서 그대로 작동한다.
	mMercenaryRosterShellImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UTexture2D* Texture = mMercenaryRosterShellTexture.LoadSynchronous())
	{
		mMercenaryRosterShellImage->SetBrushFromTexture(Texture, false);
	}

	// 새 셸에 이미 구워진 판들만 걷는다. 실제 정보/입력 위젯은 남겨 둔다.
	static const TCHAR* const LegacyPlateNames[] = {
		TEXT("MercenaryScrim"),
		TEXT("MercenaryHeaderPlate"),
		TEXT("MercenaryBoardPlate"),
		TEXT("MercenaryBoardShadow"),
		TEXT("MercenaryBoardInner"),
		TEXT("MercenaryClosePlate"),
	};
	for (const TCHAR* Name : LegacyPlateNames)
	{
		SetShown(Find<UWidget>(WidgetTree, Name), false);
	}
}

/**
 * @brief 붙은 뷰모델이 없으면 미리보기 장면을 세운다.
 *
 * @details
 * **여기서 게임모드를 찾지 않는다.** UI 는 게임 규칙도 게임모드도 직접
 * 참조하지 않는다(#426). 실제 전투 모델은 게임모드가 HUD 를 열면서 직접
 * 건네준다 -- ACombatGameMode::InitCombat 이 OpenUI() 옆에서 BindUIModel() 을
 * 부른다.
 *
 * 그래서 여기까지 왔다는 것은 붙여 줄 사람이 없다는 뜻이다. 편집기에서 WBP 를
 * 열었거나 찍는 시험이 도는 자리다. 그때만 가짜 장면을 세운다.
 *
 * 그 UIModel 과 드라이버는 위젯이 소유하고 화면과 수명을 같이 한다.
 */
void UCombatLayoutHUDWidget::StartPreviewIfUnbound()
{
	if (mUIModel != nullptr)
	{
		return;
	}

	if (!mUsePreviewData)
	{
		return;
	}
	UCombatUIModel* PreviewModel = NewObject<UCombatUIModel>(this);
	mPreviewDriver = NewObject<UMockCombatDriver>(this);
	BindUIModel(PreviewModel);
	// 편집기/캡처용 가짜 전투에는 실제 TurnContext가 없어 시작 알림이 오지
	// 않는다. 미리보기만 명시적으로 열린 턴으로 두어 작성한 HUD가 빈 화면이
	// 되지 않게 한다.
	mIsTurnActive = true;
	mPreviewDriver->Start(PreviewModel);
}

void UCombatLayoutHUDWidget::WireCommands()
{
	// 슬롯별 핸들러를 따로 두는 이유: UFUNCTION 델리게이트는 페이로드를 못 받는다.
	//
	// 동적 델리게이트는 함수 포인터가 아니라 **이름**으로 찾아서 묶는다. 그래서
	// 포인터만 슬롯별로 바꾸고 이름을 하나로 넘기면 전부 조용히 실패한다.
	// 이름도 같이 짝지어 둔다.
	struct FCommandHandler
	{
		void (UCombatLayoutHUDWidget::*Function)();
		const TCHAR* Name;
	};
	static const FCommandHandler Handlers[CommandSlotCount] = {
		{ &UCombatLayoutHUDWidget::HandleCommandClicked_0, TEXT("HandleCommandClicked_0") },
		{ &UCombatLayoutHUDWidget::HandleCommandClicked_1, TEXT("HandleCommandClicked_1") },
		{ &UCombatLayoutHUDWidget::HandleCommandClicked_2, TEXT("HandleCommandClicked_2") },
		{ &UCombatLayoutHUDWidget::HandleCommandClicked_3, TEXT("HandleCommandClicked_3") },
		{ &UCombatLayoutHUDWidget::HandleCommandClicked_4, TEXT("HandleCommandClicked_4") },
		{ &UCombatLayoutHUDWidget::HandleCommandClicked_5, TEXT("HandleCommandClicked_5") },
	};
	for (int32 Index = 0; Index < mCommandSlots.Num(); ++Index)
	{
		if (UButton* Button = mCommandSlots[Index].Button)
		{
			Button->OnClicked.__Internal_AddUniqueDynamic(
				this, Handlers[Index].Function, Handlers[Index].Name);
			BindPressFeedback(Button, mCommandSlots[Index].Root);
		}
	}
	struct FPartyHandler
	{
		void (UCombatLayoutHUDWidget::*Function)();
		const TCHAR* Name;
	};
	static const FPartyHandler PartyHandlers[PartySlotCount] = {
		{ &UCombatLayoutHUDWidget::HandlePartyClicked_0, TEXT("HandlePartyClicked_0") },
		{ &UCombatLayoutHUDWidget::HandlePartyClicked_1, TEXT("HandlePartyClicked_1") },
		{ &UCombatLayoutHUDWidget::HandlePartyClicked_2, TEXT("HandlePartyClicked_2") },
	};
	for (int32 Index = 0; Index < mPartySlots.Num(); ++Index)
	{
		if (UButton* Button = Find<UButton>(WidgetTree,
			FString::Printf(TEXT("PartyButton_%d"), Index)))
		{
			Button->OnClicked.__Internal_AddUniqueDynamic(
				this, PartyHandlers[Index].Function, PartyHandlers[Index].Name);
			BindPressFeedback(Button, mPartySlots[Index].Root);
		}
	}

	if (mEndTurnButton != nullptr)
	{
		mEndTurnButton->OnClicked.AddUniqueDynamic(
			this, &UCombatLayoutHUDWidget::HandleEndTurnClicked);
		BindPressFeedback(mEndTurnButton, mEndTurnButton);
	}

	if (mConfirmButton != nullptr)
	{
		mConfirmButton->OnClicked.AddUniqueDynamic(
			this, &UCombatLayoutHUDWidget::HandleConfirmClicked);
		BindPressFeedback(mConfirmButton, mConfirmPanel);
	}

	if (mTurnPageLeft != nullptr)
	{
		mTurnPageLeft->OnClicked.AddUniqueDynamic(
			this, &UCombatLayoutHUDWidget::HandleTurnPageLeftClicked);
		BindPressFeedback(mTurnPageLeft, mTurnPageLeft);
	}
	if (mTurnPageRight != nullptr)
	{
		mTurnPageRight->OnClicked.AddUniqueDynamic(
			this, &UCombatLayoutHUDWidget::HandleTurnPageRightClicked);
		BindPressFeedback(mTurnPageRight, mTurnPageRight);
	}

	// 상단 메뉴 넷. 1번은 보유 용병, 2번은 생존 몬스터 정보를 연다.
	mMenuButtons.Reset();
	for (int32 Index = 0; ; ++Index)
	{
		UButton* Button = Find<UButton>(WidgetTree,
			FString::Printf(TEXT("MenuButton_%d"), Index));
		if (Button == nullptr)
		{
			break;
		}
		BindPressFeedback(Button, Button);
		mMenuButtons.Add(Button);
	}
	if (mMenuButtons.IsValidIndex(1))
	{
		mMenuButtons[1]->OnClicked.AddUniqueDynamic(
			this, &UCombatLayoutHUDWidget::HandleMercenaryMenuClicked);
	}
	if (mMenuButtons.IsValidIndex(0))
	{
		mMenuButtons[0]->OnClicked.AddUniqueDynamic(
			this, &UCombatLayoutHUDWidget::HandleWorldMapMenuClicked);
	}
	if (mMenuButtons.IsValidIndex(2))
	{
		mMenuButtons[2]->SetIsEnabled(true);
		mMenuButtons[2]->OnClicked.AddUniqueDynamic(
			this, &UCombatLayoutHUDWidget::HandleMonsterMenuClicked);
	}
	if (mMercenaryCloseButton != nullptr)
	{
		mMercenaryCloseButton->OnClicked.AddUniqueDynamic(
			this, &UCombatLayoutHUDWidget::HandleMercenaryCloseClicked);
		BindPressFeedback(mMercenaryCloseButton, mMercenaryCloseButton);
	}
}

/**
 * @brief 누르는 동안 살짝 줄어들게 한다.
 * @param Button 누를 자리
 * @param Target 줄일 묶음. 버튼 자신이어도 된다
 */
void UCombatLayoutHUDWidget::BindPressFeedback(UButton* Button, UWidget* Target)
{
	if (Button == nullptr || Target == nullptr)
	{
		return;
	}
	// 가운데를 기준으로 줄어야 제자리에서 눌린 것처럼 보인다. 기본 기준점은
	// 왼쪽 위라, 그대로 두면 오른쪽 아래로 밀리면서 작아진다.
	Target->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	mPressTargets.Add(Button, Target);
	Button->OnPressed.AddUniqueDynamic(this, &UCombatLayoutHUDWidget::HandleAnyPressed);
	Button->OnReleased.AddUniqueDynamic(this, &UCombatLayoutHUDWidget::HandleAnyReleased);
}

/**
 * @brief 눌렸다. 눌린 버튼을 찾아 그 묶음을 줄인다.
 *
 * @details
 * 어느 버튼이 눌렸는지는 알림이 안 알려 준다. 눌린 상태인 것을 찾는다 --
 * 한 번에 하나만 눌리므로 이걸로 충분하다. 버튼마다 핸들러를 따로 만들면
 * 카드 여섯 · 아군 셋 · 메뉴 넷에 턴 종료까지 열넷이 된다.
 */
void UCombatLayoutHUDWidget::HandleAnyPressed()
{
	// 새 누름이다. 지난 긴 누름이 남겨 둔 삼킴 표시가 있으면 지운다 -- 발화
	// 뒤 버튼 밖에서 놓으면 클릭이 안 와서 표시가 그대로 남는다.
	mSwallowNextCommandClick = false;

	// 카드는 오래 누르면 상세다. 눌린 카드는 **줄임 피드백 대상과 따로** 찾는다 --
	// 카드 묶음(Root)이 WBP 에 없으면 그 map 에 아예 안 들어가는데, 그렇다고
	// 설명을 못 읽을 이유는 없다.
	for (int32 SlotIndex = 0; SlotIndex < mCommandSlots.Num(); ++SlotIndex)
	{
		UButton* CommandButton = mCommandSlots[SlotIndex].Button;
		if (CommandButton == nullptr || CommandButton->IsPressed() == false)
		{
			continue;
		}
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(mCommandLongPressTimerHandle,
				FTimerDelegate::CreateWeakLambda(this, [this, SlotIndex]()
				{
					HandleCommandLongPress(SlotIndex);
				}),
				LongPressSeconds, false);
		}
		break;
	}

	for (const TPair<TObjectPtr<UButton>, TObjectPtr<UWidget>>& Pair : mPressTargets)
	{
		if (Pair.Key == nullptr || Pair.Key->IsPressed() == false)
		{
			continue;
		}
		mPressedTarget = Pair.Value;
		FWidgetTransform Transform;
		Transform.Scale = FVector2D(PressedScale, PressedScale);
		Pair.Value->SetRenderTransform(Transform);
		return;
	}
}

/**
 * @brief 카드를 오래 눌렀다. 그 카드의 상세를 띄운다.
 *
 * 스킬 상세는 SetSkillDetail 로 되돌아와 패널이 뜬다. 이어질 클릭(뗌)은 고르는
 * 동작이 아니라 놓는 동작이므로 한 번 삼킨다.
 */
void UCombatLayoutHUDWidget::HandleCommandLongPress(const int32 SlotIndex)
{
	if (mUIModel == nullptr)
	{
		return;
	}
	mSwallowNextCommandClick = true;
	UE_LOG(LogRD, Log, TEXT("[탭진단] 카드 %d 긴 누름 발화 — 상세 요청, 다음 클릭 삼킴 예약"), SlotIndex);

	// 0번은 이동이다. 스킬이 아니라 게임플레이에 청할 것이 없으므로 화면이
	// 이미 받아 둔 값으로 바로 조립한다.
	if (SlotIndex == 0)
	{
		ShowMoveDetailOverlay();
		return;
	}
	mUIModel->RequestLongPressSkill(SlotIndex - 1);
}

/** @brief 왼쪽 넘김칸을 눌러 직전 열 칸 페이지로 간다. */
void UCombatLayoutHUDWidget::HandleTurnPageLeftClicked()
{
	const int32 SlotRoom = mTurnSlots.Num();
	if (SlotRoom <= 0)
	{
		return;
	}

	// 마지막 페이지가 열 칸 경계와 맞지 않아도 14 -> 10 -> 0처럼
	// 직전 정규 페이지를 거친다. 그래야 중간 순서를 건너뛰지 않는다.
	mTurnWindowStart = FMath::Max(
		((mTurnWindowStart - 1) / SlotRoom) * SlotRoom, 0);
	RefreshTurnOrder();
}

/** @brief 오른쪽 넘김칸을 눌러 다음 열 칸 페이지로 간다. */
void UCombatLayoutHUDWidget::HandleTurnPageRightClicked()
{
	if (mUIModel == nullptr)
	{
		return;
	}
	const FTurnUI& Turn = mUIModel->GetTurnUI();
	const int32 SlotRoom = mTurnSlots.Num();
	const int32 LastStart = FMath::Max(
		ProjectedTurnCount(Turn, SlotRoom) - SlotRoom, 0);
	mTurnWindowStart = FMath::Min(
		mTurnWindowStart + SlotRoom, LastStart);
	RefreshTurnOrder();
}

/** @brief 놓았다. 줄여 둔 것을 되돌린다. */
void UCombatLayoutHUDWidget::HandleAnyReleased()
{
	// 놓았으니 카드 긴 누름 판정은 끝났다.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(mCommandLongPressTimerHandle);
	}

	if (mPressedTarget != nullptr)
	{
		mPressedTarget->SetRenderTransform(FWidgetTransform());
		mPressedTarget = nullptr;
	}
}

void UCombatLayoutHUDWidget::NativeOnUIRefreshed(const ECombatUIDomain Domain)
{
	if (mUIModel == nullptr)
	{
		return;
	}
	CacheAuthoredWidgets();

	const bool bAll = Domain == ECombatUIDomain::All;
	if (bAll || Domain == ECombatUIDomain::Unit)
	{
		RefreshParty();
		RefreshEnemy();
		RefreshTurnActionPoints();
		RebuildUnitHpBars();   // 유닛 수가 바뀌면 머리 위 바도 다시 만든다.
	}
	if (bAll || Domain == ECombatUIDomain::Turn)
	{
		// 차례가 **바뀌었을 때만** 편다. 갱신이 올 때마다 펴면 접자마자 다시
		// 펴져서 접기가 안 먹는 것처럼 보인다 -- 실제로 그랬다. Turn 갱신은
		// 차례가 그대로여도 여러 번 온다.
		const int32 TurnUnitId = mUIModel != nullptr
			? mUIModel->GetTurnUI().mCurrentUnitId : INDEX_NONE;
		const int32 TurnRound = mUIModel->GetTurnUI().mRound;
		if (TurnUnitId != mLastTurnUnitId || TurnRound != mLastTurnBarRound)
		{
			mLastTurnUnitId = TurnUnitId;
			mLastTurnBarRound = TurnRound;
			SetCommandsShown(true);
			// 줄 자체가 한 칸 밀렸다. 옮겨 둔 창을 그대로 두면 다음 턴에
			// 엉뚱한 곳을 보고 있다.
			mTurnWindowStart = 0;
			// 차례가 바뀌면 지난 차례에 보던 상세는 낡았다. 위협 범위 칠은
			// 턴 종료 명령에서 게임플레이가 이미 걷었다.
			HideDetailOverlay(/*bNotifyGameplay=*/false);
		}
		// 페이지 원점 변경을 반영한 뒤 그린다. 먼저 그리면 턴이 바뀐 한
		// 프레임 동안 이전 마지막 페이지가 남는다.
		RefreshTurnOrder();
		RefreshParty();

		// 조준에 들었거나 조준이 끝났을 수 있다. 카드가 비켜 있을지 여기서
		// 다시 정한다 -- 조준 단계는 Turn 으로 실려 온다.
		RefreshCommandVisibility();
		RefreshActionButtons();
		RefreshTurnActionPoints();
		RefreshScreenScale();
	}
	if (bAll || Domain == ECombatUIDomain::Unit)
	{
		// 겨냥한 자리가 바뀌어도 카드를 억지로 펴지 않는다. 펴고 접는 것은
		// 판을 누른 그 손이 정한다 -- 여기서 같이 펴면, 닫으려고 누른 탭이
		// 자리도 바꾸는 바람에 닫자마자 도로 열린다.
		const int32 TargetId = (mUIModel != nullptr
			&& mUIModel->GetTarget().mIsValid)
			? mUIModel->GetTarget().mUnitId : INDEX_NONE;
		mLastTargetUnitId = TargetId;
		// 몬스터 탭이 떠 있으면 HP·상태 변화를 따라간다. 죽은 몬스터는 행에서
		// 빠지고 선택이 남은 행으로 옮겨 간다.
		if (IsMonsterTabShown() == true)
		{
			RefreshMonsterTab();
		}
	}
	if (bAll || Domain == ECombatUIDomain::Skill)
	{
		RefreshCommands();
		// RefreshCommands 는 내용을 채우며 칸을 켠다. 접어 둔 카드가 스킬
		// 갱신(겨냥 변경마다 온다)에 도로 펴지지 않게 표시 정책으로 되돌린다.
		RefreshCommandVisibility();
	}
	if (bAll || Domain == ECombatUIDomain::Meta)
	{
		RefreshMeta();
	}
	// 상세는 All 에 실려 오지 않는다. 롱프레스 요청의 응답으로만 열어야,
	// 바인딩 직후 All 갱신이 빈 상세 패널을 띄우지 않는다.
	if (Domain == ECombatUIDomain::UnitDetail)
	{
		// 몬스터 탭이 열려 있는 동안의 상세 응답은 탭의 스킬 칸이 받는다.
		// PR457 상세 겹을 같이 띄우면 전체 화면 탭 위에 또 한 장이 얹힌다.
		if (IsMonsterTabShown() == true)
		{
			RefreshMonsterTabDetail();
		}
		else
		{
			ShowUnitDetailOverlay();
		}
	}
	if (Domain == ECombatUIDomain::SkillDetail)
	{
		ShowSkillDetailOverlay();
	}
}

void UCombatLayoutHUDWidget::RefreshParty()
{
	const TArray<FUnitUI>& Units = mUIModel->GetUnitUIs();
	const int32 CurrentUnitId = mUIModel->GetTurnUI().mCurrentUnitId;
	UTexture2D* NormalCard = mMercenaryCardNormalTexture.LoadSynchronous();
	UTexture2D* SelectedCard = mMercenaryCardSelectedTexture.LoadSynchronous();
	const FUnitUI* FocusUnit = nullptr;
	for (const FUnitUI& Unit : Units)
	{
		if (Unit.mIsPlayer == false)
		{
			continue;
		}
		if (FocusUnit == nullptr || Unit.mUnitId == CurrentUnitId)
		{
			FocusUnit = &Unit;
		}
		if (Unit.mUnitId == CurrentUnitId)
		{
			break;
		}
	}

	int32 SlotIndex = 0;
	for (const FUnitUI& Unit : Units)
	{
		if (!Unit.mIsPlayer || !mPartySlots.IsValidIndex(SlotIndex))
		{
			continue;
		}
		const FPartySlotWidgets& Widgets = mPartySlots[SlotIndex];
		SetShown(Widgets.Root, true);
		SetShown(Widgets.Content, true);
		SetShown(Widgets.Plate, true);
		if (Widgets.Plate != nullptr)
		{
			UTexture2D* CardTexture = Unit.mUnitId == CurrentUnitId
				? SelectedCard : NormalCard;
			if (CardTexture != nullptr)
			{
				Widgets.Plate->SetBrushFromTexture(CardTexture, false);
			}
		}

		// 빈 칸 처리가 접어 둔 것들을 다시 켠다.
		//
		// 위젯이 붙는 순간의 UIModel은 비어 있다. 그 첫 갱신에서 세 칸 모두
		// 빈 칸으로 그려지며 HP 바와 초상화가 접히고, 곧이어 데이터가 들어와도
		// 여기서 켜 주지 않으면 영영 접힌 채로 남는다. 실제로 그렇게 됐다 --
		// 이름과 숫자는 나오는데 바와 얼굴만 사라진 화면이었다.
		SetShown(Widgets.HPBar, true);
		SetShown(Widgets.Portrait, true);

		SetTextIfPresent(Widgets.Name, Unit.mName);

		// 유닛 초상은 배치안이 깔아 둔 얼굴을 덮는다. 데이터에셋 초상이
		// 배경이 안 뚫린 사각형이라 금속 테를 가리는데, 그건 자산 문제라
		// 여기서 안 피한다 -- 피하면 게임에서 영영 안 보인다.
		if (Widgets.Portrait != nullptr && Unit.mPortrait != nullptr)
		{
			SetPortraitCropped(Widgets.Portrait, Unit.mPortrait);
		}
		if (Widgets.HPBar != nullptr)
		{
			Widgets.HPBar->SetPercent(Unit.mMaxHP > 0.f ? Unit.mHP / Unit.mMaxHP : 0.f);
		}
		SetTextIfPresent(Widgets.HPText, FText::FromString(FString::Printf(
			TEXT("%d/%d"), FMath::RoundToInt(Unit.mHP), FMath::RoundToInt(Unit.mMaxHP))));
		RefreshPartyActionPoints(Widgets, Unit);
		RefreshPartyStatus(Widgets, Unit);

		if (Widgets.StatusText != nullptr)
		{
			const bool bHasStatus = Unit.mStatusEffects.Num() > 0;
			SetShown(Widgets.StatusText, bHasStatus);
			SetShown(Widgets.StatusIcon, bHasStatus);
			if (bHasStatus)
			{
				const FStatusEffectUI& First = Unit.mStatusEffects[0];
				Widgets.StatusText->SetText(FText::FromString(FString::Printf(
					TEXT("%s %d턴"),
					*StatusDisplayName(First.mTag), First.mStackCount)));
			}
		}
		++SlotIndex;
	}

	// 파티가 3명보다 적어도 칸은 세 개 다 세워 둔다.
	//
	// 접어 버리면 아군이 한 명일 때 화면 왼쪽이 통째로 비어서, 자리가 원래
	// 그런 건지 뭔가 안 뜬 건지 구분이 안 된다. 빈 칸을 남겨 두면 "여기 한
	// 명 더 들어온다"가 읽히고, 유닛이 죽거나 합류해도 배치가 안 흔들린다.
	for (; SlotIndex < mPartySlots.Num(); ++SlotIndex)
	{
		ClearPartySlot(mPartySlots[SlotIndex]);
	}

	const bool bHasFocus = FocusUnit != nullptr;
	SetShown(mMercenaryHeroPortrait, bHasFocus);
	SetShown(mMercenaryDetailName, bHasFocus);
	SetShown(mMercenaryDetailHP, bHasFocus);
	SetShown(mMercenaryDetailAP, bHasFocus);
	SetShown(mMercenaryDetailSpeed, bHasFocus);
	if (bHasFocus)
	{
		if (mMercenaryHeroPortrait != nullptr && FocusUnit->mPortrait != nullptr)
		{
			SetPortraitCropped(mMercenaryHeroPortrait, FocusUnit->mPortrait);
		}
		SetTextIfPresent(mMercenaryDetailName, FocusUnit->mName);
		SetTextIfPresent(mMercenaryDetailHP, FText::FromString(FString::Printf(
			TEXT("HP  %d / %d"), FMath::RoundToInt(FocusUnit->mHP),
			FMath::RoundToInt(FocusUnit->mMaxHP))));
		SetTextIfPresent(mMercenaryDetailAP, FText::FromString(FString::Printf(
			TEXT("AP  %d / %d"), FocusUnit->mActionPoints,
			FocusUnit->mMaxActionPoints)));
		SetTextIfPresent(mMercenaryDetailSpeed, FText::FromString(FString::Printf(
			TEXT("속도  %d"), FMath::RoundToInt(FocusUnit->mSpeedPoint))));
	}
}

/**
 * @brief 빈 아군 칸을 "비어 있음"으로 그린다.
 *
 * @details
 * 칸 자체와 테두리는 남기고 안쪽 내용만 지운다. 카드가 통째로 사라지는 것과
 * 빈 카드가 놓여 있는 것은 읽히는 뜻이 다르다.
 */
void UCombatLayoutHUDWidget::ClearPartySlot(const FPartySlotWidgets& Widgets)
{
	SetShown(Widgets.Root, true);
	SetShown(Widgets.Content, false);
	SetShown(Widgets.Plate, true);
	if (Widgets.Plate != nullptr)
	{
		if (UTexture2D* NormalCard = mMercenaryCardNormalTexture.LoadSynchronous())
		{
			Widgets.Plate->SetBrushFromTexture(NormalCard, false);
		}
	}

	SetTextIfPresent(Widgets.Name, FText::GetEmpty());
	SetTextIfPresent(Widgets.HPText, FText::GetEmpty());
	SetTextIfPresent(Widgets.APText, FText::GetEmpty());
	SetShown(Widgets.APText, false);
	SetShown(Widgets.APPlate, false);
	SetShown(Widgets.StatusText, false);
	SetShown(Widgets.Portrait, false);

	if (Widgets.HPBar != nullptr)
	{
		Widgets.HPBar->SetPercent(0.f);
		SetShown(Widgets.HPBar, false);
	}
	for (UWidget* Pip : Widgets.APPips)
	{
		SetShown(Pip, false);
	}
	for (UWidget* Pip : Widgets.APPipsUsed)
	{
		SetShown(Pip, false);
	}
	for (int32 SlotNo = 0; SlotNo < Widgets.StatusFrames.Num(); ++SlotNo)
	{
		SetShown(Widgets.StatusFrames[SlotNo], false);
		if (Widgets.StatusIcons.IsValidIndex(SlotNo))
		{
			SetShown(Widgets.StatusIcons[SlotNo], false);
		}
	}
}

/**
 * @brief 아군 칸 하나에 AP 를 그린다.
 *
 * @details
 * 이 프로젝트에서 행동력은 Movement 다. 이동도 스킬도 같은 통에서 꺼내 쓰고
 * (`mRequiredMovement`), 적 계획도 그 값으로 돈다. `mActionPoints` 는 계약에
 * 칸만 있고 아무도 안 채운다.
 *
 * 왼쪽 숫자판에 `남은/전체`, 그 옆에 낱개를 편다. 낱개는 열까지만 -- 그
 * 위로는 자리가 없다. 넘으면 낱개를 다 끄고 숫자만 남긴다. 숫자판은 늘 켜
 * 두므로 낱개가 없어도 몇인지 읽힌다.
 *
 * 쓴 칸은 흐린 그림이 같은 자리에 겹쳐 있다. 둘 중 하나만 켠다.
 */
/**
 * @brief 상태이상 태그에 맞는 그림.
 *
 * @details
 * 옛 HUD 가 로그에 쓰던 그림을 그대로 쓴다. 같은 상태에 다른 그림을 두면
 * 화면 두 곳이 같은 것을 다르게 말하게 된다.
 *
 * 전용 그림이 없는 태그는 nullptr 이다 -- 그때는 홈만 서고 그림은 안 뜬다.
 * @param StatusTag 걸린 상태
 * @return 그림, 없으면 nullptr
 */
UTexture2D* UCombatLayoutHUDWidget::StatusIconFor(const FGameplayTag& StatusTag)
{
	struct FStatusArt
	{
		FGameplayTag Tag;
		TObjectPtr<UTexture2D> Texture;
	};

	// 한 번만 읽는다. 카드 셋 x 홈 셋이 매 갱신마다 부르는 자리라 매번 읽으면
	// 같은 그림을 아홉 번 찾는다.
	static TArray<FStatusArt> Loaded;
	if (Loaded.IsEmpty())
	{
		const TCHAR* Root = TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/StatusIcons/");
		const TPair<FGameplayTag, const TCHAR*> Pairs[] = {
			{ EffectTags::GameplayEffect_StatusEffect_TurnDuration_Buff_Agility, TEXT("T_Status_Agility") },
			{ EffectTags::GameplayEffect_StatusEffect_TurnDuration_Buff_Fortification, TEXT("T_Status_Fortification") },
			{ EffectTags::GameplayEffect_StatusEffect_TurnDuration_Debuff_Vulnerability, TEXT("T_Status_Vulnerability") },
			{ EffectTags::GameplayEffect_StatusEffect_TurnDuration_Debuff_Weakness, TEXT("T_Status_Weakness") },
		};
		for (const TPair<FGameplayTag, const TCHAR*>& Pair : Pairs)
		{
			const FString Path = FString::Printf(TEXT("%s%s.%s"), Root, Pair.Value, Pair.Value);
			Loaded.Add({ Pair.Key, LoadObject<UTexture2D>(nullptr, *Path) });
		}
	}

	for (const FStatusArt& Art : Loaded)
	{
		if (StatusTag.MatchesTag(Art.Tag))
		{
			return Art.Texture;
		}
	}
	return nullptr;
}

/**
 * @brief 아군 칸 하나에 상태이상을 그린다.
 * @param Widgets 그 칸의 위젯 묶음
 * @param Unit 그 칸에 선 유닛
 */
void UCombatLayoutHUDWidget::RefreshPartyStatus(
	const FPartySlotWidgets& Widgets, const FUnitUI& Unit) const
{
	const TArray<FStatusEffectUI>& Effects = Unit.mStatusEffects;
	for (int32 SlotNo = 0; SlotNo < Widgets.StatusFrames.Num(); ++SlotNo)
	{
		const bool bFilled = Effects.IsValidIndex(SlotNo);

		// 안 걸렸으면 홈까지 감춘다.
		//
		// 흐리게 세워 두었더니 빈 액자 셋이 늘 카드 위에 떠 있어, 걸린 것이
		// 있는지 없는지가 오히려 안 읽혔다. 없을 때 아무것도 없는 편이
		// "지금 걸린 게 있다" 를 또렷하게 만든다.
		SetShown(Widgets.StatusFrames[SlotNo], bFilled);

		UImage* Icon = Widgets.StatusIcons.IsValidIndex(SlotNo)
			? Widgets.StatusIcons[SlotNo].Get() : nullptr;
		if (Icon == nullptr)
		{
			continue;
		}
		UTexture2D* Art = bFilled ? StatusIconFor(Effects[SlotNo].mTag) : nullptr;
		SetShown(Icon, Art != nullptr);
		if (Art != nullptr)
		{
			Icon->SetBrushFromTexture(Art, false);
		}
	}
}

void UCombatLayoutHUDWidget::RefreshPartyActionPoints(
	const FPartySlotWidgets& Widgets, const FUnitUI& Unit) const
{
	const int32 Left = mUIModel != nullptr
		? mUIModel->ResolveDisplayedMovementPoint(Unit)
		: FMath::Max(FMath::RoundToInt(Unit.mMovementPoint), 0);
	const int32 Total = FMath::Max(
		FMath::RoundToInt(Unit.mMaxMovementPoint), Left);

	SetShown(Widgets.APPlate, true);
	SetShown(Widgets.APText, true);
	SetTextIfPresent(Widgets.APText, FText::FromString(
		FString::Printf(TEXT("%d/%d"), Left, Total)));

	const int32 PipRoom = Widgets.APPips.Num();
	const bool bTooMany = Total > PipRoom;
	for (int32 Pip = 0; Pip < PipRoom; ++Pip)
	{
		const bool bHasPip = !bTooMany && Pip < Total;
		SetShown(Widgets.APPips[Pip], bHasPip && Pip < Left);
		if (Widgets.APPipsUsed.IsValidIndex(Pip))
		{
			SetShown(Widgets.APPipsUsed[Pip], bHasPip && Pip >= Left);
		}
	}
}

/**
 * @brief 현재 라운드 잔여분 뒤에 다음 라운드 순서를 이어 턴바에 그린다.
 *
 * @details
 * 순서 배열은 현재 유닛부터 한 바퀴다. 이번 라운드에 남은 앞부분 뒤로 배열을
 * 순환 반복하면 다음 라운드 전체가 자연스럽게 이어진다. 미래 라운드는
 * 반투명으로, 경계에는 세로 막대와 R#을 표시한다.
 */
void UCombatLayoutHUDWidget::RefreshTurnOrder()
{
	const FTurnUI& Turn = mUIModel->GetTurnUI();
	const TArray<FUnitUI>& Units = mUIModel->GetUnitUIs();

	SetTextIfPresent(mRoundText, FText::FromString(
		FString::Printf(TEXT("R%d"), Turn.mRound)));

	const int32 SlotRoom = mTurnSlots.Num();
	const int32 CycleCount = Turn.mTurnOrderUnitIds.Num();
	const int32 RemainingThisRound = CycleCount > 0
		? FMath::Clamp(
			Turn.mCurrentRoundRemainingTurnCount > 0
				? Turn.mCurrentRoundRemainingTurnCount
				: CycleCount,
			1, CycleCount)
		: 0;
	const int32 Total = ProjectedTurnCount(Turn, SlotRoom);
	const int32 Start = FMath::Clamp(mTurnWindowStart, 0,
		FMath::Max(Total - SlotRoom, 0));
	mTurnWindowStart = Start;

	const int32 HiddenLeft = Start;
	const int32 HiddenRight = FMath::Max(Total - (Start + SlotRoom), 0);
	// 글자는 버튼의 자식이 아니라 옆에 있는 형제다. 버튼만 접으면 수가
	// 허공에 남는다.
	SetInteractiveShown(mTurnPageLeft, HiddenLeft > 0);
	SetShown(mTurnPageLeftText, HiddenLeft > 0);
	SetInteractiveShown(mTurnPageRight, HiddenRight > 0);
	SetShown(mTurnPageRightText, HiddenRight > 0);
	SetTextIfPresent(mTurnPageLeftText, FText::AsNumber(HiddenLeft));
	SetTextIfPresent(mTurnPageRightText, FText::AsNumber(HiddenRight));

	for (int32 Index = 0; Index < SlotRoom; ++Index)
	{
		const FTurnSlotWidgets& Widgets = mTurnSlots[Index];
		const int32 ProjectedIndex = Start + Index;
		if (CycleCount <= 0 || ProjectedIndex >= Total)
		{
			SetShown(Widgets.Root, false);
			SetShown(Widgets.RoundDivider, false);
			SetShown(Widgets.RoundLabel, false);
			SetShown(Widgets.SpeedIcon, false);
			SetShown(Widgets.Speed, false);
			continue;
		}
		const int32 UnitId =
			Turn.mTurnOrderUnitIds[ProjectedIndex % CycleCount];
		const FUnitUI* Unit = Units.FindByPredicate(
			[UnitId](const FUnitUI& Candidate) { return Candidate.mUnitId == UnitId; });
		if (Unit == nullptr)
		{
			SetShown(Widgets.Root, false);
			SetShown(Widgets.RoundDivider, false);
			SetShown(Widgets.RoundLabel, false);
			SetShown(Widgets.SpeedIcon, false);
			SetShown(Widgets.Speed, false);
			continue;
		}

		const int32 RoundOffset = ProjectedIndex < RemainingThisRound
			? 0
			: 1 + (ProjectedIndex - RemainingThisRound) / CycleCount;
		const bool bStartsShownRound = ProjectedIndex == 0
			|| (ProjectedIndex >= RemainingThisRound
				&& (ProjectedIndex - RemainingThisRound) % CycleCount == 0);

		SetShown(Widgets.Root, true);
		SetShown(Widgets.SpeedIcon, true);
		SetShown(Widgets.Speed, true);
		SetTextIfPresent(Widgets.Speed,
			FText::AsNumber(FMath::RoundToInt(Unit->mSpeedPoint)));
		Widgets.Root->SetRenderOpacity(RoundOffset == 0 ? 1.f : 0.45f);
		SetShown(Widgets.Current, ProjectedIndex == 0);
		SetTextIfPresent(Widgets.Name, Unit->mName);
		SetShown(Widgets.RoundDivider, bStartsShownRound);
		SetShown(Widgets.RoundLabel, bStartsShownRound);
		if (bStartsShownRound)
		{
			SetTextIfPresent(Widgets.RoundLabel,
				FText::FromString(FString::Printf(TEXT("R%d"),
					Turn.mRound + RoundOffset)));
		}
		// 턴바는 DA mIcon의 정사각 HeadV2를 쓴다. 아이콘이 없는 구형
		// 스냅샷은 세로 초상으로 폴백해 텍스트만 남는 회귀를 막는다.
		UTexture2D* TurnPortrait = Unit->mTurnPortrait != nullptr
			? Unit->mTurnPortrait.Get() : Unit->mPortrait.Get();
		SetShown(Widgets.Portrait, TurnPortrait != nullptr);
		if (Widgets.Portrait != nullptr && TurnPortrait != nullptr)
		{
			SetPortraitCropped(Widgets.Portrait, TurnPortrait);
		}
	}
}

/**
 * @brief 커맨드 레일을 그린다. 0번은 이동, 1번부터가 스킬이다.
 *
 * @details
 * 이동을 스킬 목록 밖에 두는 이유는 게임플레이가 그렇게 나누기 때문이다 --
 * RequestMove()와 RequestSelectSkill(index)이 서로 다른 명령이다.
 */
void UCombatLayoutHUDWidget::RefreshCommands()
{
	const TArray<FSkillUI>& Skills = mUIModel->GetSkillUIs();

	for (int32 SlotIndex = 0; SlotIndex < mCommandSlots.Num(); ++SlotIndex)
	{
		const FCommandSlotWidgets& Widgets = mCommandSlots[SlotIndex];

		if (SlotIndex == 0)
		{
			SetShown(Widgets.Root, true);
			SetTextIfPresent(Widgets.Name, LOCTEXT("Move", "이동"));
			SetTextIfPresent(Widgets.Cost, LOCTEXT("MoveCostPerTileShort", "1/칸"));
			SetTextIfPresent(Widgets.CostLine, LOCTEXT("MoveCost", "AP 1/칸"));
			SetShown(Widgets.Cooldown, false);
			SetShown(Widgets.CooldownIcon, false);
			SetShown(Widgets.Damage, false);
			// 행동력이 바닥나면 이동도 잠근다. 갈 수 있는 칸이 없다 -- 스킬은
			// 잠그면서 이동만 열어 두면 "왜 이것만 되지"가 된다. 긴 누름으로
			// 설명을 읽는 것은 스킬과 마찬가지로 잠겨도 된다.
			{
				const FUnitUI* TurnUnit = FindTurnUnit();
				const bool bCanMove = TurnUnit != nullptr
					&& FMath::RoundToInt(TurnUnit->mMovementPoint) > 0;
				SetShown(Widgets.Disabled, !bCanMove);
			}
			continue;
		}

		const int32 SkillIndex = SlotIndex - 1;
		if (!Skills.IsValidIndex(SkillIndex))
		{
			SetShown(Widgets.Root, false);
			continue;
		}
		const FSkillUI& Skill = Skills[SkillIndex];
		SetShown(Widgets.Root, true);
		SetTextIfPresent(Widgets.Name, Skill.mName);

		const int32 ShownCost = Skill.mActionPointCost;
		SetTextIfPresent(Widgets.Cost, FText::AsNumber(ShownCost));
		SetTextIfPresent(Widgets.CostLine, FText::Format(
			LOCTEXT("SkillCost", "AP {0}"), ShownCost));

		if (Widgets.Icon != nullptr && Skill.mIcon != nullptr)
		{
			Widgets.Icon->SetBrushFromTexture(Skill.mIcon, false);
		}

		// 쿨타임: 남은 턴이 있으면 그 숫자를, 없으면 설정된 쿨타임을 알려준다.
		if (Widgets.Cooldown != nullptr)
		{
			const bool bOnCooldown = Skill.mRemainingCooldown > 0;
			const bool bShowCooldown = bOnCooldown || Skill.mCooldownTurns > 0;
			SetShown(Widgets.Cooldown, bShowCooldown);
			SetShown(Widgets.CooldownIcon, bShowCooldown);
			// 숫자만 적는다. 옆에 모래시계가 붙어 있어서 "쿨"도 "턴"도
			// 같은 말을 두 번 하는 것이 된다.
			Widgets.Cooldown->SetText(FText::AsNumber(
				bOnCooldown ? Skill.mRemainingCooldown : Skill.mCooldownTurns));
		}

		if (Widgets.Damage != nullptr)
		{
			const bool bHasDamage = Skill.mDamageMax > 0;
			SetShown(Widgets.Damage, bHasDamage);
			if (bHasDamage)
			{
				// 시안은 "피해 8~14" 처럼 무엇의 숫자인지 적는다. 숫자만
				// 있으면 쿨 턴 수와 구분이 안 된다.
				Widgets.Damage->SetText(FText::FromString(FString::Printf(
					TEXT("피해 %d~%d"), Skill.mDamageMin, Skill.mDamageMax)));
			}
		}

		SetShown(Widgets.Disabled, !Skill.mIsUsable);
		if (Widgets.Button != nullptr)
		{
			// 못 쓰는 카드도 **누를 수는 있어야** 한다. 끄면 Slate 가 눌림
			// 알림을 아예 주지 않아서 길게 눌러 설명을 읽을 길도 같이 막힌다 --
			// 쿨타임이 도는 스킬이 무엇인지 알아야 다음 턴을 계획한다.
			//
			// 못 쓴다는 표시는 위의 Disabled 겹이 맡고, 고르는 것을 막는 일은
			// RequestCommand 가 맡는다.
			Widgets.Button->SetIsEnabled(true);
		}
	}
}

void UCombatLayoutHUDWidget::RefreshEnemy()
{
	// 안내판은 두 경우에만 뜬다. **짚은 적**이 있거나, **적 차례**거나.
	//
	// 전에는 아무것도 안 짚었으면 살아 있는 첫 적으로 떨어졌다. 그래서 내
	// 차례 내내 엉뚱한 적의 안내판이 떠 있었다 -- 누가 움직일 차례인지
	// 화면이 거짓말을 한 셈이다. 떨어질 곳을 없앴다.
	const TArray<FUnitUI>& Units = mUIModel->GetUnitUIs();
	const FCombatTargetUI& Target = mUIModel->GetTarget();
	const int32 TargetId = Target.mIsValid ? Target.mUnitId : INDEX_NONE;
	const FUnitUI* Shown = TargetId != INDEX_NONE
		? Units.FindByPredicate([TargetId](const FUnitUI& Unit)
			{ return Unit.mUnitId == TargetId && !Unit.mIsPlayer && Unit.mHP > 0.f; })
		: nullptr;
	if (Shown == nullptr)
	{
		const FUnitUI* TurnUnit = FindTurnUnit();
		if (TurnUnit != nullptr && !TurnUnit->mIsPlayer && TurnUnit->mHP > 0.f)
		{
			Shown = TurnUnit;
		}
	}

	SetShown(mEnemyPanel, Shown != nullptr);
	if (Shown == nullptr)
	{
		return;
	}

	SetTextIfPresent(mEnemyName, Shown->mName);
	if (mEnemyPortrait != nullptr && Shown->mPortrait != nullptr)
	{
		SetPortraitCropped(mEnemyPortrait, Shown->mPortrait);
	}
	if (mEnemyHPBar != nullptr)
	{
		mEnemyHPBar->SetPercent(
			Shown->mMaxHP > 0.f ? Shown->mHP / Shown->mMaxHP : 0.f);
	}
	SetTextIfPresent(mEnemyHPText, FText::FromString(FString::Printf(
		TEXT("%d/%d"), FMath::RoundToInt(Shown->mHP), FMath::RoundToInt(Shown->mMaxHP))));
	SetTextIfPresent(mEnemyDefenseText, FText::FromString(FString::Printf(
		TEXT("방어 %d"), FMath::RoundToInt(Shown->mDefensePoint))));

	if (mEnemyStatusText != nullptr)
	{
		TArray<FString> Parts;
		for (const FStatusEffectUI& Status : Shown->mStatusEffects)
		{
			Parts.Add(Status.mTag.GetTagName().ToString());
		}
		const bool bAny = Parts.Num() > 0;
		SetShown(mEnemyStatusText, bAny);
		if (bAny)
		{
			mEnemyStatusText->SetText(FText::FromString(
				FString::Join(Parts, TEXT(" / "))));
		}
	}

	// 예상 피해는 선택된 스킬에서 읽는다. 고른 게 없으면 감춘다.
	if (mEnemyForecastText != nullptr)
	{
		const TArray<FSkillUI>& Skills = mUIModel->GetSkillUIs();
		const int32 Selected = mUIModel->GetSelectedSkillIndex();
		const bool bHas = Skills.IsValidIndex(Selected) && Skills[Selected].mDamageMax > 0;
		SetShown(mEnemyForecastText, bHas);
		if (bHas)
		{
			mEnemyForecastText->SetText(FText::FromString(FString::Printf(
				TEXT("예상 피해 %d~%d"),
				Skills[Selected].mDamageMin, Skills[Selected].mDamageMax)));
		}
	}
}

void UCombatLayoutHUDWidget::RefreshMeta()
{
	const TArray<FUnitUI>& Units = mUIModel->GetUnitUIs();
	int32 Alive = 0;
	for (const FUnitUI& Unit : Units)
	{
		Alive += (!Unit.mIsPlayer && Unit.mHP > 0.f) ? 1 : 0;
	}
	SetTextIfPresent(mObjectiveText, FText::FromString(
		FString::Printf(TEXT("모든 적 처치 — 남은 적 %d"), Alive)));
	SetTextIfPresent(mMercenaryGoldText,
		FText::AsNumber(mUIModel->GetPlayerMeta().mGold));

	const TArray<FCombatArtifactUI>& Artifacts =
		mUIModel->GetPlayerMeta().mArtifacts;
	for (int32 Index = 0; Index < ArtifactSlotCount; ++Index)
	{
		const FCombatArtifactUI* Artifact =
			Artifacts.IsValidIndex(Index) ? &Artifacts[Index] : nullptr;
		UImage* Icon = mArtifactIcons.IsValidIndex(Index)
			? mArtifactIcons[Index].Get() : nullptr;
		UWidget* Frame = mArtifactFrames.IsValidIndex(Index)
			? mArtifactFrames[Index].Get() : nullptr;
		const bool bHasArtifact = Artifact != nullptr && Artifact->mIcon != nullptr;
		SetShown(Icon, bHasArtifact);
		SetShown(Frame, bHasArtifact);
		if (bHasArtifact)
		{
			Icon->SetBrushFromTexture(Artifact->mIcon);
		}
	}
}

void UCombatLayoutHUDWidget::SetMercenaryPanelShown(const bool bShown)
{
	if (mMercenaryPanel == nullptr)
	{
		return;
	}

	// 용병 패널과 몬스터 탭은 상호 배타 모달이다. 먼저 접어야 아래의
	// TurnPanel 접힘 결정이 이 함수 것으로 남는다.
	if (bShown == true)
	{
		SetMonsterTabShown(false);
	}

	// 먼저 패널을 세워 둔다. RequestCancel 이 즉시 UI 갱신을 쏘더라도 그
	// 갱신에서 커맨드 카드가 다시 튀어나오지 않는다.
	mMercenaryPanel->SetVisibility(
		bShown ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	// 모달 셸이 떠 있을 때 턴 라벨이 헤더 위로 새어 나오지 않게 한다.
	// Canvas 중첩의 자식 레이어는 부모 z-order와 별개로 배치될 수 있으므로,
	// 가리는 데 기대지 않고 턴 묶음을 명시적으로 접는다.
	SetShown(Find<UWidget>(WidgetTree, TEXT("TurnPanel")), bShown == false);
	if (bShown == true)
	{
		// 용병을 보러 가는 동안 앞서 열어 둔 상세와 조준은 다른 맥락이다.
		HideDetailOverlay(/*bNotifyGameplay=*/true);
		if (mUIModel != nullptr && IsAiming() == true)
		{
			mUIModel->RequestCancel();
		}
		if (mUIModel != nullptr)
		{
			RefreshParty();
			RefreshMeta();
		}
	}
	RefreshCommandVisibility();
}

bool UCombatLayoutHUDWidget::IsMercenaryPanelShown() const
{
	if (mMercenaryPanel == nullptr)
	{
		return false;
	}
	const ESlateVisibility PanelVisibility = mMercenaryPanel->GetVisibility();
	return PanelVisibility != ESlateVisibility::Collapsed
		&& PanelVisibility != ESlateVisibility::Hidden;
}

void UCombatLayoutHUDWidget::RequestCommand(const int32 SlotIndex)
{
	if (mUIModel == nullptr)
	{
		return;
	}
	// 긴 누름이 이미 상세를 열어 준 누름이다. 이 클릭은 놓는 동작일 뿐이다.
	if (mSwallowNextCommandClick == true)
	{
		mSwallowNextCommandClick = false;
		UE_LOG(LogRD, Log, TEXT("[탭진단] 카드 %d 클릭 삼킴(긴 누름 뒤 놓기)"), SlotIndex);
		return;
	}
	UE_LOG(LogRD, Log, TEXT("[탭진단] 카드 %d 클릭 → 선택 경로"), SlotIndex);
	// 카드를 골랐으면 상세는 볼 만큼 봤다. 위협 범위 칠은 게임플레이가
	// 알아서 관리하므로 화면만 닫는다.
	HideDetailOverlay(/*bNotifyGameplay=*/false);
	if (SlotIndex == 0)
	{
		// 행동력이 없으면 이동 모드에 들어가지 않는다. 갈 칸이 없는데
		// 조준만 열리면 취소밖에 할 게 없다.
		const FUnitUI* TurnUnit = FindTurnUnit();
		if (TurnUnit == nullptr || FMath::RoundToInt(TurnUnit->mMovementPoint) <= 0)
		{
			return;
		}
		mUIModel->RequestMove();
		return;
	}

	// 못 쓰는 카드는 고르지 않는다. 카드를 늘 눌리게 두었으니(RefreshCommands)
	// 막는 자리가 여기다. 판정은 게임플레이가 내린 mIsUsable 을 그대로 따른다 --
	// 화면이 사거리나 쿨타임을 다시 세지 않는다.
	const int32 SkillIndex = SlotIndex - 1;
	const TArray<FSkillUI>& Skills = mUIModel->GetSkillUIs();
	if (Skills.IsValidIndex(SkillIndex) == true && Skills[SkillIndex].mIsUsable == false)
	{
		return;
	}
	mUIModel->RequestSelectSkill(SkillIndex);
}

void UCombatLayoutHUDWidget::HandleCommandClicked_0() { RequestCommand(0); }
void UCombatLayoutHUDWidget::HandleCommandClicked_1() { RequestCommand(1); }
void UCombatLayoutHUDWidget::HandleCommandClicked_2() { RequestCommand(2); }
void UCombatLayoutHUDWidget::HandleCommandClicked_3() { RequestCommand(3); }
void UCombatLayoutHUDWidget::HandleCommandClicked_4() { RequestCommand(4); }
void UCombatLayoutHUDWidget::HandleCommandClicked_5() { RequestCommand(5); }

/**
 * @brief 명령 카드를 펴거나 접는다.
 *
 * @details
 * 카드 여섯 장이 판 한가운데를 덮는다. 어디로 갈지 보면서 골라야 하는데 그
 * 판이 가려지므로 접을 수 있어야 한다.
 *
 * 화면 상태이지 게임 상태가 아니다. UIModel 로 안 보낸다 -- 게임플레이가
 * "카드가 보이는지"를 알기 시작하면 그 다음엔 "어느 카드가 위인지"도 알게 된다.
 * @param bShown 펼지 접을지
 */
void UCombatLayoutHUDWidget::SetCommandsShown(const bool bShown)
{
	mCommandsShown = bShown;
	RefreshCommandVisibility();
}

const FUnitUI* UCombatLayoutHUDWidget::FindTurnUnit() const
{
	if (mUIModel == nullptr)
	{
		return nullptr;
	}
	const int32 TurnUnitId = mUIModel->GetTurnUI().mCurrentUnitId;
	if (TurnUnitId == INDEX_NONE)
	{
		return nullptr;
	}
	return mUIModel->GetUnitUIs().FindByPredicate(
		[TurnUnitId](const FUnitUI& Unit) { return Unit.mUnitId == TurnUnitId; });
}

bool UCombatLayoutHUDWidget::IsPlayerTurn() const
{
	const FUnitUI* TurnUnit = FindTurnUnit();
	// 차례를 모르는 짧은 공백을 아군 턴으로 간주하면, 적이 죽거나 턴 순서가
	// 갈리는 프레임에 마지막 아군 카드가 다시 나타난다. 시작 알림에서
	// TurnUI/UnitUI를 먼저 밀어주므로 모르는 동안은 안전하게 접는다.
	return TurnUnit != nullptr && TurnUnit->mIsPlayer;
}

void UCombatLayoutHUDWidget::HandleTurnPresentationBegin(
	TSharedPtr<FPresentationBarrier> /*Barrier*/)
{
	// 이전 행동의 "다음 틱에 다시 펴기" 예약이 남아 있어도 새 턴 상태보다
	// 늦게 적용되지 않게 한다.
	++mActionPresentationSerial;
	mIsActionPlaying = false;
	mIsTurnActive = true;
	RefreshCommandVisibility();
}

void UCombatLayoutHUDWidget::HandleTurnPresentationEnd(
	TSharedPtr<FPresentationBarrier> /*Barrier*/)
{
	// TurnUI는 다음 턴이 실제로 시작될 때까지 방금 끝난 아군을 가리킨다.
	// 그 스냅샷과 무관하게 종료 알림 즉시 카드를 내린다.
	++mActionPresentationSerial;
	mIsActionPlaying = false;
	mIsTurnActive = false;
	RefreshCommandVisibility();
}

void UCombatLayoutHUDWidget::HandleActionPresentationBegin(
	TSharedPtr<FPresentationBarrier> /*Barrier*/)
{
	++mActionPresentationSerial;
	mIsActionPlaying = true;
	// 행동이 실제로 굴러가기 시작했다. 상세 패널이 판을 가리고 있으면 무엇이
	// 움직이는지 안 보인다. 위협 범위 칠은 명령 쪽에서 이미 걷었다.
	HideDetailOverlay(/*bNotifyGameplay=*/false);
	RefreshCommandVisibility();
}

void UCombatLayoutHUDWidget::HandleActionPresentationEnd(
	TSharedPtr<FPresentationBarrier> /*Barrier*/)
{
	const uint64 PresentationSerial = ++mActionPresentationSerial;

	// BuildAction 종료 콜백이 반환되어야 그 다음 SkillAction이 시작된다.
	// 여기서 즉시 false로 바꾸면 두 액션 사이에 카드가 한 프레임 보인다.
	// 다음 틱으로 미루면 같은 프레임에 오는 다음 Begin이 일련번호를 바꿔
	// 이 예약을 무효화한다.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateWeakLambda(this,
				[this, PresentationSerial]()
				{
					CompleteActionPresentationEnd(PresentationSerial);
				}));
		return;
	}

	CompleteActionPresentationEnd(PresentationSerial);
}

void UCombatLayoutHUDWidget::CompleteActionPresentationEnd(
	const uint64 PresentationSerial)
{
	if (PresentationSerial != mActionPresentationSerial)
	{
		return;
	}

	mIsActionPlaying = false;
	RefreshCommandVisibility();
}

void UCombatLayoutHUDWidget::BindUIModel(UCombatUIModel* InUIModel)
{
	if (mUIModel == InUIModel)
	{
		return;
	}

	Super::BindUIModel(InUIModel);
	if (mUIModel != nullptr)
	{
		++mActionPresentationSerial;
		mIsTurnActive = false;
		mIsActionPlaying = false;

		mTurnBeginHandle = mUIModel->OnBeginAnyTurn.AddUObject(
			this, &UCombatLayoutHUDWidget::HandleTurnPresentationBegin);
		mTurnEndHandle = mUIModel->OnEndAnyTurn.AddUObject(
			this, &UCombatLayoutHUDWidget::HandleTurnPresentationEnd);
		mActionBeginHandle = mUIModel->OnBeginAnyTurnAction.AddUObject(
			this, &UCombatLayoutHUDWidget::HandleActionPresentationBegin);
		mActionEndHandle = mUIModel->OnEndAnyTurnAction.AddUObject(
			this, &UCombatLayoutHUDWidget::HandleActionPresentationEnd);

		// 전투가 끝나면 결과 연출을 맡는다. 배리어를 넘겨받아 붙잡는다.
		mVictoryWorldMapLocked = false;
		mCombatResultFlowActive = false;

		// 승리 경로는 HUD 를 접은 채로 지도를 연다(결과 화면이 끝나고 조작을
		// 되돌리지 않는 유일한 길이다). 그런데 OpenUI 는 **이미 열린 위젯이면
		// 그냥 돌아가므로** 다음 전투에서 다시 펴 주지 않는다 -- 접힌 채로
		// 남으면 그 판 내내 HUD 가 안 보인다. 붙을 때마다 여기서 편다.
		SetCombatControlsShown(true);
		mUIModel->OnEndCombat.RemoveAll(this);
		mEndCombatHandle = mUIModel->OnEndCombat.AddWeakLambda(this,
			[this](TSharedPtr<FPresentationBarrier> Barrier)
			{
				HandleEndCombatUI(MoveTemp(Barrier));
			});
		mUIModel->OnCombatResultOpenRequested.AddUniqueDynamic(
			this, &UCombatLayoutHUDWidget::HandleCombatResultOpenRequested);

		// 맞은 자리 위로 뜨는 피해 숫자.
		mUIModel->OnCombatFloatingLog.AddUniqueDynamic(
			this, &UCombatLayoutHUDWidget::HandleCombatFloatingLog);
		mUIModel->OnCombatFloatingLogMotionFinished.AddUniqueDynamic(
			this, &UCombatLayoutHUDWidget::HandleCombatFloatingLogMotionFinished);
		mUIModel->OnCombatFloatingLogsCleared.AddUniqueDynamic(
			this, &UCombatLayoutHUDWidget::HandleCombatFloatingLogsCleared);

		// 라운드 배너: 배리어를 붙잡고 틀었다가, 끝나면 놓아 첫 턴을 진행시킨다.
		// 못 틀면 즉시 놓는다 -- 안 그러면 그 라운드가 영영 안 넘어간다.
		mUIModel->OnBeginAnyRound.RemoveAll(this);
		mBeginRoundHandle = mUIModel->OnBeginAnyRound.AddWeakLambda(this,
			[this](TSharedPtr<FPresentationBarrier> Barrier)
			{
				mRoundChangeBarrier.Reset();
				mRoundChangeBarrier = MoveTemp(Barrier);
				// 배너를 안 틀 때도 **배리어는 반드시 놓는다.** 붙잡은 채로
				// 두면 그 라운드 첫 턴이 영영 안 온다.
				if (mPlayRoundBanner == false || PlayTurnChangeIntro() == false)
				{
					mRoundChangeBarrier.Reset();
				}
			});
	}
}

void UCombatLayoutHUDWidget::BindRewardUIModel(URewardUIModel* InUIModel)
{
	if (mCombatRewardUIModel == InUIModel)
	{
		return;
	}

	if (mCombatRewardUIModel != nullptr)
	{
		mCombatRewardUIModel->OnRewardClaimConfirmed.RemoveDynamic(
			this, &UCombatLayoutHUDWidget::HandleCombatRewardClaimConfirmed);
	}

	mCombatRewardUIModel = InUIModel;
	if (mCombatRewardUIModel != nullptr)
	{
		mCombatRewardUIModel->OnRewardClaimConfirmed.AddUniqueDynamic(
			this, &UCombatLayoutHUDWidget::HandleCombatRewardClaimConfirmed);
	}

	if (mCombatRewardWidget != nullptr)
	{
		mCombatRewardWidget->BindUIModel(mCombatRewardUIModel);
	}
}

void UCombatLayoutHUDWidget::UnbindUIModel()
{
	HideDetailOverlay(/*bNotifyGameplay=*/false);
	if (mUIModel != nullptr)
	{
		mUIModel->OnBeginAnyTurn.Remove(mTurnBeginHandle);
		mUIModel->OnEndAnyTurn.Remove(mTurnEndHandle);
		mUIModel->OnBeginAnyTurnAction.Remove(mActionBeginHandle);
		mUIModel->OnEndAnyTurnAction.Remove(mActionEndHandle);
		mUIModel->OnEndCombat.Remove(mEndCombatHandle);
		mUIModel->OnCombatResultOpenRequested.RemoveDynamic(
			this, &UCombatLayoutHUDWidget::HandleCombatResultOpenRequested);
		mUIModel->OnCombatFloatingLog.RemoveDynamic(
			this, &UCombatLayoutHUDWidget::HandleCombatFloatingLog);
		mUIModel->OnCombatFloatingLogMotionFinished.RemoveDynamic(
			this, &UCombatLayoutHUDWidget::HandleCombatFloatingLogMotionFinished);
		mUIModel->OnCombatFloatingLogsCleared.RemoveDynamic(
			this, &UCombatLayoutHUDWidget::HandleCombatFloatingLogsCleared);
		mUIModel->OnBeginAnyRound.Remove(mBeginRoundHandle);
	}
	++mActionPresentationSerial;
	mIsTurnActive = false;
	mIsActionPlaying = false;
	mTurnBeginHandle.Reset();
	mTurnEndHandle.Reset();
	mActionBeginHandle.Reset();
	mActionEndHandle.Reset();
	Super::UnbindUIModel();
}

bool UCombatLayoutHUDWidget::IsAiming() const
{
	return mUIModel != nullptr
		&& mUIModel->GetTurnUI().mPhase != ECombatBuildPhaseUI::None;
}

/**
 * @brief 확정 단추와 턴 종료 글자를 지금 단계에 맞춘다.
 *
 * @details
 * 확정은 **공격 범위가 뜬 그때만** 뜬다. 늘 떠 있으면 무엇을 확정하는
 * 단추인지 읽히지 않는다.
 *
 * 턴 종료는 그 사이 "취소" 가 된다. 무르는 길이 판 밖을 누르는 것뿐이면
 * 판이 화면을 거의 다 덮고 있어 무를 자리가 없다 -- 늘 같은 자리에 있는
 * 단추가 그 길이 된다.
 */
/**
 * @brief 창 크기가 바뀌면 다시 잰다.
 *
 * 갱신은 전투가 움직일 때만 온다. 창만 끌어 늘리면 아무 갱신도 안 오므로
 * 여기서 본다 -- 크기가 그대로면 아무 일도 안 한다.
 */
void UCombatLayoutHUDWidget::NativeTick(const FGeometry& MyGeometry, float DeltaTime)
{
	Super::NativeTick(MyGeometry, DeltaTime);
	RefreshScreenScale();
	// 머리 위 바는 월드 자리를 따라가야 하므로 매 프레임 다시 붙인다.
	UpdateUnitHpBars();
	RefreshPendingAPGlow(DeltaTime);
	UpdateFloatingCombatLogQueue(DeltaTime);
	UpdateFloatingCombatLogs(DeltaTime);
	if (mTurnChangeIntroPlaying)
	{
		UpdateTurnChangeIntro(DeltaTime);
	}
}

void UCombatLayoutHUDWidget::RefreshScreenScale()
{
	const UWorld* World = GetWorld();
	if (World == nullptr || World->GetGameViewport() == nullptr)
	{
		return;
	}
	FVector2D Viewport = FVector2D::ZeroVector;
	World->GetGameViewport()->GetViewportSize(OUT Viewport);
	if (Viewport.X <= 0.f || Viewport.Y <= 0.f || Viewport.Equals(mLastViewport))
	{
		return;
	}
	mLastViewport = Viewport;

	// 16:9 보다 좁아진 만큼 키운다. 넓어져도 줄이지는 않는다 -- 넓은 화면에서
	// 작아 보이는 것은 문제가 아니라 판이 넓게 보이는 것이다.
	const float Wide = 16.f / 9.f;
	const float Ratio = Viewport.X / Viewport.Y;
	const float Scale = FMath::Clamp(Wide / Ratio, 1.f, MaxScreenScale);

	// 배치 배율로 준다. 렌더 변환으로 주면 글자가 지글거린다 -- 그쪽은 이미
	// 그려 놓은 것을 늘리는 것이라 글리프 그림째로 늘어난다. 배치 배율은
	// 안쪽을 그 크기로 다시 배치하고 글자도 다시 그린다.
	for (UScaleBox* Layer : { mCommandLayer.Get(), mPartyLayer.Get() })
	{
		if (Layer != nullptr)
		{
			Layer->SetUserSpecifiedScale(Scale);
		}
	}
}

void UCombatLayoutHUDWidget::RefreshActionButtons()
{
	const ECombatBuildPhaseUI Phase = mUIModel != nullptr
		? mUIModel->GetTurnUI().mPhase : ECombatBuildPhaseUI::None;

	SetShown(mConfirmPanel, Phase == ECombatBuildPhaseUI::Preview);
	SetTextIfPresent(mEndTurnLabel, Phase == ECombatBuildPhaseUI::None
		? LOCTEXT("EndTurn", "턴 종료") : LOCTEXT("CancelAim", "취소"));
}

/** @brief 가운데 AP 막대를 지금 차례인 유닛으로 채운다. */
void UCombatLayoutHUDWidget::RefreshTurnActionPoints()
{
	const FUnitUI* TurnUnit = FindTurnUnit();
	const int32 Left = TurnUnit != nullptr
		? mUIModel->ResolveDisplayedMovementPoint(*TurnUnit) : 0;
	const int32 Total = TurnUnit != nullptr
		? FMath::Max(FMath::RoundToInt(TurnUnit->mMaxMovementPoint), Left) : 0;

	SetTextIfPresent(mTurnAPText, FText::FromString(
		FString::Printf(TEXT("%d/%d"), Left, Total)));
	mShownAPLeft = Left;

	// 고른 카드가 가져갈 몫. 남은 것보다 크면 남은 만큼만 빛낸다.
	mPendingAPCost = FMath::Clamp(GetPendingActionCost(), 0, Left);

	const int32 Room = mTurnAPPips.Num();
	const bool bTooMany = Total > Room;
	for (int32 Pip = 0; Pip < Room; ++Pip)
	{
		const bool bHasPip = !bTooMany && Pip < Total;
		SetShown(mTurnAPPips[Pip], bHasPip && Pip < Left);
		if (mTurnAPPipsUsed.IsValidIndex(Pip))
		{
			SetShown(mTurnAPPipsUsed[Pip], bHasPip && Pip >= Left);
		}
	}
	RefreshPendingAPGlow(0.f);
}

/**
 * @brief 현재 프리뷰 중인 스킬 또는 이동 경로가 가져갈 행동력.
 * @return 프리뷰가 없으면 0
 */
int32 UCombatLayoutHUDWidget::GetPendingActionCost() const
{
	if (mUIModel == nullptr)
	{
		return 0;
	}
	return mUIModel->GetPendingAction().mActionPointCost;
}

/**
 * @brief 가져갈 몫만큼 칸을 숨쉬듯 빛낸다.
 *
 * @details
 * 남은 칸의 **뒤에서부터** 빛낸다 -- 쓰면 뒤부터 없어지므로, 없어질 그 칸이
 * 빛나야 "이만큼 나간다" 로 읽힌다.
 *
 * 밝기만 흔든다. 크기를 흔들면 그린 그림을 다시 샘플링해서 지글거린다 --
 * 글자에서 이미 같은 일을 겪었다.
 *
 * @param DeltaTime 지난 시간. 0 이면 위상은 그대로 두고 다시 칠하기만 한다
 */
void UCombatLayoutHUDWidget::RefreshPendingAPGlow(const float DeltaTime)
{
	mAPGlowPhase = FMath::Fmod(mAPGlowPhase + DeltaTime * APGlowSpeed, TWO_PI);

	// 0.55 ~ 1.0 사이를 오간다. 완전히 어두워지면 칸이 사라진 것처럼 보인다.
	const float Wave = 0.5f * (1.f - FMath::Cos(mAPGlowPhase));
	const float Glow = FMath::Lerp(0.55f, 1.0f, Wave);

	const int32 Room = mTurnAPPips.Num();
	const int32 Left = mShownAPLeft;
	for (int32 Pip = 0; Pip < Room; ++Pip)
	{
		UWidget* PipWidget = mTurnAPPips[Pip];
		if (PipWidget == nullptr)
		{
			continue;
		}
		const bool bWillSpend = mPendingAPCost > 0
			&& Pip < Left && Pip >= Left - mPendingAPCost;
		PipWidget->SetRenderOpacity(bWillSpend ? Glow : 1.f);
	}
}

/** @brief 확정 단추를 눌렀다. 겨냥한 칸을 그대로 확정한다. */
void UCombatLayoutHUDWidget::HandleConfirmClicked()
{
	if (mUIModel != nullptr)
	{
		HideDetailOverlay(/*bNotifyGameplay=*/false);
		mUIModel->RequestConfirm();
	}
}

void UCombatLayoutHUDWidget::RefreshCommandVisibility()
{
	// 조건이 다 참이어야 보인다. 하나라도 아니면 접는다.
	const bool bVisible = mCommandsShown == true
		&& IsAiming() == false
		&& IsPlayerTurn() == true
		&& mIsTurnActive == true
		&& mIsActionPlaying == false
		&& IsMercenaryPanelShown() == false
		&& IsMonsterTabShown() == false;

	// [진단] 카드 표시 결정이 바뀌는 순간의 조건을 남긴다. 카드가 안 돌아오는
	// 버그를 잡으면 지운다.
	static bool bLastVisible = true;
	if (bVisible != bLastVisible)
	{
		bLastVisible = bVisible;
		UE_LOG(LogRD, Log, TEXT("[카드진단] 표시=%d (펴둠=%d 조준중=%d 아군턴=%d 턴열림=%d 연출중=%d 용병패널=%d)"),
			bVisible, mCommandsShown, IsAiming(), IsPlayerTurn(),
			mIsTurnActive, mIsActionPlaying, IsMercenaryPanelShown());
	}

	for (const FCommandSlotWidgets& Widgets : mCommandSlots)
	{
		SetShown(Widgets.Root, bVisible);
	}
}

/**
 * @brief 카드 밖을 눌렀다. 화면을 한 단계 뒤로 되돌린다.
 *
 * @details
 * 눌린 자리가 버튼이면 버튼이 먼저 가져가므로 여기는 안 불린다. 그래서 아군
 * 칸을 눌러 카드를 다시 펼 때 이 함수가 같은 탭에서 도로 접지 않는다.
 *
 * 한때 카메라가 쏘는 월드 탭 알림을 듣게 해 봤는데, 그 알림은 버튼을 눌러도
 * 똑같이 온다. 아군 칸을 눌러 편 카드가 그 자리에서 도로 접혔다.
 */
/**
 * @brief 열고 나서 화면 전체를 덮지 않게 낮춘다.
 * @param Callback 열기 연출이 끝났을 때 부를 것
 */
void UCombatLayoutHUDWidget::OpenUI(FOnEndUIOpenAnimation Callback)
{
	Super::OpenUI(MoveTemp(Callback));

	// 화면 전체가 눌림을 받아야 카드 밖 탭을 안다. 옛 전투 HUD 도 같은
	// 방식이었다 -- 자식 버튼이 먼저 가져가고, 버튼 밖만 이 위젯이 받는다.
	SetVisibility(ESlateVisibility::Visible);
	SetMercenaryPanelShown(false);
}

/**
 * @brief 모델을 붙이면서 판 탭 알림도 같이 구독한다.
 * @param InUIModel 붙일 모델
 */
FReply UCombatLayoutHUDWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}
	mPressOrigin = FVector2D(InMouseEvent.GetScreenSpacePosition());
	mPressMoved = false;
	mPressActive = true;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(mBoardLongPressTimerHandle, this,
			&UCombatLayoutHUDWidget::HandleBoardLongPress, LongPressSeconds, false);
	}
	return FReply::Handled();
}

FReply UCombatLayoutHUDWidget::NativeOnTouchStarted(const FGeometry& InGeometry,
	const FPointerEvent& InTouchEvent)
{
	mPressOrigin = FVector2D(InTouchEvent.GetScreenSpacePosition());
	mPressMoved = false;
	mPressActive = true;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(mBoardLongPressTimerHandle, this,
			&UCombatLayoutHUDWidget::HandleBoardLongPress, LongPressSeconds, false);
	}
	return FReply::Handled();
}

/**
 * @brief 판을 오래 눌렀다. 그 자리를 상세 요청으로 보낸다.
 *
 * @details
 * 이 누름은 여기서 소비한다(mPressActive 해제). 안 그러면 뗄 때 같은 누름이
 * 탭으로 한 번 더 처리되어, 상세를 열자마자 그 탭이 도로 닫는다.
 *
 * 조준 중이어도 보낸다. 긴 누름은 게임플레이 쪽에서 최우선 처리되어 조준을
 * 건드리지 않는다 -- 조준하다가 "이 적이 뭐더라" 를 볼 수 있어야 한다.
 */
void UCombatLayoutHUDWidget::HandleBoardLongPress()
{
	if (mPressActive == false || mPressMoved == true || mUIModel == nullptr
		|| IsMercenaryPanelShown() == true)
	{
		return;
	}
	mPressActive = false;

	// HUD 를 누른 것은 판을 누른 것이 아니다. 탭과 같은 규칙이다.
	if (IsOverChrome(mPressOrigin) == true)
	{
		return;
	}
	mUIModel->RequestWorldTouch(mPressOrigin, true);
}

bool UCombatLayoutHUDWidget::IsOverChrome(const FVector2D& ScreenPosition) const
{
	for (const UWidget* Chrome : mChromeWidgets)
	{
		if (Chrome == nullptr || Chrome->GetVisibility() == ESlateVisibility::Collapsed)
		{
			continue;
		}
		if (Chrome->GetCachedGeometry().IsUnderLocation(ScreenPosition) == true)
		{
			return true;
		}
	}
	return false;
}

/**
 * @brief 손가락을 뗐다. 톡 친 것이면 판 탭으로 넘긴다.
 *
 * @details
 * 모바일은 **지도를 끌어 옮기는 것도 누르는 것**이다. 누르자마자 처리하면
 * 화면을 밀 때마다 카드가 뒤집힌다 -- 실제로 그랬다.
 *
 * 그래서 누를 때는 자리만 적어 두고, 뗄 때 얼마나 움직였는지로 가른다.
 */
FReply UCombatLayoutHUDWidget::NativeOnTouchEnded(const FGeometry& InGeometry,
	const FPointerEvent& InTouchEvent)
{
	FinishBoardPress(FVector2D(InTouchEvent.GetScreenSpacePosition()));
	return Super::NativeOnTouchEnded(InGeometry, InTouchEvent);
}

FReply UCombatLayoutHUDWidget::NativeOnTouchMoved(const FGeometry& InGeometry,
	const FPointerEvent& InTouchEvent)
{
	if (FVector2D(InTouchEvent.GetScreenSpacePosition()).Equals(mPressOrigin, BoardTapSlack) == false)
	{
		mPressMoved = true;
		// 끌기 시작했다. 지도를 미는 손을 긴 누름으로 오인하지 않는다.
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(mBoardLongPressTimerHandle);
		}
	}
	return Super::NativeOnTouchMoved(InGeometry, InTouchEvent);
}

FReply UCombatLayoutHUDWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	FinishBoardPress(FVector2D(InMouseEvent.GetScreenSpacePosition()));
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

/** @brief 뗀 자리가 누른 자리 가까이면 탭, 아니면 끈 것으로 본다. */
void UCombatLayoutHUDWidget::FinishBoardPress(const FVector2D& ScreenPosition)
{
	/*
	 * 한 번 누른 것은 **한 번만** 넘긴다.
	 *
	 * 안드로이드는 터치가 마우스 이벤트도 합성해서, 손가락 하나에 터치와
	 * 마우스가 둘 다 온다. 누를 때 처리할 때는 안 드러났는데 뗄 때로 옮기니
	 * 뗌도 둘 다 와서 탭이 두 번 나갔다 -- 사거리에서 한 번 찍었는데 선택과
	 * 확정이 같이 되어 그 자리에서 발동했다.
	 */
	// 놓았으니 긴 누름 판정은 끝났다. 발화 전에 놓았으면 탭이다.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(mBoardLongPressTimerHandle);
	}

	if (mPressActive == false)
	{
		return;
	}
	mPressActive = false;

	const bool bDragged = mPressMoved
		|| ScreenPosition.Equals(mPressOrigin, BoardTapSlack) == false;
	mPressMoved = false;
	if (bDragged == true)
	{
		return;
	}
	HandleBoardPressed(ScreenPosition);
}

void UCombatLayoutHUDWidget::HandleBoardPressed(const FVector2D& ScreenPosition)
{
	if (mUIModel == nullptr)
	{
		return;
	}

	// 용병 패널은 모달이다. 패널 바깥으로 새어 온 눌림도 전장 명령이나
	// 커맨드 토글로 해석하지 않는다.
	if (IsMercenaryPanelShown() == true)
	{
		UE_LOG(LogRD, Log, TEXT("[탭진단] 판 탭 삼킴: 용병 패널 열림"));
		return;
	}

	// 상세 패널이 떠 있으면 이 탭은 닫기다. 열어 둔 채 다른 일까지 겸하면,
	// 스킬을 쏘려던 탭이었는지 패널을 닫는 탭이었는지 알 수 없다.
	if (IsDetailOverlayShown() == true)
	{
		UE_LOG(LogRD, Log, TEXT("[탭진단] 판 탭 = 상세 닫기"));
		HideDetailOverlay(/*bNotifyGameplay=*/true);
		return;
	}

	// HUD 를 누른 것은 판을 누른 것이 아니다. 조준 중이어도 마찬가지다 --
	// 안내판을 누르려다 엉뚱한 칸에 스킬을 쏘면 무를 길이 없다.
	if (IsOverChrome(ScreenPosition) == true)
	{
		UE_LOG(LogRD, Log, TEXT("[탭진단] 판 탭 삼킴: HUD(chrome) 위"));
		return;
	}

	// 조준 중이면 이 탭은 게임플레이 것이다. 사거리 안이면 그리로 쏘고
	// 밖이면 무른다 -- 어느 쪽인지는 스킬 조준만 안다. 무르고 나면 카드는
	// 저절로 돌아오므로 여기서 펴 줄 필요가 없다.
	if (IsAiming() == true)
	{
		mUIModel->RequestWorldTouch(ScreenPosition, false);
		return;
	}

	// 행동 연출이 도는 동안에는 살펴보기를 켜고 끄지 못한다. 스킬이 날아가는
	// 중에 겨냥이 갈리면 어느 것이 지금 일이고 어느 것이 봐 둔 것인지 섞인다.
	// 이미 칠해 둔 위협 범위는 **그대로 남는다** -- 막는 것은 켜고 끄기지,
	// 켜 둔 것을 끄는 것이 아니다.
	if (mIsActionPlaying == true)
	{
		return;
	}

	// 조준 중이 아닌 판 탭도 게임플레이로 보낸다. 적을 짚으면 위협 범위가
	// 판에 칠리고 안내판이 그 적으로 바뀐다 -- 길게 눌러야만 위협을 볼 수
	// 있으면 확인이 조작 사이에 못 끼어든다. 어느 칸에 무엇이 있는지는
	// 트레이스한 쪽만 안다.
	//
	// 처리는 같은 호출 안에서 끝난다(델리게이트 직행). 그래서 보내기 전후의
	// 겨냥을 비교하면 이 탭이 무엇을 한 탭인지 알 수 있다. 빈 땅 탭은
	// 살펴보기를 건드리지 않으므로 겨냥이 그대로다.
	const FCombatTargetUI BeforeTarget = mUIModel->GetTarget();

	mUIModel->RequestWorldTouch(ScreenPosition, false);

	const FCombatTargetUI& AfterTarget = mUIModel->GetTarget();
	const bool bTargetChanged = BeforeTarget.mIsValid != AfterTarget.mIsValid
		|| BeforeTarget.mUnitId != AfterTarget.mUnitId
		|| (BeforeTarget.mTile == AfterTarget.mTile) == false;

	if (bTargetChanged == true)
	{
		// 유닛을 새로 짚었으면 카드를 접는다 -- 위협 범위를 보라고 판에
		// 칠했는데 카드 여섯 장이 그 판 한가운데를 덮고 있으면 칠이 안
		// 보인다. 조준에 들어가면 카드가 비키는 것과 같은 이유다.
		//
		// 재탭으로 짚기를 풀었을 때는 카드를 **건드리지 않는다.** 그 탭은
		// 칠을 걷으려던 손이지 카드를 부르던 손이 아니다. 카드는 빈 땅
		// 탭으로만 여닫는다.
		const bool bHasUnit = AfterTarget.mIsValid == true && AfterTarget.mUnitId != INDEX_NONE;
		if (bHasUnit == true)
		{
			SetCommandsShown(false);
		}
		return;
	}

	// 겨냥이 그대로면 빈 땅 탭이다. 카드만 뒤집는다 -- 누르면 펴지고, 다시
	// 누르면 접힌다. 짚어 둔 위협 범위는 그대로 남는다.
	SetCommandsShown(!mCommandsShown);
}

/**
 * @brief 아군 칸을 눌렀다. 카드를 펴거나 접는다.
 *
 * 판 위의 유닛은 매 턴 다른 자리에 있고 작다. 아군 칸은 늘 같은 자리에 있고
 * 크며, 이미 금테두리로 "지금 차례" 를 말하고 있다.
 * @param SlotIndex 누른 칸
 */
int32 UCombatLayoutHUDWidget::PartyUnitIdAt(const int32 SlotIndex) const
{
	if (mUIModel == nullptr)
	{
		return INDEX_NONE;
	}

	int32 Seen = 0;
	for (const FUnitUI& Unit : mUIModel->GetUnitUIs())
	{
		if (Unit.mIsPlayer == false)
		{
			continue;
		}
		if (Seen == SlotIndex)
		{
			return Unit.mUnitId;
		}
		++Seen;
	}
	return INDEX_NONE;
}

void UCombatLayoutHUDWidget::HandlePartyClicked(const int32 SlotIndex)
{
	if (mUIModel == nullptr)
	{
		return;
	}
	const int32 UnitId = PartyUnitIdAt(SlotIndex);
	if (UnitId == INDEX_NONE)
	{
		return;
	}

	// 다른 용병을 보러 간다. 떠 있던 상세는 이전 맥락이다.
	HideDetailOverlay(/*bNotifyGameplay=*/true);

	// 조준 중이었으면 무른다. 남의 스킬을 보러 가면서 겨냥만 남겨 두면 그
	// 사거리가 누구 것인지 알 수 없다.
	if (IsAiming() == true)
	{
		mUIModel->RequestCancel();
	}

	// 상세 응답은 RequestInspectUnit 안에서 동기적으로 돌아올 수 있다. 목록을
	// 먼저 닫아 둬야 마지막 화면 상태가 PR457 상세 겹이 된다.
	SetMercenaryPanelShown(false);

	// 기존 계약대로 밑의 커맨드도 그 용병 것으로 갈아 끼운다. 상세를 닫은 뒤
	// 무엇을 들었는지 바로 이어 볼 수 있고, 제 차례가 아니면 게임플레이가
	// 전부 비활성으로 내려 준다.
	SetCommandsShown(true);
	mUIModel->RequestInspectUnit(UnitId);
}

void UCombatLayoutHUDWidget::HandlePartyClicked_0() { HandlePartyClicked(0); }
void UCombatLayoutHUDWidget::HandlePartyClicked_1() { HandlePartyClicked(1); }
void UCombatLayoutHUDWidget::HandlePartyClicked_2() { HandlePartyClicked(2); }
void UCombatLayoutHUDWidget::HandleMercenaryMenuClicked()
{
	SetMercenaryPanelShown(IsMercenaryPanelShown() == false);
}
void UCombatLayoutHUDWidget::HandleMonsterMenuClicked()
{
	if (mUIModel == nullptr)
	{
		return;
	}
	if (IsMonsterTabShown() == true)
	{
		SetMonsterTabShown(false);
		return;
	}
	if (mMonsterTabWidgetClass != nullptr)
	{
		SetMonsterTabShown(true);
		return;
	}

	// 몬스터 탭 WBP가 없는 환경(테스트 픽스처 등)에서는 예전 계약대로
	// 첫 생존 적의 상세 패널을 연다.
	SetMercenaryPanelShown(false);
	HideDetailOverlay(/*bNotifyGameplay=*/true);
	if (IsAiming() == true)
	{
		mUIModel->RequestCancel();
	}
	const FUnitUI* Monster = mUIModel->GetUnitUIs().FindByPredicate(
		[](const FUnitUI& Unit)
		{
			return Unit.mIsPlayer == false && Unit.mHP > 0.f;
		});
	if (Monster != nullptr)
	{
		mUIModel->RequestInspectUnit(Monster->mUnitId);
	}
}

void UCombatLayoutHUDWidget::HandleMonsterTabRowClicked_0() { HandleMonsterTabRowClicked(0); }
void UCombatLayoutHUDWidget::HandleMonsterTabRowClicked_1() { HandleMonsterTabRowClicked(1); }
void UCombatLayoutHUDWidget::HandleMonsterTabRowClicked_2() { HandleMonsterTabRowClicked(2); }

void UCombatLayoutHUDWidget::HandleMonsterTabBackClicked()
{
	SetMonsterTabShown(false);
}

void UCombatLayoutHUDWidget::HandleMonsterTabRowClicked(const int32 RowIndex)
{
	if (mMonsterTabUnitIds.IsValidIndex(RowIndex) == false)
	{
		return;
	}
	mMonsterTabSelectedRow = RowIndex;
	RefreshMonsterTab();
}

void UCombatLayoutHUDWidget::SetMonsterTabShown(const bool bShown)
{
	if (bShown == true && EnsureMonsterTabWidget() == false)
	{
		return;
	}
	if (mMonsterTabWidget == nullptr)
	{
		return;
	}

	if (bShown == true)
	{
		// 다른 모달·상세·조준을 먼저 걷는다. 용병 패널 접기가 TurnPanel을
		// 되살리므로, 이 함수의 TurnPanel 접힘이 마지막 결정이 되게 한다.
		SetMercenaryPanelShown(false);
		HideDetailOverlay(/*bNotifyGameplay=*/true);
		if (mUIModel != nullptr && IsAiming() == true)
		{
			mUIModel->RequestCancel();
		}
	}

	mMonsterTabWidget->SetVisibility(
		bShown ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	// 용병 패널과 같은 모달 계약: 탭이 떠 있는 동안 턴 묶음은 접는다.
	SetShown(Find<UWidget>(WidgetTree, TEXT("TurnPanel")),
		bShown == false && IsMercenaryPanelShown() == false);

	if (bShown == true)
	{
		RefreshMonsterTab();
	}
	else
	{
		// 상세 요청으로 칠렸을 수 있는 위협 범위를 걷으라는 신호. INDEX_NONE = "닫았다".
		if (mUIModel != nullptr && mMonsterTabInspectedUnitId != INDEX_NONE)
		{
			mUIModel->RequestLongPressUnit(INDEX_NONE);
		}
		mMonsterTabInspectedUnitId = INDEX_NONE;
	}
	RefreshCommandVisibility();
}

bool UCombatLayoutHUDWidget::IsMonsterTabShown() const
{
	if (mMonsterTabWidget == nullptr)
	{
		return false;
	}
	const ESlateVisibility TabVisibility = mMonsterTabWidget->GetVisibility();
	return TabVisibility != ESlateVisibility::Collapsed
		&& TabVisibility != ESlateVisibility::Hidden;
}

bool UCombatLayoutHUDWidget::EnsureMonsterTabWidget()
{
	if (mMonsterTabWidget != nullptr)
	{
		return true;
	}
	if (mMonsterTabWidgetClass == nullptr)
	{
		return false;
	}
	if (APlayerController* OwningPlayer = GetOwningPlayer())
	{
		mMonsterTabWidget = CreateWidget<UUserWidget>(OwningPlayer, mMonsterTabWidgetClass);
	}
	else if (UWorld* World = GetWorld())
	{
		// 에디터 자동화처럼 로컬 플레이어가 없는 월드에서도 탭 계약은 검증할
		// 수 있어야 한다. 실제 플레이에서는 늘 위의 소유 플레이어를 쓴다.
		mMonsterTabWidget = CreateWidget<UUserWidget>(World, mMonsterTabWidgetClass);
	}
	if (mMonsterTabWidget == nullptr)
	{
		return false;
	}
	// 상세 겹(50)보다 위. 탭이 떠 있는 동안 아래 HUD·판 눌림은 딤이 삼킨다.
	mMonsterTabWidget->AddToViewport(/*ZOrder=*/55);
	mMonsterTabWidget->SetVisibility(ESlateVisibility::Collapsed);

	// AddDynamic은 함수 이름을 문자열로 찍는 매크로라 포인터 배열로 돌릴 수 없다.
	if (UButton* Row0 = Cast<UButton>(mMonsterTabWidget->GetWidgetFromName(TEXT("MonsterRowButton_0"))))
	{
		Row0->OnClicked.AddDynamic(this, &UCombatLayoutHUDWidget::HandleMonsterTabRowClicked_0);
	}
	if (UButton* Row1 = Cast<UButton>(mMonsterTabWidget->GetWidgetFromName(TEXT("MonsterRowButton_1"))))
	{
		Row1->OnClicked.AddDynamic(this, &UCombatLayoutHUDWidget::HandleMonsterTabRowClicked_1);
	}
	if (UButton* Row2 = Cast<UButton>(mMonsterTabWidget->GetWidgetFromName(TEXT("MonsterRowButton_2"))))
	{
		Row2->OnClicked.AddDynamic(this, &UCombatLayoutHUDWidget::HandleMonsterTabRowClicked_2);
	}
	if (UButton* BackButton = Cast<UButton>(
		mMonsterTabWidget->GetWidgetFromName(TEXT("MonsterBackButton"))))
	{
		BackButton->OnClicked.AddDynamic(
			this, &UCombatLayoutHUDWidget::HandleMonsterTabBackClicked);
	}
	return true;
}

void UCombatLayoutHUDWidget::RefreshMonsterTab()
{
	if (mMonsterTabWidget == nullptr || mUIModel == nullptr)
	{
		return;
	}

	// 살아 있는 적을 나온 차례대로 행에 채운다. 클릭 행→유닛 매핑도 이 배열
	// 하나로 센다 -- RefreshParty()와 같은 원칙이다.
	mMonsterTabUnitIds.Reset();
	TArray<const FUnitUI*> Monsters;
	for (const FUnitUI& Unit : mUIModel->GetUnitUIs())
	{
		if (Unit.mIsPlayer == true || Unit.mHP <= 0.f)
		{
			continue;
		}
		if (Monsters.Num() >= 3)
		{
			break;
		}
		Monsters.Add(&Unit);
		mMonsterTabUnitIds.Add(Unit.mUnitId);
	}
	if (mMonsterTabSelectedRow >= Monsters.Num())
	{
		mMonsterTabSelectedRow = FMath::Max(0, Monsters.Num() - 1);
	}

	for (int32 Index = 0; Index < 3; ++Index)
	{
		UWidget* Row = mMonsterTabWidget->GetWidgetFromName(
			FName(*FString::Printf(TEXT("MonsterRow_%d"), Index)));
		const bool bHasMonster = Monsters.IsValidIndex(Index);
		if (Row != nullptr)
		{
			Row->SetVisibility(bHasMonster
				? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
		if (bHasMonster == false)
		{
			continue;
		}
		if (UWidget* Selected = mMonsterTabWidget->GetWidgetFromName(
			FName(*FString::Printf(TEXT("MonsterRowSelected_%d"), Index))))
		{
			Selected->SetVisibility(Index == mMonsterTabSelectedRow
				? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
		if (UImage* Portrait = Cast<UImage>(mMonsterTabWidget->GetWidgetFromName(
			FName(*FString::Printf(TEXT("MonsterRowPortrait_%d"), Index)))))
		{
			UTexture2D* Head = Monsters[Index]->mTurnPortrait != nullptr
				? Monsters[Index]->mTurnPortrait.Get() : Monsters[Index]->mPortrait.Get();
			if (Head != nullptr)
			{
				Portrait->SetBrushFromTexture(Head);
			}
		}
		if (UTextBlock* Name = Cast<UTextBlock>(mMonsterTabWidget->GetWidgetFromName(
			FName(*FString::Printf(TEXT("MonsterRowName_%d"), Index)))))
		{
			Name->SetText(Monsters[Index]->mName);
		}
	}

	if (Monsters.IsValidIndex(mMonsterTabSelectedRow) == false)
	{
		return;
	}
	const FUnitUI& Monster = *Monsters[mMonsterTabSelectedRow];

	const auto SetTabText = [this](const TCHAR* Name, const FText& Value)
	{
		if (UTextBlock* Text = Cast<UTextBlock>(mMonsterTabWidget->GetWidgetFromName(Name)))
		{
			Text->SetText(Value);
		}
	};
	SetTabText(TEXT("MonsterCenterNameText"), Monster.mName);
	SetTabText(TEXT("MonsterDetailNameText"), Monster.mName);
	// 종족/공격 유형은 아직 내려오는 데이터가 없다. 자리 문구를 지운다.
	if (UWidget* TypeText = mMonsterTabWidget->GetWidgetFromName(TEXT("MonsterDetailTypeText")))
	{
		TypeText->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (UProgressBar* HPBar = Cast<UProgressBar>(
		mMonsterTabWidget->GetWidgetFromName(TEXT("MonsterDetailHPBar"))))
	{
		HPBar->SetPercent(Monster.mMaxHP > 0.f ? Monster.mHP / Monster.mMaxHP : 0.f);
	}
	SetTabText(TEXT("MonsterDetailHPText"), FText::FromString(FString::Printf(
		TEXT("%d / %d"), FMath::RoundToInt(Monster.mHP), FMath::RoundToInt(Monster.mMaxHP))));
	SetTabText(TEXT("MonsterDetailAPText"), FText::FromString(FString::Printf(
		TEXT("AP  %d"), Monster.mActionPoints)));
	SetTabText(TEXT("MonsterDetailSpeedText"), FText::FromString(FString::Printf(
		TEXT("속도  %d"), FMath::RoundToInt(Monster.mSpeedPoint))));
	if (UImage* DetailPortrait = Cast<UImage>(
		mMonsterTabWidget->GetWidgetFromName(TEXT("MonsterDetailPortrait"))))
	{
		UTexture2D* Body = Monster.mPortrait != nullptr
			? Monster.mPortrait.Get() : Monster.mTurnPortrait.Get();
		if (Body != nullptr)
		{
			DetailPortrait->SetBrushFromTexture(Body);
		}
	}

	// 상태이상 두 칸: 실제 걸린 상태만 이름+스택으로 보여 주고 남는 칸은 접는다.
	for (int32 Index = 0; Index < 2; ++Index)
	{
		UTextBlock* StatusText = Cast<UTextBlock>(mMonsterTabWidget->GetWidgetFromName(
			FName(*FString::Printf(TEXT("MonsterStatusText_%d"), Index))));
		if (StatusText == nullptr)
		{
			continue;
		}
		if (Monster.mStatusEffects.IsValidIndex(Index) == true)
		{
			const FStatusEffectUI& Status = Monster.mStatusEffects[Index];
			StatusText->SetText(FText::FromString(Status.mStackCount > 1
				? FString::Printf(TEXT("%s x%d"), *StatusDisplayName(Status.mTag), Status.mStackCount)
				: StatusDisplayName(Status.mTag)));
			StatusText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			StatusText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// 스킬 이름은 목록 DTO에 없다. 상세 응답(FUnitDetailUI)이 도착하면
	// RefreshMonsterTabDetail()이 채운다. 같은 몬스터면 다시 청하지 않는다.
	if (mMonsterTabInspectedUnitId != Monster.mUnitId)
	{
		mMonsterTabInspectedUnitId = Monster.mUnitId;
		mUIModel->RequestInspectUnit(Monster.mUnitId);
	}
}

void UCombatLayoutHUDWidget::RefreshMonsterTabDetail()
{
	if (mMonsterTabWidget == nullptr || mUIModel == nullptr)
	{
		return;
	}
	const FUnitDetailUI& Detail = mUIModel->GetUnitDetail();
	// 늦게 도착한 다른 유닛의 상세로 현재 선택을 덮어쓰지 않는다.
	if (Detail.mUnitId == INDEX_NONE || Detail.mUnitId != mMonsterTabInspectedUnitId)
	{
		return;
	}

	for (int32 Index = 0; Index < 4; ++Index)
	{
		UTextBlock* SkillName = Cast<UTextBlock>(mMonsterTabWidget->GetWidgetFromName(
			FName(*FString::Printf(TEXT("MonsterSkillName_%d"), Index))));
		UWidget* SkillBox = mMonsterTabWidget->GetWidgetFromName(
			FName(*FString::Printf(TEXT("MonsterSkillBox_%d"), Index)));
		const bool bHasSkill = Detail.mSkills.IsValidIndex(Index);
		if (SkillName != nullptr)
		{
			if (bHasSkill == true)
			{
				SkillName->SetText(Detail.mSkills[Index].mName);
			}
			SkillName->SetVisibility(bHasSkill
				? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
		if (SkillBox != nullptr)
		{
			SkillBox->SetVisibility(bHasSkill
				? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
	if (Detail.mPortrait != nullptr)
	{
		if (UImage* DetailPortrait = Cast<UImage>(
			mMonsterTabWidget->GetWidgetFromName(TEXT("MonsterDetailPortrait"))))
		{
			DetailPortrait->SetBrushFromTexture(Detail.mPortrait);
		}
	}
}
void UCombatLayoutHUDWidget::HandleMercenaryCloseClicked()
{
	SetMercenaryPanelShown(false);
}

/**
 * @brief 오른쪽 아래 단추를 눌렀다.
 *
 * @details
 * 이 단추는 두 일을 한다. 무르는 중이면 '취소' 이고, 아니면 '턴 종료' 다.
 *
 * **글자만 바꾸고 하는 일은 안 바꿨었다.** 취소라고 적힌 것을 눌렀는데 턴이
 * 넘어갔다 -- 무르려다 차례를 날리는 것이라 되돌릴 길이 없다.
 */
void UCombatLayoutHUDWidget::HandleEndTurnClicked()
{
	if (mUIModel == nullptr)
	{
		return;
	}

	// 어느 쪽이든 상세를 볼 시간은 끝났다. 위협 범위 칠은 명령에서 걷는다.
	HideDetailOverlay(/*bNotifyGameplay=*/false);

	if (mUIModel->GetTurnUI().mPhase != ECombatBuildPhaseUI::None)
	{
		mUIModel->RequestCancel();
		return;
	}
	mUIModel->RequestEndTurn();
}

/* ── 상세 패널 (롱프레스 정보) ─────────────────────────────────────── */

namespace
{
	/** @brief 조준 형태의 표시 이름. 계약 enum 을 화면 글자로 바꾸는 것은 UI 소관이다. */
	const TCHAR* SelectShapeName(const ECombatSkillSelectShapeUI Shape)
	{
		switch (Shape)
		{
		case ECombatSkillSelectShapeUI::Single:   return TEXT("단일");
		case ECombatSkillSelectShapeUI::Square:   return TEXT("사각");
		case ECombatSkillSelectShapeUI::Cross:    return TEXT("십자");
		case ECombatSkillSelectShapeUI::Diagonal: return TEXT("대각");
		case ECombatSkillSelectShapeUI::Line:     return TEXT("직선");
		default:                                  return TEXT("");
		}
	}

	const TCHAR* HitShapeName(const ECombatSkillHitShapeUI Shape)
	{
		switch (Shape)
		{
		case ECombatSkillHitShapeUI::Single: return TEXT("단일");
		case ECombatSkillHitShapeUI::Cross:  return TEXT("십자");
		case ECombatSkillHitShapeUI::Circle: return TEXT("원형");
		default:                             return TEXT("");
		}
	}

	/** @brief 사거리/타격범위/곡사·관통을 한 줄로 요약한다. 없는 항목은 뺀다. */
	FString DescribeTargeting(const FSkillTargetingUI& Targeting)
	{
		TArray<FString> Parts;
		if (Targeting.mSelectShape != ECombatSkillSelectShapeUI::None)
		{
			Parts.Add(FString::Printf(TEXT("사거리 %.0f (%s)"),
				Targeting.mSelectRange, SelectShapeName(Targeting.mSelectShape)));
		}
		if (Targeting.mHitShape != ECombatSkillHitShapeUI::None)
		{
			Parts.Add(FString::Printf(TEXT("타격 %s %.0f"),
				HitShapeName(Targeting.mHitShape), Targeting.mHitRange));
		}
		if (Targeting.mIsIndirect == true)
		{
			Parts.Add(TEXT("곡사"));
		}
		if (Targeting.mIsPenetration == true)
		{
			Parts.Add(TEXT("관통"));
		}
		return FString::Join(Parts, TEXT("  ·  "));
	}
}

/**
 * @brief 상세 패널 위젯을 처음 한 번 만들어 화면에 얹는다.
 *
 * @details
 * WBP_CombatDetailOverlay 는 배치와 그림(판·틀·글꼴)을 소유하고, 내용 글자는
 * 여기서 이름으로 찾은 위젯에 채운다 -- HUD 본체와 같은 규약이라 없는 위젯은
 * 조용히 건너뛴다.
 *
 * 겹은 HitTestInvisible 로 얹는다. 눌림을 받으면 화면 전체를 덮는 한 장이
 * 되어 판 탭이 HUD 까지 안 내려온다 -- 닫는 탭을 받을 사람이 사라진다.
 */
bool UCombatLayoutHUDWidget::EnsureDetailOverlayWidget()
{
	if (mDetailOverlayWidget != nullptr)
	{
		return true;
	}
	if (mDetailOverlayWidgetClass == nullptr)
	{
		return false;
	}
	if (APlayerController* OwningPlayer = GetOwningPlayer())
	{
		mDetailOverlayWidget =
			CreateWidget<UUserWidget>(OwningPlayer, mDetailOverlayWidgetClass);
	}
	else if (UWorld* World = GetWorld())
	{
		// 에디터 자동화처럼 로컬 플레이어가 없는 월드에서도 상세 계약은
		// 검증할 수 있어야 한다. 실제 플레이에서는 늘 위의 소유 플레이어를 쓴다.
		mDetailOverlayWidget =
			CreateWidget<UUserWidget>(World, mDetailOverlayWidgetClass);
	}
	if (mDetailOverlayWidget == nullptr)
	{
		return false;
	}
	mDetailOverlayWidget->AddToViewport(/*ZOrder=*/50);
	mDetailOverlayWidget->SetVisibility(ESlateVisibility::Collapsed);

	mDetailIconImage = Cast<UImage>(mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailIconImage")));
	mDetailTitleText = Cast<UTextBlock>(mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailTitleText")));
	mDetailSubtitleText = Cast<UTextBlock>(mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailSubtitleText")));
	mDetailBodyText = Cast<UTextBlock>(mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailBodyText")));

	/*
	 * 판·틀·글자는 눌림을 **삼키지 않게** 해 둔다.
	 *
	 * 스킬 칸이 생기면서 이 겹은 눌림을 받아야 하는데, 그러면 장식까지 눌림을
	 * 먹어서 패널 위를 톡 쳐도 닫히지 않는다. 눌림을 받을 것은 스킬 칸뿐이다.
	 */
	static const TCHAR* const DecorativeNames[] = {
		TEXT("DetailScrimImage"), TEXT("DetailFrameImage"), TEXT("DetailIconImage"),
		TEXT("DetailTitleText"), TEXT("DetailSubtitleText"), TEXT("DetailBodyText") };
	for (const TCHAR* Name : DecorativeNames)
	{
		if (UWidget* Decoration = mDetailOverlayWidget->GetWidgetFromName(Name))
		{
			Decoration->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
	if (UWidget* PanelRoot = mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailPanelRoot")))
	{
		// 자기는 안 받고 자식(스킬 칸)만 받는다.
		PanelRoot->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	BuildDetailSkillRow();
	return true;
}

/**
 * @brief 상세 패널 안에 스킬 칸 줄을 짓는다.
 *
 * @details
 * 패널 묶음(DetailPanelRoot)이 캔버스면 그 아래쪽에 앵커로 붙인다. 캔버스가
 * 아니면 위젯 뿌리 캔버스에 붙여서라도 보이게 한다 -- 자리는 어긋나도 정보에
 * 닿을 길은 남긴다.
 */
void UCombatLayoutHUDWidget::BuildDetailSkillRow()
{
	if (mDetailSkillRow != nullptr || mDetailOverlayWidget == nullptr)
	{
		return;
	}
	UWidgetTree* Tree = mDetailOverlayWidget->WidgetTree;
	if (Tree == nullptr)
	{
		return;
	}

	UCanvasPanel* Host = Cast<UCanvasPanel>(mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailPanelRoot")));
	if (Host == nullptr)
	{
		Host = Cast<UCanvasPanel>(Tree->RootWidget);
	}
	if (Host == nullptr)
	{
		return;
	}

	mDetailSkillRow = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UCanvasPanelSlot* RowSlot = Host->AddChildToCanvas(mDetailSkillRow);
	if (RowSlot != nullptr)
	{
		// 본문 글자 아래 빈 자리에 앉힌다. 비율로 잡아 두면 패널 크기가 바뀌어도 따라간다.
		RowSlot->SetAnchors(FAnchors(0.10f, 0.66f, 0.90f, 0.90f));
		RowSlot->SetOffsets(FMargin(0.f));
		RowSlot->SetAlignment(FVector2D(0.f, 0.f));
	}

	static const TCHAR* const HandlerNames[DetailSkillSlotCount] = {
		TEXT("HandleDetailSkillClicked_0"), TEXT("HandleDetailSkillClicked_1"),
		TEXT("HandleDetailSkillClicked_2"), TEXT("HandleDetailSkillClicked_3"),
		TEXT("HandleDetailSkillClicked_4"), TEXT("HandleDetailSkillClicked_5") };
	void (UCombatLayoutHUDWidget::* const Handlers[DetailSkillSlotCount])() = {
		&UCombatLayoutHUDWidget::HandleDetailSkillClicked_0,
		&UCombatLayoutHUDWidget::HandleDetailSkillClicked_1,
		&UCombatLayoutHUDWidget::HandleDetailSkillClicked_2,
		&UCombatLayoutHUDWidget::HandleDetailSkillClicked_3,
		&UCombatLayoutHUDWidget::HandleDetailSkillClicked_4,
		&UCombatLayoutHUDWidget::HandleDetailSkillClicked_5 };

	mDetailSkillButtons.Reset();
	mDetailSkillIcons.Reset();
	mDetailSkillLabels.Reset();
	mDetailSkillIndices.Init(INDEX_NONE, DetailSkillSlotCount);

	for (int32 Index = 0; Index < DetailSkillSlotCount; ++Index)
	{
		UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass());
		if (UHorizontalBoxSlot* ButtonSlot = mDetailSkillRow->AddChildToHorizontalBox(Button))
		{
			ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			ButtonSlot->SetPadding(FMargin(6.f, 0.f));
		}

		// 그림과 글자를 겹쳐 둔다. 아이콘이 있으면 그림, 없으면 이름을 보여 준다.
		UOverlay* Content = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		Button->AddChild(Content);

		UImage* Icon = Tree->ConstructWidget<UImage>(UImage::StaticClass());
		if (UOverlaySlot* IconSlot = Cast<UOverlaySlot>(Content->AddChild(Icon)))
		{
			IconSlot->SetHorizontalAlignment(HAlign_Fill);
			IconSlot->SetVerticalAlignment(VAlign_Fill);
		}
		Icon->SetVisibility(ESlateVisibility::HitTestInvisible);

		UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		if (UOverlaySlot* LabelSlot = Cast<UOverlaySlot>(Content->AddChild(Label)))
		{
			LabelSlot->SetHorizontalAlignment(HAlign_Center);
			LabelSlot->SetVerticalAlignment(VAlign_Center);
		}
		Label->SetJustification(ETextJustify::Center);
		Label->SetAutoWrapText(true);
		Label->SetVisibility(ESlateVisibility::Collapsed);

		Button->OnClicked.__Internal_AddUniqueDynamic(this, Handlers[Index], HandlerNames[Index]);
		Button->SetVisibility(ESlateVisibility::Collapsed);

		mDetailSkillButtons.Add(Button);
		mDetailSkillIcons.Add(Icon);
		mDetailSkillLabels.Add(Label);
	}
}

/** @brief 상세 스냅샷의 스킬로 칸을 채운다. 남는 칸은 접어 자리를 안 차지하게 한다. */
void UCombatLayoutHUDWidget::RefreshDetailSkillRow()
{
	if (mUIModel == nullptr || mDetailSkillRow == nullptr)
	{
		return;
	}
	const TArray<FUnitDetailSkillUI>& Skills = mUIModel->GetUnitDetail().mSkills;

	for (int32 Index = 0; Index < mDetailSkillButtons.Num(); ++Index)
	{
		const bool bHasSkill = Skills.IsValidIndex(Index);
		mDetailSkillIndices[Index] = bHasSkill ? Skills[Index].mSkillIndex : INDEX_NONE;

		UButton* Button = mDetailSkillButtons[Index];
		if (Button != nullptr)
		{
			Button->SetVisibility(bHasSkill ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
		if (bHasSkill == false)
		{
			continue;
		}

		UTexture2D* IconTexture = Skills[Index].mIcon;
		if (UImage* Icon = mDetailSkillIcons[Index])
		{
			SetShown(Icon, IconTexture != nullptr);
			if (IconTexture != nullptr)
			{
				Icon->SetBrushFromTexture(IconTexture, false);
			}
		}
		// 그림이 없으면 이름으로 대신한다. 빈 칸만 늘어놓으면 무엇인지 알 수 없다.
		if (UTextBlock* Label = mDetailSkillLabels[Index])
		{
			SetShown(Label, IconTexture == nullptr);
			Label->SetText(Skills[Index].mName);
		}
	}
	SetShown(mDetailSkillRow, Skills.Num() > 0);
}

void UCombatLayoutHUDWidget::SetDetailSkillRowShown(const bool bShown)
{
	if (mDetailSkillRow != nullptr)
	{
		SetShown(mDetailSkillRow, bShown);
	}
}

/**
 * @brief 상세창의 스킬 칸을 탭했다. 그 스킬 상세를 청한다.
 *
 * 기준 유닛은 보내지 않는다 -- 상세를 내려 준 게임플레이가 기억하고 있다.
 */
void UCombatLayoutHUDWidget::HandleDetailSkillClicked(const int32 IconIndex)
{
	if (mUIModel == nullptr || mDetailSkillIndices.IsValidIndex(IconIndex) == false)
	{
		return;
	}
	const int32 SkillIndex = mDetailSkillIndices[IconIndex];
	if (SkillIndex == INDEX_NONE)
	{
		return;
	}
	mUIModel->RequestInspectUnitSkill(SkillIndex);
}

void UCombatLayoutHUDWidget::HandleDetailSkillClicked_0() { HandleDetailSkillClicked(0); }
void UCombatLayoutHUDWidget::HandleDetailSkillClicked_1() { HandleDetailSkillClicked(1); }
void UCombatLayoutHUDWidget::HandleDetailSkillClicked_2() { HandleDetailSkillClicked(2); }
void UCombatLayoutHUDWidget::HandleDetailSkillClicked_3() { HandleDetailSkillClicked(3); }
void UCombatLayoutHUDWidget::HandleDetailSkillClicked_4() { HandleDetailSkillClicked(4); }
void UCombatLayoutHUDWidget::HandleDetailSkillClicked_5() { HandleDetailSkillClicked(5); }

/**
 * @brief 유닛 상세를 패널에 채워 띄운다.
 *
 * @details
 * 라이브 스탯(HP·방어)은 상세 스냅샷에 없다 -- 진실원본 이원화를 막으려고
 * mUnitId 로 유닛 목록에서 읽는 것이 계약이다(FUnitDetailUI 주석).
 */
void UCombatLayoutHUDWidget::ShowUnitDetailOverlay()
{
	if (mUIModel == nullptr || EnsureDetailOverlayWidget() == false)
	{
		return;
	}
	const FUnitDetailUI& Detail = mUIModel->GetUnitDetail();
	if (Detail.mUnitId == INDEX_NONE)
	{
		return;
	}

	SetTextIfPresent(mDetailTitleText, Detail.mName);

	const FUnitUI* Unit = nullptr;
	for (const FUnitUI& Candidate : mUIModel->GetUnitUIs())
	{
		if (Candidate.mUnitId == Detail.mUnitId)
		{
			Unit = &Candidate;
			break;
		}
	}

	FString Subtitle = FString::Printf(TEXT("Lv.%d"), Detail.mLevel);
	if (Unit != nullptr)
	{
		Subtitle += Unit->mIsPlayer == true ? TEXT("  ·  아군") : TEXT("  ·  적");
		Subtitle += FString::Printf(TEXT("  ·  HP %.0f/%.0f"), Unit->mHP, Unit->mMaxHP);
		if (Unit->mDefensePoint > 0.f)
		{
			Subtitle += FString::Printf(TEXT("  ·  방어 %.0f"), Unit->mDefensePoint);
		}
	}
	SetTextIfPresent(mDetailSubtitleText, FText::FromString(Subtitle));

	FString Body;
	for (const FText& Passive : Detail.mPassiveDescriptions)
	{
		if (Body.IsEmpty() == false)
		{
			Body += TEXT("\n");
		}
		Body += TEXT("· ");
		Body += Passive.ToString();
	}
	if (Body.IsEmpty() == true)
	{
		Body = TEXT("패시브 없음");
	}
	if (Unit != nullptr && Unit->mIsPlayer == false)
	{
		// 판에 함께 칠린 위협 범위의 범례다. 칠만 있고 뜻을 알려 주는 곳이
		// 없으면 밴드와 채움을 구분할 길이 없다.
		Body += TEXT("\n\n판의 표시는 이 적이 한 턴에 닿는 곳이다.\n테두리 밴드 = 이동 범위, 채움 = 공격 범위.");
	}
	SetTextIfPresent(mDetailBodyText, FText::FromString(Body));

	SetPortraitCropped(mDetailIconImage, Detail.mPortrait);
	RefreshDetailSkillRow();
	// 자기는 눌림을 안 받고 스킬 칸만 받는다. 그래서 칸 밖을 톡 치면 눌림이
	// HUD 까지 내려가 패널이 닫힌다.
	mDetailOverlayWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

/** @brief 스킬 상세를 패널에 채워 띄운다. 비용/쿨타임은 카드 레일 스냅샷에서 읽는다. */
void UCombatLayoutHUDWidget::ShowSkillDetailOverlay()
{
	if (mUIModel == nullptr || EnsureDetailOverlayWidget() == false)
	{
		return;
	}
	const FSkillDetailUI& Detail = mUIModel->GetSkillDetail();
	if (Detail.mSkillIndex == INDEX_NONE)
	{
		return;
	}

	SetTextIfPresent(mDetailTitleText, Detail.mName);

	FString Subtitle;
	const TArray<FSkillUI>& Skills = mUIModel->GetSkillUIs();
	if (Skills.IsValidIndex(Detail.mSkillIndex) == true)
	{
		const FSkillUI& Skill = Skills[Detail.mSkillIndex];
		Subtitle = FString::Printf(TEXT("AP %d"), Skill.mActionPointCost);
		if (Skill.mCooldownTurns > 0)
		{
			Subtitle += FString::Printf(TEXT("  ·  쿨타임 %d턴"), Skill.mCooldownTurns);
		}
		if (Skill.mDamageMax > 0)
		{
			Subtitle += Skill.mDamageMin == Skill.mDamageMax
				? FString::Printf(TEXT("  ·  피해 %d"), Skill.mDamageMax)
				: FString::Printf(TEXT("  ·  피해 %d~%d"), Skill.mDamageMin, Skill.mDamageMax);
		}
	}
	SetTextIfPresent(mDetailSubtitleText, FText::FromString(Subtitle));

	FString Body = Detail.mDescription.ToString();
	const FString Targeting = DescribeTargeting(Detail.mTargeting);
	if (Targeting.IsEmpty() == false)
	{
		if (Body.IsEmpty() == false)
		{
			Body += TEXT("\n\n");
		}
		Body += Targeting;
	}
	SetTextIfPresent(mDetailBodyText, FText::FromString(Body));

	SetPortraitCropped(mDetailIconImage, Detail.mIcon);
	// 이 칸들은 유닛 상세의 것이다. 스킬 하나를 보는 중에는 걷는다.
	SetDetailSkillRowShown(false);
	mDetailOverlayWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

/**
 * @brief 이동 카드의 상세를 띄운다.
 *
 * @details
 * 남은 행동력이 곧 갈 수 있는 칸 수다(칸당 1). 스킬처럼 사거리나 쿨타임이
 * 없는 대신, 경유지로 길을 접는 조작법이 이동에만 있어서 그것을 적는다 --
 * 판을 여러 번 찍어 돌아가는 길을 만들 수 있다는 것을 아는 사람이 없었다.
 */
void UCombatLayoutHUDWidget::ShowMoveDetailOverlay()
{
	if (mUIModel == nullptr || EnsureDetailOverlayWidget() == false)
	{
		return;
	}

	const FUnitUI* TurnUnit = FindTurnUnit();
	const int32 Left = TurnUnit != nullptr
		? FMath::Max(FMath::RoundToInt(TurnUnit->mMovementPoint), 0) : 0;

	SetTextIfPresent(mDetailTitleText, LOCTEXT("MoveDetailTitle", "이동"));
	SetTextIfPresent(mDetailSubtitleText, FText::FromString(
		FString::Printf(TEXT("AP 1/칸  ·  남은 AP %d"), Left)));

	FString Body = FString::Printf(
		TEXT("한 칸 옮길 때마다 행동력을 1 쓴다.\n지금 남은 행동력으로 최대 %d칸 갈 수 있다."), Left);
	Body += TEXT("\n\n판을 톡 쳐서 갈 곳을 고른다. 여러 번 찍으면 그 자리가 경유지가 되어")
		TEXT(" 찍은 차례대로 돌아간다.\n마지막으로 찍은 칸을 다시 누르면 확정하고,")
		TEXT(" 판 밖을 누르면 마지막 경유지만 무른다.");
	SetTextIfPresent(mDetailBodyText, FText::FromString(Body));

	// 카드에 그려 둔 그림을 그대로 쓴다. 이동은 상세용 그림이 따로 없다.
	UTexture2D* MoveIcon = nullptr;
	if (mCommandSlots.IsValidIndex(0) && mCommandSlots[0].Icon != nullptr)
	{
		MoveIcon = Cast<UTexture2D>(mCommandSlots[0].Icon->GetBrush().GetResourceObject());
	}
	SetPortraitCropped(mDetailIconImage, MoveIcon);

	// 스킬 칸은 유닛 상세의 것이다.
	SetDetailSkillRowShown(false);
	mDetailOverlayWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UCombatLayoutHUDWidget::HideDetailOverlay(const bool bNotifyGameplay)
{
	if (IsDetailOverlayShown() == false)
	{
		return;
	}
	mDetailOverlayWidget->SetVisibility(ESlateVisibility::Collapsed);

	// 패널과 함께 칠린 위협 범위를 걷으라는 신호다. INDEX_NONE = "닫았다".
	// 칠을 걷는 것은 판(게임플레이) 소관이라 화면이 직접 지우지 않는다.
	if (bNotifyGameplay == true && mUIModel != nullptr)
	{
		mUIModel->RequestLongPressUnit(INDEX_NONE);
	}
}

bool UCombatLayoutHUDWidget::IsDetailOverlayShown() const
{
	return mDetailOverlayWidget != nullptr
		&& mDetailOverlayWidget->GetVisibility() != ESlateVisibility::Collapsed;
}

#undef LOCTEXT_NAMESPACE
