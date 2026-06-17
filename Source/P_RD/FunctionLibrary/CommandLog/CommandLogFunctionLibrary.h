/*****************************************************************//**
 * @file   CommandLogFunctionLibrary.h
 * @brief  전투 결과 계산하여 출력하는 계산기 함수 라이브러리
 * @author 김준형
 * @date   2026-06-11
 *********************************************************************/
#pragma once

#include "CommandLog.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "CommandLogFunctionLibrary.generated.h"

struct FTileIndex;
class UStaticSkillData;

// @brief 효과 묶음 구조체
USTRUCT(BlueprintType)
struct FEffectPacket
{
	GENERATED_BODY()

public:
	TArray<TPair<FGameplayTag, float>> mEffectValue;
};

/**
 * 
 */
UCLASS()
class P_RD_API UCommandLogFunctionLibrary : public UBlueprintFunctionLibrary
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
	static bool CalculateSkillCommandLog(
		FTileMapCloneData TileMap,
		const FCommandLogFunctionContext& Context,
		FCommandLog& Out_Result);

private:
	static FEffectPacket CalculateDefaultSkillEffectValue(const FTileActorCloneData& Caster, const UStaticSkillData* SkillData);

	// 효과값을 갱신하는 패시브
	// @brief 스킬 사용 전 패시브 기반으로 효과 갱신
	static void CalculateSkillPassive(const FTileActorCloneData& Caster, FEffectPacket& EffectPacket);

	// @brief 모션 전 패시브 기반으로 효과 갱신
	static void CalculateMotionPassive(const FTileActorCloneData& Caster, TPair<FGameplayTag, float>& Effect);
	
	// @brief 공격 전 패시브 기반으로 효과 갱신
	static void CalculateAttackPassive(const FTileActorCloneData& Caster, int32 TargetTileIndex, TPair<FGameplayTag, float>& Effect);
	
	// @brief 피격 전 패시브 기반으로 효과 갱신
	static void CalculateHitPassive(const FTileActorCloneData& Caster, int32 TargetTileIndex, TPair<FGameplayTag, float>& Effect);



	// 스냅샷을 기반으로 이벤트 로그 생성
	// @brief 타일 복제체를 기반으로 효과 적용
	static bool CreateEventLog(FTileMapCloneData& TileMapCloneData, int32 TargetTileIndex, TPair<FGameplayTag, float>& Effect, FEventLog& EventLog);

	
	
	// 로그를 기반으로 스냅샷 변경
	// @brief 생성한 이벤트를 복제체에 적용
	static void ChangeTileMapCloneDataFromEvent(FTileMapCloneData& TileMapCloneData, const FEventLog& TileLog);
};
