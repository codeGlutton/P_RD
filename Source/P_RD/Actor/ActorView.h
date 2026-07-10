/*****************************************************************//**
 * @file   ActorView.h
 * @brief  액터 시뮬레이션 뷰 인터페이스 정의 헤더
 * @author 모호재
 * @date   2026-06-18
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "ObjectView.h"
#include "ActorView.generated.h"

class UObjectModel;
class UActorModel;

UINTERFACE(MinimalAPI)
class UActorView : public UObjectView
{
	GENERATED_BODY()
};

/**
 * @brief  액터 시뮬레이션 뷰의 인터페이스
 * @details 액터가 구현하는 공통 인터페이스다.
 */
class P_RD_API IActorView : public IObjectView
{
	GENERATED_BODY()

	/* IObjectView 상속 */
public:
	void BindModel(UObjectModel* Model) override;
	void UnbindModel(UObjectModel* Model) override;
};
