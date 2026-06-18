/*****************************************************************//**
 * @file   ComponentModel.h
 * @brief  보드 액터에 부착되는 컴포넌트 데이터 모델 클래스 정의 헤더
 * @author 이문환
 * @date   2026-06-18
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "ObjectModel.h"
#include "ComponentModel.generated.h"

/**
 * @brief  보드 액터에 부착되는 컴포넌트 데이터 모델 클래스
 * @details 패시브, 장비 등 보드 액터가 소유하는 컴포넌트 모델이다.
 */
UCLASS()
class P_RD_API UComponentModel : public UObject, public IObjectModel
{
	GENERATED_BODY()

public:
};
