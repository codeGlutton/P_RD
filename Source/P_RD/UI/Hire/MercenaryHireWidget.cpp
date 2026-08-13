#include "UI/Hire/MercenaryHireWidget.h"

#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Texture2D.h"

#define LOCTEXT_NAMESPACE "MercenaryHire"

// 이름을 붙인 이름공간에 둔다. 익명으로 두면 유니티 빌드가 여러 파일을 한
// 덩어리로 묶을 때 전투 위젯의 같은 이름과 부딪힌다 -- 파일이 하나 늘어
// 묶이는 조합이 바뀌는 날 갑자기 깨진다.
namespace MercenaryHireDetail
{
	/** @brief 화면에 걸리는 이력서 칸 수. WBP 가 여섯 칸으로 구워져 있다. */
	constexpr int32 CardCount = 6;

	const TCHAR* IconPaths[CardCount] = {
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Knight.T_MB_HireIcon_Knight"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Mage.T_MB_HireIcon_Mage"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Ranger.T_MB_HireIcon_Ranger"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Rogue.T_MB_HireIcon_Rogue"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Barbarian.T_MB_HireIcon_Barbarian"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Druid.T_MB_HireIcon_Druid")
	};

	const TCHAR* HeroPaths[CardCount] = {
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireHero_Knight.T_MB_HireHero_Knight"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireHero_Mage.T_MB_HireHero_Mage"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireHero_Ranger.T_MB_HireHero_Ranger"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireHero_Rogue.T_MB_HireHero_Rogue"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireHero_Barbarian.T_MB_HireHero_Barbarian"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireHero_Druid.T_MB_HireHero_Druid")
	};

	UTexture2D* LoadTexture(const TCHAR* const* Paths, const int32 Index)
	{
		return Index >= 0 && Index < CardCount
			? LoadObject<UTexture2D>(nullptr, Paths[Index]) : nullptr;
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
	mCrew = Options;
	ApplyMarchboundPortraits();
	mPartySize = FMath::Max(1, PartySize);
	mChosen.Reset();
	mReviewing = mIsMarchboundLayout && !mCrew.IsEmpty() ? 0 : INDEX_NONE;
	Refresh();
}

void UMercenaryHireWidget::ApplyMarchboundPortraits()
{
	if (!mIsMarchboundLayout)
	{
		return;
	}

	// 후보 수·스탯·선택 가능 여부는 FrontendGameMode가 내려준 실제 데이터가
	// 유일한 출처다. 이 화면은 직업에 맞는 Marchbound 전용 그림만 바꾼다.
	const auto TableIndexForJob = [](const EUnitJobType JobType) -> int32
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
	};
	for (FFrontendCharacterOption& Option : mCrew)
	{
		const int32 ArtIndex = TableIndexForJob(Option.mJobType);
		if (ArtIndex == INDEX_NONE)
		{
			continue;
		}

		Option.mPortrait = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(
			MercenaryHireDetail::HeroPaths[ArtIndex]));
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
	if (mHasAppliedResponsiveLayout && bPortrait == mIsPortraitLayout)
	{
		return;
	}
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
}

void UMercenaryHireWidget::CacheWidgets()
{
	// 이름으로 찾는다. WBP 를 파이썬이 구우므로 BindWidget 으로 묶으면 이름이
	// 하나 어긋날 때 컴파일이 깨지고, 그러면 다시 굽는 것조차 못 한다.
	mIsMarchboundLayout = WidgetTree != nullptr
		&& WidgetTree->FindWidget(TEXT("HireListFrameArt")) != nullptr;
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
		mSlots[Index].mPlus = MercenaryHireDetail::Find<UWidget>(WidgetTree,
			TEXT("PartySlotPlus") + Tail);
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
	mHeroIllustration = MercenaryHireDetail::Find<UImage>(WidgetTree, TEXT("Backdrop_Art"));
	mDetailSkills.Reset();
	mDetailSkillIcons.Reset();
	for (int32 Index = 0; Index < 6; ++Index)
	{
		mDetailSkills.Add(MercenaryHireDetail::Find<UTextBlock>(WidgetTree,
			FString::Printf(TEXT("HireDetailSkillText_%d"), Index)));
		mDetailSkillIcons.Add(MercenaryHireDetail::Find<UImage>(WidgetTree,
			FString::Printf(TEXT("HireDetailSkillIcon_%d"), Index)));
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
	mReviewing = CardIndex;
	Refresh();
}

void UMercenaryHireWidget::ClickAdd()
{
	ToggleChoice(mReviewing);
	Refresh();
}

void UMercenaryHireWidget::ClickPartySlot(const int32 SlotIndex)
{
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
}

void UMercenaryHireWidget::RefreshCard(const int32 CardIndex)
{
	const FMercenaryCardWidgets& Card = mCards[CardIndex];
	const bool bHasData = mCrew.IsValidIndex(CardIndex);
	MercenaryHireDetail::SetShown(Card.mRoot,
		mIsMarchboundLayout ? true : (!mCrew.Num() || bHasData));
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
			? MercenaryHireDetail::LoadTexture(MercenaryHireDetail::IconPaths, CardIndex)
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
	if (mHeroIllustration != nullptr)
	{
		if (UTexture2D* Hero = MercenaryHireDetail::LoadTexture(
			MercenaryHireDetail::HeroPaths, DetailIndex))
		{
			mHeroIllustration->SetBrushFromTexture(Hero, false);
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
	// 유닛 DA 목록이 다섯 이상이면(베기·이동·강타… 이동까지 스킬로 들어
	// 있다) 여섯 칸에 **그 목록을 그대로** 건다 -- 전투 카드와 같은 출처,
	// 같은 순서, 같은 그림이다. 평타/이동 고정 칸은 DA 를 못 찾은 검수
	// 카드에서만 쓴다.
	const bool bRealKit = Option.mSkillNames.Num() >= 5;
	for (int32 Index = 0; Index < mDetailSkills.Num(); ++Index)
	{
		// 그림이 있으면 그림으로, 없으면 이름 글자로 -- 전투 요약 칸과 같은 규칙.
		UTexture2D* IconTexture = nullptr;
		if (bRealKit)
		{
			MercenaryHireDetail::SetTextIfPresent(mDetailSkills[Index],
				Option.mSkillNames.IsValidIndex(Index)
					? Option.mSkillNames[Index] : FText::GetEmpty());
			if (Option.mSkillIcons.IsValidIndex(Index))
			{
				IconTexture = Option.mSkillIcons[Index].LoadSynchronous();
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
					: FText::FromString(FString::Printf(TEXT("스킬 %d"), SkillIndex + 1)));
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

void UMercenaryHireWidget::RefreshBottomBar()
{
	const FString PartyCount = mIsMarchboundLayout
		? FString::Printf(TEXT("파티 %d/%d"), mChosen.Num(), mPartySize)
		: FString::Printf(TEXT("파티\n%d/%d"), mChosen.Num(), mPartySize);
	MercenaryHireDetail::SetTextIfPresent(mPartyCountText, FText::FromString(PartyCount));

	for (int32 SlotIndex = 0; SlotIndex < mSlots.Num(); ++SlotIndex)
	{
		const bool bFilled = mChosen.IsValidIndex(SlotIndex)
			&& mCrew.IsValidIndex(mChosen[SlotIndex]);
		const FMercenarySlotWidgets& Widgets = mSlots[SlotIndex];
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
