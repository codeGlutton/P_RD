#include "Pawn/Unit.h"
#include "Pawn/UnitModel.h"

UObjectModel* AUnit::GetModel_Internal() const
{
	return mUnitModel.Get();
}
