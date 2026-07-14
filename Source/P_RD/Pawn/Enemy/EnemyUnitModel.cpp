/*****************************************************************//**
 * @file   EnemyUnitModel.cpp
 * @brief  적 베이스 유닛 모델 구현
 * @author 이문환
 * @date   2026-07-01
 *********************************************************************/

#include "Pawn/Enemy/EnemyUnitModel.h"
#include "Setting/GameTeamType.h"
#include "AttributeSet/UnitAttributeSet.h"

#include "Component/SkillComponent/SkillComponentModel.h"
#include "DataAsset/UnitSpawnData/StaticEnemyUnitSpawnData.h"
#include "DataAsset/SkillData/StaticSkillData.h"

#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "TAS/Effect/Stat/TacticalEffect_Movement.h"
#include "TAS/Effect/Stat/TacticalEffect_MovementFactor_AddBase.h"
#include "TAS/Effect/TacticalEffectContext.h"

UEnemyUnitModel::UEnemyUnitModel()
{
	UUnitModel::SetGenericTeamId(EGameTeamType::Enemy);

	// 적 스탯 세트 생성 — 속성 컴포넌트가 자식 AttributeSet을 자동 수집해 스탯 커브 초기화 대상이 된다(플레이어와 동일 패턴).
	mUnitAttributeSet = CreateDefaultSubobject<UEnemyUnitAttributeSet>(TEXT("EnemyUnitAttributeSet"));
}

void UEnemyUnitModel::PostInitializeComponentModels()
{
	Super::PostInitializeComponentModels();

	// 스폰 데이터 획득
	const UStaticEnemyUnitSpawnData* EnemySpawn = Cast<UStaticEnemyUnitSpawnData>(mStaticSpawnData);
	if (EnemySpawn == nullptr)
	{
		return;
	}

	mMoveTendency = EnemySpawn->mMoveTendency;

	if (USkillComponentModel* SkillComp = GetSkillComponentModel())
	{
		// 스폰 데이터의 스킬을 로드해서 슬롯 0부터 순서대로 장착
		int32 SkillIndex = 0;
		for (const TSoftObjectPtr<UStaticSkillData>& SkillSoft : EnemySpawn->mSkillDatas)
		{
			// 로드 성공한 스킬만 슬롯에 세팅
			if (UStaticSkillData* Loaded = SkillSoft.LoadSynchronous())
			{
				SkillComp->SetSkill(SkillIndex++, Loaded);
			}
		}
	}
}

void UEnemyUnitModel::OnBeginTurn()
{
	Super::OnBeginTurn();

	/* 자기 자신에게 MovePoint 부여 */

	UAttributeSetComponentModel* OwingAttributeSetComponentModel = GetAttributeComponentModel();
	checkf(OwingAttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

	const int32 DefaultMovePoint = FMath::Max(
		OwingAttributeSetComponentModel->GetAttributeCurrentValue(UEnemyUnitAttributeSet::GetRechargeMovementAttribute()), 
		0
	);

	UTacticalEffectContext* EffectContext = OwingAttributeSetComponentModel->MakeEffectContext();
	EffectContext->SetInstigator(this);
	EffectContext->SetAttributeSetComponentModel(OwingAttributeSetComponentModel);

	FActiveTacticalEffectHandle FactorHandle;
	{
		/* 기본 Move 만큼 Factor 부여 */

		TSharedPtr<FTacticalEffectSpec> EffectSpec = OwingAttributeSetComponentModel->MakeOutgoingSpec(UTacticalEffect_MovementFactor_AddBase::StaticClass(), EffectContext);
		EffectSpec->mDynamicMagnitude = DefaultMovePoint;
		FactorHandle = OwingAttributeSetComponentModel->ApplyTacticalEffectSpecToSelf(*EffectSpec);
	}

	{
		/* MovePoint 습득 */

		UBoardCombatTargetSnapshotData* OwingSnapshot = MakeSnapshotData();
		TSharedPtr<FTacticalEffectSpec> EffectSpec = OwingAttributeSetComponentModel->MakeOutgoingSpec(UTacticalEffect_GetMovement::StaticClass(), EffectContext);
		EffectSpec->SetInstigatorSnapshotData(OwingSnapshot);
		EffectSpec->SetTargetSnapshotData(OwingSnapshot);
		OwingAttributeSetComponentModel->ApplyTacticalEffectSpecToSelf(*EffectSpec);
	}

	{
		/* 기본 Move 만큼 Factor 제거 */

		OwingAttributeSetComponentModel->RemoveActiveTacticalEffect(FactorHandle);
	}
}

int32 UEnemyUnitModel::GetBoardActorLevel() const
{
	return GetDifficulty();
}

int32 UEnemyUnitModel::GetDifficulty() const
{
	return mDifficulty;
}

EMoveTendency UEnemyUnitModel::GetMoveTendency() const
{
	return mMoveTendency;
}

