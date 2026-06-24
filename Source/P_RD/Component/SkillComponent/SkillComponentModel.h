/*****************************************************************//**
 * @file   SkillComponent.h
 * @brief  스킬 컴포넌트 기본 베이스
 * @author 김준형
 * @date   2026-05-26
 *********************************************************************/
#pragma once

#include "RDMinimal.h"
#include "Component/ComponentModel.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "DataAsset/SkillData/StaticSkillData.h"
#include "TAS/TacticalAbility_Skill.h"
#include "SkillComponentModel.generated.h"
/*
* @param SkillIndex : 변경된 스킬의 인덱스
* @param SkillData : 변경된 스킬 데이터
*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnSkillChange,
	int32, SkillIndex,
	TSoftObjectPtr<UStaticSkillData>, SkillData
);

/**
 * 
 */
UCLASS()
class P_RD_API USkillComponentModel : public UComponentModel
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USkillComponentModel();

protected:
	/**
	 * @brief 보유한 스킬 데이터
	 * 플레이어 4개 + 공격
	 * 적 가변적
	 */
	UPROPERTY(VisibleAnywhere)
	TArray<TSoftObjectPtr<class UStaticSkillData>> mSkillData;

	/**
	* @brief 스킬 어빌리티
	* ASC에서 가져올 예정
	*/
	//UPROPERTY()
	//TSoftObjectPtr<FGameplayAbilitySpec> mActivatableAbilities;

public:
	/* 델리게이트 변수*/
	FOnSkillChange OnSkillChange;

public:
	virtual void Initialize() override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	/* Get, Set */
	/**
	 * @brief 스킬 인덱스를 입력하면 Out_SkillData에 기록하는 함수
	 * @details 범위가 유효하지 않으면 false 반환
	 */
	void GetSkillData(int In_SkillIndex, OUT TSoftObjectPtr<UStaticSkillData>& Out_SkillData);

	/**
	* @brief 스킬 인덱스에 SkillData를 설정하는 함수
	* @details 범위가 유효하지 않으면 false 반환
	*/
	void SetSkillData(int SkillIndex, IN const TSoftObjectPtr<UStaticSkillData>& SkillData);

	/**
	* @brief SkillData를 추가하는 함수
	* @details
	* 적들의 스킬을 추가할 때 사용할 것
	* 플레이어는 소지 스킬 개수가 고정되어 있으므로 사용하면 안됨
	* 어차피 적만 필요할 것 같으므로 적에게만 작성해도 될 듯
	*/
	virtual void AddSkillData(IN const TSoftObjectPtr<UStaticSkillData>& SkillData);

public:
	/* 스킬 사용 */
	/**
	* @brief 스킬의 인덱스와 타일을 입력받으면 스킬 사용
	* @details
	* 이미 가지고 있는 커맨드 로그를 토대로 스킬을 진행
	* @return bool : 실패 시 false 반환
	*/
	UFUNCTION(BlueprintCallable, Category = "Skill")
	bool ActivateSkill(int32 SkillIndex, const TArray<FTileIndex> TargetTiles);

	/* 이동포인트 */
	/**
	* @brief 이동 완료 후 어트리뷰트 셋에서 moveCount를 감소 시키는 함수
	* @details
	*/
	UFUNCTION(BlueprintCallable, Category = "Move")
	void HandelMovePoint(float MovePoint);

private:
	void ApplyEffect(FTileIndex TargetTile, TArray<UTacticalEffectContext*>& EffectContexts);
};
