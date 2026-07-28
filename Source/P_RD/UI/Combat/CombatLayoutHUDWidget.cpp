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
		TEXT("PartyCard_0"), TEXT("PartyCard_1"), TEXT("PartyCard_2") })
	{
		if (UWidget* Found = Find<UWidget>(WidgetTree, Name))
		{
			mChromeWidgets.Add(Found);
		}
	}
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

	// 상단 메뉴 넷. 무엇을 열지는 아직 안 정해서 누르는 느낌만 걸어 둔다 --
	// 전투 중에 지도와 가방을 여는 것이 맞는지가 기획 결정이라 UI 혼자
	// 정하지 않는다.
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
 * @brief 왼쪽 넘김칸을 눌렀다. 창을 맨 앞으로 되돌린다.
 *
 * @details
 * 한 칸씩 밀지 않는다. 여섯 칸 창에서 볼 것이 그렇게 많지 않은데 한 칸씩
 * 밀면 여덟 번을 눌러야 하는 판이 생긴다.
 */
void UCombatLayoutHUDWidget::HandleTurnPageLeftClicked()
{
	mTurnWindowStart = 0;
	RefreshTurnOrder();
}

/** @brief 오른쪽 넘김칸을 눌렀다. 창을 맨 뒤로 옮긴다. */
void UCombatLayoutHUDWidget::HandleTurnPageRightClicked()
{
	if (mUIModel == nullptr)
	{
		return;
	}
	mTurnWindowStart = FMath::Max(
		mUIModel->GetTurnUI().mTurnOrderUnitIds.Num() - mTurnSlots.Num(), 0);
	RefreshTurnOrder();
}

/** @brief 놓았다. 줄여 둔 것을 되돌린다. */
void UCombatLayoutHUDWidget::HandleAnyReleased()
{
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
			// 줄 자체가 한 칸 밀렸다. 옮겨 둔 창을 그대로 두면 다음 턴에
			// 엉뚱한 곳을 보고 있다.
			mTurnWindowStart = 0;
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
			Widgets.Portrait->SetBrushFromTexture(Unit.mPortrait, false);
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
		if (UWidget* Frame = Widgets.StatusFrames[SlotNo])
		{
			SetShown(Frame, true);
			Frame->SetRenderOpacity(EmptyStatusOpacity);
		}
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

		// 홈은 늘 서 있고, 빈 홈만 흐려진다. 감추면 카드 위가 뻥 뚫려서
		// 원래 그런 칸인지 사라진 것인지 모른다.
		if (UWidget* Frame = Widgets.StatusFrames[SlotNo])
		{
			SetShown(Frame, true);
			Frame->SetRenderOpacity(bFilled ? 1.f : EmptyStatusOpacity);
		}

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
	const int32 Left = FMath::Max(FMath::RoundToInt(Unit.mMovementPoint), 0);
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
 * @brief 턴 순서 줄을 그린다. 칸보다 도는 유닛이 많으면 양끝에 남은 수를 적는다.
 *
 * @details
 * 칸은 여섯인데 한 판에 일곱이 돌 수 있다. 잘라 버리면 일곱째가 있는지조차
 * 모른다 -- 지금까지 그랬다. 창을 두고 양끝에 **그쪽에 가려진 수**를 적는다.
 * 화살표가 아니라 수라, 누르지 않고도 몇이 도는지 읽힌다.
 */
void UCombatLayoutHUDWidget::RefreshTurnOrder()
{
	const FTurnUI& Turn = mUIModel->GetTurnUI();
	const TArray<FUnitUI>& Units = mUIModel->GetUnitUIs();

	SetTextIfPresent(mRoundText, FText::FromString(
		FString::Printf(TEXT("ROUND %d"), Turn.mRound)));

	const int32 SlotRoom = mTurnSlots.Num();
	const int32 Total = Turn.mTurnOrderUnitIds.Num();
	const int32 Start = FMath::Clamp(mTurnWindowStart, 0,
		FMath::Max(Total - SlotRoom, 0));
	mTurnWindowStart = Start;

	const int32 HiddenLeft = Start;
	const int32 HiddenRight = FMath::Max(Total - (Start + SlotRoom), 0);
	// 글자는 버튼의 자식이 아니라 옆에 있는 형제다. 버튼만 접으면 수가
	// 허공에 남는다.
	SetShown(mTurnPageLeft, HiddenLeft > 0);
	SetShown(mTurnPageLeftText, HiddenLeft > 0);
	SetShown(mTurnPageRight, HiddenRight > 0);
	SetShown(mTurnPageRightText, HiddenRight > 0);
	SetTextIfPresent(mTurnPageLeftText, FText::AsNumber(HiddenLeft));
	SetTextIfPresent(mTurnPageRightText, FText::AsNumber(HiddenRight));

	for (int32 Index = 0; Index < SlotRoom; ++Index)
	{
		const FTurnSlotWidgets& Widgets = mTurnSlots[Index];
		if (!Turn.mTurnOrderUnitIds.IsValidIndex(Start + Index))
		{
			SetShown(Widgets.Root, false);
			continue;
		}
		const int32 UnitId = Turn.mTurnOrderUnitIds[Start + Index];
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
		// 얼굴이 있을 때만 편다. 브러시가 없는 이미지는 흰 네모로 그려져서,
		// 접힌 채로 구워 두고 여기서 편다.
		SetShown(Widgets.Portrait, Unit->mPortrait != nullptr);
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
			Widgets.Button->SetIsEnabled(Skill.mIsUsable);
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
	// 차례를 아직 모를 때는 접지 않는다. 전투가 막 시작돼 첫 갱신이 오기
	// 전이 그런데, 여기서 접으면 첫 차례에 카드가 안 뜬다.
	return TurnUnit == nullptr || TurnUnit->mIsPlayer;
}

void UCombatLayoutHUDWidget::HandleActionPresentationBegin(TSharedPtr<FPresentationBarrier> Barrier)
{
	mIsActionPlaying = true;
	RefreshCommandVisibility();
}

void UCombatLayoutHUDWidget::HandleActionPresentationEnd(TSharedPtr<FPresentationBarrier> Barrier)
{
	mIsActionPlaying = false;
	RefreshCommandVisibility();
}

void UCombatLayoutHUDWidget::BindUIModel(UCombatUIModel* InUIModel)
{
	Super::BindUIModel(InUIModel);
	if (mUIModel != nullptr)
	{
		mActionBeginHandle = mUIModel->OnBeginAnyTurnAction.AddUObject(
			this, &UCombatLayoutHUDWidget::HandleActionPresentationBegin);
		mActionEndHandle = mUIModel->OnEndAnyTurnAction.AddUObject(
			this, &UCombatLayoutHUDWidget::HandleActionPresentationEnd);
	}
}

void UCombatLayoutHUDWidget::UnbindUIModel()
{
	if (mUIModel != nullptr)
	{
		mUIModel->OnBeginAnyTurnAction.Remove(mActionBeginHandle);
		mUIModel->OnEndAnyTurnAction.Remove(mActionEndHandle);
	}
	mActionBeginHandle.Reset();
	mActionEndHandle.Reset();
	Super::UnbindUIModel();
}

bool UCombatLayoutHUDWidget::IsAiming() const
{
	return mUIModel != nullptr
		&& mUIModel->GetTurnUI().mPhase != ECombatBuildPhaseUI::None;
}

void UCombatLayoutHUDWidget::RefreshCommandVisibility()
{
	// 넷이 다 참이어야 보인다. 하나라도 아니면 접는다.
	const bool bVisible = mCommandsShown == true
		&& IsAiming() == false
		&& IsPlayerTurn() == true
		&& mIsActionPlaying == false;
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

void UCombatLayoutHUDWidget::HandleBoardPressed(const FVector2D& ScreenPosition)
{
	if (mUIModel == nullptr)
	{
		return;
	}

	// HUD 를 누른 것은 판을 누른 것이 아니다. 조준 중이어도 마찬가지다 --
	// 안내판을 누르려다 엉뚱한 칸에 스킬을 쏘면 무를 길이 없다.
	if (IsOverChrome(ScreenPosition) == true)
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
