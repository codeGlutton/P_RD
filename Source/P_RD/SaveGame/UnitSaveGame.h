/*****************************************************************//**
 * @file   UnitSaveGame.h
 * @brief  유닛 데이터를 저장하는 객체 정의 헤더
 * @author 모호재
 * @date   2026-04-29
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "GameFramework/SaveGame.h"

#include "UnitSaveGame.generated.h"

// UnitSaveGame 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogUnitSave, Log, All)

/**
 * @brief  Unit 데이터를 저장하는 객체
 */
UCLASS()
class P_RD_API UUnitSaveGame : public USaveGame
{
	GENERATED_BODY()
	
public:
	UPROPERTY(Category = Attribute, VisibleAnywhere, meta = (DisplayName = "MaxHP"))
	float mMaxHP;
	UPROPERTY(Category = Attribute, VisibleAnywhere, meta = (DisplayName = "HP"))
	float mHP;
};

/**
 * @brief  Player Unit 데이터를 저장하는 객체
 */
UCLASS()
class P_RD_API UPlayerUnitSaveGame : public UUnitSaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = Attribute, VisibleAnywhere, meta = (DisplayName = "Level"))
	float mLevel;
	UPROPERTY(Category = Attribute, VisibleAnywhere, meta = (DisplayName = "Exp"))
	float mExp;

	UPROPERTY(Category = Attribute, VisibleAnywhere, meta = (DisplayName = "Money"))
	float mMoney;
};