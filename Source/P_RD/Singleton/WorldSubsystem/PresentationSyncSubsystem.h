/*****************************************************************//**
 * @file   PresentationSyncSubsystem.h
 * @brief  연출 동기화 서브시스템 구현 헤더
 * @author 모호재
 * @date   2026-05-28
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Singleton/WorldSubsystem/PresentationBarrier.h"
#include "PresentationSyncSubsystem.generated.h"

 // Presentation Sync 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogPresentationSync, Log, All)

/**
 * @brief  연출 동기화 서브시스템
 */
UCLASS()
class P_RD_API UPresentationSyncSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	TSharedPtr<FPresentationBarrier> MakePresentationBarrier(FOnFinishPresentation OnFinishPresentation);
};
