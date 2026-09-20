/*****************************************************************//**
 * @file   FrontendMapGraphWidgets.h
 * @brief  월드맵 그래프 반복 요소 WBP 위젯 정의 헤더
 * @date   2026-06-04
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Components/Button.h"
#include "PCGStage/RoomType.h"
#include "UI/RoomViewTypes.h"
#include "UI/RDUserWidget.h"

#include "FrontendMapGraphWidgets.generated.h"

class UBorder;
class UButton;
class UImage;
class UTextBlock;
class UTexture2D;

/**
 * @brief 지도 노드 클릭 시 행/열 좌표만 부모 위젯으로 전달하는 UI delegate
 *
 * 이 이벤트는 룸 데이터나 전환 API를 직접 실행하지 않는다.
 * UMG 노드 버튼이 사용자가 누른 행/열 좌표를 부모 지도 위젯으로 넘기기 위한 표시 계층 이벤트다.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFrontendMapNodeClickedDelegate, int32, RowIndex, int32, ColumnIndex);

/**
 * @brief 지도 노드 좌표를 함께 보관하는 전용 Button
 *
 * UButton 자체는 어떤 방 좌표를 의미하는지 모르므로, 지도 노드용 버튼이 행/열 값을 함께 보관한다.
 * 클릭 시 게임 로직을 실행하지 않고 행/열 좌표만 broadcast한다.
 */
UCLASS()
class P_RD_API UFrontendMapNodeButton : public UButton
{
	GENERATED_BODY()

public:
	void SetNodeCoordinates(int32 InRowIndex, int32 InColumnIndex);

	UPROPERTY(Category = "Frontend Map", BlueprintAssignable)
	FFrontendMapNodeClickedDelegate OnMapNodeClicked;

private:
	UFUNCTION()
	void HandleClicked();

	int32 mRowIndex = INDEX_NONE;
	int32 mColumnIndex = INDEX_NONE;
	bool mClickBound = false;
};

/**
 * @brief 월드맵 방 사이를 잇는 선 하나를 표시하는 WBP 위젯
 *
 * @details
 * 지도 연결선의 위치, 길이, 각도는 맵 데이터에 따라 매번 달라진다.
 * 그래서 부모 UFrontendMapWidget이 CanvasPanelSlot 값은 계산하지만,
 * 선의 실제 모양은 WBP_FrontendMapLine 안에 둔다.
 *
 * Stage 연결 정보를 화면 선으로 표현하기 위한 표시 전용 WBP 베이스다.
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API UFrontendMapLineWidget : public URDUserWidget
{
	GENERATED_BODY()

public:
	/** @brief 선 색상을 현재 방 상태에 맞게 바꿈 */
	void SetLineColor(const FLinearColor& InColor);

	/**
	 * @brief 경로 개방 여부에 맞는 텍스처(실선/점선)와 틴트를 적용한다.
	 *
	 * @details
	 * 텍스처는 WBP Class Defaults(시안 빌더가 주입)가 소유한다. 텍스처가 없으면 기존 색상 Border로 폴백한다.
	 */
	void SetLineStyle(bool bIsOpenPath, bool bIsTraversed = false);

	/** @brief 마지막으로 적용된 연결선이 열린 경로인지 반환한다. */
	bool IsOpenPath() const { return mIsOpenPath; }

	/** @brief 마지막으로 적용된 연결선이 실제로 지나온 경로인지 반환한다. */
	bool IsTraversedPath() const { return mIsTraversedPath; }

	/** @brief 시안이 정한 선 두께. 부모가 CanvasPanelSlot 높이로 쓴다. */
	float GetLineThickness() const;

protected:
	void NativeConstruct() override;

	/** @brief 파생 레이아웃이 기존 선 표시 로직에 전용 텍스처를 공급한다. */
	void SetLineTexturesForLayout(UTexture2D* InSolid, UTexture2D* InDashed)
	{
		mSolidTexture = InSolid;
		mDashedTexture = InDashed;
	}
	void SetLineThicknessForLayout(float InThickness) { mLineThickness = InThickness; }
	void SetLinePaletteForLayout(
		const FLinearColor& InOpenTint,
		const FLinearColor& InTraversedTint,
		const FLinearColor& InLockedTint,
		const FLinearColor& InOpenGlowTint,
		const FLinearColor& InLockedGlowTint)
	{
		mOpenTint = InOpenTint;
		mTraversedTint = InTraversedTint;
		mLockedTint = InLockedTint;
		mOpenGlowTint = InOpenGlowTint;
		mLockedGlowTint = InLockedGlowTint;
	}

private:
	/** @brief WBP 안에서 실제 선 색상을 받는 Border(텍스처 미지정 시 폴백) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> LinePanel;

	/** @brief 경로 텍스처를 그리는 Image(시안 빌더가 WBP에 생성) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> LineImage;

	/** @brief 중심선 아래에 깔리는 넓은 발광선. 없는 기존 WBP는 종전처럼 중심선만 그린다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> LineGlowImage;

	/* 아래 스타일 값들은 concept 시안이 정본이며 빌더가 WBP Class Defaults로 주입한다. C++ 하드코딩 금지. */

	/** @brief 열린 경로(갈 수 있는 길) 텍스처 */
	UPROPERTY(Category = "Frontend Map|Style", EditDefaultsOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UTexture2D> mSolidTexture;

	/** @brief 잠긴 경로 점선 텍스처 */
	UPROPERTY(Category = "Frontend Map|Style", EditDefaultsOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UTexture2D> mDashedTexture;

	/** @brief 선 두께(px, 디자인 기준) */
	UPROPERTY(Category = "Frontend Map|Style", EditDefaultsOnly, meta = (AllowPrivateAccess = true))
	float mLineThickness = 14.f;

	/** @brief 열린 길 틴트(시안 mOpenTint) */
	UPROPERTY(Category = "Frontend Map|Style", EditDefaultsOnly, meta = (AllowPrivateAccess = true))
	FLinearColor mOpenTint = FLinearColor(1.f, 1.f, 1.f, 0.95f);

	/** @brief 이미 지나온 길의 밝은 금색. */
	UPROPERTY(Category = "Frontend Map|Style", EditDefaultsOnly, meta = (AllowPrivateAccess = true))
	FLinearColor mTraversedTint = FLinearColor(1.f, 1.f, 1.f, 0.95f);

	/** @brief 잠긴 길 틴트(시안 mLockedTint) */
	UPROPERTY(Category = "Frontend Map|Style", EditDefaultsOnly, meta = (AllowPrivateAccess = true))
	FLinearColor mLockedTint = FLinearColor(1.f, 1.f, 1.f, 0.55f);

	UPROPERTY(Category = "Frontend Map|Style", EditDefaultsOnly, meta = (AllowPrivateAccess = true))
	FLinearColor mOpenGlowTint = FLinearColor(1.f, 0.55f, 0.08f, 0.42f);

	UPROPERTY(Category = "Frontend Map|Style", EditDefaultsOnly, meta = (AllowPrivateAccess = true))
	FLinearColor mLockedGlowTint = FLinearColor(0.55f, 0.40f, 0.18f, 0.12f);

	/** @brief 현재 표시 중인 선 스타일. 지도 갱신 회귀 테스트와 상태 확인에 사용한다. */
	bool mIsOpenPath = false;
	bool mIsTraversedPath = false;
};

/**
 * @brief 월드맵 방 노드 하나를 표시하는 WBP 위젯
 *
 * @details
 * 이 위젯은 방 하나의 버튼/색/문구만 담당한다.
 * 어떤 방으로 이동 가능한지, 선택하면 어떤 런 데이터가 바뀌는지는 부모 UFrontendMapWidget과 GameMode가 처리한다.
 *
 * FRoom을 대체하지 않는다. FMapRoomView 한 개를 화면 노드 하나로 보여주는 UMG 베이스다.
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API UFrontendMapNodeWidget : public URDUserWidget
{
	GENERATED_BODY()

public:
	UFrontendMapNodeWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** @brief WBP 노드에 표시할 좌표와 시각 상태를 넣음 */
	void SetNodeVisual(
		int32 InRowIndex,
		int32 InColumnIndex,
		const FText& Label,
		const FText& Badge,
		const FLinearColor& PanelColor,
		const FLinearColor& TypeStripeColor,
		const FSlateColor& LabelColor,
		const FSlateColor& BadgeColor,
		ERoomType RoomType = ERoomType::None,
		EMapRoomState RoomState = EMapRoomState::Locked,
		bool bIsCurrentRoom = false);

	/** @brief 부모 위젯이 현재 노드의 버튼 활성 여부를 정할 때 사용함 */
	void SetNodeEnabled(bool bEnabled) const;

	/** @brief 노드를 눌렀을 때 부모 맵 위젯으로 행/열 좌표를 보냄 */
	UPROPERTY(Category = "Frontend Map", BlueprintAssignable)
	FFrontendMapNodeClickedDelegate OnMapNodeClicked;

protected:
	void NativeConstruct() override;
	void NativeDestruct() override;

	/** @brief 지도 스킨별 방 아이콘 선택 지점. */
	virtual UTexture2D* GetTypeIconTexture(ERoomType RoomType) const;

	/** @brief 지도 스킨별 상태 링 선택 지점. */
	virtual UTexture2D* GetStateRingTexture(EMapRoomState RoomState, bool bIsCurrentRoom) const;

	/** @brief 레이아웃 스킨별 노드 위계 크기. Canvas 클릭 슬롯은 유지하고 보이는 노드만 확대한다. */
	virtual float GetVisualScale(ERoomType RoomType, EMapRoomState RoomState, bool bIsCurrentRoom) const;

private:
	UFUNCTION()
	void HandleNodeButtonClicked();

private:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> NodeButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> NodePanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> NodeTypeStripe;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NodeLabelText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NodeBadgeText;

	/** @brief 상태 링 오버레이 Image(시안 빌더가 WBP에 생성). 링이 상태를 표현하면 색 프레임은 숨긴다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> NodeRingImage;

	/* 링/아이콘 텍스처는 concept 시안이 정본 — 빌더가 WBP Class Defaults로 주입. C++ 경로 하드코딩은 폴백 전용. */

	UPROPERTY(Category = "Frontend Map|Style", EditDefaultsOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UTexture2D> mRingNormalTexture;

	UPROPERTY(Category = "Frontend Map|Style", EditDefaultsOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UTexture2D> mRingCurrentTexture;

	UPROPERTY(Category = "Frontend Map|Style", EditDefaultsOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UTexture2D> mRingLockedTexture;

	UPROPERTY(Category = "Frontend Map|Style", EditDefaultsOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UTexture2D> mRingClearedTexture;

	UPROPERTY(Category = "Frontend Map|Style", EditDefaultsOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UTexture2D> mIconMonsterTexture;

	UPROPERTY(Category = "Frontend Map|Style", EditDefaultsOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UTexture2D> mIconEliteTexture;

	UPROPERTY(Category = "Frontend Map|Style", EditDefaultsOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UTexture2D> mIconBossTexture;

	UPROPERTY(Category = "Frontend Map|Style", EditDefaultsOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UTexture2D> mIconShopTexture;

	UPROPERTY(Category = "Frontend Map|Style", EditDefaultsOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UTexture2D> mIconTreasureTexture;

	/** @brief 잠긴 방 아이콘 곱색(시안 nodeStyle.lockedIconTint) — 잠김의 주역은 링, 이 틴트는 보조 */
	UPROPERTY(Category = "Frontend Map|Style", EditDefaultsOnly, meta = (AllowPrivateAccess = true))
	FLinearColor mLockedIconTint = FLinearColor(0.8f, 0.8f, 0.8f, 1.f);

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> mFallbackIconTreasure;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> mFallbackIconShop;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> mFallbackIconMonster;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> mFallbackIconElite;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> mFallbackIconBoss;

	int32 mRowIndex = INDEX_NONE;
	int32 mColumnIndex = INDEX_NONE;
};
