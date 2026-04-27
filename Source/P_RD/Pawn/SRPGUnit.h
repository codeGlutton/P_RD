/*****************************************************************//**
 * @file   SRPGUnit.h
 * @brief  SRPG에서 사용되는 베이스 폰 클래스 정의 파일
 * @author 모호재
 * @date   2026-04-25
 *********************************************************************/

#pragma once

#include "RDMinimal.h"

#include "GameFramework/Pawn.h"
#include "AbilitySystemInterface.h"
#include "GenericTeamAgentInterface.h"

#include "SRPGUnit.generated.h"

class UAbilitySystemComponent;

/**
 * @brief  SRPG에서 사용되는 베이스 폰 클래스
 */
UCLASS()
class P_RD_API ASRPGUnit : public APawn, public IGenericTeamAgentInterface, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ASRPGUnit();

	/* Pawn 상속 */
protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/* GenericTeamAgentInterface 상속 */
public:
	virtual void SetGenericTeamId(const FGenericTeamId& TeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;

	/* AbilitySystemInterface 상속 */
public:
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

private:
	UPROPERTY(Category = GAS, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", DisplayName = "AbilitySystemComp"))
	TObjectPtr<UAbilitySystemComponent>	mAbilitySystemComp;

private:
	FGenericTeamId mTeamId;
};
