#include "Commandlet/RepairSkillTextIdentityCommandlet.h"
#include "DataAsset/SkillData/StaticUnitSkillData.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

int32 URepairSkillTextIdentityCommandlet::Main(const FString&)
{
    struct FRepair { const TCHAR* Path; const TCHAR* Key; const TCHAR* Name; };
    const FRepair Repairs[] = {
        {TEXT("/Game/BP/DataAsset/Skill/Mercenary/Rogue/DA_Rogue_Spell_Rare_LegSweep"), TEXT("Rogue.LegSweep"), TEXT("다리 걸기")},
        {TEXT("/Game/BP/DataAsset/Skill/Mercenary/Mage/DA_Mage_Attack_Rare_ThunderStorm"), TEXT("Mage.ThunderStorm"), TEXT("벼락 폭풍")}
    };
    for (const auto& Repair : Repairs)
    {
        auto* Asset = LoadObject<UStaticUnitSkillData>(nullptr, Repair.Path);
        if (!Asset) return 1;
        Asset->mName = FText::ChangeKey(TEXT("SkillNames"), Repair.Key, FText::FromString(Repair.Name));
        Asset->MarkPackageDirty();
        FSavePackageArgs Args;
        Args.TopLevelFlags = RF_Public | RF_Standalone;
        const FString Filename = FPackageName::LongPackageNameToFilename(Asset->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
        if (!UPackage::SavePackage(Asset->GetOutermost(), Asset, *Filename, Args)) return 1;
        UE_LOG(LogTemp, Display, TEXT("Repaired localizable skill identity: %s"), Repair.Path);
    }
    return 0;
}
