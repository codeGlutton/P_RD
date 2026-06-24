/*****************************************************************//**
 * @file   TacticalFrameworkSubsystem.h
 * @brief  TAS의 글로벌 서브시스템 정의 헤더
 * @author 모호재
 * @date   2026-06-23
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "ObjectView.h"
#include "Subsystems/WorldSubsystem.h"
#include "TacticalFrameworkSubsystem.generated.h"

class UTacticalFrameworkModel;

/**
 * @brief  TAS의 글로벌 서브시스템
 */
UCLASS()
class P_RD_API UTacticalFrameworkSubsystem : public UWorldSubsystem, public IObjectView
{
	GENERATED_BODY()

	/* IObjectView 상속 */
public:
	void BindModel(UObjectModel* Model) override;
	void UnbindModel(UObjectModel* Model) override;

protected:
	UObjectModel* GetModel_Internal() const override;

protected:
	UPROPERTY(Category = Model, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "mFrameworkModel"))
	TWeakObjectPtr<UTacticalFrameworkModel> mFrameworkModel;
};
