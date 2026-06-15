/*****************************************************************//**
 * @file   StaticUnitSpawnData.h
 * @brief  유닛 생성 시 사용되는 정적 Primary Data Asset 구현 헤더
 * @author 모호재
 * @date   2026-05-20
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "DataAsset/PrimaryAssetType.h"
#include "DataAsset/BundleType.h"
#include "StaticUnitSpawnData.generated.h"

class AUnit;
class UStaticSkillData;
class UStaticEquipmentData;

/**
 * @brief  유닛 생성 시 사용되는 정적 Primary Data Asset
 */
UCLASS(abstract)
class P_RD_API UStaticUnitSpawnData : public UPrimaryDataAsset
{
	GENERATED_BODY()

#if WITH_EDITOR
public:
    EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

public:
	/**
	 * @brief AttributeSet 초기화 테이블과 저장/복원 로직에서 사용할 유닛 식별 key를 만든다.
	 *
	 * @details
	 * 화면에 보이는 이름은 번역이나 UI 문구 수정으로 바뀔 수 있으므로 GAS row key로 쓰기에는 불안정하다.
	 * 실제 스폰 클래스 이름을 기준으로 key를 만들면 DataAsset 표시 이름을 고쳐도 CurveTable row와 연결이 끊기지 않는다.
	 * Blueprint 클래스는 런타임 경로에 _C가 붙을 수 있어 row 이름과 맞추기 위해 suffix를 제거한다.
	 */
    FName GetKeyName() const;

public:
	UPROPERTY(Category = "Spawn", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Class", AssetBundles = "Actor"))
	TSoftClassPtr<AUnit> mClass;

public:
	UPROPERTY(Category = "Spawn", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SkillDatas", AssetBundles = "PAD"))
	TArray<TSoftObjectPtr<UStaticSkillData>> mSkillDatas;
    UPROPERTY(Category = "Spawn", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "EquipmentDatas", AssetBundles = "PAD"))
	TArray<TSoftObjectPtr<UStaticEquipmentData>> mEquipmentDatas;

public:
    UPROPERTY(Category = "UI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Icon", AssetBundles = "UI"))
    TSoftObjectPtr<UTexture2D> mIcon;
    UPROPERTY(Category = "UI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "DisplayName"))
    FText mDisplayName;
    UPROPERTY(Category = "UI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Description", MultiLine = true))
    FText mDescription;
};
