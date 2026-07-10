/*****************************************************************//**
 * @file   SRPGCommandRouterSubsystem.h
 * @brief  SRPG 유저 명령을 전달하는 서브시스템 정의 헤더
 * @author 모호재
 * @date   2026-06-15
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "ObjectView.h"
#include "Subsystems/WorldSubsystem.h"
#include "SRPGCommandRouterSubsystem.generated.h"

class USRPGCommandRouterModel;

/**
 * @brief  SRPG 유저 명령을 전달하는 서브시스템
 */
UCLASS()
class P_RD_API USRPGCommandRouterSubsystem : public UWorldSubsystem, public IObjectView
{
	GENERATED_BODY()

	/* IObjectView 상속 */
public:
	void BindModel(UObjectModel* Model) override;
	void UnbindModel(UObjectModel* Model) override;

protected:
	UObjectModel* GetModel_Internal() const override;

protected:
	UPROPERTY(Category = Model, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "RouterModel"))
	TWeakObjectPtr<USRPGCommandRouterModel> mRouterModel;
};
