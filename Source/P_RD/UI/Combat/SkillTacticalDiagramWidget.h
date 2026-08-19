/*****************************************************************//**
 * @file   SkillTacticalDiagramWidget.h
 * @brief  스킬 상세의 데이터 기반 9x9 사각 전술 범위 WBP 베이스.
 * @date   2026-08-19
 *********************************************************************/

#pragma once

#include "Blueprint/UserWidget.h"
#include "UI/Combat/CombatUITypes.h"

#include "SkillTacticalDiagramWidget.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(
	FSkillTacticalPreviewVisibilityChanged, bool);

/**
 * @brief 실제 스킬 DTO를 사각 타일 전술판으로 바꾸는 전용 WBP 베이스.
 * @details 월드/SceneCapture를 보지 않는다. WBP에 배치된 셀과 마커를
 *          FSkillTargetingUI로만 갱신해 모바일에서도 일정하게 보인다.
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API USkillTacticalDiagramWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** @brief 상세 DTO의 사거리/효과 범위를 WBP 셀과 라벨에 반영한다. */
	void ApplySkillDetail(const FSkillDetailUI& Detail);
	/** @brief 사정 범위 버튼과 같은 동작. 자동화와 외부 UI에서도 재사용한다. */
	void ShowSelectRangePreview();
	/** @brief 영향 범위 버튼과 같은 동작. */
	void ShowEffectRangePreview();
	/** @brief 열린 범위판을 닫고 설명 상태로 돌아간다. */
	void HideRangePreview();
	/** @brief 왼쪽 요약 버튼의 지속 선택 브러시를 결정한다. */
	bool IsSelectRangePreviewShown() const
	{
		return mPreviewMode == EPreviewMode::SelectRange;
	}
	bool IsEffectRangePreviewShown() const
	{
		return mPreviewMode == EPreviewMode::EffectRange;
	}
	FSkillTacticalPreviewVisibilityChanged& OnPreviewVisibilityChanged()
	{
		return mPreviewVisibilityChanged;
	}

protected:
	void NativeConstruct() override;
	void NativePreConstruct() override;

private:
	enum class EPreviewMode : uint8
	{
		Hidden,
		SelectRange,
		EffectRange
	};

	static constexpr int32 RowCount = 9;
	static constexpr int32 ColumnCount = 9;
	static constexpr int32 CasterRow = 4;
	static constexpr int32 CasterColumn = 4;
	static constexpr int32 RouteDotCount = 9;

	void CacheWidgetParts();
	void EnsureVisualAssets();
	void RefreshDiagram();
	void SetPreviewMode(EPreviewMode NewMode);
	UFUNCTION() void HandleSelectRangeClicked();
	UFUNCTION() void HandleEffectRangeClicked();
	bool SelectCovers(int32 Row, int32 Column, int32 OriginRow,
		int32 OriginColumn, int32 Range, ECombatSkillSelectShapeUI Shape) const;
	bool HitCovers(int32 Row, int32 Column, int32 TargetRow,
		int32 TargetColumn, int32 Range, ECombatSkillHitShapeUI Shape) const;
	FVector2D CellCenter(int32 Row, int32 Column) const;
	void PlaceCentered(UWidget* Widget, const FVector2D& Center,
		const FVector2D& Size) const;

	FSkillDetailUI mDetail;
	bool mHasDetail = false;
	EPreviewMode mPreviewMode = EPreviewMode::Hidden;
	FSkillTacticalPreviewVisibilityChanged mPreviewVisibilityChanged;

	UPROPERTY(Transient) TArray<TObjectPtr<class UImage>> mCells;
	UPROPERTY(Transient) TArray<TObjectPtr<class UImage>> mRouteDots;
	UPROPERTY(Transient) TObjectPtr<UImage> mEffectRing;
	UPROPERTY(Transient) TObjectPtr<UImage> mCasterRing;
	UPROPERTY(Transient) TObjectPtr<UImage> mCasterMarker;
	UPROPERTY(Transient) TObjectPtr<UImage> mTargetRing;
	UPROPERTY(Transient) TObjectPtr<UImage> mTargetMarker;
	UPROPERTY(Transient) TObjectPtr<class UWidget> mBackdrop;
	UPROPERTY(Transient) TObjectPtr<UWidget> mBoardTilt;
	UPROPERTY(Transient) TObjectPtr<class UButton> mSelectButton;
	UPROPERTY(Transient) TObjectPtr<UButton> mEffectButton;
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> mSelectLegend;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> mEffectLegend;

	UPROPERTY(Transient) TObjectPtr<class UTexture2D> mNeutralTileTexture;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> mSelectTileTexture;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> mEffectTileTexture;
};
