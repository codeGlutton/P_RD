/*****************************************************************//**
 * @file   PresentationSubsystem.h
 * @brief  연출 처리를 담당하는 월드 서브시스템 정의 헤더
 * @author 모호재
 * @date   2026-08-27
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Singleton/WorldSubsystem/PresentationBarrier.h"
#include "PresentationSubsystem.generated.h"

struct FPresentationQueueNodeKeyBase
{
};

struct FPresentationQueueNodeBase
{
public:
	virtual ~FPresentationQueueNodeBase() = default;

public:
	virtual void Present(const FPresentationQueueNodeKeyBase& Key, TSharedPtr<FPresentationBarrier> Barrier) const = 0;
};

USTRUCT()
struct FPresentationQueueContainerBase
{
	GENERATED_BODY()

public:
	virtual ~FPresentationQueueContainerBase() = default;

public:
	virtual UScriptStruct* GetScriptStruct() const
	{
		return FPresentationQueueContainerBase::StaticStruct();
	}

public:
	FPresentationQueueNodeBase& FindOrAddQueue(const FPresentationQueueNodeKeyBase& Key);

private:
	int32 GetDepth(const FPresentationQueueNodeKeyBase& Key) const;
	FPresentationQueueNodeBase* FindOrAddQueue_Internal(const FPresentationQueueNodeKeyBase& Key, int32 Depth);
};

/**
 * @brief  연출 처리를 담당하는 월드 서브시스템
 */
UCLASS()
class P_RD_API UPresentationSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

	/* UWorldSubsystem 상속 */
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	template<typename QueueContainerType>
	TQueue<typename QueueContainerType::Node>& FindOrAddQueue(const QueueContainerType::FKey& Key)
	{

	}

protected:
	TMap<UScriptStruct*, TInstancedStruct<FPresentationQueueContainerBase>> mQueues;
};
