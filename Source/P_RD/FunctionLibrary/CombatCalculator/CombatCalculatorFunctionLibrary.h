/*****************************************************************//**
 * @file   CombatCalculatorFunctionLibrary.h
 * @brief  전투 결과 계산하여 출력하는 계산기 함수 라이브러리
 * @author 김준형
 * @date   2026-06-11
 *********************************************************************/
#pragma once

#include "CoreMinimal.h"
#include "CombatResult.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CombatCalculatorFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class P_RD_API UCombatCalculatorFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	/**
	* @brief 스킬 적용 결과를 반환하는 함수
	* @details
	* @param[in] Caster : 캐스터의 위치
	* @param[in] SkillData : 적용할 스킬
	* @param[in] Tiles : 선택한 타일
	* @param[out] Out_Result : 스킬 사용 결과
	* @return bool : 실패 시 false 반환
	*/
	static bool CalculateSkillResult(FTileIndex CasterTile, const class UStaticSkillData* SkillData, TArray<FTileIndex> Tiles, FSkillCommitResult& Out_Result);
};
