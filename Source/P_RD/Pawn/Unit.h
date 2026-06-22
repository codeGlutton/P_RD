/*****************************************************************//**
 * @file   Unit.h
 * @brief  턴을 소유할 수 있는 베이스 폰 클래스 정의 파일
 * @author 모호재
 * @date   2026-04-25
 *********************************************************************/

#pragma once

#include "RDMinimal.h"

#include "Actor/ActorView.h"
#include "GameFramework/Pawn.h"

#include "Unit.generated.h"

class UUnitModel;

/**
 * @brief  턴을 소유할 수 있는 베이스 폰 클래스
 */
UCLASS(abstract)
class P_RD_API AUnit : public APawn, public IActorView
{
	GENERATED_BODY()

	/* IActorView 상속 */
protected:
	UObjectModel* GetModel_Internal() const override;

protected:
	TWeakObjectPtr<UUnitModel> mUnitModel;
};
