/*****************************************************************//**
 * @file   LevelAttributeSet.h
 * @brief  Player 레벨에 대한 Attribute Set 정의 헤더
 * @author 모호재
 * @date   2026-05-16
 *********************************************************************/

#pragma once

#include "AttributeSet/AttributeSetMinimal.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "DataAsset/RarityRate.h"
#include "AttributeSet/LevelAttributeCache.h"
#include "LevelAttributeSet.generated.h"

/**
 * @brief  Level에 대한 Attribute Set 정의
 */
UCLASS()
class P_RD_API ULevelAttributeSet : public UTacticalAttributeSet
{
	GENERATED_BODY()
	
public:
	ULevelAttributeSet();

public:
	/*
	 * @brief 레벨별 속성(FRarityRate 및 Price) 캐싱 데이터 구조체를 생성합니다.
	 * @param WorldContextObject 월드 컨텍스트 객체
	 * @return 생성된 FLevelAttributeCache
	 */
	static FLevelAttributeCache MakeCache(const UObject* WorldContextObject);

public:
	/*
	 * @brief 레벨별 희귀도 비율(FRarityRate) 배열을 추출합니다.
	 * @param WorldContextObject 월드 컨텍스트 객체
	 * @return 레벨 순서대로 나열된 FRarityRate 배열
	 */
	static TArray<FRarityRate> GetRarityRates(const UObject* WorldContextObject);

	/*
	 * @brief 특정 레벨의 희귀도 비율(FRarityRate)을 추출합니다.
	 * @param WorldContextObject 월드 컨텍스트 객체
	 * @param Level 대상 레벨
	 * @return 해당 레벨의 FRarityRate
	 */
	static FRarityRate GetRarityRate(const UObject* WorldContextObject, int32 Level);

	/**
	 * @brief 요청 레벨에 실제로 사용할 수 있는 스킬 희귀도 가중치가 있는지 확인한다.
	 * @param WorldContextObject 월드 컨텍스트 객체
	 * @param Level 대상 레벨(1-indexed)
	 * @param OutRate 성공 시 해당 레벨의 가중치
	 * @return 레벨이 커브 범위 안이고 가중치가 유효하면 true
	 */
	static bool TryGetRarityRate(const UObject* WorldContextObject, int32 Level, OUT FRarityRate& OutRate);

	/*
	 * @brief 레벨별 가격(Price) 배열을 추출합니다.
	 * @param WorldContextObject 월드 컨텍스트 객체
	 * @return 레벨 순서대로 나열된 Price 배열
	 */
	static TArray<float> GetPrices(const UObject* WorldContextObject);

	/*
	 * @brief 특정 레벨의 가격(Price)을 추출합니다.
	 * @param WorldContextObject 월드 컨텍스트 객체
	 * @param Level 대상 레벨
	 * @return 해당 레벨의 Price
	 */
	static float GetPrice(const UObject* WorldContextObject, int32 Level);

public:
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(ULevelAttributeSet, CommonSkillWeight)
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(ULevelAttributeSet, RareSkillWeight)
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(ULevelAttributeSet, EpicSkillWeight)

	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(ULevelAttributeSet, Price)

public:
	static const FName KEY_NAME;

protected:
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData CommonSkillWeight;
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData RareSkillWeight;
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData EpicSkillWeight;

	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData Price;
};

