/*****************************************************************//**
 * @file   BoardEventTrack.h
 * @brief  단발성 및 구간성 보드 이벤트를 동시에 지원하는 커스텀 레벨 시퀀스 트랙 헤더
 * @author 모호재
 * @date   2026-08-17
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "MovieSceneNameableTrack.h"
#include "Compilation/IMovieSceneDeterminismSource.h"
#include "BoardEventTrack.generated.h"

/**
 * @brief 단발성(Trigger) 및 구간성(Duration) 보드 이벤트를 함께 배치 및 중첩(Multi-row)할 수 있는 커스텀 레벨 시퀀스 트랙
 */
UCLASS(BlueprintType)
class P_RD_API UBoardEventTrack : public UMovieSceneNameableTrack, public IMovieSceneDeterminismSource
{
	GENERATED_BODY()

public:
	UBoardEventTrack();

	/* UMovieSceneNameableTrack 상속 */
public:
	bool SupportsType(TSubclassOf<UMovieSceneSection> SectionClass) const override;
	
	void AddSection(UMovieSceneSection& Section) override;
	void RemoveSection(UMovieSceneSection& Section) override;
	void RemoveSectionAt(int32 SectionIndex) override;

	UMovieSceneSection* CreateNewSection() override;
	const TArray<UMovieSceneSection*>& GetAllSections() const override;
	bool HasSection(const UMovieSceneSection& Section) const override;
	
	bool IsEmpty() const override;
	bool SupportsMultipleRows() const override;

#if WITH_EDITORONLY_DATA
	void PostRename(UObject* OldOuter, const FName OldName) override;

	FText GetDefaultDisplayName() const override;
#endif

	/* IMovieSceneDeterminismSource 상속 */
public:
	void PopulateDeterminismData(FMovieSceneDeterminismData& OutData, const TRange<FFrameNumber>& Range) const override;

public:
	UPROPERTY(Category = TrackEvent, EditAnywhere, meta = (DisplayName = "FireEventsWhenForwards"))
	uint32 mFireEventsWhenForwards : 1;

	UPROPERTY(Category = TrackEvent, EditAnywhere, meta = (DisplayName = "FireEventsWhenBackwards"))
	uint32 mFireEventsWhenBackwards : 1;

	/* 멤버 변수 */
private:
	UPROPERTY()
	TArray<TObjectPtr<UMovieSceneSection>> mSections;
};
