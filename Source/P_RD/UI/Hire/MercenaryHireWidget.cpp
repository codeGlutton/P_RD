#include "UI/Hire/MercenaryHireWidget.h"
#include "UI/DetailOverlayInputShield.h"
#include "UI/Shop/ShopUITypes.h"
#include "UI/Combat/CombatUITypes.h"
#include "UI/Combat/SkillDetailOverlayPresenter.h"
#include "UI/Combat/SkillDetailUIBuilder.h"
#include "UI/Combat/SkillTacticalDiagramWidget.h"
#include "UI/TextOpticalAlignment.h"

#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Actor/TileMap/TileLayer.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "MercenaryHire"

// 이름을 붙인 이름공간에 둔다. 익명으로 두면 유니티 빌드가 여러 파일을 한
// 덩어리로 묶을 때 전투 위젯의 같은 이름과 부딪힌다 -- 파일이 하나 늘어
// 묶이는 조합이 바뀌는 날 갑자기 깨진다.
namespace MercenaryHireDetail
{
	/** @brief 화면에 걸리는 이력서 칸 수. WBP 가 여섯 칸으로 구워져 있다. */
	constexpr int32 CardCount = 6;
	/** Marchbound 용병판이 타이틀과 공유하는 폰트 스타일의 전역 광학 보정값. */
	constexpr float HireTextOpticalBiasY = 1.0f;

	const TCHAR* IconPaths[CardCount] = {
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Knight.T_MB_HireIcon_Knight"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Mage.T_MB_HireIcon_Mage"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Ranger.T_MB_HireIcon_Ranger"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Rogue.T_MB_HireIcon_Rogue"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Barbarian.T_MB_HireIcon_Barbarian"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Druid.T_MB_HireIcon_Druid")
	};

	/** 배경을 제거한 전경 전용 원화. 뒤에는 직업별 생성 배경을 따로 그린다. */
	const TCHAR* HeroCutoutPaths[CardCount] = {
		TEXT("/Game/UI/MercenaryHire/HeroCutouts/T_HireHeroCutout_Knight_v1.T_HireHeroCutout_Knight_v1"),
		TEXT("/Game/UI/MercenaryHire/HeroCutouts/T_HireHeroCutout_Mage_v1.T_HireHeroCutout_Mage_v1"),
		TEXT("/Game/UI/MercenaryHire/HeroCutouts/T_HireHeroCutout_Ranger_v1.T_HireHeroCutout_Ranger_v1"),
		TEXT("/Game/UI/MercenaryHire/HeroCutouts/T_HireHeroCutout_Rogue_v1.T_HireHeroCutout_Rogue_v1"),
		TEXT("/Game/UI/MercenaryHire/HeroCutouts/T_HireHeroCutout_Barbarian_v1.T_HireHeroCutout_Barbarian_v1"),
		TEXT("/Game/UI/MercenaryHire/HeroCutouts/T_HireHeroCutout_Druid_v1.T_HireHeroCutout_Druid_v1")
	};

	/** 각 직업의 전경과 독립적으로 제작한 초광폭 색상 배경. */
	const TCHAR* GeneratedBackgroundPaths[CardCount] = {
		TEXT("/Game/UI/MercenaryHire/GeneratedBackgrounds/T_HireGeneratedBG_Knight_v1.T_HireGeneratedBG_Knight_v1"),
		TEXT("/Game/UI/MercenaryHire/GeneratedBackgrounds/T_HireGeneratedBG_Mage_v1.T_HireGeneratedBG_Mage_v1"),
		TEXT("/Game/UI/MercenaryHire/GeneratedBackgrounds/T_HireGeneratedBG_Ranger_v1.T_HireGeneratedBG_Ranger_v1"),
		TEXT("/Game/UI/MercenaryHire/GeneratedBackgrounds/T_HireGeneratedBG_Rogue_v1.T_HireGeneratedBG_Rogue_v1"),
		TEXT("/Game/UI/MercenaryHire/GeneratedBackgrounds/T_HireGeneratedBG_Barbarian_v1.T_HireGeneratedBG_Barbarian_v1"),
		TEXT("/Game/UI/MercenaryHire/GeneratedBackgrounds/T_HireGeneratedBG_Druid_v1.T_HireGeneratedBG_Druid_v1")
	};

	UTexture2D* LoadTexture(const TCHAR* const* Paths, const int32 Index)
	{
		return Index >= 0 && Index < CardCount
			? LoadObject<UTexture2D>(nullptr, Paths[Index]) : nullptr;
	}

	int32 ArtIndexForJob(const EUnitJobType JobType)
	{
		switch (JobType)
		{
		case EUnitJobType::Knight:    return 0;
		case EUnitJobType::Mage:      return 1;
		case EUnitJobType::Ranger:    return 2;
		case EUnitJobType::Rogue:     return 3;
		case EUnitJobType::Barbarian: return 4;
		case EUnitJobType::Druid:     return 5;
		default:                      return INDEX_NONE;
		}
	}

	FText JobDisplayName(const EUnitJobType JobType)
	{
		switch (JobType)
		{
		case EUnitJobType::Knight: return LOCTEXT("HireJobKnight", "기사");
		case EUnitJobType::Mage: return LOCTEXT("HireJobMage", "마법사");
		case EUnitJobType::Ranger: return LOCTEXT("HireJobRanger", "레인저");
		case EUnitJobType::Rogue: return LOCTEXT("HireJobRogue", "도적");
		case EUnitJobType::Barbarian: return LOCTEXT("HireJobBarbarian", "야만전사");
		case EUnitJobType::Druid: return LOCTEXT("HireJobDruid", "드루이드");
		default: return LOCTEXT("HireJobUnknown", "용병");
		}
	}

	template <typename T>
	T* Find(UWidgetTree* Tree, const FString& Name)
	{
		return Tree != nullptr ? Cast<T>(Tree->FindWidget(FName(*Name)))
			: nullptr;
	}

	void SetShown(UWidget* Widget, const bool bShown)
	{
		if (Widget != nullptr)
		{
			Widget->SetVisibility(bShown
				? ESlateVisibility::SelfHitTestInvisible
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

	/** @brief 못 부르는 이력서는 흐리게. 눌러도 아무 일 없다는 표시다. */
	void SetDimmed(UWidget* Widget, const bool bDimmed)
	{
		if (Widget != nullptr)
		{
			Widget->SetRenderOpacity(bDimmed ? 0.45f : 1.0f);
		}
	}

	/**
	 * @brief 파티 슬롯 이름 밴드를 레벨 줄 유무에 맞춘다.
	 *
	 * @details
	 * 빌더는 이름을 칸 상단 62%, 레벨을 하단 밴드로 굽는다. 레벨을 안 쓰는
	 * 캐릭터 선택 화면에서 그대로 두면 이름이 위로 치우쳐 보이므로, 레벨이
	 * 없을 때는 이름 밴드를 칸 전체로 늘여 세로 중앙에 오게 한다.
	 */
	void SetPartySlotNameBand(const FMercenarySlotWidgets& Widgets, const bool bWithLevelRow)
	{
		if (Widgets.mNameBand == nullptr || Widgets.mNameBandBaseSize.IsNearlyZero())
		{
			return;
		}
		UCanvasPanelSlot* NameSlot = Cast<UCanvasPanelSlot>(Widgets.mNameBand->Slot);
		if (NameSlot == nullptr)
		{
			return;
		}

		if (bWithLevelRow || Widgets.mLevelBandBaseSize.IsNearlyZero())
		{
			NameSlot->SetPosition(Widgets.mNameBandBasePosition);
			NameSlot->SetSize(Widgets.mNameBandBaseSize);
			return;
		}

		// 레벨을 표시하지 않는 초기 편성 화면에서는 두 Center 슬롯의 실제
		// 합집합을 이름 슬롯이 차지한다. 빌더 비율(0.62/0.38)을 다시 추측하지 않는다.
		const FVector2D UnionMin(
			FMath::Min(Widgets.mNameBandBasePosition.X, Widgets.mLevelBandBasePosition.X),
			FMath::Min(Widgets.mNameBandBasePosition.Y, Widgets.mLevelBandBasePosition.Y));
		const FVector2D NameMax = Widgets.mNameBandBasePosition + Widgets.mNameBandBaseSize;
		const FVector2D LevelMax = Widgets.mLevelBandBasePosition + Widgets.mLevelBandBaseSize;
		const FVector2D UnionMax(FMath::Max(NameMax.X, LevelMax.X),
			FMath::Max(NameMax.Y, LevelMax.Y));
		NameSlot->SetPosition(UnionMin);
		NameSlot->SetSize(UnionMax - UnionMin);
	}

}

UMercenaryHireWidget::UMercenaryHireWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 문자열 로드만 두면 Android cook에서 상세 WBP가 빠질 수 있어 생성자 하드
	// 레퍼런스로 잡는다. 전투 HUD가 같은 자산에 쓰는 방식과 같다.
	static ConstructorHelpers::FClassFinder<UUserWidget> DetailOverlayClassFinder(
		TEXT("/Game/UI/CombatDetail/WBP_CombatDetailOverlay"));
	if (DetailOverlayClassFinder.Succeeded())
	{
		mSkillDetailOverlayClass = DetailOverlayClassFinder.Class;
	}
	static ConstructorHelpers::FClassFinder<USkillTacticalDiagramWidget> DiagramClassFinder(
		TEXT("/Game/UI/CombatDetail/SkillTactical/WBP_SkillTacticalDiagram"));
	if (DiagramClassFinder.Succeeded())
	{
		mSkillTacticalDiagramClass = DiagramClassFinder.Class;
	}
}

void UMercenaryHireWidget::NativeConstruct()
{
	Super::NativeConstruct();
	CacheWidgets();
	ApplyMarchboundPortraits();
	// 월드 위젯은 뷰포트에 붙기 전에 데이터를 받는다. 이 시점에 신규 레이아웃
	// 여부가 확정되므로 첫 후보를 상세 대상으로 잡아 실제 능력치를 즉시 갱신한다.
	if (mIsMarchboundLayout && !mCrew.IsEmpty() && !mCrew.IsValidIndex(mReviewing))
	{
		mReviewing = 0;
	}
	Refresh();
}

void UMercenaryHireWidget::NativeDestruct()
{
	if (mSkillDetailPresenter != nullptr)
	{
		mSkillDetailPresenter->Teardown();
		mSkillDetailPresenter = nullptr;
	}
	if (mSkillDetailOverlay != nullptr)
	{
		mSkillDetailOverlay->RemoveFromParent();
		mSkillDetailOverlay = nullptr;
	}
	Super::NativeDestruct();
}

void UMercenaryHireWidget::ApplyCloseUI()
{
	HideSkillDetailOverlay();
	Super::ApplyCloseUI();
}

void UMercenaryHireWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!mIsMarchboundLayout)
	{
		return;
	}

	const FVector2D ViewportSize = MyGeometry.GetLocalSize();
	if (ViewportSize.X > 1.0f && ViewportSize.Y > 1.0f
		&& !ViewportSize.Equals(mLastResponsiveSize, 1.0f))
	{
		mLastResponsiveSize = ViewportSize;
		ApplyResponsiveLayout(ViewportSize);
	}
}

void UMercenaryHireWidget::SetCharacterOptions(
	const TArray<FFrontendCharacterOption>& Options, const int32 PartySize)
{
	HideSkillDetailOverlay();
	mIsShopMode = false;
	mShopCandidates.Reset();
	mShopPartySlots.Reset();
	mCrew = Options;
	ApplyMarchboundPortraits();
	mPartySize = FMath::Max(1, PartySize);
	mChosen.Reset();
	mReviewing = mIsMarchboundLayout && !mCrew.IsEmpty() ? 0 : INDEX_NONE;
	Refresh();
}

void UMercenaryHireWidget::SetShopMode(
	const TArray<FShopMercenaryUI>& Candidates,
	const TArray<FShopMercenaryPartySlotUI>& PartySlots, const int32 CurrentGold)
{
	HideSkillDetailOverlay();
	mIsShopMode = true;
	mShopCandidates = Candidates;
	mShopPartySlots = PartySlots;
	mShopGold = FMath::Max(0, CurrentGold);
	mCrew.Reset(Candidates.Num());
	for (const FShopMercenaryUI& Candidate : Candidates)
	{
		mCrew.Add(Candidate.mCharacter);
	}
	ApplyMarchboundPortraits();
	mChosen.Reset();
	mPartySize = 3;
	mShopTargetPartyViewIndex = FMath::Clamp(
		mShopTargetPartyViewIndex, 0, FMath::Max(0, PartySlots.Num() - 1));
	mReviewing = mCrew.IsEmpty() ? INDEX_NONE : 0;
	Refresh();
}

void UMercenaryHireWidget::ClearShopMode()
{
	mIsShopMode = false;
	mShopCandidates.Reset();
	mShopPartySlots.Reset();
	mShopTargetPartyViewIndex = 0;
	mShopGold = 0;
}

void UMercenaryHireWidget::ApplyMarchboundPortraits()
{
	if (!mIsMarchboundLayout)
	{
		return;
	}

	// 후보 수·스탯·선택 가능 여부는 FrontendGameMode가 내려준 실제 데이터가
	// 유일한 출처다. 이 화면은 직업에 맞는 Marchbound 전용 그림만 바꾼다.
	for (FFrontendCharacterOption& Option : mCrew)
	{
		const int32 ArtIndex = MercenaryHireDetail::ArtIndexForJob(Option.mJobType);
		if (ArtIndex == INDEX_NONE)
		{
			continue;
		}

		Option.mIcon = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(
			MercenaryHireDetail::IconPaths[ArtIndex]));
	}
}

void UMercenaryHireWidget::ApplyResponsiveLayout(const FVector2D& ViewportSize)
{
	/*
	 * 비율에 따른 세로형 재배치는 쓰지 않는다(0811 결정).
	 *
	 * 1.15 경계로 목록이 좌측 세로 ↔ 하단 그리드로 갈아타는 구조였는데, 창
	 * 비율이 바뀔 때마다 화면 구조가 통째로 바뀌어 "맘대로 바뀐다"고 읽혔고
	 * 세로형에서는 출발 단추가 스탯 줄과 겹치기까지 했다. 구조는 가로형
	 * 하나로 고정한다 -- 좁은 화면에서는 각 구역 ScaleBox 가 구조를 그대로
	 * 둔 채 줄어든다. 세로형이 다시 필요하면 이 함수 이력을 되살릴 것.
	 */
	const bool bPortrait = false;
	mHasAppliedResponsiveLayout = true;
	mIsPortraitLayout = bPortrait;

	auto SetAnchors = [this](const TCHAR* Name, const FAnchors Anchors)
	{
		if (UWidget* Widget = WidgetTree->FindWidget(Name))
		{
			if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Widget->Slot))
			{
				Slot->SetAnchors(Anchors);
				Slot->SetOffsets(FMargin(0.0f));
			}
		}
	};
	auto SetDesignSize = [this](const TCHAR* Name, const FVector2D Size)
	{
		if (USizeBox* Box = MercenaryHireDetail::Find<USizeBox>(WidgetTree, Name))
		{
			Box->SetWidthOverride(Size.X);
			Box->SetHeightOverride(Size.Y);
		}
	};
	auto SetRect = [this](const TCHAR* Name, const FVector2D Position, const FVector2D Size)
	{
		if (UWidget* Widget = WidgetTree->FindWidget(Name))
		{
			if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Widget->Slot))
			{
				Slot->SetPosition(Position);
				Slot->SetSize(Size);
			}
		}
	};

	if (bPortrait)
	{
		// 세로 화면에서는 세로 목록을 억지로 축소하지 않고 2x3 터치 그리드로
		// 재배치한다. 중앙 영웅은 위쪽, 파티는 우상단, 후보는 하단을 차지한다.
		SetAnchors(TEXT("HireCenterScale"), FAnchors(0.02f, 0.0f, 0.98f, 0.68f));
		// 파티 열은 아래로 더 내려 잡는다. 0.61 에서 끊으면 그 안의 출발
		// 단추가 중앙 스탯 줄 높이에 걸려, 스탯 판이 출발을 반쯤 덮었다(0811).
		SetAnchors(TEXT("HireRightScale"), FAnchors(0.53f, 0.06f, 0.98f, 0.68f));
		SetAnchors(TEXT("HireLeftScale"), FAnchors(0.0f, 0.63f, 1.0f, 1.0f));
		SetDesignSize(TEXT("HireLeftSize"), FVector2D(1080.0f, 500.0f));
		// 출발도 프레임 아래로 더 내려 스탯 줄과 세로로 분리한다.
		SetRect(TEXT("DepartHolder"), FVector2D(100.0f, 890.0f), FVector2D(340.0f, 160.0f));
		MercenaryHireDetail::SetShown(WidgetTree->FindWidget(TEXT("HireListFrameArt")), false);
		for (int32 Index = 0; Index < MercenaryHireDetail::CardCount; ++Index)
		{
			SetRect(*FString::Printf(TEXT("HireCard_%d"), Index),
				FVector2D(Index % 2 == 0 ? 90.0f : 570.0f,
					20.0f + 124.0f * (Index / 2)), FVector2D(420.0f, 116.0f));
		}
		SetRect(TEXT("HireBackHolder"), FVector2D(405.0f, 390.0f), FVector2D(270.0f, 106.0f));
	}
	else
	{
		SetAnchors(TEXT("HireLeftScale"), FAnchors(0.0f, 0.0f, 0.30f, 1.0f));
		SetAnchors(TEXT("HireCenterScale"), FAnchors(0.27f, 0.0f, 0.73f, 1.0f));
		SetAnchors(TEXT("HireRightScale"), FAnchors(0.70f, 0.0f, 1.0f, 1.0f));
		SetDesignSize(TEXT("HireLeftSize"), FVector2D(555.0f, 1080.0f));
		// 가로형 원래 자리로 되돌린다 -- 세로형에서 내려 둔 값이 남으면 안 된다.
		SetRect(TEXT("DepartHolder"), FVector2D(148.0f, 962.0f), FVector2D(224.0f, 106.0f));
		SetRect(TEXT("HireAddHolder"), FVector2D(287.5f, 962.0f), FVector2D(270.0f, 106.0f));
		MercenaryHireDetail::SetShown(WidgetTree->FindWidget(TEXT("HireListFrameArt")), true);
		for (int32 Index = 0; Index < MercenaryHireDetail::CardCount; ++Index)
		{
			SetRect(*FString::Printf(TEXT("HireCard_%d"), Index),
				FVector2D(95.0f, 158.0f + 124.0f * Index), FVector2D(420.0f, 116.0f));
		}
		SetRect(TEXT("HireBackHolder"), FVector2D(70.0f, 962.0f), FVector2D(270.0f, 106.0f));
	}

	// The three regional ScaleBoxes preserve their own authored widths. On a Fold
	// viewport width becomes the limiting axis, leaving a vertical gutter above
	// and below each 1080-high regional board. Keep the candidate/party columns
	// vertically centered, while stats + skill descriptions form a bottom detail
	// cluster directly above Add. The three actions remain pinned to the bottom.
	const auto LocalVerticalEdgeOffset = [&ViewportSize](
		const float RegionWidthRatio, const float RegionDesignWidth) -> float
	{
		constexpr float RegionDesignHeight = 1080.0f;
		const float Scale = FMath::Min(
			(ViewportSize.X * RegionWidthRatio) / RegionDesignWidth,
			ViewportSize.Y / RegionDesignHeight);
		if (Scale <= UE_SMALL_NUMBER)
		{
			return 0.0f;
		}
		const float VerticalGutter = FMath::Max(0.0f,
			(ViewportSize.Y - RegionDesignHeight * Scale) * 0.5f);
		return VerticalGutter / Scale;
	};
	const float LeftEdgeOffset = LocalVerticalEdgeOffset(0.30f, 555.0f);
	const float CenterEdgeOffset = LocalVerticalEdgeOffset(0.46f, 845.0f);
	const float RightEdgeOffset = LocalVerticalEdgeOffset(0.30f, 520.0f);

	const auto SetTranslation = [this](const TCHAR* Name, const float Y)
	{
		if (UWidget* Widget = WidgetTree != nullptr
			? WidgetTree->FindWidget(Name) : nullptr)
		{
			Widget->SetRenderTranslation(FVector2D(0.0f, Y));
		}
	};
	SetTranslation(TEXT("HireLeftTopGroup"), 0.0f);
	SetTranslation(TEXT("HireCenterTopGroup"), 0.0f);
	SetTranslation(TEXT("HireCenterMiddleGroup"), CenterEdgeOffset);
	SetTranslation(TEXT("HireRightTopGroup"), 0.0f);
	SetTranslation(TEXT("HireBackHolder"), LeftEdgeOffset);
	SetTranslation(TEXT("HireAddHolder"), CenterEdgeOffset);
	SetTranslation(TEXT("DepartHolder"), RightEdgeOffset);
}

void UMercenaryHireWidget::CacheWidgets()
{
	// 이름으로 찾는다. WBP 를 파이썬이 구우므로 BindWidget 으로 묶으면 이름이
	// 하나 어긋날 때 컴파일이 깨지고, 그러면 다시 굽는 것조차 못 한다.
	mIsMarchboundLayout = WidgetTree != nullptr
		&& WidgetTree->FindWidget(TEXT("HireListFrameArt")) != nullptr;
	// 중앙 일러스트를 가리던 중복 제목/클래스명 판은 전체를 걷는다. 빌더가
	// 구운 기본값도 숨기지만, 이전 WBP가 들어와도 런타임 계약은 유지한다.
	MercenaryHireDetail::SetShown(WidgetTree != nullptr
		? WidgetTree->FindWidget(TEXT("HireTitlePanel")) : nullptr, false);
	MercenaryHireDetail::SetShown(WidgetTree != nullptr
		? WidgetTree->FindWidget(TEXT("HireDetailNamePanel")) : nullptr, false);
	mCards.Reset();
	mCards.SetNum(MercenaryHireDetail::CardCount);
	for (int32 Index = 0; Index < MercenaryHireDetail::CardCount; ++Index)
	{
		const FString Tail = FString::Printf(TEXT("_%d"), Index);
		FMercenaryCardWidgets& Card = mCards[Index];
		Card.mRoot = MercenaryHireDetail::Find<UWidget>(WidgetTree, TEXT("HireCard") + Tail);
		Card.mButton = MercenaryHireDetail::Find<UButton>(WidgetTree, TEXT("HireButton") + Tail);
		Card.mPortrait = MercenaryHireDetail::Find<UImage>(WidgetTree, TEXT("HirePortrait") + Tail);
		Card.mName = MercenaryHireDetail::Find<UTextBlock>(WidgetTree, TEXT("HireName") + Tail);
		Card.mRole = MercenaryHireDetail::Find<UTextBlock>(WidgetTree, TEXT("HireRole") + Tail);
		if (Card.mName != nullptr) Card.mName->SetJustification(ETextJustify::Center);
		if (Card.mRole != nullptr) Card.mRole->SetJustification(ETextJustify::Center);
		Card.mHP = MercenaryHireDetail::Find<UTextBlock>(WidgetTree, TEXT("HireHP") + Tail);
		// 신규 목록 행은 상태 배지 대신 선택 프레임과 우측 인장을 쓴다.
		Card.mBadge = mIsMarchboundLayout ? nullptr
			: MercenaryHireDetail::Find<UTextBlock>(WidgetTree, TEXT("HireBadge") + Tail);
		Card.mSeal = MercenaryHireDetail::Find<UWidget>(WidgetTree, TEXT("HireSeal") + Tail);
		Card.mSelected = MercenaryHireDetail::Find<UWidget>(WidgetTree, TEXT("HireSelected") + Tail);
		Card.mTrait = MercenaryHireDetail::Find<UTextBlock>(WidgetTree, TEXT("HireTrait") + Tail);

		Card.mSkills.Reset();
		if (!mIsMarchboundLayout)
		{
			for (int32 Line = 0; Line < 2; ++Line)
			{
				Card.mSkills.Add(MercenaryHireDetail::Find<UTextBlock>(WidgetTree,
					FString::Printf(TEXT("HireSkill_%d_%d"), Index, Line)));
			}
		}
	}

	mSlots.Reset();
	mSlots.SetNum(3);
	for (int32 Index = 0; Index < mSlots.Num(); ++Index)
	{
		const FString Tail = FString::Printf(TEXT("_%d"), Index);
		mSlots[Index].mRoot = MercenaryHireDetail::Find<UWidget>(WidgetTree, TEXT("PartySlot") + Tail);
		mSlots[Index].mButton = MercenaryHireDetail::Find<UButton>(WidgetTree,
			TEXT("PartySlotButton") + Tail);
		mSlots[Index].mFace = MercenaryHireDetail::Find<UImage>(WidgetTree,
			TEXT("PartySlotFace") + Tail);
		mSlots[Index].mName = MercenaryHireDetail::Find<UTextBlock>(WidgetTree,
			TEXT("PartySlotName") + Tail);
		mSlots[Index].mLevel = MercenaryHireDetail::Find<UTextBlock>(WidgetTree,
			TEXT("PartySlotLevel") + Tail);
		mSlots[Index].mPlus = MercenaryHireDetail::Find<UWidget>(WidgetTree,
			TEXT("PartySlotPlus") + Tail);
		mSlots[Index].mNameBand = MercenaryHireDetail::Find<UWidget>(WidgetTree,
			TEXT("PartySlotName") + Tail + TEXT("_Center"));
		mSlots[Index].mLevelBand = MercenaryHireDetail::Find<UWidget>(WidgetTree,
			TEXT("PartySlotLevel") + Tail + TEXT("_Center"));
		if (mSlots[Index].mName != nullptr)
		{
			mSlots[Index].mName->SetJustification(ETextJustify::Center);
		}
		if (mSlots[Index].mLevel != nullptr)
		{
			mSlots[Index].mLevel->SetJustification(ETextJustify::Center);
		}
		if (UCanvasPanelSlot* NameSlot = mSlots[Index].mNameBand != nullptr
			? Cast<UCanvasPanelSlot>(mSlots[Index].mNameBand->Slot) : nullptr)
		{
			mSlots[Index].mNameBandBasePosition = NameSlot->GetPosition();
			mSlots[Index].mNameBandBaseSize = NameSlot->GetSize();
		}
		if (UCanvasPanelSlot* LevelSlot = mSlots[Index].mLevelBand != nullptr
			? Cast<UCanvasPanelSlot>(mSlots[Index].mLevelBand->Slot) : nullptr)
		{
			mSlots[Index].mLevelBandBasePosition = LevelSlot->GetPosition();
			mSlots[Index].mLevelBandBaseSize = LevelSlot->GetSize();
		}
	}

	mPartyCountText = MercenaryHireDetail::Find<UTextBlock>(WidgetTree, TEXT("PartyCountText"));

	mAddButton = MercenaryHireDetail::Find<UButton>(WidgetTree, TEXT("HireAddButton"));
	mAddLabel = MercenaryHireDetail::Find<UTextBlock>(WidgetTree, TEXT("HireAddLabel"));
	mDepartButton = MercenaryHireDetail::Find<UButton>(WidgetTree, TEXT("DepartButton"));
	mDepartLabel = MercenaryHireDetail::Find<UTextBlock>(WidgetTree, TEXT("DepartLabel"));
	mBackButton = MercenaryHireDetail::Find<UButton>(WidgetTree, TEXT("HireBackButton"));
	mDetailName = MercenaryHireDetail::Find<UTextBlock>(WidgetTree, TEXT("HireDetailName"));
	mDetailHP = MercenaryHireDetail::Find<UTextBlock>(WidgetTree, TEXT("HireDetailHP"));
	mDetailAP = MercenaryHireDetail::Find<UTextBlock>(WidgetTree, TEXT("HireDetailAP"));
	mDetailSpeed = MercenaryHireDetail::Find<UTextBlock>(WidgetTree, TEXT("HireDetailSpeed"));
	for (UTextBlock* Text : { mPartyCountText.Get(), mAddLabel.Get(),
		mDepartLabel.Get(), mDetailName.Get(), mDetailHP.Get(),
		mDetailAP.Get(), mDetailSpeed.Get() })
	{
		if (Text != nullptr)
		{
			Text->SetJustification(ETextJustify::Center);
		}
	}
	mHeroIllustration = MercenaryHireDetail::Find<UImage>(WidgetTree, TEXT("Backdrop_Art"));
	mHeroGeneratedBackground = MercenaryHireDetail::Find<UImage>(
		WidgetTree, TEXT("HireGeneratedBackgroundArt"));
	mDetailSkills.Reset();
	mDetailSkillIcons.Reset();
	mDetailSkillButtons.Reset();
	for (int32 Index = 0; Index < 6; ++Index)
	{
		mDetailSkills.Add(MercenaryHireDetail::Find<UTextBlock>(WidgetTree,
			FString::Printf(TEXT("HireDetailSkillText_%d"), Index)));
		mDetailSkillIcons.Add(MercenaryHireDetail::Find<UImage>(WidgetTree,
			FString::Printf(TEXT("HireDetailSkillIcon_%d"), Index)));
		mDetailSkillButtons.Add(MercenaryHireDetail::Find<UButton>(WidgetTree,
			FString::Printf(TEXT("HireDetailSkillButton_%d"), Index)));
	}

	// 슬롯별 핸들러를 따로 두는 이유: 동적 델리게이트는 페이로드를 못 받고
	// 함수 포인터가 아니라 이름으로 묶여서, 하나로 넘기면 조용히 실패한다.
	struct FCardHandler
	{
		void (UMercenaryHireWidget::*Function)();
		const TCHAR* Name;
	};
	static const FCardHandler Handlers[MercenaryHireDetail::CardCount] = {
		{ &UMercenaryHireWidget::HandleCardClicked_0, TEXT("HandleCardClicked_0") },
		{ &UMercenaryHireWidget::HandleCardClicked_1, TEXT("HandleCardClicked_1") },
		{ &UMercenaryHireWidget::HandleCardClicked_2, TEXT("HandleCardClicked_2") },
		{ &UMercenaryHireWidget::HandleCardClicked_3, TEXT("HandleCardClicked_3") },
		{ &UMercenaryHireWidget::HandleCardClicked_4, TEXT("HandleCardClicked_4") },
		{ &UMercenaryHireWidget::HandleCardClicked_5, TEXT("HandleCardClicked_5") },
	};
	for (int32 Index = 0; Index < MercenaryHireDetail::CardCount; ++Index)
	{
		if (UButton* Button = mCards[Index].mButton)
		{
			Button->OnClicked.__Internal_AddUniqueDynamic(
				this, Handlers[Index].Function, Handlers[Index].Name);
		}
	}
	struct FSkillClickHandler
	{
		void (UMercenaryHireWidget::*Function)();
		const TCHAR* Name;
	};
	static const FSkillClickHandler SkillClickHandlers[6] = {
		{ &UMercenaryHireWidget::HandleSkillClicked_0, TEXT("HandleSkillClicked_0") },
		{ &UMercenaryHireWidget::HandleSkillClicked_1, TEXT("HandleSkillClicked_1") },
		{ &UMercenaryHireWidget::HandleSkillClicked_2, TEXT("HandleSkillClicked_2") },
		{ &UMercenaryHireWidget::HandleSkillClicked_3, TEXT("HandleSkillClicked_3") },
		{ &UMercenaryHireWidget::HandleSkillClicked_4, TEXT("HandleSkillClicked_4") },
		{ &UMercenaryHireWidget::HandleSkillClicked_5, TEXT("HandleSkillClicked_5") },
	};
	for (int32 Index = 0; Index < mDetailSkillButtons.Num(); ++Index)
	{
		if (UButton* Button = mDetailSkillButtons[Index])
		{
			// 짧게 한 번 터치해도 상세가 즉시 열린다. PreciseTap은 스크롤/드래그로
			// 손가락이 빠진 경우 클릭이 잘못 확정되는 것만 막는다.
			Button->SetTouchMethod(EButtonTouchMethod::PreciseTap);
			Button->SetClickMethod(EButtonClickMethod::PreciseClick);
			Button->OnClicked.__Internal_AddUniqueDynamic(this,
				SkillClickHandlers[Index].Function, SkillClickHandlers[Index].Name);
		}
	}
	struct FPartySlotHandler
	{
		void (UMercenaryHireWidget::*Function)();
		const TCHAR* Name;
	};
	static const FPartySlotHandler PartySlotHandlers[3] = {
		{ &UMercenaryHireWidget::HandlePartySlotClicked_0, TEXT("HandlePartySlotClicked_0") },
		{ &UMercenaryHireWidget::HandlePartySlotClicked_1, TEXT("HandlePartySlotClicked_1") },
		{ &UMercenaryHireWidget::HandlePartySlotClicked_2, TEXT("HandlePartySlotClicked_2") },
	};
	for (int32 Index = 0; Index < mSlots.Num(); ++Index)
	{
		if (UButton* Button = mSlots[Index].mButton)
		{
			Button->OnClicked.__Internal_AddUniqueDynamic(
				this, PartySlotHandlers[Index].Function, PartySlotHandlers[Index].Name);
		}
	}
	if (mDepartButton != nullptr)
	{
		mDepartButton->OnClicked.AddUniqueDynamic(
			this, &UMercenaryHireWidget::HandleDepartClicked);
	}
	if (mAddButton != nullptr)
	{
		mAddButton->OnClicked.AddUniqueDynamic(
			this, &UMercenaryHireWidget::HandleAddClicked);
	}
	if (mBackButton != nullptr)
	{
		mBackButton->OnClicked.AddUniqueDynamic(
			this, &UMercenaryHireWidget::HandleBackClicked);
	}
}

EMercenaryCardState UMercenaryHireWidget::StateOf(const int32 CardIndex) const
{
	if (mIsShopMode)
	{
		return mReviewing == CardIndex
			? EMercenaryCardState::Reviewing
			: EMercenaryCardState::Open;
	}
	if (mChosen.Contains(CardIndex))
	{
		return EMercenaryCardState::Chosen;
	}
	if (mReviewing == CardIndex)
	{
		return EMercenaryCardState::Reviewing;
	}
	return mChosen.Num() >= mPartySize
		? EMercenaryCardState::Full
		: EMercenaryCardState::Open;
}

/**
 * @brief 출발할 수 있는가.
 *
 * @details
 * 한 명만 있어도 출발한다. 파티 칸은 셋이지만, 용병 자료가 아직 다 안 들어와
 * 셋을 채울 수 없는 상태에서 **화면 전체가 막혀 버리기 때문**이다. 자료가
 * 갖춰지면 mMinPartySize 를 mPartySize 와 같게 올리면 원래 규칙으로 돌아온다.
 *
 * 게임모드 쪽은 이미 한 명이면 통과시킨다(AFrontendGameMode::IsAnyPlayerUnitIdValid).
 */
bool UMercenaryHireWidget::IsReadyToDepart() const
{
	return mChosen.Num() >= FMath::Min(mMinPartySize, mPartySize);
}

void UMercenaryHireWidget::ClickCard(const int32 CardIndex)
{
	// 칸 수로 막는다. 위젯을 못 찾았다고 규칙까지 멈추면 안 된다 -- WBP 없이
	// 규칙만 시험할 때 아무 일도 안 일어나서 시험이 통과해 버린다.
	if (CardIndex < 0 || CardIndex >= MercenaryHireDetail::CardCount)
	{
		return;
	}
	// 잠긴 후보는 눌러도 아무 일이 없다. 잠금 판정은 게임 모드가 끝내 놓았고
	// 화면은 그 bool 만 믿는다 -- 여기서 다시 따지면 두 곳이 어긋난다.
	if (mCrew.IsValidIndex(CardIndex) && !mCrew[CardIndex].mSelectable)
	{
		return;
	}

	// 목록은 상세 검토만 바꾼다. 파티 편성은 중앙 하단의 추가 버튼 한 곳에서
	// 명시적으로 수행해, 후보를 둘러보다가 파티가 바뀌는 일을 막는다.
	HideSkillDetailOverlay();
	mReviewing = CardIndex;
	Refresh();
}

void UMercenaryHireWidget::ClickAdd()
{
	if (mIsShopMode)
	{
		if (!mShopCandidates.IsValidIndex(mReviewing)
			|| !mShopPartySlots.IsValidIndex(mShopTargetPartyViewIndex))
		{
			return;
		}
		const FShopMercenaryUI& Candidate = mShopCandidates[mReviewing];
		if (Candidate.mIsSoldOut || !Candidate.mIsAffordable
			|| !Candidate.mCharacter.mSelectable)
		{
			return;
		}
		mOnShopHireRequested.Broadcast(Candidate.mSlotIndex,
			mShopPartySlots[mShopTargetPartyViewIndex].mUnitIndex);
		return;
	}
	ToggleChoice(mReviewing);
	Refresh();
}

void UMercenaryHireWidget::ClickPartySlot(const int32 SlotIndex)
{
	if (mIsShopMode)
	{
		if (mShopPartySlots.IsValidIndex(SlotIndex))
		{
			mShopTargetPartyViewIndex = SlotIndex;
			RefreshBottomBar();
		}
		return;
	}
	if (!mChosen.IsValidIndex(SlotIndex))
	{
		// 빈 슬롯은 자리 표시만 한다. 추가는 명시적인 추가 버튼만 맡는다.
		return;
	}

	const int32 RemovedCardIndex = mChosen[SlotIndex];
	mChosen.RemoveAt(SlotIndex);
	mReviewing = RemovedCardIndex;
	Refresh();
}

/**
 * @brief 빈 자리가 있을 때만 고른다.
 *
 * @details
 * 목록 클릭은 상세 검토만 바꾸며, 중앙의 추가 버튼만 이 함수를 호출한다.
 * 빼기는 오른쪽 파티 칸이 맡는다(ClickPartySlot).
 * @param CardIndex 추가할 후보의 목록 인덱스
 */
void UMercenaryHireWidget::ToggleChoice(const int32 CardIndex)
{
	if (!mCrew.IsValidIndex(CardIndex) || !mCrew[CardIndex].mSelectable
		|| mChosen.Contains(CardIndex) || mChosen.Num() >= mPartySize)
	{
		return;   // 상세만 갈리고 편성은 그대로.
	}
	mChosen.Add(CardIndex);
}

void UMercenaryHireWidget::Refresh()
{
	for (int32 Index = 0; Index < mCards.Num(); ++Index)
	{
		RefreshCard(Index);
	}
	if (mIsMarchboundLayout)
	{
		RefreshDetail();
	}
	RefreshBottomBar();
	ApplyTextOpticalAlignment();
}

void UMercenaryHireWidget::ApplyTextOpticalAlignment()
{
	if (!mIsMarchboundLayout)
	{
		return;
	}

	auto Apply = [](UTextBlock* Text)
	{
		RDTextOpticalAlignment::Apply(Text, MercenaryHireDetail::HireTextOpticalBiasY);
	};

	for (const FMercenaryCardWidgets& Card : mCards)
	{
		for (UTextBlock* Text : { Card.mName.Get(), Card.mRole.Get(), Card.mHP.Get(),
			Card.mBadge.Get(), Card.mTrait.Get() })
		{
			Apply(Text);
		}
		for (UTextBlock* Text : Card.mSkills)
		{
			Apply(Text);
		}
	}

	for (const FMercenarySlotWidgets& SlotWidgets : mSlots)
	{
		Apply(SlotWidgets.mName);
		Apply(SlotWidgets.mLevel);
	}

	for (UTextBlock* Text : { mPartyCountText.Get(), mAddLabel.Get(),
		mDepartLabel.Get(), mDetailName.Get(), mDetailHP.Get(),
		mDetailAP.Get(), mDetailSpeed.Get(),
		MercenaryHireDetail::Find<UTextBlock>(WidgetTree, TEXT("HireBackLabel")) })
	{
		Apply(Text);
	}
	for (UTextBlock* Text : mDetailSkills)
	{
		Apply(Text);
	}
}

void UMercenaryHireWidget::RefreshCard(const int32 CardIndex)
{
	const FMercenaryCardWidgets& Card = mCards[CardIndex];
	const bool bHasData = mCrew.IsValidIndex(CardIndex);
	MercenaryHireDetail::SetShown(Card.mRoot,
		mIsShopMode ? bHasData
			: (mIsMarchboundLayout ? true : (!mCrew.Num() || bHasData)));
	if (!bHasData)
	{
		// 데이터가 없으면 WBP 에 구워 둔 글자를 그대로 둔다. 지우면 에디터에서
		// 화면을 열었을 때 빈 종이만 남아 배치를 볼 수가 없다.
		if (mIsMarchboundLayout && !mCrew.IsEmpty())
		{
			MercenaryHireDetail::SetDimmed(Card.mRoot, true);
			MercenaryHireDetail::SetShown(Card.mSelected, false);
			MercenaryHireDetail::SetShown(Card.mSeal, false);
			if (Card.mButton != nullptr)
			{
				Card.mButton->SetIsEnabled(false);
			}
		}
		return;
	}

	const FFrontendCharacterOption& Option = mCrew[CardIndex];
	MercenaryHireDetail::SetTextIfPresent(Card.mName, Option.mDisplayName);
	// 역할 알약은 좁다. 긴 문구를 넣으면 넘친다.
	MercenaryHireDetail::SetTextIfPresent(Card.mRole,
		mIsMarchboundLayout && Option.mRoleShort.IsEmpty()
			? Option.mRoleText : Option.mRoleShort);
	MercenaryHireDetail::SetTextIfPresent(Card.mHP, FText::FromString(
		FString::Printf(TEXT("HP %d"), Option.mMaxHP)));
	// 특성 자리에는 설명 문구가 들어간다. 왜 데려가는지 한 줄로 말해 주는
	// 값이 이미 있는데 같은 뜻의 칸을 하나 더 두면 둘이 어긋난다.
	MercenaryHireDetail::SetTextIfPresent(Card.mTrait, Option.mDescription);

	for (int32 Line = 0; Line < Card.mSkills.Num(); ++Line)
	{
		const bool bHas = Option.mSkillNames.IsValidIndex(Line);
		MercenaryHireDetail::SetShown(Card.mSkills[Line], bHas);
		if (bHas)
		{
			MercenaryHireDetail::SetTextIfPresent(Card.mSkills[Line], Option.mSkillNames[Line]);
		}
	}

	if (Card.mPortrait != nullptr)
	{
		UTexture2D* Face = mIsMarchboundLayout
			? Option.mIcon.LoadSynchronous()
			: Option.mPortrait.LoadSynchronous();
		if (Face != nullptr)
		{
			Card.mPortrait->SetBrushFromTexture(Face, false);
		}
	}

	const EMercenaryCardState State = StateOf(CardIndex);
	MercenaryHireDetail::SetShown(Card.mSeal, State == EMercenaryCardState::Chosen);
	if (mIsMarchboundLayout)
	{
		// 빨간 인장은 파티 편성 상태이고, 파란 테두리는 가운데 상세가 누구를
		// 보여 주는지만 뜻한다. 둘을 같은 조건으로 켜면 파티원 전부가 파랗게
		// 남아서 현재 보고 있는 한 명을 구별할 수 없다.
		MercenaryHireDetail::SetShown(Card.mSelected, mReviewing == CardIndex);
	}
	MercenaryHireDetail::SetShown(Card.mBadge, State != EMercenaryCardState::Chosen);
	// 자리가 찼다고 흐리지 않는다. 눌리는 것을 흐리게 두면 못 누르는 줄 안다.
	MercenaryHireDetail::SetDimmed(Card.mRoot, !Option.mSelectable);
	if (mIsMarchboundLayout && Card.mButton != nullptr)
	{
		Card.mButton->SetIsEnabled(Option.mSelectable);
	}

	if (!Option.mSelectable)
	{
		// 잠긴 사유는 게임 모드가 문구까지 만들어 준다. 없으면 그냥 잠김.
		MercenaryHireDetail::SetTextIfPresent(Card.mBadge, Option.mDisabledReason.IsEmpty()
			? LOCTEXT("StateLocked", "잠김") : Option.mDisabledReason);
		return;
	}
	if (mIsShopMode && mShopCandidates.IsValidIndex(CardIndex))
	{
		const FShopMercenaryUI& Candidate = mShopCandidates[CardIndex];
		const FText StateText = Candidate.mIsSoldOut
			? LOCTEXT("ShopCandidateSold", "고용 완료")
			: FText::Format(LOCTEXT("ShopCandidatePrice", "{0} G"),
				FText::AsNumber(Candidate.mPrice));
		MercenaryHireDetail::SetTextIfPresent(Card.mBadge, StateText);
		MercenaryHireDetail::SetShown(Card.mBadge, true);
		MercenaryHireDetail::SetShown(Card.mSeal, Candidate.mIsSoldOut);
		return;
	}

	switch (State)
	{
	case EMercenaryCardState::Reviewing:
		MercenaryHireDetail::SetTextIfPresent(Card.mBadge, LOCTEXT("StateReviewing", "검토 중"));
		break;
	case EMercenaryCardState::Full:
		MercenaryHireDetail::SetTextIfPresent(Card.mBadge, LOCTEXT("StateFull", "바꾸려면 누르기"));
		break;
	default:
		MercenaryHireDetail::SetTextIfPresent(Card.mBadge, LOCTEXT("StateOpen", "모집 중"));
		break;
	}
}

void UMercenaryHireWidget::RefreshDetail()
{
	int32 DetailIndex = mReviewing;
	if (!mCrew.IsValidIndex(DetailIndex) && !mChosen.IsEmpty())
	{
		DetailIndex = mChosen.Last();
	}
	if (!mCrew.IsValidIndex(DetailIndex))
	{
		return;
	}

	const FFrontendCharacterOption& Option = mCrew[DetailIndex];
	const int32 ArtIndex = Option.mJobType == EUnitJobType::Knight ? 0
		: Option.mJobType == EUnitJobType::Mage ? 1
		: Option.mJobType == EUnitJobType::Ranger ? 2
		: Option.mJobType == EUnitJobType::Rogue ? 3
		: Option.mJobType == EUnitJobType::Barbarian ? 4
		: Option.mJobType == EUnitJobType::Druid ? 5 : DetailIndex;
	if (mHeroIllustration != nullptr)
	{
		if (UTexture2D* Hero = MercenaryHireDetail::LoadTexture(
			MercenaryHireDetail::HeroCutoutPaths, ArtIndex))
		{
			mHeroIllustration->SetBrushFromTexture(Hero, false);
		}
	}
	if (mHeroGeneratedBackground != nullptr)
	{
		if (UTexture2D* Background = MercenaryHireDetail::LoadTexture(
			MercenaryHireDetail::GeneratedBackgroundPaths, ArtIndex))
		{
			mHeroGeneratedBackground->SetBrushFromTexture(Background, false);
		}
	}
	MercenaryHireDetail::SetTextIfPresent(mDetailName, Option.mDisplayName);
	MercenaryHireDetail::SetTextIfPresent(mDetailHP, FText::FromString(
		FString::Printf(TEXT("HP %d"), Option.mMaxHP)));
	MercenaryHireDetail::SetTextIfPresent(mDetailAP, FText::FromString(
		FString::Printf(TEXT("AP %d"), Option.mMaxAP)));
	MercenaryHireDetail::SetTextIfPresent(mDetailSpeed, FText::FromString(
		FString::Printf(TEXT("SPEED %d"), Option.mSpeed)));

	static const FText CoreActions[2] = {
		LOCTEXT("BasicAttack", "평타"),
		LOCTEXT("Move", "이동")
	};
	// 실제 플레이어 유닛 DA에는 평타를 포함한 스킬 다섯 개만 있고 이동은
	// 별도 커맨드다. 전투 HUD와 똑같이 0번 이동 + 1~5번 DA 스킬로 놓는다.
	// 구형/검수 데이터의 두 고정 칸 규칙은 작은 목록에만 그대로 남긴다.
	const bool bRealKit = Option.mSkillNames.Num() >= 5;
	for (int32 Index = 0; Index < mDetailSkills.Num(); ++Index)
	{
		// 그림이 있으면 그림으로, 없으면 이름 글자로 -- 전투 요약 칸과 같은 규칙.
		UTexture2D* IconTexture = nullptr;
		if (bRealKit)
		{
			const int32 SkillIndex = Index - 1;
			MercenaryHireDetail::SetTextIfPresent(mDetailSkills[Index], Index == 0
				? CoreActions[1]
				: (Option.mSkillNames.IsValidIndex(SkillIndex)
					? Option.mSkillNames[SkillIndex] : FText::GetEmpty()));
			if (Index > 0 && Option.mSkillIcons.IsValidIndex(SkillIndex))
			{
				IconTexture = Option.mSkillIcons[SkillIndex].LoadSynchronous();
			}
		}
		else if (Index < 2)
		{
			MercenaryHireDetail::SetTextIfPresent(mDetailSkills[Index], CoreActions[Index]);
		}
		else
		{
			const int32 SkillIndex = Index - 2;
			MercenaryHireDetail::SetTextIfPresent(mDetailSkills[Index],
				Option.mSkillNames.IsValidIndex(SkillIndex)
					? Option.mSkillNames[SkillIndex]
					: FText::Format(LOCTEXT("SkillSlotFallback", "스킬 {0}"),
					SkillIndex + 1));
			if (Option.mSkillIcons.IsValidIndex(SkillIndex))
			{
				IconTexture = Option.mSkillIcons[SkillIndex].LoadSynchronous();
			}
		}

		UImage* IconImage = mDetailSkillIcons.IsValidIndex(Index)
			? mDetailSkillIcons[Index].Get() : nullptr;
		if (IconImage != nullptr && IconTexture != nullptr)
		{
			IconImage->SetBrushFromTexture(IconTexture, false);
			// 판이 빈 칸 흰 사각을 막느라 NoDraw 로 두었을 수 있다. 그림을
			// 넣을 때 Image 로 되돌리지 않으면 영영 안 그려진다(전투 카드 실측).
			FSlateBrush IconBrush = IconImage->GetBrush();
			IconBrush.DrawAs = ESlateBrushDrawType::Image;
			IconImage->SetBrush(IconBrush);
		}
		MercenaryHireDetail::SetShown(IconImage, IconTexture != nullptr);
		MercenaryHireDetail::SetShown(mDetailSkills[Index], IconTexture == nullptr);
	}
}

int32 UMercenaryHireWidget::GetSkillDataIndexForSlot(
	const FFrontendCharacterOption& Option, const int32 SlotIndex) const
{
	if (SlotIndex < 0 || SlotIndex >= 6)
	{
		return INDEX_NONE;
	}
	// 실제 전투 kit는 0번 이동 + DA 스킬 다섯 개다. 이동에는 대응하는
	// FFrontendSkillOption이 없고, 1~5번만 0~4번 상세에 정확히 대응한다.
	// 구형/검수 데이터는 앞의 평타·이동 두 칸을 가상으로 유지한다.
	const bool bRealKit = Option.mSkillNames.Num() >= 5;
	const int32 DataIndex = bRealKit ? SlotIndex - 1 : SlotIndex - 2;
	return Option.mSkillDetails.IsValidIndex(DataIndex) ? DataIndex : INDEX_NONE;
}

void UMercenaryHireWidget::HandleSkillClicked(const int32 SlotIndex)
{
	if (!mCrew.IsValidIndex(mReviewing))
	{
		return;
	}
	const FFrontendCharacterOption& Option = mCrew[mReviewing];
	const int32 DataIndex = GetSkillDataIndexForSlot(Option, SlotIndex);
	if (!Option.mSkillDetails.IsValidIndex(DataIndex))
	{
		return;
	}
	ShowSkillDetailOverlay(Option.mSkillDetails[DataIndex]);
}

bool UMercenaryHireWidget::EnsureSkillDetailOverlay()
{
	if (mSkillDetailOverlayClass == nullptr)
	{
		return false;
	}
	if (mSkillDetailPresenter == nullptr)
	{
		/*
		 * 전투 HUD와 같은 리치 상세 프레젠터를 쓴다. 예전에는 이 화면이
		 * 같은 WBP를 제 손으로 채우는 두 번째 구현을 들고 있었다 -- 채우는
		 * 쪽이 갈라지면 표기도 갈라진다. Z=60은 기존 겹과 같은 층이다.
		 */
		mSkillDetailPresenter = NewObject<USkillDetailOverlayPresenter>(this);
		mSkillDetailPresenter->Initialize(GetWorld(), mSkillDetailOverlayClass,
			mSkillTacticalDiagramClass, /*ViewportZOrder=*/60);
	}
	if (mSkillDetailPresenter->EnsureOverlayWidget(GetOwningPlayer()) == false)
	{
		return false;
	}
	/* 전면 받이는 입력만 삼키고 상세를 유지한다. 닫기는 닫기 단추만 한다. */
	if (mSkillDetailOverlay == nullptr)
	{
		mSkillDetailOverlay = mSkillDetailPresenter->GetOverlayWidget();
	}
	return mSkillDetailOverlay != nullptr;
}

void UMercenaryHireWidget::ShowSkillDetailOverlay(const FFrontendSkillOption& Skill)
{
	if (!EnsureSkillDetailOverlay())
	{
		return;
	}
	FSkillDetailUI Detail;
	SkillDetailUIBuilder::FillFromFrontendOption(Skill, OUT Detail);
	mSkillDetailPresenter->Present(Detail);
}

void UMercenaryHireWidget::HideSkillDetailOverlay()
{
	if (mSkillDetailPresenter != nullptr)
	{
		mSkillDetailPresenter->Dismiss();
	}
	else if (mSkillDetailOverlay != nullptr)
	{
		mSkillDetailOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UMercenaryHireWidget::HandleSkillDetailCloseClicked()
{
	HideSkillDetailOverlay();
}

void UMercenaryHireWidget::HandleSkillClicked_0() { HandleSkillClicked(0); }
void UMercenaryHireWidget::HandleSkillClicked_1() { HandleSkillClicked(1); }
void UMercenaryHireWidget::HandleSkillClicked_2() { HandleSkillClicked(2); }
void UMercenaryHireWidget::HandleSkillClicked_3() { HandleSkillClicked(3); }
void UMercenaryHireWidget::HandleSkillClicked_4() { HandleSkillClicked(4); }
void UMercenaryHireWidget::HandleSkillClicked_5() { HandleSkillClicked(5); }

#if WITH_DEV_AUTOMATION_TESTS

int32 UMercenaryHireWidget::GetSkillDataIndexForSlotForTest(
	const int32 SlotIndex) const
{
	return mCrew.IsValidIndex(mReviewing)
		? GetSkillDataIndexForSlot(mCrew[mReviewing], SlotIndex) : INDEX_NONE;
}
#endif

void UMercenaryHireWidget::RefreshBottomBar()
{
	if (mIsShopMode)
	{
		MercenaryHireDetail::SetTextIfPresent(mPartyCountText,
			LOCTEXT("ShopReplaceTarget", "교체 대상"));
		for (int32 SlotIndex = 0; SlotIndex < mSlots.Num(); ++SlotIndex)
		{
			const bool bHasSlot = mShopPartySlots.IsValidIndex(SlotIndex);
			const bool bFilled = bHasSlot && mShopPartySlots[SlotIndex].mIsOccupied;
			const FMercenarySlotWidgets& Widgets = mSlots[SlotIndex];
			MercenaryHireDetail::SetShown(Widgets.mPlus, !bFilled);
			MercenaryHireDetail::SetShown(Widgets.mFace, bFilled);
			MercenaryHireDetail::SetShown(Widgets.mName, bFilled);
			MercenaryHireDetail::SetShown(Widgets.mLevel, bFilled);
			MercenaryHireDetail::SetPartySlotNameBand(Widgets, bFilled);
			if (Widgets.mButton != nullptr)
			{
				Widgets.mButton->SetIsEnabled(bHasSlot);
			}
			MercenaryHireDetail::SetDimmed(Widgets.mRoot,
				bHasSlot && SlotIndex != mShopTargetPartyViewIndex);
			if (!bHasSlot)
			{
				continue;
			}
			if (!bFilled)
			{
				MercenaryHireDetail::SetTextIfPresent(Widgets.mName,
					LOCTEXT("ShopPartyEmpty", "빈 자리"));
				continue;
			}

			const FShopMercenaryPartySlotUI& Unit = mShopPartySlots[SlotIndex];
			MercenaryHireDetail::SetTextIfPresent(Widgets.mName,
				MercenaryHireDetail::JobDisplayName(Unit.mJobType));
			MercenaryHireDetail::SetTextIfPresent(Widgets.mLevel,
				FText::Format(LOCTEXT("ShopPartyLevel", "Lv.{0}"),
					FText::AsNumber(Unit.mLevel)));
			if (Widgets.mFace != nullptr)
			{
				if (UTexture2D* Face = MercenaryHireDetail::LoadTexture(
					MercenaryHireDetail::IconPaths,
					MercenaryHireDetail::ArtIndexForJob(Unit.mJobType)))
				{
					Widgets.mFace->SetBrushFromTexture(Face, false);
				}
			}
		}

		const FShopMercenaryUI* Candidate =
			mShopCandidates.IsValidIndex(mReviewing)
				? &mShopCandidates[mReviewing] : nullptr;
		const bool bCanHire = Candidate != nullptr
			&& mShopPartySlots.IsValidIndex(mShopTargetPartyViewIndex)
			&& Candidate->mCharacter.mSelectable
			&& Candidate->mIsAffordable && !Candidate->mIsSoldOut;
		if (mAddButton != nullptr)
		{
			mAddButton->SetIsEnabled(bCanHire);
		}
		MercenaryHireDetail::SetDimmed(mAddLabel, !bCanHire);
		MercenaryHireDetail::SetTextIfPresent(mAddLabel,
			Candidate != nullptr ? LOCTEXT("ShopHireCTA", "고용")
				: LOCTEXT("ShopHireUnavailable", "고용 불가"));
		if (mDepartButton != nullptr)
		{
			mDepartButton->SetIsEnabled(false);
		}
		MercenaryHireDetail::SetDimmed(mDepartLabel, false);
		MercenaryHireDetail::SetTextIfPresent(mDepartLabel, Candidate != nullptr
			? FText::Format(LOCTEXT("ShopHireCost", "{0}G"),
				FText::AsNumber(Candidate->mPrice))
			: FText::GetEmpty());
		if (mDepartLabel != nullptr)
		{
			if (mDepartLabelBaseFont.IsSet() == false)
			{
				mDepartLabelBaseFont = mDepartLabel->GetFont();
			}
			FSlateFontInfo CostFont = mDepartLabelBaseFont.GetValue();
			CostFont.Size = 20;
			mDepartLabel->SetFont(CostFont);
		}
		/* 규칙 전용 테스트는 WBP 없이 돌아 WidgetTree가 없다. */
		MercenaryHireDetail::SetShown(WidgetTree != nullptr
			? WidgetTree->FindWidget(TEXT("HireBackHolder")) : nullptr, false);
		return;
	}
	MercenaryHireDetail::SetShown(WidgetTree != nullptr
		? WidgetTree->FindWidget(TEXT("HireBackHolder")) : nullptr, true);
	if (mDepartLabel != nullptr && mDepartLabelBaseFont.IsSet())
	{
		/*
		 * 상점 고용비용 표기를 거쳤을 때만 복원한다. 캐릭터 선택 화면에서는
		 * WBP가 구운 크기가 그대로라, 임의 값(32)으로 덮으면 이웃한
		 * 추가/뒤로 라벨과 크기가 어긋났다.
		 */
		mDepartLabel->SetFont(mDepartLabelBaseFont.GetValue());
	}

	const FText PartyCount = FText::Format(mIsMarchboundLayout
		? LOCTEXT("PartyCountInline", "파티 {0}/{1}")
		: LOCTEXT("PartyCountStacked", "파티\n{0}/{1}"),
		mChosen.Num(), mPartySize);
	MercenaryHireDetail::SetTextIfPresent(mPartyCountText, PartyCount);

	for (int32 SlotIndex = 0; SlotIndex < mSlots.Num(); ++SlotIndex)
	{
		const bool bFilled = mChosen.IsValidIndex(SlotIndex)
			&& mCrew.IsValidIndex(mChosen[SlotIndex]);
		const FMercenarySlotWidgets& Widgets = mSlots[SlotIndex];
		MercenaryHireDetail::SetShown(Widgets.mLevel, false);
		MercenaryHireDetail::SetPartySlotNameBand(Widgets, false);
		if (mIsMarchboundLayout)
		{
			MercenaryHireDetail::SetShown(Widgets.mPlus, !bFilled);
		}
		MercenaryHireDetail::SetShown(Widgets.mFace, bFilled);
		if (mIsMarchboundLayout)
		{
			MercenaryHireDetail::SetShown(Widgets.mName, bFilled);
		}
		if (!bFilled)
		{
			MercenaryHireDetail::SetTextIfPresent(Widgets.mName,
				mIsMarchboundLayout ? FText::GetEmpty() : LOCTEXT("SlotEmpty", "빈 자리"));
			continue;
		}
		const FFrontendCharacterOption& Option = mCrew[mChosen[SlotIndex]];
		MercenaryHireDetail::SetTextIfPresent(Widgets.mName, Option.mDisplayName);
		if (Widgets.mFace != nullptr)
		{
			UTexture2D* Face = mIsMarchboundLayout
				? MercenaryHireDetail::LoadTexture(MercenaryHireDetail::IconPaths,
					mChosen[SlotIndex])
				: Option.mPortrait.LoadSynchronous();
			if (Face != nullptr)
			{
				Widgets.mFace->SetBrushFromTexture(Face, false);
			}
		}
	}

	const bool bReady = IsReadyToDepart();
	const bool bCanAdd = mCrew.IsValidIndex(mReviewing)
		&& mCrew[mReviewing].mSelectable
		&& !mChosen.Contains(mReviewing)
		&& mChosen.Num() < mPartySize;
	if (mAddButton != nullptr)
	{
		mAddButton->SetIsEnabled(bCanAdd);
	}
	MercenaryHireDetail::SetDimmed(mAddLabel, !bCanAdd);
	if (mDepartButton != nullptr)
	{
		mDepartButton->SetIsEnabled(bReady);
	}
	MercenaryHireDetail::SetDimmed(mDepartLabel, !bReady);
}

void UMercenaryHireWidget::HandleBackClicked()
{
	HideSkillDetailOverlay();
	if (mOnBackRequested.IsBound())
	{
		mOnBackRequested.Broadcast();
		return;
	}
	CloseUI();
}

void UMercenaryHireWidget::HandleAddClicked()
{
	ClickAdd();
}

void UMercenaryHireWidget::HandleDepartClicked()
{
	ConfirmParty();
}

void UMercenaryHireWidget::ConfirmParty()
{
	if (!IsReadyToDepart())
	{
		return;
	}
	/*
	 * 받는 쪽(URunPersistData::StartRun)은 **파티 칸 수만큼** 오기를 요구한다
	 * -- 길이가 다르면 "파티 멤버 부족" 으로 죽는다. 빈 칸은 무효 id 로
	 * 표현하고, 저쪽 반복문이 그것을 건너뛴다.
	 *
	 * 고른 것만 보내다가 한 명 출발에서 그대로 터졌다.
	 */
	TArray<FPrimaryAssetId> Party;
	Party.Init(FPrimaryAssetId(), mPartySize);
	for (int32 SlotIndex = 0; SlotIndex < mPartySize; ++SlotIndex)
	{
		if (mChosen.IsValidIndex(SlotIndex) == false)
		{
			continue;
		}
		const int32 Index = mChosen[SlotIndex];
		if (mCrew.IsValidIndex(Index))
		{
			Party[SlotIndex] = mCrew[Index].mPlayerUnitId;
		}
	}
	// 파티 저장과 화면 전환은 여기서 하지 않는다. 화면이 흐름까지 쥐면
	// 다른 곳에서 이 화면을 못 쓴다 -- 받는 쪽이 정한다.
	mOnPartyConfirmed.Broadcast(Party);
}

void UMercenaryHireWidget::HandleCardClicked_0() { ClickCard(0); }
void UMercenaryHireWidget::HandleCardClicked_1() { ClickCard(1); }
void UMercenaryHireWidget::HandleCardClicked_2() { ClickCard(2); }
void UMercenaryHireWidget::HandleCardClicked_3() { ClickCard(3); }
void UMercenaryHireWidget::HandleCardClicked_4() { ClickCard(4); }
void UMercenaryHireWidget::HandleCardClicked_5() { ClickCard(5); }
void UMercenaryHireWidget::HandlePartySlotClicked_0() { ClickPartySlot(0); }
void UMercenaryHireWidget::HandlePartySlotClicked_1() { ClickPartySlot(1); }
void UMercenaryHireWidget::HandlePartySlotClicked_2() { ClickPartySlot(2); }

#undef LOCTEXT_NAMESPACE
