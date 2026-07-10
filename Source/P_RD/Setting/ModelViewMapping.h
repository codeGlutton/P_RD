/*****************************************************************//**
 * @file   ModelViewMapping.h
 * @brief  모델에 대한 뷰 모음에 대한 헤더
 * @author 모호재
 * @date   2026-06-18
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "ModelViewMapping.generated.h"

class UObjectModel;

 /**
  * @brief  서브시스템 모델에 대한 뷰 모음
  */
USTRUCT(BlueprintType)
struct FSubsystemModelViewMapping
{
    GENERATED_BODY()

public:
    FSubsystemModelViewMapping() = default;
    FSubsystemModelViewMapping(UClass* ModelClass);
    FSubsystemModelViewMapping(TSoftClassPtr<UObjectModel> ModelClass);

public:
    bool operator==(const FSubsystemModelViewMapping& Other) const
    {
        return mModelClass == Other.mModelClass;
    }

    friend uint32 GetTypeHash(const FSubsystemModelViewMapping& Mapping)
    {
        return GetTypeHash(Mapping.mModelClass);
    }

public:
    UPROPERTY(Category = Model, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "ModelClass"))
    TSoftClassPtr<UObjectModel> mModelClass;

public:
    UPROPERTY(Category = View, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "SubsystemViewClass", MustImplement = "/Script/P_RD.ObjectView"))
    TSoftClassPtr<UWorldSubsystem> mViewClass = nullptr;
};

/**
 * @brief  월드 모델에 대한 뷰 모음
 */
USTRUCT(BlueprintType)
struct FWorldModelViewMapping
{
    GENERATED_BODY()

public:
    FWorldModelViewMapping() = default;
    FWorldModelViewMapping(UClass* ModelClass);
    FWorldModelViewMapping(TSoftClassPtr<UObjectModel> ModelClass);

public:
    bool operator==(const FWorldModelViewMapping& Other) const
    {
        return mModelClass == Other.mModelClass;
    }

    friend uint32 GetTypeHash(const FWorldModelViewMapping& Mapping)
    {
        return GetTypeHash(Mapping.mModelClass);
    }

public:
    UPROPERTY(Category = Model, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "ModelClass"))
    TSoftClassPtr<UObjectModel> mModelClass;

public:
    UPROPERTY(Category = View, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "ViewClass", MustImplement = "/Script/P_RD.ActorView"))
    TSoftClassPtr<AActor> mViewClass = nullptr;
};