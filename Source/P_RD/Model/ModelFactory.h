/*****************************************************************//**
 * @file   ModelFactory.h
 * @brief  모델 생성 용 팩토리 헤더
 * @author 모호재
 * @date   2026-06-17
 *********************************************************************/
#pragma once

#include "RDMinimal.h"
#include "ModelFactory.generated.h"

// Model Factory 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogModelFactory, Log, All)

UINTERFACE(MinimalAPI)
class UModelFactory : public UInterface
{
	GENERATED_BODY()
};

class P_RD_API IModelFactory
{
	GENERATED_BODY()

public:
	
};

