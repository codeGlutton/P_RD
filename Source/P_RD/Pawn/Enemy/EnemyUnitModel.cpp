#include "Pawn/Enemy/EnemyUnitModel.h"
#include "Setting/GameTeamType.h"

UEnemyUnitModel::UEnemyUnitModel()
{
	SetGenericTeamId(EGameTeamType::Enemy);
}

int32 UEnemyUnitModel::GetDifficulty() const
{
	return mDifficulty;
}
