#include "UI/Combat/CombatLayoutHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/Combat/MockCombatDriver.h"
#include "GameMode/CombatGameMode.h"

#define LOCTEXT_NAMESPACE "CombatLayoutHUD"

namespace
{
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
	mCached = true;

	mRoundText = Find<UTextBlock>(WidgetTree, TEXT("RoundText"));
	mObjectiveText = Find<UTextBlock>(WidgetTree, TEXT("ObjectiveText"));

	mPartySlots.SetNum(PartySlotCount);
	for (int32 Index = 0; Index < PartySlotCount; ++Index)
	{
		FPartySlotWidgets& Widgets = mPartySlots[Index];
		const FString Suffix = FString::Printf(TEXT("_%d"), Index);
		Widgets.Root = Find<UWidget>(WidgetTree, TEXT("PartyCard") + Suffix);
		Widgets.Content = Find<UWidget>(WidgetTree, TEXT("PartyContent") + Suffix);
		Widgets.Selected = Find<UWidget>(WidgetTree, TEXT("PartySelected") + Suffix);
		Widgets.Portrait = Find<UImage>(WidgetTree, TEXT("PartyPortrait") + Suffix);
		Widgets.Name = Find<UTextBlock>(WidgetTree, TEXT("PartyName") + Suffix);
		Widgets.HPBar = Find<UProgressBar>(WidgetTree, TEXT("PartyHPBar") + Suffix);
		Widgets.HPText = Find<UTextBlock>(WidgetTree, TEXT("PartyHPText") + Suffix);
		Widgets.APText = Find<UTextBlock>(WidgetTree, TEXT("PartyAPText") + Suffix);
		Widgets.APIcon = Find<UWidget>(WidgetTree, TEXT("PartyAPIcon") + Suffix);
		Widgets.StatusText = Find<UTextBlock>(WidgetTree, TEXT("PartyStatus") + Suffix);
		Widgets.StatusIcon = Find<UWidget>(WidgetTree, TEXT("PartyStatusIcon") + Suffix);
		Widgets.APPips.Reset();
		for (int32 Pip = 0; Pip < 8; ++Pip)
		{
			UWidget* Found = Find<UWidget>(WidgetTree,
				FString::Printf(TEXT("PartyAPPip_%d_%d"), Index, Pip));
			if (Found == nullptr)
			{
				break;
			}
			Widgets.APPips.Add(Found);
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
		Widgets.Selected = Find<UWidget>(WidgetTree, TEXT("CommandSelected") + Suffix);
	}

	mTurnSlots.SetNum(TurnSlotCount);
	for (int32 Index = 0; Index < TurnSlotCount; ++Index)
	{
		FTurnSlotWidgets& Widgets = mTurnSlots[Index];
		const FString Suffix = FString::Printf(TEXT("_%d"), Index);
		Widgets.Root = Find<UWidget>(WidgetTree, TEXT("TurnToken") + Suffix);
		Widgets.Portrait = Find<UImage>(WidgetTree, TEXT("TurnPortrait") + Suffix);
		Widgets.Name = Find<UTextBlock>(WidgetTree, TEXT("TurnName") + Suffix);
		Widgets.Current = Find<UWidget>(WidgetTree, TEXT("TurnCurrent") + Suffix);
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
}

/**
 * @brief 실제 전투 뷰모델에 붙고, 없으면 미리보기 장면을 세운다.
 *
 * @details
 * 전투 중이면 ACombatGameMode가 들고 있는 공용 UCombatUIModel에 붙는다.
 * HUD를 만든 쪽은 위젯을 CreateWidget으로 만들고 OpenUI()만 부르므로, 모델을
 * 찾아 붙이는 건 위젯 몫이다 -- 기존 UCombatTileMapHUDWidget도 같은 자리에서
 * 같은 일을 한다. 이걸 빼면 인게임에서 가짜 데이터가 그려진다.
 *
 * 전투 게임모드가 아니면(에디터 미리보기, 캡처 테스트) 가짜 장면을 세운다.
 * 그 UIModel과 드라이버는 위젯이 소유하고 화면과 수명을 같이 한다.
 */
void UCombatLayoutHUDWidget::StartPreviewIfUnbound()
{
	if (mUIModel != nullptr)
	{
		return;
	}

	if (const UWorld* World = GetWorld())
	{
		if (ACombatGameMode* GameMode = World->GetAuthGameMode<ACombatGameMode>())
		{
			if (UCombatUIModel* LiveModel = GameMode->GetCombatUIModel())
			{
				BindUIModel(LiveModel);
				return;
			}
		}
	}

	if (!mUsePreviewData)
	{
		return;
	}
	UCombatUIModel* PreviewModel = NewObject<UCombatUIModel>(this);
	mPreviewDriver = NewObject<UMockCombatDriver>(this);
	BindUIModel(PreviewModel);
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
		}
	}

	if (mEndTurnButton != nullptr)
	{
		mEndTurnButton->OnClicked.AddUniqueDynamic(
			this, &UCombatLayoutHUDWidget::HandleEndTurnClicked);
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
	}
	if (bAll || Domain == ECombatUIDomain::Turn)
	{
		RefreshTurnOrder();
		RefreshParty();
		// 차례가 **바뀌었을 때만** 편다. 갱신이 올 때마다 펴면 접자마자 다시
		// 펴져서 접기가 안 먹는 것처럼 보인다 -- 실제로 그랬다. Turn 갱신은
		// 차례가 그대로여도 여러 번 온다.
		const int32 TurnUnitId = mUIModel != nullptr
			? mUIModel->GetTurnUI().mCurrentUnitId : INDEX_NONE;
		if (TurnUnitId != mLastTurnUnitId)
		{
			mLastTurnUnitId = TurnUnitId;
			SetCommandsShown(true);
		}

		// 조준에 들었거나 조준이 끝났을 수 있다. 카드가 비켜 있을지 여기서
		// 다시 정한다 -- 조준 단계는 Turn 으로 실려 온다.
		RefreshCommandVisibility();
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
	}
	if (bAll || Domain == ECombatUIDomain::Skill)
	{
		RefreshCommands();
	}
	if (bAll || Domain == ECombatUIDomain::Meta)
	{
		RefreshMeta();
	}
}

namespace CombatLayoutDummy
{
	/**
	 * @brief 전투 모델이 아직 안 채우는 칸을 임시 값으로 메운다.
	 *
	 * @details
	 * 인게임에서 이름이 빈칸으로 나오고 AP가 0/0으로 찍혔다. ACombatGameMode가
	 * FUnitUI를 채울 때 HP만 넣고 이름과 AP를 안 넣기 때문인데, 그 파일은
	 * 전투 쪽 소유라 여기서 손대지 않는다.
	 *
	 * 지금 봐야 하는 것은 배치가 맞느냐다. 빈칸이면 화면에서 자리를 볼 수가
	 * 없으므로, **비어 있을 때만** 임시 값을 넣는다. 모델이 값을 채우기
	 * 시작하면 이 코드는 아무것도 하지 않고 저절로 비켜난다 -- 실제 값을
	 * 덮어쓰지 않는다.
	 *
	 * 끄려면 콘솔에서 RD.UI.DummyFill 0.
	 */
	static TAutoConsoleVariable<int32> CVarDummyFill(
		TEXT("RD.UI.DummyFill"), 1,
		TEXT("전투 모델이 안 채운 UI 칸을 임시 값으로 메운다(1=켬)."),
		ECVF_Default);

	static bool Enabled()
	{
		return CVarDummyFill.GetValueOnGameThread() != 0;
	}

	/** @brief 이름이 없을 때 쓰는 임시 이름. 자리를 보기 위한 것이다. */
	static FText UnitName(const int32 SlotIndex)
	{
		static const TCHAR* Names[] = { TEXT("기사"), TEXT("궁수"), TEXT("마법사") };
		const int32 Index = FMath::Clamp(SlotIndex, 0, UE_ARRAY_COUNT(Names) - 1);
		return FText::FromString(Names[Index]);
	}
}

void UCombatLayoutHUDWidget::RefreshParty()
{
	const TArray<FUnitUI>& Units = mUIModel->GetUnitUIs();
	const int32 CurrentUnitId = mUIModel->GetTurnUI().mCurrentUnitId;

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
		SetShown(Widgets.Selected, Unit.mUnitId == CurrentUnitId);

		// 빈 칸 처리가 접어 둔 것들을 다시 켠다.
		//
		// 위젯이 붙는 순간의 UIModel은 비어 있다. 그 첫 갱신에서 세 칸 모두
		// 빈 칸으로 그려지며 HP 바와 초상화가 접히고, 곧이어 데이터가 들어와도
		// 여기서 켜 주지 않으면 영영 접힌 채로 남는다. 실제로 그렇게 됐다 --
		// 이름과 숫자는 나오는데 바와 얼굴만 사라진 화면이었다.
		SetShown(Widgets.HPBar, true);
		SetShown(Widgets.Portrait, true);

		const bool bDummy = CombatLayoutDummy::Enabled();
		SetTextIfPresent(Widgets.Name,
			(bDummy && Unit.mName.IsEmpty())
				? CombatLayoutDummy::UnitName(SlotIndex) : Unit.mName);

		// 유닛 초상은 배치안이 깔아 둔 얼굴을 덮는다. 임시 채움 중에는
		// 덮지 않는다 -- 유닛 데이터에셋 초상이 배경이 뚫리지 않은 사각형
		// 이라 금속 테를 가려서, 배치를 볼 수가 없다.
		if (Widgets.Portrait != nullptr && Unit.mPortrait != nullptr && !bDummy)
		{
			Widgets.Portrait->SetBrushFromTexture(Unit.mPortrait, false);
		}
		if (Widgets.HPBar != nullptr)
		{
			Widgets.HPBar->SetPercent(Unit.mMaxHP > 0.f ? Unit.mHP / Unit.mMaxHP : 0.f);
		}
		SetTextIfPresent(Widgets.HPText, FText::FromString(FString::Printf(
			TEXT("%d/%d"), FMath::RoundToInt(Unit.mHP), FMath::RoundToInt(Unit.mMaxHP))));
		// AP 에는 상한이 없다. 그래서 "3/4" 처럼 못 적는다 -- 분모가 없다.
		const bool bDummyAP = bDummy && Unit.mActionPoints <= 0;
		const int32 ShownAP = bDummyAP ? 3 : Unit.mActionPoints;

		// 남은 만큼 낱개로 켠다. 자리보다 많으면 낱개로는 못 보여주므로
		// 아이콘 하나에 "x N" 을 붙인다. 자리를 무한히 잡아 둘 수는 없다.
		const int32 PipRoom = Widgets.APPips.Num();
		const bool bOverflow = PipRoom > 0 && ShownAP > PipRoom;
		for (int32 Pip = 0; Pip < PipRoom; ++Pip)
		{
			SetShown(Widgets.APPips[Pip], !bOverflow && Pip < ShownAP);
		}
		SetShown(Widgets.APIcon, bOverflow);
		SetShown(Widgets.APText, bOverflow);
		if (bOverflow)
		{
			SetTextIfPresent(Widgets.APText, FText::FromString(
				FString::Printf(TEXT("x %d"), ShownAP)));
		}

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
	SetShown(Widgets.Selected, false);

	SetTextIfPresent(Widgets.Name, FText::GetEmpty());
	SetTextIfPresent(Widgets.HPText, FText::GetEmpty());
	SetTextIfPresent(Widgets.APText, FText::GetEmpty());
	SetShown(Widgets.APText, false);
	SetShown(Widgets.APIcon, false);
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
}

void UCombatLayoutHUDWidget::RefreshTurnOrder()
{
	const FTurnUI& Turn = mUIModel->GetTurnUI();
	const TArray<FUnitUI>& Units = mUIModel->GetUnitUIs();

	SetTextIfPresent(mRoundText, FText::FromString(
		FString::Printf(TEXT("ROUND %d"), Turn.mRound)));

	for (int32 Index = 0; Index < mTurnSlots.Num(); ++Index)
	{
		const FTurnSlotWidgets& Widgets = mTurnSlots[Index];
		if (!Turn.mTurnOrderUnitIds.IsValidIndex(Index))
		{
			SetShown(Widgets.Root, false);
			continue;
		}
		const int32 UnitId = Turn.mTurnOrderUnitIds[Index];
		const FUnitUI* Unit = Units.FindByPredicate(
			[UnitId](const FUnitUI& Candidate) { return Candidate.mUnitId == UnitId; });
		if (Unit == nullptr)
		{
			SetShown(Widgets.Root, false);
			continue;
		}
		SetShown(Widgets.Root, true);
		SetShown(Widgets.Current, UnitId == Turn.mCurrentUnitId);
		SetTextIfPresent(Widgets.Name, Unit->mName);
		if (Widgets.Portrait != nullptr && Unit->mPortrait != nullptr)
		{
			Widgets.Portrait->SetBrushFromTexture(Unit->mPortrait, false);
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
	const int32 SelectedSkill = mUIModel->GetSelectedSkillIndex();

	for (int32 SlotIndex = 0; SlotIndex < mCommandSlots.Num(); ++SlotIndex)
	{
		const FCommandSlotWidgets& Widgets = mCommandSlots[SlotIndex];

		if (SlotIndex == 0)
		{
			SetShown(Widgets.Root, true);
			SetTextIfPresent(Widgets.Name, LOCTEXT("Move", "이동"));
			SetTextIfPresent(Widgets.Cost, FText::FromString(TEXT("1")));
			SetTextIfPresent(Widgets.CostLine, LOCTEXT("MoveCost", "AP 1"));
			SetShown(Widgets.Cooldown, false);
			SetShown(Widgets.CooldownIcon, false);
			SetShown(Widgets.Damage, false);
			SetShown(Widgets.Disabled, false);
			SetShown(Widgets.Selected, false);
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

		// 값이 안 오면 배지와 AP 줄이 0으로 찍혀 자리를 볼 수 없다.
		const int32 ShownCost = (CombatLayoutDummy::Enabled()
			&& Skill.mActionPointCost <= 0) ? 1 : Skill.mActionPointCost;
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
			Widgets.Cooldown->SetText(bOnCooldown
				? FText::FromString(FString::Printf(TEXT("%d턴"), Skill.mRemainingCooldown))
				: FText::FromString(FString::Printf(TEXT("쿨 %d턴"), Skill.mCooldownTurns)));
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
		SetShown(Widgets.Selected, SkillIndex == SelectedSkill);
		if (Widgets.Button != nullptr)
		{
			Widgets.Button->SetIsEnabled(Skill.mIsUsable);
		}
	}
}

void UCombatLayoutHUDWidget::RefreshEnemy()
{
	// 겨냥한 유닛을 먼저 본다. 없으면 살아 있는 첫 적으로 떨어진다 -- 전투가
	// 막 시작돼 아무 데도 안 눌렀을 때가 그렇다.
	const TArray<FUnitUI>& Units = mUIModel->GetUnitUIs();
	const FCombatTargetUI& Target = mUIModel->GetTarget();
	const int32 TargetId = Target.mIsValid ? Target.mUnitId : INDEX_NONE;
	const FUnitUI* Shown = TargetId != INDEX_NONE
		? Units.FindByPredicate([TargetId](const FUnitUI& Unit)
			{ return Unit.mUnitId == TargetId && Unit.mHP > 0.f; })
		: nullptr;
	if (Shown == nullptr)
	{
		Shown = Units.FindByPredicate(
			[](const FUnitUI& Unit) { return !Unit.mIsPlayer && Unit.mHP > 0.f; });
	}

	SetShown(mEnemyPanel, Shown != nullptr);
	if (Shown == nullptr)
	{
		return;
	}

	SetTextIfPresent(mEnemyName, Shown->mName);
	if (mEnemyPortrait != nullptr && Shown->mPortrait != nullptr)
	{
		mEnemyPortrait->SetBrushFromTexture(Shown->mPortrait, false);
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
}

void UCombatLayoutHUDWidget::RequestCommand(const int32 SlotIndex)
{
	if (mUIModel == nullptr)
	{
		return;
	}
	if (SlotIndex == 0)
	{
		mUIModel->RequestMove();
		return;
	}
	mUIModel->RequestSelectSkill(SlotIndex - 1);
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

bool UCombatLayoutHUDWidget::IsAiming() const
{
	return mUIModel != nullptr
		&& mUIModel->GetTurnUI().mPhase != ECombatBuildPhaseUI::None;
}

void UCombatLayoutHUDWidget::RefreshCommandVisibility()
{
	const bool bVisible = mCommandsShown == true && IsAiming() == false;
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
	HandleBoardPressed(FVector2D(InMouseEvent.GetScreenSpacePosition()));
	return FReply::Handled();
}

FReply UCombatLayoutHUDWidget::NativeOnTouchStarted(const FGeometry& InGeometry,
	const FPointerEvent& InTouchEvent)
{
	HandleBoardPressed(FVector2D(InTouchEvent.GetScreenSpacePosition()));
	return FReply::Handled();
}

void UCombatLayoutHUDWidget::HandleBoardPressed(const FVector2D& ScreenPosition)
{
	if (mUIModel == nullptr)
	{
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

	// 조준 중이 아니면 판 탭은 카드를 뒤집는 것뿐이다. 어느 칸을 눌렀는지는
	// 안 본다 -- 타일을 눌러 카드를 여는 규칙은 없앴다. 판 아무 데나 누르면
	// 펴지고, 다시 누르면 접힌다.
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

	// 조준 중이었으면 무른다. 남의 스킬을 보러 가면서 겨냥만 남겨 두면 그
	// 사거리가 누구 것인지 알 수 없다.
	if (IsAiming() == true)
	{
		mUIModel->RequestCancel();
	}

	// 그 용병의 스킬로 갈아 끼워 달라고 한다. 제 차례가 아니면 전부 꺼진 채로
	// 온다 -- 끄는 판정은 게임플레이가 한다.
	mUIModel->RequestInspectUnit(PartyUnitIdAt(SlotIndex));

	// 접혀 있었으면 편다. 스킬을 보러 눌렀는데 안 펴지면 아무 일도 안 일어난
	// 것처럼 보인다.
	SetCommandsShown(true);
}

void UCombatLayoutHUDWidget::HandlePartyClicked_0() { HandlePartyClicked(0); }
void UCombatLayoutHUDWidget::HandlePartyClicked_1() { HandlePartyClicked(1); }
void UCombatLayoutHUDWidget::HandlePartyClicked_2() { HandlePartyClicked(2); }

void UCombatLayoutHUDWidget::HandleEndTurnClicked()
{
	if (mUIModel != nullptr)
	{
		mUIModel->RequestEndTurn();
	}
}

#undef LOCTEXT_NAMESPACE
