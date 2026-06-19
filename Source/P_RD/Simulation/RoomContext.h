/*****************************************************************//**
 * @file   RoomContext.h
 * @brief  방의 흐름 상태 정의 헤더
 * @author 모호재
 * @date   2026-06-16
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "RoomContext.generated.h"

class URoomInstance;
class UObjectModelFactory;
class UEventLogger;

/**
 * @brief  방의 흐름 상태
 */
USTRUCT(Blueprintable)
struct FRoomContext
{
	GENERATED_BODY()

public:
	// @brief 생성 및 제거될 데이터 객체
	UPROPERTY(Category = Instance, EditAnywhere, BlueprintReadWrite)
	TObjectPtr<URoomInstance> mRoomInstance;

public:
	// @brief 사용할 모델 팩토리
	UPROPERTY(Category = Factory, EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UObjectModelFactory> mModelFactory;

	// @brief 사용할 이벤트 로거
	UPROPERTY(Category = Handler, EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UEventLogger> mEventLogger;
};

