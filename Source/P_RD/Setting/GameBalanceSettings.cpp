#include "Setting/GameBalanceSettings.h"

FName UGameBalanceSettings::GetCategoryName() const
{
    return FName(TEXT("Game"));
}

#if WITH_EDITOR
FText UGameBalanceSettings::GetSectionText() const
{
    return FText::FromString(TEXT("Global Game Balance Settings"));
}

FText UGameBalanceSettings::GetSectionDescription() const
{
    return FText::FromString(TEXT("Set up settings for the RD global game balance"));
}
#endif
