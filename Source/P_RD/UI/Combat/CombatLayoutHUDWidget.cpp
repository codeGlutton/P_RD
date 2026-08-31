#include "UI/Combat/CombatLayoutHUDWidget.h"

#include "Actor/TileMap/TileLayer.h"
#include "Singleton/WorldSubsystem/WorldWidgetType.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/TextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraActor.h"
#include "Engine/Font.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EngineUtils.h"
#include "FunctionLibrary/CameraFunctionLibrary.h"
#include "Internationalization/Internationalization.h"
#include "Pawn/Camera/CombatCameraPawn.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/Combat/SimulationPreviewUIModel.h"
#include "UI/Combat/SkillDetailOverlayPresenter.h"
#include "UI/Combat/SkillTacticalDiagramWidget.h"
#include "UI/DetailOverlayInputShield.h"
#include "UI/Combat/SkillCutInWidget.h"
#include "UI/SettingsPanelWidget.h"
#include "UI/Combat/MockCombatDriver.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "UI/Reward/RewardConcept03Widget.h"
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
	// 명단 셸 그림은 지웠다(새 그림 예정). 비워 두면 EnsureMercenaryRosterShell()
	// 이 그림 없이 자리만 잡는다 -- LoadSynchronous() 가 null 을 주고 넘어간다.
	// 확정 시안(0806): 목록 줄은 고용 화면과 같은 양피지 줄판을 쓴다.
	// 옛 남색 카드(T_MB_MercenaryCard_*)는 세로 카드용이라 가로 줄에 안 맞았다.
	mMercenaryCardNormalTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Hire/T_MB_HireRowNormal.T_MB_HireRowNormal")));
	mMercenaryCardSelectedTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Hire/T_MB_HireRowSelected.T_MB_HireRowSelected")));

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
	RD_LOAD_TEX(mUnitHpGlowTexture,        "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/UnitHpBar/T_CombatHUD_UnitHpBar_FrameGlow.T_CombatHUD_UnitHpBar_FrameGlow");
	RD_LOAD_TEX(mUnitStatusSlotTexture,    "/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_StatusSlot_Frame.T_MB_StatusSlot_Frame");
	RD_LOAD_TEX(mCommandMoveIconTexture, "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/StatusIcons/T_Status_GetMove.T_Status_GetMove");
	RD_LOAD_TEX(mLogIconHpDamage,      "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/StatusIcons/T_Status_HP_Damage.T_Status_HP_Damage");
	RD_LOAD_TEX(mLogIconHpRecovery,    "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/StatusIcons/T_Status_HP_Recovery.T_Status_HP_Recovery");
	RD_LOAD_TEX(mLogIconGetMove,       "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/StatusIcons/T_Status_GetMove.T_Status_GetMove");
	RD_LOAD_TEX(mLogIconGetDefense,    "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/StatusIcons/T_Status_GetDefense.T_Status_GetDefense");
	RD_LOAD_TEX(mLogIconVigor,       "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/StatusIcons/T_Status_Agility.T_Status_Agility");
	RD_LOAD_TEX(mLogIconFortification, "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/StatusIcons/T_Status_Fortification.T_Status_Fortification");
	RD_LOAD_TEX(mLogIconVulnerability, "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/StatusIcons/T_Status_Vulnerability.T_Status_Vulnerability");
	RD_LOAD_TEX(mLogIconWeakness,      "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/StatusIcons/T_Status_Weakness.T_Status_Weakness");
	// 플로팅 로그 글꼴. 새로 만든 TextBlock 은 엔진 기본 Roboto 로 남으므로
	// HUD 공용 숫자 글꼴(F_HUD_Oswald)을 여기서 물어 둔다(UIFont::ProjectFont 짝).
	{
		static ConstructorHelpers::FObjectFinder<UFont> FloatingLogFontFinder(
			TEXT("/Game/SVN/OutSideAsset/Fonts/F_HUD_Oswald.F_HUD_Oswald"));
		if (FloatingLogFontFinder.Succeeded())
		{
			mFloatingLogFont = FloatingLogFontFinder.Object;
		}
	}
	RD_LOAD_TEX(mSkillVisualRingTexture, "/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_StatChip_Ring.T_KitA_StatChip_Ring");
	RD_LOAD_TEX(mSkillVisualCellNormalTexture, "/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_Cell_Disabled.T_KitA_Cell_Disabled");
	RD_LOAD_TEX(mSkillVisualCellSelectedTexture, "/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_Cell_Selected.T_KitA_Cell_Selected");
	// 스킬 상세 수치는 별도 컨셉 아이콘을 만들지 않고 전투 HUD가 이미 쓰는
	// 브러시를 그대로 쓴다. AP는 커맨드 카드의 파란 비용 보석, 피해는 전투
	// 피드백의 HP 피해 표식이다. 프레젠터가 ResolveDetailStatTexture()로 HUD
	// 인스턴스의 실제 브러시를 한 번 더 우선 조회하므로 WBP 쪽 교체도 자동으로 따라간다.
	RD_LOAD_TEX(mSkillVisualAPIconTexture, "/Game/SVN/OutSideAsset/AICreation/UI/HUD04/KK_HUD04_zone_cost_badge.KK_HUD04_zone_cost_badge");
	RD_LOAD_TEX(mSkillVisualDamageIconTexture, "/Game/SVN/OutSideAsset/AICreation/UI/CombatDetail/SkillTactical/T_SkillStat_Damage_Simple_v2.T_SkillStat_Damage_Simple_v2");
	RD_LOAD_TEX(mSkillVisualCooldownIconTexture, "/Game/SVN/OutSideAsset/AICreation/UI/HUD04/KK_HUD04_zone_cooldown_badge.KK_HUD04_zone_cooldown_badge");
	RD_LOAD_TEX(mSkillVisualCriticalIconTexture, "/Game/SVN/OutSideAsset/AICreation/UI/CombatDetail/SkillTactical/T_SkillStat_Critical_Clear_v1.T_SkillStat_Critical_Clear_v1");
	RD_LOAD_TEX(mSkillVisualCasterIconTexture, "/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_OptionsIcon_MercenaryGlyph.T_MB_OptionsIcon_MercenaryGlyph");
	RD_LOAD_TEX(mSkillVisualTargetIconTexture, "/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_OptionsIcon_MonsterGlyph.T_MB_OptionsIcon_MonsterGlyph");
	RD_LOAD_TEX(mSkillRangeButtonTexture, "/Game/SVN/OutSideAsset/AICreation/UI/CombatDetail/SkillTactical/T_SkillRangeButton_Normal_v1.T_SkillRangeButton_Normal_v1");
	RD_LOAD_TEX(mSkillRangeButtonSelectedTexture, "/Game/SVN/OutSideAsset/AICreation/UI/CombatDetail/SkillTactical/T_SkillRangeButton_Selected_v1.T_SkillRangeButton_Selected_v1");
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
	static ConstructorHelpers::FObjectFinder<UFont> ReadableDetailFontFinder(
		TEXT("/Game/SVN/OutSideAsset/Fonts/GowunBatang/F_GowunBatang.F_GowunBatang"));
	if (ReadableDetailFontFinder.Succeeded())
	{
		mReadableDetailFont = ReadableDetailFontFinder.Object;
	}
	static ConstructorHelpers::FClassFinder<USkillTacticalDiagramWidget>
		SkillTacticalDiagramClassFinder(
			TEXT("/Game/UI/CombatDetail/SkillTactical/WBP_SkillTacticalDiagram"));
	if (SkillTacticalDiagramClassFinder.Succeeded())
	{
		mSkillTacticalDiagramWidgetClass =
			SkillTacticalDiagramClassFinder.Class;
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
		TEXT("/Game/UI/WorldMapLandscape/WBP_FrontendMapLandscape"));
	if (WorldMapClassFinder.Succeeded())
	{
		mWorldMapWidgetClass = WorldMapClassFinder.Class;
	}

#define RD_LOAD_SOUND(Member, Path) 	{ static ConstructorHelpers::FObjectFinder<USoundBase> Finder(TEXT(Path)); if (Finder.Succeeded()) { Member = Finder.Object; } }
	RD_LOAD_SOUND(mVictoryJingleSound, "/Game/SVN/OutSideAsset/Music/OpenGameArt/Jingle/BGM_Jingle_Victory_Fupi_CC0.BGM_Jingle_Victory_Fupi_CC0");
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

	struct FProjectedTurnToken
	{
		int32 mUnitId = INDEX_NONE;
		int32 mRoundOffset = 0;
		bool mStartsRound = false;
		bool mEmptyRound = false;
	};

	/** @brief 현재 잔여 턴과 모델이 계산한 미래 라운드를 한 줄로 펼친다. */
	TArray<FProjectedTurnToken> BuildProjectedTurnTokens(const FTurnUI& Turn)
	{
		TArray<FProjectedTurnToken> Tokens;
		for (const int32 UnitId : Turn.mTurnOrderUnitIds)
		{
			FProjectedTurnToken& Token = Tokens.AddDefaulted_GetRef();
			Token.mUnitId = UnitId;
		}

		if (Turn.mPredictedRounds.IsEmpty() == false)
		{
			for (const FTurnRoundForecastUI& Round : Turn.mPredictedRounds)
			{
				bool StartsRound = true;
				for (const int32 UnitId : Round.mTurnOrderUnitIds)
				{
					FProjectedTurnToken& Token = Tokens.AddDefaulted_GetRef();
					Token.mUnitId = UnitId;
					Token.mRoundOffset = FMath::Max(Round.mRoundOffset, 1);
					Token.mStartsRound = StartsRound;
					StartsRound = false;
				}
			}
			return Tokens;
		}

		// 구형 데이터 호환. 예전 계약은 다음 턴 전의 빈 라운드도 한 칸씩 썼다.
		if (Turn.mNextRoundUnitIds.IsEmpty() == false)
		{
			const int32 NextRoundOffset = FMath::Max(Turn.mNextRoundOffset, 1);
			for (int32 RoundOffset = 1; RoundOffset < NextRoundOffset; ++RoundOffset)
			{
				FProjectedTurnToken& Token = Tokens.AddDefaulted_GetRef();
				Token.mRoundOffset = RoundOffset;
				Token.mStartsRound = true;
				Token.mEmptyRound = true;
			}
			bool StartsRound = true;
			for (const int32 UnitId : Turn.mNextRoundUnitIds)
			{
				FProjectedTurnToken& Token = Tokens.AddDefaulted_GetRef();
				Token.mUnitId = UnitId;
				Token.mRoundOffset = NextRoundOffset;
				Token.mStartsRound = StartsRound;
				StartsRound = false;
			}
		}
		return Tokens;
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
	FText StatusDisplayName(const FGameplayTag& Tag)
	{
		FString Full = Tag.GetTagName().ToString();
		if (Full.IsEmpty())
		{
			return NSLOCTEXT("CombatLayoutHUD", "StatusUnknown", "이상");
		}

		FString Leaf = Full;
		int32 Dot = INDEX_NONE;
		if (Full.FindLastChar(TEXT('.'), Dot))
		{
			Leaf = Full.Mid(Dot + 1);
		}

		static const TMap<FString, FText> Names = {
			{ TEXT("Weakness"),      NSLOCTEXT("CombatLayoutHUD", "StatusWeakness", "약화") },
			{ TEXT("Vulnerability"), NSLOCTEXT("CombatLayoutHUD", "StatusVulnerability", "취약") },
			{ TEXT("Vigor"),         NSLOCTEXT("CombatLayoutHUD", "StatusVigor", "활력") },
			{ TEXT("Fortification"), NSLOCTEXT("CombatLayoutHUD", "StatusFortification", "강화") },
			{ TEXT("Haste"),         NSLOCTEXT("CombatLayoutHUD", "StatusHaste", "신속") },
			{ TEXT("Exhaustion"),    NSLOCTEXT("CombatLayoutHUD", "StatusExhaustion", "탈진") },
			{ TEXT("Slow"),          NSLOCTEXT("CombatLayoutHUD", "StatusSlow", "둔화") },
			{ TEXT("Frail"),         NSLOCTEXT("CombatLayoutHUD", "StatusFrail", "쇠약") },
			{ TEXT("Root"),          NSLOCTEXT("CombatLayoutHUD", "StatusRoot", "속박") },
			{ TEXT("Poison"),        NSLOCTEXT("CombatLayoutHUD", "StatusPoison", "중독") },
			{ TEXT("Bleed"),         NSLOCTEXT("CombatLayoutHUD", "StatusBleed", "출혈") },
			{ TEXT("Stun"),          NSLOCTEXT("CombatLayoutHUD", "StatusStun", "기절") },
			{ TEXT("Stealth"),       NSLOCTEXT("CombatLayoutHUD", "StatusStealth", "은신") },
			{ TEXT("Dead"),          NSLOCTEXT("CombatLayoutHUD", "StatusDead", "전투불능") },
		};
		if (const FText* Found = Names.Find(Leaf))
		{
			return *Found;
		}
		return FText::FromString(Leaf);
	}
}

void UCombatLayoutHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CacheAuthoredWidgets();
	ApplyActionLabelOpticalAlignment();
	EnsureMercenarySkillButtons();
	WireCommands();
	EnsureCombatAnnouncementWidgets();
	StartPreviewIfUnbound();
	// 실제 전투에서는 모델이 이미 채워진 뒤에 붙으므로, 붙자마자 한 번 그린다.
	// BindUIModel의 All 갱신은 위젯을 찾기 전에 올 수도 있다.
	NativeOnUIRefreshed(ECombatUIDomain::All);
	// 카메라가 UI 레이아웃을 추측하지 않도록 위젯 생성 시 기본 앵커부터 등록한다.
	if (mUIModel != nullptr)
	{
		mUIModel->SetFocusScreenAnchor(ComputeCommandRingAnchor());
	}
}

void UCombatLayoutHUDWidget::NativeDestruct()
{
	// 화면이 바뀐 뒤 0.5초 타이머가 살아서 닫힌 HUD 위에 상세를 여는 일을 막는다.
	CancelStatusPress();
	CancelMonsterSkillPress();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(mCommandLongPressTimerHandle);
		World->GetTimerManager().ClearTimer(mBoardLongPressTimerHandle);
		World->GetTimerManager().ClearTimer(mCombatAnnouncementTimerHandle);
		World->GetTimerManager().ClearTimer(mCombatResultStartDelayTimerHandle);
	}
	mCombatAnnouncementPlaying = false;
	mCombatAnnouncementKind = ECombatAnnouncementKind::None;
	mCombatAnnouncementBarrier.Reset();
	// HUD가 사라진 뒤까지 모달 잠금이 Pawn에 남으면 다음 전투에서 카메라가
	// 영구 정지한다. 제거 전에 반드시 기본 입력 상태로 돌린다.
	ACombatCameraPawn* CameraPawn = nullptr;
	if (APlayerController* OwningPlayer = GetOwningPlayer())
	{
		CameraPawn = Cast<ACombatCameraPawn>(OwningPlayer->GetPawn());
	}
	if (CameraPawn == nullptr)
	{
		CameraPawn = UCameraFunctionLibrary::GetMainCameraPawn(this);
	}
	if (CameraPawn != nullptr)
	{
		CameraPawn->SetTouchGestureInputEnabled(true);
	}
	ReleaseSkillWorldPreview();
	// 상세 겹(프레젠터 소유) 정리는 여기서 하지 않는다. NativeDestruct는 Slate
	// 수명 이벤트라 위젯 렌더러(FWidgetRenderer)가 임시 트리를 버릴 때도 불리는데,
	// 그때 프레젠터를 끊으면 살아 있는 HUD의 상세 배선(전술판 버튼 등)이 소리
	// 없이 죽는다. 진짜 제거 경로인 RemoveFromParent가 맡는다.
	if (mMonsterTabWidget != nullptr)
	{
		mMonsterTabWidget->RemoveFromParent();
		mMonsterTabWidget = nullptr;
	}
	Super::NativeDestruct();
}

void UCombatLayoutHUDWidget::RemoveFromParent()
{
	// 뷰포트/레벨 정리가 HUD를 실제로 걷어낼 때만 상세 겹을 함께 걷는다.
	// (CloseUI→ApplyCloseUI→RemoveFromParent, 월드 전환의 뷰포트 위젯 정리.)
	if (mDetailPresenter != nullptr)
	{
		// 겹 제거는 소유자인 프레젠터가 한다. 비친 포인터만 함께 비운다.
		mDetailPresenter->Teardown();
	}
	mDetailOverlayWidget = nullptr;
	mSkillTacticalDiagramWidget = nullptr;
	Super::RemoveFromParent();
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

	mRoundPanel = Find<UWidget>(WidgetTree, TEXT("RoundPanel"));
	mRoundText = Find<UTextBlock>(WidgetTree, TEXT("RoundText"));
	// 0823 확정: 좌상단 ROUND 배지는 그대로 두고, 그 아래에 두 자리 숫자("01")를
	// 따로 크게 보여 준다. 숫자 칸은 빌더가 굽는다(구형 WBP 면 없을 수 있다).
	mRoundNumberText = Find<UTextBlock>(WidgetTree, TEXT("RoundNumberText"));
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
		// 현재 저작 WBP는 Badge 이름을 쓴다. 구형 Icon 이름도 받아 오래된
		// 전투 배치에서 빈 원형 배지가 다시 남지 않게 한다.
		Widgets.CooldownIcon = Find<UWidget>(
			WidgetTree, TEXT("CommandCooldownBadge") + Suffix);
		if (Widgets.CooldownIcon == nullptr)
		{
			Widgets.CooldownIcon = Find<UWidget>(
				WidgetTree, TEXT("CommandCooldownIcon") + Suffix);
		}
		Widgets.CooldownOverlayRoot = Find<UWidget>(
			WidgetTree, TEXT("CommandCooldownOverlayRoot") + Suffix);
		Widgets.CooldownOverlay = Find<UTextBlock>(
			WidgetTree, TEXT("CommandCooldownOverlay") + Suffix);
		Widgets.Damage = Find<UTextBlock>(WidgetTree, TEXT("CommandDamage") + Suffix);
		Widgets.Disabled = Find<UWidget>(WidgetTree, TEXT("CommandDisabled") + Suffix);
	}
	mTurnSlots.SetNum(TurnSlotCount);
	for (int32 Index = 0; Index < TurnSlotCount; ++Index)
	{
		FTurnSlotWidgets& Widgets = mTurnSlots[Index];
		const FString Suffix = FString::Printf(TEXT("_%d"), Index);
		Widgets.Root = Find<UWidget>(WidgetTree, TEXT("TurnToken") + Suffix);
		Widgets.Button = Find<UButton>(WidgetTree, TEXT("TurnTokenButton") + Suffix);
		Widgets.Portrait = Find<UImage>(WidgetTree, TEXT("TurnPortrait") + Suffix);
		Widgets.Name = Find<UTextBlock>(WidgetTree, TEXT("TurnName") + Suffix);
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
	mEnemyCritText = Find<UTextBlock>(WidgetTree, TEXT("EnemyCritText"));
	mEnemySpeedText = Find<UTextBlock>(WidgetTree, TEXT("EnemySpeedText"));
	mEnemyStatusText = Find<UTextBlock>(WidgetTree, TEXT("EnemyStatus"));
	mEnemyForecastText = Find<UTextBlock>(WidgetTree, TEXT("EnemyForecast"));
	mEnemyNextSkillFrame = Find<UWidget>(WidgetTree, TEXT("EnemyNextSkillFrame"));
	mEnemyNextSkillIcon = Find<UImage>(WidgetTree, TEXT("EnemyNextSkillIcon"));
	// 머리 위 월드 HP바와 같은 값을 좁은 요약판에서 반복하지 않는다.
	// 비운 한 줄만큼 AP/속도/상태를 위로 당기고, 작은 화면에서도 얼굴과
	// 상태 아이콘이 한눈에 읽히도록 시각 크기를 키운다.
	const auto ConfigureCompactSummary = [this](const TCHAR* Prefix)
	{
		for (const TCHAR* Suffix : { TEXT("HPBack"), TEXT("HPBar"), TEXT("HPText") })
		{
			SetShown(Find<UWidget>(WidgetTree,
				FString::Printf(TEXT("%s%s"), Prefix, Suffix)), false);
		}
		for (const TCHAR* Suffix : { TEXT("PortraitFrame"), TEXT("Portrait") })
		{
			if (UWidget* Widget = Find<UWidget>(WidgetTree,
				FString::Printf(TEXT("%s%s"), Prefix, Suffix)))
			{
				FWidgetTransform Transform;
				Transform.Scale = FVector2D(1.22f, 1.22f);
				Widget->SetRenderTransform(Transform);
				Widget->SetRenderTransformPivot(FVector2D(.5f, .5f));
			}
		}
		// TextBlock 자체는 AutoFit ScaleBox 안에서 클리핑된다. 잎 TextBlock을
		// 이동하면 래퍼의 클립 사각 밖으로 빠져 글자가 통째로 사라지므로,
		// 텍스트 행은 바깥 Center 래퍼를 이동한다.
		for (const TCHAR* Suffix : { TEXT("APPlate"), TEXT("APText_Center"),
			TEXT("SpeedPlate"), TEXT("SpeedIcon"), TEXT("SpeedText_Center"),
			TEXT("StatusLabel"), TEXT("Status") })
		{
			if (UWidget* Widget = Find<UWidget>(WidgetTree,
				FString::Printf(TEXT("%s%s"), Prefix, Suffix)))
			{
				FWidgetTransform Transform;
				Transform.Translation = FVector2D(0.f, -52.f);
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
				if (UWidget* Widget = Find<UWidget>(WidgetTree, FString::Printf(
					TEXT("%sStatus%s_%d"), Prefix, Kind, Index)))
				{
					FWidgetTransform Transform;
					Transform.Translation = FVector2D(0.f, -52.f);
					Transform.Scale = FVector2D(1.18f, 1.18f);
					Widget->SetRenderTransform(Transform);
					Widget->SetRenderTransformPivot(FVector2D(.5f, .5f));
				}
			}
		}
	};
	ConfigureCompactSummary(TEXT("Enemy"));
	// 0823 확정: 요약판 AP 는 문구("AP n/n")로만 보여 준다. 보석 아이콘
	// 행만 걷는다(WBP 에 구워져 있으므로 이름으로 찾아 접는다).
	mEnemyAPText = Find<UTextBlock>(WidgetTree, TEXT("EnemyAPText"));
	if (UWidget* PipRow = Find<UWidget>(WidgetTree, TEXT("EnemyAPPipRow")))
	{
		PipRow->SetVisibility(ESlateVisibility::Collapsed);
	}
	// 세로 전용 범용 프레임의 저작 크기를 런타임에서 덮어쓰지 않는다.
	for (const TCHAR* SummaryName : { TEXT("EnemyPanel"), TEXT("AllyPanel") })
	{
		if (UWidget* Summary = Find<UWidget>(WidgetTree, SummaryName))
		{
			Summary->SetClipping(EWidgetClipping::Inherit);
		}
	}
	SetShown(mEnemyNextSkillFrame, false);
	SetShown(mEnemyNextSkillIcon, false);
	SetInteractiveShown(Find<UButton>(WidgetTree, TEXT("EnemyNextSkillButton")), false);
	// WBP 에 번역 키 없이(구형 bake) 박힌 라벨을 로컬라이즈 텍스트로 갈아
	// 끼운다. 빌더는 이미 NSLOCTEXT 를 쓰지만 마지막 리베이크가 그 이전이다.
	{
		const TPair<const TCHAR*, FText> BakedLabels[] = {
			{ TEXT("MercenaryCritLabel"),
				NSLOCTEXT("CombatHUD", "MercenaryCrit", "치명타") },
			{ TEXT("MercenaryCloseText"),
				NSLOCTEXT("CombatHUD", "MercenaryBack", "닫기") },
			{ TEXT("ConfirmLabel"),
				NSLOCTEXT("CombatHUD", "ConfirmLabel", "확정") },
		};
		for (const TPair<const TCHAR*, FText>& Label : BakedLabels)
		{
			if (UTextBlock* Text = Find<UTextBlock>(WidgetTree, Label.Key))
			{
				Text->SetText(Label.Value);
			}
		}
	}
	mEnemyStatusFrames.Reset();
	mEnemyStatusIcons.Reset();
	mEnemyStatusCounts.Reset();
	mEnemyStatusButtons.Reset();
	for (int32 Index = 0; Index < 3; ++Index)
	{
		mEnemyStatusFrames.Add(Find<UWidget>(WidgetTree,
			FString::Printf(TEXT("EnemyStatusFrame_%d"), Index)));
		mEnemyStatusIcons.Add(Find<UImage>(WidgetTree,
			FString::Printf(TEXT("EnemyStatusIcon_%d"), Index)));
		mEnemyStatusCounts.Add(Find<UTextBlock>(WidgetTree,
			FString::Printf(TEXT("EnemyStatusCount_%d"), Index)));
		mEnemyStatusButtons.Add(Find<UButton>(WidgetTree,
			FString::Printf(TEXT("EnemyStatusButton_%d"), Index)));
	}
	mAllyPanel = Find<UWidget>(WidgetTree, TEXT("AllyPanel"));
	mAllyPortrait = Find<UImage>(WidgetTree, TEXT("AllyPortrait"));
	mAllyName = Find<UTextBlock>(WidgetTree, TEXT("AllyName"));
	mAllyHPBar = Find<UProgressBar>(WidgetTree, TEXT("AllyHPBar"));
	mAllyHPText = Find<UTextBlock>(WidgetTree, TEXT("AllyHPText"));
	mAllyAPText = Find<UTextBlock>(WidgetTree, TEXT("AllyAPText"));
	mAllySpeedText = Find<UTextBlock>(WidgetTree, TEXT("AllySpeedText"));
	mAllyStatusText = Find<UTextBlock>(WidgetTree, TEXT("AllyStatus"));
	ConfigureCompactSummary(TEXT("Ally"));
	mAllyStatusFrames.Reset();
	mAllyStatusIcons.Reset();
	mAllyStatusCounts.Reset();
	mAllyStatusButtons.Reset();
	for (int32 Index = 0; Index < 3; ++Index)
	{
		mAllyStatusFrames.Add(Find<UWidget>(WidgetTree,
			FString::Printf(TEXT("AllyStatusFrame_%d"), Index)));
		mAllyStatusIcons.Add(Find<UImage>(WidgetTree,
			FString::Printf(TEXT("AllyStatusIcon_%d"), Index)));
		mAllyStatusCounts.Add(Find<UTextBlock>(WidgetTree,
			FString::Printf(TEXT("AllyStatusCount_%d"), Index)));
		mAllyStatusButtons.Add(Find<UButton>(WidgetTree,
			FString::Printf(TEXT("AllyStatusButton_%d"), Index)));
	}

	mEndTurnButton = Find<UButton>(WidgetTree, TEXT("EndTurnButton"));
	mSkillToggleButton = Find<UButton>(WidgetTree, TEXT("SkillToggleButton"));
	mSkillTogglePanel = Find<UWidget>(WidgetTree, TEXT("SkillTogglePanel"));
	mSkillTogglePlate = Find<UWidget>(WidgetTree, TEXT("SkillTogglePlate"));
	mSkillToggleLabel = Find<UWidget>(WidgetTree, TEXT("SkillToggleLabel"));

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
	mMercenaryDetailSection = Find<UWidget>(WidgetTree, TEXT("MercDetailSection"));
	mMercenaryInventoryButton = Find<UButton>(
		WidgetTree, TEXT("MercenaryInventoryButton"));
	mMercenaryInventoryPage = Find<UWidget>(
		WidgetTree, TEXT("MercenaryInventoryPage"));
	mMercenaryInventoryPlate = Find<UImage>(
		WidgetTree, TEXT("MercenaryInventoryTabPlate"));
	mMercenaryInventoryGoldText = Find<UTextBlock>(
		WidgetTree, TEXT("MercenaryInventoryGoldText"));
	// 설명 띠와 고름 테두리는 뺐다(0809 시안) — 아티팩트를 누르면 바로
	// 상세 팝업이 뜨므로 면 안에 남길 상태 표시가 없다.
	mMercenaryInventoryArtifactFrames.Reset();
	mMercenaryInventoryArtifactIcons.Reset();
	mMercenaryInventoryArtifactNames.Reset();
	mMercenaryInventoryArtifactButtons.Reset();
	for (int32 Index = 0; ; ++Index)
	{
		UWidget* Frame = Find<UWidget>(WidgetTree,
			FString::Printf(TEXT("MercenaryInventoryArtifactFrame_%d"), Index));
		UImage* Icon = Find<UImage>(WidgetTree,
			FString::Printf(TEXT("MercenaryInventoryArtifactIcon_%d"), Index));
		UTextBlock* Name = Find<UTextBlock>(WidgetTree,
			FString::Printf(TEXT("MercenaryInventoryArtifactName_%d"), Index));
		UButton* Button = Find<UButton>(WidgetTree,
			FString::Printf(TEXT("MercenaryInventoryArtifactButton_%d"), Index));
		if (Frame == nullptr && Icon == nullptr && Name == nullptr
			&& Button == nullptr)
		{
			break;
		}
		mMercenaryInventoryArtifactFrames.Add(Frame);
		mMercenaryInventoryArtifactIcons.Add(Icon);
		mMercenaryInventoryArtifactNames.Add(Name);
		mMercenaryInventoryArtifactButtons.Add(Button);
	}
	// WBP 기본값이 잘못 저장되어도 전투 진입 때는 닫힌 상태로 시작한다.
	// 이후 도메인 갱신에서도 CacheAuthoredWidgets가 다시 불린다. 그때까지
	// 접으면, 골드가 갱신되는 순간 사용자가 열어 둔 탭이 닫혀 버린다.
	if (bFirstMercenaryPanelCache == true && mMercenaryPanel != nullptr)
	{
		mMercenaryPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	mConfirmPanel = Find<UWidget>(WidgetTree, TEXT("ConfirmPanel"));
	mConfirmButton = Find<UButton>(WidgetTree, TEXT("ConfirmButton"));
	mEndTurnPanel = Find<UWidget>(WidgetTree, TEXT("EndTurnPanel"));
	mEndTurnLabel = Find<UTextBlock>(WidgetTree, TEXT("EndTurnLabel"));
	mCancelPanel = Find<UWidget>(WidgetTree, TEXT("CancelPanel"));
	mCancelButton = Find<UButton>(WidgetTree, TEXT("CancelButton"));
	mCancelLabel = Find<UTextBlock>(WidgetTree, TEXT("CancelLabel"));
	mTurnAPRoot = Find<UWidget>(WidgetTree, TEXT("TurnAPScale"));
	mTurnAPText = Find<UTextBlock>(WidgetTree, TEXT("TurnAPText"));
	if (mTurnAPText != nullptr)
	{
		// 왼쪽 전용 AP 배지 안에서 두 번째 줄을 채운다.
		mTurnAPText->SetMargin(FMargin(0.f));
		FSlateFontInfo APFont = mTurnAPText->GetFont();
		APFont.Size = 30;
		mTurnAPText->SetFont(APFont);
	}
	mTurnAPPips.Reset();
	mTurnAPPipsUsed.Reset();
	mTurnAPPipGlows.Reset();
	UMaterialInterface* APGemFlashMaterial = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Game/UI/CombatLayouts/M_APGemFlash.M_APGemFlash"));
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
		UWidget* FlashWidget = Find<UWidget>(WidgetTree,
			FString::Printf(TEXT("TurnAPPipGlow_%d"), Pip));
		if (UImage* FlashImage = Cast<UImage>(FlashWidget))
		{
			// 단색 브러시는 위젯 사각형 전체를 칠해 원본 AP 보석과 다른
			// 다이아로 보였다. UI 재질이 원본 브러시의 알파만 마스크로 써서
			// 보석 외곽 그대로 백청색으로 점등하게 한다.
			if (APGemFlashMaterial != nullptr)
			{
				FlashImage->SetBrushFromMaterial(APGemFlashMaterial);
				if (UMaterialInstanceDynamic* FlashMID = FlashImage->GetDynamicMaterial())
				{
					if (const UImage* PipImage = Cast<UImage>(Found))
					{
						if (UTexture* PipTexture = Cast<UTexture>(
							PipImage->GetBrush().GetResourceObject()))
						{
							FlashMID->SetTextureParameterValue(
								TEXT("PipTexture"), PipTexture);
						}
					}
				}
			}
			FlashImage->SetColorAndOpacity(FLinearColor::White);
			FlashImage->SetRenderTransformPivot(FVector2D(.5f));
			FlashImage->SetRenderTransformAngle(0.f);
		}
		mTurnAPPipGlows.Add(FlashWidget);
	}
	mArtifactButtons.SetNum(ArtifactSlotCount);
	for (int32 Index = 0; Index < ArtifactSlotCount; ++Index)
	{
		mArtifactButtons[Index] = Find<UButton>(WidgetTree,
			FString::Printf(TEXT("ArtifactButton_%d"), Index));
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
	SetShown(Find<UWidget>(WidgetTree, TEXT("ArtifactStrip")), false);
	SetShown(Find<UWidget>(WidgetTree, TEXT("ArtifactStripPlate")), false);
	SetShown(Find<UWidget>(WidgetTree, TEXT("ArtifactStripLabel")), false);

	mTurnPageLeft = Find<UButton>(WidgetTree, TEXT("TurnPageLeft"));
	mTurnPageRight = Find<UButton>(WidgetTree, TEXT("TurnPageRight"));
	mTurnPageLeftText = Find<UTextBlock>(WidgetTree, TEXT("TurnPageLeftText"));
	mTurnPageRightText = Find<UTextBlock>(WidgetTree, TEXT("TurnPageRightText"));
	mTurnPanel = Find<UWidget>(WidgetTree, TEXT("TurnPanel"));

	// 눌림을 삼킬 묶음들. 카드는 안 넣는다 -- 카드는 제 버튼이 가져간다.
	//
	// 묶음(Canvas) 을 잡는다. 그 안의 판과 글자는 SelfHitTestInvisible 이라
	// 눌림이 그대로 뿌리까지 내려오는데, 묶음의 자리를 재면 그 안 아무 데나
	// 눌러도 걸린다.
	mChromeWidgets.Reset();
	for (const TCHAR* Name : { TEXT("RoundPanel"), TEXT("TurnPanel"),
		TEXT("ObjectivePanel"), TEXT("EnemyPanel"), TEXT("AllyPanel"), TEXT("EndTurnPanel"),
		TEXT("PartyCard_0"), TEXT("PartyCard_1"), TEXT("PartyCard_2"),
		TEXT("MercenaryPanel"), TEXT("ConfirmPanel"), TEXT("CancelPanel") })
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

	// 인벤토리의 오른쪽 내용은 하나의 Page 아래에 묶여 있다. 개별 슬롯을
	// 옮기면 4x2 피치와 중앙 정렬이 흐트러지므로, 부모를 살짝 좌상단으로
	// 옮겨 용병 상세와 같은 내용 여백을 쓴다.
	if (UWidget* InventoryPage = Find<UWidget>(
		WidgetTree, TEXT("MercenaryInventoryPage")))
	{
		if (UCanvasPanelSlot* InventorySlot =
			Cast<UCanvasPanelSlot>(InventoryPage->Slot))
		{
			// 현재 직렬화된 WBP의 위치(584, 212)를 기준으로 좌 24 / 상 18.
			InventorySlot->SetPosition(FVector2D(560.f, 194.f));
		}
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
	if (mSkillToggleButton != nullptr)
	{
		mSkillToggleButton->OnClicked.AddUniqueDynamic(
			this, &UCombatLayoutHUDWidget::HandleSkillToggleClicked);
		BindPressFeedback(mSkillToggleButton, mSkillToggleButton);
	}

	if (mConfirmButton != nullptr)
	{
		mConfirmButton->OnClicked.AddUniqueDynamic(
			this, &UCombatLayoutHUDWidget::HandleConfirmClicked);
		BindPressFeedback(mConfirmButton, mConfirmPanel);
	}
	if (mCancelButton != nullptr)
	{
		mCancelButton->OnClicked.AddUniqueDynamic(
			this, &UCombatLayoutHUDWidget::HandleCancelClicked);
		BindPressFeedback(mCancelButton, mCancelPanel);
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
	if (mMenuButtons.IsValidIndex(3))
	{
		// 넷째 톱니는 아무 데도 안 붙어 있었다. 눌러도 조용히 아무 일이 없었다.
		mMenuButtons[3]->SetIsEnabled(true);
		mMenuButtons[3]->OnClicked.AddUniqueDynamic(
			this, &UCombatLayoutHUDWidget::HandleSettingsMenuClicked);
	}
	if (mMercenaryInventoryButton != nullptr)
	{
		mMercenaryInventoryButton->OnClicked.AddUniqueDynamic(
			this, &UCombatLayoutHUDWidget::HandleInventoryClicked);
		BindPressFeedback(mMercenaryInventoryButton, mMercenaryInventoryButton);
	}
	{
		using FInventoryClickHandler = void (UCombatLayoutHUDWidget::*)();
		static const FInventoryClickHandler ClickHandlers[11] = {
			&UCombatLayoutHUDWidget::HandleMercenaryInventoryArtifactClicked_0,
			&UCombatLayoutHUDWidget::HandleMercenaryInventoryArtifactClicked_1,
			&UCombatLayoutHUDWidget::HandleMercenaryInventoryArtifactClicked_2,
			&UCombatLayoutHUDWidget::HandleMercenaryInventoryArtifactClicked_3,
			&UCombatLayoutHUDWidget::HandleMercenaryInventoryArtifactClicked_4,
			&UCombatLayoutHUDWidget::HandleMercenaryInventoryArtifactClicked_5,
			&UCombatLayoutHUDWidget::HandleMercenaryInventoryArtifactClicked_6,
			&UCombatLayoutHUDWidget::HandleMercenaryInventoryArtifactClicked_7,
			&UCombatLayoutHUDWidget::HandleMercenaryInventoryArtifactClicked_8,
			&UCombatLayoutHUDWidget::HandleMercenaryInventoryArtifactClicked_9,
			&UCombatLayoutHUDWidget::HandleMercenaryInventoryArtifactClicked_10 };
		static const TCHAR* const ClickNames[11] = {
			TEXT("HandleMercenaryInventoryArtifactClicked_0"),
			TEXT("HandleMercenaryInventoryArtifactClicked_1"),
			TEXT("HandleMercenaryInventoryArtifactClicked_2"),
			TEXT("HandleMercenaryInventoryArtifactClicked_3"),
			TEXT("HandleMercenaryInventoryArtifactClicked_4"),
			TEXT("HandleMercenaryInventoryArtifactClicked_5"),
			TEXT("HandleMercenaryInventoryArtifactClicked_6"),
			TEXT("HandleMercenaryInventoryArtifactClicked_7"),
			TEXT("HandleMercenaryInventoryArtifactClicked_8"),
			TEXT("HandleMercenaryInventoryArtifactClicked_9"),
			TEXT("HandleMercenaryInventoryArtifactClicked_10") };
		for (int32 Index = 0;
			Index < FMath::Min(mMercenaryInventoryArtifactButtons.Num(), 11); ++Index)
		{
			if (UButton* Button = mMercenaryInventoryArtifactButtons[Index])
			{
				Button->OnClicked.__Internal_AddUniqueDynamic(
					this, ClickHandlers[Index], ClickNames[Index]);
				BindPressFeedback(Button, Button);
			}
		}
	}
	// 아티팩트 칸은 꾹 눌러야 상세가 뜬다. 짧게 누르면 아무 일도 없다 --
	// 전투 중에 잘못 눌러 화면이 덮이면 곤란하다.
	{
		using FArtifactHandler = void (UCombatLayoutHUDWidget::*)();
		static const FArtifactHandler PressHandlers[ArtifactSlotCount] = {
			&UCombatLayoutHUDWidget::HandleArtifactPressed_0,
			&UCombatLayoutHUDWidget::HandleArtifactPressed_1,
			&UCombatLayoutHUDWidget::HandleArtifactPressed_2,
			&UCombatLayoutHUDWidget::HandleArtifactPressed_3,
			&UCombatLayoutHUDWidget::HandleArtifactPressed_4,
			&UCombatLayoutHUDWidget::HandleArtifactPressed_5 };
		static const TCHAR* const PressNames[ArtifactSlotCount] = {
			TEXT("HandleArtifactPressed_0"), TEXT("HandleArtifactPressed_1"),
			TEXT("HandleArtifactPressed_2"), TEXT("HandleArtifactPressed_3"),
			TEXT("HandleArtifactPressed_4"), TEXT("HandleArtifactPressed_5") };
		for (int32 Index = 0; Index < ArtifactSlotCount; ++Index)
		{
			UButton* Button = mArtifactButtons.IsValidIndex(Index)
				? mArtifactButtons[Index].Get() : nullptr;
			if (Button == nullptr)
			{
				continue;
			}
			Button->OnPressed.__Internal_AddUniqueDynamic(
				this, PressHandlers[Index], PressNames[Index]);
			Button->OnReleased.AddUniqueDynamic(
				this, &UCombatLayoutHUDWidget::HandleArtifactReleased);
			BindPressFeedback(Button, Button);
		}
	}
	if (mMercenaryCloseButton != nullptr)
	{
		mMercenaryCloseButton->OnClicked.AddUniqueDynamic(
			this, &UCombatLayoutHUDWidget::HandleMercenaryCloseClicked);
		BindPressFeedback(mMercenaryCloseButton, mMercenaryCloseButton);
	}

	// 요약판 상태 아이콘 긴 누름 → 상태 상세. 판이 소켓 위에 투명 단추를
	// 깔아 둔다. 단순 탭은 아무 일도 하지 않으며, 0.5초를 채운 한 번만 연다.
	{
		using FStatusHandler = void (UCombatLayoutHUDWidget::*)();
		static const FStatusHandler PressHandlers[2][3] = {
			{ &UCombatLayoutHUDWidget::HandleAllyStatusPressed_0,
			  &UCombatLayoutHUDWidget::HandleAllyStatusPressed_1,
			  &UCombatLayoutHUDWidget::HandleAllyStatusPressed_2 },
			{ &UCombatLayoutHUDWidget::HandleEnemyStatusPressed_0,
			  &UCombatLayoutHUDWidget::HandleEnemyStatusPressed_1,
			  &UCombatLayoutHUDWidget::HandleEnemyStatusPressed_2 } };
		static const FStatusHandler ReleaseHandlers[2][3] = {
			{ &UCombatLayoutHUDWidget::HandleAllyStatusReleased_0,
			  &UCombatLayoutHUDWidget::HandleAllyStatusReleased_1,
			  &UCombatLayoutHUDWidget::HandleAllyStatusReleased_2 },
			{ &UCombatLayoutHUDWidget::HandleEnemyStatusReleased_0,
			  &UCombatLayoutHUDWidget::HandleEnemyStatusReleased_1,
			  &UCombatLayoutHUDWidget::HandleEnemyStatusReleased_2 } };
		static const TCHAR* const PressHandlerNames[2][3] = {
			{ TEXT("HandleAllyStatusPressed_0"), TEXT("HandleAllyStatusPressed_1"),
			  TEXT("HandleAllyStatusPressed_2") },
			{ TEXT("HandleEnemyStatusPressed_0"), TEXT("HandleEnemyStatusPressed_1"),
			  TEXT("HandleEnemyStatusPressed_2") } };
		static const TCHAR* const ReleaseHandlerNames[2][3] = {
			{ TEXT("HandleAllyStatusReleased_0"), TEXT("HandleAllyStatusReleased_1"),
			  TEXT("HandleAllyStatusReleased_2") },
			{ TEXT("HandleEnemyStatusReleased_0"), TEXT("HandleEnemyStatusReleased_1"),
			  TEXT("HandleEnemyStatusReleased_2") } };
		const TArray<TObjectPtr<UButton>>* Buttons[2] = {
			&mAllyStatusButtons, &mEnemyStatusButtons };
		for (int32 Side = 0; Side < 2; ++Side)
		{
			for (int32 Index = 0; Index < 3; ++Index)
			{
				UButton* Button = Buttons[Side]->IsValidIndex(Index)
					? (*Buttons[Side])[Index].Get() : nullptr;
				if (Button != nullptr)
				{
					// 손가락이 요약판 밖으로 드래그되면 눌림을 즉시 풀어,
					// 떠난 아이콘의 상세가 0.5초 뒤 열리지 않게 한다.
					Button->SetTouchMethod(EButtonTouchMethod::PreciseTap);
					Button->SetClickMethod(EButtonClickMethod::PreciseClick);
					Button->OnPressed.__Internal_AddUniqueDynamic(
						this, PressHandlers[Side][Index], PressHandlerNames[Side][Index]);
					Button->OnReleased.__Internal_AddUniqueDynamic(
						this, ReleaseHandlers[Side][Index], ReleaseHandlerNames[Side][Index]);
				}
			}
		}
	}

	// 턴 칸 클릭 → 그 유닛을 화면 가운데로 + (아군이면) 카드 펴기.
	{
		using FClickHandler = void (UCombatLayoutHUDWidget::*)();
		static const FClickHandler TurnHandlers[TurnSlotCount] = {
			&UCombatLayoutHUDWidget::HandleTurnTokenClicked_0,
			&UCombatLayoutHUDWidget::HandleTurnTokenClicked_1,
			&UCombatLayoutHUDWidget::HandleTurnTokenClicked_2,
			&UCombatLayoutHUDWidget::HandleTurnTokenClicked_3,
			&UCombatLayoutHUDWidget::HandleTurnTokenClicked_4,
			&UCombatLayoutHUDWidget::HandleTurnTokenClicked_5,
			&UCombatLayoutHUDWidget::HandleTurnTokenClicked_6,
			&UCombatLayoutHUDWidget::HandleTurnTokenClicked_7,
			&UCombatLayoutHUDWidget::HandleTurnTokenClicked_8,
			&UCombatLayoutHUDWidget::HandleTurnTokenClicked_9 };
		static const TCHAR* const TurnHandlerNames[TurnSlotCount] = {
			TEXT("HandleTurnTokenClicked_0"), TEXT("HandleTurnTokenClicked_1"),
			TEXT("HandleTurnTokenClicked_2"), TEXT("HandleTurnTokenClicked_3"),
			TEXT("HandleTurnTokenClicked_4"), TEXT("HandleTurnTokenClicked_5"),
			TEXT("HandleTurnTokenClicked_6"), TEXT("HandleTurnTokenClicked_7"),
			TEXT("HandleTurnTokenClicked_8"), TEXT("HandleTurnTokenClicked_9") };
		for (int32 Index = 0; Index < TurnSlotCount; ++Index)
		{
			UButton* Button = Find<UButton>(WidgetTree,
				FString::Printf(TEXT("TurnTokenButton_%d"), Index));
			if (Button != nullptr)
			{
				Button->OnClicked.__Internal_AddUniqueDynamic(
					this, TurnHandlers[Index], TurnHandlerNames[Index]);
			}
		}
	}

	// 적 요약판 다음 스킬 소켓 클릭 → 그 스킬 상세.
	if (UButton* NextSkillButton = Find<UButton>(WidgetTree,
		TEXT("EnemyNextSkillButton")))
	{
		NextSkillButton->OnClicked.AddUniqueDynamic(
			this, &UCombatLayoutHUDWidget::HandleEnemyNextSkillClicked);
	}

	// 용병탭 스킬 소켓 클릭 → 스킬 상세 (0번 이동 카드는 이동 상세).
	{
		using FClickHandler = void (UCombatLayoutHUDWidget::*)();
		static const FClickHandler SkillHandlers[6] = {
			&UCombatLayoutHUDWidget::HandleMercenarySkillClicked_0,
			&UCombatLayoutHUDWidget::HandleMercenarySkillClicked_1,
			&UCombatLayoutHUDWidget::HandleMercenarySkillClicked_2,
			&UCombatLayoutHUDWidget::HandleMercenarySkillClicked_3,
			&UCombatLayoutHUDWidget::HandleMercenarySkillClicked_4,
			&UCombatLayoutHUDWidget::HandleMercenarySkillClicked_5 };
		static const TCHAR* const SkillHandlerNames[6] = {
			TEXT("HandleMercenarySkillClicked_0"), TEXT("HandleMercenarySkillClicked_1"),
			TEXT("HandleMercenarySkillClicked_2"), TEXT("HandleMercenarySkillClicked_3"),
			TEXT("HandleMercenarySkillClicked_4"), TEXT("HandleMercenarySkillClicked_5") };
		for (int32 Index = 0; Index < 6; ++Index)
		{
			UButton* Button = Find<UButton>(WidgetTree, FString::Printf(
				TEXT("MercenarySkillButton_%d"), Index));
			if (Button != nullptr)
			{
				// 모바일에서는 길게 누르기와 구분할 필요가 없는 조회 버튼이다.
				// 손가락을 뗀 한 번의 탭으로 확정되게 명시한다.
				Button->SetTouchMethod(EButtonTouchMethod::PreciseTap);
				Button->SetClickMethod(EButtonClickMethod::PreciseClick);
				Button->OnClicked.__Internal_AddUniqueDynamic(
					this, SkillHandlers[Index], SkillHandlerNames[Index]);
				BindPressFeedback(Button, Button);
			}
		}
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

	// 몬스터 상세의 첫 목록 줄은 디자인 좌표 X=265, Y=271에서 시작한다.
	// 용병 목록은 부모가 (231,279), 자식 ScaleBox가 (-21,1)에 있어 실제 시작점이
	// (210,280)이었다. 같은 종류의 탐색 목록인데 용병 쪽만 55px 왼쪽으로 튀므로
	// 부모 묶음을 옮겨 세 카드와 인벤토리 단추를 한 번에 같은 열에 맞춘다.
	if (UWidget* Roster = Find<UWidget>(WidgetTree, TEXT("MercRosterSection")))
	{
		if (UCanvasPanelSlot* RosterSlot = Cast<UCanvasPanelSlot>(Roster->Slot))
		{
			RosterSlot->SetPosition(FVector2D(286.f, 270.f));
		}
	}
	// 목록을 오른쪽으로 맞춘 뒤 초상과 목록 사이가 지나치게 붙었다. 몬스터 상세의
	// 목록-초상 간격과 맞도록 용병 상세 묶음(초상·스탯·스킬)을 함께 옮긴다.
	if (UWidget* Detail = Find<UWidget>(WidgetTree, TEXT("MercDetailSection")))
	{
		if (UCanvasPanelSlot* DetailSlot = Cast<UCanvasPanelSlot>(Detail->Slot))
		{
			DetailSlot->SetPosition(FVector2D(48.f, 0.f));
		}
	}
	// 상세 오버레이처럼 NativeConstruct 이후 생기는 버튼도 공용 클릭음을 놓치지 않는다.
	ApplyCommonButtonPressSound(Button);
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

	// 상세를 여는 김에 그 스킬을 쓸 유닛을 화면 가운데로 데려온다.
	// 스킬 칸은 판 아래에 따로 있어서, 누른 스킬이 누구 것인지 판을 봐야
	// 안다 -- 카메라가 그 유닛을 잡아 주면 그 왕복이 없어진다(0806 합의).
	FocusCameraOnTurnUnit();

	// 0번은 이동이다. 스킬이 아니라 게임플레이에 청할 것이 없으므로 화면이
	// 이미 받아 둔 값으로 바로 조립한다.
	if (SlotIndex == 0)
	{
		ShowMoveDetailOverlay();
		return;
	}
	mUIModel->RequestLongPressSkill(SlotIndex - 1);
}

/**
 * @brief 카드 고리의 가운데 자리(0~1 비율). 카메라 초점이 놓일 곳이다.
 *
 * @details "가운데" 는 화면 한가운데가 아니라 스킬 카드 여섯 장이 둘러싼
 * 자리다(0807 합의). 카드 자리는 WBP 캔버스에 고정돼 있으므로 슬롯
 * 좌표의 평균을 판 크기로 나눠 비율로 만든다 -- 해상도가 바뀌어도 맞는다.
 * 생성 직후(그리기 전)라 판 크기가 아직 0이어도 저작 캔버스 크기(1920x1080)
 * 를 기준으로 계산하므로 유효한 앵커를 낸다.
 */
FVector2D UCombatLayoutHUDWidget::ComputeCommandRingAnchor() const
{
	/*
	 * 슬롯의 캔버스 좌표를 쓰면 안 된다 -- 카드마다 앵커 기준이 달라서(가운데
	 * 앵커는 좌표가 음수) 평균이 쓰레기가 된다(0807 검증 로그: 앵커 (-1,67)).
	 * 대신 **실제 그려진 지오메트리**로 잰다. 카드 자리는 고정이라, 지금
	 * 접혀 있어도 마지막으로 그려진 값이 그대로 맞다.
	 */
	FVector2D Sum = FVector2D::ZeroVector;
	int32 Count = 0;
	for (const FCommandSlotWidgets& Widgets : mCommandSlots)
	{
		if (Widgets.Root == nullptr)
		{
			continue;
		}
		const FGeometry& Geometry = Widgets.Root->GetCachedGeometry();
		if (Geometry.GetAbsoluteSize().X <= 0.f)
		{
			continue;   // 아직 한 번도 안 그려진 칸
		}
		Sum += FVector2D(Geometry.GetAbsolutePosition())
			+ FVector2D(Geometry.GetAbsoluteSize()) * 0.5f;
		++Count;
	}
	const FGeometry& RootGeometry = GetCachedGeometry();
	if (Count > 0)
	{
		const FVector2D RootSize = FVector2D(RootGeometry.GetAbsoluteSize());
		if (RootSize.X > 0.f && RootSize.Y > 0.f)
		{
			// 같은 절대(데스크톱) 좌표계에서 판 원점을 빼고 크기로 나눠 비율로.
			const FVector2D Anchor =
				(Sum / Count - FVector2D(RootGeometry.GetAbsolutePosition()))
				/ RootSize;
			if (Anchor.X >= 0.f && Anchor.X <= 1.f
				&& Anchor.Y >= 0.f && Anchor.Y <= 1.f)
			{
				return Anchor;
			}
		}
	}

	/*
	 * 아직 카드가 한 번도 안 그려졌다(전투 들어와서 첫 클릭 -- 카드는 이제
	 * 저절로 안 펴지니 흔한 일이다). 그려진 자국 대신 **슬롯 레이아웃**으로
	 * 같은 값을 계산한다(0807 검수: 첫 클릭만 세부조정이 빠짐).
	 */
	FVector2D RootLocal = RootGeometry.GetLocalSize();
	if (RootLocal.X <= 0.f || RootLocal.Y <= 0.f)
	{
		// 생성 직후라 판이 아직 안 재졌다. 슬롯 좌표는 어차피 저작 캔버스
		// (WBP_CombatHUD04, 1920x1080) 기준이므로 그 크기를 분모로 쓴다.
		RootLocal = FVector2D(1920.f, 1080.f);
	}
	Sum = FVector2D::ZeroVector;
	Count = 0;
	for (const FCommandSlotWidgets& Widgets : mCommandSlots)
	{
		const UCanvasPanelSlot* CardSlot = Widgets.Root != nullptr
			? Cast<UCanvasPanelSlot>(Widgets.Root->Slot) : nullptr;
		if (CardSlot == nullptr)
		{
			continue;
		}
		const FAnchors Anchors = CardSlot->GetAnchors();
		if (Anchors.Minimum != Anchors.Maximum)
		{
			continue;   // 늘어나는 앵커는 여기서 계산할 일이 없다
		}
		// 점 앵커 배치 공식 그대로: 앵커점 + 오프셋 - 정렬*크기 = 왼쪽 위.
		const FVector2D Size = CardSlot->GetSize();
		const FVector2D TopLeft = RootLocal * Anchors.Minimum
			+ CardSlot->GetPosition() - CardSlot->GetAlignment() * Size;
		Sum += TopLeft + Size * 0.5f;
		++Count;
	}
	if (Count == 0)
	{
		return FVector2D(0.5f, 0.5f);
	}
	const FVector2D Anchor = (Sum / Count) / RootLocal;
	if (Anchor.X < 0.f || Anchor.X > 1.f || Anchor.Y < 0.f || Anchor.Y > 1.f)
	{
		return FVector2D(0.5f, 0.5f);
	}
	return Anchor;
}

/** @brief 이 유닛을 화면 가운데로 데려오라고 청한다. */
void UCombatLayoutHUDWidget::RequestCameraFocus(const int32 UnitId,
	const bool bWithCommandRing)
{
	if (mUIModel == nullptr || UnitId == INDEX_NONE)
	{
		return;
	}
	// 카드 고리 세부조정은 카드가 실제로 뜰 때만 한다(0807). 카드 없이
	// 고리 자리로 내리면 화면 가운데를 비워 둔 채 유닛만 아래로 처진다.
	mUIModel->SetFocusScreenAnchor(bWithCommandRing == true
		? ComputeCommandRingAnchor() : FVector2D(0.5f, 0.5f));
	mUIModel->RequestFocusUnit(UnitId);
}

/** @brief 지금 조종 중인 유닛을 화면 가운데로. 누구 스킬인지 판에서 보여 준다. */
void UCombatLayoutHUDWidget::FocusCameraOnTurnUnit()
{
	if (mUIModel == nullptr)
	{
		return;
	}
	// 지금 들여다보는 용병이 기준이다. 목록에서 고른 줄이 먼저고, 판에서
	// 직접 누른 아군이 그 다음, 둘 다 없을 때만 차례인 유닛으로 간다.
	// 판 탭은 목록 줄을 안 건드리므로 그 경로를 빼면 남의 카드를 보다
	// 상세를 열 때 카메라만 차례 유닛으로 튄다(0824 검수).
	int32 FocusUnitId = INDEX_NONE;
	if (mMercenarySelectedSlot != INDEX_NONE)
	{
		FocusUnitId = PartyUnitIdAt(mMercenarySelectedSlot);
	}
	if (FocusUnitId == INDEX_NONE)
	{
		FocusUnitId = mInspectedAllyUnitId;
	}
	if (FocusUnitId == INDEX_NONE)
	{
		FocusUnitId = mUIModel->GetTurnUI().mCurrentUnitId;
	}
	// 용병 패널이 덮고 있으면 카드는 안 보인다 -- 그때는 한가운데로.
	RequestCameraFocus(FocusUnitId,
		/*bWithCommandRing=*/IsMercenaryPanelShown() == false);
}

/** @brief 왼쪽 넘김칸을 눌러 직전 열 칸 페이지로 간다. */
void UCombatLayoutHUDWidget::HandleTurnPageLeftClicked()
{
	// 모델이 풀린 뒤(전투 정리 중) 눌러도 안전해야 한다 -- 오른쪽과 짝.
	const int32 SlotRoom = mTurnSlots.Num();
	if (mUIModel == nullptr || SlotRoom <= 0)
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
	const int32 ProjectedCount = BuildProjectedTurnTokens(Turn).Num();
	const int32 LastStart = FMath::Max(
		ProjectedCount - SlotRoom, 0);
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
		// 차례가 바뀌면 지난 카드 상태를 접는다. 큰 카드 고리는 사용자가
		// 좌하단 스킬 버튼을 눌렀을 때만 연다.
		const int32 TurnUnitId = mUIModel != nullptr
			? mUIModel->GetTurnUI().mCurrentUnitId : INDEX_NONE;
		const int32 TurnRound = mUIModel->GetTurnUI().mRound;
		if (TurnUnitId != mLastTurnUnitId || TurnRound != mLastTurnBarRound)
		{
			mLastTurnUnitId = TurnUnitId;
			mLastTurnBarRound = TurnRound;
			// 이전 턴에 수동으로 살펴보던 다른 용병을 놓는다. 그렇지 않으면
			// 스킬은 새 턴 유닛 것으로 바뀌어도 용병 강조/후속 롱프레스 초점은
			// 지난 선택을 계속 가리켜 한 화면에서 두 용병이 현재처럼 보인다.
			mMercenarySelectedSlot = INDEX_NONE;
			mInspectedAllyUnitId = INDEX_NONE;
			SetCommandsShown(false);
			// 카메라 이동은 이어지는 OnBeginAnyTurn 프레젠테이션에서 한 번만
			// 요청한다. SetTurnUI 알림에서도 움직이면 같은 턴에 트윈이 두 번
			// 시작돼 폴더블 기기에서 짧게 튀는 현상이 생긴다.
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
		// 적 차례가 시작되면 선택 대상 갱신 없이도 현재 적의 요약판/AP가
		// 즉시 갈려야 한다. Unit 갱신에만 맡기면 WBP 기본 AP가 한 프레임이
		// 아니라 턴 내내 남을 수 있다.
		RefreshEnemy();

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
		else if (mSuppressNextUnitDetailOverlay == true)
		{
			// 카드만 갈아 끼우려고 청한 상세다. 겹은 띄우지 않는다 --
			// "용병 상세 필요 없다"(0806). 값은 이미 모델에 들어와 있어
			// 용병 패널 오른쪽이 그대로 쓴다.
			mSuppressNextUnitDetailOverlay = false;
			// 비동기 상세 응답이 들어온 바로 그 프레임에 오른쪽 스킬 목록도
			// 선택한 용병 것으로 갈아 끼운다. 요청 전에 한 RefreshParty는
			// 이전 UnitDetail을 읽으므로 이것이 없으면 한 번 늦게 바뀐다.
			RefreshParty();
		}
		else
		{
			ShowUnitInspection();
		}
	}
	if (Domain == ECombatUIDomain::SkillDetail)
	{
		// 몬스터 탭의 스킬 슬롯도 이 공용 겹을 쓴다. 겹은 viewport Z=70으로
		// 탭(55) 위에 오며, 탭 자체는 뒤에 유지돼 닫으면 같은 몬스터로 돌아간다.
		ShowSkillDetailOverlay();
	}
}

void UCombatLayoutHUDWidget::RefreshParty()
{
	const TArray<FUnitUI>& Units = mUIModel->GetUnitUIs();
	const int32 CurrentUnitId = mUIModel->GetTurnUI().mCurrentUnitId;
	UTexture2D* NormalCard = mMercenaryCardNormalTexture.LoadSynchronous();
	UTexture2D* SelectedCard = mMercenaryCardSelectedTexture.LoadSynchronous();
	if (mMercenaryInventoryPlate != nullptr)
	{
		UTexture2D* InventoryTexture = mMercenaryInventoryShown
			? SelectedCard : NormalCard;
		if (InventoryTexture != nullptr)
		{
			mMercenaryInventoryPlate->SetBrushFromTexture(InventoryTexture, false);
		}
	}
	/*
	 * 오른쪽 상세가 누구를 보여 줄지.
	 *
	 * 목록에서 고른 줄이 있으면 그 용병, 없으면 지금 차례인 용병이다.
	 * 전에는 줄을 누르면 패널을 닫고 상세 겹을 띄웠는데, 상세는 이미 이
	 * 패널 오른쪽에 있어서 같은 것을 두 번 보여 주는 꼴이었다(0806).
	 */
	const FUnitUI* FocusUnit = nullptr;
	int32 PlayerIndex = 0;
	for (const FUnitUI& Unit : Units)
	{
		if (Unit.mIsPlayer == false)
		{
			continue;
		}
		if (FocusUnit == nullptr)
		{
			FocusUnit = &Unit;
		}
		if (mMercenarySelectedSlot != INDEX_NONE)
		{
			if (PlayerIndex == mMercenarySelectedSlot)
			{
				FocusUnit = &Unit;
				break;
			}
		}
		else if (Unit.mUnitId == CurrentUnitId)
		{
			FocusUnit = &Unit;
			break;
		}
		++PlayerIndex;
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
			// 고른 줄이 있으면 그 줄이, 없으면 지금 차례인 용병이 밝은 판이다.
			const bool bHighlighted = mMercenaryInventoryShown == false && (
				mMercenarySelectedSlot != INDEX_NONE
				? SlotIndex == mMercenarySelectedSlot
				: Unit.mUnitId == CurrentUnitId);
			UTexture2D* CardTexture = bHighlighted ? SelectedCard : NormalCard;
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
		if (Widgets.Name != nullptr)
		{
			// 빈 칸 처리가 흐린 색으로 바꿔 뒀을 수 있다. 되돌린다.
			Widgets.Name->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		}

		// 목록 칸은 정사각 대갈치기 전용이다. 큰 히어로 일러스트(mPortrait)를
		// 이 작은 칸에 다시 크롭하면 얼굴과 실루엣이 뭉개진다.
		UTexture2D* ListPortrait = Unit.mTurnPortrait != nullptr
			? Unit.mTurnPortrait.Get() : Unit.mPortrait.Get();
		if (Widgets.Portrait != nullptr && ListPortrait != nullptr)
		{
			SetPortraitCropped(Widgets.Portrait, ListPortrait);
		}
		if (Widgets.HPBar != nullptr)
		{
			Widgets.HPBar->SetPercent(Unit.mMaxHP > 0.f ? Unit.mHP / Unit.mMaxHP : 0.f);
		}
		SetTextIfPresent(Widgets.HPText, FText::FromString(FString::Printf(
			TEXT("%d/%d"), FMath::RoundToInt(Unit.mHP), FMath::RoundToInt(Unit.mMaxHP))));
		RefreshPartyActionPoints(Widgets, Unit);
		RefreshPartyStatus(Widgets, Unit);

		/*
		 * 확정 시안(0806): 목록 줄은 **초상 · 이름 · Lv** 만 보여 준다.
		 *
		 * HP·AP·상태는 오른쪽 상세 판이 이미 크게 적는다. 줄에까지 겹쳐 적으면
		 * 좁은 칸에 숫자가 다닥다닥 붙어 이름이 안 읽혔다. 위에서 값을 채운 뒤
		 * 여기서 걷는다 -- 값 채우는 코드를 지우지 않는 것은, 줄에 다시 정보를
		 * 얹기로 하면 이 몇 줄만 지우면 되게 하기 위해서다.
		 */
		SetShown(Widgets.HPBar, false);
		SetShown(Widgets.HPText, false);
		SetShown(Widgets.APText, false);
		SetShown(Widgets.APPlate, false);
		SetShown(Widgets.StatusText, false);
		SetShown(Widgets.StatusIcon, false);
		SetShown(Find<UWidget>(WidgetTree,
			FString::Printf(TEXT("PartyAPPipRow_%d"), SlotIndex)), false);
		SetShown(Find<UWidget>(WidgetTree,
			FString::Printf(TEXT("PartyStatusRow_%d"), SlotIndex)), false);
		SetTextIfPresent(Find<UTextBlock>(WidgetTree,
			FString::Printf(TEXT("PartyLevel_%d"), SlotIndex)),
			FText::FromString(FString::Printf(TEXT("Lv %d"),
				FMath::Max(1, Unit.mLevel))));
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
	const bool bShowMercenaryDetail = bHasFocus && mMercenaryInventoryShown == false;
	SetShown(mMercenaryDetailSection, bShowMercenaryDetail);
	SetShown(mMercenaryHeroPortrait, bShowMercenaryDetail);
	SetShown(mMercenaryDetailName, bShowMercenaryDetail);
	SetShown(mMercenaryDetailHP, bShowMercenaryDetail);
	SetShown(mMercenaryDetailAP, bShowMercenaryDetail);
	SetShown(mMercenaryDetailSpeed, bShowMercenaryDetail);
	if (bHasFocus)
	{
		// 확정 시안(0806): 큰 일러스트 대신 정사각 대갈치기를 건다.
		UTexture2D* HeroFace = FocusUnit->mTurnPortrait != nullptr
			? FocusUnit->mTurnPortrait.Get() : FocusUnit->mPortrait.Get();
		if (mMercenaryHeroPortrait != nullptr && HeroFace != nullptr)
		{
			SetPortraitCropped(mMercenaryHeroPortrait, HeroFace);
		}
		SetTextIfPresent(mMercenaryDetailName, FocusUnit->mName);
		// 라벨(HP/AP/속도)은 줄판 왼쪽에 따로 있으니 수치에는 값만 적는다.
		SetTextIfPresent(mMercenaryDetailHP, FText::FromString(FString::Printf(
			TEXT("%d/%d"), FMath::RoundToInt(FocusUnit->mHP),
			FMath::RoundToInt(FocusUnit->mMaxHP))));
		SetTextIfPresent(mMercenaryDetailAP, FText::FromString(FString::Printf(
			TEXT("%d/%d"), FocusUnit->mActionPoints,
			FocusUnit->mMaxActionPoints)));
		SetTextIfPresent(mMercenaryDetailSpeed, FText::AsNumber(
			FMath::RoundToInt(FocusUnit->mSpeedPoint)));
	}

	// 우측 아래는 전투 중 현재 용병이 실제로 쓸 수 있는 여섯 커맨드의 요약이다.
	// 0번 이동 + 스킬 다섯 칸(첫 스킬이 평타)으로, 전투 레일과 같은 순서라
	// 메뉴를 닫은 뒤 카드 위치가 바뀌지 않는다.
	const TArray<FSkillUI>& Skills = mUIModel->GetSkillUIs();
	const FUnitDetailUI& InspectedDetail = mUIModel->GetUnitDetail();
	const bool bUseInspectedSkills = FocusUnit != nullptr
		&& InspectedDetail.mUnitId == FocusUnit->mUnitId;
	for (int32 Index = 0; Index < CommandSlotCount; ++Index)
	{
		UWidget* Frame = Find<UWidget>(WidgetTree,
			FString::Printf(TEXT("MercenarySkillFrame_%d"), Index));
		UImage* Icon = Find<UImage>(WidgetTree,
			FString::Printf(TEXT("MercenarySkillIcon_%d"), Index));
		UTextBlock* Name = Find<UTextBlock>(WidgetTree,
			FString::Printf(TEXT("MercenarySkillName_%d"), Index));
		UTextBlock* Cost = Find<UTextBlock>(WidgetTree,
			FString::Printf(TEXT("MercenarySkillCost_%d"), Index));

		const bool bHasSkill = Index > 0 && (bUseInspectedSkills
			? InspectedDetail.mSkills.IsValidIndex(Index - 1)
			: Skills.IsValidIndex(Index - 1));
		const bool bHasCommand = bShowMercenaryDetail
			&& (Index == 0 || bHasSkill);
		SetShown(Frame, bShowMercenaryDetail);
		SetShown(Icon, bHasCommand);
		SetShown(Name, bHasCommand);
		SetShown(Cost, bHasCommand);
		if (bHasCommand == false)
		{
			continue;
		}

		UTexture2D* CommandIcon = nullptr;
		if (Index == 0)
		{
			SetTextIfPresent(Name, LOCTEXT("MercenaryMoveSummary", "이동"));
			SetTextIfPresent(Cost, FText::AsNumber(1));
			CommandIcon = mCommandMoveIconTexture;
		}
		else
		{
			if (bUseInspectedSkills)
			{
				const FUnitDetailSkillUI& Skill = InspectedDetail.mSkills[Index - 1];
				SetTextIfPresent(Name, Skill.mName);
				SetTextIfPresent(Cost, Skill.mActionPointCost >= 0
					? FText::AsNumber(Skill.mActionPointCost) : FText::GetEmpty());
				CommandIcon = Skill.mIcon;
			}
			else
			{
				const FSkillUI& Skill = Skills[Index - 1];
				SetTextIfPresent(Name, Skill.mName);
				SetTextIfPresent(Cost, FText::AsNumber(Skill.mActionPointCost));
				CommandIcon = Skill.mIcon;
			}
		}
		// 목업/구형 데이터는 Skill.mIcon이 비어 있을 수 있다. 전투 카드가 이미
		// 들고 있는 기본 아이콘을 복사해 요약 칸만 텅 비는 것을 막는다.
		if (CommandIcon == nullptr && mCommandSlots.IsValidIndex(Index)
			&& mCommandSlots[Index].Icon != nullptr)
		{
			CommandIcon = Cast<UTexture2D>(
				mCommandSlots[Index].Icon->GetBrush().GetResourceObject());
		}
		/*
		 * 그림이 없으면 **아이콘을 끈다.**
		 *
		 * 전에는 그림이 없어도 켜 두었다. 브러시에 텍스처가 없는 Image 는
		 * 흰 사각으로 그려져서, 스킬 칸 두 개가 흰 판으로 떠 있었다.
		 * 그림이 없는 칸은 이름만 보여 주는 편이 낫다.
		 */
		SetShown(Icon, CommandIcon != nullptr);
		if (Icon != nullptr && CommandIcon != nullptr)
		{
			Icon->SetBrushFromTexture(CommandIcon, false);
		}
		// 그림이 있으면 칸은 그림으로 말한다. 없을 때만 이름을 띄운다.
		SetShown(Name, CommandIcon == nullptr);
	}
	RefreshMercenaryInventory();
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

	// 이름을 비우면 카드가 뜻 없는 검은 상자로 남는다. 자리가 비었다고 적는다.
	SetTextIfPresent(Widgets.Name, LOCTEXT("PartySlotEmpty", "빈 자리"));
	if (Widgets.Name != nullptr)
	{
		Widgets.Name->SetColorAndOpacity(FSlateColor(FLinearColor(0.62f, 0.56f, 0.46f, 1.f)));
	}
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
			// 게임 태그 명칭은 Vigor로 바뀌었지만 기존 런타임 그림 파일명은
			// T_Status_Agility다. 생성되지 않은 새 파일명을 요청하지 않는다.
			{ EffectTags::GameplayEffect_StatusEffect_RoundDuration_Buff_Vigor, TEXT("T_Status_Agility") },
			{ EffectTags::GameplayEffect_StatusEffect_RoundDuration_Buff_Fortification, TEXT("T_Status_Fortification") },
			{ EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Vulnerability, TEXT("T_Status_Vulnerability") },
			{ EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Weakness, TEXT("T_Status_Weakness") },
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
		? mUIModel->GetDisplayedMovementPoint(Unit)
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
 * @brief 현재 라운드 잔여분 뒤에 미래 열 라운드 순서를 이어 턴바에 그린다.
 *
 * @details
 * 전투 모델이 계산한 라운드별 순서를 한 줄로 펼친다. 미래 라운드는
 * 반투명으로, 각 라운드 경계에는 세로 막대와 R#을 표시한다. 화면에는
 * 열 턴씩 보이고 양끝 버튼 또는 가로 스와이프로 다음 페이지를 연다.
 */
void UCombatLayoutHUDWidget::RefreshTurnOrder()
{
	const FTurnUI& Turn = mUIModel->GetTurnUI();
	const TArray<FUnitUI>& Units = mUIModel->GetUnitUIs();
	const TArray<FProjectedTurnToken> ProjectedTurns = BuildProjectedTurnTokens(Turn);

	// 일부 원본 WBP는 편집 중 방해되지 않도록 독립 라운드 패널을
	// Collapsed로 저장한다. 런타임에서는 부모 패널까지 명시적으로 켠다.
	SetShown(mRoundPanel, true);
	// 0823 확정: 배지는 "ROUND" 글자만, 라운드 수는 아래 두 자리 숫자가 맡는다.
	SetTextIfPresent(mRoundText, FText::FromString(TEXT("ROUND")));
	SetTextIfPresent(mRoundNumberText, FText::FromString(
		FString::Printf(TEXT("%02d"), Turn.mRound)));

	const int32 SlotRoom = mTurnSlots.Num();
	const int32 Total = ProjectedTurns.Num();
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

	// 어느 칸이 누구인지 기억해 둔다 -- 칸을 누르면 그 유닛을 잡아야 한다.
	mTurnSlotUnitIds.Init(INDEX_NONE, SlotRoom);

	for (int32 Index = 0; Index < SlotRoom; ++Index)
	{
		const FTurnSlotWidgets& Widgets = mTurnSlots[Index];
		const int32 ProjectedIndex = Start + Index;
		if (ProjectedIndex >= Total)
		{
			SetShown(Widgets.Root, false);
			SetInteractiveShown(Widgets.Button, false);
			SetShown(Widgets.RoundDivider, false);
			SetShown(Widgets.RoundLabel, false);
			continue;
		}
		const FProjectedTurnToken& ProjectedTurn = ProjectedTurns[ProjectedIndex];
		if (ProjectedTurn.mEmptyRound)
		{
			/*
			 * 아무도 턴을 못 차는 라운드. 칸 하나를 비워 "이런 라운드가
			 * 지나간다" 만 알린다(0807 합의) -- 안 그리면 R2 다음에 R4 가
			 * 붙어 라운드 번호가 뛰는 것처럼 보인다.
			 */
			mTurnSlotUnitIds[Index] = INDEX_NONE;
			SetShown(Widgets.Root, true);
			SetInteractiveShown(Widgets.Button, false);
			if (Widgets.Root != nullptr)
			{
				Widgets.Root->SetRenderOpacity(0.45f);
			}
			SetShown(Widgets.Portrait, false);
			SetTextIfPresent(Widgets.Name, FText::GetEmpty());
			SetShown(Widgets.Current, false);
			SetShown(Widgets.RoundDivider, true);
			SetShown(Widgets.RoundLabel, true);
			SetTextIfPresent(Widgets.RoundLabel,
				FText::FromString(FString::Printf(TEXT("R%d"),
					Turn.mRound + ProjectedTurn.mRoundOffset)));
			continue;
		}

		const int32 UnitId = ProjectedTurn.mUnitId;
		const FUnitUI* Unit = Units.FindByPredicate(
			[UnitId](const FUnitUI& Candidate) { return Candidate.mUnitId == UnitId; });
		if (Unit == nullptr)
		{
			SetShown(Widgets.Root, false);
			SetInteractiveShown(Widgets.Button, false);
			SetShown(Widgets.RoundDivider, false);
			SetShown(Widgets.RoundLabel, false);
			continue;
		}

		const int32 RoundOffset = ProjectedTurn.mRoundOffset;
		// 라운드가 바뀌는 자리마다 배지를 단다 -- **이번 라운드가 시작되는
		// 첫 칸도 포함**이다. 전에는 다음 라운드 경계에만 달아, 턴바에 R2는
		// 있는데 R1이 없어 "이 줄이 어느 라운드부터인가"를 알 수 없었다
		// (0824 검수 2번). 왼쪽 ROUND 배지와 겹치는 정보이긴 하지만, 턴바를
		// 읽는 눈이 왼쪽까지 다녀오지 않아도 되게 한다.
		const bool bStartsShownRound = ProjectedIndex == 0
			|| ProjectedTurn.mStartsRound;

		mTurnSlotUnitIds[Index] = UnitId;
		SetShown(Widgets.Root, true);
		// 클릭 받이는 TurnToken의 자식이 아니라 TurnPanel의 직계 형제다.
		// 토큰만 켜서는 donor에 저장된 Collapsed 버튼이 되살아나지 않는다.
		SetInteractiveShown(Widgets.Button, true);
		if (Widgets.Root != nullptr)
		{
			Widgets.Root->SetRenderOpacity(RoundOffset == 0 ? 1.f : 0.45f);
		}
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
			SetShown(Widgets.CooldownOverlayRoot, false);
			SetShown(Widgets.Damage, false);
			// 이동 카드도 아이콘을 갖는다. 스킬 카드만 그림이 생기니 이
			// 칸만 비어 보였다. (판 기본이 NoDraw라 DrawAs 도 되돌린다.)
			if (Widgets.Icon != nullptr && mCommandMoveIconTexture != nullptr)
			{
				Widgets.Icon->SetBrushFromTexture(mCommandMoveIconTexture, false);
				FSlateBrush MoveIconBrush = Widgets.Icon->GetBrush();
				MoveIconBrush.DrawAs = ESlateBrushDrawType::Image;
				Widgets.Icon->SetBrush(MoveIconBrush);
				SetShown(Widgets.Icon, true);
			}
			// 행동력이 바닥나면 이동도 잠근다. 갈 수 있는 칸이 없다 -- 스킬은
			// 잠그면서 이동만 열어 두면 "왜 이것만 되지"가 된다. 긴 누름으로
			// 설명을 읽는 것은 스킬과 마찬가지로 잠겨도 된다.
			// 남의 레일(들여다보기)에서도 잠근다 -- 스킬 카드는 mIsUsable 로
			// 꺼지는데 이동만 열려 보이면 눌러 보게 된다(0823 검수).
			{
				const FUnitUI* TurnUnit = FindTurnUnit();
				const bool bCanMove = mUIModel->IsSkillRailOwnTurn()
					&& TurnUnit != nullptr
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

		if (Widgets.Icon != nullptr)
		{
			if (Skill.mIcon != nullptr)
			{
				Widgets.Icon->SetBrushFromTexture(Skill.mIcon, false);
				// 판은 빈 칸이 흰 사각으로 뜨지 말라고 NoDraw로 두었다.
				// SetBrushFromTexture는 그림만 갈아끼우고 그리기 방식은 안
				// 바꾸므로, 여기서 Image로 돌려놓지 않으면 아이콘이 영영 안
				// 그려진다 — 데이터에 아이콘이 다 있는데 카드가 비어 있던 원인.
				FSlateBrush IconBrush = Widgets.Icon->GetBrush();
				IconBrush.DrawAs = ESlateBrushDrawType::Image;
				Widgets.Icon->SetBrush(IconBrush);
			}
			SetShown(Widgets.Icon, Skill.mIcon != nullptr);
		}

		// 기존 우하단 배지의 남은 턴 표기는 그대로 유지한다. 아이콘 중앙
		// 숫자는 먼 거리에서도 쿨타임을 읽게 하는 추가 피드백이지,
		// 기존 카드 정보를 대체하지 않는다.
		const bool bOnCooldown = Skill.mRemainingCooldown > 0;
		SetShown(Widgets.Cooldown, bOnCooldown);
		SetShown(Widgets.CooldownIcon, bOnCooldown);
		if (Widgets.Cooldown != nullptr)
		{
			Widgets.Cooldown->SetText(
				FText::AsNumber(Skill.mRemainingCooldown));
		}
		SetShown(Widgets.CooldownOverlayRoot, bOnCooldown);
		if (Widgets.CooldownOverlay != nullptr)
		{
			Widgets.CooldownOverlay->SetText(
				FText::AsNumber(Skill.mRemainingCooldown));
		}
		if (Widgets.Icon != nullptr)
		{
			const float Brightness = bOnCooldown
				? .22f : (Skill.mIsUsable ? 1.f : .46f);
			Widgets.Icon->SetColorAndOpacity(FLinearColor(
				Brightness, Brightness, Brightness, 1.f));
		}

		if (Widgets.Damage != nullptr)
		{
			const bool bHasDamage = Skill.mDamageMax > 0;
			SetShown(Widgets.Damage, bHasDamage);
			if (bHasDamage)
			{
				// 시안은 "피해 8~14" 처럼 무엇의 숫자인지 적는다. 숫자만
				// 있으면 쿨 턴 수와 구분이 안 된다.
				Widgets.Damage->SetText(FText::Format(
					LOCTEXT("SkillCardDamage", "피해 {0}~{1}"),
					Skill.mDamageMin, Skill.mDamageMax));
			}
		}

		// 쿨타임은 어두운 원본 아이콘+숫자가 직접 설명한다. 이때 옛 교차검
		// 비활성 겹까지 덮으면 어떤 스킬인지 다시 사라진다.
		SetShown(Widgets.Disabled, !Skill.mIsUsable && !bOnCooldown);
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
	// 눌러 둔 0.5초 사이에 대상/턴이 바뀌면 같은 슬롯 번호가 다른 상태를
	// 가리킬 수 있다. 스냅샷을 갈기 전에 그 누름부터 무른다.
	CancelStatusPress();

	// 안내판은 두 경우에만 뜬다. **짚은 적**이 있거나, **적 차례**거나.
	//
	// 전에는 아무것도 안 짚었으면 살아 있는 첫 적으로 떨어졌다. 그래서 내
	// 차례 내내 엉뚱한 적의 안내판이 떠 있었다 -- 누가 움직일 차례인지
	// 화면이 거짓말을 한 셈이다. 떨어질 곳을 없앴다.
	const TArray<FUnitUI>& Units = mUIModel->GetUnitUIs();
	const FCombatTargetUI& Target = mUIModel->GetTarget();
	const int32 TargetId = Target.mIsValid ? Target.mUnitId : INDEX_NONE;
	const FUnitUI* TargetUnit = TargetId != INDEX_NONE
		? Units.FindByPredicate([TargetId](const FUnitUI& Unit)
			{ return Unit.mUnitId == TargetId && Unit.mHP > 0.f; })
		: nullptr;
	const FUnitUI* TurnUnit = FindTurnUnit();
	const FUnitUI* AllyShown = nullptr;
	const FUnitUI* Shown = nullptr;

	// 요약판 둘은 같은 자리를 쓴다. 짚은 유닛이 있으면 그 유닛의 판 하나만,
	// 없으면 현재 턴 유닛의 판 하나만 편다. 내 차례 기본 화면에는 용병이
	// 보이고, 적을 클릭한 순간에는 적 요약으로 정확히 교체된다.
	if (TargetUnit != nullptr)
	{
		AllyShown = TargetUnit->mIsPlayer ? TargetUnit : nullptr;
		Shown = TargetUnit->mIsPlayer ? nullptr : TargetUnit;
	}
	else if (TurnUnit != nullptr && TurnUnit->mHP > 0.f)
	{
		AllyShown = TurnUnit->mIsPlayer ? TurnUnit : nullptr;
		Shown = TurnUnit->mIsPlayer ? nullptr : TurnUnit;
	}

	SetShown(mAllyPanel, AllyShown != nullptr);
	if (AllyShown == nullptr)
	{
		mAllyShownStatuses.Reset();
		for (const TObjectPtr<UButton>& Button : mAllyStatusButtons)
		{
			SetInteractiveShown(Button.Get(), false);
		}
	}
	if (AllyShown != nullptr)
	{
		SetTextIfPresent(mAllyName, AllyShown->mName);
		UTexture2D* AllyPortrait = AllyShown->mTurnPortrait != nullptr
			? AllyShown->mTurnPortrait.Get() : AllyShown->mPortrait.Get();
		if (mAllyPortrait != nullptr && AllyPortrait != nullptr)
		{
			SetPortraitCropped(mAllyPortrait, AllyPortrait);
		}
		if (mAllyHPBar != nullptr)
		{
			mAllyHPBar->SetPercent(AllyShown->mMaxHP > 0.f
				? AllyShown->mHP / AllyShown->mMaxHP : 0.f);
		}
		SetTextIfPresent(mAllyHPText, FText::FromString(FString::Printf(
			TEXT("%d/%d"), FMath::RoundToInt(AllyShown->mHP),
			FMath::RoundToInt(AllyShown->mMaxHP))));
		// 확정 시안 표기: "AP 10/10", 속도는 아이콘 옆 숫자만.
		SetTextIfPresent(mAllyAPText, FText::FromString(AllyShown->mMaxActionPoints > 0
			? FString::Printf(TEXT("AP %d/%d"), AllyShown->mActionPoints,
				AllyShown->mMaxActionPoints)
			: FString::Printf(TEXT("AP %d"), AllyShown->mActionPoints)));
		SetTextIfPresent(mAllySpeedText, FText::AsNumber(
			FMath::RoundToInt(AllyShown->mSpeedPoint)));

		// 확정 시안: 상태는 아이콘으로만 보여 준다. 글줄은 걷는다.
		SetShown(mAllyStatusText, false);
		mAllyShownStatuses = AllyShown->mStatusEffects;   // 아이콘 클릭 → 상태 상세용

		for (int32 Index = 0; Index < mAllyStatusFrames.Num(); ++Index)
		{
			const bool bHasStatus = AllyShown->mStatusEffects.IsValidIndex(Index);
			SetShown(mAllyStatusFrames[Index], bHasStatus);
			if (mAllyStatusButtons.IsValidIndex(Index))
			{
				SetInteractiveShown(mAllyStatusButtons[Index].Get(), bHasStatus);
			}
			UImage* Icon = mAllyStatusIcons.IsValidIndex(Index)
				? mAllyStatusIcons[Index].Get() : nullptr;
			UTextBlock* Count = mAllyStatusCounts.IsValidIndex(Index)
				? mAllyStatusCounts[Index].Get() : nullptr;
			UTexture2D* Art = bHasStatus
				? StatusIconFor(AllyShown->mStatusEffects[Index].mTag) : nullptr;
			SetShown(Icon, Art != nullptr);
			if (Icon != nullptr && Art != nullptr)
			{
				Icon->SetBrushFromTexture(Art, false);
			}
			const int32 StackCount = bHasStatus
				? AllyShown->mStatusEffects[Index].mStackCount : 0;
			SetShown(Count, StackCount > 1);
			if (Count != nullptr && StackCount > 1)
			{
				Count->SetText(FText::AsNumber(StackCount));
			}
		}
	}

	SetShown(mEnemyPanel, Shown != nullptr);
	if (Shown == nullptr)
	{
		mEnemyShownStatuses.Reset();
		for (const TObjectPtr<UButton>& Button : mEnemyStatusButtons)
		{
			SetInteractiveShown(Button.Get(), false);
		}
		return;
	}

	SetTextIfPresent(mEnemyName, Shown->mName);
	// 아군과 같은 대비: 요약 초상이 비면 턴바용 정사각(HeadV2)으로 대신 건다.
	// 적 유닛은 mPortrait 가 비어 있는 경우가 많아 원이 검게 비어 있었다.
	UTexture2D* EnemyPortraitArt = Shown->mTurnPortrait != nullptr
		? Shown->mTurnPortrait.Get() : Shown->mPortrait.Get();
	if (mEnemyPortrait != nullptr && EnemyPortraitArt != nullptr)
	{
		SetPortraitCropped(mEnemyPortrait, EnemyPortraitArt);
	}
	if (mEnemyHPBar != nullptr)
	{
		mEnemyHPBar->SetPercent(
			Shown->mMaxHP > 0.f ? Shown->mHP / Shown->mMaxHP : 0.f);
	}
	// 확정 시안 표기: "100/100" · "AP 0/5" · 속도는 아이콘 옆 숫자만.
	// 0823 확정: AP 보석 행은 걷었고 문구만 남긴다.
	SetTextIfPresent(mEnemyHPText, FText::FromString(FString::Printf(
		TEXT("%d/%d"), FMath::RoundToInt(Shown->mHP), FMath::RoundToInt(Shown->mMaxHP))));
	const int32 EnemyAPLeft = mUIModel != nullptr
		? mUIModel->GetDisplayedMovementPoint(*Shown)
		: FMath::Max(Shown->mActionPoints, 0);
	const int32 EnemyAPTotal = FMath::Max(
		FMath::RoundToInt(Shown->mMaxMovementPoint),
		FMath::Max(Shown->mMaxActionPoints, EnemyAPLeft));
	SetTextIfPresent(mEnemyAPText, FText::FromString(
		FString::Printf(TEXT("AP %d/%d"), EnemyAPLeft, EnemyAPTotal)));
	SetTextIfPresent(mEnemySpeedText, FText::AsNumber(
		FMath::RoundToInt(Shown->mSpeedPoint)));
	// 치명 확률은 아직 FUnitUI 계약에 없으므로 값을 지어내지 않는다. 다만
	// 용병 요약판과 짝인 프레임은 보이고 미지원 값은 명시적으로 '-'다.
	SetTextIfPresent(mEnemyCritText, FText::FromString(TEXT("-")));

	// 확정 시안: 상태는 아이콘으로만 보여 준다. 글줄은 걷는다.
	SetShown(mEnemyStatusText, false);
	mEnemyShownStatuses = Shown->mStatusEffects;   // 아이콘 클릭 → 상태 상세용

	for (int32 Index = 0; Index < mEnemyStatusFrames.Num(); ++Index)
	{
		const bool bHasStatus = Shown->mStatusEffects.IsValidIndex(Index);
		SetShown(mEnemyStatusFrames[Index], bHasStatus);
		if (mEnemyStatusButtons.IsValidIndex(Index))
		{
			SetInteractiveShown(mEnemyStatusButtons[Index].Get(), bHasStatus);
		}

		UImage* Icon = mEnemyStatusIcons.IsValidIndex(Index)
			? mEnemyStatusIcons[Index].Get() : nullptr;
		UTextBlock* Count = mEnemyStatusCounts.IsValidIndex(Index)
			? mEnemyStatusCounts[Index].Get() : nullptr;
		UTexture2D* Art = bHasStatus
			? StatusIconFor(Shown->mStatusEffects[Index].mTag) : nullptr;
		SetShown(Icon, Art != nullptr);
		if (Icon != nullptr && Art != nullptr)
		{
			Icon->SetBrushFromTexture(Art, false);
		}

		const int32 StackCount = bHasStatus
			? Shown->mStatusEffects[Index].mStackCount : 0;
		SetShown(Count, StackCount > 1);
		if (Count != nullptr && StackCount > 1)
		{
			Count->SetText(FText::AsNumber(StackCount));
		}
	}

	// 세로 요약판은 상태이상 여부까지만 보여 준다. 다음 행동을 미리 알려 주는
	// 데이터는 전투 모델에 남겨도 이 화면에서는 의도적으로 소비하지 않는다.
	SetShown(mEnemyForecastText, false);
	mEnemyShownUnitId = Shown->mUnitId;                     // 소켓 클릭 → 상세용
	mEnemyShownNextSkillIndex = INDEX_NONE;
	SetShown(mEnemyNextSkillFrame, false);
	SetShown(mEnemyNextSkillIcon, false);
	SetInteractiveShown(Find<UButton>(WidgetTree, TEXT("EnemyNextSkillButton")), false);
}

void UCombatLayoutHUDWidget::RefreshMeta()
{
	const TArray<FUnitUI>& Units = mUIModel->GetUnitUIs();
	int32 Alive = 0;
	for (const FUnitUI& Unit : Units)
	{
		Alive += (!Unit.mIsPlayer && Unit.mHP > 0.f) ? 1 : 0;
	}
	SetTextIfPresent(mObjectiveText, FText::Format(
		LOCTEXT("CombatObjectiveRemaining", "모든 적 처치 — 남은 적 {0}"), Alive));
	SetTextIfPresent(mMercenaryGoldText,
		FText::AsNumber(mUIModel->GetPlayerMeta().mGold));

	// 아티팩트는 용병 패널의 인벤토리 면에서만 보여 준다. 전투 화면의
	// 좌하단 ArtifactStrip은 기존 WBP 호환을 위해 남겨도 항상 접는다.
	for (int32 Index = 0; Index < ArtifactSlotCount; ++Index)
	{
		UImage* Icon = mArtifactIcons.IsValidIndex(Index)
			? mArtifactIcons[Index].Get() : nullptr;
		UWidget* Frame = mArtifactFrames.IsValidIndex(Index)
			? mArtifactFrames[Index].Get() : nullptr;
		SetShown(Icon, false);
		SetShown(Frame, false);
	}
	RefreshMercenaryInventory();
}

void UCombatLayoutHUDWidget::RefreshMercenaryInventory()
{
	SetShown(mMercenaryInventoryPage, mMercenaryInventoryShown);
	// HandleInventoryClicked() 뒤에는 메타 갱신이 연달아 들어온다. 제목도 이
	// 최종 새로고침에서 다시 확정해, 디자이너 기본값(용병)이 런타임 제목을
	// 덮어쓴 채 캡처되거나 표시되지 않게 한다.
	SetTextIfPresent(Find<UTextBlock>(WidgetTree, TEXT("MercenaryTitleText")),
		mMercenaryInventoryShown
			? NSLOCTEXT("CombatHUD", "MercenaryInventoryPageTitle", "인벤토리")
			: NSLOCTEXT("CombatHUD", "MercenaryPageTitle", "용병"));
	if (mMercenaryInventoryShown == false || mUIModel == nullptr)
	{
		return;
	}

	const FPlayerMetaUI& Meta = mUIModel->GetPlayerMeta();
	SetTextIfPresent(mMercenaryInventoryGoldText, FText::AsNumber(Meta.mGold));
	for (int32 Index = 0; Index < mMercenaryInventoryArtifactFrames.Num(); ++Index)
	{
		const FCombatArtifactUI* Artifact = Meta.mArtifacts.IsValidIndex(Index)
			? &Meta.mArtifacts[Index] : nullptr;
		const bool bHasArtifact = Artifact != nullptr;
		const bool bHasIcon = bHasArtifact && Artifact->mIcon != nullptr;
		SetShown(mMercenaryInventoryArtifactFrames[Index], true);
		if (mMercenaryInventoryArtifactIcons.IsValidIndex(Index))
		{
			UImage* Icon = mMercenaryInventoryArtifactIcons[Index];
			SetShown(Icon, bHasIcon);
			if (Icon != nullptr && bHasIcon)
			{
				Icon->SetBrushFromTexture(Artifact->mIcon.Get(), false);
			}
		}
		if (mMercenaryInventoryArtifactNames.IsValidIndex(Index))
		{
			UTextBlock* Name = mMercenaryInventoryArtifactNames[Index];
			// 이름 표시 여부는 WBP 디자이너 값에 맡긴다. C++은 내용만 바꾼다.
			SetTextIfPresent(Name, bHasArtifact
				? (Artifact->mName.IsEmpty()
					? NSLOCTEXT("CombatHUD", "ArtifactFallbackName", "Artifact")
					: Artifact->mName)
				: FText::GetEmpty());
		}
		if (mMercenaryInventoryArtifactButtons.IsValidIndex(Index)
			&& mMercenaryInventoryArtifactButtons[Index] != nullptr)
		{
			mMercenaryInventoryArtifactButtons[Index]->SetIsEnabled(bHasArtifact);
		}
	}
}

void UCombatLayoutHUDWidget::SelectMercenaryInventoryArtifact(const int32 SlotIndex)
{
	if (mUIModel == nullptr
		|| mUIModel->GetPlayerMeta().mArtifacts.IsValidIndex(SlotIndex) == false)
	{
		return;
	}
	// 인벤토리 면은 조회 전용이다. 짧게 누르면 바로 기존 아티팩트 상세 WBP를
	// 열어야 하며, 전투 HUD의 롱프레스 규칙을 여기까지 끌고 오지 않는다.
	ShowArtifactDetailOverlay(SlotIndex);
}

void UCombatLayoutHUDWidget::SetMercenaryInventoryShown(const bool bShown)
{
	mMercenaryInventoryShown = bShown;
	SetShown(mMercenaryInventoryPage, bShown);
	SetShown(mMercenaryDetailSection, bShown == false);
	SetTextIfPresent(Find<UTextBlock>(WidgetTree, TEXT("MercenaryTitleText")),
		bShown
			? NSLOCTEXT("CombatHUD", "MercenaryInventoryPageTitle", "인벤토리")
			: NSLOCTEXT("CombatHUD", "MercenaryPageTitle", "용병"));

	// 이전 WBP의 상세 부품 중 일부는 MercDetailSection 밖에 직접 놓여 있었다.
	// 페이지 Canvas만 올리면 그 초상화 틀과 수치 칩이 인벤토리 뒤에 유령처럼
	// 남는다. 이름으로 한 번에 접어 같은 용병판의 두 얼굴을 완전히 분리한다.
	static const TCHAR* const DetailWidgetNames[] = {
		TEXT("MercenaryHeroPortrait"), TEXT("MercenaryPortraitFrame"),
		TEXT("MercenaryNamePlate"), TEXT("MercenaryDetailName"),
		TEXT("MercenaryDetailHP"), TEXT("MercenaryDetailAP"),
		TEXT("MercenaryDetailSpeed"), TEXT("MercenaryCritPlate"),
		TEXT("MercenaryCritIcon"),
		TEXT("MercenaryCritLabel"), TEXT("MercenaryCritValue"),
		TEXT("MercenarySkillHeading"), TEXT("MercenarySkillDivider") };
	for (const TCHAR* WidgetName : DetailWidgetNames)
	{
		SetShown(Find<UWidget>(WidgetTree, WidgetName), bShown == false);
	}
	for (int32 Index = 0; Index < 3; ++Index)
	{
		SetShown(Find<UWidget>(WidgetTree,
			FString::Printf(TEXT("MercenaryChip%dFrame"), Index)), bShown == false);
		SetShown(Find<UWidget>(WidgetTree,
			FString::Printf(TEXT("MercenaryChip%dLabel"), Index)), bShown == false);
	}
	for (int32 Index = 0; Index < 6; ++Index)
	{
		for (const TCHAR* Prefix : { TEXT("MercenarySkillFrame"),
			TEXT("MercenarySkillIcon"), TEXT("MercenarySkillName"),
			TEXT("MercenarySkillCost") })
		{
			SetShown(Find<UWidget>(WidgetTree,
				FString::Printf(TEXT("%s_%d"), Prefix, Index)), bShown == false);
		}
		// 장식과 같은 SetShown()을 쓰면 보이는 순간 SelfHitTestInvisible이 되어
		// 버튼이 존재하고 OnClicked도 묶였는데 실제 손가락 입력만 사라진다.
		SetInteractiveShown(Find<UButton>(WidgetTree,
			FString::Printf(TEXT("MercenarySkillButton_%d"), Index)), bShown == false);
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
		CancelStatusPress();
		SetMonsterTabShown(false);
		// 열 때는 지금 차례인 용병부터 보여 준다. 지난번에 고른 줄이
		// 남아 있으면 누구를 보는지 알 수 없다.
		mMercenarySelectedSlot = INDEX_NONE;
		mInspectedAllyUnitId = INDEX_NONE;
		SetMercenaryInventoryShown(false);
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
	RefreshWorldGestureInputBlock();
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
		// 남의 레일(들여다보기)에서는 이동 카드가 구경용이다(0823 검수).
		if (mUIModel->IsSkillRailOwnTurn() == false)
		{
			return;
		}
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
	// 카드가 열리는 즉시 좌하단 Skill을 비활성 Confirm으로 교체하고, 카드가
	// 닫히면 원래 Skill로 되돌린다. 전투 단계 변화가 없어도 UI 상태가 바로
	// 반영되어야 하므로 TurnUI 갱신을 기다리지 않는다.
	RefreshActionButtons();
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
	TSharedPtr<FPresentationBarrier> Barrier)
{
	// 이전 행동의 "다음 틱에 다시 펴기" 예약이 남아 있어도 새 턴 상태보다
	// 늦게 적용되지 않게 한다.
	++mActionPresentationSerial;
	mIsActionPlaying = false;
	mIsTurnActive = false;
	SetCommandsShown(false);

	// 이름 고지가 끝난 다음에만 입력 가능한 스킬 UI를 연다.
	if (PlayCombatAnnouncement(GetCurrentTurnAnnouncementText(),
		ECombatAnnouncementKind::TurnStart, MoveTemp(Barrier)))
	{
		return;
	}
	CompleteTurnPresentationBegin();
}

bool UCombatLayoutHUDWidget::IsWorldInputModalShown() const
{
	return IsMercenaryPanelShown() || IsMonsterTabShown()
		|| IsDetailOverlayShown();
}

void UCombatLayoutHUDWidget::RefreshWorldGestureInputBlock()
{
	APlayerController* OwningPlayer = GetOwningPlayer();
	ACombatCameraPawn* CameraPawn = OwningPlayer != nullptr
		? Cast<ACombatCameraPawn>(OwningPlayer->GetPawn()) : nullptr;
	if (CameraPawn == nullptr)
	{
		CameraPawn = UCameraFunctionLibrary::GetMainCameraPawn(this);
	}
	if (CameraPawn != nullptr)
	{
		CameraPawn->SetTouchGestureInputEnabled(IsWorldInputModalShown() == false);
	}
}

/**
 * @brief 용병 패널의 스킬 그림 위에 한 번 탭하는 투명 버튼을 보장한다.
 *
 * @details 일부 기존 WBP에는 Frame/Icon/Cost만 있고 Button이 빠져 있었다.
 * 런타임 바인딩 코드는 버튼 이름을 찾으므로, 이 상태에서는 그림을 눌러도
 * 이벤트가 만들어지지 않는다. WBP를 다시 굽기 전의 자산도 안전하게 동작하게
 * 프레임과 같은 Canvas 슬롯을 복제한다.
 */
void UCombatLayoutHUDWidget::EnsureMercenarySkillButtons()
{
	if (WidgetTree == nullptr)
	{
		return;
	}

	for (int32 Index = 0; Index < CommandSlotCount; ++Index)
	{
		const FName ButtonName(*FString::Printf(TEXT("MercenarySkillButton_%d"), Index));
		if (WidgetTree->FindWidget(ButtonName) != nullptr)
		{
			continue;
		}

		UWidget* Frame = WidgetTree->FindWidget(FName(*FString::Printf(
			TEXT("MercenarySkillFrame_%d"), Index)));
		UCanvasPanel* CanvasParent = Frame != nullptr
			? Cast<UCanvasPanel>(Frame->GetParent()) : nullptr;
		UOverlay* OverlayParent = Frame != nullptr
			? Cast<UOverlay>(Frame->GetParent()) : nullptr;
		const UCanvasPanelSlot* FrameSlot = Frame != nullptr
			? Cast<UCanvasPanelSlot>(Frame->Slot) : nullptr;
		if (Frame == nullptr || (CanvasParent == nullptr && OverlayParent == nullptr))
		{
			continue;
		}

		UButton* Button = WidgetTree->ConstructWidget<UButton>(
			UButton::StaticClass(), ButtonName);
		FButtonStyle Style = Button->GetStyle();
		Style.Normal.DrawAs = ESlateBrushDrawType::NoDrawType;
		Style.Hovered.DrawAs = ESlateBrushDrawType::NoDrawType;
		Style.Pressed.DrawAs = ESlateBrushDrawType::NoDrawType;
		Style.Disabled.DrawAs = ESlateBrushDrawType::NoDrawType;
		Button->SetStyle(Style);
		Button->SetTouchMethod(EButtonTouchMethod::PreciseTap);
		Button->SetClickMethod(EButtonClickMethod::PreciseClick);

		if (CanvasParent != nullptr && FrameSlot != nullptr)
		{
			UCanvasPanelSlot* ButtonSlot = CanvasParent->AddChildToCanvas(Button);
			ButtonSlot->SetAnchors(FrameSlot->GetAnchors());
			ButtonSlot->SetAlignment(FrameSlot->GetAlignment());
			ButtonSlot->SetAutoSize(false);
			ButtonSlot->SetPosition(FrameSlot->GetPosition());
			ButtonSlot->SetSize(FrameSlot->GetSize());
			ButtonSlot->SetZOrder(FrameSlot->GetZOrder() + 20);
		}
		else if (OverlayParent != nullptr)
		{
			if (UOverlaySlot* ButtonSlot = OverlayParent->AddChildToOverlay(Button))
			{
				ButtonSlot->SetPadding(FMargin(0.f));
				ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
				ButtonSlot->SetVerticalAlignment(VAlign_Fill);
			}
		}
	}
}

void UCombatLayoutHUDWidget::CompleteTurnPresentationBegin()
{
	mIsTurnActive = true;

	// 큰 카드 고리는 자동으로 열지 않는다. 좌하단 스킬 버튼을 누를 때만
	// RefreshCommandVisibility가 큰 카드 고리를 연다.
	const bool bPlayerTurn = IsPlayerTurn();
	SetCommandsShown(false);
	if (bPlayerTurn == true && mUIModel != nullptr)
	{
		const int32 TurnUnitId = mUIModel->GetTurnUI().mCurrentUnitId;
		RequestCameraFocus(TurnUnitId, /*bWithCommandRing=*/false);
	}
}

void UCombatLayoutHUDWidget::HandleTurnPresentationEnd(
	TSharedPtr<FPresentationBarrier> /*Barrier*/)
{
	// TurnUI는 다음 턴이 실제로 시작될 때까지 방금 끝난 아군을 가리킨다.
	// 그 스냅샷과 무관하게 종료 알림 즉시 카드를 내린다.
	++mActionPresentationSerial;
	mIsActionPlaying = false;
	mIsTurnActive = false;
	// 상태까지 접는다. 가리기만 하면 다음 턴이 열리는 순간 카드가 저절로
	// 되살아난다 -- 카드는 스킬 단추로만 연다는 계약(0807)이 깨진다.
	SetCommandsShown(false);
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
	mInitialFocusAnchorRegistered = false;
	if (mUIModel != nullptr)
	{
		++mActionPresentationSerial;
		mIsTurnActive = false;
		mIsActionPlaying = false;

		mUIModel->OnBeginCombat.RemoveAll(this);
		mBeginCombatHandle = mUIModel->OnBeginCombat.AddWeakLambda(this,
			[this](TSharedPtr<FPresentationBarrier> Barrier)
			{
				mIsTurnActive = false;
				SetCommandsShown(false);
				PlayCombatAnnouncement(
					NSLOCTEXT("CombatAnnouncement", "BattleStart", "BATTLE START"),
					ECombatAnnouncementKind::CombatStart, MoveTemp(Barrier));
			});

		mTurnBeginHandle = mUIModel->OnBeginAnyTurn.AddUObject(
			this, &UCombatLayoutHUDWidget::HandleTurnPresentationBegin);
		mTurnEndHandle = mUIModel->OnEndAnyTurn.AddUObject(
			this, &UCombatLayoutHUDWidget::HandleTurnPresentationEnd);
		mActionBeginHandle = mUIModel->OnBeginAnyTurnAction.AddUObject(
			this, &UCombatLayoutHUDWidget::HandleActionPresentationBegin);
		mActionEndHandle = mUIModel->OnEndAnyTurnAction.AddUObject(
			this, &UCombatLayoutHUDWidget::HandleActionPresentationEnd);
		mSkillCutInHandle = mUIModel->OnPrePlaySkillCutIn.AddUObject(
			this, &UCombatLayoutHUDWidget::HandlePrePlaySkillCutIn);

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
		mUIModel->OnSaveAndExitCompleted.AddUniqueDynamic(
			this, &UCombatLayoutHUDWidget::HandleSaveAndExitCompleted);
		mUIModel->OnAbandonRunCompleted.AddUniqueDynamic(
			this, &UCombatLayoutHUDWidget::HandleAbandonRunCompleted);

		// 맞은 자리 위로 뜨는 피해 숫자.
		mUIModel->OnCombatFloatingLog.AddUniqueDynamic(
			this, &UCombatLayoutHUDWidget::HandleCombatFloatingLog);
		mUIModel->OnCombatFloatingLogMotionFinished.AddUniqueDynamic(
			this, &UCombatLayoutHUDWidget::HandleCombatFloatingLogMotionFinished);
		mUIModel->OnCombatFloatingLogsCleared.AddUniqueDynamic(
			this, &UCombatLayoutHUDWidget::HandleCombatFloatingLogsCleared);

		// 미리보기 로그는 예측 전용 모델에서 따로 받는다 — 실전 표시와 저장 자리가 다르다.
		if (USimulationPreviewUIModel* SimulationPreviewUIModel = mUIModel->GetSimulationPreviewUIModel())
		{
			SimulationPreviewUIModel->OnPreviewEventBatch.AddUniqueDynamic(
				this, &UCombatLayoutHUDWidget::HandleSimulationPreviewBatch);
			SimulationPreviewUIModel->OnPreviewCleared.AddUniqueDynamic(
				this, &UCombatLayoutHUDWidget::HandleSimulationPreviewCleared);
		}

		// 라운드 고지도 전투/턴 고지와 같은 텍스트 연출을 쓴다. 예전 33장
		// 프레임 시퀀스는 쓰지 않으며, 고지 타이머가 배리어 해제를 보장한다.
		mUIModel->OnBeginAnyRound.RemoveAll(this);
		mBeginRoundHandle = mUIModel->OnBeginAnyRound.AddWeakLambda(this,
			[this](TSharedPtr<FPresentationBarrier> Barrier)
			{
				if (mPlayRoundBanner == false)
				{
					return;
				}
				const int32 Round = mUIModel != nullptr
					? FMath::Max(1, mUIModel->GetTurnUI().mRound) : 1;
				PlayCombatAnnouncement(FText::Format(
					NSLOCTEXT("CombatAnnouncement", "Round", "ROUND {0}"),
					FText::AsNumber(Round)), ECombatAnnouncementKind::RoundStart,
					MoveTemp(Barrier));
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
	if (mCombatRewardConceptWidget != nullptr)
	{
		mCombatRewardConceptWidget->BindUIModel(mCombatRewardUIModel);
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
		mUIModel->OnPrePlaySkillCutIn.Remove(mSkillCutInHandle);
		mUIModel->OnBeginCombat.Remove(mBeginCombatHandle);
		mUIModel->OnEndCombat.Remove(mEndCombatHandle);
		mUIModel->OnCombatResultOpenRequested.RemoveDynamic(
			this, &UCombatLayoutHUDWidget::HandleCombatResultOpenRequested);
		mUIModel->OnSaveAndExitCompleted.RemoveDynamic(
			this, &UCombatLayoutHUDWidget::HandleSaveAndExitCompleted);
		mUIModel->OnAbandonRunCompleted.RemoveDynamic(
			this, &UCombatLayoutHUDWidget::HandleAbandonRunCompleted);
		mUIModel->OnCombatFloatingLog.RemoveDynamic(
			this, &UCombatLayoutHUDWidget::HandleCombatFloatingLog);
		mUIModel->OnCombatFloatingLogMotionFinished.RemoveDynamic(
			this, &UCombatLayoutHUDWidget::HandleCombatFloatingLogMotionFinished);
		mUIModel->OnCombatFloatingLogsCleared.RemoveDynamic(
			this, &UCombatLayoutHUDWidget::HandleCombatFloatingLogsCleared);
		if (USimulationPreviewUIModel* SimulationPreviewUIModel = mUIModel->GetSimulationPreviewUIModel())
		{
			SimulationPreviewUIModel->OnPreviewEventBatch.RemoveDynamic(
				this, &UCombatLayoutHUDWidget::HandleSimulationPreviewBatch);
			SimulationPreviewUIModel->OnPreviewCleared.RemoveDynamic(
				this, &UCombatLayoutHUDWidget::HandleSimulationPreviewCleared);
		}
		mUIModel->OnBeginAnyRound.Remove(mBeginRoundHandle);
	}
	++mActionPresentationSerial;
	mIsTurnActive = false;
	mIsActionPlaying = false;
	mTurnBeginHandle.Reset();
	mTurnEndHandle.Reset();
	mActionBeginHandle.Reset();
	mActionEndHandle.Reset();
	mSkillCutInHandle.Reset();
	mBeginCombatHandle.Reset();
	mCombatAnnouncementBarrier.Reset();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(mSkillCutInSafetyTimerHandle);
	}
	mSkillCutInPlaying = false;
	if (IsValid(mSkillCutInWidget))
	{
		mSkillCutInWidget->StopCutIn(/*bNotifyCompletion=*/false);
		mSkillCutInWidget->RemoveFromParent();
		mSkillCutInWidget = nullptr;
	}
	Super::UnbindUIModel();

	// Reset으로 스킬이 동기 재개될 수 있으므로 Unbind의 마지막 작업이다.
	TSharedPtr<FPresentationBarrier> PrimaryBarrier = MoveTemp(mSkillCutInBarrier);
	TArray<TSharedPtr<FPresentationBarrier>> OverlappingBarriers = MoveTemp(mOverlappingSkillCutInBarriers);
	mOverlappingSkillCutInBarriers.Reset();
	PrimaryBarrier.Reset();
	for (TSharedPtr<FPresentationBarrier>& OverlappingBarrier : OverlappingBarriers)
	{
		OverlappingBarrier.Reset();
	}
}

bool UCombatLayoutHUDWidget::IsAiming() const
{
	return mUIModel != nullptr
		&& mUIModel->GetTurnUI().mPhase != ECombatBuildPhaseUI::None;
}

/**
 * @brief 확정·취소·턴 종료 단추를 지금 단계에 맞춘다.
 *
 * @details
 * 스킬을 고른 즉시 확정을 최종 위치에 회색으로 보여 주고, 대상·방향 선택이
 * 끝나 Preview가 된 뒤에만 활성화한다. 조준 중 턴 종료는 숨기며, 그 자리를
 * 취소로 바꾸지 않고 확정 오른쪽의 별도 붉은 취소 단추를 사용한다.
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
	PollTurnBarMouseSwipe();
	if (mSkillWorldPreviewActive)
	{
		SyncSkillWorldPreviewCamera(false);
	}
	RefreshScreenScale();
	if (mDetailPresenter != nullptr)
	{
		mDetailPresenter->RefreshResponsiveLayout();
	}
	// 머리 위 바는 월드 자리를 따라가야 하므로 매 프레임 다시 붙인다.
	UpdateUnitHpBars();
	UpdateCommandRevealAnimation(DeltaTime);
	RefreshPendingAPGlow(DeltaTime);
	UpdateFloatingCombatLogQueue(DeltaTime);
	UpdateFloatingCombatLogs(DeltaTime);
	UpdateCombatAnnouncement(DeltaTime);
	// 해상도/레터박스 변화 시 앵커 비율이 달라질 수 있어 다시 등록한다.
	const FVector2D LocalSize = MyGeometry.GetLocalSize();
	if (mUIModel != nullptr && LocalSize.X > 0.0f && LocalSize.Y > 0.0f
		&& (mInitialFocusAnchorRegistered == false
			|| FMath::Abs(LocalSize.X - mLastFocusAnchorLocalSize.X) > 0.5f
			|| FMath::Abs(LocalSize.Y - mLastFocusAnchorLocalSize.Y) > 0.5f))
	{
		mInitialFocusAnchorRegistered = true;
		mLastFocusAnchorLocalSize = LocalSize;
		mUIModel->SetFocusScreenAnchor(ComputeCommandRingAnchor());
	}
}

FReply UCombatLayoutHUDWidget::NativeOnPreviewMouseButtonDown(
	const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	mTurnSwipeTracking = false;
	mTurnSwipeConsumed = false;
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton
		&& mTurnPanel != nullptr
		&& mTurnPanel->GetVisibility() != ESlateVisibility::Collapsed
		&& mTurnPanel->GetVisibility() != ESlateVisibility::Hidden)
	{
		mTurnSwipeOrigin = FVector2D(InMouseEvent.GetScreenSpacePosition());
		mTurnSwipeTracking = mTurnPanel->GetCachedGeometry().IsUnderLocation(
			mTurnSwipeOrigin);
	}
	// Do not steal a tap: turn-token buttons still open their normal detail action.
	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UCombatLayoutHUDWidget::NativeOnMouseMove(const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton)
		&& TryConsumeTurnSwipe(FVector2D(InMouseEvent.GetScreenSpacePosition())))
	{
		return FReply::Handled();
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

bool UCombatLayoutHUDWidget::TryConsumeTurnSwipe(
	const FVector2D& ScreenPosition)
{
	if (!mTurnSwipeTracking || mTurnSwipeConsumed)
	{
		return false;
	}
	const FVector2D Delta = ScreenPosition - mTurnSwipeOrigin;
	if (FMath::Abs(Delta.X) < TurnSwipeSlack
		|| FMath::Abs(Delta.X) <= FMath::Abs(Delta.Y))
	{
		return false;
	}
	mTurnSwipeConsumed = true;
	mTurnSwipeTracking = false;
	if (Delta.X < 0.f)
	{
		HandleTurnPageRightClicked();
	}
	else
	{
		HandleTurnPageLeftClicked();
	}
	return true;
}

void UCombatLayoutHUDWidget::PollTurnBarMouseSwipe()
{
	if (!mTurnSwipeTracking || mTurnSwipeConsumed
		|| !FSlateApplication::IsInitialized())
	{
		return;
	}
	const TSet<FKey>& PressedButtons =
		FSlateApplication::Get().GetPressedMouseButtons();
	if (!PressedButtons.Contains(EKeys::LeftMouseButton))
	{
		mTurnSwipeTracking = false;
		return;
	}
	// PIE에서는 토큰 SButton이 마우스를 캡처해 부모 NativeOnMouseMove가 오지
	// 않을 수 있다. Slate의 실제 커서 좌표를 Tick에서 같이 읽어 같은 판정을
	// 수행하면 자식 캡처 여부와 무관하게 드래그가 동작한다.
	TryConsumeTurnSwipe(FSlateApplication::Get().GetCursorPos());
}

void UCombatLayoutHUDWidget::RefreshScreenScale()
{
	/*
	 * 카드/파티는 WBP 저작 크기를 그대로 쓴다. 해상도 대응은 전역 DPI 규칙
	 * 하나만 맡고 이 레이어에는 어떤 추가 배율도 적용하지 않는다.
	 */
	if (mCommandLayer != nullptr)
	{
		mCommandLayer->SetUserSpecifiedScale(1.f);
	}
	if (mPartyLayer != nullptr)
	{
		mPartyLayer->SetUserSpecifiedScale(1.f);
	}
}

void UCombatLayoutHUDWidget::RefreshActionButtons()
{
	ApplyActionLabelOpticalAlignment();

	const ECombatBuildPhaseUI Phase = mUIModel != nullptr
		? mUIModel->GetTurnUI().mPhase : ECombatBuildPhaseUI::None;
	const bool bBrowsing = Phase == ECombatBuildPhaseUI::None;
	const bool bTargeting = !bBrowsing;
	// 열기 요청만으로는 부족하다. 적 턴, 행동 연출, 용병/몬스터 패널처럼
	// 실제 카드 표시 조건에서 막힌 경우에는 Skill을 Confirm으로 바꾸지 않는다.
	const bool bSkillSelectionOpen = bBrowsing && mCommandsVisibleLastFrame;
	const bool bCanConfirm = Phase == ECombatBuildPhaseUI::Preview;

	// 스킬을 고른 순간부터 확정 단추의 최종 위치를 보여 준다. 대상/방향 등
	// 필요한 선택이 덜 끝난 동안에는 패널 전체를 비활성화해 회색으로 그리고,
	// 실제 Preview 단계에 들어온 뒤에만 누를 수 있게 한다.
	SetShown(mConfirmPanel, bTargeting || bSkillSelectionOpen);
	// 판은 장식이라 SelfHitTestInvisible 로 두지만, 그 안의 확정 단추는
	// 실제로 눌려야 한다. 에셋 기본값이 무엇이든 여기서 히트를 살린다 --
	// 안 살리면 눌림이 뿌리로 새어 판 탭(무르기)이 되어, 확정을 눌렀는데
	// 경로만 사라진다.
	SetInteractiveShown(mConfirmButton, bTargeting || bSkillSelectionOpen);
	if (mConfirmPanel != nullptr)
	{
		mConfirmPanel->SetIsEnabled(bCanConfirm);
	}
	if (mConfirmButton != nullptr)
	{
		mConfirmButton->SetIsEnabled(bCanConfirm);
	}
	// 턴 종료는 어떤 상태에서도 취소로 변신하지 않는다. 조준 중에는 통째로
	// 숨기고, 좌하단 확정 오른쪽의 붉은 전용 취소 단추만 켜 근육 기억
	// 실수를 막는다.
	SetShown(mEndTurnPanel, bBrowsing);
	SetInteractiveShown(mEndTurnButton, bBrowsing);
	SetTextIfPresent(mEndTurnLabel, LOCTEXT("EndTurn", "턴 종료"));
	SetShown(mCancelPanel, bTargeting);
	SetInteractiveShown(mCancelButton, bTargeting);
	SetTextIfPresent(mCancelLabel, LOCTEXT("CancelAim", "취소"));

	// 조준에 들어가면 스킬 단추는 비킨다 -- 같은 자리에 확정 단추가 선다
	// (0807 검수: 둘이 겹침). 조준 중 카드 여닫기는 어차피 막혀 있다.
	const bool bSkillButtonShown = bBrowsing && !bSkillSelectionOpen;
	SetShown(mSkillTogglePanel, bSkillButtonShown);
	SetInteractiveShown(mSkillToggleButton, bSkillButtonShown);
	SetShown(mSkillTogglePlate, bSkillButtonShown);
	SetShown(mSkillToggleLabel, bSkillButtonShown);
}

void UCombatLayoutHUDWidget::ApplyActionLabelOpticalAlignment()
{
	// Overlay/TextBlock의 레이아웃 박스는 버튼 중심과 0.5px 이내로 이미 맞는다.
	// 다만 합성 글꼴의 ascent/descent 때문에 실제 잉크는 ko 약 5px, en 약
	// 2.5px 아래에 그려진다. 라벨 슬롯의 아래 여백을 더 줘 같은 양만 위로
	// 올린다. 문자열이 동적으로 바뀌어도 같은 언어/글꼴 메트릭을 그대로 쓴다.
	const FString CultureName =
		FInternationalization::Get().GetCurrentCulture()->GetName();
	const float OffsetY = CultureName.StartsWith(TEXT("ko"), ESearchCase::IgnoreCase)
		? -5.f : -2.5f;

	auto ApplyOffset = [OffsetY](UWidget* Label)
	{
		if (Label == nullptr)
		{
			return;
		}
		if (UOverlaySlot* Slot = Cast<UOverlaySlot>(Label->Slot))
		{
			const FMargin Padding = Slot->GetPadding();
			Slot->SetPadding(FMargin(Padding.Left, Padding.Top, Padding.Right,
				Padding.Top - 2.f * OffsetY));
		}
	};

	ApplyOffset(mSkillToggleLabel);
	ApplyOffset(mEndTurnLabel);
	ApplyOffset(mCancelLabel);
}

#if WITH_EDITOR
void UCombatLayoutHUDWidget::ApplyActionLabelOpticalAlignmentForCapture()
{
	CacheAuthoredWidgets();
	ApplyActionLabelOpticalAlignment();
}
#endif

/** @brief 좌상단 AP 막대를 지금 차례인 유닛으로 채운다. */
void UCombatLayoutHUDWidget::RefreshTurnActionPoints()
{
	const FUnitUI* TurnUnit = FindTurnUnit();
	const bool bShowPlayerActionPoints = TurnUnit != nullptr && TurnUnit->mIsPlayer;
	SetShown(mTurnAPRoot, bShowPlayerActionPoints);
	if (bShowPlayerActionPoints == false)
	{
		mShownAPLeft = 0;
		mShownAPTotal = 0;
		mPendingAPCost = 0;
		RefreshPendingAPGlow(0.f);
		return;
	}
	const int32 Left = TurnUnit != nullptr
		? mUIModel->GetDisplayedMovementPoint(*TurnUnit) : 0;
	const int32 Total = TurnUnit != nullptr
		? FMath::Max(FMath::RoundToInt(TurnUnit->mMaxMovementPoint), Left) : 0;

	SetTextIfPresent(mTurnAPText, FText::FromString(
		FString::Printf(TEXT("%d/%d"), Left, Total)));
	mShownAPLeft = Left;

	// 고른 카드가 가져갈 몫. 남은 것보다 크면 남은 만큼만 빛낸다.
	const int32 NewPendingAPCost = FMath::Clamp(GetPendingActionCost(), 0, Left);
	if (NewPendingAPCost > 0 && NewPendingAPCost != mPendingAPCost)
	{
		// 고른 몫 전체를 먼저 어둡게 보여 개수를 고정해 둔다. 첫 칸부터
		// 어두움 -> 보통 -> 빛남 순서로 진행하고 마지막 뒤에는 짧게 쉰다.
		mAPGlowElapsed = 0.f;
	}
	mPendingAPCost = NewPendingAPCost;

	/*
	 * 칸보다 AP 가 많으면 **칸 수에서 멈춘다**(0806 합의, 0811 재확인).
	 * 칸은 15개까지만 보여 주고 넘치는 몫은 옆의 숫자가 말한다 -- 전에는
	 * 넘치면 아이콘을 전부 접었는데, 그러면 18/18 인데 빈 막대만 남아
	 * "AP 없음" 으로 읽혔다.
	 */
	const int32 Room = mTurnAPPips.Num();
	const int32 ShownTotal = FMath::Min(Total, Room);
	const int32 ShownLeft = FMath::Min(Left, Room);
	mShownAPTotal = ShownTotal;
	for (int32 Pip = 0; Pip < Room; ++Pip)
	{
		const bool bHasPip = Pip < ShownTotal;
		// 소모 예정 강조는 왼쪽부터 진행하지만 실제 AP가 줄 때는 오른쪽
		// 보석부터 빈 보석으로 바뀐다. 남은 개수가 항상 왼쪽에 붙어 있어
		// 현재 보유량을 바로 셀 수 있다.
		SetShown(mTurnAPPips[Pip], bHasPip && Pip < ShownLeft);
		if (mTurnAPPipsUsed.IsValidIndex(Pip))
		{
			SetShown(mTurnAPPipsUsed[Pip], bHasPip && Pip >= ShownLeft);
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
 * @brief 가져갈 몫을 어둡게 고정하고 왼쪽부터 차례로 번쩍인다.
 *
 * @details
 * 예정 비용은 **왼쪽부터** 빛내 빠르게 셀 수 있게 한다. 확정 뒤 실제 AP는
 * 오른쪽부터 빈 보석으로 바뀌어 남은 보석이 항상 왼쪽에 붙는다.
 *
 * 예정된 칸은 모두 어두워서 비용 개수가 계속 보인다. 위치와 크기는
 * 움직이지 않고 각 칸의 원본 보석이 어두움 -> 보통 -> 빛남 순서로만
 * 변한다. 별도 기호나 파티클은 사용하지 않는다.
 *
 * @param DeltaTime 지난 시간. 0 이면 위상은 그대로 두고 다시 칠하기만 한다
 */
void UCombatLayoutHUDWidget::RefreshPendingAPGlow(const float DeltaTime)
{
	const int32 Room = mTurnAPPips.Num();
	const int32 Left = mShownAPLeft;
	const int32 ShownLeft = FMath::Min(Left, Room);
	// 15칸을 넘는 AP는 숫자만 맡지만 예정 비용 피드백까지 숨기면 안 된다.
	// 예: 18/18에서 비용 1~2인 이동도 실제 칸 감소 여부와 별개로 보이는 묶음의
	// 왼쪽부터 비용만큼 강조해 사용량을 항상 읽을 수 있게 한다.
	const int32 VisiblePending = FMath::Clamp(mPendingAPCost, 0, ShownLeft);
	if (VisiblePending > 0)
	{
		const float CycleDuration = (VisiblePending - 1) * APGlowStagger
			+ APGlowFlashDuration + APGlowRepeatGap;
		mAPGlowElapsed = FMath::Fmod(
			mAPGlowElapsed + FMath::Max(DeltaTime, 0.f), CycleDuration);
	}
	else
	{
		mAPGlowElapsed = 0.f;
	}

	for (int32 Pip = 0; Pip < Room; ++Pip)
	{
		UWidget* PipWidget = mTurnAPPips[Pip];
		if (PipWidget == nullptr)
		{
			continue;
		}
		const bool bWillSpend = VisiblePending > 0 && Pip < VisiblePending;
		const int32 PendingIndex = Pip;
		const float FlashAge = bWillSpend
			? mAPGlowElapsed - PendingIndex * APGlowStagger : -1.f;
		float FlashIntensity = 0.f;
		float NormalIntensity = 0.f;
		if (FlashAge >= 0.f && FlashAge < APGlowFlashDuration)
		{
			if (FlashAge < APGlowNormalTime)
			{
				NormalIntensity = FlashAge / APGlowNormalTime;
			}
			else if (FlashAge <= APGlowFlashPeakTime)
			{
				NormalIntensity = 1.f;
				FlashIntensity = (FlashAge - APGlowNormalTime)
					/ (APGlowFlashPeakTime - APGlowNormalTime);
			}
			else
			{
				const float Fade = 1.f - (FlashAge - APGlowFlashPeakTime)
					/ (APGlowFlashDuration - APGlowFlashPeakTime);
				NormalIntensity = FMath::Clamp(Fade, 0.f, 1.f);
				FlashIntensity = FMath::Square(NormalIntensity);
			}
		}

		const bool bEffectVisible = bWillSpend && FlashIntensity > KINDA_SMALL_NUMBER;
		// 제곱 감쇠된 원 파형을 그대로 쓰면 최고 밝기가 몇 프레임뿐이다.
		// 밝기만 제곱근으로 넓혀 01안의 짧고 확실한 플래시를 남긴다.
		const float FlashOpacity = FMath::Sqrt(FlashIntensity);

		// 소모 예정 칸 전체는 같은 어두운 색으로 유지해 비용 개수가 한눈에
		// 보인다. 현재 순서의 칸만 보통 밝기를 거쳐 점등한다. 크기와 위치는
		// 절대 바꾸지 않는다.
		PipWidget->SetRenderOpacity(1.f);
		PipWidget->SetRenderTransformPivot(FVector2D(.5f));
		PipWidget->SetRenderScale(FVector2D(1.f));
		PipWidget->SetRenderTranslation(FVector2D::ZeroVector);
		if (UImage* PipImage = Cast<UImage>(PipWidget))
		{
			const FLinearColor PendingDark(.34f, .42f, .56f, 1.f);
			PipImage->SetColorAndOpacity(bWillSpend
				? FMath::Lerp(PendingDark, FLinearColor::White, NormalIntensity)
				: FLinearColor::White);
		}
		UWidget* GlowWidget = mTurnAPPipGlows.IsValidIndex(Pip)
			? mTurnAPPipGlows[Pip].Get() : nullptr;
		SetShown(GlowWidget, bEffectVisible);
		if (GlowWidget != nullptr)
		{
			GlowWidget->SetRenderOpacity(bEffectVisible ? FlashOpacity : 0.f);
			GlowWidget->SetRenderScale(FVector2D(1.f));
			GlowWidget->SetRenderTranslation(FVector2D::ZeroVector);
			if (UImage* GlowImage = Cast<UImage>(GlowWidget))
			{
				GlowImage->SetColorAndOpacity(FLinearColor::White);
			}
		}
	}
}

/** @brief 확정 단추를 눌렀다. 겨냥한 칸을 그대로 확정한다. */
void UCombatLayoutHUDWidget::HandleConfirmClicked()
{
	// 비활성 버튼의 델리게이트를 코드나 자동화에서 직접 호출해도, 대상 선택이
	// 끝나기 전에는 Confirm 의도가 게임플레이로 새지 않게 이중으로 막는다.
	if (mUIModel != nullptr
		&& mUIModel->GetTurnUI().mPhase == ECombatBuildPhaseUI::Preview)
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

	// 접혀 있다가 다시 펴지는 순간마다 등장 연출을 처음부터 재생한다.
	if (bVisible == true && mCommandsVisibleLastFrame == false)
	{
		RestartCommandRevealAnimation();
	}
	mCommandsVisibleLastFrame = bVisible;

	for (const FCommandSlotWidgets& Widgets : mCommandSlots)
	{
		SetShown(Widgets.Root, bVisible);
	}
	if (bVisible == false)
	{
		// 접힌 카드는 다음에 펼 때까지 연출 값을 들고 있을 필요가 없다.
		mCommandRevealElapsed = -1.f;
		return;
	}
	// 켜는 프레임에 첫 값을 미리 발라 둔다. 다음 Tick 까지 한 프레임 동안
	// 제 크기로 번쩍 떴다가 줄어드는 것을 막는다. 연출을 안 거는 곳
	// (편집기 캡처)에서는 아무 값도 건드리지 않는다.
	UpdateCommandRevealAnimation(0.f);
}

/**
 * @brief 다음 표시에서 카드 등장 연출을 처음부터 재생한다.
 *
 * @details 틱이 도는 곳에서만 재생한다. 편집기 캡처는 한 프레임만 그리고
 * 끝나므로, 거기서 연출을 걸면 카드가 시작 상태(투명 + 0.86배)로 굳어
 * 찍힌 그림에서 사라진다 -- 실제로 그렇게 나왔다(0824). 연출은 눈으로
 * 보는 값이지 배치가 아니므로, 못 돌릴 곳에서는 아예 걸지 않는다.
 */
void UCombatLayoutHUDWidget::RestartCommandRevealAnimation()
{
	const UWorld* World = GetWorld();
	if (World == nullptr || World->IsGameWorld() == false)
	{
		mCommandRevealElapsed = -1.f;
		return;
	}
	mCommandRevealElapsed = 0.f;
}

/**
 * @brief 카드 등장 연출 한 프레임.
 *
 * @details 카드마다 CommandRevealStagger 만큼 늦게 시작해
 * CommandRevealDuration 동안 0.86배 -> 1배로 커지며 투명도가 0 -> 1 이 된다.
 * 크기는 카드 한가운데를 축으로 바꾼다 -- 카드 고리의 자리는 그대로 두고
 * 카드만 부푼다.
 *
 * 연출이 끝나면 값을 원본(불투명, 배율 1)으로 되돌리고 더는 만지지 않는다.
 * 그래야 눌림 축소 같은 다른 연출과 다투지 않는다.
 */
void UCombatLayoutHUDWidget::UpdateCommandRevealAnimation(const float InDeltaTime)
{
	if (mCommandRevealElapsed < 0.f)
	{
		return;
	}
	mCommandRevealElapsed += InDeltaTime;

	bool bAnyPlaying = false;
	for (int32 SlotIndex = 0; SlotIndex < mCommandSlots.Num(); ++SlotIndex)
	{
		UWidget* Root = mCommandSlots[SlotIndex].Root;
		if (Root == nullptr)
		{
			continue;
		}
		const float SlotElapsed = mCommandRevealElapsed
			- CommandRevealStagger * SlotIndex;
		const float Alpha = FMath::Clamp(SlotElapsed / CommandRevealDuration, 0.f, 1.f);
		if (Alpha < 1.f)
		{
			bAnyPlaying = true;
		}
		// 뒤로 갈수록 느려지는 곡선. 마지막에 제자리에 앉는 느낌이 난다.
		const float Eased = 1.f - FMath::Pow(1.f - Alpha, 3.f);
		Root->SetRenderOpacity(Eased);
		FWidgetTransform Transform;
		const float Scale = FMath::Lerp(CommandRevealStartScale, 1.f, Eased);
		Transform.Scale = FVector2D(Scale, Scale);
		Root->SetRenderTransformPivot(FVector2D(.5f, .5f));
		Root->SetRenderTransform(Transform);
	}

	if (bAnyPlaying == false)
	{
		// 다 앉았다. 원본 값으로 돌려 놓고 연출을 끈다.
		for (const FCommandSlotWidgets& Widgets : mCommandSlots)
		{
			if (Widgets.Root != nullptr)
			{
				Widgets.Root->SetRenderOpacity(1.f);
				Widgets.Root->SetRenderTransform(FWidgetTransform());
			}
		}
		mCommandRevealElapsed = -1.f;
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
	if (IsWorldInputModalShown())
	{
		return FReply::Handled();
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
	if (IsWorldInputModalShown())
	{
		return FReply::Handled();
	}
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
		|| IsWorldInputModalShown())
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
	if (IsWorldInputModalShown())
	{
		mPressActive = false;
		mPressMoved = false;
		return FReply::Handled();
	}
	FinishBoardPress(FVector2D(InTouchEvent.GetScreenSpacePosition()));
	return Super::NativeOnTouchEnded(InGeometry, InTouchEvent);
}

FReply UCombatLayoutHUDWidget::NativeOnTouchMoved(const FGeometry& InGeometry,
	const FPointerEvent& InTouchEvent)
{
	if (IsWorldInputModalShown())
	{
		mPressActive = false;
		mPressMoved = false;
		return FReply::Handled();
	}
	if (FVector2D(InTouchEvent.GetScreenSpacePosition()).Equals(mPressOrigin, BoardTapSlack) == false)
	{
		const bool bDragJustStarted = mPressActive && mPressMoved == false;
		mPressMoved = true;
		// 끌기 시작했다. 지도를 미는 손을 긴 누름으로 오인하지 않는다.
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(mBoardLongPressTimerHandle);
		}
		// 0823 확정: 카드는 손을 뗄 때가 아니라 **끌기 시작하는 순간** 접는다.
		// 손을 뗀 뒤에 접히면 화면을 미는 내내 카드가 시야를 가린다.
		if (bDragJustStarted && IsAiming() == false && mIsActionPlaying == false
			&& IsOverChrome(mPressOrigin) == false)
		{
			SetCommandsShown(false);
		}
	}
	return Super::NativeOnTouchMoved(InGeometry, InTouchEvent);
}

FReply UCombatLayoutHUDWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (IsWorldInputModalShown())
	{
		mPressActive = false;
		mPressMoved = false;
		return FReply::Handled();
	}
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
	if (mTurnSwipeConsumed)
	{
		mPressActive = false;
		mPressMoved = false;
		return;
	}

	if (mPressActive == false)
	{
		return;
	}
	mPressActive = false;

	const FVector2D DragDelta = ScreenPosition - mPressOrigin;
	const bool bDragged = mPressMoved
		|| ScreenPosition.Equals(mPressOrigin, BoardTapSlack) == false;
	const bool bStartedOnTurnPanel = mTurnPanel != nullptr
		&& mTurnPanel->GetVisibility() != ESlateVisibility::Collapsed
		&& mTurnPanel->GetVisibility() != ESlateVisibility::Hidden
		&& mTurnPanel->GetCachedGeometry().IsUnderLocation(mPressOrigin);
	mPressMoved = false;
	if (bDragged == true)
	{
		// 턴바 위의 가로 끌기는 전장을 미는 입력이 아니라 턴 예측 페이지
		// 슬라이드다. 세로 끌기와 짧은 흔들림은 페이지를 바꾸지 않는다.
		if (bStartedOnTurnPanel
			&& FMath::Abs(DragDelta.X) >= TurnSwipeSlack
			&& FMath::Abs(DragDelta.X) > FMath::Abs(DragDelta.Y))
		{
			if (DragDelta.X < 0.f)
			{
				HandleTurnPageRightClicked();
			}
			else
			{
				HandleTurnPageLeftClicked();
			}
			return;
		}

		// 0823 확정: 지도를 끄는 것도 판을 만진 것이다. 펴 둔 카드는 접는다.
		// 조준 중(끌며 겨냥 확인)·연출 중·HUD 위에서 시작한 끌기는 건드리지
		// 않는다 -- 탭의 예외 규칙과 같다.
		if (IsAiming() == false && mIsActionPlaying == false
			&& IsOverChrome(mPressOrigin) == false)
		{
			SetCommandsShown(false);
		}
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
	if (IsMercenaryPanelShown() == true || IsMonsterTabShown() == true)
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

	// 조준 중이면 이 탭은 게임플레이 것이다. 조준 가능한 타일이면 그리로
	// 겨냥하고, 진짜 보드 밖이면 빌드 단계를 하나 무른다. 보드 안의 빈 타일이나
	// 사거리 밖 타일은 현재 단계를 유지한다. 어느 경우인지는 스킬 조준만 안다.
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
		/*
		 * 유닛을 새로 짚었다.
		 *
		 * 아군이면 요약판과 함께 **그 용병의 카드도 편다** -- 판에서 누른
		 * 손과 카드를 부르는 손이 따로면, 쓰려는 스킬을 보려고 턴 칸까지
		 * 다시 가야 한다(0806 검수).
		 *
		 * 적이면 카드를 접는다. 위협 범위를 보라고 판에 칠했는데 카드 여섯
		 * 장이 그 한가운데를 덮으면 칠이 안 보인다.
		 */
		const bool bHasUnit = AfterTarget.mIsValid == true
			&& AfterTarget.mUnitId != INDEX_NONE;
		if (bHasUnit == true)
		{
			const TArray<FUnitUI>& Units = mUIModel->GetUnitUIs();
			const int32 TappedId = AfterTarget.mUnitId;
			const FUnitUI* Tapped = Units.FindByPredicate(
				[TappedId](const FUnitUI& Candidate)
				{ return Candidate.mUnitId == TappedId; });
			const bool bIsAlly = Tapped != nullptr && Tapped->mIsPlayer;

			// 짚은 유닛을 화면 가운데로 -- 판에서 직접 누른 손도 턴 칸을
			// 누른 손과 같은 대접을 받는다(0806). 카드 고리 세부조정은
			// 카드가 뜨는 아군일 때만(0807).
			RequestCameraFocus(TappedId, /*bWithCommandRing=*/bIsAlly);

			if (bIsAlly == true)
			{
				// 카드만 갈아 끼운다. 상세 겹은 안 띄운다.
				mSuppressNextUnitDetailOverlay = true;
				mInspectedAllyUnitId = TappedId;
				mUIModel->RequestInspectUnit(TappedId);
			}
			else
			{
				// 적을 짚으면 아군 카드가 접히므로 살펴보던 아군도 놓는다.
				mInspectedAllyUnitId = INDEX_NONE;
			}
			SetCommandsShown(bIsAlly);
		}
		else
		{
			// 겨냥이 걷혔다(빈 땅 탭으로 살펴보기 해제). 카드도 함께 접는다 --
			// 요약판은 내려갔는데 카드만 남으면 화면이 안 닫힌 것처럼
			// 보인다(0806 검수: 밖을 눌러도 스킬 UI 가 안 내려감).
			SetCommandsShown(false);
		}
		return;
	}

	/*
	 * 겨냥이 그대로다 = **판 밖**(타일 없는 곳)을 눌렀다. 전부 내린다.
	 *
	 * 타일 위 탭은 어떤 경우든 겨냥을 움직인다(유닛 탭 = 짚기, 빈 타일 탭 =
	 * 살펴보기 해제, 같은 유닛 재탭 = 무르기). 그런데 판 밖 탭은 트레이스에
	 * 타일이 없어 겨냥 알림 자체가 안 오고, 그래서 여기로 온다. 이 손의
	 * 뜻은 "다 치워라"다 -- 카드도, 짚어 둔 요약판도, 위협 칠도 내린다
	 * (0806 검수: 밖을 눌러도 스킬 UI 가 안 내려감).
	 */
	SetCommandsShown(false);
	if (AfterTarget.mIsValid == true)
	{
		mUIModel->SetTarget(FCombatTargetUI());
		// 칠 걷기는 판 소관이다. INDEX_NONE = "그만 본다".
		mUIModel->RequestLongPressUnit(INDEX_NONE);
	}
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

	/*
	 * 목록에서 고르기만 한다 -- 패널은 그대로 두고 오른쪽 상세만 갈린다.
	 *
	 * 전에는 여기서 패널을 닫고 상세 겹(유닛 상세창)을 띄웠다. 그런데 그
	 * 내용은 이 패널 오른쪽이 이미 크게 보여 주고 있어서, 누를 때마다 같은
	 * 것이 다른 판으로 한 번 더 뜨고 목록이 사라졌다(0806 검수: "용병 상세탭
	 * 필요 없다"). 고른 줄만 기억하고 다시 그린다.
	 */
	SetMercenaryInventoryShown(false);
	mMercenarySelectedSlot = SlotIndex;
	HideDetailOverlay(/*bNotifyGameplay=*/false);
	RefreshParty();

	// 어느 화면에서 골랐든 "이 용병을 본다" 는 게임플레이에 알린다 -- 스킬
	// 소켓 상세가 이 유닛 기준으로 풀려야 해서다(0807 감사: 패널 선택이
	// GameMode 살펴보기 유닛과 어긋나 엉뚱한 스킬 상세가 떴다). 상세 겹은
	// 안 띄운다.
	mSuppressNextUnitDetailOverlay = true;
	mInspectedAllyUnitId = UnitId;
	mUIModel->RequestInspectUnit(UnitId);

	// 전투 화면에서 아군 칸을 누른 것은 "이 용병 스킬을 보겠다"다. 차례가
	// 와도 카드가 저절로 안 열리는 새 계약(0807)에서는 여기가 여는 손 중
	// 하나다. 용병 패널이 열려 있으면 목록 고르기일 뿐이라 안 편다.
	if (IsMercenaryPanelShown() == false)
	{
		RequestCameraFocus(UnitId, /*bWithCommandRing=*/true);
		SetCommandsShown(true);
	}
}

void UCombatLayoutHUDWidget::HandlePartyClicked_0() { HandlePartyClicked(0); }
void UCombatLayoutHUDWidget::HandlePartyClicked_1() { HandlePartyClicked(1); }
void UCombatLayoutHUDWidget::HandlePartyClicked_2() { HandlePartyClicked(2); }

void UCombatLayoutHUDWidget::HandleInventoryClicked()
{
	if (mUIModel == nullptr)
	{
		return;
	}

	// 네 번째 로스터 줄은 같은 용병 WBP 안의 인벤토리 페이지를 연다.
	// 별도 월드 위젯을 띄우지 않으므로 왼쪽 네 탭과 닫기 흐름이 그대로 남는다.
	mMercenarySelectedSlot = INDEX_NONE;
	SetMercenaryInventoryShown(true);
	HideDetailOverlay(/*bNotifyGameplay=*/false);
	RefreshParty();
	RefreshMeta();
}

void UCombatLayoutHUDWidget::HandleMercenaryInventoryArtifactClicked_0()
{
	SelectMercenaryInventoryArtifact(0);
}
void UCombatLayoutHUDWidget::HandleMercenaryInventoryArtifactClicked_1()
{
	SelectMercenaryInventoryArtifact(1);
}
void UCombatLayoutHUDWidget::HandleMercenaryInventoryArtifactClicked_2()
{
	SelectMercenaryInventoryArtifact(2);
}
void UCombatLayoutHUDWidget::HandleMercenaryInventoryArtifactClicked_3()
{
	SelectMercenaryInventoryArtifact(3);
}
void UCombatLayoutHUDWidget::HandleMercenaryInventoryArtifactClicked_4()
{
	SelectMercenaryInventoryArtifact(4);
}
void UCombatLayoutHUDWidget::HandleMercenaryInventoryArtifactClicked_5()
{
	SelectMercenaryInventoryArtifact(5);
}
void UCombatLayoutHUDWidget::HandleMercenaryInventoryArtifactClicked_6()
{
	SelectMercenaryInventoryArtifact(6);
}
void UCombatLayoutHUDWidget::HandleMercenaryInventoryArtifactClicked_7()
{
	SelectMercenaryInventoryArtifact(7);
}
void UCombatLayoutHUDWidget::HandleMercenaryInventoryArtifactClicked_8()
{
	SelectMercenaryInventoryArtifact(8);
}
void UCombatLayoutHUDWidget::HandleMercenaryInventoryArtifactClicked_9()
{
	SelectMercenaryInventoryArtifact(9);
}
void UCombatLayoutHUDWidget::HandleMercenaryInventoryArtifactClicked_10()
{
	SelectMercenaryInventoryArtifact(10);
}

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

/**
 * @brief 공용 설정 팝업을 연다. 여기서 만들지 않고 서브시스템 것을 쓴다.
 */
void UCombatLayoutHUDWidget::HandleSettingsMenuClicked()
{
	UWorld* World = GetWorld();
	UWorldWidgetSubsystem* WorldWidgetSubsystem =
		World != nullptr ? World->GetSubsystem<UWorldWidgetSubsystem>() : nullptr;
	if (WorldWidgetSubsystem == nullptr)
	{
		return;
	}
	USettingsPanelWidget* Settings = WorldWidgetSubsystem->GetWorldWidget<USettingsPanelWidget>(
		EWorldWidgetType::InGameSettings);
	if (Settings == nullptr)
	{
		return;
	}
	// SettingsPanel은 Back에서 자기 팝업을 먼저 닫고 이 이벤트를 알린다.
	// 연 쪽도 멱등 CloseUI를 유지해 오래된 WBP/Blueprint 호출 경로가 직접
	// OnBackRequested만 보내더라도 사용자가 설정 화면에 갇히지 않게 한다.
	Settings->OnBackRequested.AddUniqueDynamic(
		this, &UCombatLayoutHUDWidget::HandleSettingsPanelBackRequested);
	Settings->OnSaveAndExitRequested.AddUniqueDynamic(
		this, &UCombatLayoutHUDWidget::HandleSettingsPanelSaveAndExitRequested);
	Settings->OnAbandonRunConfirmed.AddUniqueDynamic(
		this, &UCombatLayoutHUDWidget::HandleSettingsPanelAbandonRunConfirmed);
	Settings->SetPanelMode(ESettingsPanelMode::InGame);
	// 전투 중에는 런을 저장하거나 포기할 수 있다. 타이틀에서 열 때와 다른 점이다.
	Settings->RefreshPanelState(true, true);
	Settings->HideAbandonConfirm();
	Settings->SetStatusText(FText::GetEmpty());
	CancelStatusPress();
	Settings->OpenUI();
}

/** @brief 설정 패널의 Back 요청을 받아 패널을 닫는다. */
void UCombatLayoutHUDWidget::HandleSettingsPanelBackRequested()
{
	UWorld* World = GetWorld();
	UWorldWidgetSubsystem* WorldWidgetSubsystem =
		World != nullptr ? World->GetSubsystem<UWorldWidgetSubsystem>() : nullptr;
	if (WorldWidgetSubsystem == nullptr)
	{
		return;
	}
	if (USettingsPanelWidget* Settings = WorldWidgetSubsystem->GetWorldWidget<USettingsPanelWidget>(
		EWorldWidgetType::InGameSettings))
	{
		Settings->CloseUI();
	}
}

/** @brief 런 액션을 잠그고 저장 후 종료 의도를 게임플레이에 전달한다. */
void UCombatLayoutHUDWidget::HandleSettingsPanelSaveAndExitRequested()
{
	UWorld* World = GetWorld();
	UWorldWidgetSubsystem* WorldWidgetSubsystem =
		World != nullptr ? World->GetSubsystem<UWorldWidgetSubsystem>() : nullptr;
	USettingsPanelWidget* Settings = WorldWidgetSubsystem != nullptr
		? WorldWidgetSubsystem->GetWorldWidget<USettingsPanelWidget>(
			EWorldWidgetType::InGameSettings)
		: nullptr;
	if (Settings != nullptr)
	{
		Settings->SetRunActionsEnabled(false);
		Settings->SetStatusText(LOCTEXT("SavingRun", "저장 중..."));
	}

	if (mUIModel != nullptr)
	{
		mUIModel->RequestSaveAndExitRun();
		return;
	}
	HandleSaveAndExitCompleted(false);
}

/** @brief 확인이 끝난 런 포기 요청을 기존 UIModel -> CombatGameMode 경로로 보낸다. */
void UCombatLayoutHUDWidget::HandleSettingsPanelAbandonRunConfirmed()
{
	UWorld* World = GetWorld();
	UWorldWidgetSubsystem* WorldWidgetSubsystem =
		World != nullptr ? World->GetSubsystem<UWorldWidgetSubsystem>() : nullptr;
	USettingsPanelWidget* Settings = WorldWidgetSubsystem != nullptr
		? WorldWidgetSubsystem->GetWorldWidget<USettingsPanelWidget>(
			EWorldWidgetType::InGameSettings)
		: nullptr;
	if (Settings != nullptr)
	{
		Settings->HideAbandonConfirm();
		Settings->SetRunActionsEnabled(false);
		Settings->SetStatusText(FText::GetEmpty());
	}

	if (mUIModel != nullptr)
	{
		mUIModel->RequestAbandonRun();
		return;
	}
	if (Settings != nullptr)
	{
		HandleAbandonRunCompleted(false);
	}
}

/** @brief 저장/전환 실패만 현재 화면에서 복구한다. 성공 시 방 전환이 이어진다. */
void UCombatLayoutHUDWidget::HandleSaveAndExitCompleted(const bool bSuccess)
{
	if (bSuccess)
	{
		return;
	}

	UWorld* World = GetWorld();
	UWorldWidgetSubsystem* WorldWidgetSubsystem =
		World != nullptr ? World->GetSubsystem<UWorldWidgetSubsystem>() : nullptr;
	if (WorldWidgetSubsystem == nullptr)
	{
		return;
	}
	if (USettingsPanelWidget* Settings =
		WorldWidgetSubsystem->GetWorldWidget<USettingsPanelWidget>(
			EWorldWidgetType::InGameSettings))
	{
		Settings->SetRunActionsEnabled(true);
		Settings->SetStatusText(LOCTEXT("SaveAndExitFailed", "저장 후 종료에 실패했습니다."));
	}
}

/** @brief 포기/전환 실패만 현재 화면에서 복구한다. 성공 시 방 전환이 이어진다. */
void UCombatLayoutHUDWidget::HandleAbandonRunCompleted(const bool bSuccess)
{
	if (bSuccess)
	{
		return;
	}

	UWorld* World = GetWorld();
	UWorldWidgetSubsystem* WorldWidgetSubsystem =
		World != nullptr ? World->GetSubsystem<UWorldWidgetSubsystem>() : nullptr;
	if (WorldWidgetSubsystem == nullptr)
	{
		return;
	}
	if (USettingsPanelWidget* Settings =
		WorldWidgetSubsystem->GetWorldWidget<USettingsPanelWidget>(
			EWorldWidgetType::InGameSettings))
	{
		Settings->SetRunActionsEnabled(true);
		Settings->SetStatusText(LOCTEXT("AbandonRunFailed", "런 포기 요청에 실패했습니다."));
	}
}

void UCombatLayoutHUDWidget::HandleArtifactPressed_0() { BeginArtifactPress(0); }
void UCombatLayoutHUDWidget::HandleArtifactPressed_1() { BeginArtifactPress(1); }
void UCombatLayoutHUDWidget::HandleArtifactPressed_2() { BeginArtifactPress(2); }
void UCombatLayoutHUDWidget::HandleArtifactPressed_3() { BeginArtifactPress(3); }
void UCombatLayoutHUDWidget::HandleArtifactPressed_4() { BeginArtifactPress(4); }
void UCombatLayoutHUDWidget::HandleArtifactPressed_5() { BeginArtifactPress(5); }

void UCombatLayoutHUDWidget::BeginArtifactPress(const int32 SlotIndex)
{
	mArtifactPressedSlot = SlotIndex;
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}
	World->GetTimerManager().SetTimer(mArtifactLongPressTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this, SlotIndex]()
			{
				HandleArtifactLongPress(SlotIndex);
			}),
		LongPressSeconds, false);
}

void UCombatLayoutHUDWidget::HandleArtifactReleased()
{
	// 꾹 누르기 전에 떼면 아무 일도 없다.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(mArtifactLongPressTimerHandle);
	}
	mArtifactPressedSlot = INDEX_NONE;
}

void UCombatLayoutHUDWidget::HandleArtifactLongPress(const int32 SlotIndex)
{
	ShowArtifactDetailOverlay(SlotIndex);
}

/**
 * @brief 아티팩트 상세를 공용 상세 겹에 채워 띄운다.
 *
 * @details 값은 이미 내려와 있는 PlayerMeta 를 읽는다. 스킬/유닛 상세와 달리
 * 게임플레이에 따로 청하지 않는다 -- 아티팩트는 전투 중에 바뀌지 않아 목록
 * 스냅샷으로 충분하다. 실제 렌더는 프레젠터(PresentArtifact)가 DTO만 받아
 * 그리고, HUD 는 스킬 상세와 같은 방식으로 HUD 상태만 맡는다.
 */
void UCombatLayoutHUDWidget::ShowArtifactDetailOverlay(const int32 SlotIndex)
{
	if (mUIModel == nullptr || EnsureDetailOverlayWidget() == false)
	{
		return;
	}
	const TArray<FCombatArtifactUI>& Artifacts = mUIModel->GetPlayerMeta().mArtifacts;
	if (Artifacts.IsValidIndex(SlotIndex) == false)
	{
		return;
	}
	mDetailPresenter->PresentArtifact(Artifacts[SlotIndex]);
	// 이 칸들은 유닛 상세의 것이다. 아티팩트 하나를 보는 중에는 걷는다.
	SetDetailSkillRowShown(false);
	RefreshWorldGestureInputBlock();
}

/**
 * @brief 상태이상 상세 -- 스킬 상세와 같은 판을 쓴다.
 *
 * @details 설명은 화면이 쥔 고정 표다. 게임플레이에 상태 설명 API 가 아직
 * 없어서다. 새 상태가 오면 이름만 뜨고 설명은 기본 문구로 빠진다.
 */
void UCombatLayoutHUDWidget::ShowStatusDetailOverlay(
	const FGameplayTag& StatusTag, const int32 StackCount)
{
	if (EnsureDetailOverlayWidget() == false)
	{
		return;
	}
	ApplyReadableDetailTypography(false);
	ApplyDetailColumnLayout(true);

	SetTextIfPresent(mDetailTitleText, StatusDisplayName(StatusTag));
	SetTextIfPresent(mDetailSubtitleText, StackCount > 1
		? FText::Format(
			LOCTEXT("StatusSubtitleStacked", "상태이상  ·  {0}중첩"), StackCount)
		: LOCTEXT("StatusSubtitle", "상태이상"));

	// 잎 이름 -> 효과 설명. 기획 수치가 붙으면 게임플레이 쪽 표로 옮긴다.
	static const TMap<FString, FText> Descriptions = {
		{ TEXT("Fortification"), LOCTEXT("StatusDescFortification", "받는 피해가 줄어든다.") },
		{ TEXT("Vulnerability"), LOCTEXT("StatusDescVulnerability", "받는 피해가 늘어난다.") },
		{ TEXT("Weakness"),      LOCTEXT("StatusDescWeakness", "주는 피해가 줄어든다.") },
		{ TEXT("Vigor"),         LOCTEXT("StatusDescVigor", "행동력 효율이 올라간다.") },
		{ TEXT("Haste"),         LOCTEXT("StatusDescHaste", "속도가 올라간다.") },
		{ TEXT("Exhaustion"),    LOCTEXT("StatusDescExhaustion", "행동력 효율이 내려간다.") },
		{ TEXT("Slow"),          LOCTEXT("StatusDescSlow", "속도가 내려간다.") },
		{ TEXT("Frail"),         LOCTEXT("StatusDescFrail", "방어력이 내려간다.") },
		{ TEXT("Root"),          LOCTEXT("StatusDescRoot", "이동할 수 없다.") },
		{ TEXT("Poison"),        LOCTEXT("StatusDescPoison", "턴마다 피해를 입는다.") },
		{ TEXT("Bleed"),         LOCTEXT("StatusDescBleed", "턴마다 피해를 입는다.") },
		{ TEXT("Stun"),          LOCTEXT("StatusDescStun", "턴을 진행할 수 없다.") },
		{ TEXT("Stealth"),       LOCTEXT("StatusDescStealth", "적의 대상이 되지 않는다.") },
	};
	FString Leaf = StatusTag.GetTagName().ToString();
	int32 Dot = INDEX_NONE;
	if (Leaf.FindLastChar(TEXT('.'), Dot))
	{
		Leaf = Leaf.Mid(Dot + 1);
	}
	const FText* Description = Descriptions.Find(Leaf);
	SetTextIfPresent(mDetailBodyText, FText::Format(
		LOCTEXT("StatusDescBodyFmt", "{0}\n턴이 지나면 사라진다."),
		Description != nullptr
			? *Description
			: LOCTEXT("StatusDescMissing", "효과 설명이 아직 없다.")));

	SetPortraitCropped(mDetailIconImage, StatusIconFor(StatusTag));

	ClearDetailGrids();
	ClearDetailChips();
	SetShown(mDetailStatBlock, false);
	SetDetailSkillRowShown(false);
	SetTextIfPresent(mDetailExtraHeading, FText::GetEmpty());
	SetTextIfPresent(mDetailExtraText, FText::GetEmpty());
	ShowDetailRightBlock(nullptr);
	mDetailOverlayWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	RefreshWorldGestureInputBlock();
}

void UCombatLayoutHUDWidget::HandleStatusClicked(const bool bAlly, const int32 SlotIndex)
{
	const TArray<FStatusEffectUI>& Statuses = bAlly
		? mAllyShownStatuses : mEnemyShownStatuses;
	if (Statuses.IsValidIndex(SlotIndex) == false)
	{
		return;
	}
	ShowStatusDetailOverlay(Statuses[SlotIndex].mTag,
		Statuses[SlotIndex].mStackCount);
}

/** @brief 상태 소켓의 0.5초 긴 누름을 시작한다. */
void UCombatLayoutHUDWidget::BeginStatusPress(const bool bAlly, const int32 SlotIndex)
{
	// 둘째 손가락/마우스 합성 입력이 오면 먼저 잡은 상태를 발화시키지 않는다.
	// 새로 실제 눌린 소켓 하나만 긴 누름 후보가 된다.
	CancelStatusPress();

	const TArray<FStatusEffectUI>& Statuses = bAlly
		? mAllyShownStatuses : mEnemyShownStatuses;
	if (Statuses.IsValidIndex(SlotIndex) == false)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}
	mStatusPressedAlly = bAlly;
	mStatusPressedSlot = SlotIndex;
	mStatusPressActive = true;
	World->GetTimerManager().SetTimer(mStatusLongPressTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this, bAlly, SlotIndex]()
		{
			HandleStatusLongPress(bAlly, SlotIndex);
		}), LongPressSeconds, false);
}

/** @brief 같은 소켓에서 손을 뗐을 때 아직 발화하지 않은 긴 누름만 취소한다. */
void UCombatLayoutHUDWidget::EndStatusPress(const bool bAlly, const int32 SlotIndex)
{
	// 첫 손가락 뒤에 둘째 소켓을 눌렀다면 첫 손가락의 늦은 Release가 둘째
	// 타이머를 취소하면 안 된다. 현재 후보와 정확히 같은 Release만 받는다.
	if (mStatusPressActive == true && mStatusPressedAlly == bAlly
		&& mStatusPressedSlot == SlotIndex)
	{
		CancelStatusPress();
	}
}

/** @brief 누름이 같은 소켓에서 0.5초 유지됐을 때 상세를 한 번만 연다. */
void UCombatLayoutHUDWidget::HandleStatusLongPress(
	const bool bAlly, const int32 SlotIndex)
{
	if (mStatusPressActive == false || mStatusPressedAlly != bAlly
		|| mStatusPressedSlot != SlotIndex)
	{
		return;
	}

	// 상세를 열기 전에 후보와 타이머를 소비한다. 테스트가 0.5초 전에 강제로
	// 발화한 경우에도 예약된 콜백이 뒤늦게 한 번 더 남지 않는다.
	CancelStatusPress();
	HandleStatusClicked(bAlly, SlotIndex);
}

/** @brief 대상 교체/패널 종료/손 뗌에서 남은 상태 긴 누름을 전부 무른다. */
void UCombatLayoutHUDWidget::CancelStatusPress()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(mStatusLongPressTimerHandle);
	}
	mStatusLongPressTimerHandle.Invalidate();
	mStatusPressActive = false;
	mStatusPressedSlot = INDEX_NONE;
	mStatusPressedAlly = false;
}

#if WITH_DEV_AUTOMATION_TESTS
void UCombatLayoutHUDWidget::TriggerStatusLongPressForTest(
	const bool bAlly, const int32 SlotIndex)
{
	HandleStatusLongPress(bAlly, SlotIndex);
}

bool UCombatLayoutHUDWidget::IsStatusLongPressPendingForTest() const
{
	const UWorld* World = GetWorld();
	return mStatusPressActive == true && World != nullptr
		&& World->GetTimerManager().IsTimerActive(mStatusLongPressTimerHandle);
}

bool UCombatLayoutHUDWidget::IsStatusPressActiveForTest(
	const bool bAlly, const int32 SlotIndex) const
{
	return mStatusPressActive == true && mStatusPressedAlly == bAlly
		&& mStatusPressedSlot == SlotIndex;
}
#endif

void UCombatLayoutHUDWidget::HandleAllyStatusPressed_0() { BeginStatusPress(true, 0); }
void UCombatLayoutHUDWidget::HandleAllyStatusPressed_1() { BeginStatusPress(true, 1); }
void UCombatLayoutHUDWidget::HandleAllyStatusPressed_2() { BeginStatusPress(true, 2); }
void UCombatLayoutHUDWidget::HandleEnemyStatusPressed_0() { BeginStatusPress(false, 0); }
void UCombatLayoutHUDWidget::HandleEnemyStatusPressed_1() { BeginStatusPress(false, 1); }
void UCombatLayoutHUDWidget::HandleEnemyStatusPressed_2() { BeginStatusPress(false, 2); }
void UCombatLayoutHUDWidget::HandleAllyStatusReleased_0() { EndStatusPress(true, 0); }
void UCombatLayoutHUDWidget::HandleAllyStatusReleased_1() { EndStatusPress(true, 1); }
void UCombatLayoutHUDWidget::HandleAllyStatusReleased_2() { EndStatusPress(true, 2); }
void UCombatLayoutHUDWidget::HandleEnemyStatusReleased_0() { EndStatusPress(false, 0); }
void UCombatLayoutHUDWidget::HandleEnemyStatusReleased_1() { EndStatusPress(false, 1); }
void UCombatLayoutHUDWidget::HandleEnemyStatusReleased_2() { EndStatusPress(false, 2); }

/** @brief 용병탭 스킬 소켓 클릭. 0번은 이동, 나머지는 전투 레일과 같은 스킬 번호. */
void UCombatLayoutHUDWidget::HandleMercenarySkillClicked(const int32 SlotIndex)
{
	// 용병 탭은 정보 모달이다. 여기서 카메라를 움직이면 상세를 읽으려는 손이
	// 전장을 탐색하는 입력으로 바뀐다. 카메라 포커스는 전투판/턴 칸에만 맡긴다.
	if (SlotIndex == 0)
	{
		ShowMoveDetailOverlay();
		return;
	}
	if (mUIModel != nullptr)
	{
		// 용병 패널은 기본으로 현재 턴 용병을 보여 주지만, 목록을 직접 고르기
		// 전에는 UnitDetail 기준(mDetailUnitModel)이 아직 없을 수 있다. 카드 레일과
		// 같은 inspected/current fallback 경로를 쓰면 기본 용병과 고른 용병 모두
		// 한 번의 탭으로 정확한 상세를 받는다.
		mUIModel->RequestLongPressSkill(SlotIndex - 1);
	}
}

/**
 * @brief 턴 칸을 눌렀다.
 *
 * @details 아군/적 구분 없이 카메라만 옮긴다. 턴바는 순서를 확인하고 대상을
 * 찾는 탐색 장치이며, 스킬 UI를 여는 입력은 전투판 선택에만 맡긴다.
 */
void UCombatLayoutHUDWidget::HandleTurnTokenClicked(const int32 SlotIndex)
{
	// 미리보기 단계에서 드래그로 페이지를 넘긴 손은 자식 Button의 release/click이
	// 뒤늦게 도착할 수 있다. 그 클릭을 한 번 삼켜 카메라가 엉뚱한 토큰으로
	// 이동하지 않게 한다.
	if (mTurnSwipeConsumed)
	{
		mTurnSwipeConsumed = false;
		return;
	}
	if (mUIModel == nullptr || mTurnSlotUnitIds.IsValidIndex(SlotIndex) == false)
	{
		return;
	}
	const int32 UnitId = mTurnSlotUnitIds[SlotIndex];
	if (UnitId == INDEX_NONE)
	{
		return;
	}

	/*
	 * 조준 중에는 아무것도 안 한다. 여기서 겨냥을 덮으면 확정 단추가
	 * "겨냥한 칸 재탭" 을 흉내 내는 구조라, 조준해 둔 칸이 아니라 턴 칩의
	 * 유닛 칸으로 확정이 나간다(0807 감사).
	 */
	if (IsAiming() == true)
	{
		return;
	}

	// 떠 있던 상세를 **먼저** 닫는다. 카메라를 잡은 뒤에 닫으면 닫기가 방금
	// 잡은 것을 도로 놓아 버려 화면이 안 움직였다(0806 검수).
	HideDetailOverlay(/*bNotifyGameplay=*/false);
	SetCommandsShown(false);

	const TArray<FUnitUI>& Units = mUIModel->GetUnitUIs();
	const FUnitUI* Unit = Units.FindByPredicate(
		[UnitId](const FUnitUI& Candidate) { return Candidate.mUnitId == UnitId; });
	const bool bIsAlly = Unit != nullptr && Unit->mIsPlayer;

	// 턴바 탭은 카메라 탐색만 한다. 커맨드 고리를 열지 않으므로 화면의
	// 실제 가운데에 놓는다.
	RequestCameraFocus(UnitId, /*bWithCommandRing=*/false);

	if (bIsAlly == false)
	{
		// 적이면 짚어 주기만 한다 -- 요약판이 그 적으로 갈린다.
		FCombatTargetUI Target;
		Target.mIsValid = true;
		Target.mUnitId = UnitId;
		Target.mTile = Unit != nullptr ? Unit->mTile : FTileIndex();
		mUIModel->SetTarget(Target);
	}
}

void UCombatLayoutHUDWidget::HandleTurnTokenClicked_0() { HandleTurnTokenClicked(0); }
void UCombatLayoutHUDWidget::HandleTurnTokenClicked_1() { HandleTurnTokenClicked(1); }
void UCombatLayoutHUDWidget::HandleTurnTokenClicked_2() { HandleTurnTokenClicked(2); }
void UCombatLayoutHUDWidget::HandleTurnTokenClicked_3() { HandleTurnTokenClicked(3); }
void UCombatLayoutHUDWidget::HandleTurnTokenClicked_4() { HandleTurnTokenClicked(4); }
void UCombatLayoutHUDWidget::HandleTurnTokenClicked_5() { HandleTurnTokenClicked(5); }
void UCombatLayoutHUDWidget::HandleTurnTokenClicked_6() { HandleTurnTokenClicked(6); }
void UCombatLayoutHUDWidget::HandleTurnTokenClicked_7() { HandleTurnTokenClicked(7); }
void UCombatLayoutHUDWidget::HandleTurnTokenClicked_8() { HandleTurnTokenClicked(8); }
void UCombatLayoutHUDWidget::HandleTurnTokenClicked_9() { HandleTurnTokenClicked(9); }

void UCombatLayoutHUDWidget::HandleMercenarySkillClicked_0() { HandleMercenarySkillClicked(0); }
void UCombatLayoutHUDWidget::HandleMercenarySkillClicked_1() { HandleMercenarySkillClicked(1); }
void UCombatLayoutHUDWidget::HandleMercenarySkillClicked_2() { HandleMercenarySkillClicked(2); }
void UCombatLayoutHUDWidget::HandleMercenarySkillClicked_3() { HandleMercenarySkillClicked(3); }
void UCombatLayoutHUDWidget::HandleMercenarySkillClicked_4() { HandleMercenarySkillClicked(4); }
void UCombatLayoutHUDWidget::HandleMercenarySkillClicked_5() { HandleMercenarySkillClicked(5); }

void UCombatLayoutHUDWidget::HandleMonsterTabRowClicked_0() { HandleMonsterTabRowClicked(0); }
void UCombatLayoutHUDWidget::HandleMonsterTabRowClicked_1() { HandleMonsterTabRowClicked(1); }
void UCombatLayoutHUDWidget::HandleMonsterTabRowClicked_2() { HandleMonsterTabRowClicked(2); }

void UCombatLayoutHUDWidget::HandleMonsterTabBackClicked()
{
	SetMonsterTabShown(false);
}

void UCombatLayoutHUDWidget::HandleMonsterSkillPressed_0() { BeginMonsterSkillPress(0); }
void UCombatLayoutHUDWidget::HandleMonsterSkillPressed_1() { BeginMonsterSkillPress(1); }
void UCombatLayoutHUDWidget::HandleMonsterSkillPressed_2() { BeginMonsterSkillPress(2); }
void UCombatLayoutHUDWidget::HandleMonsterSkillPressed_3() { BeginMonsterSkillPress(3); }
void UCombatLayoutHUDWidget::HandleMonsterSkillReleased_0() { EndMonsterSkillPress(0); }
void UCombatLayoutHUDWidget::HandleMonsterSkillReleased_1() { EndMonsterSkillPress(1); }
void UCombatLayoutHUDWidget::HandleMonsterSkillReleased_2() { EndMonsterSkillPress(2); }
void UCombatLayoutHUDWidget::HandleMonsterSkillReleased_3() { EndMonsterSkillPress(3); }

void UCombatLayoutHUDWidget::HandleMonsterSkillClicked_0() { HandleMonsterSkillClicked(0); }
void UCombatLayoutHUDWidget::HandleMonsterSkillClicked_1() { HandleMonsterSkillClicked(1); }
void UCombatLayoutHUDWidget::HandleMonsterSkillClicked_2() { HandleMonsterSkillClicked(2); }
void UCombatLayoutHUDWidget::HandleMonsterSkillClicked_3() { HandleMonsterSkillClicked(3); }

/** @brief 몬스터 탭 스킬은 일반 버튼처럼 한 번 탭하면 즉시 상세를 연다. */
void UCombatLayoutHUDWidget::HandleMonsterSkillClicked(const int32 SlotIndex)
{
	CancelMonsterSkillPress();
	if (mUIModel == nullptr || IsMonsterTabShown() == false
		|| IsDetailOverlayShown() == true
		|| mMonsterTabSkillIndices.IsValidIndex(SlotIndex) == false)
	{
		return;
	}
	const int32 SkillIndex = mMonsterTabSkillIndices[SlotIndex];
	if (SkillIndex != INDEX_NONE)
	{
		mUIModel->RequestInspectUnitSkill(SkillIndex);
	}
}

/** @brief 몬스터 도감의 스킬 슬롯 누름을 롱프레스 후보로 잡는다. */
void UCombatLayoutHUDWidget::BeginMonsterSkillPress(const int32 SlotIndex)
{
	CancelMonsterSkillPress();
	if (IsMonsterTabShown() == false || IsDetailOverlayShown() == true
		|| mMonsterTabSkillIndices.IsValidIndex(SlotIndex) == false
		|| mMonsterTabSkillIndices[SlotIndex] == INDEX_NONE)
	{
		return;
	}
	mMonsterSkillPressActive = true;
	mMonsterSkillPressedSlot = SlotIndex;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(mMonsterSkillLongPressTimerHandle,
			FTimerDelegate::CreateWeakLambda(this, [this, SlotIndex]()
			{
				HandleMonsterSkillLongPress(SlotIndex);
			}), LongPressSeconds, false);
	}
}

/** @brief 짧게 뗀 손은 상세를 열지 않고 후보만 무른다. */
void UCombatLayoutHUDWidget::EndMonsterSkillPress(const int32 SlotIndex)
{
	if (mMonsterSkillPressActive == true && mMonsterSkillPressedSlot == SlotIndex)
	{
		CancelMonsterSkillPress();
	}
}

/** @brief 슬롯 DTO의 실제 스킬 index로 게임플레이에 상세를 청한다. */
void UCombatLayoutHUDWidget::HandleMonsterSkillLongPress(const int32 SlotIndex)
{
	if (mMonsterSkillPressActive == false || mMonsterSkillPressedSlot != SlotIndex
		|| mUIModel == nullptr || IsMonsterTabShown() == false
		|| IsDetailOverlayShown() == true
		|| mMonsterTabSkillIndices.IsValidIndex(SlotIndex) == false)
	{
		return;
	}
	const int32 SkillIndex = mMonsterTabSkillIndices[SlotIndex];
	CancelMonsterSkillPress();
	if (SkillIndex != INDEX_NONE)
	{
		mUIModel->RequestInspectUnitSkill(SkillIndex);
	}
}

void UCombatLayoutHUDWidget::CancelMonsterSkillPress()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(mMonsterSkillLongPressTimerHandle);
	}
	mMonsterSkillLongPressTimerHandle.Invalidate();
	mMonsterSkillPressedSlot = INDEX_NONE;
	mMonsterSkillPressActive = false;
}

void UCombatLayoutHUDWidget::HandleMonsterTabRowClicked(const int32 RowIndex)
{
	if (mMonsterTabUnitIds.IsValidIndex(RowIndex) == false)
	{
		return;
	}
	CancelMonsterSkillPress();
	HideDetailOverlay(/*bNotifyGameplay=*/true);
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
		CancelStatusPress();
		CancelMonsterSkillPress();
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
		CancelMonsterSkillPress();
		// 아래에서 탭 조사 종료 신호를 한 번 보낸다. 겹은 여기서 표시만 걷는다.
		HideDetailOverlay(/*bNotifyGameplay=*/false);
		// 상세 요청으로 칠렸을 수 있는 위협 범위를 걷으라는 신호. INDEX_NONE = "닫았다".
		if (mUIModel != nullptr && mMonsterTabInspectedUnitId != INDEX_NONE)
		{
			mUIModel->RequestLongPressUnit(INDEX_NONE);
		}
		mMonsterTabInspectedUnitId = INDEX_NONE;
	}
	RefreshCommandVisibility();
	RefreshWorldGestureInputBlock();
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
	// 평상시에는 HUD보다 위. 몬스터 스킬 상세(70)는 이 탭보다 위에 놓인다.
	mMonsterTabWidget->AddToViewport(MonsterTabViewportZOrder);
	mMonsterTabWidget->SetVisibility(ESlateVisibility::Collapsed);

	// WBP 에 번역 키 없이 박힌 라벨을 로컬라이즈 텍스트로 갈아 끼운다.
	if (UTextBlock* CritLabel = Cast<UTextBlock>(
		mMonsterTabWidget->GetWidgetFromName(TEXT("MonsterChip2Label"))))
	{
		CritLabel->SetText(NSLOCTEXT("CombatHUD", "MercenaryCrit", "치명타"));
	}
	if (UTextBlock* BackText = Cast<UTextBlock>(
		mMonsterTabWidget->GetWidgetFromName(TEXT("MonsterBackText"))))
	{
		BackText->SetText(NSLOCTEXT("CombatHUD", "MercenaryBack", "닫기"));
	}

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
	using FMonsterSkillHandler = void (UCombatLayoutHUDWidget::*)();
	static const FMonsterSkillHandler ClickHandlers[4] = {
		&UCombatLayoutHUDWidget::HandleMonsterSkillClicked_0,
		&UCombatLayoutHUDWidget::HandleMonsterSkillClicked_1,
		&UCombatLayoutHUDWidget::HandleMonsterSkillClicked_2,
		&UCombatLayoutHUDWidget::HandleMonsterSkillClicked_3 };
	static const TCHAR* const ClickNames[4] = {
		TEXT("HandleMonsterSkillClicked_0"), TEXT("HandleMonsterSkillClicked_1"),
		TEXT("HandleMonsterSkillClicked_2"), TEXT("HandleMonsterSkillClicked_3") };
	for (int32 Index = 0; Index < 4; ++Index)
	{
		if (UButton* Button = Cast<UButton>(mMonsterTabWidget->GetWidgetFromName(
			FName(*FString::Printf(TEXT("MonsterSkillButton_%d"), Index)))))
		{
			Button->SetTouchMethod(EButtonTouchMethod::PreciseTap);
			Button->SetClickMethod(EButtonClickMethod::PreciseClick);
			Button->OnClicked.__Internal_AddUniqueDynamic(
				this, ClickHandlers[Index], ClickNames[Index]);
		}
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
				SetPortraitCropped(Portrait, Head);
			}
		}
		if (UTextBlock* Name = Cast<UTextBlock>(mMonsterTabWidget->GetWidgetFromName(
			FName(*FString::Printf(TEXT("MonsterRowName_%d"), Index)))))
		{
			Name->SetText(Monsters[Index]->mName);
		}
		// 용병 목록처럼 줄 오른쪽에 레벨 배지 (0806 확정 시안)
		if (UTextBlock* Level = Cast<UTextBlock>(mMonsterTabWidget->GetWidgetFromName(
			FName(*FString::Printf(TEXT("MonsterRowLevel_%d"), Index)))))
		{
			Level->SetText(FText::FromString(FString::Printf(
				TEXT("Lv %d"), FMath::Max(1, Monsters[Index]->mLevel))));
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
	// 라벨(HP/AP/속도/치명타)은 줄판에 따로 있으니 값만 적는다.
	// 치명타 확률은 아직 게임 데이터에 없어 판이 "-" 로 두고 있다(0806 합의).
	SetTabText(TEXT("MonsterDetailHPText"), FText::FromString(FString::Printf(
		TEXT("%d/%d"), FMath::RoundToInt(Monster.mHP), FMath::RoundToInt(Monster.mMaxHP))));
	SetTabText(TEXT("MonsterDetailAPText"), FText::FromString(FString::Printf(
		TEXT("%d/%d"), Monster.mActionPoints, Monster.mMaxActionPoints)));
	SetTabText(TEXT("MonsterDetailSpeedText"), FText::AsNumber(
		FMath::RoundToInt(Monster.mSpeedPoint)));
	if (UImage* DetailPortrait = Cast<UImage>(
		mMonsterTabWidget->GetWidgetFromName(TEXT("MonsterDetailPortrait"))))
	{
		UTexture2D* Body = Monster.mPortrait != nullptr
			? Monster.mPortrait.Get() : Monster.mTurnPortrait.Get();
		if (Body != nullptr)
		{
			SetPortraitCropped(DetailPortrait, Body);
		}
	}

	// 상태이상 칸은 뺐다(0809) -- 이 탭은 도감이다. 지금 걸린 상태는
	// 요약판(버프/디버프 띠)이 이미 보여 준다.
	for (int32 Index = 0; Index < 2; ++Index)
	{
		if (UTextBlock* StatusText = Cast<UTextBlock>(
			mMonsterTabWidget->GetWidgetFromName(
				FName(*FString::Printf(TEXT("MonsterStatusText_%d"), Index)))))
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
	// 응답 알림에만 기대지 않고 한 번 더 그린다.
	//
	// 탭을 여는 순간의 요청은 응답이 동기로 돌아오는데, 그때는 아직 탭이 보이는
	// 상태가 아니어서 알림 쪽 분기가 상세창을 대신 열고 지나간다. 그러면 스킬 칸은
	// 만든 그대로(빈 이름 + 흰 사각형) 남는다. 실제로 화면에 그렇게 나왔다.
	RefreshMonsterTabDetail();
}

void UCombatLayoutHUDWidget::RefreshMonsterTabDetail()
{
	if (mMonsterTabWidget == nullptr || mUIModel == nullptr)
	{
		return;
	}
	const FUnitDetailUI& Detail = mUIModel->GetUnitDetail();
	// 늦게 도착한 다른 유닛의 상세로 현재 선택을 덮어쓰지 않는다. 아직 안 왔으면
	// 칸을 접어 둔다 -- 만든 그대로 두면 빈 이름과 흰 사각형이 그대로 남는다.
	const bool bHasDetail = Detail.mUnitId != INDEX_NONE
		&& Detail.mUnitId == mMonsterTabInspectedUnitId;
	const int32 SkillCount = bHasDetail ? Detail.mSkills.Num() : 0;
	mMonsterTabSkillIndices.Init(INDEX_NONE, 4);

	// 레벨은 목록 줄의 Lv 배지가 맡는다(0806 확정 시안). 이름판 밑 TypeText 는
	// 목록 갱신이 접은 그대로 둔다.
	if (UTextBlock* Heading = Cast<UTextBlock>(
		mMonsterTabWidget->GetWidgetFromName(TEXT("MonsterSkillHeading"))))
	{
		Heading->SetText(SkillCount > 0 ? LOCTEXT("MonsterTabSkills", "스킬")
			: LOCTEXT("MonsterTabNoSkills", "스킬  ·  없음"));
	}

	for (int32 Index = 0; Index < 4; ++Index)
	{
		UTextBlock* SkillName = Cast<UTextBlock>(mMonsterTabWidget->GetWidgetFromName(
			FName(*FString::Printf(TEXT("MonsterSkillName_%d"), Index))));
		const bool bHasSkill = Index < SkillCount;
		if (bHasSkill == true)
		{
			mMonsterTabSkillIndices[Index] = Detail.mSkills[Index].mSkillIndex;
		}
		if (SkillName != nullptr)
		{
			if (bHasSkill == true)
			{
				SkillName->SetText(Detail.mSkills[Index].mName);
			}
			SkillName->SetVisibility(bHasSkill
				? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
		// 아이콘 칸과 그 틀. 민무늬 글상자만 보여 주던 것을 용병탭과 같은
		// 규칙(그림 + 이름)으로 맞춘다. 그림이 없는 스킬은 칸만 비워 둔다.
		if (UImage* SkillIcon = Cast<UImage>(mMonsterTabWidget->GetWidgetFromName(
			FName(*FString::Printf(TEXT("MonsterSkillIcon_%d"), Index)))))
		{
			UTexture2D* Icon = bHasSkill ? Detail.mSkills[Index].mIcon.Get() : nullptr;
			if (Icon != nullptr)
			{
				SkillIcon->SetBrushFromTexture(Icon, false);
			}
			SkillIcon->SetVisibility(bHasSkill && Icon != nullptr
				? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
		if (UWidget* SkillSlot = mMonsterTabWidget->GetWidgetFromName(
			FName(*FString::Printf(TEXT("MonsterSkillSlot_%d"), Index))))
		{
			SkillSlot->SetVisibility(bHasSkill
				? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
		if (UButton* SkillButton = Cast<UButton>(mMonsterTabWidget->GetWidgetFromName(
			FName(*FString::Printf(TEXT("MonsterSkillButton_%d"), Index)))))
		{
			SkillButton->SetVisibility(bHasSkill
				? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
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
/**
 * @brief 스킬 단추. 차례 유닛의 카드를 펴고 카메라를 그 유닛에 맞춘다.
 *
 * @details 차례가 와도 카드는 저절로 안 편다(0807) -- 이 단추가 카드를 여는
 * 정문이다. 다시 누르면 접는다.
 */
void UCombatLayoutHUDWidget::HandleSkillToggleClicked()
{
	if (mUIModel == nullptr)
	{
		return;
	}
	if (mCommandsShown == true)
	{
		SetCommandsShown(false);
		return;
	}
	const int32 TurnUnitId = mUIModel->GetTurnUI().mCurrentUnitId;
	// 적 차례에는 펼 카드가 없다. 여기서 그냥 열면 억제 플래그만 세워 두고
	// 응답이 안 와, 다음 정상 상세가 조용히 한 번 먹힌다(0807 감사).
	if (TurnUnitId == INDEX_NONE || IsPlayerTurn() == false)
	{
		return;
	}
	// 다른 용병을 살펴보던 중이었어도 카드는 차례 유닛 것으로 되돌린다.
	mSuppressNextUnitDetailOverlay = true;
	mInspectedAllyUnitId = TurnUnitId;
	mUIModel->RequestInspectUnit(TurnUnitId);
	RequestCameraFocus(TurnUnitId, /*bWithCommandRing=*/true);
	SetCommandsShown(true);
}

void UCombatLayoutHUDWidget::HandleEndTurnClicked()
{
	if (mUIModel == nullptr || IsAiming())
	{
		return;
	}

	// 턴 종료는 턴 종료만 수행한다. 조준 취소는 별도 CancelButton이 맡아
	// 같은 화면 위치가 상태에 따라 정반대 의미로 바뀌지 않는다.
	HideDetailOverlay(/*bNotifyGameplay=*/false);
	mUIModel->RequestEndTurn();
}

/* ── 상세 패널 (롱프레스 정보) ─────────────────────────────────────── */

/**
 * @brief 스킬 상세 프레젠터를 한 번 만들고 설정을 최신으로 맞춘다.
 *
 * @details
 * 겹 클래스·전술 WBP 클래스·그림 묶음은 생성자가 하드 참조로 물어 둔 것을
 * 그대로 넘긴다(쿡 유지는 HUD 몫). 겹이 아직 없으면 클래스 교체(시험 훅)도
 * 매번 반영한다.
 */
bool UCombatLayoutHUDWidget::EnsureDetailPresenter()
{
	if (mDetailPresenter == nullptr)
	{
		mDetailPresenter = NewObject<USkillDetailOverlayPresenter>(this);
		mDetailPresenter->Initialize(GetWorld(), mDetailOverlayWidgetClass,
			mSkillTacticalDiagramWidgetClass, DetailOverlayViewportZOrder);
		mDetailPresenter->SetReadableDetailFont(mReadableDetailFont);
		FSkillDetailOverlayVisualAssets Assets;
		Assets.mRingTexture = mSkillVisualRingTexture;
		Assets.mCellNormalTexture = mSkillVisualCellNormalTexture;
		Assets.mCellSelectedTexture = mSkillVisualCellSelectedTexture;
		Assets.mAPIconTexture = mSkillVisualAPIconTexture;
		Assets.mDamageIconTexture = mSkillVisualDamageIconTexture;
		Assets.mCooldownIconTexture = mSkillVisualCooldownIconTexture;
		Assets.mCriticalIconTexture = mSkillVisualCriticalIconTexture;
		Assets.mCasterIconTexture = mSkillVisualCasterIconTexture;
		Assets.mTargetIconTexture = mSkillVisualTargetIconTexture;
		Assets.mRangeButtonTexture = mSkillRangeButtonTexture;
		Assets.mRangeButtonSelectedTexture = mSkillRangeButtonSelectedTexture;
		mDetailPresenter->SetVisualAssets(Assets);
		// AP/쿨타임 아이콘은 HUD WBP 의 실제 배지 브러시를 따라간다.
		mDetailPresenter->SetStatTextureResolver(
			FSkillDetailStatTextureResolver::CreateUObject(this,
				&UCombatLayoutHUDWidget::ResolveDetailStatTexture));
		// 확정 시안의 "닫기" 단추 -> 기존 닫기 경로(위협 범위 걷기 포함).
		mDetailPresenter->OnCloseClicked().AddUObject(this,
			&UCombatLayoutHUDWidget::HandleDetailCloseCatchClicked);
		// 프레젠터가 상세 화면을 갈아 끼울 때 SceneCapture 도 함께 멈춘다.
		mDetailPresenter->OnWorldPreviewStopRequested().AddUObject(this,
			&UCombatLayoutHUDWidget::StopSkillWorldPreview);
	}
	// 시험이 TakeWidget 뒤에 클래스를 바꿔 끼우는 경로. 겹이 없을 때만 반영된다.
	mDetailPresenter->SetOverlayWidgetClass(mDetailOverlayWidgetClass);
	return true;
}

UTexture2D* UCombatLayoutHUDWidget::ResolveDetailStatTexture(
	const FName SourceWidgetName, UTexture2D* Fallback)
{
	if (const UImage* SourceImage = Cast<UImage>(GetWidgetFromName(SourceWidgetName)))
	{
		if (UTexture2D* Texture = Cast<UTexture2D>(
			SourceImage->GetBrush().GetResourceObject()))
		{
			return Texture;
		}
	}
	return Fallback;
}

/**
 * @brief 상세 패널 위젯을 처음 한 번 만들어 화면에 얹는다.
 *
 * @details
 * 실제 생성·부품 캐시·모달 실드·닫기 배선은 프레젠터가 한다. HUD 는 남은
 * 상세 경로(유닛/상태/아티팩트)가 예전 코드 그대로 읽도록 부품 포인터를
 * 비추고, UIModel 이 필요한 스킬 칸 줄만 직접 짓는다.
 */
bool UCombatLayoutHUDWidget::EnsureDetailOverlayWidget()
{
	if (EnsureDetailPresenter() == false
		|| mDetailPresenter->EnsureOverlayWidget(GetOwningPlayer()) == false)
	{
		return false;
	}
	mDetailOverlayWidget = mDetailPresenter->GetOverlayWidget();
	mDetailIconImage = mDetailPresenter->GetIconImage();
	mDetailTitleText = mDetailPresenter->GetTitleText();
	mDetailSubtitleText = mDetailPresenter->GetSubtitleText();
	mDetailBodyText = mDetailPresenter->GetBodyText();
	mDetailStatBlock = mDetailPresenter->GetStatBlock();
	mDetailSkillBlock = mDetailPresenter->GetSkillBlock();
	mDetailExtraHeading = mDetailPresenter->GetExtraHeading();
	mDetailExtraText = mDetailPresenter->GetExtraText();
	mDetailDivider0 = mDetailPresenter->GetDivider0();
	mDetailDivider1 = mDetailPresenter->GetDivider1();
	mSkillTacticalDiagramWidget = mDetailPresenter->GetTacticalDiagram();
	mSkillDetailContentWidget = mDetailPresenter->GetSkillContentWidget();
	mSkillWorldPreviewImage = mDetailPresenter->GetWorldPreviewImage();

	// 스킬 칸 줄은 UIModel 의 유닛 상세를 읽고 HUD 핸들러로 배선되므로 남는다.
	// 자체 가드가 있어 두 번째 호출부터는 아무 일도 안 한다.
	BuildDetailSkillRow();
	return true;
}

void UCombatLayoutHUDWidget::ApplyReadableDetailTypography(const bool bReadable)
{
	if (mDetailPresenter != nullptr)
	{
		mDetailPresenter->ApplyReadableDetailTypography(bReadable);
	}
}

void UCombatLayoutHUDWidget::HandleDetailCloseCatchClicked()
{
	HideDetailOverlay(/*bNotifyGameplay=*/true);
}

/**
 * @brief 적 요약판의 다음 스킬 소켓 클릭 → 그 스킬 상세.
 *
 * @details 유닛 조사와 스킬 조사를 이어 청한다. 응답이 동기로 돌아오는 흐름이라
 * 마지막 응답(스킬 상세)이 화면에 남는다.
 */
void UCombatLayoutHUDWidget::HandleEnemyNextSkillClicked()
{
	if (mUIModel == nullptr || mEnemyShownUnitId == INDEX_NONE)
	{
		return;
	}
	mUIModel->RequestInspectUnit(mEnemyShownUnitId);
	if (mEnemyShownNextSkillIndex != INDEX_NONE)
	{
		mUIModel->RequestInspectUnitSkill(mEnemyShownNextSkillIndex);
	}
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

	// 판이 자리를 잡아 준 칸이 있으면 그 안에 넣는다. 없으면 옛 판이므로
	// 예전처럼 패널 비율 자리에 앉힌다 -- 상세창은 화면 담당이 갈아 끼우는
	// 자산이라 이쪽이 특정 위젯을 강제하지 않는다.
	UCanvasPanel* Host = Cast<UCanvasPanel>(
		mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailSkillRowHost")));
	const bool bHosted = Host != nullptr;
	if (Host == nullptr)
	{
		Host = Cast<UCanvasPanel>(mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailPanelRoot")));
	}
	if (Host == nullptr)
	{
		Host = Cast<UCanvasPanel>(Tree->RootWidget);
	}
	if (Host == nullptr)
	{
		return;
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

	/*
	 * 칸은 판이 미리 만들어 둔 것을 쓴다.
	 *
	 * 여기서 UButton 을 만들면 아무 스타일도 못 받아 슬레이트 기본 밝은 회색
	 * 사각으로 그려진다. 상세창 오른쪽 열을 덮고 있던 회색 판이 그것이다.
	 * 런타임에는 부품 그림을 불러올 수 없으므로(PR#300) 판에서 입혀야 한다.
	 *
	 * 칸이 없는 옛 판이면 예전처럼 만든다 -- 회색이지만 눌리기는 한다.
	 */
	UButton* const FirstPrebuilt = Cast<UButton>(
		mDetailOverlayWidget->GetWidgetFromName(TEXT("DetailSkillButton_0")));
	UHorizontalBox* FallbackRow = nullptr;
	if (FirstPrebuilt != nullptr)
	{
		mDetailSkillRow = Host;
	}
	else
	{
		FallbackRow = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		mDetailSkillRow = FallbackRow;
		if (UCanvasPanelSlot* RowSlot = Host->AddChildToCanvas(FallbackRow))
		{
			RowSlot->SetAnchors(bHosted
				? FAnchors(0.f, 0.f, 1.f, 1.f) : FAnchors(0.10f, 0.66f, 0.90f, 0.90f));
			RowSlot->SetOffsets(FMargin(0.f));
			RowSlot->SetAlignment(FVector2D(0.f, 0.f));
		}
	}

	for (int32 Index = 0; Index < DetailSkillSlotCount; ++Index)
	{
		UButton* Button = nullptr;
		UImage* Icon = nullptr;
		UTextBlock* Label = nullptr;

		if (FallbackRow == nullptr)
		{
			Button = Cast<UButton>(mDetailOverlayWidget->GetWidgetFromName(
				*FString::Printf(TEXT("DetailSkillButton_%d"), Index)));
			Icon = Cast<UImage>(mDetailOverlayWidget->GetWidgetFromName(
				*FString::Printf(TEXT("DetailSkillIcon_%d"), Index)));
			Label = Cast<UTextBlock>(mDetailOverlayWidget->GetWidgetFromName(
				*FString::Printf(TEXT("DetailSkillLabel_%d"), Index)));
		}
		else
		{
			Button = Tree->ConstructWidget<UButton>(UButton::StaticClass());
			if (UHorizontalBoxSlot* ButtonSlot = FallbackRow->AddChildToHorizontalBox(Button))
			{
				ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				ButtonSlot->SetPadding(FMargin(6.f, 0.f));
			}

			// 그림과 글자를 겹쳐 둔다. 아이콘이 있으면 그림, 없으면 이름을 보여 준다.
			UOverlay* Content = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
			Button->AddChild(Content);

			Icon = Tree->ConstructWidget<UImage>(UImage::StaticClass());
			if (UOverlaySlot* IconSlot = Cast<UOverlaySlot>(Content->AddChild(Icon)))
			{
				IconSlot->SetHorizontalAlignment(HAlign_Fill);
				IconSlot->SetVerticalAlignment(VAlign_Fill);
			}

			Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			if (UOverlaySlot* LabelSlot = Cast<UOverlaySlot>(Content->AddChild(Label)))
			{
				LabelSlot->SetHorizontalAlignment(HAlign_Center);
				LabelSlot->SetVerticalAlignment(VAlign_Center);
			}
			Label->SetJustification(ETextJustify::Center);
			Label->SetAutoWrapText(true);
		}

		if (Button == nullptr)
		{
			continue;
		}
		if (Icon != nullptr)
		{
			Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		if (Label != nullptr)
		{
			Label->SetVisibility(ESlateVisibility::Collapsed);
		}

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
/**
 * @brief (0809) 유닛 상세 겹은 뺐다. 적이면 몬스터탭(도감)을 그 몬스터로 연다.
 *
 * @details 아군 상세는 용병탭이 맡으므로 여기서는 아무것도 안 띄운다.
 * 몬스터탭 WBP가 없는 환경(시험 픽스처)만 예전 상세 겹으로 보여 준다.
 */
void UCombatLayoutHUDWidget::ShowUnitInspection()
{
	if (mUIModel == nullptr)
	{
		return;
	}
	const FUnitDetailUI& Detail = mUIModel->GetUnitDetail();
	if (Detail.mUnitId == INDEX_NONE)
	{
		return;
	}
	const FUnitUI* Unit = nullptr;
	int32 EnemyRow = INDEX_NONE;
	int32 AliveEnemyCount = 0;
	for (const FUnitUI& Candidate : mUIModel->GetUnitUIs())
	{
		const bool bAliveEnemy = Candidate.mIsPlayer == false && Candidate.mHP > 0.f;
		if (Candidate.mUnitId == Detail.mUnitId)
		{
			Unit = &Candidate;
			if (bAliveEnemy == true && AliveEnemyCount < 3)
			{
				EnemyRow = AliveEnemyCount;
			}
		}
		if (bAliveEnemy == true)
		{
			++AliveEnemyCount;
		}
	}
	if (Unit == nullptr || Unit->mIsPlayer == true)
	{
		return;
	}
	if (mMonsterTabWidgetClass == nullptr)
	{
		ShowUnitDetailOverlay();
		return;
	}
	if (EnemyRow != INDEX_NONE)
	{
		mMonsterTabSelectedRow = EnemyRow;
	}
	SetMonsterTabShown(true);
}

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
	ApplyReadableDetailTypography(false);
	ApplyDetailColumnLayout(false);

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

	FText Subtitle = FText::Format(LOCTEXT("UnitDetailLevel", "Lv.{0}"),
		FMath::Max(1, Detail.mLevel));
	if (Unit != nullptr)
	{
		const FText Side = Unit->mIsPlayer == true
			? LOCTEXT("UnitDetailAlly", "아군")
			: LOCTEXT("UnitDetailEnemy", "적");
		Subtitle = FText::Format(
			LOCTEXT("UnitDetailIdentityAndHp", "{0}  ·  {1}  ·  HP {2}/{3}"),
			Subtitle, Side, FMath::RoundToInt(Unit->mHP),
			FMath::RoundToInt(Unit->mMaxHP));
		if (Unit->mDefensePoint > 0.f)
		{
			Subtitle = FText::Format(
				LOCTEXT("UnitDetailWithDefense", "{0}  ·  방어 {1}"), Subtitle,
				FMath::RoundToInt(Unit->mDefensePoint));
		}
	}
	SetTextIfPresent(mDetailSubtitleText, Subtitle);

	// 소켓에는 이 유닛의 초상을 건다. 안 걸면 직전 스킬 아이콘이 그대로 남는다
	// (0806 검수: 적 상세에 칼 아이콘이 떠 있었다).
	if (Unit != nullptr)
	{
		UTexture2D* Portrait = Unit->mTurnPortrait != nullptr
			? Unit->mTurnPortrait.Get() : Unit->mPortrait.Get();
		if (Portrait != nullptr)
		{
			SetPortraitCropped(mDetailIconImage, Portrait);
		}
	}

	// 유닛에는 조준/타격 형태가 없다. 스킬 상세가 칠해 둔 칸을 걷고, 칩은 유닛
	// 스탯으로 갈아 끼운다 -- 스킬 값이 남아 있으면 다른 대상 수치로 읽힌다.
	ClearDetailGrids();
	ClearDetailChips();
	SetDetailChip(0, LOCTEXT("DetailChipLevel", "레벨"), FText::AsNumber(Detail.mLevel));
	if (Unit != nullptr)
	{
		SetDetailChip(1, LOCTEXT("DetailChipHp", "HP"), FText::FromString(
			FString::Printf(TEXT("%.0f/%.0f"), Unit->mHP, Unit->mMaxHP)));
		SetDetailChip(2, LOCTEXT("DetailChipDefense", "방어"),
			FText::AsNumber(FMath::RoundToInt(Unit->mDefensePoint)));
		SetDetailChip(3, LOCTEXT("DetailChipSpeed", "속도"),
			FText::AsNumber(FMath::RoundToInt(Unit->mSpeedPoint)));
		SetDetailChip(4, LOCTEXT("DetailChipAp", "AP"), FText::AsNumber(Unit->mActionPoints));
	}

	TArray<FText> PassiveLines;
	for (const FText& Passive : Detail.mPassiveDescriptions)
	{
		PassiveLines.Add(FText::Format(
			LOCTEXT("UnitDetailPassiveLine", "· {0}"), Passive));
	}
	FText Body = PassiveLines.IsEmpty()
		? LOCTEXT("UnitDetailNoPassives", "패시브 없음")
		: FText::Join(FText::FromString(TEXT("\n")), PassiveLines);
	if (Unit != nullptr && Unit->mIsPlayer == false)
	{
		// 판에 함께 칠린 위협 범위의 범례다. 칠만 있고 뜻을 알려 주는 곳이
		// 없으면 밴드와 채움을 구분할 길이 없다.
		Body = FText::Format(LOCTEXT("UnitDetailEnemyThreatLegend",
			"{0}\n\n판의 표시는 이 적이 한 턴에 닿는 곳이다.\n테두리 밴드 = 이동 범위, 채움 = 공격 범위."), Body);
	}
	SetTextIfPresent(mDetailBodyText, Body);

	SetPortraitCropped(mDetailIconImage, Detail.mPortrait);
	SetShown(mDetailStatBlock, true);
	ShowDetailRightBlock(mDetailSkillBlock);
	RefreshDetailSkillRow();
	// 자기는 눌림을 안 받고 스킬 칸만 받는다. 그래서 칸 밖을 톡 치면 눌림이
	// HUD 까지 내려가 패널이 닫힌다.
	mDetailOverlayWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	RefreshWorldGestureInputBlock();
}

#if WITH_DEV_AUTOMATION_TESTS
void UCombatLayoutHUDWidget::TriggerMonsterSkillLongPressForTest(const int32 SlotIndex)
{
	HandleMonsterSkillLongPress(SlotIndex);
}

bool UCombatLayoutHUDWidget::IsMonsterSkillLongPressPendingForTest() const
{
	const UWorld* World = GetWorld();
	return mMonsterSkillPressActive == true && World != nullptr
		&& World->GetTimerManager().IsTimerActive(mMonsterSkillLongPressTimerHandle);
}

bool UCombatLayoutHUDWidget::IsMonsterSkillPressActiveForTest(const int32 SlotIndex) const
{
	return mMonsterSkillPressActive == true && mMonsterSkillPressedSlot == SlotIndex;
}

int32 UCombatLayoutHUDWidget::GetMonsterTabViewportZOrderForTest() const
{
	return MonsterTabViewportZOrder;
}

int32 UCombatLayoutHUDWidget::GetDetailOverlayViewportZOrderForTest() const
{
	return DetailOverlayViewportZOrder;
}
#endif

/**
 * @brief 오른쪽 열의 세 덩어리 중 하나만 켠다. 실제 일은 프레젠터가 한다.
 * @param Wanted 켤 덩어리. nullptr 이면 셋 다 끈다
 */
void UCombatLayoutHUDWidget::ShowDetailRightBlock(const UWidget* Wanted)
{
	if (mDetailPresenter != nullptr)
	{
		mDetailPresenter->ShowDetailRightBlock(Wanted);
	}
}

/**
 * @brief 상세 종류에 맞춰 열 자체를 재배치한다. 실제 일은 프레젠터가 한다.
 */
void UCombatLayoutHUDWidget::ApplyDetailColumnLayout(const bool bArtifactTwoColumn)
{
	if (mDetailPresenter != nullptr)
	{
		mDetailPresenter->ApplyDetailColumnLayout(bArtifactTwoColumn);
	}
}

void UCombatLayoutHUDWidget::SetDetailChip(int32 ChipSlot, const FText& Label, const FText& Value)
{
	if (mDetailPresenter != nullptr)
	{
		mDetailPresenter->SetDetailChip(ChipSlot, Label, Value);
	}
}

void UCombatLayoutHUDWidget::ClearDetailChips()
{
	if (mDetailPresenter != nullptr)
	{
		mDetailPresenter->ClearDetailChips();
	}
}

void UCombatLayoutHUDWidget::ClearDetailGrids()
{
	if (mDetailPresenter != nullptr)
	{
		mDetailPresenter->ClearDetailGrids();
	}
}

bool UCombatLayoutHUDWidget::StartSkillWorldPreview()
{
	// 미리보기 이미지 위젯은 프레젠터가 짓는다. 겹이 아직 없으면 그냥 실패한다.
	if (mDetailPresenter != nullptr)
	{
		mDetailPresenter->EnsureSkillVisualPreviewBuilt();
		mSkillWorldPreviewImage = mDetailPresenter->GetWorldPreviewImage();
	}
	UWorld* World = GetWorld();
	if (World == nullptr || mSkillWorldPreviewImage == nullptr)
	{
		return false;
	}

	UCameraComponent* SourceCamera = nullptr;
	if (ACombatCameraPawn* MainCamera = UCameraFunctionLibrary::GetMainCameraPawn(this))
	{
		SourceCamera = MainCamera->GetCameraComponent();
	}
	if (SourceCamera == nullptr)
	{
		// 편집기 자동 캡처처럼 PlayerController가 아직 Pawn을 소유하기 전에도
		// 실제 카메라 액터가 있으면 같은 경로를 검수할 수 있게 한다.
		for (TActorIterator<ACameraActor> It(World); It; ++It)
		{
			SourceCamera = It->GetCameraComponent();
			break;
		}
	}
	if (SourceCamera == nullptr)
	{
		return false;
	}
	mSkillWorldPreviewSourceCamera = SourceCamera;

	if (mSkillWorldPreviewRenderTarget == nullptr)
	{
		mSkillWorldPreviewRenderTarget = NewObject<UTextureRenderTarget2D>(this,
			TEXT("SkillDetailWorldRenderTarget"), RF_Transient);
		mSkillWorldPreviewRenderTarget->RenderTargetFormat = RTF_RGBA8_SRGB;
		mSkillWorldPreviewRenderTarget->ClearColor = FLinearColor(0.008f, 0.01f, 0.015f, 1.f);
		mSkillWorldPreviewRenderTarget->InitCustomFormat(1024, 384, PF_B8G8R8A8, true);
		mSkillWorldPreviewRenderTarget->UpdateResourceImmediate(true);
	}
	if (mSkillWorldPreviewCapture == nullptr)
	{
		FActorSpawnParameters Params;
		Params.Name = MakeUniqueObjectName(World, ASceneCapture2D::StaticClass(),
			TEXT("SkillDetailSceneCapture"));
		Params.ObjectFlags |= RF_Transient;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		mSkillWorldPreviewCapture = World->SpawnActor<ASceneCapture2D>(Params);
	}
	USceneCaptureComponent2D* CaptureComponent = mSkillWorldPreviewCapture != nullptr
		? mSkillWorldPreviewCapture->GetCaptureComponent2D() : nullptr;
	if (CaptureComponent == nullptr)
	{
		return false;
	}
	CaptureComponent->TextureTarget = mSkillWorldPreviewRenderTarget;
	CaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	CaptureComponent->bCaptureEveryFrame = true;
	CaptureComponent->bCaptureOnMovement = true;
	CaptureComponent->bAlwaysPersistRenderingState = true;
	CaptureComponent->ShowFlags.SetMotionBlur(false);

	FSlateBrush Brush;
	Brush.SetResourceObject(mSkillWorldPreviewRenderTarget);
	Brush.ImageSize = FVector2D(1024.f, 384.f);
	Brush.DrawAs = ESlateBrushDrawType::Image;
	mSkillWorldPreviewImage->SetBrush(Brush);
	mSkillWorldPreviewImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	mSkillWorldPreviewActive = true;
	SyncSkillWorldPreviewCamera(true);
	return true;
}

void UCombatLayoutHUDWidget::SyncSkillWorldPreviewCamera(const bool bCaptureImmediately)
{
	if (mSkillWorldPreviewCapture == nullptr || mSkillWorldPreviewSourceCamera == nullptr)
	{
		StopSkillWorldPreview();
		return;
	}
	UCameraComponent* Source = mSkillWorldPreviewSourceCamera;
	USceneCaptureComponent2D* Capture = mSkillWorldPreviewCapture->GetCaptureComponent2D();
	if (Source == nullptr || Capture == nullptr)
	{
		StopSkillWorldPreview();
		return;
	}
	mSkillWorldPreviewCapture->SetActorTransform(Source->GetComponentTransform());
	Capture->ProjectionType = Source->ProjectionMode;
	Capture->FOVAngle = Source->FieldOfView;
	// 패널이 본 화면보다 가로로 길다. 같은 폭을 쓰면 전장이 지나치게 작아지므로
	// 실제 카메라 중심은 유지하고 약간만 줌인한다.
	Capture->OrthoWidth = Source->OrthoWidth * 0.78f;
	Capture->PostProcessSettings = Source->PostProcessSettings;
	Capture->PostProcessBlendWeight = Source->PostProcessBlendWeight;
	if (bCaptureImmediately)
	{
		Capture->CaptureScene();
	}
}

void UCombatLayoutHUDWidget::StopSkillWorldPreview()
{
	mSkillWorldPreviewActive = false;
	if (mSkillWorldPreviewCapture != nullptr
		&& mSkillWorldPreviewCapture->GetCaptureComponent2D() != nullptr)
	{
		mSkillWorldPreviewCapture->GetCaptureComponent2D()->bCaptureEveryFrame = false;
	}
	SetShown(mSkillWorldPreviewImage, false);
}

void UCombatLayoutHUDWidget::ReleaseSkillWorldPreview()
{
	StopSkillWorldPreview();
	if (mSkillWorldPreviewCapture != nullptr)
	{
		mSkillWorldPreviewCapture->Destroy();
	}
	mSkillWorldPreviewCapture = nullptr;
	mSkillWorldPreviewSourceCamera = nullptr;
	if (mSkillWorldPreviewRenderTarget != nullptr)
	{
		mSkillWorldPreviewRenderTarget->ReleaseResource();
	}
	mSkillWorldPreviewRenderTarget = nullptr;
	mSkillWorldPreviewImage = nullptr;
}

/** @brief 스킬 상세를 패널에 채워 띄운다. 수치는 요청한 유닛의 상세 DTO에서 읽는다. */
void UCombatLayoutHUDWidget::ShowSkillDetailOverlay()
{
	if (mUIModel == nullptr)
	{
		return;
	}
	const FSkillDetailUI& Detail = mUIModel->GetSkillDetail();
	if (Detail.mSkillIndex == INDEX_NONE)
	{
		return;
	}
	if (!Detail.mName.IsEmpty() || Detail.mIcon != nullptr)
	{
		ShowSkillDetailOverlay(Detail);
		return;
	}

	// 데이터 생산자가 부분 DTO만 돌려준 경우에도 빈 상세판을 띄우지 않는다.
	// 전투 카드 View에 이미 내려온 이름/아이콘/AP를 같은 슬롯에서 보완한다.
	FSkillDetailUI Fallback = Detail;
	const TArray<FSkillUI>& Skills = mUIModel->GetSkillUIs();
	const FSkillUI* Skill = Skills.FindByPredicate([&Detail](const FSkillUI& Candidate)
	{
		return Candidate.mSkillIndex == Detail.mSkillIndex;
	});
	if (Skill != nullptr)
	{
		Fallback.mName = Skill->mName;
		Fallback.mIcon = Skill->mIcon;
		Fallback.mActionPointCost = Skill->mActionPointCost;
		Fallback.mDescription = LOCTEXT("SkillDetailFallbackDescription",
			"현재 전투에서 사용할 수 있는 스킬입니다.");
		ShowSkillDetailOverlay(Fallback);
	}
}

void UCombatLayoutHUDWidget::HandleCancelClicked()
{
	if (mUIModel == nullptr || !IsAiming())
	{
		return;
	}
	HideDetailOverlay(/*bNotifyGameplay=*/false);
	mUIModel->RequestCancel();
}

void UCombatLayoutHUDWidget::ShowSkillDetailOverlay(const FSkillDetailUI& Detail)
{
	// 풍부한 렌더는 프레젠터(Present)가 DTO만 받아 그린다. HUD 는 겹 부품을
	// 비추고, 유닛 상세의 스킬 칸 줄과 월드 제스처 잠금 같은 HUD 상태만 맡는다.
	if (EnsureDetailOverlayWidget() == false)
	{
		return;
	}
	#if WITH_DEV_AUTOMATION_TESTS
	// WBP 없이 도는 자동화에서도 실제 렌더 함수가 어느 DTO를 소비했는지 남긴다.
	mRenderedSkillDetailForTest = Detail;
	#endif
	mDetailPresenter->Present(Detail);
	// 이 칸들은 유닛 상세의 것이다. 스킬 하나를 보는 중에는 걷는다.
	SetDetailSkillRowShown(false);
	RefreshWorldGestureInputBlock();
}

#if WITH_DEV_AUTOMATION_TESTS
FString UCombatLayoutHUDWidget::GetDetailChipValueForTest(const int32 ChipIndex) const
{
	return mDetailPresenter != nullptr
		? mDetailPresenter->GetChipValueString(ChipIndex) : FString();
}

FString UCombatLayoutHUDWidget::GetDetailSubtitleForTest() const
{
	return mDetailSubtitleText != nullptr
		? mDetailSubtitleText->GetText().ToString() : FString();
}
#endif

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
	if (mUIModel == nullptr)
	{
		return;
	}
	const FUnitUI* TurnUnit = FindTurnUnit();
	const int32 Left = TurnUnit != nullptr
		? FMath::Max(FMath::RoundToInt(TurnUnit->mMovementPoint), 0) : 0;

	// 이동도 별도 판을 손으로 조립하지 않고 스킬과 같은 DTO→공용 상세 WBP
	// 렌더 경로를 탄다. 그래서 제목/수치 메달/전술 보드/닫기 동작이 동일하다.
	UTexture2D* MoveIcon = nullptr;
	if (mCommandSlots.IsValidIndex(0) && mCommandSlots[0].Icon != nullptr)
	{
		MoveIcon = Cast<UTexture2D>(mCommandSlots[0].Icon->GetBrush().GetResourceObject());
	}

	FSkillDetailUI MoveDetail;
	MoveDetail.mSkillIndex = -2; // INDEX_NONE과 구분되는 UI 전용 이동 식별자.
	MoveDetail.mName = LOCTEXT("MoveDetailTitle", "이동");
	MoveDetail.mDescription = FText::Format(LOCTEXT("MoveDetailDescription",
		"한 칸 옮길 때마다 행동력을 1 쓴다. 지금 남은 행동력으로 최대 {0}칸 갈 수 있다.\n\n판을 톡 쳐서 갈 곳을 고르고, 마지막 칸을 다시 누르면 이동을 확정한다."),
		Left);
	MoveDetail.mIcon = MoveIcon;
	MoveDetail.mActionPointCost = 1;
	MoveDetail.mTargeting.mSelectShape = ECombatSkillSelectShapeUI::Cross;
	MoveDetail.mTargeting.mSelectRange = static_cast<float>(Left);
	MoveDetail.mTargeting.mHitShape = ECombatSkillHitShapeUI::Single;
	MoveDetail.mTargeting.mHitRange = 0.f;
	MoveDetail.mTargeting.mAimBlockerMask = 0;
	MoveDetail.mTargeting.mEffectBlockerMask = 0;
	ShowSkillDetailOverlay(MoveDetail);
}

void UCombatLayoutHUDWidget::HideDetailOverlay(const bool bNotifyGameplay)
{
	if (IsDetailOverlayShown() == false)
	{
		return;
	}
	mDetailOverlayWidget->SetVisibility(ESlateVisibility::Collapsed);
	StopSkillWorldPreview();
	RefreshWorldGestureInputBlock();

	// 카메라는 안 건드린다. 즉시 이동 방식이라 잡아 둔 것이 없고,
	// 해제 신호를 보내 봐야 게임플레이가 무시한다(0807 감사에서 노-옵 확인).

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
