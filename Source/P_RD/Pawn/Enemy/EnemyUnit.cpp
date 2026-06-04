#include "Pawn/Enemy/EnemyUnit.h"

#include "Setting/UnitTeamType.h"

AEnemyUnit::AEnemyUnit()
{
	SetGenericTeamId(EUnitTeamType::Enemy);
}

int32 AEnemyUnit::GetDifficulty() const
{
	return mDifficulty;
}

UUserWidget* AEnemyUnit::GetInfoPanel() const
{
	return nullptr;
}
