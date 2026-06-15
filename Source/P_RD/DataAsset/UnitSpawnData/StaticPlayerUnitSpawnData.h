/*****************************************************************//**
 * @file   StaticPlayerUnitSpawnData.h
 * @brief  플레이어 유닛 생성 시 사용되는 정적 Primary Data Asset 구현 헤더
 * @author 모호재
 * @date   2026-05-20
 *********************************************************************/

#pragma once

#include "DataAsset/UnitSpawnData/StaticUnitSpawnData.h"
#include "DataAsset/UnitSpawnData/PlayerJobType.h"
#include "StaticPlayerUnitSpawnData.generated.h"

class UStaticDiceData;

/**
 * @brief  플레이어 유닛 생성 시 사용되는 정적 Primary Data Asset
 */
UCLASS()
class P_RD_API UStaticPlayerUnitSpawnData : public UStaticUnitSpawnData
{
	GENERATED_BODY()

public:
	FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(UnitPrimaryAssetTypes::GetPlayerUnitType(), GetFName());
	}

#if WITH_EDITOR
public:
	void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

public:
	float GetDefaultMaxHP(int32 Difficulty) const;
	float GetDefaultMoney(int32 Difficulty) const;

public:
	UPROPERTY(Category = "Spawn", VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "JobType"))
	EPlayerJobType mJobType = EPlayerJobType::None;

public:
	/**
	 * @brief 새 런 시작 시 지급할 초기 골드
	 *
	 * @details
	 * HP는 GAS/CurveTable에서 난이도별 전투 스탯으로 관리하지만, 시작 골드는 캐릭터 선택 결과로
	 * 런 데이터에 들어갈 초기 자원이다. 그래서 전투 AttributeSet 기본값과 섞지 않고 PlayerUnit
	 * DataAsset에서 직접 조정할 수 있게 둔다.
	 */
	UPROPERTY(Category = "Spawn", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "StartingGold", ClampMin = "0", UIMin = "0"))
	int32 mStartingGold = 0;

	UPROPERTY(Category = "Spawn", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "DiceDatas", AssetBundles = "PAD"))
	TArray<TSoftObjectPtr<UStaticDiceData>> mDiceDatas;

public:
	UPROPERTY(Category = "UI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Portrait", AssetBundles = "UI"))
	TSoftObjectPtr<UTexture2D> mPortrait;
};
