#include "Setting/UnitTeamSettings.h"

FUnitTeamRelation::FUnitTeamRelation()
{
    for (int32 i = 0; i < EUnitTeamType::Count; ++i)
    {
        mAttitudes.Add(static_cast<EUnitTeamType::Type>(i), ETeamAttitude::Neutral);
    }

    mAttitudes[EUnitTeamType::AllNeutral] = ETeamAttitude::Neutral;
    mAttitudes[EUnitTeamType::AllHostile] = ETeamAttitude::Hostile;
}

UUnitTeamSettings::UUnitTeamSettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    // 기본 팀 파악용 Solver로 등록
    FGenericTeamId::SetAttitudeSolver(&UUnitTeamSettings::GetAttitude);
}

FName UUnitTeamSettings::GetCategoryName() const
{
    return FName(TEXT("Game"));
}

#if WITH_EDITOR
FText UUnitTeamSettings::GetSectionText() const
{
    return FText::FromString(TEXT("Team Setting"));
}

FText UUnitTeamSettings::GetSectionDescription() const
{
    return FText::FromString(TEXT("Set up relationships between teams so that AI can recognize relationships"));
}
#endif

ETeamAttitude::Type UUnitTeamSettings::GetAttitude(FGenericTeamId OwnId, FGenericTeamId OtherId)
{
    const auto& TeamRelations = GetDefault<UUnitTeamSettings>()->mTeamRelations;
    const auto* OwnTeamRelation = TeamRelations.Find(StaticCast<TEnumAsByte<EUnitTeamType::Type>>(OwnId.GetId()));
    if (OwnTeamRelation != nullptr)
    {
        const auto* AttitudeToOther = OwnTeamRelation->mAttitudes.Find(StaticCast<TEnumAsByte<EUnitTeamType::Type>>(OtherId.GetId()));
        if (AttitudeToOther != nullptr)
        {
            return *AttitudeToOther;
        }
    }
    return ETeamAttitude::Neutral;
}


