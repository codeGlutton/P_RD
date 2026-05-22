#include "Setting/AssetNamingConventionSettings.h"

FName UAssetNamingConventionSettings::GetCategoryName() const
{
	return FName(TEXT("Convention"));
}

#if WITH_EDITOR

FText UAssetNamingConventionSettings::GetSectionText() const
{
	return FText::FromString(TEXT("Asset Naming Convention"));
}

FText UAssetNamingConventionSettings::GetSectionDescription() const
{
	return FText::FromString(TEXT("Settings to maintain the same asset naming convention among team members"));
}

void UAssetNamingConventionSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    mNamingRuleSets.StableSort([](const FAssetNamingRuleSet& Lhs, const FAssetNamingRuleSet& Rhs) {
            return Lhs.mPriority < Rhs.mPriority;
        });
    SaveConfig();

    Super::PostEditChangeProperty(PropertyChangedEvent);
}

#endif

