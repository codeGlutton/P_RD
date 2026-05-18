#include "Validator/EditorValidator_NamingConvention.h"
#include "Misc/DataValidation.h"

#include "Setting/AssetNamingConventionSettings.h"

bool UEditorValidator_NamingConvention::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
    const EDataValidationUsecase Case = InContext.GetValidationUsecase();
    if (Case != EDataValidationUsecase::Manual && Case != EDataValidationUsecase::Commandlet && Case != EDataValidationUsecase::Save)
    {
        return false;
    }
    if (InAssetData.GetAsset() == nullptr)
    {
        return false;
    }

    const UAssetNamingConventionSettings* Settings = GetDefault<UAssetNamingConventionSettings>();
    const TArray<FString>& TargetPaths = Settings->mTargetContentFolderPaths;
    const FString AssetPath = InAssetData.PackagePath.ToString();
    for (const FString& TargetPath : TargetPaths)
    {
        if (AssetPath.StartsWith(TargetPath) == true)
        {
            return true;
        }
    }
    return false;
}

EDataValidationResult UEditorValidator_NamingConvention::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
    if (InAsset == nullptr)
    {
        return EDataValidationResult::NotValidated;
    }

    // 규칙 없는 타입은 통과
    const FAssetNamingRule* Rule = GetExpectedNamingRule(InAsset);
    if (Rule == nullptr)
    {
        return EDataValidationResult::Valid;
    }

    // Prefix 검사
    const FString AssetName = InAsset->GetName();
    if (AssetName.StartsWith(Rule->mPrefix + TEXT("_")) == false)
    {
        Context.AddError(
            FText::FromString(FString::Printf(TEXT("[%s] 에셋은 반드시 [%s] 의 접두사를 가져야 함"), *AssetName, *Rule->mPrefix))
        );

        return EDataValidationResult::Invalid;
    }
    return EDataValidationResult::Valid;
}

const FAssetNamingRule* UEditorValidator_NamingConvention::GetExpectedNamingRule(UObject* InAsset) const
{
    const UAssetNamingConventionSettings* Settings = GetDefault<UAssetNamingConventionSettings>();

    for (auto Iter = Settings->mNamingRuleSets.rbegin(); Iter != Settings->mNamingRuleSets.rend(); ++Iter)
    {
        /*for (auto& Pair : Iter->mClassRules)
        {
            const TSubclassOf<UObject>& TargetClass = Pair.Key;
            if (TargetClass == nullptr)
            {
                continue;
            }

            if (InAsset->GetClass()->IsChildOf(TargetClass) == true)
            {
                return &Pair.Value;
            }
        }*/

        for (auto& Pair : Iter->mRules)
        {
            const TSubclassOf<UObject>& TargetClass = Cast<UClass>(Pair.Key.LoadSynchronous());
            if (TargetClass == nullptr)
            {
                continue;
            }

            if (InAsset->GetClass()->IsChildOf(TargetClass) == true)
            {
                return &Pair.Value;
            }
        }
    }
    return nullptr;
}
