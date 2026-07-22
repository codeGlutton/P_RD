#include "UI/Combat/MockCombatDriver.h"

#include "UI/Combat/CombatUIModel.h"

#define LOCTEXT_NAMESPACE "MockCombatDriver"

/** @brief UIModel에 mock 전투 스냅샷을 채우고 UI 입력 델리게이트가 왕복하는지 검증한다. */
void UMockCombatDriver::Start(UCombatUIModel* UIModel)
{
	if (UIModel == nullptr)
	{
		return;
	}
	mUIModel = UIModel;

	// 위젯이 보낸 입력(의도)을 받아 흉내낸다.
	mUIModel->OnCombatCommand.AddDynamic(this, &UMockCombatDriver::HandleCommand);
	mUIModel->OnCombatWorldTouch.AddDynamic(this, &UMockCombatDriver::HandleWorldTouch);

	// 가짜 유닛: 플레이어 1 + 적 2
	TArray<FUnitUI> Units;
	{
		FUnitUI Player;
		Player.mUnitId = 0;
		Player.mIsPlayer = true;
		Player.mHP = 30.f; Player.mMaxHP = 30.f;
		Player.mDamagePoint = 5.f; Player.mDefensePoint = 2.f; Player.mMovementPoint = 0.f; Player.mSkillPoint = 0.f;
		Player.mTile = FTileIndex(4, 8);
		Units.Add(Player);

		for (int32 i = 0; i < 2; ++i)
		{
			FUnitUI Enemy;
			Enemy.mUnitId = 1 + i;
			Enemy.mIsPlayer = false;
			Enemy.mHP = 12.f; Enemy.mMaxHP = 12.f;
			Enemy.mDamagePoint = 4.f; Enemy.mDefensePoint = 1.f;
			Enemy.mTile = FTileIndex(3 + i, 3);
			Units.Add(Enemy);
		}
	}
	mUIModel->SetUnitUIs(Units);

	// 가짜 스킬 6개
	TArray<FSkillUI> Skills;
	for (int32 i = 0; i < 6; ++i)
	{
		FSkillUI Skill;
		Skill.mSkillIndex = i;
		Skill.mName = FText::Format(
			LOCTEXT("Skill {0}", "Skill {0}"),
			FText::AsNumber(i + 1));
		Skill.mIsUsable = true;
		Skills.Add(Skill);
	}
	mUIModel->SetSkillUIs(Skills);

	// 가짜 턴
	FTurnUI Turn;
	Turn.mCurrentUnitId = 0;
	Turn.mRound = 1;
	Turn.mPhase = ECombatBuildPhaseUI::None;
	Turn.mTurnOrderUnitIds = { 0, 1, 2 };
	mUIModel->SetTurnUI(Turn);

	// 가짜 장비 3슬롯
	TArray<FEquipmentUI> Equipment;
	for (int32 i = 0; i < 3; ++i)
	{
		FEquipmentUI Equip;
		Equip.mSlotIndex = i;
		Equip.mName = FText::Format(
			LOCTEXT("Equip {0}", "Equip {0}"),
			FText::AsNumber(i + 1));
		Equip.mIsEquipped = (i == 0);
		Equipment.Add(Equip);
	}
	mUIModel->SetEquipmentUIs(Equipment);

	// 가짜 플레이어 메타(돈/경험치/레벨)
	FPlayerMetaUI Meta;
	Meta.mGold = 120;
	Meta.mLevel = 3;
	Meta.mExp = 40.f; Meta.mMaxExp = 100.f;
	mUIModel->SetPlayerMeta(Meta);
}

/** @brief UIModel 입력 의도를 mock 상태 변경/상세 push로 흉내낸다. */
void UMockCombatDriver::HandleCommand(ECombatInputType Type, int32 IntPayload)
{
	switch (Type)
	{
	case ECombatInputType::Cancel:
		break;

	case ECombatInputType::LongPressUnit:
		if (mUIModel != nullptr)
		{
			// HP/스탯은 FUnitDetailUI가 더 이상 보관하지 않는다 — UI가 mUnitId로 FUnitUI를 찾아 읽는다.
			FUnitDetailUI Detail;
			Detail.mUnitId = IntPayload;
			Detail.mName = FText::Format(
				LOCTEXT("Enemy {0}", "Enemy {0}"),
				FText::AsNumber(IntPayload));
			Detail.mLevel = 1;
			Detail.mPassiveDescriptions = { LOCTEXT("(mock passive)", "(mock passive)") };
			mUIModel->SetUnitDetail(Detail);
		}
		break;

	default:
		UE_LOG(LogRD, Display, TEXT("MockCombatDriver: command %d (payload %d)"), static_cast<int32>(Type), IntPayload);
		break;
	}
}

/** @brief 월드 터치 계약이 도달했음을 로그로 확인한다. mock은 타일 변환을 수행하지 않는다. */
void UMockCombatDriver::HandleWorldTouch(FVector2D ScreenPosition, bool bLongPress)
{
	UE_LOG(LogRD, Display, TEXT("MockCombatDriver: world touch (%.0f,%.0f) long=%d"), ScreenPosition.X, ScreenPosition.Y, bLongPress ? 1 : 0);
}

/** @brief 데미지/상태이상/힐 큐 노드 fixture를 push해 WBP 큐 재생을 검증한다. */
void UMockCombatDriver::PushMockPreview()
{
	if (mUIModel == nullptr)
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
	Status.mLabel = LOCTEXT("Stun", "Stun");
	Queue.Add(Status);

	FCombatQueueNode Heal;
	Heal.mSourceUnitId = 0; Heal.mTargetUnitId = 0; Heal.mAmount = 3;
	Heal.mLabel = FText::FromString(TEXT("+3"));
	Queue.Add(Heal);

	mUIModel->SetActionQueue(Queue);
}

/** @brief UIModel의 큐 앞 노드 하나를 resolved 처리해 연속 재생 UI를 검증한다. */
void UMockCombatDriver::PlayNextQueueNode()
{
	if (mUIModel != nullptr)
	{
		mUIModel->ResolveFrontQueueNode();
	}
}

#undef LOCTEXT_NAMESPACE
