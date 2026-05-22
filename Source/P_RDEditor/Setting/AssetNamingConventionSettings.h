/*****************************************************************//**
 * @file   AssetNamingConventionSettings.h
 * @brief  에셋 네이밍 컨벤션 설정 객체 정의 헤더
 * @author 모호재
 * @date   2026-05-13
 *********************************************************************/

#pragma once

#include "RDEditorMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AssetNamingConventionSettings.generated.h"

USTRUCT(BlueprintType)
struct P_RDEDITOR_API FAssetNamingRule
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = Rule, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Prefix"))
	FString mPrefix;
};

USTRUCT(BlueprintType)
struct P_RDEDITOR_API FAssetNamingRuleSet
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = RuleSet, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Priority"))
	int32 mPriority = 0;
	UPROPERTY(Category = RuleSet, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "InstanceRules"))
	TMap<TSoftObjectPtr<UObject>, FAssetNamingRule> mRules;
};

/**
 * @brief  에셋 네이밍 컨벤션 설정 객체
 */
UCLASS(Config = Editor, DefaultConfig, meta = (DisplayName = "Asset Naming Convention Editing"))
class P_RDEDITOR_API UAssetNamingConventionSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
	/* UDeveloperSettings 상속 */
public:
	FName GetCategoryName() const override;

#if WITH_EDITOR
	FText GetSectionText() const override;
	FText GetSectionDescription() const override;

	void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	UPROPERTY(Config, Category = RuleSet, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "NamingRuleSets", ToolTip = "네이밍 컨벤션을 이루는 네이밍 룰 셋", EditFixedOrder))
	TArray<FAssetNamingRuleSet> mNamingRuleSets;

public:
	UPROPERTY(Config, Category = Path, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "TargetContentFolderPaths", ToolTip = "검사 대상이 되는 상대 경로 폴더들"))
	TArray<FString> mTargetContentFolderPaths;
};
