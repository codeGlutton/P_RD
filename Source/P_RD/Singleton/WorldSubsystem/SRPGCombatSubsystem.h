/*****************************************************************//**
 * @file   SRPGCombatSubsystem.h
 * @brief  SRPG 턴제 전투를 제어하기 위한 서브 시스템 구현 헤더
 * @author 모호재
 * @date   2026-05-06
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ObjectView.h"

#include "Tool/CircularList.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "SRPGFramework/SRPGTurnContext.h"

#include "SRPGCombatSubsystem.generated.h"

class USRPGCombatModel;
class IObjectModel;
class UCombatUIModel;

class USRPGTurnContext;
class USRPGAction;
struct FPresentationBarrier;

/**
 * @brief  SRPG 턴제 전투를 제어하기 위한 서브 시스템
 */
UCLASS()
class P_RD_API USRPGCombatSubsystem : public UTickableWorldSubsystem, public IObjectView
{
	GENERATED_BODY()

	/* UTickableWorldSubsystem 상속 */
public:
	void Tick(float DeltaTime) override;
	TStatId GetStatId() const override;

	/* IObjectView 상속 */
public:
	void BindModel(UObjectModel* Model) override;
	void UnbindModel(UObjectModel* Model) override;

protected:
	UObjectModel* GetModel_Internal() const override;

protected:
	void MoveCameraOnBeginTurn(TSharedPtr<FPresentationBarrier> Barrier, const USRPGTurnContext* TurnContext);
	void FixCameraOnBeginMoveAction(TSharedPtr<FPresentationBarrier> Barrier, const USRPGTurnContext* TurnContext, const USRPGAction* Action);
	void ReleaseCameraOnEndMoveAction(TSharedPtr<FPresentationBarrier> Barrier, const USRPGTurnContext* TurnContext, const USRPGAction* Action, ESRPGActionResult Result);

private:
	AActor* GetTurnOwnerView(const USRPGTurnContext* TurnContext) const;

protected:
	UPROPERTY(Category = Model, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "CombatModel"))
	TWeakObjectPtr<USRPGCombatModel> mCombatModel;
};
