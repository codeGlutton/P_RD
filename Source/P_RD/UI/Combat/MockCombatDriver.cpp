#include "UI/Combat/MockCombatDriver.h"

#include "UI/Combat/CombatUIModel.h"
#include "Engine/Texture2D.h"

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

	// 가짜 유닛: 용병 3명 + 적 2. 배치안 목업과 같은 상황을 쓴다 --
	// 이미지로 평가한 안과 실제 WBP가 같은 수치로 그려져야 비교가 된다.
	TArray<FUnitUI> Units;
	{
		struct FMockAlly
		{
			const TCHAR* Name;
			float HP;
			int32 AP;
			float Speed;
			const TCHAR* TurnPortraitPath;
			const TCHAR* HeroPortraitPath;
		};
		const FMockAlly Allies[] = {
			{ TEXT("기사"), 90.f, 3, 5.f,
				TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Knight.T_MB_HireIcon_Knight"),
				TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireHero_Knight.T_MB_HireHero_Knight") },
			{ TEXT("궁수"), 100.f, 4, 7.f,
				TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Ranger.T_MB_HireIcon_Ranger"),
				TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireHero_Ranger.T_MB_HireHero_Ranger") },
			{ TEXT("마법사"), 75.f, 4, 4.f,
				TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Mage.T_MB_HireIcon_Mage"),
				TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireHero_Mage.T_MB_HireHero_Mage") },
		};
		for (int32 i = 0; i < UE_ARRAY_COUNT(Allies); ++i)
		{
			FUnitUI Ally;
			Ally.mUnitId = i;
			Ally.mIsPlayer = true;
			Ally.mName = FText::FromString(Allies[i].Name);
			Ally.mHP = Allies[i].HP; Ally.mMaxHP = 100.f;
			Ally.mActionPoints = Allies[i].AP; Ally.mMaxActionPoints = 4;
			Ally.mSpeedPoint = Allies[i].Speed;
			Ally.mTurnPortrait = LoadObject<UTexture2D>(nullptr,
				Allies[i].TurnPortraitPath);
			Ally.mPortrait = LoadObject<UTexture2D>(nullptr,
				Allies[i].HeroPortraitPath);
			Ally.mMovementPoint = Allies[i].AP; Ally.mMaxMovementPoint = 4.f;
			Ally.mDamagePoint = 5.f; Ally.mDefensePoint = 2.f;
			Ally.mTile = FTileIndex(4 + i, 8);
			// 목업의 기사에는 상태이상 표가 붙어 있다. 미리보기에도 하나는
			// 있어야 그 자리가 비었는지 아닌지를 배치안마다 볼 수 있다.
			if (i == 0)
			{
				FStatusEffectUI Debuff;
				Debuff.mTag = FGameplayTag::RequestGameplayTag(TEXT(
					"GameplayEffect.StatusEffect.TurnDuration.Debuff.Weakness"));
				Debuff.mStackCount = 2;
				Ally.mStatusEffects.Add(Debuff);
			}
			Units.Add(Ally);
		}

		struct FMockFoe
		{
			const TCHAR* Name;
			float HP;
			float MaxHP;
			const TCHAR* TurnPortraitPath;
		};
		const FMockFoe Foes[] = {
			{ TEXT("독수리"), 50.f, 50.f,
				TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Portraits/KK_Face_Enemy_Eagle_HeadV2.KK_Face_Enemy_Eagle_HeadV2") },
			{ TEXT("독수리"), 38.f, 50.f,
				TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Portraits/KK_Face_Enemy_Eagle_HeadV2.KK_Face_Enemy_Eagle_HeadV2") },
		};
		for (int32 i = 0; i < UE_ARRAY_COUNT(Foes); ++i)
		{
			FUnitUI Foe;
			Foe.mUnitId = 100 + i;
			Foe.mIsPlayer = false;
			Foe.mName = FText::FromString(Foes[i].Name);
			Foe.mHP = Foes[i].HP; Foe.mMaxHP = Foes[i].MaxHP;
			Foe.mActionPoints = 4; Foe.mMaxActionPoints = 4;
			Foe.mSpeedPoint = 3.f + i;
			Foe.mTurnPortrait = LoadObject<UTexture2D>(nullptr,
				Foes[i].TurnPortraitPath);
			Foe.mMovementPoint = 4.f; Foe.mMaxMovementPoint = 4.f;
			Foe.mDamagePoint = 4.f; Foe.mDefensePoint = 0.f;
			Foe.mTile = FTileIndex(3 + i, 3);
			// 적 머리 위 바도 상태 슬롯을 반드시 검증한다. 첫 적은 취약,
			// 둘째 적은 민첩을 넣어 아군/적 공통 표시 계약을 고정한다.
			FStatusEffectUI Status;
			Status.mTag = FGameplayTag::RequestGameplayTag(i == 0
				? TEXT("GameplayEffect.StatusEffect.TurnDuration.Debuff.Vulnerability")
				: TEXT("GameplayEffect.StatusEffect.TurnDuration.Buff.Agility"));
			Status.mStackCount = i == 0 ? 2 : 1;
			Foe.mStatusEffects.Add(Status);
			Units.Add(Foe);
		}
	}
	mUIModel->SetUnitUIs(Units);

	// 가짜 스킬 5개. 돌파 베기는 쿨타임 중이라 사용 불가 상태를 만든다 --
	// 비활성 표현이 배치안 평가의 핵심 항목이다.
	//
	// 다섯 개인 이유: 배치안들은 이동 + 스킬 5개로 여섯 칸을 잡는다. 네 개만
	// 채우면 마지막 칸이 비어서, 가운데 정렬한 레일이 한쪽으로 쏠린 것처럼
	// 보이고 방사형 배치는 원이 이가 빠진 것처럼 보인다. 배치를 비교하려는데
	// 데이터가 모자라 생긴 구멍을 배치 결함으로 읽게 된다.
	TArray<FSkillUI> Skills;
	{
		struct FMockSkill
		{
			const TCHAR* Name; int32 Cost; int32 Cooldown; int32 Remaining;
			int32 DamageMin; int32 DamageMax; bool bUsable;
		};
		const FMockSkill Table[] = {
			{ TEXT("평타"),      1, 0, 0,  4,  7, true  },
			{ TEXT("방패 강타"), 2, 2, 0,  8, 14, true  },
			{ TEXT("고정 참격"), 1, 0, 0,  5,  9, true  },
			{ TEXT("돌파 베기"), 2, 3, 3, 12, 18, false },
			{ TEXT("반격 태세"), 1, 1, 0,  0,  0, true  },
		};
		const TCHAR* SkillIconPaths[] = {
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/HUD04/KK_HUD04_action_left_upper__action_icon.KK_HUD04_action_left_upper__action_icon"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/HUD04/KK_HUD04_action_right_upper__action_icon.KK_HUD04_action_right_upper__action_icon"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/HUD04/KK_HUD04_action_left_lower__action_icon.KK_HUD04_action_left_lower__action_icon"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/HUD04/KK_HUD04_action_right_lower__action_icon.KK_HUD04_action_right_lower__action_icon"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/HUD04/KK_HUD04_action_bottom__action_icon.KK_HUD04_action_bottom__action_icon"),
		};
		for (int32 i = 0; i < UE_ARRAY_COUNT(Table); ++i)
		{
			FSkillUI Skill;
			Skill.mSkillIndex = i;
			Skill.mName = FText::FromString(Table[i].Name);
			Skill.mActionPointCost = Table[i].Cost;
			Skill.mCooldownTurns = Table[i].Cooldown;
			Skill.mRemainingCooldown = Table[i].Remaining;
			Skill.mDamageMin = Table[i].DamageMin;
			Skill.mDamageMax = Table[i].DamageMax;
			Skill.mCriticalDamage = FMath::RoundToInt(Table[i].DamageMax * 1.5f);
			Skill.mIsUsable = Table[i].bUsable;
			Skill.mIcon = LoadObject<UTexture2D>(nullptr, SkillIconPaths[i]);
			Skills.Add(Skill);
		}
	}
	mUIModel->SetSkillUIs(Skills);
	// 방패 강타를 고른 상태로 둔다. 선택 강조가 어떻게 읽히는지가 평가
	// 항목이고, 10안은 아예 "조준 중"을 그리는 배치다.
	mUIModel->SetSelectedSkill(1);
	FCombatPendingActionUI PendingAction;
	PendingAction.mType = ECombatPendingActionType::Skill;
	PendingAction.mActionPointCost = Skills[1].mActionPointCost;
	mUIModel->SetPendingAction(PendingAction);

	// 가짜 턴
	FTurnUI Turn;
	Turn.mCurrentUnitId = 0;
	Turn.mRound = 1;
	Turn.mPhase = ECombatBuildPhaseUI::None;
	// 기사 -> 독수리 -> 궁수 -> 독수리 -> 마법사. 목업 열 장이 그리는
	// 상황과 같아야 나란히 놓고 비교가 된다.
	Turn.mTurnOrderUnitIds = { 0, 100, 1, 101, 2 };
	Turn.mCurrentRoundRemainingTurnCount = Turn.mTurnOrderUnitIds.Num();
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
	const TCHAR* ArtifactPaths[] = {
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MapNode/T_MapNode_Treasure.T_MapNode_Treasure"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MapNode/T_MapNode_RareTreasure.T_MapNode_RareTreasure"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MapNode/T_MapNode_EpicTreasure.T_MapNode_EpicTreasure"),
	};
	const FLinearColor ArtifactColors[] = {
		FLinearColor(0.86f, 0.98f, 0.94f, 1.f),
		FLinearColor(0.55f, 0.72f, 1.f, 1.f),
		FLinearColor(0.82f, 0.58f, 1.f, 1.f),
	};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(ArtifactPaths); ++Index)
	{
		FCombatArtifactUI& Artifact = Meta.mArtifacts.AddDefaulted_GetRef();
		Artifact.mName = FText::Format(
			LOCTEXT("PreviewArtifactName", "아티팩트 {0}"),
			FText::AsNumber(Index + 1));
		Artifact.mIcon = LoadObject<UTexture2D>(nullptr, ArtifactPaths[Index]);
		Artifact.mRarityColor = ArtifactColors[Index];
	}
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
