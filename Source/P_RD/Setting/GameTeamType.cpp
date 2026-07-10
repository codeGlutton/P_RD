#include "Setting/GameTeamType.h"

FGameTeamRelation::FGameTeamRelation()
{
    for (int32 i = 0; i < EGameTeamType::Count; ++i)
    {
        mAttitudes.Add(static_cast<EGameTeamType::Type>(i), ETeamAttitude::Neutral);
    }

    mAttitudes[EGameTeamType::AllNeutral] = ETeamAttitude::Neutral;
    mAttitudes[EGameTeamType::AllHostile] = ETeamAttitude::Hostile;
}