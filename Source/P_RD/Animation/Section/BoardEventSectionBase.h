/*****************************************************************//**
 * @file   BoardEventSectionBase.h
 * @brief  보드 레벨 시퀀스 이벤트를 처리하는 최상위 공통 섹션 정의 헤더
 * @author 모호재
 * @date   2026-08-17
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "MovieSceneSection.h"
#include "Animation/Channel/BoardEventChannel.h"
#include "BoardEventSectionBase.generated.h"

/**
 * @brief 단발성 및 구간성 보드 이벤트 섹션의 공통 베이스 클래스
 */
UCLASS(BlueprintType, Abstract)
class P_RD_API UBoardEventSectionBase : public UMovieSceneSection
{
	GENERATED_BODY()

public:
	UBoardEventSectionBase();

#if WITH_EDITOR
	/* UMovieSceneSection 상속 */
public:
	virtual void PostRename(UObject* OldOuter, const FName OldName) override;
	virtual void PostDuplicate(bool bDuplicateForPIE) override;
	virtual void RemoveForCook() override;
#endif
};
