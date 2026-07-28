/*****************************************************************//**
 * @file   MercenaryHireWidget.h
 * @brief  용병 선택 게시판. 여섯 중 셋을 고른다.
 * @details
 * 화면 모양은 WBP(WBP_MercenaryHire)에 있고 여기서는 값과 규칙만 다룬다.
 * 위젯은 이름으로 찾는다 -- WBP 를 파이썬이 구우므로 BindWidget 으로 묶으면
 * 이름이 하나 어긋날 때 컴파일이 깨지고, 그러면 다시 굽는 것조차 못 한다.
 * 못 찾은 위젯은 건너뛴다. 배치안이 어떤 칸을 안 그리기로 하는 것은 설계이지
 * 오류가 아니다.
 *
 * 걸어 놓을 값은 FFrontendCharacterOption 을 그대로 받는다. 캐릭터 선택
 * 화면이 쓰던 그릇이고 이름/역할/HP/초상/설명/식별자를 이미 다 들고 있다.
 * 같은 것을 담는 그릇을 하나 더 두면 둘이 어긋나는 날이 오고, 그날 어느
 * 쪽이 맞는지 아무도 모른다.
 *
 * 고르는 것은 두 단계다. 다른 이력서를 처음 누르면 검토 대상이 되고, 같은
 * 것을 다시 누르면 정해지거나 풀린다. 한 번에 정해지면 잘못 누른 것을
 * 되돌리기 번거롭다.
 *
 * 값을 치르는 개념은 없다. 처음 시작할 때는 그냥 셋을 고른다 -- 돈으로 사는
 * 것은 나중에 상점에서 들어온다.
 * @author 박용수
 * @date   2026-07-27
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Frontend/CharacterSelectTypes.h"
#include "UI/RDUserWidget.h"
#include "MercenaryHireWidget.generated.h"

class UButton;
class UImage;
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

/**
 * @brief 출발을 눌렀을 때. 파티 저장과 화면 전환은 받는 쪽이 한다.
 *
 * 넘기는 것은 식별자뿐이다. 표시용 값까지 넘기면 받는 쪽이 그걸로 런을
 * 만들고 싶어지는데, 런을 만드는 데 필요한 것은 식별자밖에 없다.
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMercenaryPartyConfirmed,
	const TArray<FPrimaryAssetId>& /*Chosen*/);

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
	/** @brief 특성 한 줄. 왜 이 사람을 데려가는지. 설명 문구를 그대로 건다. */
	UPROPERTY() TObjectPtr<UTextBlock> mTrait = nullptr;
	/** @brief 검토 중 금색 테두리. 판에 없어 낱장으로 얹는다. */
	UPROPERTY() TObjectPtr<UWidget> mSelected = nullptr;
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
 * @brief 용병 선택 화면.
 */
UCLASS()
class P_RD_API UMercenaryHireWidget : public URDUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * @brief 게시판에 걸 후보를 넣는다.
	 *
	 * AFrontendGameMode::GetCharacterOptions() 가 준 것을 그대로 넘기면 된다.
	 * 안 넘기면 WBP 에 구워 둔 시안 값이 그대로 남는다 -- 콘솔로 화면만 열어
	 * 볼 때가 그렇다.
	 *
	 * @param Options   걸어 놓을 후보들
	 * @param PartySize 데리고 갈 인원
	 */
	void SetCharacterOptions(const TArray<FFrontendCharacterOption>& Options,
		int32 PartySize = 3);

	/**
	 * @brief 이력서 한 장을 누른다.
	 *
	 * 버튼이 부르는 것과 같은 자리다. 시험이 규칙을 그대로 다시 적는 대신
	 * 이걸 부르게 하려고 열어 둔다 -- 규칙을 두 번 적으면 시험은 코드가
	 * 틀려도 통과한다.
	 */
	void ClickCard(int32 CardIndex);

	/** @brief 이력서 한 장이 지금 어떤 상태인가. */
	EMercenaryCardState StateOf(int32 CardIndex) const;

	/** @brief 고른 이력서 번호. 누른 차례대로. */
	const TArray<int32>& GetChosenIndices() const { return mChosen; }

	/** @brief 인원을 다 채웠나. */
	bool IsReadyToDepart() const;

	/** @brief 출발. 다 안 채웠으면 아무 일도 안 한다. 출발 버튼이 부른다. */
	void ConfirmParty();

	/** @brief 출발을 눌렀을 때 알려준다. 고른 차례대로 온다. */
	FOnMercenaryPartyConfirmed mOnPartyConfirmed;

protected:
	virtual void NativeConstruct() override;

private:
	void CacheWidgets();
	void Refresh();
	void RefreshCard(int32 CardIndex);
	void RefreshBottomBar();

	void ToggleChoice(int32 CardIndex);

	UFUNCTION() void HandleCardClicked_0();
	UFUNCTION() void HandleCardClicked_1();
	UFUNCTION() void HandleCardClicked_2();
	UFUNCTION() void HandleCardClicked_3();
	UFUNCTION() void HandleCardClicked_4();
	UFUNCTION() void HandleCardClicked_5();
	UFUNCTION() void HandleDepartClicked();

	/** @brief 화면에 걸린 후보들. 비어 있으면 시안 값이 그대로 남는다. */
	UPROPERTY() TArray<FFrontendCharacterOption> mCrew;

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
