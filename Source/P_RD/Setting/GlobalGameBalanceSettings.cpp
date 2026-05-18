#include "Setting/GlobalGameBalanceSettings.h"

FName UGlobalGameBalanceSettings::GetCategoryName() const
{
    return FName(TEXT("Game"));
}

#if WITH_EDITOR
FText UGlobalGameBalanceSettings::GetSectionText() const
{
    return FText::FromString(TEXT("Global Game Valance Settings"));
}

FText UGlobalGameBalanceSettings::GetSectionDescription() const
{
    return FText::FromString(TEXT("Set up settings for the RD global game valance"));
}
#endif
