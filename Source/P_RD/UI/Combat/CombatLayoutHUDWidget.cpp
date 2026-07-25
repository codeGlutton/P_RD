#include "UI/Combat/CombatLayoutHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/Combat/MockCombatDriver.h"

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
}

void UCombatLayoutHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	CacheAuthoredWidgets();
	WireCommands();
	StartPreviewIfUnbound();
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
		Widgets.Selected = Find<UWidget>(WidgetTree, TEXT("PartySelected") + Suffix);
		Widgets.Portrait = Find<UImage>(WidgetTree, TEXT("PartyPortrait") + Suffix);
		Widgets.Name = Find<UTextBlock>(WidgetTree, TEXT("PartyName") + Suffix);
		Widgets.HPBar = Find<UProgressBar>(WidgetTree, TEXT("PartyHPBar") + Suffix);
		Widgets.HPText = Find<UTextBlock>(WidgetTree, TEXT("PartyHPText") + Suffix);
		Widgets.APText = Find<UTextBlock>(WidgetTree, TEXT("PartyAPText") + Suffix);
		Widgets.StatusText = Find<UTextBlock>(WidgetTree, TEXT("PartyStatus") + Suffix);
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
		Widgets.Cooldown = Find<UTextBlock>(WidgetTree, TEXT("CommandCooldown") + Suffix);
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
 * @brief 붙은 UIModel이 없으면 미리보기용 전투 장면을 세운다.
 *
 * @details
 * UIModel과 드라이버를 위젯이 소유한다. 미리보기 전용이라 수명이 화면과 같고,
 * 실제 전투에서는 이 경로를 아예 타지 않으므로 소유권이 겹치지 않는다.
 */
void UCombatLayoutHUDWidget::StartPreviewIfUnbound()
{
	if (!mUsePreviewData || mUIModel != nullptr)
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
		SetShown(Widgets.Selected, Unit.mUnitId == CurrentUnitId);

		SetTextIfPresent(Widgets.Name, Unit.mName);
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
		SetTextIfPresent(Widgets.APText, FText::FromString(FString::Printf(
			TEXT("%d/%d"), Unit.mActionPoints, Unit.mMaxActionPoints)));

		// 칸(pip) 방식으로 그리는 배치안을 위해. 숫자만 쓰는 안은 pip이 없어 그냥 넘어간다.
		for (int32 Pip = 0; Pip < Widgets.APPips.Num(); ++Pip)
		{
			SetShown(Widgets.APPips[Pip], Pip < Unit.mActionPoints);
		}

		if (Widgets.StatusText != nullptr)
		{
			const bool bHasStatus = Unit.mStatusEffects.Num() > 0;
			SetShown(Widgets.StatusText, bHasStatus);
			if (bHasStatus)
			{
				const FStatusEffectUI& First = Unit.mStatusEffects[0];
				Widgets.StatusText->SetText(FText::FromString(FString::Printf(
					TEXT("%s %d턴"),
					*First.mTag.GetTagName().ToString(), First.mStackCount)));
			}
		}
		++SlotIndex;
	}

	// 파티가 3명보다 적을 때 남는 칸은 접는다.
	for (; SlotIndex < mPartySlots.Num(); ++SlotIndex)
	{
		SetShown(mPartySlots[SlotIndex].Root, false);
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
			SetShown(Widgets.Cooldown, false);
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
		SetTextIfPresent(Widgets.Cost, FText::AsNumber(Skill.mActionPointCost));

		if (Widgets.Icon != nullptr && Skill.mIcon != nullptr)
		{
			Widgets.Icon->SetBrushFromTexture(Skill.mIcon, false);
		}

		// 쿨타임: 남은 턴이 있으면 그 숫자를, 없으면 설정된 쿨타임을 알려준다.
		if (Widgets.Cooldown != nullptr)
		{
			const bool bOnCooldown = Skill.mRemainingCooldown > 0;
			SetShown(Widgets.Cooldown, bOnCooldown || Skill.mCooldownTurns > 0);
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
				Widgets.Damage->SetText(FText::FromString(FString::Printf(
					TEXT("%d~%d"), Skill.mDamageMin, Skill.mDamageMax)));
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
	// 지금은 살아 있는 적 중 첫 번째를 보여준다. 대상 선택이 붙으면
	// UIModel의 선택 상태를 읽도록 바꾼다.
	const TArray<FUnitUI>& Units = mUIModel->GetUnitUIs();
	const FUnitUI* Target = Units.FindByPredicate(
		[](const FUnitUI& Unit) { return !Unit.mIsPlayer && Unit.mHP > 0.f; });

	SetShown(mEnemyPanel, Target != nullptr);
	if (Target == nullptr)
	{
		return;
	}

	SetTextIfPresent(mEnemyName, Target->mName);
	if (mEnemyPortrait != nullptr && Target->mPortrait != nullptr)
	{
		mEnemyPortrait->SetBrushFromTexture(Target->mPortrait, false);
	}
	if (mEnemyHPBar != nullptr)
	{
		mEnemyHPBar->SetPercent(
			Target->mMaxHP > 0.f ? Target->mHP / Target->mMaxHP : 0.f);
	}
	SetTextIfPresent(mEnemyHPText, FText::FromString(FString::Printf(
		TEXT("%d/%d"), FMath::RoundToInt(Target->mHP), FMath::RoundToInt(Target->mMaxHP))));
	SetTextIfPresent(mEnemyDefenseText, FText::FromString(FString::Printf(
		TEXT("방어 %d"), FMath::RoundToInt(Target->mDefensePoint))));

	if (mEnemyStatusText != nullptr)
	{
		TArray<FString> Parts;
		for (const FStatusEffectUI& Status : Target->mStatusEffects)
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

void UCombatLayoutHUDWidget::HandleEndTurnClicked()
{
	if (mUIModel != nullptr)
	{
		mUIModel->RequestEndTurn();
	}
}

#undef LOCTEXT_NAMESPACE
