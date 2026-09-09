#pragma once

#include "DataAsset/PrimaryAssetType.h"

// Historical fixtures remain loadable by editor tests, but cannot be rolled or
// offered to players. Do not reject all "Test" names: production rooms and the
// six real mercenaries still use that naming convention.
namespace GameplayAssetPolicy
{
    inline bool IsPlayerFacing(const FPrimaryAssetId& Id)
    {
        if (!Id.IsValid()) return false;
        const FString Name = Id.PrimaryAssetName.ToString();
        if (Id.PrimaryAssetType == ArtifactPrimaryAssetTypes::GetArtifactType())
            return !Name.StartsWith(TEXT("DA_TestArtifact_"));
        if (Id.PrimaryAssetType == UnitPrimaryAssetTypes::GetPlayerUnitType())
            return Name != TEXT("DA_TestPlayerUnit");
        if (Id.PrimaryAssetType == SkillPrimaryAssetTypes::GetActiveType())
        {
            static const TCHAR* Jobs[] = { TEXT("Barbarian"), TEXT("Druid"), TEXT("Knight"), TEXT("Mage"), TEXT("Ranger"), TEXT("Rogue") };
            for (const TCHAR* Job : Jobs)
                if (Name.StartsWith(FString::Printf(TEXT("DA_Test%s_Spell_"), Job))) return false;
        }
        return true;
    }

    inline bool HasPlayerFacingAssets(const TArray<FPrimaryAssetId>& Ids)
    {
        return Ids.ContainsByPredicate([](const FPrimaryAssetId& Id) { return IsPlayerFacing(Id); });
    }
}
