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

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/* Get, Set */
public:
	/**
	 * @brief 스킬 인덱스를 입력하면 Out_SkillData에 기록하는 함수
	 * 범위가 유효하지 않으면 false 반환
	 */
	bool GetSkillData(int In_SkillIndex, TSoftObjectPtr<UStaticSkillData>& Out_SkillData);
	/**
	* @brief 스킬 인덱스에 SkillData를 설정하는 함수
	* 범위가 유효하지 않으면 false 반환
	*/
	bool SetSkillData(int SkillIndex, TSoftObjectPtr<UStaticSkillData> SkillData);

	/* 요청값 반환 */
public:
	/**
	* @details 해당 스킬의 예측값을 반환하는 함수
	* @param[in] In_SkillIndex : 선택한 스킬의 인덱스
	* @param[out] Out_TagValue : FString은 태그로 변경할 예정, float는 결과값 
	* @return bool : 실패 시 false 반환
	*/
	bool CalculatePredictedSales(int32 In_SkillIndex, TArray<TPair<FString, float>>& Out_TagValue);

	/* 스킬 사용 */
public:
	/**
	* @details 스킬의 인덱스와 타일을 입력받으면 스킬 사용
	* @ SkillIndex : 사용할 스킬의 인덱스
	* @ Tiles : 스킬 효과를 적용할 타일들
	* @return bool : 실패 시 false 반환
	*/
	bool ActivateSkill(int32 SkillIndex, TArray<TPair<int32,int32>> Tiles);

	/**
	* @details 스킬의 인덱스와 타일을 입력받으면 스킬 사용
	* @ SkillIndex : 사용할 스킬의 인덱스
	* @ UnitArray : 사용할 대상
	* @return bool : 실패 시 false 반환
	*/
	UFUNCTION(BlueprintCallable, Category = "Skill")
	bool TestActivateSkill(int32 SkillIndex, TArray<class AUnit*> UnitArray);

};
