#include "Setting/GamePlaySettings.h"

UGamePlaySettings::UGamePlaySettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    // 기본 팀 파악용 Solver로 등록
    FGenericTeamId::SetAttitudeSolver(&UGamePlaySettings::GetAttitude);
}

FName UGamePlaySettings::GetCategoryName() const
{
    return FName(TEXT("Game"));
}

#if WITH_EDITOR
FText UGamePlaySettings::GetSectionText() const
{
    return FText::FromString(TEXT("Game Play Setting"));
}

FText UGamePlaySettings::GetSectionDescription() const
{
    return FText::FromString(TEXT("Set up settings related to gameplay"));
}
#endif

ETeamAttitude::Type UGamePlaySettings::GetAttitude(FGenericTeamId OwnId, FGenericTeamId OtherId)
{
    const auto& TeamRelations = GetDefault<UGamePlaySettings>()->mTeamRelations;
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


