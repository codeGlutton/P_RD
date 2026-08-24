#include "UI/Combat/CombatLayoutHUDWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "UI/Combat/CombatUIModel.h"

/**
 * @file   CombatLayoutHUDWidget_CombatLog.cpp
 * @brief  전투 플로팅 로그(대상 머리 위 텍스트) + 턴 라운드 배너의 표시 구현.
 *
 * @details
 *  ## 이 파일이 하는 일
 *  게임플레이가 CombatUIModel 경계로 "여기에 이런 로그 띄워라 / 이 모션 로그 치워라"를 보내면,
 *  그걸 받아 월드 좌표 위 위젯으로 그리고, 매 프레임 위치를 갱신하고, 때가 되면 지운다.
 *  좌표는 머리 위 HP바와 똑같은 "월드→스크린 투영"이라 카메라가 움직여도 대상 위에 붙어 다닌다.
 *
 *  ## 로그 두 종류 (mIsPreview로 갈림)
 *  - **실행 로그(mIsPreview=false)** : 위젯 레이어는 "-12" 같은 순간 연출(juice)을 지원한다. 다만 현재 CombatGameMode는
 *    액션 종료 로그를 UI로 보내지 않고 소비만 하므로, 실제 전투 흐름에서는 조준 미리보기 로그만 표시된다.
 *    다시 쓰게 되면 대기 큐(mPendingFloatingCombatLogs)에 쌓였다 FloatingLogQueueInterval 간격으로 하나씩 스폰된다.
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
 *  미리보기 로그만 예외로 예측 전용 모델(USimulationPreviewUIModel)의
 *  OnPreviewEventBatch / OnPreviewCleared 로 받는다 — 실전과 저장 자리가 다르다.
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
	constexpr float FloatingLogDismissDuration = 0.4f;    // 모션 종료 퇴장 연출 길이(초)
	constexpr float FloatingLogDismissSlideSpeed = 140.0f;// 퇴장 시 초당 오른쪽 이동 픽셀
	constexpr float TurnBannerLifetime = 1.6f;        // 배너 표시 수명(초)
	constexpr float TurnBannerFadePortion = 0.55f;    // 배너 페이드 시작 비율

	/**
	 * @brief 색 의미(enum) → 실제 화면 색. "어떤 빨강이냐" 같은 디자인 값은 게임플레이가 아니라 여기서 정한다.
	 * @param ColorType 게임플레이가 넘긴 의미값(Damage/Heal/Buff/Debuff/Warning/Move/Neutral).
	 * @return 해당 의미에 대응하는 실제 색.
	 * @note 0822 확정: 색은 빨강/파랑 둘뿐이다. 해로운 것(피해·디버프·경고)은
	 *       빨강, 나머지(회복·버프·강화·이동·중립)는 파랑. 일곱 색이던 시절에는
	 *       한 화면에서 색이 의미를 잃었다.
	 */
	FLinearColor ResolveFloatingLogColor(EFloatingLogColorType ColorType)
	{
		const FLinearColor HarmfulRed(1.0f, 0.25f, 0.2f, 1.0f);
		const FLinearColor HelpfulBlue(0.4f, 0.7f, 1.0f, 1.0f);
		switch (ColorType)
		{
		case EFloatingLogColorType::Damage:
		case EFloatingLogColorType::Debuff:
		case EFloatingLogColorType::Warning:
			return HarmfulRed;
		default:
			return HelpfulBlue;
		}
	}
}

/**
 * @brief 아이콘 의미(종류+색) → 실제 텍스처. None이면 아이콘 없이 텍스트만 띄운다.
 * @param IconType 게임플레이가 넘긴 아이콘 종류(HP/GetMove/GetDefense/Agility/Fortification/Vulnerability/Weakness 등).
 * @param ColorType 색 의미. HP는 이 값으로 피해(Damage)/회복(Heal) 아이콘을 가른다.
 * @return 해당 텍스처. None이거나 전용 아이콘이 아직 없는 종류(Poison/Fire/Move)면 nullptr.
 * @note StatusIcons 8종 연결됨. Poison/Fire/Move는 발행 배선도 아직 없어 아이콘이 준비돼도 지금은 안 뜬다.
 */
UTexture2D* UCombatLayoutHUDWidget::ResolveFloatingLogIcon(EFloatingLogIconType IconType, EFloatingLogColorType ColorType) const
{
	switch (IconType)
	{
	case EFloatingLogIconType::HP:
		// HP는 색으로 피해/회복을 가른다(Heal=회복, 그 외=피해).
		return ColorType == EFloatingLogColorType::Heal ? mLogIconHpRecovery : mLogIconHpDamage;
	case EFloatingLogIconType::GetMove:       return mLogIconGetMove;
	case EFloatingLogIconType::GetDefense:    return mLogIconGetDefense;
	case EFloatingLogIconType::Vigor:			return mLogIconVigor;
	case EFloatingLogIconType::Fortification: return mLogIconFortification;
	case EFloatingLogIconType::Vulnerability: return mLogIconVulnerability;
	case EFloatingLogIconType::Weakness:      return mLogIconWeakness;
	default:
		// None + 아직 전용 아이콘이 없는 Poison/Fire/Move는 아이콘 없이 텍스트만 띄운다.
		return nullptr;
	}
}

/**
 * @brief [수신] 게임플레이가 보낸 플로팅 로그 요청 1건을 접수한다(OnCombatFloatingLog 구독).
 * @details 미리보기(mIsPreview)면 조준 목록이 한꺼번에 보여야 하므로 대기 큐를 건너뛰고 즉시 스폰한다.
 *          실행 로그면 대기 큐(mPendingFloatingCombatLogs)에 넣고, UpdateFloatingCombatLogQueue가 간격을 두고 하나씩 꺼내 스폰한다.
 * @param Request 위치/문구/아이콘·색 의미/모션 인덱스/미리보기 여부를 담은 요청(값 복사 — 다이나믹 델리게이트 규약).
 */
void UCombatLayoutHUDWidget::HandleCombatFloatingLog(FCombatFloatingLogRequest Request)
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
void UCombatLayoutHUDWidget::HandleCombatFloatingLogMotionFinished(int32 MotionIndex)
{
	RemoveFloatingCombatLogsByMotionIndex(MotionIndex);
}

/**
 * @brief [제거·전체] 현재 떠 있는/대기 중인 로그를 전부 지운다(OnCombatFloatingLogsCleared 구독).
 * @details 시뮬레이션이 다른 것으로 넘어가 미리보기 목록을 통째로 버려야 할 때 쓰는 안전망.
 *          대기 큐를 먼저 비워 다음 프레임 부활을 막고 큐 상태를 초기화한 뒤, 화면 위젯을 캔버스에서 떼어낸다.
 */
void UCombatLayoutHUDWidget::HandleCombatFloatingLogsCleared()
{
	// 아직 안 뜬 대기분을 먼저 비운다(다음 프레임에 되살아나지 않게).
	mPendingFloatingCombatLogs.Reset();
	mFloatingCombatLogQueueCooldown = 0.0f;
	mNextFloatingCombatLogArrivalOrder = 0;

	// 화면에 떠 있는 로그는 즉시 떼지 않고 퇴장 연출(오른쪽으로 흐르며 페이드아웃)로 보낸다.
	// (연출이 끝나면 UpdateFloatingCombatLogs가 제거한다. 애니 종료 후 "뿅" 사라짐 방지.)
	for (int32 LogIndex = mFloatingCombatLogs.Num() - 1; LogIndex >= 0; --LogIndex)
	{
		FFloatingCombatLogEntry& Entry = mFloatingCombatLogs[LogIndex];
		if (Entry.mRoot == nullptr)
		{
			mFloatingCombatLogs.RemoveAt(LogIndex);
			continue;
		}
		if (Entry.mIsDismissing == false)
		{
			Entry.mIsDismissing = true;
			Entry.mDismissElapsed = 0.0f;
		}
	}
}

/**
 * @brief [수신·미리보기] 예측 모델이 보낸 미리보기 배치 하나를 화면에 갈아 끼운다(OnPreviewEventBatch 구독).
 * @details 새 배치는 이전 미리보기 표시를 전제부터 대체하므로, 먼저 미리보기분만 걷고 새 배치를 즉시 스폰한다.
 *          실전 로그(수명 juice)는 건드리지 않는다. 정렬은 실전 경로(NotifyCombatFloatingLogs)와 같은
 *          규칙 — mSequence 오름차순, 같으면 입력 순서(StableSort).
 * @param Batch 미리보기 플로팅 로그 스냅샷(값 복사 — 다이나믹 델리게이트 규약).
 */
void UCombatLayoutHUDWidget::HandleSimulationPreviewBatch(FCombatEventBatchUI Batch)
{
	RetireSimulationPreviewFloatingLogs();

	Batch.mFloatingLogs.StableSort(
		[](const FCombatFloatingLogRequest& Lhs, const FCombatFloatingLogRequest& Rhs) {
			return Lhs.mSequence < Rhs.mSequence;
		});
	for (const FCombatFloatingLogRequest& Request : Batch.mFloatingLogs)
	{
		// 미리보기 배치 규약(mIsPreview=true) 위반분은 실전 대기 큐로 새지 않게 버린다.
		if (Request.mIsPreview == false)
		{
			continue;
		}
		HandleCombatFloatingLog(Request);
	}
}

/**
 * @brief [제거·미리보기] 미리보기가 통째로 버려졌다(OnPreviewCleared 구독). 미리보기 표시만 걷는다.
 * @details 취소/새 조준/턴 종료가 예측 모델의 ClearPreview를 부르면 여기로 온다.
 *          실전 로그는 수명 규칙 그대로 남는다 — 전체 클리어(HandleCombatFloatingLogsCleared)와 다른 점이다.
 */
void UCombatLayoutHUDWidget::HandleSimulationPreviewCleared()
{
	RetireSimulationPreviewFloatingLogs();
}

/**
 * @brief 대기 큐·화면에서 mIsPreview==true 인 로그만 걷어낸다(전체 클리어 로직의 미리보기 한정판).
 * @details 실전 큐 상태(쿨다운·도착 순번)는 건드리지 않는다. 떠 있는 미리보기 로그는 전체 클리어와
 *          같은 퇴장 연출(오른쪽 슬라이드+페이드)로 보내고, 실제 제거는 UpdateFloatingCombatLogs가 한다.
 */
void UCombatLayoutHUDWidget::RetireSimulationPreviewFloatingLogs()
{
	// 아직 안 뜬 대기분 중 미리보기만 비운다(다음 프레임에 되살아나지 않게).
	for (int32 LogIndex = mPendingFloatingCombatLogs.Num() - 1; LogIndex >= 0; --LogIndex)
	{
		if (mPendingFloatingCombatLogs[LogIndex].mRequest.mIsPreview == true)
		{
			mPendingFloatingCombatLogs.RemoveAt(LogIndex);
		}
	}

	for (int32 LogIndex = mFloatingCombatLogs.Num() - 1; LogIndex >= 0; --LogIndex)
	{
		FFloatingCombatLogEntry& Entry = mFloatingCombatLogs[LogIndex];
		if (Entry.mIsPreview == false)
		{
			continue;
		}
		if (Entry.mRoot == nullptr)
		{
			mFloatingCombatLogs.RemoveAt(LogIndex);
			continue;
		}
		if (Entry.mIsDismissing == false)
		{
			Entry.mIsDismissing = true;
			Entry.mDismissElapsed = 0.0f;
		}
	}
}

/**
 * @brief [스폰 페이싱] 대기 큐에서 실행 로그를 FloatingLogQueueInterval 간격으로 하나씩 꺼내 스폰한다.
 * @details 한 번에 여러 로그가 몰려도 같은 프레임에 다 뜨지 않고 순차로 나와 겹침/난잡함을 줄인다. NativeTick에서 매 프레임 호출.
 *          (미리보기 로그는 이 큐를 타지 않고 즉시 스폰되므로 여기 관여하지 않는다.)
 * @param InDeltaTime 이번 프레임 경과 시간(쿨다운 차감용).
 */
void UCombatLayoutHUDWidget::UpdateFloatingCombatLogQueue(float InDeltaTime)
{
	// 지도(풀스크린) 열림 중에는 새 플로팅 로그를 스폰하지 않는다(탑바만 남기는 뷰).
	if (GetVisibility() == ESlateVisibility::Collapsed)
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
void UCombatLayoutHUDWidget::SpawnFloatingCombatLogAtWorld(const FCombatFloatingLogRequest& Request)
{
	if (mRootCanvas == nullptr || WidgetTree == nullptr)
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

	UTexture2D* Icon = ResolveFloatingLogIcon(Request.mIconType, Request.mColorType);
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
	// HUD 공용 숫자 글꼴로 통일한다. 한 게임에서 글꼴이 둘이면 같은 값도
	// 다른 화면처럼 읽힌다. 글꼴을 못 물어 왔으면 기본 글꼴로 크기만 키운다.
	FSlateFontInfo LogFont = LogText->GetFont();
	if (mFloatingLogFont != nullptr)
	{
		LogFont.FontObject = mFloatingLogFont;
	}
	LogFont.Size = 22;
	// 밝은 배경(모래/눈밭) 위에서도 읽히도록 검은 윤곽선을 두른다.
	LogFont.OutlineSettings.OutlineSize = 2;
	LogFont.OutlineSettings.OutlineColor = FLinearColor::Black;
	LogText->SetFont(LogFont);
	if (UHorizontalBoxSlot* TextSlot = LogBox->AddChildToHorizontalBox(LogText))
	{
		TextSlot->SetVerticalAlignment(VAlign_Center);
	}

	if (UCanvasPanelSlot* LogSlot = mRootCanvas->AddChildToCanvas(LogBox))
	{
		LogSlot->SetAutoSize(true);
		LogSlot->SetAlignment(FVector2D(0.5f, 1.0f)); // 바닥-중앙 기준으로 머리 위에 선다.
		LogSlot->SetZOrder(30);                        // HP바보다 위에 그린다.
	}

	FFloatingCombatLogEntry Entry;
	Entry.mRoot = LogBox;
	Entry.mWorldLocation = Request.mWorldLocation;
	Entry.mTurnIndex = Request.mTurnIndex;
	Entry.mActionIndex = Request.mActionIndex;
	Entry.mMotionIndex = Request.mMotionIndex;
	Entry.mElapsed = 0.0f;
	Entry.mIsPreview = Request.mIsPreview;

	if (Request.mIsPreview == true)
	{
		// 미리보기는 상승/페이드가 없으므로, 같은 위치에 이미 뜬 미리보기 수만큼 위로 고정 오프셋을 준다.
		int32 StackCount = 0;
		for (const FFloatingCombatLogEntry& Existing : mFloatingCombatLogs)
		{
			// 퇴장 중인 로그는 곧 사라지므로 쌓기 오프셋 계산에서 제외한다.
			if (Existing.mIsPreview == true && Existing.mIsDismissing == false
				&& Existing.mWorldLocation.Equals(Request.mWorldLocation, 1.0f))
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
void UCombatLayoutHUDWidget::UpdateFloatingCombatLogs(float InDeltaTime)
{
	if (mFloatingCombatLogs.Num() == 0)
	{
		return;
	}

	// 지도(풀스크린) 열림 중에는 떠 있는 로그를 숨긴다(수명/이동은 멈춤 — 지도 닫으면 재개).
	if (GetVisibility() == ESlateVisibility::Collapsed)
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
		// 퇴장 연출 중이면 수명 만료보다 퇴장(슬라이드+페이드) 완료를 우선한다.
		if (LogRoot == nullptr || (Entry.mIsPreview == false && Entry.mIsDismissing == false && Entry.mElapsed >= FloatingLogLifetime))
		{
			if (LogRoot != nullptr)
			{
				LogRoot->RemoveFromParent();
			}
			mFloatingCombatLogs.RemoveAt(LogIndex);
			continue;
		}

		// 퇴장 중(모션 종료)이면 오른쪽으로 흐르며 페이드아웃하고 연출이 끝나면 제거한다.
		if (Entry.mIsDismissing == true)
		{
			Entry.mDismissElapsed += InDeltaTime;
			if (Entry.mDismissElapsed >= FloatingLogDismissDuration)
			{
				LogRoot->RemoveFromParent();
				mFloatingCombatLogs.RemoveAt(LogIndex);
				continue;
			}
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

		// 퇴장 중에는 진행률만큼 오른쪽으로 밀리는 X 오프셋이 붙는다.
		const float DismissOffsetX = Entry.mIsDismissing == true
			? FloatingLogDismissSlideSpeed * Entry.mDismissElapsed
			: 0.0f;

		if (UCanvasPanelSlot* LogSlot = Cast<UCanvasPanelSlot>(LogRoot->Slot))
		{
			// 미리보기: 고정 위치(겹치면 위로 쌓기) / 실행: 시간에 따라 상승.
			const float OffsetY = Entry.mIsPreview == true
				? (FloatingLogBaseOffsetY - Entry.mStackOffsetY)
				: (FloatingLogBaseOffsetY - FloatingLogRiseSpeed * Entry.mElapsed);
			LogSlot->SetPosition(ScreenPosition + FVector2D(DismissOffsetX, OffsetY));
		}

		// 퇴장 중에는 진행률에 따라 서서히 투명해진다(미리보기/실행 공통).
		if (Entry.mIsDismissing == true)
		{
			LogRoot->SetRenderOpacity(FMath::Clamp(1.0f - Entry.mDismissElapsed / FloatingLogDismissDuration, 0.0f, 1.0f));
			continue;
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
void UCombatLayoutHUDWidget::RemoveFloatingCombatLogsByMotionIndex(int32 MotionIndex)
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

		// 뿅 사라지지 않고 퇴장 연출(오른쪽으로 흐르며 페이드아웃)로 넘긴다. 실제 제거는 UpdateFloatingCombatLogs가 한다.
		if (Entry.mRoot != nullptr)
		{
			Entry.mIsDismissing = true;
			Entry.mDismissElapsed = 0.0f;
		}
		else
		{
			mFloatingCombatLogs.RemoveAt(LogIndex);
		}
	}
}


