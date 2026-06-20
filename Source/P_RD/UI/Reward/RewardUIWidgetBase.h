#pragma once

/** @brief 전투 보상 화면 WBP가 상속하는 베이스입니다. 보상 뷰모델에 묶여 표시·입력만 담당합니다. */
// @file RewardUIWidgetBase.h
// 레이아웃은 전부 WBP(create_reward_wbp.py)가 만든다. C++은 BindWidget 위젯에 값/브러시만 넣고
// 런타임으로 위젯을 생성하지 않는다(컨벤션). 보상 항목은 방 타입상 0~1개라 항목 카드도 static 1개.

#include "RDMinimal.h"
#include "UI/RDUserWidget.h"
#include "UI/Reward/RewardUITypes.h"

#include "RewardUIWidgetBase.generated.h"

class UBorder;
class UButton;
class UImage;
class UTextBlock;
class URewardUIModel;

/** @brief '받기'로 보상 화면이 닫혔음을 알린다(승리 흐름이 이걸 받아 다음 단계=월드맵을 연다). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRewardClosed);

UCLASS(Abstract)
class P_RD_API URewardUIWidgetBase : public URDUserWidget
{
	GENERATED_BODY()

public:
	/** @brief 보상 화면이 전투 HUD/월드맵 위에 뜨도록 팝업 뷰포트 ZOrder를 설정한다. */
	URewardUIWidgetBase(const FObjectInitializer& ObjectInitializer);

	/** @brief 보상 뷰모델에 연결하고 갱신 알림을 구독한다. */
	UFUNCTION(BlueprintCallable, Category = "Reward|UI")
	void BindUIModel(URewardUIModel* InUIModel);

	/** @brief WBP가 현재 UIModel을 읽기 위한 접근자. */
	UFUNCTION(BlueprintPure, Category = "Reward|UI")
	URewardUIModel* GetUIModel() const { return mUIModel; }

	/** @brief '받기' 버튼이 호출. 의도만 뷰모델로 넘긴다. */
	UFUNCTION(BlueprintCallable, Category = "Reward|UI")
	void Claim();

	/** @brief '받기'로 보상 화면이 닫혔을 때 발생. 승리 흐름이 구독해 다음 단계(월드맵)를 연다. */
	UPROPERTY(BlueprintAssignable, Category = "Reward|UI")
	FOnRewardClosed OnClosed;

protected:
	/** @brief 받기 버튼 클릭을 연결하고, 이미 바인딩된 보상이 있으면 즉시 그린다. */
	virtual void NativeConstruct() override;

	/** @brief 화면 이탈 시 UIModel 델리게이트 구독을 정리한다. */
	virtual void NativeDestruct() override;

private:
	/** @brief '받기' 버튼 클릭 → Claim 의도 전달 후 화면을 닫는다. */
	UFUNCTION() void HandleCloseClicked();

	/** @brief UIModel 변경 알림을 받아 BindWidget 위젯에 값을 채운다. */
	UFUNCTION() void HandleUIChanged();

	/** @brief 선택지(항목) 변경 알림을 받아 항목 카드를 채운다. */
	UFUNCTION() void HandleChoicesChanged();

	/** @brief 현재 UIModel 구독을 해제하고 참조를 비운다. */
	void UnbindUIModel();

	/** @brief 제목 + 골드/경험치 값을 칩 위젯에 반영한다(아이콘은 WBP가 고정). */
	void RefreshSummary();

	/** @brief 그 방의 보상 항목(0~1개)을 항목 카드에 반영하거나 카드를 숨긴다. */
	void RefreshItem();

	/** @brief '받기' 버튼에 타이틀/캐릭터선택과 동일한 다크판타지 버튼 텍스처를 입힌다(위젯 생성 아님 — 스타일만). */
	void ApplyClaimButtonStyle() const;

protected:
	// ---- WBP BindWidget (이름은 create_reward_wbp.py 위젯명과 일치). C++은 값/브러시만 설정. ----
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mTitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mGoldValue;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mGoldSub;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mExpValue;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mExpSub;

	/** @brief 보상 항목 카드(희귀도 테두리 색 + 있고없음 표시용). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> mItemCard;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> mItemIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mItemName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mItemDetail;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> mCloseButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mCloseButtonText;

	/** @brief 현재 바인딩된 보상 상태 소유자; 위젯은 이 객체를 소유하지 않고 구독만 한다. */
	UPROPERTY(BlueprintReadOnly, Category = "Reward|UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<URewardUIModel> mUIModel;
};
