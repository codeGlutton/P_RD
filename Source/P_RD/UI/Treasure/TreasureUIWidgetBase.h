/*****************************************************************//**
 * @file   TreasureUIWidgetBase.h
 * @brief  보물방 화면 WBP가 상속하는 베이스 정의 헤더
 * @author 이문환
 * @date   2026-08-05
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "UI/RDUserWidget.h"
#include "UI/Treasure/TreasureUITypes.h"
#include "TreasureUIWidgetBase.generated.h"

class UButton;
class UHorizontalBox;
class UTextBlock;
class UTreasureUIModel;

/** @brief 보물방 화면 WBP 베이스. 뷰모델에 묶여 표시·입력만 담당 */
// 이 베이스를 상속한 WBP는:
// - BindUIModel()로 UTreasureUIModel에 연결하면 C++가 BindWidget 위젯에 값을 채움
// - 상자 개봉은 OpenBox(), 나가기는 Leave()로 의도만 전달
// 위젯은 보상 지급을 하지 않음. 진실은 게임플레이에 있음
UCLASS(Abstract)
class P_RD_API UTreasureUIWidgetBase : public URDUserWidget
{
	GENERATED_BODY()

public:
	/** @brief 보물방 화면이 다른 UI 위에 뜨도록 팝업 ZOrder 설정 */
	UTreasureUIWidgetBase(const FObjectInitializer& ObjectInitializer);

	/** @brief 보물방 뷰모델에 연결하고 갱신 알림 구독 */
	UFUNCTION(BlueprintCallable, Category = "Treasure|UI")
	void BindUIModel(UTreasureUIModel* InUIModel);

	/** @brief WBP가 보상 카드를 그릴 때 현재 UIModel을 읽기 위한 접근자 */
	UFUNCTION(BlueprintPure, Category = "Treasure|UI")
	UTreasureUIModel* GetUIModel() const { return mUIModel; }

	/** @brief 상자 개봉이 확정됐을 때 호출 (보통 개봉 연출 뒤). 의도만 뷰모델로 전달 */
	UFUNCTION(BlueprintCallable, Category = "Treasure|UI")
	void OpenBox();

	/** @brief 나가기 버튼이 호출. 의도만 뷰모델로 전달 */
	UFUNCTION(BlueprintCallable, Category = "Treasure|UI")
	void Leave();

protected:
	/** @brief 보물방 값이 들어왔을 때 호출. WBP가 추가 연출을 얹는 지점 (선택) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Treasure|UI")
	void OnTreasureRefreshed();

	/** @brief 버튼 클릭을 연결하고, 이미 바인딩된 모델이 있으면 즉시 그림 */
	virtual void NativeConstruct() override;

	/** @brief 화면 이탈 시 UIModel 델리게이트 구독 정리 */
	virtual void NativeDestruct() override;

private:
	/** @brief 개봉 버튼 클릭 → 개봉 의도 전달 */
	UFUNCTION() void HandleOpenClicked();

	/** @brief 나가기 버튼 클릭 → 나가기 의도 전달 후 화면 닫기 */
	UFUNCTION() void HandleCloseClicked();

	/** @brief UIModel 변경 알림 수신. 보상 도메인 알림만 처리 */
	UFUNCTION() void HandleUIChanged(ETreasureUIDomain Domain);

	/** @brief 현재 UIModel 구독을 해제하고 참조를 비움 */
	void UnbindUIModel();

	/** @brief 현재 모델의 상자 상태/보상 카드를 BindWidget 위젯에 반영 */
	void RefreshView();

protected:
	// ---- WBP BindWidget (WBP 위젯 이름은 아래 멤버명과 일치해야 자동 연결) ----
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mTitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> mItemBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> mOpenButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mOpenButtonText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> mCloseButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mCloseButtonText;

	/** @brief 현재 바인딩된 보물방 상태 소유자; 위젯은 이 객체를 소유하지 않고 구독만 함 */
	UPROPERTY(BlueprintReadOnly, Category = "Treasure|UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTreasureUIModel> mUIModel;
};
