/*****************************************************************//**
 * @file   TileMapModel.h
 * @brief  타일맵 데이터 모델 클래스 정의 헤더
 * @author 이문환
 * @date   2026-06-18
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "ObjectModel.h"
#include "TileMapModel.generated.h"

/**
 * @brief  타일맵 데이터 모델 클래스
 * @details 보드 액터들이 배치되는 타일맵의 데이터 모델이다.
 */
UCLASS()
class P_RD_API UTileMapModel : public UObject, public IObjectModel
{
	GENERATED_BODY()

public:
};
