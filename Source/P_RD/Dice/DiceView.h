// @file DiceView.h
// @brief 주사위 한 개의 시각 표현(View). 데이터/로직은 UDiceModel이 갖고 여기는 시각 전용 상태만 둔다.
// @date 2026-06-18

#pragma once

#include "RDMinimal.h"
#include "ObjectView.h"
#include "DataAsset/RarityType.h"

#include "DiceView.generated.h"

class UDiceModel;
class UTexture;

/** @brief 주사위 한 개의 시각 표현입니다. 모든 시뮬레이션 뷰의 공통 루트인 IObjectView를 구현합니다. */
// 데이터/로직(면/굴림값/사용여부)은 UDiceModel(Model)이 들고, 이 View는 모델을 참조만 한다(역참조 금지).
// View가 추가로 들고 가는 것은 "시각 전용" 값뿐이다:
//   - 희귀도 enum을 UI가 바로 쓸 색/문구로 변환한 결과(UI가 enum을 직접 모르게).
//   - 스킬 빌드에 올렸는지 같은 UI 선택 강조 상태.
//   - 3D 굴림 면 프리뷰 렌더타깃 슬롯(캡처 계층이 채움).
UCLASS(BlueprintType)
class P_RD_API UDiceView : public UObject, public IObjectView
{
	GENERATED_BODY()

public:
	/** @brief 표현할 Model을 붙이고 희귀도 색/문구를 갱신한다. */
	void BindModel(UDiceModel* InModel);

	/** @brief 스킬 빌드에 올린 주사위 강조 여부(UI 선택 상태)를 설정한다. */
	void SetSelected(bool bSelected) { mIsSelected = bSelected; }

	/** @brief 3D 굴림 면 프리뷰 렌더타깃을 받아둔다(별도 캡처 계층이 채움). */
	void SetPreviewTexture(UTexture* InTexture) { mPreviewTexture = InTexture; }

	/* ── Model에서 그대로 읽는 표시값(View는 보관하지 않고 모델을 위임 조회) ── */
	int32 GetResultValue() const;        // 굴림 결과값(0=미굴림)
	int32 GetFaceCount() const;          // 면 수(d2..d20)
	int32 GetRolledFaceIndex() const;    // 굴러진 물리 면 index
	bool IsRolled() const;
	bool IsUsed() const;

	/* ── View 전용 시각 값 ── */
	bool IsSelected() const { return mIsSelected; }
	FLinearColor GetRarityColor() const { return mRarityColor; }
	FText GetRarityText() const { return mRarityText; }
	UTexture* GetPreviewTexture() const { return mPreviewTexture; }

	/** @brief 이 View가 표현하는 데이터 모델. */
	UDiceModel* GetModel() const { return mModel.Get(); }

private:
	/** @brief Model의 희귀도 enum을 UI 표시용 색/문구로 변환해 캐싱한다. */
	void RefreshRarityVisual();

	/** @brief 표현 대상 데이터 모델. View가 Model을 참조하며, Model은 View를 모른다(단방향). */
	UPROPERTY()
	TWeakObjectPtr<UDiceModel> mModel;

	/** @brief 스킬 빌드에 올린 주사위 강조용 UI 선택 상태. */
	UPROPERTY()
	bool mIsSelected = false;

	/** @brief 희귀도 테두리/배경 색. UI가 희귀도 enum을 직접 모르게 어댑터 대신 View가 변환해 담는다. */
	UPROPERTY()
	FLinearColor mRarityColor = FLinearColor::White;

	/** @brief 희귀도 라벨 문구(Common/Rare/Epic 등). */
	UPROPERTY()
	FText mRarityText;

	/** @brief 3D 굴림 면 프리뷰 렌더타깃. 캡처 계층이 채우기 전엔 비어 있을 수 있다. */
	UPROPERTY()
	TObjectPtr<UTexture> mPreviewTexture = nullptr;
};
