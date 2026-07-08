#include "UI/CombatTileMapHUDWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "UI/Combat/CombatUIModel.h"

/**
 * @file   CombatTileMapHUDWidget_CombatLog.cpp
 * @brief  전투 플로팅 로그(대상 머리 위 텍스트) + 턴 라운드 배너의 표시 구현.
 *
 * @details
 *  ## 이 파일이 하는 일
 *  게임플레이가 CombatUIModel 경계로 "여기에 이런 로그 띄워라 / 이 모션 로그 치워라"를 보내면,
 *  그걸 받아 월드 좌표 위 위젯으로 그리고, 매 프레임 위치를 갱신하고, 때가 되면 지운다.
 *  좌표는 머리 위 HP바와 똑같은 "월드→스크린 투영"이라 카메라가 움직여도 대상 위에 붙어 다닌다.
 *
 *  ## 로그 두 종류 (mIsPreview로 갈림)
 *  - **실행 로그(mIsPreview=false)** : "-12" 같은 순간 연출(juice). 뜨면 위로 떠오르며 FloatingLogLifetime(1.2s) 뒤
 *    자동 소멸. 여러 개가 몰리면 대기 큐(mPendingFloatingCombatLogs)에 쌓였다 FloatingLogQueueInterval 간격으로
 *    하나씩 스폰(같은 프레임에 다 뜨지 않게).
 *  - **미리보기 로그(mIsPreview=true)** : 조준 시 결과를 미리 보여주는 목록. 자동 소멸하지 않고 고정 자리에 떠 있다가,
 *    그 로그가 속한 모션이 끝나거나(MotionFinished) 전체 클리어될 때만 사라진다. 스태거 없이 즉시 뜬다.
 *
 *  ## 로그 한 건의 일생
 *   1. 수신  HandleCombatFloatingLog(Request)      — 실행이면 대기 큐로, 미리보기면 즉시 스폰
 *   2. 스폰  SpawnFloatingCombatLogAtWorld(Request) — 아이콘+텍스트 위젯 생성→캔버스 부착→추적목록(mFloatingCombatLogs) 등록
 *   3. 갱신  UpdateFloatingCombatLogs(dt)           — 매 프레임 월드→스크린 재배치 + (실행)상승·페이드·수명소멸
 *   4. 제거  아래 3경로 중 하나
 *        - 자동      : 실행 로그가 수명(1.2s) 다함 (Update 안에서)
 *        - 모션 종료 : RemoveFloatingCombatLogsByMotionIndex(N) — 같은 mMotionIndex 로그를 통째로
 *        - 전체      : HandleCombatFloatingLogsCleared() — 시뮬 전환/취소 시 전부
 *
 *  ## 데이터 경계 (누가 뭘 정하나)
 *  들어오는 값은 전부 CombatUIModel 델리게이트로만 받는다
 *  (OnCombatFloatingLog / OnCombatFloatingLogMotionFinished / OnCombatFloatingLogsCleared / FTurnUI.mRound=배너).
 *  게임플레이는 아이콘/색 "의미(enum)"만 넘기고, 실제 텍스처·FLinearColor는 여기(ResolveFloatingLogIcon/Color)가 정한다.
 */

namespace
{
	constexpr float FloatingLogLifetime = 1.2f;       // 플로팅 로그 표시 수명(초)
	constexpr float FloatingLogQueueInterval = 0.28f; // 순차 로그 간 표시 간격(초)
	constexpr float FloatingLogRiseSpeed = 46.0f;     // 초당 상승 픽셀
	constexpr float FloatingLogBaseOffsetY = -96.0f;  // 머리 위 HP바(-70)보다 위에서 시작
	constexpr float FloatingLogFadePortion = 0.6f;    // 수명의 이 비율 이후부터 페이드아웃
	constexpr float FloatingLogPreviewRowSpacing = 30.0f; // 미리보기 로그가 같은 위치에 겹칠 때 위로 쌓는 간격(px)
	constexpr float TurnBannerLifetime = 1.6f;        // 배너 표시 수명(초)
	constexpr float TurnBannerFadePortion = 0.55f;    // 배너 페이드 시작 비율

	/**
	 * @brief 색 의미(enum) → 실제 화면 색. "어떤 빨강이냐" 같은 디자인 값은 게임플레이가 아니라 여기서 정한다.
	 * @param ColorType 게임플레이가 넘긴 의미값(Damage/Heal/Buff/Debuff/Warning/Move/Neutral).
	 * @return 해당 의미에 대응하는 실제 색. 미지정(Neutral·그 외)은 흰색.
	 */
	FLinearColor ResolveFloatingLogColor(EFloatingLogColorType ColorType)
	{
		switch (ColorType)
		{
		case EFloatingLogColorType::Damage:
			return FLinearColor(1.0f, 0.25f, 0.2f, 1.0f);
		case EFloatingLogColorType::Heal:
			return FLinearColor(0.35f, 1.0f, 0.4f, 1.0f);
		case EFloatingLogColorType::Buff:
			return FLinearColor(0.45f, 0.75f, 1.0f, 1.0f);
		case EFloatingLogColorType::Debuff:
			return FLinearColor(0.75f, 0.45f, 1.0f, 1.0f);
		case EFloatingLogColorType::Warning:
			return FLinearColor(1.0f, 0.8f, 0.2f, 1.0f);
		case EFloatingLogColorType::Move:
			return FLinearColor(0.85f, 0.85f, 0.85f, 1.0f);
		case EFloatingLogColorType::Neutral:
		default:
			return FLinearColor::White;
		}
	}
}

/**
 * @brief 아이콘 의미(enum) → 실제 텍스처. None이면 아이콘 없이 텍스트만 띄운다.
 * @param IconType 게임플레이가 넘긴 의미값(HP/Poison/Fire/Shield/Move/None).
 * @return 해당 텍스처. None이면 nullptr. (첫 로드 후 엔진 캐시에 남아 재로드 비용 없음)
 * @note 현재는 HP 하트 하나만 연결돼 있어 None을 뺀 모든 종류가 임시로 하트로 나온다.
 *       전용 에셋이 정해지면 종류별 case를 분리해 경로만 갈아끼우면 된다.
 */
UTexture2D* UCombatTileMapHUDWidget::ResolveFloatingLogIcon(EFloatingLogIconType IconType) const
{
	// None은 아이콘 없음(텍스트만). 나머지 아이콘 종류는 임시로 전부 HP(하트) 텍스처로 연결한다.
	// Poison/Fire/GetDefense/Move 전용 아이콘 에셋이 준비되면 case를 분리해 경로만 갈아끼우면 됨.
	if (IconType == EFloatingLogIconType::None)
	{
		return nullptr;
	}

	return mFloatingLogHpIconTexture;
}

/**
 * @brief [수신] 게임플레이가 보낸 플로팅 로그 요청 1건을 접수한다(OnCombatFloatingLog 구독).
 * @details 미리보기(mIsPreview)면 조준 목록이 한꺼번에 보여야 하므로 대기 큐를 건너뛰고 즉시 스폰한다.
 *          실행 로그면 대기 큐(mPendingFloatingCombatLogs)에 넣고, UpdateFloatingCombatLogQueue가 간격을 두고 하나씩 꺼내 스폰한다.
 * @param Request 위치/문구/아이콘·색 의미/모션 인덱스/미리보기 여부를 담은 요청(값 복사 — 다이나믹 델리게이트 규약).
 */
void UCombatTileMapHUDWidget::HandleCombatFloatingLog(FCombatFloatingLogRequest Request)
{
	// 미리보기 로그는 스태거 없이 즉시 띄운다(조준 시 목록이 한꺼번에 나열돼야 하므로).
	if (Request.mIsPreview == true)
	{
		SpawnFloatingCombatLogAtWorld(Request);
		return;
	}

	FQueuedFloatingCombatLogEntry Entry;
	Entry.mRequest = Request;
	Entry.mArrivalOrder = mNextFloatingCombatLogArrivalOrder++;
	mPendingFloatingCombatLogs.Add(Entry);
}

/**
 * @brief [제거·모션] 한 모션 연출이 끝났으니 그 모션에 묶인 로그를 걷어낸다(OnCombatFloatingLogMotionFinished 구독).
 * @param MotionIndex 끝난 모션의 배열 인덱스. 같은 인덱스를 가진 로그가 여러 개여도 통째로 사라진다.
 */
void UCombatTileMapHUDWidget::HandleCombatFloatingLogMotionFinished(int32 MotionIndex)
{
	RemoveFloatingCombatLogsByMotionIndex(MotionIndex);
}

/**
 * @brief [제거·전체] 현재 떠 있는/대기 중인 로그를 전부 지운다(OnCombatFloatingLogsCleared 구독).
 * @details 시뮬레이션이 다른 것으로 넘어가 미리보기 목록을 통째로 버려야 할 때 쓰는 안전망.
 *          대기 큐를 먼저 비워 다음 프레임 부활을 막고 큐 상태를 초기화한 뒤, 화면 위젯을 캔버스에서 떼어낸다.
 */
void UCombatTileMapHUDWidget::HandleCombatFloatingLogsCleared()
{
	// 아직 안 뜬 대기분을 먼저 비운다(다음 프레임에 되살아나지 않게).
	mPendingFloatingCombatLogs.Reset();
	mFloatingCombatLogQueueCooldown = 0.0f;
	mNextFloatingCombatLogArrivalOrder = 0;

	// 이미 화면에 떠 있는 로그 위젯을 캔버스에서 즉시 떼어낸다.
	for (FFloatingCombatLogEntry& Entry : mFloatingCombatLogs)
	{
		if (Entry.mRoot != nullptr)
		{
			Entry.mRoot->RemoveFromParent();
		}
	}
	mFloatingCombatLogs.Reset();
}

/**
 * @brief [스폰 페이싱] 대기 큐에서 실행 로그를 FloatingLogQueueInterval 간격으로 하나씩 꺼내 스폰한다.
 * @details 한 번에 여러 로그가 몰려도 같은 프레임에 다 뜨지 않고 순차로 나와 겹침/난잡함을 줄인다. NativeTick에서 매 프레임 호출.
 *          (미리보기 로그는 이 큐를 타지 않고 즉시 스폰되므로 여기 관여하지 않는다.)
 * @param InDeltaTime 이번 프레임 경과 시간(쿨다운 차감용).
 */
void UCombatTileMapHUDWidget::UpdateFloatingCombatLogQueue(float InDeltaTime)
{
	// 지도(풀스크린) 열림 중에는 새 플로팅 로그를 스폰하지 않는다(탑바만 남기는 뷰).
	if (mCombatControlsHidden)
	{
		return;
	}

	if (mPendingFloatingCombatLogs.Num() == 0)
	{
		mFloatingCombatLogQueueCooldown = 0.0f;
		return;
	}

	mFloatingCombatLogQueueCooldown -= InDeltaTime;
	if (mFloatingCombatLogQueueCooldown > 0.0f)
	{
		return;
	}

	const FCombatFloatingLogRequest Request = mPendingFloatingCombatLogs[0].mRequest;
	mPendingFloatingCombatLogs.RemoveAt(0);
	SpawnFloatingCombatLogAtWorld(Request);
	mFloatingCombatLogQueueCooldown = FloatingLogQueueInterval;
}

/**
 * @brief [스폰] 요청 한 건을 실제 위젯(아이콘+텍스트)으로 만들어 캔버스에 붙이고 추적 목록에 등록한다.
 * @details 아이콘 없으면 텍스트만. 바닥-중앙 정렬로 대상 머리 위에 서고 ZOrder 30으로 HP바보다 위에 그린다.
 *          겹침 처리는 종류별로 다르다:
 *           - 미리보기 : 같은 위치에 이미 뜬 미리보기 수만큼 위로 고정 오프셋(mStackOffsetY)을 줘 세로로 쌓는다.
 *           - 실행     : 뒤따르는 숫자와 안 겹치게 같은-위치 기존 로그의 mElapsed를 올려 살짝 먼저 떠오르게 민다.
 *          여기서는 위젯을 만들기만 하고, 실제 위치/투명도 갱신은 UpdateFloatingCombatLogs가 매 프레임 맡는다.
 * @param Request 스폰할 로그 요청.
 */
void UCombatTileMapHUDWidget::SpawnFloatingCombatLogAtWorld(const FCombatFloatingLogRequest& Request)
{
	if (RootCanvas == nullptr || WidgetTree == nullptr)
	{
		return;
	}

	// 아이콘+텍스트를 한 덩어리로 움직이게 가로 박스에 담는다(아이콘 없으면 텍스트만).
	UHorizontalBox* LogBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UTextBlock* LogText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	if (LogBox == nullptr || LogText == nullptr)
	{
		return;
	}
	LogBox->SetVisibility(ESlateVisibility::HitTestInvisible);

	UTexture2D* Icon = ResolveFloatingLogIcon(Request.mIconType);
	if (Icon != nullptr)
	{
		if (UImage* LogIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass()))
		{
			LogIcon->SetBrushFromTexture(Icon, false);
			LogIcon->SetDesiredSizeOverride(FVector2D(26.0f, 26.0f));
			if (UHorizontalBoxSlot* IconSlot = LogBox->AddChildToHorizontalBox(LogIcon))
			{
				IconSlot->SetVerticalAlignment(VAlign_Center);
				IconSlot->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));
			}
		}
	}

	LogText->SetJustification(ETextJustify::Center);
	LogText->SetText(Request.mText);
	LogText->SetColorAndOpacity(FSlateColor(ResolveFloatingLogColor(Request.mColorType)));
	// 기본 폰트에서 크기만 키운다(전투 화면 위에서 읽히는 최소 크기).
	FSlateFontInfo LogFont = LogText->GetFont();
	LogFont.Size = 22;
	LogText->SetFont(LogFont);
	if (UHorizontalBoxSlot* TextSlot = LogBox->AddChildToHorizontalBox(LogText))
	{
		TextSlot->SetVerticalAlignment(VAlign_Center);
	}

	if (UCanvasPanelSlot* LogSlot = RootCanvas->AddChildToCanvas(LogBox))
	{
		LogSlot->SetAutoSize(true);
		LogSlot->SetAlignment(FVector2D(0.5f, 1.0f)); // 바닥-중앙 기준으로 머리 위에 선다.
		LogSlot->SetZOrder(30);                        // HP바보다 위에 그린다.
	}

	FFloatingCombatLogEntry Entry;
	Entry.mRoot = LogBox;
	Entry.mWorldLocation = Request.mWorldLocation;
	Entry.mMotionIndex = Request.mMotionIndex;
	Entry.mElapsed = 0.0f;
	Entry.mIsPreview = Request.mIsPreview;

	if (Request.mIsPreview == true)
	{
		// 미리보기는 상승/페이드가 없으므로, 같은 위치에 이미 뜬 미리보기 수만큼 위로 고정 오프셋을 준다.
		int32 StackCount = 0;
		for (const FFloatingCombatLogEntry& Existing : mFloatingCombatLogs)
		{
			if (Existing.mIsPreview == true && Existing.mWorldLocation.Equals(Request.mWorldLocation, 1.0f))
			{
				++StackCount;
			}
		}
		Entry.mStackOffsetY = StackCount * FloatingLogPreviewRowSpacing;
	}
	mFloatingCombatLogs.Add(Entry);

	// (실행 로그 전용) 같은 위치에 로그가 연달아 뜰 때(예: 데미지+쓰러짐) 겹치지 않게 기존 로그를 위로 민다.
	// 미리보기는 위 StackOffset으로 처리하므로 여기서 건드리지 않는다.
	if (Request.mIsPreview == false)
	{
		for (int32 LogIndex = 0; LogIndex < mFloatingCombatLogs.Num() - 1; ++LogIndex)
		{
			FFloatingCombatLogEntry& Existing = mFloatingCombatLogs[LogIndex];
			if (Existing.mIsPreview == false && Existing.mWorldLocation.Equals(Entry.mWorldLocation, 1.0f))
			{
				Existing.mElapsed = FMath::Max(Existing.mElapsed, 0.35f);
			}
		}
	}
}

/**
 * @brief [갱신] 떠 있는 모든 로그를 매 프레임 월드→스크린 투영으로 재배치하고, 실행 로그는 상승·페이드·수명소멸시킨다.
 * @details 대상이 화면 밖이면 위젯을 숨기되(Collapsed) 수명은 계속 흐른다.
 *          미리보기 로그는 예외 — 수명 소멸/상승/페이드 없이 고정 위치(겹치면 위로 쌓인 자리)에 또렷하게 유지되고,
 *          MotionFinished/Clear로만 제거된다. 역순 순회라 도중에 항목을 지워도 인덱스가 안 꼬인다.
 * @param InDeltaTime 이번 프레임 경과 시간(수명/상승/페이드 누적용).
 */
void UCombatTileMapHUDWidget::UpdateFloatingCombatLogs(float InDeltaTime)
{
	if (mFloatingCombatLogs.Num() == 0)
	{
		return;
	}

	// 지도(풀스크린) 열림 중에는 떠 있는 로그를 숨긴다(수명/이동은 멈춤 — 지도 닫으면 재개).
	if (mCombatControlsHidden)
	{
		for (FFloatingCombatLogEntry& Entry : mFloatingCombatLogs)
		{
			if (Entry.mRoot != nullptr) { Entry.mRoot->SetVisibility(ESlateVisibility::Collapsed); }
		}
		return;
	}

	APlayerController* PlayerController = GetOwningPlayer();

	for (int32 LogIndex = mFloatingCombatLogs.Num() - 1; LogIndex >= 0; --LogIndex)
	{
		FFloatingCombatLogEntry& Entry = mFloatingCombatLogs[LogIndex];
		Entry.mElapsed += InDeltaTime;

		UWidget* LogRoot = Entry.mRoot;
		// 미리보기 로그는 수명으로 사라지지 않는다(MotionFinished/Clear로만). 실행 로그만 수명 만료 시 제거.
		if (LogRoot == nullptr || (Entry.mIsPreview == false && Entry.mElapsed >= FloatingLogLifetime))
		{
			if (LogRoot != nullptr)
			{
				LogRoot->RemoveFromParent();
			}
			mFloatingCombatLogs.RemoveAt(LogIndex);
			continue;
		}

		// HP바와 같은 투영. 화면 밖이면 숨기되 수명은 계속 흘려보낸다.
		FVector2D ScreenPosition;
		const bool bOnScreen = PlayerController != nullptr
			&& UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
				PlayerController, Entry.mWorldLocation, ScreenPosition, false);
		if (bOnScreen == false)
		{
			LogRoot->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}
		LogRoot->SetVisibility(ESlateVisibility::HitTestInvisible);

		if (UCanvasPanelSlot* LogSlot = Cast<UCanvasPanelSlot>(LogRoot->Slot))
		{
			// 미리보기: 고정 위치(겹치면 위로 쌓기) / 실행: 시간에 따라 상승.
			const float OffsetY = Entry.mIsPreview == true
				? (FloatingLogBaseOffsetY - Entry.mStackOffsetY)
				: (FloatingLogBaseOffsetY - FloatingLogRiseSpeed * Entry.mElapsed);
			LogSlot->SetPosition(ScreenPosition + FVector2D(0.0f, OffsetY));
		}

		// 미리보기는 페이드 없이 계속 또렷하게(모션 종료/클리어로만 사라짐).
		if (Entry.mIsPreview == true)
		{
			LogRoot->SetRenderOpacity(1.0f);
			continue;
		}

		// (실행 로그) 수명 후반부에 서서히 사라진다.
		const float FadeStart = FloatingLogLifetime * FloatingLogFadePortion;
		const float Opacity = Entry.mElapsed <= FadeStart
			? 1.0f
			: 1.0f - (Entry.mElapsed - FadeStart) / (FloatingLogLifetime - FadeStart);
		LogRoot->SetRenderOpacity(FMath::Clamp(Opacity, 0.0f, 1.0f));
	}
}

/**
 * @brief 대기·표시 중인 로그에서 MotionIndex가 같은 항목을 모두 제거한다(모션 단위 쳐내기의 실제 구현).
 * @details 매칭을 개별 효과가 아니라 "모션 단위"로 한다 — 로그를 만들 때 실은 mMotionIndex와, "이 모션 끝났다"고
 *          알려온 인덱스를 비교해 그 묶음을 통째로 걷어낸다. 그래서 대상/효과종류/중복순번 같은 개별 식별자가 필요 없다.
 *          INDEX_NONE(모션에 안 묶인 실행 juice)은 대상이 아니라 여기서 즉시 반환한다.
 * @param MotionIndex 제거할 모션 배열 인덱스.
 */
void UCombatTileMapHUDWidget::RemoveFloatingCombatLogsByMotionIndex(int32 MotionIndex)
{
	if (MotionIndex == INDEX_NONE)
	{
		return;
	}

	for (int32 LogIndex = mPendingFloatingCombatLogs.Num() - 1; LogIndex >= 0; --LogIndex)
	{
		if (mPendingFloatingCombatLogs[LogIndex].mRequest.mMotionIndex == MotionIndex)
		{
			mPendingFloatingCombatLogs.RemoveAt(LogIndex);
		}
	}

	for (int32 LogIndex = mFloatingCombatLogs.Num() - 1; LogIndex >= 0; --LogIndex)
	{
		FFloatingCombatLogEntry& Entry = mFloatingCombatLogs[LogIndex];
		if (Entry.mMotionIndex != MotionIndex)
		{
			continue;
		}

		if (Entry.mRoot != nullptr)
		{
			Entry.mRoot->RemoveFromParent();
		}
		mFloatingCombatLogs.RemoveAt(LogIndex);
	}
}

void UCombatTileMapHUDWidget::RefreshTurnRoundBanner()
{
	if (mCombatUIModel == nullptr || RootCanvas == nullptr || WidgetTree == nullptr)
	{
		return;
	}

	// 라운드가 실제로 바뀔 때만 배너를 띄운다. 같은 라운드 안의 턴 전환/스냅샷 push에는 반응하지 않는다.
	const int32 Round = mCombatUIModel->GetTurnUI().mRound;
	if (Round <= 0 || Round == mLastShownTurnRound)
	{
		return;
	}
	mLastShownTurnRound = Round;

	if (mTurnRoundBannerText == nullptr)
	{
		mTurnRoundBannerText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("TurnRoundBannerText"));
		if (mTurnRoundBannerText == nullptr)
		{
			return;
		}
		mTurnRoundBannerText->SetVisibility(ESlateVisibility::HitTestInvisible);
		mTurnRoundBannerText->SetJustification(ETextJustify::Center);
		mTurnRoundBannerText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.92f, 0.62f, 1.0f)));
		FSlateFontInfo BannerFont = mTurnRoundBannerText->GetFont();
		BannerFont.Size = 34;
		mTurnRoundBannerText->SetFont(BannerFont);

		if (UCanvasPanelSlot* BannerSlot = RootCanvas->AddChildToCanvas(mTurnRoundBannerText))
		{
			BannerSlot->SetAutoSize(true);
			BannerSlot->SetAnchors(FAnchors(0.5f, 0.2f)); // 중앙 상단(탑바 아래)
			BannerSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			BannerSlot->SetPosition(FVector2D::ZeroVector);
			BannerSlot->SetZOrder(31);
		}
	}

	mTurnRoundBannerText->SetText(FText::Format(
		NSLOCTEXT("CombatTileMapHUDWidget", "TurnRoundBanner", "{0}번째 턴"), FText::AsNumber(Round)));
	mTurnRoundBannerText->SetRenderOpacity(1.0f);
	mTurnRoundBannerText->SetVisibility(ESlateVisibility::HitTestInvisible);
	mTurnRoundBannerElapsed = 0.0f;
}

void UCombatTileMapHUDWidget::UpdateTurnRoundBanner(float InDeltaTime)
{
	if (mTurnRoundBannerText == nullptr
		|| mTurnRoundBannerText->GetVisibility() == ESlateVisibility::Collapsed)
	{
		return;
	}

	mTurnRoundBannerElapsed += InDeltaTime;
	if (mTurnRoundBannerElapsed >= TurnBannerLifetime)
	{
		mTurnRoundBannerText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const float FadeStart = TurnBannerLifetime * TurnBannerFadePortion;
	const float Opacity = mTurnRoundBannerElapsed <= FadeStart
		? 1.0f
		: 1.0f - (mTurnRoundBannerElapsed - FadeStart) / (TurnBannerLifetime - FadeStart);
	mTurnRoundBannerText->SetRenderOpacity(FMath::Clamp(Opacity, 0.0f, 1.0f));
}
