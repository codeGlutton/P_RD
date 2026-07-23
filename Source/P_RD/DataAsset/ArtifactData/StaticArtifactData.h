/*****************************************************************//**
 * @file   StaticArtifactData.h
 * @brief  아티펙트 생성 시 사용되는 정적 Primary Data Asset 구현 헤더
 * @author 이문환
 * @date   2026-07-22
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "DataAsset/PrimaryAssetType.h"
#include "DataAsset/BundleType.h"
#include "DataAsset/PassiveData/StaticPassiveData.h"
#include "TAS/Effect/TacticalEffect.h"
#include "StaticArtifactData.generated.h"

/**
 * @brief  아티펙트 생성 시 사용되는 정적 Primary Data Asset
 * @details
 * 파티가 보유하면 소속 용병 전원이 장착한 것과 같은 효과를 냄.
 * 슬롯 개념이 없어 단일 클래스로 사용.
 */
UCLASS()
class P_RD_API UStaticArtifactData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 에셋 ID를 (Artifact, 에셋이름)으로 보고 (ini 스캔 등록과 매칭)
	FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(ArtifactPrimaryAssetTypes::GetArtifactType(), GetFName());
	}

	/**
	 * @brief PrimaryAssetId로 아티펙트 DA 로드
	 * @details 이미 로드돼 있으면 재로드 없이 그대로 반환
	 * @param ArtifactId 로드할 아티펙트의 PrimaryAssetId
	 * @return 로드된 아티펙트 데이터 (실패 시 nullptr)
	 */
	static UStaticArtifactData* LoadByAssetId(const FPrimaryAssetId& ArtifactId);

public:
	UPROPERTY(Category = "Default", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Name"))
	FText mName;

	UPROPERTY(Category = "Default", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Icon", AssetBundles = "UI"))
	TSoftObjectPtr<UTexture2D> mIcon;

	UPROPERTY(Category = "Default", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Price"))
	int32 mPrice;

	/**
	 * @brief 아티펙트 희귀도
	 * @details
	 * 에셋 레지스트리 태그로 기록되어 로드 없이 희귀도 필터 조회 가능
	 */
	UPROPERTY(Category = "Artifact", EditAnywhere, BlueprintReadWrite, AssetRegistrySearchable, meta = (DisplayName = "RarityType"))
	ERarityType mRarityType;

	// 장착 시 용병 각각의 패시브 컴포넌트에 설치될 패시브 목록
	UPROPERTY(Category = "Artifact", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Passive", AssetBundles = "Actor"))
	TArray<TSoftObjectPtr<UStaticPassiveData>> mStaticPassiveData;

	/**
	 * @brief 아티펙트 고유 스탯 수정자
	 * @details
	 * 장착 시 무한(Infinite) 이펙트로 AttributeSet에 즉시 적용하고, 해제 시 핸들로 제거.
	 */
	UPROPERTY(Category = "Artifact", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "StatModifiers"))
	TArray<FTacticalModifierInfo> mStatModifiers;
};
