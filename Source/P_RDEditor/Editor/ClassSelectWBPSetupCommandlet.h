/*****************************************************************//**
 * @file   ClassSelectWBPSetupCommandlet.h
 * @brief  클래스 선택 WBP 디자인 정리용 에디터 commandlet
 * @date   2026-06-14
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"

#include "ClassSelectWBPSetupCommandlet.generated.h"

UCLASS()
class P_RDEDITOR_API UClassSelectWBPSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UClassSelectWBPSetupCommandlet();

	int32 Main(const FString& Params) override;
};

UCLASS()
class P_RDEDITOR_API UClassSelectIconVisibilityFixCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UClassSelectIconVisibilityFixCommandlet();

	int32 Main(const FString& Params) override;
};
