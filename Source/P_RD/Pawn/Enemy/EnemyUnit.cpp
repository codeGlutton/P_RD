#include "Pawn/Enemy/EnemyUnit.h"

int32 AEnemyUnit::GetDifficulty() const
{
	return mDifficulty;
}

UUserWidget* AEnemyUnit::GetInfoPanel() const
{
	return nullptr;
}
