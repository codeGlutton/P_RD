#pragma once

/** @brief 전투 보상 화면 WBP가 상속하는 베이스입니다. 보상 뷰모델에 묶여 표시·입력만 담당합니다. */
// @file RewardUIWidgetBase.h
// 레이아웃은 WBP가 만든다. 보상 줄은 WBP_RewardRow를 반복 생성하고 C++은 값/아이콘만 넣는다.

#include "RDMinimal.h"
#include "UI/RDUserWidget.h"
#include "UI/Reward/RewardUITypes.h"

#include "RewardUIWidgetBase.generated.h"

class UButton;
class UImage;
class UWidget;
class UTextBlock;
class UTexture2D;
class UVerticalBox;
class URewardRowWidgetBase;
class URewardUIModel;

/** @brief '받기'로 보상 화면이 닫혔음을 알린다(승리 흐름이 이걸 받아 다음 단계=월드맵을 연다). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRewardClosed);

struct FRewardClaimRow
{
	ERewardClaimKind mKind = ERewardClaimKind::Gold;
	int32 mChoiceIndex = INDEX_NONE;
	bool mClaimed = false;
};

UCLASS(Abstract)
class P_RD_API URewardUIWidgetBase : public URDUserWidget
{
	GENERATED_BODY()

public:
	/** @brief 보상 화면이 전투 HUD/월드맵 위에 뜨도록 팝업 뷰포트 ZOrder를 설정한다. */
	URewardUIWidgetBase(const FObjectInitializer& ObjectInitializer);

	/**
	 * @brief 보상 화면은 연출 없이 즉시 연다.
	 *
	 * @details 보상 확인 흐름에 시간 기반 애니메이션을 다시 넣지 못하도록
	 * 열기/닫기 도중 재생 중인 UMG 애니메이션도 정지하고 즉시 완료한다.
	 */
	virtual void OpenUI(FOnEndUIOpenAnimation Callback = FOnEndUIOpenAnimation()) override;
	virtual void CloseUI(FOnEndUICloseAnimation Callback = FOnEndUICloseAnimation()) override;

	/** @brief 보상 뷰모델에 연결하고 갱신 알림을 구독한다. */
	UFUNCTION(BlueprintCallable, Category = "Reward|UI")
	void BindUIModel(URewardUIModel* InUIModel);

	/** @brief WBP가 현재 UIModel을 읽기 위한 접근자. */
	UFUNCTION(BlueprintPure, Category = "Reward|UI")
	URewardUIModel* GetUIModel() const { return mUIModel; }

	/** @brief '받기' 버튼이 호출. 의도만 뷰모델로 넘긴다. */
	UFUNCTION(BlueprintCallable, Category = "Reward|UI")
	void Claim();

	/** @brief 게임플레이가 이 보상(종류/인덱스) 지급을 확정했음을 알린다. 행은 남기고 수령 완료 상태로 잠근다. */
	// 지급 성공 판정은 게임플레이가 한다. 위젯은 스스로 받았다고 표시하지 않고 성공 ACK만 반영한다.
	void NotifyRewardClaimed(ERewardClaimKind ClaimKind, int32 ChoiceIndex);

	/** @brief 표시 중인 전체 보상 행 수. 자동화 시험과 접근성 상태 표시에 사용한다. */
	UFUNCTION(BlueprintPure, Category = "Reward|UI")
	int32 GetRewardRowCount() const { return mRewardClaimRows.Num(); }

	/** @brief 게임플레이가 지급 성공을 확정한 보상 행 수. */
	UFUNCTION(BlueprintPure, Category = "Reward|UI")
	int32 GetClaimedRewardRowCount() const;

	/** @brief 보상이 없거나 모든 보상의 지급이 확정됐는지 반환한다. */
	UFUNCTION(BlueprintPure, Category = "Reward|UI")
	bool IsRewardClaimComplete() const { return AreAllRewardRowsClaimed(); }

	/** @brief 현재 보상 배경과 행 프레임이 모두 준비됐는지 반환한다. */
	UFUNCTION(BlueprintPure, Category = "Reward|Art")
	bool HasCurrentRewardArt() const
	{
		return mRewardBackgroundTexture != nullptr && mRewardRowFrameTexture != nullptr;
	}

	/** @brief 현재 동적으로 그린 보상 행 프레임 수. 자동화 시험에서 행/아트 결합을 확인한다. */
	UFUNCTION(BlueprintPure, Category = "Reward|Art")
	int32 GetCurrentRewardRowFrameCount() const { return mRewardRowFrameImages.Num(); }

	/** @brief '받기'로 보상 화면이 닫혔을 때 발생. 승리 흐름이 구독해 다음 단계(월드맵)를 연다. */
	UPROPERTY(BlueprintAssignable, Category = "Reward|UI")
	FOnRewardClosed OnClosed;

protected:
	/** @brief 받기 버튼 클릭을 연결하고, 이미 바인딩된 보상이 있으면 즉시 그린다. */
	virtual void NativeConstruct() override;

	/** @brief 화면 이탈 시 UIModel 델리게이트 구독을 정리한다. */
	virtual void NativeDestruct() override;

	/** @brief Blueprint 연출 없이 OpenUI/CloseUI 생명주기를 즉시 완료한다. */
	virtual void PlayOpenUIAnimation_Implementation() override;
	virtual void PlayCloseUIAnimation_Implementation() override;

private:
	/** @brief '받기' 버튼 클릭 → Claim 의도 전달 후 화면을 닫는다. */
	UFUNCTION() void HandleCloseClicked();

	/** @brief UIModel 변경 알림을 받아 BindWidget 위젯에 값을 채운다. */
	UFUNCTION() void HandleUIChanged();

	/** @brief 선택지(항목) 변경 알림을 받아 항목 카드를 채운다. */
	UFUNCTION() void HandleChoicesChanged();

	/** @brief 보상 행 클릭 → 해당 보상 지급 요청 후 행을 받은 상태로 바꾼다. */
	UFUNCTION() void HandleRewardRowClicked(int32 RewardRowIndex);

	/** @brief 게임플레이가 지급 성공을 확정했을 때만 행을 수령 완료 상태로 바꾼다. */
	UFUNCTION() void HandleRewardClaimConfirmed(ERewardClaimKind ClaimKind, int32 ChoiceIndex);

	/** @brief 현재 UIModel 구독을 해제하고 참조를 비운다. */
	void UnbindUIModel();

	/** @brief 현재 HUD 톤의 정적 배경을 16:9 디자인 캔버스 가장 뒤에 설치한다. */
	void EnsureBackgroundArt();

	/** @brief 골드/경험치 값을 WBP 텍스트 슬롯에 반영한다. */
	void RefreshSummary();

	/** @brief 보상 스냅샷과 룸 보상 배열을 mRewardRowsBox에 다시 그린다. */
	void RefreshRows();

	/** @brief mCloseButton은 클릭 영역만 남기고 기본 Slate 배경을 투명화한다. */
	void ApplyTransparentCloseButtonStyle() const;

	/** @brief 모든 행을 받았을 때만 닫기 버튼을 노출한다. */
	void UpdateCloseButtonVisibility() const;

	bool AreAllRewardRowsClaimed() const;

private:
	UTexture2D* GetRewardIcon(ERewardChoiceKind Kind) const;

protected:
	// ---- WBP BindWidget (WBP_Reward의 위젯명과 일치). ----
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> mRewardRowsBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> mCloseButton;

	/** @brief 새 보상 시안이 별도 제목 슬롯을 제공할 때 mTitle을 즉시 표시한다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mTitleText;

	/** @brief 현재 바인딩된 보상 상태 소유자; 위젯은 이 객체를 소유하지 않고 구독만 한다. */
	UPROPERTY(BlueprintReadOnly, Category = "Reward|UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<URewardUIModel> mUIModel;

	/** @brief 밝은 원목/은색/양피지 HUD 톤으로 다시 제작한 정적 보상 배경. */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Reward|Art")
	TObjectPtr<UTexture2D> mRewardBackgroundTexture;

	/** @brief WBP 16:9 디자인 캔버스에 런타임으로 한 번만 추가하는 배경 이미지. */
	UPROPERTY(Transient)
	TObjectPtr<UImage> mRewardBackgroundImage;

	/** @brief 동적 보상 행 전체를 감싸는 원목/은장/남색 프레임. */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Reward|Art")
	TObjectPtr<UTexture2D> mRewardRowFrameTexture;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Reward|UI")
	TObjectPtr<UTexture2D> mEquipmentIcon;
	UPROPERTY(BlueprintReadOnly, Category = "Reward|UI")
	TObjectPtr<UTexture2D> mSkillIcon;
	UPROPERTY(BlueprintReadOnly, Category = "Reward|UI")
	TObjectPtr<UTexture2D> mGoldIcon;
	UPROPERTY(BlueprintReadOnly, Category = "Reward|UI")
	TObjectPtr<UTexture2D> mRewardGoldIconTexture;

	/** @brief 경험치 행 전용 아이콘. 골드 아이콘과 구분되는 별도 텍스처(RewardV4_11). */
	UPROPERTY(BlueprintReadOnly, Category = "Reward|UI")
	TObjectPtr<UTexture2D> mRewardExpIconTexture;

	/** @brief 보상 한 줄 WBP. 디자이너는 /Game/UI/WBP_RewardRow에서 내부 배치를 조정한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|UI")
	TSubclassOf<URewardRowWidgetBase> mRewardRowWidgetClass;

	/** @brief mRewardRowsBox에 추가되는 각 줄 사이 여백. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|UI")
	FMargin mRewardRowPadding = FMargin(0.0f, 0.0f, 0.0f, 10.0f);

	/** @brief 1672x941 보상 디자인 캔버스 기준 동적 행 크기. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|UI")
	FVector2D mRewardRowSize = FVector2D(900.0f, 105.0f);

private:
	TArray<FRewardClaimRow> mRewardClaimRows;

	UPROPERTY(Transient)
	TArray<TObjectPtr<URewardRowWidgetBase>> mRewardRowWidgets;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> mRewardRowFrameImages;

	/** @brief 수령 완료 시 행 전체를 흐리게 표시하기 위한 동적 행 호스트. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidget>> mRewardRowVisuals;

	/** @brief C++/Blueprint 양쪽 클릭 배선이나 빠른 연타가 닫힘 신호를 중복 전송하지 않게 한다. */
	bool mCloseCommitted = false;
};
