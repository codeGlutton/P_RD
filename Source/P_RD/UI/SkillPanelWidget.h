/** @brief 인게임 스킬 버튼으로 여는 임시 스킬 패널. */
// @file SkillPanelWidget.h

#pragma once

#include "RDMinimal.h"
#include "UI/CarouselPanelWidget.h"

#include "SkillPanelWidget.generated.h"

class UButton;
class UCanvasPanel;
class UTextBlock;

/** @brief 스킬 카드형 목록을 보여주는 플로팅 패널 */
// 현재 단계에서는 실제 스킬 실행 로직을 붙이지 않고, WBP로 만든 스킬 패널을 인게임에서 열고 닫을 수 있게 한다.
// 항목 선택/닫기 동작은 UCarouselPanelWidget의 공통 흐름을 그대로 사용한다.
// 왜 DicePanel과 같은 베이스를 쓰는가:
// 스킬과 주사위는 최종 게임 로직은 다르지만, "버튼으로 열림", "여러 카드 중 하나를 고름", "닫기 버튼으로 닫힘"이라는 UI 흐름은 같다.
// 이 공통 흐름을 UCarouselPanelWidget에 모아두면 스킬 데이터가 붙을 때도 패널 배치/터치 규칙을 다시 만들 필요가 없다.
// 이 클래스는 아직 스킬 사용 요청을 발생시키지 않는다.
// 추후 실제 기능이 붙으면 선택된 스킬 ID를 전투/스킬 Subsystem으로 넘기는 역할만 추가하고,
// 효과 적용 자체는 UI 바깥 로직에 맡기는 방향이 맞다.
UCLASS(BlueprintType, Blueprintable)
class P_RD_API USkillPanelWidget : public UCarouselPanelWidget
{
	GENERATED_BODY()

public:
	/** @brief 스킬 패널 기본 객체를 생성한다. */
	// 카드 슬롯 캐싱, 선택, 닫기 처리는 부모 클래스가 담당한다.
	USkillPanelWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	/** @brief WBP 목록과 런타임 상세보기 버튼을 준비한다. */
	void NativeConstruct() override;

	/** @brief 런타임 상세보기 버튼 이벤트를 해제한다. */
	void NativeDestruct() override;

	/** @brief 패널을 열 때 항상 전체 스킬 목록부터 보여준다. */
	void ApplyOpenUI() override;

	/** @brief 상세보기 상태에서 PC/에디터 마우스 스와이프를 시작한다. */
	FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** @brief 상세보기 상태에서 PC/에디터 마우스 스와이프를 확정한다. */
	FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** @brief 상세보기 상태에서 모바일 터치 스와이프를 시작한다. */
	FReply NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;

	/** @brief 상세보기 상태에서 모바일 터치 스와이프를 확정한다. */
	FReply NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;

	/** @brief 상세보기 상태에서도 부모 캐러셀 목록을 유지한다. */
	int32 GetCarouselItemDisplayCount() const override;

	/** @brief 스킬 카드를 선택하면 상세보기로 들어간다. */
	void HandleCarouselSelectionChanged(int32 PreviousIndex, int32 NewIndex) override;

private:
	/** @brief 상세보기 런타임 위젯을 만든다. */
	void EnsureSkillDetailWidgets();

	/** @brief WBP 카드 위에 얹을 목록 선택용 투명 버튼을 만든다. */
	void EnsureSkillSelectButtons();

	/** @brief 상세보기 런타임 위젯 위치를 맞춘다. */
	void ApplySkillDetailLayout() const;

	/** @brief 목록 선택용 투명 버튼 위치를 WBP 카드 목록 위치와 맞춘다. */
	void ApplySkillSelectButtonLayout() const;

	/** @brief 지정 스킬을 상세보기로 연다. */
	void ShowSkillDetail(int32 SkillIndex);

	/** @brief 상세보기에서 전체 목록으로 돌아간다. */
	void ShowSkillList();

	/** @brief 상세보기 텍스트/버튼 표시 상태를 갱신한다. */
	void RefreshSkillDetailWidgets();

	/** @brief 상세보기 여부에 따라 목록 선택용 투명 버튼 표시를 갱신한다. */
	void RefreshSkillSelectButtons();

	/** @brief 목록 상태에서 지정 스킬 카드를 선택한다. */
	void SelectSkillFromList(int32 SkillIndex);

	/** @brief 상세보기에서 지정 방향으로 스킬과 캐러셀 선택을 함께 넘긴다. */
	void MoveSkillDetail(int32 Direction);

	/** @brief 상세보기 스와이프 입력 후보를 기록한다. */
	bool BeginSkillDetailSwipe(const FVector2D& ScreenPosition);

	/** @brief 스와이프 이동량이 충분하면 이전/다음 스킬로 넘긴다. */
	bool FinishSkillDetailSwipe(const FVector2D& ScreenPosition);

	/** @brief 좌우/뒤로가기 버튼 위에서는 스와이프 캡처를 시작하지 않는다. */
	bool IsPointerOverSkillDetailControl(const FVector2D& ScreenPosition) const;

	/** @brief 목록 선택 버튼을 해당 스킬 index 핸들러에 연결한다. */
	void BindSkillSelectButton(UButton* SelectButton, int32 SkillIndex);

	/** @brief 목록 선택 버튼의 해당 스킬 index 핸들러 연결을 해제한다. */
	void UnbindSkillSelectButton(UButton* SelectButton, int32 SkillIndex);

	/** @brief 생성된 모든 목록 선택 버튼 이벤트를 해제한다. */
	void UnbindSkillSelectButtons();

	/** @brief 목록 선택 버튼 공통 처리. */
	void HandleSkillSelectButtonClicked(int32 SkillIndex);

	UFUNCTION()
	void HandleSkillDetailBackButtonClicked();

	UFUNCTION()
	void HandleSkillDetailPreviousButtonClicked();

	UFUNCTION()
	void HandleSkillDetailNextButtonClicked();

	UFUNCTION()
	void HandleSkillSelectButton0Clicked();

	UFUNCTION()
	void HandleSkillSelectButton1Clicked();

	UFUNCTION()
	void HandleSkillSelectButton2Clicked();

	UFUNCTION()
	void HandleSkillSelectButton3Clicked();

	UFUNCTION()
	void HandleSkillSelectButton4Clicked();

	UFUNCTION()
	void HandleSkillSelectButton5Clicked();

private:
	/** @brief 상세/투명 선택 버튼을 붙일 Canvas 루트. 부모 카드 위에 런타임 UI를 겹친다. */
	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> mRootCanvas;

	/** @brief 상세보기 제목 텍스트. 현재는 mock Skill N 문구를 표시한다. */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mSkillDetailTitleText;

	/** @brief 상세보기 본문 텍스트. 실제 스킬 데이터 연결 전까지 설명 fixture를 표시한다. */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mSkillDetailBodyText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> mSkillDetailPreviousButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> mSkillDetailNextButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> mSkillDetailBackButton;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> mSkillSelectButtons;

	/** @brief true면 캐러셀 카드 목록 위에 상세 오버레이가 열린 상태다. */
	bool mShowingSkillDetail = false;

	/** @brief 현재 상세보기 중인 스킬 index. INDEX_NONE이면 상세 선택이 없다. */
	int32 mDetailSkillIndex = INDEX_NONE;

	/** @brief 상세보기 화면에서 좌우 스와이프 후보를 추적 중인지 여부. */
	bool mTrackingSkillDetailSwipe = false;

	/** @brief 스와이프 시작 좌표. 종료 좌표와 비교해 이전/다음 이동 여부를 판정한다. */
	FVector2D mSkillDetailSwipeStartPosition = FVector2D::ZeroVector;
};
