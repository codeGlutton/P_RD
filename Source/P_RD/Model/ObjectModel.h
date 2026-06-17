/*****************************************************************//**
 * @file   ObjectModel.h
 * @brief  시뮬레이션 데이터 모델 최상위 인터페이스 정의 헤더
 * @author 이문환
 * @date   2026-06-17
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "UObject/Interface.h"
#include "ObjectModel.generated.h"

UINTERFACE(MinimalAPI)
class UObjectModel : public UInterface
{
	GENERATED_BODY()
};

/**
 * @brief  시뮬레이션 데이터 모델의 최상위 인터페이스
 * @details 모든 하위 데이터 모델(보드 액터, 컴포넌트 등)이 구현하는 공통 인터페이스다.
 */
class P_RD_API IObjectModel
{
	GENERATED_BODY()

public:
};
