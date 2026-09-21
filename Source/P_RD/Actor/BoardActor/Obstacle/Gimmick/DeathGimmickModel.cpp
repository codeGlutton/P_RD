#include "Actor/BoardActor/Obstacle/Gimmick/DeathGimmickModel.h"
#include "Setting/GameTeamType.h"

UDeathGimmickModel::UDeathGimmickModel()
{
	mTeamId = EGameTeamType::AllHostile;

	// 사망 시 1회 발동
	mRemainingTriggerCount = 1;
}

bool UDeathGimmickModel::IsTargetable() const
{
	// 살아있는 동안 스킬의 조준 및 피격 대상이 됨
	return Super::IsTargetable();
}

void UDeathGimmickModel::OnPostDead()
{
	Super::OnPostDead();

	// 사망 시 자기 타일을 조준해 기믹 발동
	TryTriggerGimmick(GetTileTransform().mIndex);
}

bool UDeathGimmickModel::CanTriggerGimmick() const
{
	// 수명을 다 썼으면 발동 불가
	if (mRemainingTriggerCount == 0)
	{
		return false;
	}

	// 사망 상태인데 사망 발동이 허용되지 않은 경우 발동 불가
	if (IsDead() == true && mCanTriggerWhenDead == false)
	{
		return false;
	}

	return true;
}