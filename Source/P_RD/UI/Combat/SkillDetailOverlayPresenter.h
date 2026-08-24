/*****************************************************************//**
 * @file   SkillDetailOverlayPresenter.h
 * @brief  공용 상세 WBP(WBP_CombatDetailOverlay)를 소유하고 스킬 상세를 그리는 프레젠터.
 * @date   2026-08-19
 *********************************************************************/

#pragma once

/**
 * @brief 스킬 상세 겹의 재사용 프레젠터.
 *
 * @details
 * 전투 HUD 안에 있던 풍부한 스킬 상세 렌더(수치 메달·범위 버튼·전술 WBP)를
 * 떼어 낸 것이다. 전투 밖 화면(고용/상점 등)도 같은 상세 겹을 띄울 수 있게,
 * PR#426 의 Model-View 규칙 그대로 **FSkillDetailUI DTO 하나만 소비한다.**
 * 게임모드·UIModel·게임플레이 모델은 보지 않는다 -- 그런 상태를 읽어야 하는
 * 부분(용병 스킬 줄, 월드 제스처 잠금, 닫기 후 위협 범위 걷기)은 호출자가
 * 래퍼에서 처리하고 여기에는 값만 내려 준다.
 *
 * 겹 WBP 는 손으로 만든 자산이라 위젯을 전부 이름으로 찾고, 없는 것은
 * 조용히 건너뛴다 -- HUD 본체와 같은 규약이다.
 */

#include "RDMinimal.h"
#include "Templates/SubclassOf.h"
#include "UI/Combat/CombatUITypes.h"

#include "SkillDetailOverlayPresenter.generated.h"

class UButton;
class UCanvasPanel;
class UFont;
class UImage;
class UOverlay;
class UScrollBox;
class USkillTacticalDiagramWidget;
class UTextBlock;
class UUserWidget;
class UWidget;
class UWidgetSwitcher;

/**
 * @brief 통합 미리보기가 쓰는 그림 묶음.
 *
 * 쿡 유지용 하드 레퍼런스는 호출자(생성자 ConstructorHelpers)가 쥔다 -- 이
 * 구조체는 전달용이고, 프레젠터가 UPROPERTY 로 다시 붙잡는다.
 */
struct FSkillDetailOverlayVisualAssets
{
	UTexture2D* mRingTexture = nullptr;
	UTexture2D* mCellNormalTexture = nullptr;
	UTexture2D* mCellSelectedTexture = nullptr;
	UTexture2D* mAPIconTexture = nullptr;
	UTexture2D* mDamageIconTexture = nullptr;
	UTexture2D* mCooldownIconTexture = nullptr;
	UTexture2D* mCriticalIconTexture = nullptr;
	UTexture2D* mCasterIconTexture = nullptr;
	UTexture2D* mTargetIconTexture = nullptr;
	UTexture2D* mRangeButtonTexture = nullptr;
	UTexture2D* mRangeButtonSelectedTexture = nullptr;
};

/**
 * @brief 호스트 화면의 실제 브러시를 우선 쓰기 위한 조회 델리게이트.
 *
 * AP/쿨타임 아이콘은 전투 HUD WBP 의 배지 브러시를 그대로 따라간다(교체 시
 * 자동 추종). 전투 밖 호출자는 안 바인딩하면 대체 그림이 쓰인다.
 * @param SourceWidgetName 호스트에서 찾을 Image 위젯 이름
 * @param Fallback 못 찾으면 쓸 그림
 */
DECLARE_DELEGATE_RetVal_TwoParams(UTexture2D*, FSkillDetailStatTextureResolver,
	FName /*SourceWidgetName*/, UTexture2D* /*Fallback*/);

UCLASS()
class P_RD_API USkillDetailOverlayPresenter : public UObject
{
	GENERATED_BODY()

public:
	/* ── 수명·설정 ────────────────────────────────────────────────────── */

	/**
	 * @brief 그림 묶음의 기본값을 하드 참조로 든다.
	 *
	 * @details
	 * 전투 밖 호스트(고용/지도/상점 레일)는 HUD처럼 SetVisualAssets를 불러 줄
	 * 그림 창고가 없다. 기본값이 없으면 수치 아이콘과 범위 버튼이 투명하게
	 * 사라진다 -- 실제 기기에서 그렇게 나왔다.
	 */
	USkillDetailOverlayPresenter();

	/**
	 * @brief 지연 생성에 필요한 값을 넣는다. 위젯은 아직 만들지 않는다.
	 * @param World 겹을 얹을 월드. 소유 플레이어가 없는 자동화 경로의 대비책
	 * @param OverlayClass 공용 상세 WBP 클래스
	 * @param DiagramClass 스킬 전술 범위판 WBP 클래스. 없으면 범위판만 빠진다
	 * @param ViewportZOrder 뷰포트에 얹을 Z. 몬스터 도감(55)보다 위여야 한다
	 */
	void Initialize(UWorld* World, TSubclassOf<UUserWidget> OverlayClass,
		TSubclassOf<USkillTacticalDiagramWidget> DiagramClass,
		int32 ViewportZOrder);

	/** @brief 겹이 아직 안 만들어졌을 때만 반영된다. 시험이 클래스를 갈아 끼울 때 쓴다. */
	void SetOverlayWidgetClass(TSubclassOf<UUserWidget> OverlayClass);

	/** @brief 장문 한글 상세 전용 OFL 폰트. 안 주면 처음 쓸 때 LoadObject 로 받는다. */
	void SetReadableDetailFont(UFont* Font);

	/** @brief 통합 미리보기 그림 묶음을 건다. Ensure 전에 한 번 부른다. */
	void SetVisualAssets(const FSkillDetailOverlayVisualAssets& Assets);

	/** @brief AP/쿨타임 브러시를 호스트 화면에서 조회할 델리게이트. */
	void SetStatTextureResolver(FSkillDetailStatTextureResolver Resolver)
	{
		mStatTextureResolver = MoveTemp(Resolver);
	}

	/**
	 * @brief 상세 겹을 처음 한 번 만들어 화면에 얹는다.
	 * @param OwningPlayer 실제 플레이의 소유 플레이어. null 이면 Initialize 의
	 *        월드로 만든다(편집기 자동화 경로)
	 */
	bool EnsureOverlayWidget(APlayerController* OwningPlayer = nullptr);

	/** @brief HUD 파괴 등에서 겹을 화면에서 떼고 내부 참조를 비운다. */
	void Teardown();

	/* ── 핵심 API ─────────────────────────────────────────────────────── */

	/** @brief 스킬 상세 DTO 하나로 풍부한 상세 화면 전체를 그린다. */
	void Present(const FSkillDetailUI& Detail);

	/**
	 * @brief 아티팩트 상세 DTO 하나로 아티팩트 전용 2열 화면을 그린다.
	 *
	 * @details 전투 HUD 의 ShowArtifactDetailOverlay 렌더를 그대로 옮긴 것이다.
	 * 긴 효과 본문은 스킬 상세처럼 전용 ScrollBox 에 담아 넘치면 스크롤한다.
	 * 서체도 같은 규칙(ApplyReadableDetailTypography)을 탄다.
	 */
	void PresentArtifact(const FCombatArtifactUI& Detail);

	/** @brief 겹을 접는다. 호출자 쪽 정리(위협 범위 걷기 등)는 호출자가 한다. */
	void Dismiss();

	/** @brief 겹이 지금 화면에 떠 있는가. */
	bool IsShowing() const;

	/** @brief 확정 시안 "닫기" 단추. 바인딩이 있으면 호출자에게 맡기고, 없으면 스스로 닫는다. */
	FSimpleMulticastDelegate& OnCloseClicked() { return mOnCloseClicked; }

	/** @brief 상세 전용 SceneCapture 를 멈추라는 신호. 캡처 장치는 호출자 소유다. */
	FSimpleMulticastDelegate& OnWorldPreviewStopRequested()
	{
		return mOnWorldPreviewStopRequested;
	}

	/* ── 다른 상세 종류(유닛/상태/아티팩트)와 나눠 쓰는 공용 헬퍼 ────────
	 *
	 * 그 화면들은 UIModel 상태를 읽어야 해서 호출자에 남는다. 대신 같은 겹
	 * 위젯을 그리므로, 판 부품과 배치 헬퍼는 여기 것을 그대로 쓴다.
	 */
	static constexpr int32 DetailChipCount = 5;
	static constexpr int32 DetailGridExtent = 5;   // 5x5. 가운데가 시전자.

	void ApplyReadableDetailTypography(bool bReadable);
	/** @brief 상세 종류에 맞춰 3열 또는 아티팩트 전용 2열로 실제 열을 재배치한다. */
	void ApplyDetailColumnLayout(bool bArtifactTwoColumn);
	void SetDetailChip(int32 ChipSlot, const FText& Label, const FText& Value);
	void ClearDetailChips();
	/** @brief 형태·거리로 칸을 칠한다. 게임플레이가 내려준 스킬 스펙 그대로 그린다. */
	void PaintSelectGrid(const FSkillTargetingUI& Targeting);
	void PaintHitGrid(const FSkillTargetingUI& Targeting);
	void ClearDetailGrids();
	/** @brief 오른쪽 열의 세 덩어리 중 하나만 켠다. nullptr 이면 셋 다 끈다. */
	void ShowDetailRightBlock(const UWidget* Wanted);
	void SetSkillVisualPreviewShown(bool bShown);
	/** @brief 실제 스킬 DTO로 수치 메달과 통합 전술 보드를 갱신한다. */
	void UpdateSkillVisualPreview(const FSkillDetailUI& Detail);
	/** @brief 통합 미리보기 묶음만 미리 짓는다. 겹이 없으면 아무 일도 안 한다. */
	void EnsureSkillVisualPreviewBuilt() { BuildSkillVisualPreview(); }

	/* ── 겹 부품 접근자. 호출자의 나머지 상세 경로가 같은 판을 그릴 때 쓴다. ── */
	UUserWidget* GetOverlayWidget() const { return mDetailOverlayWidget; }
	/** @brief 디자이너에서 직접 편집하는 스킬 전용 정보면 WBP 인스턴스. */
	UUserWidget* GetSkillContentWidget() const { return mSkillContentWidget; }
	USkillTacticalDiagramWidget* GetTacticalDiagram() const
	{
		return mSkillTacticalDiagramWidget;
	}
	UImage* GetIconImage() const { return mDetailIconImage; }
	UTextBlock* GetTitleText() const { return mDetailTitleText; }
	UTextBlock* GetSubtitleText() const { return mDetailSubtitleText; }
	UTextBlock* GetBodyText() const { return mDetailBodyText; }
	UWidget* GetStatBlock() const { return mDetailStatBlock; }
	UWidget* GetTargetBlock() const { return mDetailTargetBlock; }
	UWidget* GetSkillBlock() const { return mDetailSkillBlock; }
	UWidget* GetExtraBlock() const { return mDetailExtraBlock; }
	UTextBlock* GetExtraHeading() const { return mDetailExtraHeading; }
	UTextBlock* GetExtraText() const { return mDetailExtraText; }
	UWidget* GetDivider0() const { return mDetailDivider0; }
	UWidget* GetDivider1() const { return mDetailDivider1; }
	/** @brief 상세 전용 월드 미리보기 Image. 캡처 브러시는 호출자가 건다. */
	UImage* GetWorldPreviewImage() const { return mSkillWorldPreviewImage; }
	/** @brief 자동화 검증용 수치 칩 값 문자열. */
	FString GetChipValueString(int32 ChipIndex) const;
	/**
	 * @brief 전역 DPI 이후 1920x1080 상세 디자인 전체를 화면에 맞출 배율.
	 * @param ViewportSize 물리 픽셀 기준 화면 크기
	 * @param ViewportScale 현재 전역 UMG DPI 배율
	 */
	static float CalculateResponsiveSkillPanelScale(FVector2D ViewportSize,
		float ViewportScale);
	/** @brief 스킬 외 상세판도 같은 전체 디자인 기준으로 맞춘 배율. */
	static float CalculateResponsiveDetailPanelScale(FVector2D ViewportSize,
		float ViewportScale);
	/** @brief 열린 상세판을 현재 뷰포트 크기에 다시 맞춘다. */
	void RefreshResponsiveLayout();

private:
	/* ── 이미지 시안 기반 통합 스킬 미리보기 ─────────────────────────────
	 * 수치 메달과 선택/효과 범위를 한 정보면에 겹쳐 그린다. 실제 전투의
	 * FSkillTargetingUI만 소비하며, 장애물 판정까지 지어내지는 않는다.
	 */
	static constexpr int32 SkillPreviewStatCount = 4;
	static constexpr int32 SkillPreviewRows = 5;
	static constexpr int32 SkillPreviewColumns = 7;

	/** @brief 상세창의 수치 칩과 범위 칸을 이름으로 찾아 둔다. 없으면 건너뛴다. */
	void BindDetailExtras();
	/**
	 * @brief 모든 상세판의 1920x1080 디자인 전체를 화면 안에 중앙 배치한다.
	 *
	 * @details 전역 DPI가 이미 모자란 축을 맞춘 뒤이므로 여기서 판만 다시
	 * 확대하지 않는다. 남는 논리 공간에 디자인 캔버스를 중앙 배치한다.
	 */
	void ApplyResponsiveSkillPanelScale(bool bSkillDetail);
	/**
	 * @brief 아티팩트 효과 본문 전용 ScrollBox 묶음을 처음 한 번 짓는다.
	 *
	 * @details 공용 DetailBodyText 는 고정 높이 Canvas 슬롯이라 효과 줄이 길면
	 * 아래로 잘려 나갔다. 스킬 설명 ScrollBox 와 같은 1920 디자인 축척 계약을
	 * 따로 세워, 아티팩트 상태에서만 본문 자리를 실제 ScrollBox 로 쓴다.
	 */
	void BuildArtifactDescriptionScroll();
	/** @brief 이미지 시안의 수치 메달/통합 전술 보드를 실제 상세 WBP 인스턴스에 짓는다. */
	void BuildSkillVisualPreview();
	/** @brief 별도 WBP가 가진 스킬 정보면을 공용 상세판에 한 번 꽂고 이름으로 바인딩한다. */
	bool BindAuthoredSkillContent();
	/** @brief 실제 월드 프리뷰가 없을 때만 쓰는 구형 모식도 묶음을 켜거나 끈다. */
	void SetSyntheticSkillDiagramShown(bool bShown);
	/** @brief 생성 이미지의 정상/선택 브러시를 현재 범위 토글 상태에 맞춘다. */
	void RefreshSkillRangeButtonStyles();
	/** @brief 상세 전용 월드 미리보기 그림을 접고 캡처 정지를 호출자에게 알린다. */
	void HideWorldPreviewImage();

	/** @brief 확정 시안의 "닫기" 단추 처리. */
	UFUNCTION() void HandleCloseClicked();
	/** @brief 왼쪽 요약열의 사정 범위 버튼. 기존 전술 WBP의 토글 동작을 호출한다. */
	UFUNCTION() void HandleSkillSelectRangeButtonClicked();
	/** @brief 왼쪽 요약열의 영향 범위 버튼. 기존 전술 WBP의 토글 동작을 호출한다. */
	UFUNCTION() void HandleSkillEffectRangeButtonClicked();
	/** @brief 범위판 토글에 맞춰 설명문과 판이 같은 중앙 공간을 번갈아 쓴다. */
	void HandleSkillTacticalPreviewVisibilityChanged(bool bPreviewShown);

	/* ── 설정 ─────────────────────────────────────────────────────────── */
	UPROPERTY(Transient) TWeakObjectPtr<UWorld> mWorld;
	UPROPERTY() TSubclassOf<UUserWidget> mDetailOverlayWidgetClass;
	/** 스킬 정보면은 별도 WBP가 모든 내부 배치를 소유한다. */
	UPROPERTY() TSubclassOf<UUserWidget> mSkillDetailContentWidgetClass;
	UPROPERTY() TSubclassOf<USkillTacticalDiagramWidget>
		mSkillTacticalDiagramWidgetClass;
	int32 mViewportZOrder = 70;
	FSkillDetailStatTextureResolver mStatTextureResolver;
	FSimpleMulticastDelegate mOnCloseClicked;
	FSimpleMulticastDelegate mOnWorldPreviewStopRequested;

	/* ── 겹과 이름으로 찾은 부품 ──────────────────────────────────────── */
	UPROPERTY(Transient) TObjectPtr<UUserWidget> mDetailOverlayWidget;
	UPROPERTY(Transient) TObjectPtr<UImage> mDetailIconImage;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> mDetailTitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> mDetailSubtitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> mDetailBodyText;
	/** 상세 종류별 안전 사각형으로 전역 DPI 축소를 보정하는 디자인 캔버스. */
	UPROPERTY(Transient) TObjectPtr<UWidget> mDetailDesignCanvas;
	FVector2D mDefaultDetailDesignScale = FVector2D(1.f, 1.f);
	FVector2D mDefaultDetailDesignTranslation = FVector2D::ZeroVector;
	FVector2D mDefaultDetailDesignPivot = FVector2D(0.5f, 0.5f);
	FVector2D mLastResponsiveViewportSize = FVector2D::ZeroVector;
	float mLastResponsiveViewportScale = 0.f;
	bool mDetailScaleDefaultsCached = false;
	bool mResponsiveDetailActive = false;
	bool mResponsiveDetailIsSkill = false;

	/** 장문 한글 상세 전용 OFL 폰트. 유닛/상태 상세의 기존 서체는 유지한다. */
	UPROPERTY() TObjectPtr<UFont> mReadableDetailFont;
	FSlateFontInfo mDefaultDetailTitleFont;
	FSlateFontInfo mDefaultDetailSubtitleFont;
	FSlateFontInfo mDefaultDetailBodyFont;
	bool mDefaultDetailFontsCached = false;

	/* ── 상세창 수치 칩과 범위 그림 ────────────────────────────────────── */
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> mDetailChipLabels;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> mDetailChipValues;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> mDetailSelectCells;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> mDetailHitCells;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> mDetailSelectCaptionText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> mDetailHitCaptionText;

	/* ── 오른쪽 열의 세 덩어리와 실제 열 ──────────────────────────────── */
	UPROPERTY(Transient) TObjectPtr<UWidget> mDetailStatBlock;
	UPROPERTY(Transient) TObjectPtr<UWidget> mDetailTargetBlock;
	UPROPERTY(Transient) TObjectPtr<UWidget> mDetailSkillBlock;
	UPROPERTY(Transient) TObjectPtr<UWidget> mDetailExtraBlock;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> mDetailExtraHeading;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> mDetailExtraText;
	UPROPERTY(Transient) TObjectPtr<UWidget> mDetailIdentityColumn;
	UPROPERTY(Transient) TObjectPtr<UWidget> mDetailStatColumn;
	UPROPERTY(Transient) TObjectPtr<UWidget> mDetailRightColumn;
	UPROPERTY(Transient) TObjectPtr<UWidget> mDetailWideColumn;
	UPROPERTY(Transient) TObjectPtr<UWidget> mDetailDivider0;
	UPROPERTY(Transient) TObjectPtr<UWidget> mDetailDivider1;

	/* ── WBP authored 스킬 정보면 ─────────────────────────────────────── */
	UPROPERTY(Transient) TObjectPtr<UUserWidget> mSkillContentWidget;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> mSkillVisualPreview;
	UPROPERTY(Transient) TObjectPtr<UImage> mSkillIconImage;
	/** @brief 전투 HUD의 실제 AP/피해/쿨타임/치명타 브러시를 복제한 4행 아이콘. */
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> mSkillVisualStatIcons;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> mSkillVisualStatTexts;
	/** @brief 긴 설명을 고정 폭에서 스크롤하는 스킬 전용 본문. */
	UPROPERTY(Transient) TObjectPtr<UScrollBox> mSkillDescriptionScrollBox;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> mSkillDescriptionText;
	UPROPERTY(Transient) TObjectPtr<UWidgetSwitcher> mSkillContentSwitcher;

	/* ── 아티팩트 효과 본문 전용 스크롤 묶음 ──────────────────────────── */
	/** @brief 1920 디자인 축척을 거는 루트(ScaleBox). 켜고 끄는 단위다. */
	UPROPERTY(Transient) TObjectPtr<UWidget> mArtifactDescriptionRoot;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> mArtifactDescriptionScrollBox;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> mArtifactDescriptionText;
	/** @brief 수치 바로 아래의 클릭 가능한 범위 버튼과 동적 라벨. */
	UPROPERTY(Transient) TObjectPtr<UButton> mSkillSelectRangeButton;
	UPROPERTY(Transient) TObjectPtr<UButton> mSkillEffectRangeButton;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> mSkillSelectRangeText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> mSkillEffectRangeText;
	UPROPERTY(Transient) TObjectPtr<UImage> mSkillSelectRangePlate;
	UPROPERTY(Transient) TObjectPtr<UImage> mSkillEffectRangePlate;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> mSkillVisualCells;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> mSkillVisualConnectorPips;
	UPROPERTY(Transient) TObjectPtr<UImage> mSkillVisualCasterHalo;
	UPROPERTY(Transient) TObjectPtr<UImage> mSkillVisualEffectHalo;
	UPROPERTY(Transient) TObjectPtr<UImage> mSkillVisualCasterMarker;
	UPROPERTY(Transient) TObjectPtr<UImage> mSkillVisualTargetMarker;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> mSkillVisualSelectLegend;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> mSkillVisualEffectLegend;
	/** 첨부 시안형 범위 디오라마. WBP 자체가 셀/마커/중앙정렬 라벨을 소유한다. */
	UPROPERTY(Transient) TObjectPtr<USkillTacticalDiagramWidget>
		mSkillTacticalDiagramWidget;
	/** 실제 전투 장면이 표시되는 WBP Image. 캡처 브러시는 호출자가 건다. */
	UPROPERTY(Transient) TObjectPtr<UImage> mSkillWorldPreviewImage;

	/* ── 전달받아 다시 붙잡은 그림 묶음 ───────────────────────────────── */
	UPROPERTY() TObjectPtr<UTexture2D> mSkillVisualRingTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mSkillVisualCellNormalTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mSkillVisualCellSelectedTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mSkillVisualAPIconTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mSkillVisualDamageIconTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mSkillVisualCooldownIconTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mSkillVisualCriticalIconTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mSkillVisualCasterIconTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mSkillVisualTargetIconTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mSkillRangeButtonTexture;
	UPROPERTY() TObjectPtr<UTexture2D> mSkillRangeButtonSelectedTexture;
};
