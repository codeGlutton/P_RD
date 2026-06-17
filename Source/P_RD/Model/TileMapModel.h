/*****************************************************************//**
 * @file   TileMapModel.h
 * @brief  타일맵 데이터 모델 인터페이스 정의 헤더
 * @author 이문환
 * @date   2026-06-17
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "UObject/Interface.h"
#include "Model/ObjectModel.h"
#include "TileMapModel.generated.h"

UINTERFACE(MinimalAPI)
class UTileMapModel : public UObjectModel
{
	GENERATED_BODY()
};

/**
 * @brief  타일맵 데이터 모델 인터페이스
 * @details 보드 액터들이 배치되는 타일맵의 데이터 모델 공통 인터페이스다.
 */
class P_RD_API ITileMapModel : public IObjectModel
{
	GENERATED_BODY()

public:
};
