#include "UI/Combat/MockCombatDriver.h"

#include "UI/Combat/CombatViewModel.h"

void UMockCombatDriver::Start(UCombatViewModel* ViewModel)
{
	if (ViewModel == nullptr)
	{
		return;
	}
	mViewModel = ViewModel;

	// 위젯이 보낸 입력(의도)을 받아 흉내낸다.
	mViewModel->OnCombatCommand.AddDynamic(this, &UMockCombatDriver::HandleCommand);
	mViewModel->OnCombatWorldTouch.AddDynamic(this, &UMockCombatDriver::HandleWorldTouch);

	// 가짜 유닛: 플레이어 1 + 적 2
	TArray<FUnitView> Units;
	{
		FUnitView Player;
		Player.mUnitId = 0;
		Player.mIsPlayer = true;
		Player.mHP = 30.f; Player.mMaxHP = 30.f;
		Player.mDamagePoint = 5.f; Player.mDefensePoint = 2.f; Player.mMovementPoint = 0.f; Player.mSkillPoint = 0.f;
		Player.mTile = FTileIndex(4, 8);
		Units.Add(Player);

		for (int32 i = 0; i < 2; ++i)
		{
			FUnitView Enemy;
			Enemy.mUnitId = 1 + i;
			Enemy.mIsPlayer = false;
			Enemy.mHP = 12.f; Enemy.mMaxHP = 12.f;
			Enemy.mDamagePoint = 4.f; Enemy.mDefensePoint = 1.f;
			Enemy.mTile = FTileIndex(3 + i, 3);
			Units.Add(Enemy);
		}
	}
	mViewModel->SetUnitViews(Units);

	// 가짜 주사위 6개
	mDice.Reset();
	const FLinearColor RarityColors[] = { FLinearColor(0.86f, 0.98f, 0.94f, 1.f), FLinearColor(0.55f, 0.72f, 1.f, 1.f), FLinearColor(0.82f, 0.58f, 1.f, 1.f) };
	const TCHAR* RarityTexts[] = { TEXT("Common"), TEXT("Rare"), TEXT("Epic") };
	for (int32 i = 0; i < 6; ++i)
	{
		FDiceSlotView Die;
		Die.mResultValue = 0;
		Die.mIsRolled = false;
		const int32 Tier = i % 3;
		Die.mRarityColor = RarityColors[Tier];
		Die.mRarityText = FText::FromString(RarityTexts[Tier]);
		mDice.Add(Die);
	}
	RebuildDicePush();

	// 가짜 스킬 6개
	TArray<FSkillView> Skills;
	for (int32 i = 0; i < 6; ++i)
	{
		FSkillView Skill;
		Skill.mSkillIndex = i;
		Skill.mName = FText::FromString(FString::Printf(TEXT("Skill %d"), i + 1));
		Skill.mDiceCost = 1 + (i % 3);
		Skill.mIsUsable = true;
		Skills.Add(Skill);
	}
	mViewModel->SetSkillViews(Skills);

	// 가짜 턴
	FTurnView Turn;
	Turn.mCurrentUnitId = 0;
	Turn.mRound = 1;
	Turn.mPhase = ECombatBuildPhaseView::None;
	Turn.mTurnOrderUnitIds = { 0, 1, 2 };
	mViewModel->SetTurnView(Turn);

	// 가짜 장비 3슬롯
	TArray<FEquipmentView> Equipment;
	for (int32 i = 0; i < 3; ++i)
	{
		FEquipmentView Equip;
		Equip.mSlotIndex = i;
		Equip.mName = FText::FromString(FString::Printf(TEXT("Equip %d"), i + 1));
		Equip.mIsEquipped = (i == 0);
		Equipment.Add(Equip);
	}
	mViewModel->SetEquipmentViews(Equipment);

	// 가짜 플레이어 메타(돈/경험치/레벨)
	FPlayerMetaView Meta;
	Meta.mGold = 120;
	Meta.mLevel = 3;
	Meta.mExp = 40.f; Meta.mMaxExp = 100.f;
	mViewModel->SetPlayerMeta(Meta);
}

void UMockCombatDriver::RebuildDicePush()
{
	for (int32 i = 0; i < mDice.Num(); ++i)
	{
		mDice[i].mIsSelected = mSelectedDice.Contains(i);
	}
	if (mViewModel != nullptr)
	{
		mViewModel->SetDiceViews(mDice);

		int32 Sum = 0;
		for (int32 Index : mSelectedDice)
		{
			if (mDice.IsValidIndex(Index))
			{
				Sum += mDice[Index].mResultValue;
			}
		}
		mViewModel->SetSelectedDice(mSelectedDice, Sum);
	}
}

void UMockCombatDriver::HandleCommand(ECombatInputType Type, int32 IntPayload)
{
	switch (Type)
	{
	case ECombatInputType::RollDice:
		for (FDiceSlotView& Die : mDice)
		{
			Die.mResultValue = FMath::RandRange(1, 6);
			Die.mIsRolled = true;
		}
		RebuildDicePush();
		break;

	case ECombatInputType::ToggleDice:
		if (mSelectedDice.Contains(IntPayload))
		{
			mSelectedDice.Remove(IntPayload);
		}
		else if (mDice.IsValidIndex(IntPayload))
		{
			mSelectedDice.Add(IntPayload);
		}
		RebuildDicePush();
		break;

	case ECombatInputType::Cancel:
		mSelectedDice.Reset();
		RebuildDicePush();
		break;

	case ECombatInputType::LongPressUnit:
		if (mViewModel != nullptr)
		{
			FUnitDetailView Detail;
			Detail.mUnitId = IntPayload;
			Detail.mName = FText::FromString(FString::Printf(TEXT("Enemy %d"), IntPayload));
			Detail.mLevel = 1;
			Detail.mHP = 12.f; Detail.mMaxHP = 12.f; Detail.mDamagePoint = 4.f;
			Detail.mPassiveDescriptions = { FText::FromString(TEXT("(mock passive)")) };
			mViewModel->SetUnitDetail(Detail);
		}
		break;

	default:
		UE_LOG(LogRD, Display, TEXT("MockCombatDriver: command %d (payload %d)"), static_cast<int32>(Type), IntPayload);
		break;
	}
}

void UMockCombatDriver::HandleWorldTouch(FVector2D ScreenPosition, bool bLongPress)
{
	UE_LOG(LogRD, Display, TEXT("MockCombatDriver: world touch (%.0f,%.0f) long=%d"), ScreenPosition.X, ScreenPosition.Y, bLongPress ? 1 : 0);
}

void UMockCombatDriver::PushMockPreview()
{
	if (mViewModel == nullptr)
	{
		return;
	}
	TArray<FCombatQueueNode> Queue;

	FCombatQueueNode Damage;
	Damage.mSourceUnitId = 0; Damage.mTargetUnitId = 1; Damage.mAmount = 7;
	Damage.mLabel = FText::FromString(TEXT("-7"));
	Queue.Add(Damage);

	FCombatQueueNode Status;
	Status.mSourceUnitId = 0; Status.mTargetUnitId = 1; Status.mAmount = 0;
	Status.mLabel = FText::FromString(TEXT("Stun"));
	Queue.Add(Status);

	FCombatQueueNode Heal;
	Heal.mSourceUnitId = 0; Heal.mTargetUnitId = 0; Heal.mAmount = 3;
	Heal.mLabel = FText::FromString(TEXT("+3"));
	Queue.Add(Heal);

	mViewModel->SetActionQueue(Queue);
}

void UMockCombatDriver::PlayNextQueueNode()
{
	if (mViewModel != nullptr)
	{
		mViewModel->ResolveFrontQueueNode();
	}
}
