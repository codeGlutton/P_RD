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
 * 후보를 누르면 상세만 바뀌고, 별도의 추가 버튼으로 파티에 넣는다. 상세를
 * 둘러보는 입력과 편성 입력을 분리해 실수로 파티가 바뀌지 않게 한다.
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
class USkillDetailOverlayPresenter;
class USkillTacticalDiagramWidget;
class UTextBlock;
class UUserWidget;
class UWidget;
struct FShopMercenaryUI;
struct FShopMercenaryPartySlotUI;

/** @brief 이력서 한 장이 지금 어떤 상태인가. */
UENUM(BlueprintType)
enum class EMercenaryCardState : uint8
{
	Open = 0,		/** 고를 수 있다 */
	Reviewing,		/** 지금 상세를 보는 중. 마지막으로 누른 후보다 */
	Chosen,			/** 정해짐 */
	Full			/** 자리가 찼다. 눌러도 되고, 그러면 마지막 자리를 뺏는다 */
};

/**
 * @brief 출발을 눌렀을 때. 파티 저장과 화면 전환은 받는 쪽이 한다.
 *
 * 넘기는 것은 식별자뿐이다. 표시용 값까지 넘기면 받는 쪽이 그걸로 런을
 * 만들고 싶어지는데, 런을 만드는 데 필요한 것은 식별자밖에 없다.
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMercenaryPartyConfirmed,
	const TArray<FPrimaryAssetId>& /*Chosen*/);

/** @brief 뒤로 버튼을 눌러 타이틀로 돌아가 달라는 요청. */
DECLARE_MULTICAST_DELEGATE(FOnMercenaryHireBackRequested);

/** @brief 상점 모드에서 후보 판매 슬롯과 교체 대상 파티 슬롯을 확정한다. */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnShopMercenaryHireRequested,
	int32 /*CandidateSlotIndex*/, int32 /*PartySlotIndex*/);

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
	UPROPERTY() TObjectPtr<UWidget> mSelected = nullptr;
	/** @brief 특성 한 줄. 왜 이 사람을 데려가는지. 설명 문구를 그대로 건다. */
	UPROPERTY() TObjectPtr<UTextBlock> mTrait = nullptr;
	/** @brief 검토 중 금색 테두리. 판에 없어 낱장으로 얹는다. */
};

/**
 * @brief 파티 슬롯 한 칸.
 */
USTRUCT()
struct FMercenarySlotWidgets
{
	GENERATED_BODY()

	UPROPERTY() TObjectPtr<UWidget> mRoot = nullptr;
	UPROPERTY() TObjectPtr<UButton> mButton = nullptr;
	UPROPERTY() TObjectPtr<UImage> mFace = nullptr;
	UPROPERTY() TObjectPtr<UTextBlock> mName = nullptr;
	UPROPERTY() TObjectPtr<UTextBlock> mLevel = nullptr;
	UPROPERTY() TObjectPtr<UWidget> mPlus = nullptr;

	/** 빌더가 구운 이름 밴드 크기. 레벨 줄이 없는 화면에서 밴드를 칸 전체로 늘일 때 기준. */
	FVector2D mNameBandBase = FVector2D::ZeroVector;
};

/**
 * @brief 용병 선택 화면.
 */
UCLASS()
class P_RD_API UMercenaryHireWidget : public URDUserWidget
{
	GENERATED_BODY()

public:
	UMercenaryHireWidget(const FObjectInitializer& ObjectInitializer);

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
	 * @brief 기존 용병 선택 화면을 상점의 1인 고용/교체 모드로 연다.
	 * @details 왼쪽은 판매 후보, 오른쪽은 현재 파티, 가운데 추가 버튼은 고용
	 * 결제로 바뀐다. 실제 과금과 모델 교체는 ShopGameMode가 수행한다.
	 */
	void SetShopMode(const TArray<FShopMercenaryUI>& Candidates,
		const TArray<FShopMercenaryPartySlotUI>& PartySlots, int32 CurrentGold);

	/** @brief 상점 모드를 해제하고 처음 파티 구성 규칙으로 되돌린다. */
	void ClearShopMode();

	/**
	 * @brief 이력서 한 장을 누른다.
	 *
	 * 버튼이 부르는 것과 같은 자리다. 시험이 규칙을 그대로 다시 적는 대신
	 * 이걸 부르게 하려고 열어 둔다 -- 규칙을 두 번 적으면 시험은 코드가
	 * 틀려도 통과한다.
	 */
	void ClickCard(int32 CardIndex);

	/** @brief 현재 검토 중인 용병을 명시적으로 파티에 추가한다. */
	void ClickAdd();

	/** @brief 채워진 파티 슬롯을 눌러 그 용병을 파티에서 뺀다. */
	void ClickPartySlot(int32 SlotIndex);

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

	/** @brief 뒤로 버튼을 눌렀을 때 알려준다. 화면 전환은 프론트엔드가 맡는다. */
	FOnMercenaryHireBackRequested mOnBackRequested;

	/** @brief 상점 고용 확정. 부모 상점 WBP가 UIModel 의도로 변환한다. */
	FOnShopMercenaryHireRequested mOnShopHireRequested;

#if WITH_DEV_AUTOMATION_TESTS
	/** @brief 자동화가 실제 세로/가로 재배치를 한 프레임 기다리지 않고 검사한다. */
	void ApplyResponsiveLayoutForTest(const FVector2D& ViewportSize)
	{
		ApplyResponsiveLayout(ViewportSize);
	}
	/** @brief 실제 단일 클릭과 같은 상세 발화 경로를 검증한다. */
	void TriggerSkillClickForTest(int32 SlotIndex) { HandleSkillClicked(SlotIndex); }
	int32 GetSkillDataIndexForSlotForTest(int32 SlotIndex) const;
	UUserWidget* GetSkillDetailOverlayForTest() const { return mSkillDetailOverlay; }
	USkillDetailOverlayPresenter* GetSkillDetailPresenterForTest() const
	{
		return mSkillDetailPresenter;
	}
#endif

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;
	virtual void ApplyCloseUI() override;

private:
	void CacheWidgets();
	void ApplyMarchboundPortraits();
	void ApplyResponsiveLayout(const FVector2D& ViewportSize);
	void Refresh();
	void RefreshCard(int32 CardIndex);
	void RefreshBottomBar();
	void RefreshDetail();
	int32 GetSkillDataIndexForSlot(const FFrontendCharacterOption& Option,
		int32 SlotIndex) const;
	void HandleSkillClicked(int32 SlotIndex);
	bool EnsureSkillDetailOverlay();
	void ShowSkillDetailOverlay(const FFrontendSkillOption& Skill);
	void HideSkillDetailOverlay();

	void ToggleChoice(int32 CardIndex);

	UFUNCTION() void HandleCardClicked_0();
	UFUNCTION() void HandleCardClicked_1();
	UFUNCTION() void HandleCardClicked_2();
	UFUNCTION() void HandleCardClicked_3();
	UFUNCTION() void HandleCardClicked_4();
	UFUNCTION() void HandleCardClicked_5();
	UFUNCTION() void HandlePartySlotClicked_0();
	UFUNCTION() void HandlePartySlotClicked_1();
	UFUNCTION() void HandlePartySlotClicked_2();
	UFUNCTION() void HandleAddClicked();
	UFUNCTION() void HandleDepartClicked();
	UFUNCTION() void HandleBackClicked();
	UFUNCTION() void HandleSkillClicked_0();
	UFUNCTION() void HandleSkillClicked_1();
	UFUNCTION() void HandleSkillClicked_2();
	UFUNCTION() void HandleSkillClicked_3();
	UFUNCTION() void HandleSkillClicked_4();
	UFUNCTION() void HandleSkillClicked_5();
	UFUNCTION() void HandleSkillDetailCloseClicked();

	/** @brief 화면에 걸린 후보들. 비어 있으면 시안 값이 그대로 남는다. */
	UPROPERTY() TArray<FFrontendCharacterOption> mCrew;
	UPROPERTY() TArray<FShopMercenaryUI> mShopCandidates;
	UPROPERTY() TArray<FShopMercenaryPartySlotUI> mShopPartySlots;

	UPROPERTY() TArray<FMercenaryCardWidgets> mCards;
	UPROPERTY() TArray<FMercenarySlotWidgets> mSlots;

	UPROPERTY() TObjectPtr<UTextBlock> mPartyCountText = nullptr;
	UPROPERTY() TObjectPtr<UButton> mAddButton = nullptr;
	UPROPERTY() TObjectPtr<UTextBlock> mAddLabel = nullptr;
	UPROPERTY() TObjectPtr<UButton> mDepartButton = nullptr;
	UPROPERTY() TObjectPtr<UTextBlock> mDepartLabel = nullptr;
	/** 상점 고용비용 표기가 폰트를 줄이므로, 복원할 원래 폰트를 첫 변경 전에 보관한다. */
	TOptional<FSlateFontInfo> mDepartLabelBaseFont;
	/** 빌더가 구운 광학 오프셋. 폰트를 줄일 때 비례 축소해 다시 건다. */
	TOptional<FVector2D> mDepartLabelBaseShift;
	UPROPERTY() TObjectPtr<UButton> mBackButton = nullptr;
	UPROPERTY() TObjectPtr<UTextBlock> mDetailName = nullptr;
	UPROPERTY() TObjectPtr<UTextBlock> mDetailHP = nullptr;
	UPROPERTY() TObjectPtr<UTextBlock> mDetailAP = nullptr;
	UPROPERTY() TObjectPtr<UTextBlock> mDetailSpeed = nullptr;
	UPROPERTY() TObjectPtr<UImage> mHeroIllustration = nullptr;
	/** 현재 영웅의 전용 생성 색상 배경. 전경 원화와 별개로 화면 전체를 채운다. */
	UPROPERTY() TObjectPtr<UImage> mHeroGeneratedBackground = nullptr;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> mDetailSkills;
	// 스킬 칸 그림. 아이콘이 있으면 그림을, 없으면 글자(mDetailSkills)를 보인다.
	UPROPERTY() TArray<TObjectPtr<UImage>> mDetailSkillIcons;
	UPROPERTY() TArray<TObjectPtr<UButton>> mDetailSkillButtons;

	/** @brief 전투 HUD와 같은 상세판. 별도 viewport 겹으로 띄운다. */
	UPROPERTY() TSubclassOf<UUserWidget> mSkillDetailOverlayClass;
	UPROPERTY() TSubclassOf<USkillTacticalDiagramWidget> mSkillTacticalDiagramClass;
	/** 전투 HUD와 공용인 리치 상세 프레젠터. mSkillDetailOverlay는 그 안의 겹을 비춘다. */
	UPROPERTY(Transient) TObjectPtr<USkillDetailOverlayPresenter> mSkillDetailPresenter = nullptr;
	UPROPERTY(Transient) TObjectPtr<UUserWidget> mSkillDetailOverlay = nullptr;

	/** @brief 신규 Marchbound 레이아웃에서만 추가 표시 규칙을 사용한다. */
	bool mIsMarchboundLayout = false;
	bool mHasAppliedResponsiveLayout = false;
	bool mIsPortraitLayout = false;
	FVector2D mLastResponsiveSize = FVector2D::ZeroVector;

	/** @brief 고른 이력서 번호. 누른 차례가 곧 파티 칸 순서다. */
	TArray<int32> mChosen;

	/** @brief 지금 검토 중인 이력서. 없으면 INDEX_NONE. */
	int32 mReviewing = INDEX_NONE;

	int32 mPartySize = 3;
	int32 mShopTargetPartyViewIndex = 0;
	int32 mShopGold = 0;
	bool mIsShopMode = false;

	/**
	 * @brief 출발에 필요한 최소 인원.
	 *
	 * 용병 자료가 다 들어오면 mPartySize 와 같게 올린다. 지금은 셋을 채울
	 * 자료가 없어 한 명으로 출발한다.
	 */
	int32 mMinPartySize = 1;
};
