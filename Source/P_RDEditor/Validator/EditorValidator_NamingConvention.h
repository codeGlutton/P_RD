/*****************************************************************//**
 * @file   EditorValidator_NamingConvention.h
 * @brief  이름 컨벤션을 검사하는 Validator 정의 헤더
 * @author 모호재
 * @date   2026-05-13
 *********************************************************************/

#pragma once

#include "RDEditorMinimal.h"
#include "EditorValidatorBase.h"
#include "EditorValidator_NamingConvention.generated.h"

struct FAssetNamingRule;

/**
 * @brief  이름 컨벤션을 검사하는 Validator
 */
UCLASS()
class P_RDEDITOR_API UEditorValidator_NamingConvention : public UEditorValidatorBase
{
	GENERATED_BODY()

public:
    bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;
    EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;

private:
    const FAssetNamingRule* GetExpectedNamingRule(UObject* InAsset) const;
};
