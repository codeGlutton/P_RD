#include "Pawn/UnitModel.h"
#include "Setting/GameTeamType.h"

#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Component/SkillComponent/SkillComponentModel.h"
#include "Component/PassiveComponent/PassiveComponentModel.h"
#include "Component/EquipmentComponent/EquipmentComponentModel.h"

#include "DataAsset/UnitSpawnData/StaticUnitSpawnData.h"

#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"
#include "Singleton/WorldSubsystem/TacticalFrameworkSubsystem.h"

#include "TAS/Passive/TacticalPassive.h"
#include "TAS/Passive/PassiveActivateContext.h"
#include "TAS/Passive/DynamicPassiveData.h"

#include "AttributeSet/UnitAttributeSet.h"

UUnitModel::UUnitModel() : mTeamId(EGameTeamType::AllNeutral)
{
	mAttributeCompModel = CreateDefaultSubobject<UAttributeSetComponentModel>(TEXT("AttributeSetComponentModel"));
	mSkillCompModel = CreateDefaultSubobject<USkillComponentModel>(TEXT("SkillComponentModel"));
	mPassiveCompModel = CreateDefaultSubobject<UPassiveComponentModel>(TEXT("PassiveComponentModel"));
	mEquipmentCompModel = CreateDefaultSubobject<UEquipmentComponentModel>(TEXT("EquipmentComponentModel"));

	mTileLayerFlags = StaticCast<int32>(ETileLayerFlag::Unit);
	mBlockLayerFlags = StaticCast<int32>(ETileLayerFlag::Unit | ETileLayerFlag::Obstacle);
	mReplaceLayerFlags = StaticCast<int32>(ETileLayerFlag::None);
	mOverlayLayerPriority = 0;
}

void UUnitModel::PostInitializeComponentModels()
{
	Super::PostInitializeComponentModels();

	UTacticalFrameworkSubsystem* TacticalFrameworkSubsystem = GetWorld()->GetSubsystem<UTacticalFrameworkSubsystem>();
	checkf(TacticalFrameworkSubsystem != nullptr, TEXT("전략 프레임워크 서브시스템 nullptr"));

	UTacticalFrameworkModel* TacticalFrameworkModel = TacticalFrameworkSubsystem->GetModel<UTacticalFrameworkModel>();
	checkf(TacticalFrameworkModel != nullptr, TEXT("전략 프레임워크 모델 nullptr"));

	TacticalFrameworkModel->GetAttributeSetInitter()->InitAttributeSetDefaults(GetAttributeComponentModel(), GetBoardActorKeyName(), GetDifficulty(), true);

	// 스폰 데이터에 지정된 장비를 일괄 장착
	if (UStaticUnitSpawnData* UnitSpawn = Cast<UStaticUnitSpawnData>(mStaticSpawnData))
	{
		GetEquipmentComponentModel()->EquipFrom(UnitSpawn->mEquipmentDatas);
	}
}

void UUnitModel::OnBeginRoom()
{
	Super::OnBeginRoom();

	// 전투 시작 패시브 처리
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

	// 전투 종료 패시브 처리
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

	// 모두 제거
	mAttributeCompModel->ApplyModToAttribute(UCombatTargetAttributeSet::GetDefenseAttribute(), ETacticalModOp::Override, 0.f);
	mAttributeCompModel->ApplyModToAttribute(UCombatTargetAttributeSet::GetMovementAttribute(), ETacticalModOp::Override, 0.f);
	mAttributeCompModel->RemoveLooseGameplayTagsMatchingTag(EffectTags::GameplayEffect_StatusEffect, INT_MAX);
}

void UUnitModel::OnBeginTurn()
{
	// 방어도 제거
	mAttributeCompModel->ApplyModToAttribute(UCombatTargetAttributeSet::GetDefenseAttribute(), ETacticalModOp::Override, 0.f);

	// 턴 시작 패시브 처리
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
}

void UUnitModel::OnEndTurn()
{
	// 턴 종료 패시브 처리
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

	// 이동력 제거
	mAttributeCompModel->ApplyModToAttribute(UCombatTargetAttributeSet::GetMovementAttribute(), ETacticalModOp::Override, 0.f);
	// 턴제 상태이상 한 스택 감소
	mAttributeCompModel->RemoveLooseGameplayTagsMatchingTag(EffectTags::GameplayEffect_StatusEffect_TurnDuration, 1);
}

void UUnitModel::SetGenericTeamId(const FGenericTeamId& TeamID)
{
	mTeamId = TeamID;
}

FGenericTeamId UUnitModel::GetGenericTeamId() const
{
	return mTeamId;
}

UAttributeSetComponentModel* UUnitModel::GetAttributeComponentModel() const
{
	return mAttributeCompModel;
}

USkillComponentModel* UUnitModel::GetSkillComponentModel() const
{
	return mSkillCompModel;
}

UPassiveComponentModel* UUnitModel::GetPassiveComponentModel() const
{
	return mPassiveCompModel;
}

UEquipmentComponentModel* UUnitModel::GetEquipmentComponentModel() const
{
	return mEquipmentCompModel;
}

