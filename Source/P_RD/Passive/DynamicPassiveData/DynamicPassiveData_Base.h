/*****************************************************************
 * @file   DynamicPassiveData_Base.h
 * @brief  패시브 장착 시 패시브 컴포넌트가 함께 생성할 동적 데이터 베이스
 * @author 김준형
 * @date   2026-06-18
 ****************************************************************/
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "DynamicPassiveData_Base.generated.h"

/**
 * 
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class UDynamicPassiveData_Base : public UObject
{
    GENERATED_BODY()

public:

};
