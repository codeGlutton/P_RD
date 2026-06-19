/*****************************************************************//**
 * @file   ModelViewMapping.h
 * @brief  모델에 대한 뷰 모음에 대한 헤더
 * @author 모호재
 * @date   2026-06-18
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "ObjectView.h"
#include "ModelViewMapping.generated.h"

/**
 * @brief  모델에 대한 뷰 모음
 */
USTRUCT(BlueprintType)
struct FModelViewMapping
{
    GENERATED_BODY()

public:
    FModelViewMapping() = default;
    FModelViewMapping(UClass* ModelClass);
    FModelViewMapping(TSoftClassPtr<UObjectModel> ModelClass);

public:
    bool operator==(const FModelViewMapping& Other) const
    {
        return mModelClass == Other.mModelClass;
    }

    friend uint32 GetTypeHash(const FModelViewMapping& Mapping)
    {
        return GetTypeHash(Mapping.mModelClass);
    }

public:
    UPROPERTY(Category = Model, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "ModelClass"))
    TSoftClassPtr<UObjectModel> mModelClass;

    UPROPERTY(Category = View, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "ViewClass", MustImplement = "/Script/P_RD.ActorView"))
    TSoftClassPtr<AActor> mViewClass;
};