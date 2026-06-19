#include "Pawn/Enemy/EnemyUnit.h"

#include "Setting/GameTeamType.h"

AEnemyUnit::AEnemyUnit()
{
	SetGenericTeamId(EGameTeamType::Enemy);
}

int32 AEnemyUnit::GetDifficulty() const
{
	return mDifficulty;
}

UUserWidget* AEnemyUnit::GetInfoPanel() const
{
	return nullptr;
}
