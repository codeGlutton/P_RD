/*****************************************************************//**
 * @file   BoardEventChannelCurveModel.h
 * @brief  보드 이벤트 채널의 커브 에디터 모델 및 채널 에디터 인터페이스 헬퍼 정의 헤더
 * @author 모호재
 * @date   2026-08-19
 *********************************************************************/

#pragma once

#include "RDEditorMinimal.h"
#include "CurveModel.h"
#include "SequencerChannelInterface.h"
#include "Animation/Channel/BoardEventChannel.h"
#include "MovieSceneClipboard.h"

#include "Channels/MovieSceneChannelHandle.h"

class FCurveEditor;
class ISequencer;
class UMovieSceneSection;
class UObject;
struct FCurveAttributes;
struct FCurveEditorScreenSpace;
struct FKeyAttributes;
struct FKeyDrawInfo;
struct FKeyPosition;
struct FMovieSceneEventChannel;

namespace MovieSceneClipboard
{
	template<> inline FName GetKeyTypeName<FBoardEventTriggerData>()
	{
		return "BoardEventTriggerData";
	}
}

/**
 * @brief 보드 이벤트 채널의 커브 에디터 모델 클래스 (필요시 커브 그래프 확장 지원)
 */
class FBoardEventChannelCurveModel : public FCurveModel
{
public:
	FBoardEventChannelCurveModel(TMovieSceneChannelHandle<FBoardEventTriggerChannel> InChannel, UMovieSceneSection* InOwningSection, TWeakPtr<ISequencer> InWeakSequencer);

	/* FCurveModel 상속 */
public:
	const void* GetCurve() const override;

	void Modify() override;

	void DrawCurve(const FCurveEditor& CurveEditor, const FCurveEditorScreenSpace& ScreenSpace, TArray<TTuple<double, double>>& InterpolatingPoints) const override;
	void GetKeys(double MinTime, double MaxTime, double MinValue, double MaxValue, TArray<FKeyHandle>& OutKeyHandles) const override;
	void GetKeyDrawInfo(ECurvePointType PointType, const FKeyHandle InKeyHandle, FKeyDrawInfo& OutDrawInfo) const override;

	void GetKeyPositions(TArrayView<const FKeyHandle> InKeys, TArrayView<FKeyPosition> OutKeyPositions) const override;
	void SetKeyPositions(TArrayView<const FKeyHandle> InKeys, TArrayView<const FKeyPosition> InKeyPositions, EPropertyChangeType::Type ChangeType) override;

	void GetKeyAttributes(TArrayView<const FKeyHandle> InKeys, TArrayView<FKeyAttributes> OutAttributes) const override;
	void SetKeyAttributes(TArrayView<const FKeyHandle> InKeys, TArrayView<const FKeyAttributes> InAttributes, EPropertyChangeType::Type ChangeType = EPropertyChangeType::Unspecified) override;

	void GetCurveAttributes(FCurveAttributes& OutCurveAttributes) const override;
	void SetCurveAttributes(const FCurveAttributes& InCurveAttributes) override;
	void GetTimeRange(double& MinTime, double& MaxTime) const override;
	void GetValueRange(double& MinValue, double& MaxValue) const override;
	int32 GetNumKeys() const override;
	void GetNeighboringKeys(const FKeyHandle InKeyHandle, TOptional<FKeyHandle>& OutPreviousKeyHandle, TOptional<FKeyHandle>& OutNextKeyHandle) const override {}
	bool Evaluate(double ProspectiveTime, double& OutValue) const override;
	void AddKeys(TArrayView<const FKeyPosition> InKeyPositions, TArrayView<const FKeyAttributes> InAttributes, TArrayView<TOptional<FKeyHandle>>* OutKeyHandles) override;
	void RemoveKeys(TArrayView<const FKeyHandle> InKeys, double InCurrentTime) override;

	void CreateKeyProxies(TWeakPtr<FCurveEditor> InWeakCurveEditor, FCurveModelID InCurveModelID, TArrayView<const FKeyHandle> InKeyHandles, TArrayView<UObject*> OutObjects) override;

public:
	static ECurveEditorViewID EventView;

private:
	TMovieSceneChannelHandle<FBoardEventTriggerChannel> mChannelHandle;
	TWeakObjectPtr<UMovieSceneSection> mWeakSection;
	TWeakPtr<ISequencer> mWeakSequencer;
};

/**
 * @brief  커브 에디터 지원 여부 반환
 */
P_RDEDITOR_API bool SupportsCurveEditorModels(const TMovieSceneChannelHandle<FBoardEventTriggerChannel>& InChannelHandle);

/**
 * @brief  커브 에디터 모델 생성 함수
 */
P_RDEDITOR_API TUniquePtr<FCurveModel> CreateCurveEditorModel(const TMovieSceneChannelHandle<FBoardEventTriggerChannel>& InChannelHandle, const UE::Sequencer::FCreateCurveEditorModelParams& InParams);

/**
 * @brief  시퀀서 타임라인 상의 키 프레임 점(아이콘) 렌더링 파라미터 지정 함수
 */
P_RDEDITOR_API void DrawKeys(FBoardEventTriggerChannel* InChannel, TArrayView<const FKeyHandle> InKeyHandles, const UMovieSceneSection* InOwner, TArrayView<FKeyDrawParams> OutKeyDrawParams);


