/*****************************************************************//**
 * @file   GameplayAttributeComponent.h
 * @brief  게임 플레이 속성 컴포넌트 정의 헤더
 * @author 모호재
 * @date   2026-06-16
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAttributeComponent.generated.h"

UCLASS( ClassGroup=(GameplayAttribute), meta=(BlueprintSpawnableComponent) )
class P_RD_API UGameplayAttributeComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UGameplayAttributeComponent();

protected:
	void BeginPlay() override;

public:	
	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
