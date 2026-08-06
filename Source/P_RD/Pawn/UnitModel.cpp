#include "Pawn/UnitModel.h"
#include "Setting/GameTeamType.h"

#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Component/SkillComponent/UnitSkillComponentModel.h"
#include "Component/PassiveComponent/PassiveComponentModel.h"

#include "DataAsset/UnitSpawnData/StaticUnitSpawnData.h"

#include "TAS/Passive/TacticalPassive.h"
#include "TAS/Passive/PassiveActivateContext.h"
#include "TAS/Passive/DynamicPassiveData.h"

#include "AttributeSet/UnitAttributeSet.h"
#include "TAS/Effect/Stat/TacticalEffect_ActionPoint.h"
#include "TAS/Effect/Stat/TacticalEffect_ActionPointFactor.h"
#include "TAS/Effect/Stat/TacticalEffect_SpeedPoint.h"
#include "TAS/Effect/Stat/TacticalEffect_SpeedPointFactor.h"
#include "TAS/Effect/TacticalEffectContext.h"

UUnitModel::UUnitModel() : mTeamId(EGameTeamType::AllNeutral)
{
	mAttributeCompModel = CreateDefaultSubobject<UAttributeSetComponentModel>(TEXT("AttributeSetComponentModel"));
	mSkillCompModel = CreateDefaultSubobject<UUnitSkillComponentModel>(TEXT("UnitSkillComponentModel"));
	mPassiveCompModel = CreateDefaultSubobject<UPassiveComponentModel>(TEXT("PassiveComponentModel"));

	mTileLayerFlags = StaticCast<int32>(ETileLayerFlag::Unit);
	mBlockLayerFlags = StaticCast<int32>(ETileLayerFlag::Unit | ETileLayerFlag::Obstacle);
	mReplaceLayerFlags = StaticCast<int32>(ETileLayerFlag::None);
	mOverlayLayerPriority = 0;
}

void UUnitModel::OnBeginRoom()
{
	Super::OnBeginRoom();

	/* 전투 시작 패시브 처리 */

	TArray<UTacticalPassive*> Passives = mPassiveCompModel->GetPassivesByTiming(AbilityTags::GameplayAbility_Passive_OnStartRoom);

	UBoardCombatTargetSnapshotData* UnitSnapshot = MakeSnapshotData();

	FPassiveActivateContext PassiveContext;
	PassiveContext.mOwner = this;
	PassiveContext.mOwnerSnapshot = UnitSnapshot;
	PassiveContext.mTargets.Add(this);
	PassiveContext.mTargetSnapshots.Add(UnitSnapshot);

	for (UTacticalPassive*& Passive : Passives)
	{
		TInstancedStruct<FDynamicPassiveData> DynamicPassiveData;
		Passive->ActivatePassive(AbilityTags::GameplayAbility_Passive_OnStartRoom, PassiveContext, OUT DynamicPassiveData);
		Passive->CommitPassive(DynamicPassiveData);
	}
}

void UUnitModel::OnEndRoom()
{
	Super::OnEndRoom();

	/* 전투 종료 패시브 처리 */

	TArray<UTacticalPassive*> Passives = mPassiveCompModel->GetPassivesByTiming(AbilityTags::GameplayAbility_Passive_OnEndRoom);

	UBoardCombatTargetSnapshotData* UnitSnapshot = MakeSnapshotData();

	FPassiveActivateContext PassiveContext;
	PassiveContext.mOwner = this;
	PassiveContext.mOwnerSnapshot = UnitSnapshot;
	PassiveContext.mTargets.Add(this);
	PassiveContext.mTargetSnapshots.Add(UnitSnapshot);

	for (UTacticalPassive*& Passive : Passives)
	{
		TInstancedStruct<FDynamicPassiveData> DynamicPassiveData;
		Passive->ActivatePassive(AbilityTags::GameplayAbility_Passive_OnEndRoom, PassiveContext, OUT DynamicPassiveData);
		Passive->CommitPassive(DynamicPassiveData);
	}

	/* 모두 제거 */

	mAttributeCompModel->ApplyModToAttribute(UUnitAttributeSet::GetDefenseAttribute(), ETacticalModOp::Override, 0.f);
	mAttributeCompModel->ApplyModToAttribute(UUnitAttributeSet::GetActionPointAttribute(), ETacticalModOp::Override, 0.f);
	mAttributeCompModel->RemoveLooseGameplayTagsMatchingTag(EffectTags::GameplayEffect_StatusEffect, INT_MAX);
}

void UUnitModel::OnBeginRound(int32 RoundCount)
{
	Super::OnBeginRound(RoundCount);

	/* 자기 자신에게 SpeedPoint 부여 */

	const int32 DefaultSpeedPoint = FMath::Max(
		mAttributeCompModel->GetAttributeCurrentValue(UUnitAttributeSet::GetRechargeSpeedPointAttribute()),
		0
	);

	UTacticalEffectContext* EffectContext = mAttributeCompModel->MakeEffectContext();

	/* 스피드 습득 */

	FActiveTacticalEffectHandle FactorHandle;
	{
		/* 기본 스피드 만큼 Factor 부여 */

		TSharedPtr<FTacticalEffectSpec> EffectSpec = mAttributeCompModel->MakeOutgoingSpec(UTacticalEffect_SpeedPointFactor_AddBase::StaticClass(), EffectContext);
		EffectSpec->mDynamicMagnitude = DefaultSpeedPoint;
		FactorHandle = mAttributeCompModel->ApplyTacticalEffectSpecToSelf(*EffectSpec);
	}

	{
		/* 스피드 습득 */

		UBoardCombatTargetSnapshotData* OwingSnapshot = MakeSnapshotData();
		TSharedPtr<FTacticalEffectSpec> EffectSpec = mAttributeCompModel->MakeOutgoingSpec(UTacticalEffect_RechargeSpeedPoint::StaticClass(), EffectContext);
		EffectSpec->SetInstigatorSnapshotData(OwingSnapshot);
		EffectSpec->SetTargetSnapshotData(OwingSnapshot);
		mAttributeCompModel->ApplyTacticalEffectSpecToSelf(*EffectSpec);
	}

	{
		/* 기본 스피드 만큼 Factor 제거 */

		mAttributeCompModel->RemoveActiveTacticalEffect(FactorHandle);
	}
}

void UUnitModel::OnBeginTurn(int32 TurnCount)
{
	/* 방어도 제거 */

	mAttributeCompModel->ApplyModToAttribute(UUnitAttributeSet::GetDefenseAttribute(), ETacticalModOp::Override, 0.f);

	/* 턴 시작 패시브 처리 */

	TArray<UTacticalPassive*> Passives = mPassiveCompModel->GetPassivesByTiming(AbilityTags::GameplayAbility_Passive_OnStartTurn);

	UBoardCombatTargetSnapshotData* OwnerSnapshot = MakeSnapshotData();

	FPassiveActivateContext PassiveContext;
	PassiveContext.mOwner = this;
	PassiveContext.mOwnerSnapshot = OwnerSnapshot;
	PassiveContext.mTargets.Add(this);
	PassiveContext.mTargetSnapshots.Add(OwnerSnapshot);

	for (UTacticalPassive*& Passive : Passives)
	{
		TInstancedStruct<FDynamicPassiveData> DynamicPassiveData;
		Passive->ActivatePassive(AbilityTags::GameplayAbility_Passive_OnStartTurn, PassiveContext, OUT DynamicPassiveData);
		Passive->CommitPassive(DynamicPassiveData);
	}

	/* 자기 자신에게 MovePoint 부여 */

	const int32 DefaultMovePoint = FMath::Max(
		mAttributeCompModel->GetAttributeCurrentValue(UUnitAttributeSet::GetRechargeActionPointAttribute()),
		0
	);

	UTacticalEffectContext* EffectContext = mAttributeCompModel->MakeEffectContext();

	FActiveTacticalEffectHandle FactorHandle;
	{
		/* 기본 행동력 만큼 Factor 부여 */

		TSharedPtr<FTacticalEffectSpec> EffectSpec = mAttributeCompModel->MakeOutgoingSpec(UTacticalEffect_ActionPointFactor_AddBase::StaticClass(), EffectContext);
		EffectSpec->mDynamicMagnitude = DefaultMovePoint;
		FactorHandle = mAttributeCompModel->ApplyTacticalEffectSpecToSelf(*EffectSpec);
	}

	{
		/* 행동력 습득 */

		UBoardCombatTargetSnapshotData* OwingSnapshot = MakeSnapshotData();
		TSharedPtr<FTacticalEffectSpec> EffectSpec = mAttributeCompModel->MakeOutgoingSpec(UTacticalEffect_RechargeActionPoint::StaticClass(), EffectContext);
		EffectSpec->SetInstigatorSnapshotData(OwingSnapshot);
		EffectSpec->SetTargetSnapshotData(OwingSnapshot);
		mAttributeCompModel->ApplyTacticalEffectSpecToSelf(*EffectSpec);
	}

	{
		/* 기본 행동력 만큼 Factor 제거 */

		mAttributeCompModel->RemoveActiveTacticalEffect(FactorHandle);
	}
}

void UUnitModel::OnEndTurn(int32 TurnCount)
{
	/* 턴 종료 패시브 처리 */

	TArray<UTacticalPassive*> Passives = mPassiveCompModel->GetPassivesByTiming(AbilityTags::GameplayAbility_Passive_OnEndTurn);

	UBoardCombatTargetSnapshotData* OwnerSnapshot = MakeSnapshotData();

	FPassiveActivateContext PassiveContext;
	PassiveContext.mOwner = this;
	PassiveContext.mOwnerSnapshot = OwnerSnapshot;
	PassiveContext.mTargets.Add(this);
	PassiveContext.mTargetSnapshots.Add(OwnerSnapshot);

	for (UTacticalPassive*& Passive : Passives)
	{
		TInstancedStruct<FDynamicPassiveData> DynamicPassiveData;
		Passive->ActivatePassive(AbilityTags::GameplayAbility_Passive_OnEndTurn, PassiveContext, OUT DynamicPassiveData);
		Passive->CommitPassive(DynamicPassiveData);
	}

	/* 행동력 제거 */

	mAttributeCompModel->ApplyModToAttribute(UUnitAttributeSet::GetActionPointAttribute(), ETacticalModOp::Override, 0.f);
	
	/* 턴제 상태이상 한 스택 감소 */

	mAttributeCompModel->RemoveLooseGameplayTagsMatchingTag(EffectTags::GameplayEffect_StatusEffect_TurnDuration, 1);
}

UAttributeSetComponentModel* UUnitModel::GetAttributeComponentModel() const
{
	return mAttributeCompModel;
}

USkillComponentModel* UUnitModel::GetSkillComponentModel() const
{
	return mSkillCompModel;
}

void UUnitModel::SetGenericTeamId(const FGenericTeamId& TeamID)
{
	mTeamId = TeamID;
}

FGenericTeamId UUnitModel::GetGenericTeamId() const
{
	return mTeamId;
}

void UUnitModel::OnStartUsingSkill(const FActiveSkillContext& Context, int32 SkillIndex)
{
	IBoardCombatTarget::OnStartUsingSkill(Context, SkillIndex);

	/* 스킬 사용 시 패시브 발동 */

	TArray<UTacticalPassive*> Passives = mPassiveCompModel->GetPassivesByTiming(AbilityTags::GameplayAbility_Passive_OnStartUsingSkill);

	UBoardCombatTargetSnapshotData* Snapshot = MakeSnapshotData();

	FPassiveActivateContext PassiveContext;
	PassiveContext.mOwner = this;
	PassiveContext.mOwnerSnapshot = Snapshot;
	PassiveContext.mTargets.Add(this);
	PassiveContext.mTargetSnapshots.Add(Snapshot);

	for (UTacticalPassive*& Passive : Passives)
	{
		TInstancedStruct<FDynamicPassiveData> DynamicPassiveData;
		Passive->ActivatePassive(AbilityTags::GameplayAbility_Passive_OnStartUsingSkill, PassiveContext, OUT DynamicPassiveData);
		Passive->CommitPassive(DynamicPassiveData);
	}
}

void UUnitModel::OnEndUsingSkill(int32 SkillIndex)
{
	IBoardCombatTarget::OnEndUsingSkill(SkillIndex);

	/* 스킬 종료 시 패시브 발동 */

	TArray<UTacticalPassive*> Passives = mPassiveCompModel->GetPassivesByTiming(AbilityTags::GameplayAbility_Passive_OnEndUsingSkill);

	UBoardCombatTargetSnapshotData* Snapshot = MakeSnapshotData();

	FPassiveActivateContext PassiveContext;
	PassiveContext.mOwner = this;
	PassiveContext.mOwnerSnapshot = Snapshot;
	PassiveContext.mTargets.Add(this);
	PassiveContext.mTargetSnapshots.Add(Snapshot);

	for (UTacticalPassive*& Passive : Passives)
	{
		TInstancedStruct<FDynamicPassiveData> DynamicPassiveData;
		Passive->ActivatePassive(AbilityTags::GameplayAbility_Passive_OnEndUsingSkill, PassiveContext, OUT DynamicPassiveData);
		Passive->CommitPassive(DynamicPassiveData);
	}
}

void UUnitModel::OnStartApplyingEffects(const FActiveSkillContext& Context, int32 PhaseIndex)
{
	IBoardCombatTarget::OnStartApplyingEffects(Context, PhaseIndex);

	/* 이펙트 가격 전 패시브 적용 */

	UBoardCombatTargetSnapshotData* Snapshot = MakeSnapshotData();

	TArray<UTacticalPassive*> Passives = mPassiveCompModel->GetPassivesByTiming(AbilityTags::GameplayAbility_Passive_OnStartApplyingEffect);

	FPassiveActivateContext PassiveContext;
	PassiveContext.mOwner = this;
	PassiveContext.mOwnerSnapshot = Snapshot;

	TArray<UBoardCombatTargetSnapshotData*> OtherSnapshots;
	OtherSnapshots.Reserve(Context.mOtherCombatTargets.Num());
	for (IBoardCombatTarget* OtherCombatTarget : Context.mOtherCombatTargets)
	{
		UBoardActorModel* OtherActorModel = Cast<UBoardActorModel>(OtherCombatTarget);
		checkf(OtherActorModel != nullptr, TEXT("스킬을 받는 타겟이 유효하지 않음"));
		OtherSnapshots.Add(OtherCombatTarget->MakeSnapshotData());

		PassiveContext.mTargets.Add(OtherActorModel);
		PassiveContext.mTargetSnapshots.Add(OtherSnapshots.Last());
	}

	for (UTacticalPassive* Passive : Passives)
	{
		TInstancedStruct<FDynamicPassiveData> DynamicPassiveData;

		Passive->ActivatePassive(AbilityTags::GameplayAbility_Passive_OnStartApplyingEffect, PassiveContext, OUT DynamicPassiveData);
		Passive->CommitPassive(DynamicPassiveData);
	}
}

void UUnitModel::OnEndApplyingEffects(const FActiveSkillContext& Context, int32 PhaseIndex)
{
	IBoardCombatTarget::OnEndApplyingEffects(Context, PhaseIndex);

	/* 이펙트 가격 후 패시브 적용 */

	UBoardCombatTargetSnapshotData* Snapshot = MakeSnapshotData();

	TArray<UTacticalPassive*> Passives = mPassiveCompModel->GetPassivesByTiming(AbilityTags::GameplayAbility_Passive_OnEndApplyingEffect);

	FPassiveActivateContext PassiveContext;
	PassiveContext.mOwner = this;
	PassiveContext.mOwnerSnapshot = Snapshot;

	TArray<UBoardCombatTargetSnapshotData*> OtherSnapshots;
	OtherSnapshots.Reserve(Context.mOtherCombatTargets.Num());
	for (IBoardCombatTarget* OtherCombatTarget : Context.mOtherCombatTargets)
	{
		UBoardActorModel* OtherActorModel = Cast<UBoardActorModel>(OtherCombatTarget);
		checkf(OtherActorModel != nullptr, TEXT("스킬을 받는 타겟이 유효하지 않음"));
		OtherSnapshots.Add(OtherCombatTarget->MakeSnapshotData());

		PassiveContext.mTargets.Add(OtherActorModel);
		PassiveContext.mTargetSnapshots.Add(OtherSnapshots.Last());
	}

	for (UTacticalPassive* Passive : Passives)
	{
		TInstancedStruct<FDynamicPassiveData> DynamicPassiveData;

		Passive->ActivatePassive(AbilityTags::GameplayAbility_Passive_OnEndApplyingEffect, PassiveContext, OUT DynamicPassiveData);
		Passive->CommitPassive(DynamicPassiveData);
	}
}

void UUnitModel::OnStartReceivingEffects(UBoardCombatTargetSnapshotData* InstigatorSnapshot, const FActiveSkillContext& Context, int32 PhaseIndex)
{
	IBoardCombatTarget::OnStartReceivingEffects(InstigatorSnapshot, Context, PhaseIndex);

	/* 이펙트 피격 전 패시브 적용 */

	TArray<UTacticalPassive*> Passives = mPassiveCompModel->GetPassivesByTiming(AbilityTags::GameplayAbility_Passive_OnStartReceivingEffect);
	UBoardCombatTargetSnapshotData* Snapshot = MakeSnapshotData();

	UBoardActorModel* InstigatorBoardActorModel = Cast<UBoardActorModel>(Context.mInstigator.GetObject());

	FPassiveActivateContext PassiveContext;
	PassiveContext.mOwner = this;
	PassiveContext.mOwnerSnapshot = Snapshot;
	PassiveContext.mTargets.Add(InstigatorBoardActorModel);
	PassiveContext.mTargetSnapshots.Add(InstigatorSnapshot);

	for (UTacticalPassive* Passive : Passives)
	{
		TInstancedStruct<FDynamicPassiveData> DynamicPassiveData;

		Passive->ActivatePassive(AbilityTags::GameplayAbility_Passive_OnStartReceivingEffect, PassiveContext, OUT DynamicPassiveData);
		Passive->CommitPassive(DynamicPassiveData);
	}
}

void UUnitModel::OnEndReceivingEffects(UBoardCombatTargetSnapshotData* InstigatorSnapshot, const FActiveSkillContext& Context, int32 PhaseIndex)
{
	IBoardCombatTarget::OnEndReceivingEffects(InstigatorSnapshot, Context, PhaseIndex);

	/* 이펙트 피격 후 패시브 적용 */

	TArray<UTacticalPassive*> Passives = mPassiveCompModel->GetPassivesByTiming(AbilityTags::GameplayAbility_Passive_OnEndReceivingEffect);
	UBoardCombatTargetSnapshotData* Snapshot = MakeSnapshotData();

	UBoardActorModel* InstigatorBoardActorModel = Cast<UBoardActorModel>(Context.mInstigator.GetObject());

	FPassiveActivateContext PassiveContext;
	PassiveContext.mOwner = this;
	PassiveContext.mOwnerSnapshot = Snapshot;
	PassiveContext.mTargets.Add(InstigatorBoardActorModel);
	PassiveContext.mTargetSnapshots.Add(InstigatorSnapshot);

	for (UTacticalPassive* Passive : Passives)
	{
		TInstancedStruct<FDynamicPassiveData> DynamicPassiveData;

		Passive->ActivatePassive(AbilityTags::GameplayAbility_Passive_OnEndReceivingEffect, PassiveContext, OUT DynamicPassiveData);
		Passive->CommitPassive(DynamicPassiveData);
	}
}

UPassiveComponentModel* UUnitModel::GetPassiveComponentModel() const
{
	return mPassiveCompModel;
}

