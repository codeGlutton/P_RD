/*****************************************************************//**
 * @file   SkillComponentModel.h
 * @brief  액티브 스킬 컴포넌트 모델 구현 정의 헤더
 * @author 모호재
 * @date   2026-06-30
 *********************************************************************/
#pragma once

#include "RDMinimal.h"
#include "Component/ComponentModel.h"
#include "Actor/TileMap/TileLayer.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "SkillComponentModel.generated.h"

class UStaticSkillData;

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnChangeSkillUI, int32 /*SkillIndex*/, const UStaticSkillData* /*PreSkillData*/, const UStaticSkillData* /*NewSkillData*/);

/**
 * @brief 한 슬롯에 장착된 스킬과 그로 인해 설치된 런타임 객체 추적
 */
USTRUCT(BlueprintType)
struct FSkillEntry
{
	GENERATED_BODY()

public:
	FSkillEntry() = default;
	FSkillEntry(UStaticSkillData* Data);

public:
	bool IsValid() const;

public:
	// @brief 장착된 고정 스킬 데이터
	UPROPERTY(Category = "Static", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Data"))
	TObjectPtr<UStaticSkillData> mData = nullptr;

	// @brief 현재 선택된 스킬 여부
	UPROPERTY(Category = "Runtime", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "IsSelected"))
	bool mIsSelected = false;
};

/**
 * @brief  액티브 스킬 컴포넌트 모델
 */
UCLASS()
class P_RD_API USkillComponentModel : public UComponentModel
{
	GENERATED_BODY()

public:
	USkillComponentModel();

public:
	/**
	 * @brief 스킬 목록(스폰 데이터 등)을 일괄 장착
	 * @param SkillList 스킬 목록
	 */
	void SetSkillFrom(const TArray<TSoftObjectPtr<UStaticSkillData>>& SkillList);
	void SetSkillFrom(const TArray<FPrimaryAssetId>& SkillList);

public:
	FSkillEntry* GetSkill(int32 SkillIndex);
	void SetSkill(int32 SkillIndex, UStaticSkillData* SkillData);

	// [216 통합] PushSkillUI 등 UI가 슬롯 수를 알아야 해서 추가한 카운트 게터.
	int32 GetSkillNum() const { return mSkillEntries.Num(); }

public:
	/**
	* @brief 액티브 스킬을 활성화하는 함수
	* @param SkillIndex 사용할 스킬의 인덱스
	* @param TargetTiles 타겟팅한 타일들
	* @param DiceSum 주사위 눈금 합
	* @return 사용 성공 여부
	*/
	bool ActivateSkill(int32 SkillIndex, const TArray<FTileIndex>& TargetTiles, float DiceSum);

public:
	FOnChangeSkillUI OnChangeSkillUI;

protected:
	UPROPERTY(Category = "Entry", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SkillEntries"))
	TArray<FSkillEntry> mSkillEntries;
};
