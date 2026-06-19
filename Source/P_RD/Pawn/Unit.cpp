#include "Pawn/Unit.h"
#include "Pawn/Unit/UnitModel.h"

UObjectModel* AUnit::GetModel_Internal() const
{
	return mUnitModel;
}
