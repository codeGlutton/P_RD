#include "Setting/UnitTeamType.h"

FUnitTeamRelation::FUnitTeamRelation()
{
    for (int32 i = 0; i < EUnitTeamType::Count; ++i)
    {
        mAttitudes.Add(static_cast<EUnitTeamType::Type>(i), ETeamAttitude::Neutral);
    }

    mAttitudes[EUnitTeamType::AllNeutral] = ETeamAttitude::Neutral;
    mAttitudes[EUnitTeamType::AllHostile] = ETeamAttitude::Hostile;
}