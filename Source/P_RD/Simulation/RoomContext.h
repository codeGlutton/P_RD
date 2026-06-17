/*****************************************************************//**
 * @file   RoomContext.h
 * @brief  방의 흐름 상태 정의 헤더
 * @author 모호재
 * @date   2026-06-16
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "RoomContext.generated.h"

class IModelFactory;
class IEventLogger;

/**
 * @brief  방의 흐름 상태
 */
USTRUCT(Blueprintable)
struct FRoomContext
{
	GENERATED_BODY()

public:
	//UPROPERTY(Category = Model, EditAnywhere, BlueprintReadWrite)
	//TObjectPtr<USRPGCombatModel> mCombatModel;

public:
	UPROPERTY(Category = Factory, EditAnywhere, BlueprintReadWrite)
	TScriptInterface<IModelFactory> mModelFactory;

	//UPROPERTY(Category = Notifier, EditAnywhere, BlueprintReadWrite)
	//TScriptInterface<IViewNotifier> mViewNotifier;

	UPROPERTY(Category = Handler, EditAnywhere, BlueprintReadWrite)
	TScriptInterface<IEventLogger> mEventLogger;
};

