#include "Setting/SRPGFrameworkSettings.h"

FName USRPGFrameworkSettings::GetCategoryName() const
{
    return FName(TEXT("Game"));
}

#if WITH_EDITOR
FText USRPGFrameworkSettings::GetSectionText() const
{
    return FText::FromString(TEXT("SRPG Framework Settings"));
}

FText USRPGFrameworkSettings::GetSectionDescription() const
{
    return FText::FromString(TEXT("Set up settings used in the SRPG framework"));
}
#endif
