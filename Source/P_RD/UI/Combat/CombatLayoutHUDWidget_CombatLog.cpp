#include "UI/Combat/CombatLayoutHUDWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateBrush.h"
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
	constexpr float FloatingLogLifetime = 1.45f;      // 읽을 시간을 확보한 플로팅 로그 표시 수명(초)
	constexpr float FloatingLogQueueInterval = 0.18f; // 연타는 타격 템포를 유지하되 같은 프레임에는 겹치지 않는다
	constexpr float FloatingLogRiseSpeed = 34.0f;     // 숫자가 너무 빨리 도망가지 않도록 완만하게 상승
	constexpr float FloatingLogBaseOffsetY = -96.0f;  // 머리 위 HP바(-70)보다 위에서 시작
	constexpr float FloatingLogFadePortion = 0.72f;   // 일반 텍스트 로그의 페이드 시작 비율
	constexpr float DamageNumberImpactEnd = 0.18f;    // 숫자 팝이 끝나 제자리에 안착하는 시점
	constexpr float DamageNumberExitStart = 0.92f;    // 숫자를 충분히 읽힌 뒤 퇴장을 시작하는 시점
	constexpr float DamageNumberStackSpacing = 30.0f;// 같은 대상에 동시에 뜬 결과 사이 간격(px)
	constexpr float FloatingLogPreviewRowSpacing = 30.0f; // 미리보기 로그가 같은 위치에 겹칠 때 위로 쌓는 간격(px)
	constexpr float FloatingLogDismissDuration = 0.4f;    // 모션 종료 퇴장 연출 길이(초)
	constexpr float FloatingLogDismissSlideSpeed = 140.0f;// 퇴장 시 초당 오른쪽 이동 픽셀
	constexpr float TurnBannerLifetime = 1.6f;        // 배너 표시 수명(초)
	constexpr float TurnBannerFadePortion = 0.55f;    // 배너 페이드 시작 비율
	constexpr int32 DamageNumberAtlasColumns = 4;
	constexpr int32 DamageNumberAtlasRows = 3;
	constexpr float DamageSlashCoreLength = 126.0f;
	constexpr float DamageSlashGlowLength = 142.0f;

	UImage* AddDamageSlashLayer(UWidgetTree* WidgetTree, UOverlay* Parent,
		UTexture2D* WhiteTexture, const FVector2D& Size, const float Angle,
		const FLinearColor& Color)
	{
		if (WidgetTree == nullptr || Parent == nullptr || WhiteTexture == nullptr)
		{
			return nullptr;
		}

		UImage* Slash = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		if (Slash == nullptr)
		{
			return nullptr;
		}

		Slash->SetBrushFromTexture(WhiteTexture, false);
		Slash->SetDesiredSizeOverride(Size);
		Slash->SetColorAndOpacity(Color);
		Slash->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Slash->SetRenderTransformAngle(Angle);
		Slash->SetRenderOpacity(0.0f);
		Slash->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UOverlaySlot* Slot = Parent->AddChildToOverlay(Slash))
		{
			Slot->SetHorizontalAlignment(HAlign_Center);
			Slot->SetVerticalAlignment(VAlign_Center);
		}
		return Slash;
	}

	int32 DamageNumberGlyphIndex(const TCHAR Character)
	{
		if (Character >= TEXT('0') && Character <= TEXT('9'))
		{
			return Character - TEXT('0');
		}
		if (Character == TEXT('+'))
		{
			return 10;
		}
		if (Character == TEXT('-'))
		{
			return 11;
		}
		return INDEX_NONE;
	}

	bool AddDamageNumberGlyphs(UWidgetTree* WidgetTree, UHorizontalBox* Parent,
		UTexture2D* Atlas, const FText& NumberText, const bool bCritical)
	{
		if (WidgetTree == nullptr || Parent == nullptr || Atlas == nullptr)
		{
			return false;
		}

		const FString Characters = NumberText.ToString();
		if (Characters.IsEmpty())
		{
			return false;
		}
		for (const TCHAR Character : Characters)
		{
			if (DamageNumberGlyphIndex(Character) == INDEX_NONE)
			{
				return false;
			}
		}

		const FIntPoint ImportedSize = Atlas->GetImportedSize();
		const float CellAspect = ImportedSize.Y > 0
			? (static_cast<float>(ImportedSize.X) / DamageNumberAtlasColumns)
				/ (static_cast<float>(ImportedSize.Y) / DamageNumberAtlasRows)
			: 0.8f;
		const float GlyphHeight = bCritical ? 72.0f : 60.0f;
		const float GlyphWidth = GlyphHeight * CellAspect;

		for (const TCHAR Character : Characters)
		{
			const int32 GlyphIndex = DamageNumberGlyphIndex(Character);
			const int32 Column = GlyphIndex % DamageNumberAtlasColumns;
			const int32 Row = GlyphIndex / DamageNumberAtlasColumns;
			const FVector2f UVMin(
				static_cast<float>(Column) / DamageNumberAtlasColumns,
				static_cast<float>(Row) / DamageNumberAtlasRows);
			const FVector2f UVMax(
				static_cast<float>(Column + 1) / DamageNumberAtlasColumns,
				static_cast<float>(Row + 1) / DamageNumberAtlasRows);

			UImage* Glyph = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			if (Glyph == nullptr)
			{
				return false;
			}
			FSlateBrush Brush;
			Brush.SetResourceObject(Atlas);
			Brush.DrawAs = ESlateBrushDrawType::Image;
			Brush.ImageSize = FVector2D(GlyphWidth, GlyphHeight);
			Brush.SetUVRegion(FBox2f(UVMin, UVMax));
			Glyph->SetBrush(Brush);
			Glyph->SetDesiredSizeOverride(FVector2D(GlyphWidth, GlyphHeight));
			Glyph->SetVisibility(ESlateVisibility::HitTestInvisible);
			if (UHorizontalBoxSlot* GlyphSlot = Parent->AddChildToHorizontalBox(Glyph))
			{
				GlyphSlot->SetVerticalAlignment(VAlign_Center);
				GlyphSlot->SetPadding(FMargin(-5.0f, 0.0f, -5.0f, 0.0f));
			}
		}
		return true;
	}

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
 * @return 해당 텍스처. 전용 그림이 없는 상태는 범용 상태 프레임을 쓴다.
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
	case EFloatingLogIconType::Status:        return mUnitStatusSlotTexture;
	case EFloatingLogIconType::Poison:        return mLogIconPoison;
	case EFloatingLogIconType::Stun:          return mLogIconStun;
	case EFloatingLogIconType::Move:          return mLogIconGetMove;
	default:
		// None과 아직 정의되지 않은 원소는 문구만 띄운다.
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
	// 실전 로그도 이제 애니메이션의 실제 타격 이벤트마다 들어온다. 여기서 다시
	// 0.18초씩 지연하면 광역기의 같은 타격이 대상마다 따로 맞는 것처럼 보이므로
	// 미리보기와 마찬가지로 받은 프레임에 즉시 스폰한다.
	SpawnFloatingCombatLogAtWorld(Request);
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

	// 아이콘+텍스트 또는 이미지 숫자를 한 덩어리로 움직이게 가로 박스에 담는다.
	UHorizontalBox* LogBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (LogBox == nullptr)
	{
		return;
	}
	LogBox->SetVisibility(ESlateVisibility::HitTestInvisible);
	LogBox->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));

	UTexture2D* DamageNumberAtlas = nullptr;
	if (Request.mIsPreview == false && Request.mIconType == EFloatingLogIconType::HP)
	{
		if (Request.mColorType == EFloatingLogColorType::Heal)
		{
			DamageNumberAtlas = mDamageNumberHealAtlas;
		}
		else if (Request.mColorType == EFloatingLogColorType::Damage)
		{
			DamageNumberAtlas = Request.mIsCritical
				? mDamageNumberCriticalAtlas : mDamageNumberNormalAtlas;
		}
	}
	const bool bUsesDamageNumberSkin = AddDamageNumberGlyphs(
		WidgetTree, LogBox, DamageNumberAtlas, Request.mText, Request.mIsCritical);

	if (bUsesDamageNumberSkin == false)
	{
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

		UTextBlock* LogText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		if (LogText == nullptr)
		{
			return;
		}
		LogText->SetJustification(ETextJustify::Center);
		LogText->SetText(Request.mText);
		LogText->SetColorAndOpacity(FSlateColor(ResolveFloatingLogColor(Request.mColorType)));
		FSlateFontInfo LogFont = LogText->GetFont();
		if (mFloatingLogFont != nullptr)
		{
			LogFont.FontObject = mFloatingLogFont;
		}
		LogFont.Size = 22;
		LogFont.OutlineSettings.OutlineSize = 2;
		LogFont.OutlineSettings.OutlineColor = FLinearColor::Black;
		LogText->SetFont(LogFont);
		if (UHorizontalBoxSlot* TextSlot = LogBox->AddChildToHorizontalBox(LogText))
		{
			TextSlot->SetVerticalAlignment(VAlign_Center);
		}
	}

	UWidget* LogRoot = LogBox;
	UImage* SlashGlowA = nullptr;
	UImage* SlashGlowB = nullptr;
	UImage* SlashCoreA = nullptr;
	UImage* SlashCoreB = nullptr;
	if (bUsesDamageNumberSkin)
	{
		// 숫자는 전경에 고정하고, 짧게 터지는 X자 접촉광만 별도 레이어로 움직인다.
		// 흰 사각 엔진 텍스처를 가늘게 늘여 쓰므로 별도 VFX 에셋 없이도 색/길이 조절이 가능하다.
		UOverlay* EffectRoot = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		UTexture2D* WhiteTexture = LoadObject<UTexture2D>(
			nullptr, TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
		if (EffectRoot != nullptr && WhiteTexture != nullptr)
		{
			EffectRoot->SetVisibility(ESlateVisibility::HitTestInvisible);
			EffectRoot->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));

			const bool bHeal = Request.mColorType == EFloatingLogColorType::Heal;
			const FLinearColor GlowColor = bHeal
				? FLinearColor(0.35f, 1.0f, 0.08f, 0.38f)
				: (Request.mIsCritical
					? FLinearColor(1.0f, 0.06f, 0.01f, 0.58f)
					: FLinearColor(1.0f, 0.16f, 0.01f, 0.42f));
			const FLinearColor CoreColor = bHeal
				? FLinearColor(0.92f, 1.0f, 0.32f, 0.90f)
				: (Request.mIsCritical
					? FLinearColor(1.0f, 0.90f, 0.34f, 1.0f)
					: FLinearColor(1.0f, 0.48f, 0.08f, 0.96f));
			const float Angle = bHeal ? 24.0f : 34.0f;
			const float LengthMultiplier = Request.mIsCritical ? 1.22f : 1.0f;

			SlashGlowA = AddDamageSlashLayer(WidgetTree, EffectRoot, WhiteTexture,
				FVector2D(DamageSlashGlowLength * LengthMultiplier, bHeal ? 8.0f : 12.0f), Angle, GlowColor);
			SlashGlowB = AddDamageSlashLayer(WidgetTree, EffectRoot, WhiteTexture,
				FVector2D(DamageSlashGlowLength * LengthMultiplier, bHeal ? 8.0f : 12.0f), -Angle, GlowColor);
			SlashCoreA = AddDamageSlashLayer(WidgetTree, EffectRoot, WhiteTexture,
				FVector2D(DamageSlashCoreLength * LengthMultiplier, bHeal ? 2.0f : 4.0f), Angle, CoreColor);
			SlashCoreB = AddDamageSlashLayer(WidgetTree, EffectRoot, WhiteTexture,
				FVector2D(DamageSlashCoreLength * LengthMultiplier, bHeal ? 2.0f : 4.0f), -Angle, CoreColor);

			if (UOverlaySlot* NumberSlot = EffectRoot->AddChildToOverlay(LogBox))
			{
				NumberSlot->SetHorizontalAlignment(HAlign_Center);
				NumberSlot->SetVerticalAlignment(VAlign_Center);
			}
			LogRoot = EffectRoot;
		}
	}

	if (UCanvasPanelSlot* LogSlot = mRootCanvas->AddChildToCanvas(LogRoot))
	{
		LogSlot->SetAutoSize(true);
		LogSlot->SetAlignment(FVector2D(0.5f, 1.0f)); // 바닥-중앙 기준으로 머리 위에 선다.
		LogSlot->SetZOrder(30);                        // HP바보다 위에 그린다.
	}

	FFloatingCombatLogEntry Entry;
	Entry.mRoot = LogRoot;
	Entry.mNumberRoot = LogBox;
	Entry.mSlashGlowA = SlashGlowA;
	Entry.mSlashGlowB = SlashGlowB;
	Entry.mSlashCoreA = SlashCoreA;
	Entry.mSlashCoreB = SlashCoreB;
	Entry.mWorldLocation = Request.mWorldLocation;
	Entry.mTurnIndex = Request.mTurnIndex;
	Entry.mActionIndex = Request.mActionIndex;
	Entry.mMotionIndex = Request.mMotionIndex;
	Entry.mElapsed = 0.0f;
	Entry.mIsPreview = Request.mIsPreview;
	Entry.mUsesDamageNumberSkin = bUsesDamageNumberSkin;
	Entry.mIsCritical = Request.mIsCritical;
	Entry.mIsHeal = bUsesDamageNumberSkin
		&& Request.mColorType == EFloatingLogColorType::Heal;

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

	// 같은 타격에서 한 대상에게 피해와 상태 결과가 함께 떠도 겹치지 않게
	// 먼저 뜬 로그를 한 줄 위로 민다. 시간값을 강제로 당기지 않아 충돌 연출은 보존한다.
	if (Request.mIsPreview == false)
	{
		for (int32 LogIndex = 0; LogIndex < mFloatingCombatLogs.Num() - 1; ++LogIndex)
		{
			FFloatingCombatLogEntry& Existing = mFloatingCombatLogs[LogIndex];
			if (Existing.mIsPreview == false && Existing.mWorldLocation.Equals(Entry.mWorldLocation, 1.0f))
			{
				Existing.mStackOffsetY += DamageNumberStackSpacing;
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

		float DamageNumberOffsetX = 0.0f;
		float DamageNumberOffsetY = 0.0f;

		// 이미지 숫자는 타격점에서 팝→정착한 뒤 움직이지 않는다.
		// 충격의 방향과 속도는 숫자가 아니라 뒤쪽 X자 접촉광이 담당한다.
		if (Entry.mUsesDamageNumberSkin == true && Entry.mIsPreview == false)
		{
			UWidget* NumberRoot = Entry.mNumberRoot != nullptr ? Entry.mNumberRoot.Get() : LogRoot;
			FVector2D NumberScale = FVector2D::UnitVector;
			const float PeakScale = Entry.mIsCritical ? 1.18f : (Entry.mIsHeal ? 1.08f : 1.10f);
			if (Entry.mElapsed < 0.065f)
			{
				const float Alpha = FMath::SmoothStep(0.0f, 1.0f, Entry.mElapsed / 0.065f);
				const float ScaleValue = FMath::Lerp(0.82f, PeakScale, Alpha);
				NumberScale = FVector2D(ScaleValue, ScaleValue);
			}
			else if (Entry.mElapsed < DamageNumberImpactEnd)
			{
				const float Alpha = FMath::SmoothStep(0.0f, 1.0f,
					(Entry.mElapsed - 0.065f) / (DamageNumberImpactEnd - 0.065f));
				const float ScaleValue = FMath::Lerp(PeakScale, 1.0f, Alpha);
				NumberScale = FVector2D(ScaleValue, ScaleValue);
			}

			if (Entry.mElapsed >= DamageNumberExitStart)
			{
				const float ExitAlpha = FMath::Clamp((Entry.mElapsed - DamageNumberExitStart)
					/ (FloatingLogLifetime - DamageNumberExitStart), 0.0f, 1.0f);
				const float ScaleValue = FMath::Lerp(1.0f, 0.96f, ExitAlpha);
				NumberScale = FVector2D(ScaleValue, ScaleValue);
			}
			NumberRoot->SetRenderScale(NumberScale);
			NumberRoot->SetRenderTransformAngle(0.0f);

			// X자 참격은 0.06초에 터지고, 약 0.3초 동안 길이 잔상만 남긴다.
			const float EffectEnd = Entry.mIsCritical ? 0.40f : (Entry.mIsHeal ? 0.46f : 0.32f);
			float SlashLengthScale = 0.0f;
			float SlashCoreOpacity = 0.0f;
			float SlashGlowOpacity = 0.0f;
			if (Entry.mElapsed < 0.06f)
			{
				const float Alpha = FMath::SmoothStep(0.0f, 1.0f, Entry.mElapsed / 0.06f);
				SlashLengthScale = FMath::Lerp(0.05f, 1.15f, Alpha);
				SlashCoreOpacity = Alpha;
				SlashGlowOpacity = Alpha * (Entry.mIsHeal ? 0.65f : 1.0f);
			}
			else if (Entry.mElapsed < EffectEnd)
			{
				const float Alpha = FMath::Clamp((Entry.mElapsed - 0.06f)
					/ (EffectEnd - 0.06f), 0.0f, 1.0f);
				SlashLengthScale = FMath::Lerp(1.15f, 1.0f, Alpha);
				SlashCoreOpacity = 1.0f - FMath::SmoothStep(0.0f, 0.62f, Alpha);
				SlashGlowOpacity = 1.0f - FMath::SmoothStep(0.18f, 1.0f, Alpha);
			}

			for (UWidget* Slash : { Entry.mSlashGlowA.Get(), Entry.mSlashGlowB.Get() })
			{
				if (Slash != nullptr)
				{
					Slash->SetRenderScale(FVector2D(SlashLengthScale, 1.0f));
					Slash->SetRenderOpacity(SlashGlowOpacity);
				}
			}
			for (UWidget* Slash : { Entry.mSlashCoreA.Get(), Entry.mSlashCoreB.Get() })
			{
				if (Slash != nullptr)
				{
					Slash->SetRenderScale(FVector2D(SlashLengthScale, 1.0f));
					Slash->SetRenderOpacity(SlashCoreOpacity);
				}
			}

			// 숫자 전체는 월드 타격점에 고정한다. 마지막까지 Y 이동을 주지 않는다.
			DamageNumberOffsetY = -10.0f;
		}

		// 퇴장 중에는 진행률만큼 오른쪽으로 밀리는 X 오프셋이 붙는다.
		const float DismissOffsetX = Entry.mIsDismissing == true && Entry.mUsesDamageNumberSkin == false
			? FloatingLogDismissSlideSpeed * Entry.mDismissElapsed
			: 0.0f;

		if (UCanvasPanelSlot* LogSlot = Cast<UCanvasPanelSlot>(LogRoot->Slot))
		{
			// 미리보기: 고정 / 이미지 숫자: 충돌 곡선 / 일반 텍스트: 기존 선형 상승.
			const float OffsetY = Entry.mIsPreview == true
				? (FloatingLogBaseOffsetY - Entry.mStackOffsetY)
				: (Entry.mUsesDamageNumberSkin
					? FloatingLogBaseOffsetY - Entry.mStackOffsetY + DamageNumberOffsetY
					: FloatingLogBaseOffsetY - Entry.mStackOffsetY - FloatingLogRiseSpeed * Entry.mElapsed);
			LogSlot->SetPosition(ScreenPosition
				+ FVector2D(DismissOffsetX + DamageNumberOffsetX, OffsetY));
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
		const float FadeStart = Entry.mUsesDamageNumberSkin
			? DamageNumberExitStart
			: FloatingLogLifetime * FloatingLogFadePortion;
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


