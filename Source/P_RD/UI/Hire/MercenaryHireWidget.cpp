#include "UI/Hire/MercenaryHireWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Texture2D.h"

#define LOCTEXT_NAMESPACE "MercenaryHire"

namespace
{
	/** @brief 화면에 걸리는 이력서 칸 수. WBP 가 여섯 칸으로 구워져 있다. */
	constexpr int32 CardCount = 6;

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
	Refresh();
}

void UMercenaryHireWidget::SetCharacterOptions(
	const TArray<FFrontendCharacterOption>& Options, const int32 PartySize)
{
	mCrew = Options;
	mPartySize = FMath::Max(1, PartySize);
	mChosen.Reset();
	mReviewing = INDEX_NONE;
	Refresh();
}

void UMercenaryHireWidget::CacheWidgets()
{
	// 이름으로 찾는다. WBP 를 파이썬이 구우므로 BindWidget 으로 묶으면 이름이
	// 하나 어긋날 때 컴파일이 깨지고, 그러면 다시 굽는 것조차 못 한다.
	mCards.Reset();
	mCards.SetNum(CardCount);
	for (int32 Index = 0; Index < CardCount; ++Index)
	{
		const FString Tail = FString::Printf(TEXT("_%d"), Index);
		FMercenaryCardWidgets& Card = mCards[Index];
		Card.mRoot = Find<UWidget>(WidgetTree, TEXT("HireCard") + Tail);
		Card.mButton = Find<UButton>(WidgetTree, TEXT("HireButton") + Tail);
		Card.mPortrait = Find<UImage>(WidgetTree, TEXT("HirePortrait") + Tail);
		Card.mName = Find<UTextBlock>(WidgetTree, TEXT("HireName") + Tail);
		Card.mRole = Find<UTextBlock>(WidgetTree, TEXT("HireRole") + Tail);
		Card.mHP = Find<UTextBlock>(WidgetTree, TEXT("HireHP") + Tail);
		Card.mBadge = Find<UTextBlock>(WidgetTree, TEXT("HireBadge") + Tail);
		Card.mSeal = Find<UWidget>(WidgetTree, TEXT("HireSeal") + Tail);
		Card.mTrait = Find<UTextBlock>(WidgetTree, TEXT("HireTrait") + Tail);
		Card.mSelected = Find<UWidget>(WidgetTree, TEXT("HireSelected") + Tail);

		Card.mSkills.Reset();
		for (int32 Line = 0; Line < 2; ++Line)
		{
			Card.mSkills.Add(Find<UTextBlock>(WidgetTree,
				FString::Printf(TEXT("HireSkill_%d_%d"), Index, Line)));
		}
	}

	mSlots.Reset();
	mSlots.SetNum(3);
	for (int32 Index = 0; Index < mSlots.Num(); ++Index)
	{
		const FString Tail = FString::Printf(TEXT("_%d"), Index);
		mSlots[Index].mRoot = Find<UWidget>(WidgetTree, TEXT("PartySlot") + Tail);
		mSlots[Index].mFace = Find<UImage>(WidgetTree,
			TEXT("PartySlotFace") + Tail);
		mSlots[Index].mName = Find<UTextBlock>(WidgetTree,
			TEXT("PartySlotName") + Tail);
	}

	mPartyCountText = Find<UTextBlock>(WidgetTree, TEXT("PartyCountText"));

	mNoticeText = Find<UTextBlock>(WidgetTree, TEXT("NoticeText"));
	mDepartButton = Find<UButton>(WidgetTree, TEXT("DepartButton"));
	mDepartLabel = Find<UTextBlock>(WidgetTree, TEXT("DepartLabel"));

	// 슬롯별 핸들러를 따로 두는 이유: 동적 델리게이트는 페이로드를 못 받고
	// 함수 포인터가 아니라 이름으로 묶여서, 하나로 넘기면 조용히 실패한다.
	struct FCardHandler
	{
		void (UMercenaryHireWidget::*Function)();
		const TCHAR* Name;
	};
	static const FCardHandler Handlers[CardCount] = {
		{ &UMercenaryHireWidget::HandleCardClicked_0, TEXT("HandleCardClicked_0") },
		{ &UMercenaryHireWidget::HandleCardClicked_1, TEXT("HandleCardClicked_1") },
		{ &UMercenaryHireWidget::HandleCardClicked_2, TEXT("HandleCardClicked_2") },
		{ &UMercenaryHireWidget::HandleCardClicked_3, TEXT("HandleCardClicked_3") },
		{ &UMercenaryHireWidget::HandleCardClicked_4, TEXT("HandleCardClicked_4") },
		{ &UMercenaryHireWidget::HandleCardClicked_5, TEXT("HandleCardClicked_5") },
	};
	for (int32 Index = 0; Index < CardCount; ++Index)
	{
		if (UButton* Button = mCards[Index].mButton)
		{
			Button->OnClicked.__Internal_AddUniqueDynamic(
				this, Handlers[Index].Function, Handlers[Index].Name);
		}
	}
	if (mDepartButton != nullptr)
	{
		mDepartButton->OnClicked.AddUniqueDynamic(
			this, &UMercenaryHireWidget::HandleDepartClicked);
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

bool UMercenaryHireWidget::IsReadyToDepart() const
{
	return mChosen.Num() == mPartySize;
}

void UMercenaryHireWidget::ClickCard(const int32 CardIndex)
{
	// 칸 수로 막는다. 위젯을 못 찾았다고 규칙까지 멈추면 안 된다 -- WBP 없이
	// 규칙만 시험할 때 아무 일도 안 일어나서 시험이 통과해 버린다.
	if (CardIndex < 0 || CardIndex >= CardCount)
	{
		return;
	}
	// 잠긴 후보는 눌러도 아무 일이 없다. 잠금 판정은 게임 모드가 끝내 놓았고
	// 화면은 그 bool 만 믿는다 -- 여기서 다시 따지면 두 곳이 어긋난다.
	if (mCrew.IsValidIndex(CardIndex) && !mCrew[CardIndex].mSelectable)
	{
		return;
	}

	// 두 단계로 나눈다. 한 번에 정해지면 잘못 누른 것을 되돌리기 번거롭다.
	if (mReviewing == CardIndex || mChosen.Contains(CardIndex))
	{
		ToggleChoice(CardIndex);
	}
	else
	{
		mReviewing = CardIndex;
	}
	Refresh();
}

void UMercenaryHireWidget::ToggleChoice(const int32 CardIndex)
{
	if (mChosen.Remove(CardIndex) > 0)
	{
		mReviewing = CardIndex;
		return;
	}
	if (mChosen.Num() >= mPartySize)
	{
		return;
	}
	mChosen.Add(CardIndex);
	mReviewing = INDEX_NONE;
}

void UMercenaryHireWidget::Refresh()
{
	for (int32 Index = 0; Index < mCards.Num(); ++Index)
	{
		RefreshCard(Index);
	}
	RefreshBottomBar();
}

void UMercenaryHireWidget::RefreshCard(const int32 CardIndex)
{
	const FMercenaryCardWidgets& Card = mCards[CardIndex];
	const bool bHasData = mCrew.IsValidIndex(CardIndex);
	SetShown(Card.mRoot, !mCrew.Num() || bHasData);
	if (!bHasData)
	{
		// 데이터가 없으면 WBP 에 구워 둔 글자를 그대로 둔다. 지우면 에디터에서
		// 화면을 열었을 때 빈 종이만 남아 배치를 볼 수가 없다.
		return;
	}

	const FFrontendCharacterOption& Option = mCrew[CardIndex];
	SetTextIfPresent(Card.mName, Option.mDisplayName);
	// 역할 알약은 좁다. 긴 문구를 넣으면 넘친다.
	SetTextIfPresent(Card.mRole, Option.mRoleShort);
	SetTextIfPresent(Card.mHP, FText::FromString(
		FString::Printf(TEXT("HP %d"), Option.mMaxHP)));
	// 특성 자리에는 설명 문구가 들어간다. 왜 데려가는지 한 줄로 말해 주는
	// 값이 이미 있는데 같은 뜻의 칸을 하나 더 두면 둘이 어긋난다.
	SetTextIfPresent(Card.mTrait, Option.mDescription);

	for (int32 Line = 0; Line < Card.mSkills.Num(); ++Line)
	{
		const bool bHas = Option.mSkillNames.IsValidIndex(Line);
		SetShown(Card.mSkills[Line], bHas);
		if (bHas)
		{
			SetTextIfPresent(Card.mSkills[Line], Option.mSkillNames[Line]);
		}
	}

	if (Card.mPortrait != nullptr)
	{
		if (UTexture2D* Face = Option.mPortrait.LoadSynchronous())
		{
			Card.mPortrait->SetBrushFromTexture(Face, false);
		}
	}

	const EMercenaryCardState State = StateOf(CardIndex);
	SetShown(Card.mSelected, State == EMercenaryCardState::Reviewing);
	SetShown(Card.mSeal, State == EMercenaryCardState::Chosen);
	SetShown(Card.mBadge, State != EMercenaryCardState::Chosen);
	SetDimmed(Card.mRoot,
		State == EMercenaryCardState::Full || !Option.mSelectable);

	if (!Option.mSelectable)
	{
		// 잠긴 사유는 게임 모드가 문구까지 만들어 준다. 없으면 그냥 잠김.
		SetTextIfPresent(Card.mBadge, Option.mDisabledReason.IsEmpty()
			? LOCTEXT("StateLocked", "잠김") : Option.mDisabledReason);
		return;
	}

	switch (State)
	{
	case EMercenaryCardState::Reviewing:
		SetTextIfPresent(Card.mBadge, LOCTEXT("StateReviewing", "검토 중"));
		break;
	case EMercenaryCardState::Full:
		SetTextIfPresent(Card.mBadge, LOCTEXT("StateFull", "자리 참"));
		break;
	default:
		SetTextIfPresent(Card.mBadge, LOCTEXT("StateOpen", "모집 중"));
		break;
	}
}

void UMercenaryHireWidget::RefreshBottomBar()
{
	SetTextIfPresent(mPartyCountText, FText::FromString(FString::Printf(
		TEXT("파티\n%d/%d"), mChosen.Num(), mPartySize)));

	for (int32 SlotIndex = 0; SlotIndex < mSlots.Num(); ++SlotIndex)
	{
		const bool bFilled = mChosen.IsValidIndex(SlotIndex)
			&& mCrew.IsValidIndex(mChosen[SlotIndex]);
		const FMercenarySlotWidgets& Widgets = mSlots[SlotIndex];
		SetShown(Widgets.mFace, bFilled);
		if (!bFilled)
		{
			SetTextIfPresent(Widgets.mName, LOCTEXT("SlotEmpty", "빈 자리"));
			continue;
		}
		const FFrontendCharacterOption& Option = mCrew[mChosen[SlotIndex]];
		SetTextIfPresent(Widgets.mName, Option.mDisplayName);
		if (Widgets.mFace != nullptr)
		{
			if (UTexture2D* Face = Option.mPortrait.LoadSynchronous())
			{
				Widgets.mFace->SetBrushFromTexture(Face, false);
			}
		}
	}

	const bool bReady = IsReadyToDepart();
	if (mDepartButton != nullptr)
	{
		mDepartButton->SetIsEnabled(bReady);
	}
	SetDimmed(mDepartLabel, !bReady);

	if (bReady)
	{
		SetTextIfPresent(mNoticeText, LOCTEXT("NoticeGo", "출발 준비가 됐습니다."));
	}
	else if (mReviewing != INDEX_NONE)
	{
		SetTextIfPresent(mNoticeText,
			LOCTEXT("NoticeConfirm", "한 번 더 누르면 정해집니다."));
	}
	else
	{
		SetTextIfPresent(mNoticeText, FText::FromString(FString::Printf(
			TEXT("용병 %d명을 더 고르세요."),
			FMath::Max(0, mPartySize - mChosen.Num()))));
	}
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
	TArray<FPrimaryAssetId> Party;
	for (const int32 Index : mChosen)
	{
		if (mCrew.IsValidIndex(Index))
		{
			Party.Add(mCrew[Index].mPlayerUnitId);
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

#undef LOCTEXT_NAMESPACE
