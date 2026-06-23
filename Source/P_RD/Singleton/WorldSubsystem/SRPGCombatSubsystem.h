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

	/* 전투 UI 데이터 경계 (MVVM ViewModel; 전투 수명 동안 1개 소유) */
public:
	/** @brief 전투 UI가 읽고 어댑터가 쓰는 단일 CombatUIModel. 없으면 생성한다(권장 소유 위치=전투 서브시스템). */
	UCombatUIModel* GetCombatUIModel();

protected:
	UPROPERTY(Category = Model, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "CombatModel"))
	TWeakObjectPtr<USRPGCombatModel> mCombatModel;

	UPROPERTY()
	TObjectPtr<UCombatUIModel> mCombatUIModel;
};
