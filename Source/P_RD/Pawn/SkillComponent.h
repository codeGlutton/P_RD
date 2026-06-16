/*****************************************************************//**
 * @file   SkillComponent.h
 * @brief  스킬 컴포넌트 기본 베이스
 * @author 김준형
 * @date   2026-05-26
 *********************************************************************/
#pragma once

#include "GAS/GASMinimal.h"
#include "Components/ActorComponent.h"
#include "DataAsset/SkillData/StaticSkillData.h"
#include "SRPGFramework/SRPGFrameworkType.h"
//#include "Pawn/SkillComponent/PreviewEffectIconData.h"
#include "../FunctionLibrary/CombatCalculator/CombatResult.h"
#include "FunctionLibrary/CommandLog/CommandLog.h"
#include "SRPGFramework/TileActor.h"
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
	UPROPERTY(VisibleAnywhere)
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
	FOnSkillChange OnSkillChange;

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
	virtual bool AddSkillData(TSoftObjectPtr<UStaticSkillData> SkillData);

public:
	/* 스킬 사용 */
	/**
	* @brief 스킬의 인덱스와 타일을 입력받으면 스킬 사용
	* @details 
	* 이미 가지고 있는 커맨드 로그를 토대로 스킬을 진행
	* @return bool : 실패 시 false 반환
	*/
	UFUNCTION(BlueprintCallable, Category = "Skill")
	bool ActivateSkill(const FCommandLog& SkillResult);

	/**
	* @brief 스킬 결과 값 계산
	* @details 
	* 스킬 인덱스와 타일이 들어오면 스킬 결과 값을 계산한다.
	* @param[in] SkillIndex : 선택한 스킬의 인덱스
	* @param[in] In_CloneData : 타일맵 복제 데이터
	* @param[in] In_Context: 계산에 필요한 정보들
	* @param[out] Out_Result: 결과 로그
	* @return bool : 실패 시 false 반환
	*/
	UFUNCTION(BlueprintCallable, Category = "Skill")
	bool CalculateSkillResult(int32 SkillIndex, const FTileMapCloneData& In_CloneData, const FCommandLogFunctionContext& In_Context, FCommandLog& Out_Result);
};
