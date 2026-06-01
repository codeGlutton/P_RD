#include "Setting/GamePlaySettings.h"

#include "UI/MsgNotifyWidget.h"
#include "UI/SaveNotifyWidget.h"
#include "UI/TopMenuBarWidget.h"

UGamePlaySettings::UGamePlaySettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    // 기본 팀 파악용 Solver로 등록
    FGenericTeamId::SetAttitudeSolver(&UGamePlaySettings::GetAttitude);

    mWorldWidgetClasses[StaticCast<uint8>(EWorldWidgetType::TopMenuBar)] = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/WBP_TopMenuBar.WBP_TopMenuBar_C"));
    if (mWorldWidgetClasses[StaticCast<uint8>(EWorldWidgetType::TopMenuBar)] == nullptr)
    {
        mWorldWidgetClasses[StaticCast<uint8>(EWorldWidgetType::TopMenuBar)] = UTopMenuBarWidget::StaticClass();
    }

    mWorldWidgetClasses[StaticCast<uint8>(EWorldWidgetType::MsgNotify)] = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/WBP_MsgNotify.WBP_MsgNotify_C"));
    if (mWorldWidgetClasses[StaticCast<uint8>(EWorldWidgetType::MsgNotify)] == nullptr)
    {
        mWorldWidgetClasses[StaticCast<uint8>(EWorldWidgetType::MsgNotify)] = UMsgNotifyWidget::StaticClass();
    }

    mWorldWidgetClasses[StaticCast<uint8>(EWorldWidgetType::SaveNotify)] = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/WBP_SaveNotify.WBP_SaveNotify_C"));
    if (mWorldWidgetClasses[StaticCast<uint8>(EWorldWidgetType::SaveNotify)] == nullptr)
    {
        mWorldWidgetClasses[StaticCast<uint8>(EWorldWidgetType::SaveNotify)] = USaveNotifyWidget::StaticClass();
    }
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


