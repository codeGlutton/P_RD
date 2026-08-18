/*****************************************************************//**
 * @file   BoardEventTriggerSection.h
 * @brief  단발성 이벤트를 발동하는 레벨 시퀀스 커스텀 섹션 정의 헤더
 * @author 모호재
 * @date   2026-08-17
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Animation/Section/BoardEventSectionBase.h"
#include "Animation/Channel/BoardEventChannel.h"
#include "EntitySystem/IMovieSceneEntityProvider.h"
#include "BoardEventTriggerSection.generated.h"

/**
 * @brief 단발성 이벤트를 발동하는 레벨 시퀀스 커스텀 섹션 (AnimNotify 모방)
 */
UCLASS(BlueprintType)
class P_RD_API UBoardEventTriggerSection : public UBoardEventSectionBase, public IMovieSceneEntityProvider
{
	GENERATED_BODY()

public:
	UBoardEventTriggerSection(const FObjectInitializer& ObjInit);

	/* IMovieSceneEntityProvider 상속 */
public:
	void ImportEntityImpl(UMovieSceneEntitySystemLinker* EntityLinker, const FEntityImportParams& Params, FImportedEntity* OutImportedEntity) override;

public:
	UPROPERTY()
	FBoardEventTriggerChannel mEventChannel;
};
