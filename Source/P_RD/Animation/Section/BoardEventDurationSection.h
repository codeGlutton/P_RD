/*****************************************************************//**
 * @file   BoardEventDurationSection.h
 * @brief  구간성 이벤트를 유지하는 레벨 시퀀스 커스텀 섹션 정의 헤더
 * @author 모호재
 * @date   2026-08-17
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Animation/Section/BoardEventSectionBase.h"
#include "Animation/Channel/BoardEventChannel.h"
#include "EntitySystem/IMovieSceneEntityProvider.h"
#include "BoardEventDurationSection.generated.h"

/**
 * @brief 시작 시점과 종료 시점에 훅을 발동하는 레벨 시퀀스 커스텀 섹션 (AnimNotifyState 모방)
 */
UCLASS(BlueprintType)
class P_RD_API UBoardEventDurationSection : public UBoardEventSectionBase, public IMovieSceneEntityProvider
{
	GENERATED_BODY()

public:
	UBoardEventDurationSection();

	/* IMovieSceneEntityProvider 상속 */
public:
	void ImportEntityImpl(UMovieSceneEntitySystemLinker* EntityLinker, const FEntityImportParams& Params, FImportedEntity* OutImportedEntity) override;

public:
	UPROPERTY(Category = "Event", EditAnywhere, meta = (DisplayName = "Event"))
	FBoardEventDurationData mEvent;
};
