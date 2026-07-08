/*****************************************************************//**
 * @file   EventTriggerPayload.h
 * @brief  이벤트 호출 애님 노티파이의 추가 데이터 정의 헤더
 * @author 모호재
 * @date   2026-07-08
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "NiagaraSystem.h"
#include "EventTriggerPayload.generated.h"

/**
 * @brief  이벤트 호출 애님 노티파이의 추가 데이터
 */
USTRUCT(BlueprintType)
struct FEventTriggerPayload
{
    GENERATED_BODY()

public:
    virtual ~FEventTriggerPayload() = default;

public:
    virtual UScriptStruct* GetScriptStruct() const
    {
        return FEventTriggerPayload::StaticStruct();
    }
};

USTRUCT(BlueprintType)
struct FApplyNiagaraSpawnData
{
    GENERATED_BODY()

public:
    UPROPERTY(Category = "Niagara", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "NiagaraSystem"))
    TObjectPtr<UNiagaraSystem> mNiagaraSystem = nullptr;

    UPROPERTY(Category = "Niagara", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "SocketName"))
    FName mSocketName = NAME_None;

    UPROPERTY(Category = "Niagara", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "RelativeTransform"))
    FTransform mRelativeTransform = FTransform::Identity;

    UPROPERTY(Category = "Niagara", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Attached"))
    bool mAttached = true;
};

/**
 * @brief  적용 이벤트 호출 애님 노티파이의 추가 데이터
 */
USTRUCT(BlueprintType)
struct FApplyEventTriggerPayload : public FEventTriggerPayload
{
    GENERATED_BODY()

public:
    UScriptStruct* GetScriptStruct() const override
    {
        return FApplyEventTriggerPayload::StaticStruct();
    }

public:
    UPROPERTY(Category = "Niagara", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "NiagaraSpawnDatas"))
    TArray<FApplyNiagaraSpawnData> mNiagaraSpawnDatas;
};

