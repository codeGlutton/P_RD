/*****************************************************************//**
 * @file   MercenaryHireWidget.h
 * @brief  용병 고용 게시판. 여섯 중 셋을 고른다.
 * @details
 * 화면 모양은 WBP(WBP_MercenaryHire)에 있고 여기서는 값과 규칙만 다룬다.
 * 위젯은 이름으로 찾는다 -- WBP 를 파이썬이 구우므로 BindWidget 으로 묶으면
 * 이름이 하나 어긋날 때 컴파일이 깨지고, 그러면 다시 굽는 것조차 못 한다.
 * 못 찾은 위젯은 건너뛴다. 배치안이 어떤 칸을 안 그리기로 하는 것은 설계이지
 * 오류가 아니다.
 *
 * 고르는 것은 두 단계다. 다른 이력서를 처음 누르면 검토 대상이 되고, 같은
 * 것을 다시 누르면 정해지거나 풀린다. 한 번에 정해지면 잘못 누른 것을
 * 되돌리기 번거롭다.
 *
 * 값을 치르는 개념은 없다. 처음 시작할 때는 그냥 셋을 고른다 -- 돈으로 사는
 * 것은 나중에 상점에서 들어온다. 시안에 그려진 고용비와 소지금 칸은 접어
 * 둔다.
 * @author 박용수
 * @date   2026-07-27
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MercenaryHireWidget.generated.h"

class UButton;
class UImage;
class UMercenaryBoardData;
class UMercenaryData;
class UTextBlock;
class UWidget;

/** @brief 이력서 한 장이 지금 어떤 상태인가. */
UENUM(BlueprintType)
enum class EMercenaryCardState : uint8
{
	Open = 0,		/** 고를 수 있다 */
	Reviewing,		/** 검토 중. 한 번 더 누르면 정해진다 */
	Chosen,			/** 정해짐 */
	Full			/** 자리가 다 찼다 */
};

/** @brief 출발을 눌렀을 때. 파티 저장과 화면 전환은 받는 쪽이 한다. */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMercenaryPartyConfirmed,
	const TArray<TObjectPtr<UMercenaryData>>& /*Chosen*/);

/**
 * @brief 이력서 한 장에 딸린 위젯들.
 */
USTRUCT()
struct FMercenaryCardWidgets
{
	GENERATED_BODY()

	UPROPERTY() TObjectPtr<UWidget> mRoot = nullptr;
	UPROPERTY() TObjectPtr<UButton> mButton = nullptr;
	UPROPERTY() TObjectPtr<UImage> mPortrait = nullptr;
	UPROPERTY() TObjectPtr<UTextBlock> mName = nullptr;
	UPROPERTY() TObjectPtr<UTextBlock> mRole = nullptr;
	UPROPERTY() TObjectPtr<UTextBlock> mHP = nullptr;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> mSkills;
	UPROPERTY() TObjectPtr<UTextBlock> mBadge = nullptr;
	UPROPERTY() TObjectPtr<UWidget> mSeal = nullptr;
	/** @brief 고용비 칸. 값은 안 쓰지만 접어 두려고 들고 있다. */
	UPROPERTY() TObjectPtr<UTextBlock> mCost = nullptr;
};

/**
 * @brief 파티 슬롯 한 칸.
 */
USTRUCT()
struct FMercenarySlotWidgets
{
	GENERATED_BODY()

	UPROPERTY() TObjectPtr<UWidget> mRoot = nullptr;
	UPROPERTY() TObjectPtr<UImage> mFace = nullptr;
	UPROPERTY() TObjectPtr<UTextBlock> mName = nullptr;
};

/**
 * @brief 고용 게시판 화면.
 */
UCLASS()
class P_RD_API UMercenaryHireWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** @brief 게시판에 걸 용병을 넣는다. 없으면 시안 값이 그대로 남는다. */
	void SetBoardData(UMercenaryBoardData* BoardData);

	/** @brief 출발을 눌렀을 때 알려준다. */
	FOnMercenaryPartyConfirmed mOnPartyConfirmed;

protected:
	virtual void NativeConstruct() override;

private:
	void CacheWidgets();
	void LoadBoard();
	void Refresh();
	void RefreshCard(int32 CardIndex);
	void RefreshBottomBar();

	EMercenaryCardState StateOf(int32 CardIndex) const;
	bool IsReadyToDepart() const;

	void HandleCardClicked(int32 CardIndex);
	void ToggleChoice(int32 CardIndex);

	UFUNCTION() void HandleCardClicked_0();
	UFUNCTION() void HandleCardClicked_1();
	UFUNCTION() void HandleCardClicked_2();
	UFUNCTION() void HandleCardClicked_3();
	UFUNCTION() void HandleCardClicked_4();
	UFUNCTION() void HandleCardClicked_5();
	UFUNCTION() void HandleDepartClicked();

	/** @brief 화면에 걸린 용병들. 게시판 데이터에서 편다. */
	UPROPERTY() TArray<TObjectPtr<UMercenaryData>> mCrew;

	UPROPERTY() TObjectPtr<UMercenaryBoardData> mBoardData = nullptr;

	UPROPERTY() TArray<FMercenaryCardWidgets> mCards;
	UPROPERTY() TArray<FMercenarySlotWidgets> mSlots;

	UPROPERTY() TObjectPtr<UTextBlock> mPartyCountText = nullptr;
	UPROPERTY() TObjectPtr<UTextBlock> mNoticeText = nullptr;
	UPROPERTY() TObjectPtr<UButton> mDepartButton = nullptr;
	UPROPERTY() TObjectPtr<UTextBlock> mDepartLabel = nullptr;

	/** @brief 고른 이력서 번호. 누른 차례가 곧 파티 칸 순서다. */
	TArray<int32> mChosen;

	/** @brief 지금 검토 중인 이력서. 없으면 INDEX_NONE. */
	int32 mReviewing = INDEX_NONE;

	int32 mPartySize = 3;
};
