/*****************************************************************//**
 * @file   SkillComponent.h
 * @brief  스킬 컴포넌트 기본 베이스
 * @author 김준형
 * @date   2026-05-26
 *********************************************************************/
#pragma once

#include "../GAS/GASMinimal.h"
#include "Components/ActorComponent.h"
#include "../DataAsset/SkillData/StaticSkillData.h"
#include "SkillComponent.generated.h"

 /**
 * @brief 스킬 변경 시 사용되는 델리게이트(대리자)
 * @details SkillIndex의 데이터가 SkillData로 변경되었다는 것을 알리는 델리게이트(대리자)
 * @param SkillIndex : 변경된 스킬의 인덱스
 * @param SkillData : 변경된 스킬 데이터
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnSkillChange,
	int32, SkillIndex,
	TSoftObjectPtr<UStaticSkillData>, SkillData
);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class P_RD_API USkillComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USkillComponent();

protected:
	/**
	 * @brief 보유한 스킬 데이터
	 * 플레이어 4개 + 공격
	 * 적 가변적
	 */
	UPROPERTY()
	TArray<TSoftObjectPtr<UStaticSkillData>> mSkillData;

	/**
	* @brief 스킬 어빌리티
	* ASC에서 가져올 예정
	*/
	//UPROPERTY()
	//TSoftObjectPtr<FGameplayAbilitySpec> mActivatableAbilities;

	/**
	* @brief 액터의 ASC
	* 편하게 사용하기 위해 WeakPtr로 참조
	*/
	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> mAbilitySystemComp;

public:
	/* 델리게이트 변수*/
	FOnSkillChange mOnSkillChange;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	/* Get, Set */
	/**
	 * @brief 스킬 인덱스를 입력하면 Out_SkillData에 기록하는 함수
	 * @details 범위가 유효하지 않으면 false 반환
	 */
	bool GetSkillData(int In_SkillIndex, TSoftObjectPtr<UStaticSkillData>& Out_SkillData);

	/**
	* @brief 스킬 인덱스에 SkillData를 설정하는 함수
	* @details 범위가 유효하지 않으면 false 반환
	*/
	bool SetSkillData(int SkillIndex, TSoftObjectPtr<UStaticSkillData> SkillData);

	/**
	* @brief SkillData를 추가하는 함수
	* @details
	* 적들의 스킬을 추가할 때 사용할 것
	* 플레이어는 소지 스킬 개수가 고정되어 있으므로 사용하면 안됨
	* 어차피 적만 필요할 것 같으므로 적에게만 작성해도 될 듯
	*/
	//virtual bool AddSkillData(TSoftObjectPtr<UStaticSkillData> SkillData);

public:
	/* 요청값 반환 */
	/**
	* @details 해당 스킬의 예측값을 반환하는 함수
	* @param[in] In_SkillIndex : 선택한 스킬의 인덱스
	* @param[out] Out_TagValue : FString은 태그로 변경할 예정, float는 결과값 
	* @return bool : 실패 시 false 반환
	*/
	bool CalculatePredictedSales(int32 In_SkillIndex, TArray<TPair<FString, float>>& Out_TagValue);

public:
	/* 스킬 사용 */
	/**
	* @details 스킬의 인덱스와 타일을 입력받으면 스킬 사용
	* @param SkillIndex : 사용할 스킬의 인덱스
	* @param Tiles : 스킬 효과를 적용할 타일들
	* @return bool : 실패 시 false 반환
	*/
	bool ActivateSkill(int32 SkillIndex, TArray<TPair<int32,int32>> Tiles);

	/**
	* @brief 테스트용 스킬 사용 함수
	* @details 테스트용 GA를 사용해서 다른 유닛들 효과를 적용하는 함수
	* @param SkillIndex : 사용할 스킬의 인덱스
	* @param UnitArray : 사용할 대상
	* @return bool : 실패 시 false 반환
	*/
	UFUNCTION(BlueprintCallable, Category = "Skill")
	bool TestActivateSkill(int32 SkillIndex, TArray<AUnit*> UnitArray);

};
