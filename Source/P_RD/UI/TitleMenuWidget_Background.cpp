/**
 * @file TitleMenuWidget_Background.cpp
 * @brief 타이틀 메뉴 위젯의 "배경 루프 영상" 표시/뷰포트 핏 로직 구현부.
 * @details
 *   WBP_TitleMenu가 제공한 배경 Image 슬롯에 런타임 MediaTexture(루프 mp4)를 브러시로 주입하고,
 *   영상 원본 비율을 기준으로 화면 전체를 덮도록 좌우맞춤 + 상하크롭(중앙 기준)으로 비율을 맞춘다.
 *   미디어 객체(Player/Texture/Source)는 WBP에 저장하지 않고 런타임 Transient로만 다루며,
 *   UTitleBackgroundVideoSubsystem이 미리 만들어 둔 공유 미디어가 있으면 그것을 우선 재사용한다.
 *   (UTitleMenuWidget의 나머지 멤버/메뉴 로직은 다른 번역 단위에 있다. 여기는 배경 영상 전용 partial.)
 * @author 박용수
 * @date 2026-06-26
 */
#include "UI/TitleMenuWidget.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "FileMediaSource.h"
#include "Framework/Application/SlateApplication.h"
#include "MediaPlayer.h"
#include "MediaTexture.h"
#include "Misc/FileHelper.h"
#include "Setting/GamePlaySettings.h"
#include "Singleton/InstanceSubsystem/TitleBackgroundVideoSubsystem.h"
#include "UI/UITextureLoader.h"
#include "Widgets/SWindow.h"

// 이 번역 단위 내부에서만 쓰는 배경 영상 핏 보조 함수/상수 모음(외부 링크 노출 방지용 익명 namespace).
namespace
{
	const TCHAR* const FallbackTitleBackgroundVideoPath = TEXT("SVN/OutSideAsset/AICreation/campfire_titleloop_idle_x3preview.mp4");

	FString GetTitleBackgroundVideoPath()
	{
		const UGamePlaySettings* GamePlaySettings = GetDefault<UGamePlaySettings>();
		if (GamePlaySettings != nullptr && GamePlaySettings->mTitleBackgroundVideoPath.IsEmpty() == false)
		{
			return GamePlaySettings->mTitleBackgroundVideoPath;
		}

		return FString(FallbackTitleBackgroundVideoPath);
	}

	/**
	 * @brief 후보 크기가 유효(양수)하고 면적이 더 크면 현재 최선값을 갱신한다.
	 * @details 뷰포트 크기를 여러 소스(게임 뷰포트/창/위젯 지오메트리)에서 모아 가장 신뢰도 높은(=가장 큰) 값을 고르기 위한 헬퍼.
	 * @param Candidate 검사할 후보 크기(0 이하 성분이 있으면 무시).
	 * @param InOutBest 지금까지의 최선 크기. 후보 면적이 더 크면 여기에 덮어쓴다.
	 */
	void PickLargestPositiveSize(FVector2D Candidate, FVector2D& InOutBest)
	{
		if (Candidate.X <= 0.0f || Candidate.Y <= 0.0f)
		{
			return;
		}

		// 면적(X*Y) 비교로 더 큰 후보를 채택한다. 0 크기 지오메트리(아직 레이아웃 전)는 위 가드에서 이미 걸러진다.
		if (Candidate.X * Candidate.Y > InOutBest.X * InOutBest.Y)
		{
			InOutBest = Candidate;
		}
	}

	/**
	 * @brief 위젯의 캐시된 지오메트리 로컬 크기로부터 면적을 구한다(유효하지 않으면 0).
	 * @param Widget 면적을 구할 위젯(nullptr 허용).
	 * @return 위젯의 로컬 면적(픽셀^2). 위젯이 없거나 크기가 0 이하면 0.
	 */
	float GetWidgetArea(const UWidget* Widget)
	{
		if (Widget == nullptr)
		{
			return 0.0f;
		}

		const FVector2D Size = Widget->GetCachedGeometry().GetLocalSize();
		return Size.X > 0.0f && Size.Y > 0.0f ? Size.X * Size.Y : 0.0f;
	}

	/**
	 * @brief 배경 영상을 맞출 "화면 크기"를 여러 소스에서 모아 가장 큰 유효 크기로 결정한다.
	 * @details
	 *   단일 소스만 믿으면 타이밍/플랫폼에 따라 0이거나 세이프존만큼 작은 값이 나올 수 있어,
	 *   게임 뷰포트 → 활성 최상위 창 클라이언트 → 위젯 자신/루트 위젯 지오메트리 순으로 후보를 모으고
	 *   PickLargestPositiveSize로 면적이 가장 큰 값을 택한다(레터박스/세이프 마진을 영상 밖으로 밀어내기 위함).
	 * @param Owner 크기 기준이 되는 타이틀 메뉴 위젯(nullptr 허용).
	 * @return 채택된 화면 크기. 어떤 소스도 유효하지 않으면 ZeroVector.
	 */
	FVector2D ResolveTitleMenuViewportSize(const UTitleMenuWidget* Owner)
	{
		if (Owner == nullptr)
		{
			return FVector2D::ZeroVector;
		}

		FVector2D BestSize = FVector2D::ZeroVector;

		// 1순위 후보: 실제 게임 렌더 뷰포트 크기.
		const UWorld* World = Owner->GetWorld();
		const UGameViewportClient* GameViewport = World != nullptr ? World->GetGameViewport() : nullptr;
		FVector2D ViewportSize = FVector2D::ZeroVector;
		if (GameViewport != nullptr)
		{
			GameViewport->GetViewportSize(OUT ViewportSize);
			PickLargestPositiveSize(ViewportSize, BestSize);
		}

		// 2순위 후보: OS 창(에디터 PIE/스탠드얼론)의 클라이언트 영역 크기.
		if (FSlateApplication::IsInitialized())
		{
			const TSharedPtr<SWindow> ActiveWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
			if (ActiveWindow.IsValid())
			{
				PickLargestPositiveSize(ActiveWindow->GetClientSizeInScreen(), BestSize);
			}
		}

		// 3순위 후보: 위젯 자신의 캐시된 지오메트리(레이아웃이 한 번이라도 돈 뒤라면 유효).
		const FVector2D CachedWidgetSize = Owner->GetCachedGeometry().GetLocalSize();
		PickLargestPositiveSize(CachedWidgetSize, BestSize);

		// 4순위 후보: 루트 위젯 지오메트리(세이프존 하위가 아닌 진짜 루트일 가능성이 높음).
		if (const UWidget* RootWidget = Owner->GetRootWidget())
		{
			const FVector2D RootSize = RootWidget->GetCachedGeometry().GetLocalSize();
			PickLargestPositiveSize(RootSize, BestSize);
		}

		return BestSize;
	}
}

/** @brief WBP가 가진 배경 Image에 런타임 MediaTexture를 연결해 타이틀 루프 영상을 시작한다. */
void UTitleMenuWidget::StartTitleBackgroundVideo()
{
	/*
	 * 표시 Image는 WBP가 제공한다.
	 * C++이 Image를 새로 만들지 않는 이유는 타이틀의 정적 배치/해상도 대응을
	 * WBP 자산 한곳에서 검토하게 하기 위해서다. 없으면 영상만 생략한다.
	 */
	if (TitleBackgroundImage == nullptr)
	{
		return;
	}

	if (TryUseSharedTitleBackgroundVideo())
	{
		FitTitleBackgroundVideoToViewport();
		return;
	}

	EnsureTitleBackgroundMediaObjects();
	FitTitleBackgroundVideoToViewport();
	PlayTitleBackgroundVideo();
}

/** @brief 타이틀 위젯이 소유하는 Transient 미디어 객체를 지연 생성하고 델리게이트를 연결한다. */
void UTitleMenuWidget::EnsureTitleBackgroundMediaObjects()
{
	if (mBackgroundRuntime.mUsesSharedMedia)
	{
		return;
	}

	/*
	 * Media 객체들은 WBP에 저장하지 않고 런타임 Transient로만 둔다.
	 * 배경 영상 파일 경로/재생 상태는 실행 환경에 묶이고, WBP는 화면 배치만 검토 가능한 상태로 남아야 한다.
	 */
	if (mBackgroundRuntime.mMediaPlayer == nullptr)
	{
		mBackgroundRuntime.mMediaPlayer = NewObject<UMediaPlayer>(this, TEXT("TitleBackgroundMediaPlayer"));
#if PLATFORM_WINDOWS || PLATFORM_MAC
		mBackgroundRuntime.mMediaPlayer->SetDesiredPlayerName(FName(TEXT("ElectraProtron")));
#elif PLATFORM_ANDROID
		// Electra가 자동 선택되면 본 프로젝트의 로컬 H.264 mp4를 "No playable streams"로 거부한다(5.7 실측).
		// OS MediaPlayer 경로(AndroidMedia)가 로컬 파일 재생에 검증돼 있어 명시 지정한다.
		mBackgroundRuntime.mMediaPlayer->SetDesiredPlayerName(FName(TEXT("AndroidMedia")));
#endif
	}

	if (mBackgroundRuntime.mMediaPlayer != nullptr)
	{
		mBackgroundRuntime.mMediaPlayer->PlayOnOpen = true;
		mBackgroundRuntime.mMediaPlayer->SetLooping(true);
		mBackgroundRuntime.mMediaPlayer->OnMediaOpened.AddUniqueDynamic(this, &UTitleMenuWidget::HandleTitleBackgroundMediaOpened);
		mBackgroundRuntime.mMediaPlayer->OnMediaOpenFailed.AddUniqueDynamic(this, &UTitleMenuWidget::HandleTitleBackgroundMediaOpenFailed);
	}

	if (mBackgroundRuntime.mMediaTexture == nullptr)
	{
		mBackgroundRuntime.mMediaTexture = NewObject<UMediaTexture>(this, TEXT("TitleBackgroundMediaTexture"));
		mBackgroundRuntime.mMediaTexture->AutoClear = true;
		mBackgroundRuntime.mMediaTexture->ClearColor = FLinearColor::Black;
	}

	if (mBackgroundRuntime.mMediaTexture != nullptr)
	{
		mBackgroundRuntime.mMediaTexture->SetMediaPlayer(mBackgroundRuntime.mMediaPlayer);
		mBackgroundRuntime.mMediaTexture->UpdateResource();
	}

	if (mBackgroundRuntime.mMediaSource == nullptr)
	{
		mBackgroundRuntime.mMediaSource = NewObject<UFileMediaSource>(this, TEXT("TitleBackgroundMediaSource"));
		mBackgroundRuntime.mMediaSource->PrecacheFile = true;
	}
}

/**
 * @brief 게임 인스턴스 서브시스템이 미리 만들어 둔 공유 배경 영상을 재사용 시도한다.
 * @details
 *   UTitleBackgroundVideoSubsystem이 살아 있으면 영상을 프리로드시키고, 그 Player/Texture를 받아
 *   이 위젯의 런타임 핸들로 채택한다(매번 새 MediaPlayer를 만들지 않아 화면 전환 시 재로딩/깜빡임을 줄인다).
 *   채택에 성공하면 mUsesSharedMedia=true가 되어 이후 정리/재생 로직이 "공유 소유"임을 알고 닫지 않는다.
 * @return 공유 미디어를 채택했으면 true(호출자는 자체 미디어 생성을 건너뛴다). 서브시스템/리소스가 없으면 false.
 */
bool UTitleMenuWidget::TryUseSharedTitleBackgroundVideo()
{
	UGameInstance* GameInstance = GetGameInstance();
	UTitleBackgroundVideoSubsystem* VideoSubsystem = GameInstance != nullptr
		? GameInstance->GetSubsystem<UTitleBackgroundVideoSubsystem>()
		: nullptr;
	if (VideoSubsystem == nullptr)
	{
		return false;
	}

	const FString RequestedPath = GetTitleBackgroundVideoPath();
	VideoSubsystem->PreloadTitleBackgroundVideo(RequestedPath);

	UMediaPlayer* SharedMediaPlayer = VideoSubsystem->GetMediaPlayer();
	UMediaTexture* SharedMediaTexture = VideoSubsystem->GetMediaTexture();
	if (SharedMediaPlayer == nullptr || SharedMediaTexture == nullptr)
	{
		// 공유 리소스가 아직(또는 영영) 준비되지 않음 → 호출자가 위젯 소유 미디어를 직접 만들도록 false 반환.
		return false;
	}

	// 아직 공유 Player로 바꾸지 않은 경우에만 핸들을 교체한다(이미 같은 Player면 재바인딩 불필요).
	if (mBackgroundRuntime.mMediaPlayer != SharedMediaPlayer)
	{
		// 이전에 위젯 소유 Player를 쓰고 있었다면, 그쪽 델리게이트를 먼저 떼서 콜백이 중복되지 않게 한다.
		if (mBackgroundRuntime.mMediaPlayer != nullptr)
		{
			mBackgroundRuntime.mMediaPlayer->OnMediaOpened.RemoveAll(this);
			mBackgroundRuntime.mMediaPlayer->OnMediaOpenFailed.RemoveAll(this);
		}

		mBackgroundRuntime.mMediaPlayer = SharedMediaPlayer;
		mBackgroundRuntime.mMediaTexture = SharedMediaTexture;
		mBackgroundRuntime.mMediaSource = nullptr;       // 공유 모드에선 MediaSource를 위젯이 들고 있지 않는다(서브시스템 소유).
		mBackgroundRuntime.mUsesSharedMedia = true;      // 이후 Stop/Play 로직이 "남의 리소스"임을 알게 하는 플래그.
		mBackgroundRuntime.mMediaPlayer->OnMediaOpened.AddUniqueDynamic(this, &UTitleMenuWidget::HandleTitleBackgroundMediaOpened);
		mBackgroundRuntime.mMediaPlayer->OnMediaOpenFailed.AddUniqueDynamic(this, &UTitleMenuWidget::HandleTitleBackgroundMediaOpenFailed);
	}

	// 서브시스템이 이미 알고 있는 원본 해상도가 있으면 파일 파싱 없이 즉시 사용한다.
	const FVector2D SharedVideoSize = VideoSubsystem->GetVideoNativeSize();
	if (SharedVideoSize.X > 0.0f && SharedVideoSize.Y > 0.0f)
	{
		mBackgroundRuntime.mVideoNativeSize = SharedVideoSize;
	}

	// 이미 열려 재생 중이면 OnMediaOpened 콜백이 다시 오지 않으므로, 여기서 곧바로 브러시를 반영한다.
	if (VideoSubsystem->IsMediaOpened())
	{
		ApplyTitleBackgroundVideoBrush();
	}

	return true;
}

/** @brief MediaTexture를 Slate Brush에 넣어 WBP 배경 Image에 반영한다. */
void UTitleMenuWidget::ApplyTitleBackgroundVideoBrush()
{
	if (TitleBackgroundImage == nullptr || mBackgroundRuntime.mMediaTexture == nullptr)
	{
		return;
	}

	// WBP는 Image의 위치와 크기만 소유하고, 런타임 영상 리소스는 이 브러시로 주입한다.
	mBackgroundRuntime.mVideoBrush = FSlateBrush();
	mBackgroundRuntime.mVideoBrush.DrawAs = ESlateBrushDrawType::Image;
	mBackgroundRuntime.mVideoBrush.ImageSize = mBackgroundRuntime.mVideoNativeSize.X > 0.0f && mBackgroundRuntime.mVideoNativeSize.Y > 0.0f
		? mBackgroundRuntime.mVideoNativeSize
		: FVector2D(1280.0f, 1280.0f);
	mBackgroundRuntime.mVideoBrush.SetResourceObject(mBackgroundRuntime.mMediaTexture);
	TitleBackgroundImage->SetBrush(mBackgroundRuntime.mVideoBrush);

	// concept_title_01_classic의 TitleVignetteImage는 투명 오버레이가 아니라 불투명 정적 배경이다.
	// 영상 브러시가 실제로 적용된 뒤에는 숨겨야 재생 중인 MediaTexture가 화면에 보인다.
	if (UWidget* TitleVignetteWidget = GetWidgetFromName(TEXT("TitleVignetteImage")))
	{
		TitleVignetteWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

/** @brief 파일 존재와 MediaSource 열기까지 책임지고, 실패해도 타이틀 UI 자체는 유지한다. */
void UTitleMenuWidget::PlayTitleBackgroundVideo()
{
	EnsureTitleBackgroundMediaObjects();

	if (mBackgroundRuntime.mUsesSharedMedia)
	{
		if (mBackgroundRuntime.mMediaPlayer != nullptr && mBackgroundRuntime.mMediaPlayer->IsPlaying() == false)
		{
			mBackgroundRuntime.mMediaPlayer->Play();
		}
		return;
	}

	if (mBackgroundRuntime.mMediaPlayer == nullptr || mBackgroundRuntime.mMediaSource == nullptr)
	{
		return;
	}

	const FString VideoPath = ResolveTitleBackgroundVideoPath();
	if (FPaths::FileExists(VideoPath) == false)
	{
		// WBP 배경 Image 자체는 남기고 영상 재생만 중단한다. 패키징 누락을 로그에서 찾기 위한 경고다.
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: title background video file missing: %s"), *VideoPath);
		return;
	}

	if (mBackgroundRuntime.mVideoNativeSize.X <= 0.0f || mBackgroundRuntime.mVideoNativeSize.Y <= 0.0f)
	{
		const FVector2D FileVideoSize = ReadTitleBackgroundVideoFileDimensions(VideoPath);
		mBackgroundRuntime.mVideoNativeSize = FileVideoSize.X > 0.0f && FileVideoSize.Y > 0.0f
			? FileVideoSize
			: FVector2D(1280.0f, 1280.0f);
	}
	FitTitleBackgroundVideoToViewport();

	mBackgroundRuntime.mMediaPlayer->PlayOnOpen = true;
	mBackgroundRuntime.mMediaPlayer->SetLooping(true);
	mBackgroundRuntime.mMediaPlayer->Close();
	mBackgroundRuntime.mMediaSource->SetFilePath(VideoPath);

	if (mBackgroundRuntime.mMediaPlayer->OpenSource(mBackgroundRuntime.mMediaSource) == false)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: failed to open title background video: %s"), *VideoPath);
	}
	else
	{
		mBackgroundRuntime.mMediaPlayer->Play();
	}
}

/** @brief WBP Image 슬롯을 영상 원본 비율로 키워 화면 전체를 덮고, 남는 방향을 중앙 기준으로 잘라낸다. */
void UTitleMenuWidget::FitTitleBackgroundVideoToViewport() const
{
	if (TitleBackgroundImage == nullptr)
	{
		return;
	}

	FVector2D VideoSize = mBackgroundRuntime.mVideoNativeSize;
	if (VideoSize.X <= 0.0f || VideoSize.Y <= 0.0f)
	{
		VideoSize = mBackgroundRuntime.mVideoBrush.ImageSize;
	}
	if (VideoSize.X <= 0.0f || VideoSize.Y <= 0.0f)
	{
		VideoSize = FVector2D(1280.0f, 1280.0f);
	}

	const FVector2D ViewportSize = ResolveTitleMenuViewportSize(this);
	if (ViewportSize.X <= 0.0f || ViewportSize.Y <= 0.0f)
	{
		return;
	}

	// WBP_TitleMenu has a SafeZone/SafeCanvas for interactive controls, but the video
	// background must use the real root canvas so tablet safe margins do not become
	// visible side bars.
	// 배경 영상이 붙을 캔버스를 부모 체인에서 "면적이 가장 큰 UCanvasPanel"로 고른다.
	// 부모 체인을 위로 올라가며 가장 넓은 캔버스(=세이프존 바깥의 진짜 루트)를 찾아, 영상을 그 위에 깐다.
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(GetRootWidget());
	float RootCanvasArea = GetWidgetArea(RootCanvas);
	for (UWidget* Parent = TitleBackgroundImage->GetParent(); Parent != nullptr; Parent = Parent->GetParent())
	{
		if (UCanvasPanel* CanvasParent = Cast<UCanvasPanel>(Parent))
		{
			const float CanvasArea = GetWidgetArea(CanvasParent);
			if (RootCanvas == nullptr || CanvasArea >= RootCanvasArea)
			{
				RootCanvas = CanvasParent;
				RootCanvasArea = CanvasArea;
			}
		}
	}
	// 현재 부모가 그 루트 캔버스가 아니면, Image를 떼어 루트 캔버스로 재부착하고 맨 뒤(Z=-100)로 보낸다.
	if (RootCanvas != nullptr && TitleBackgroundImage->GetParent() != RootCanvas)
	{
		if (UPanelWidget* PreviousParent = TitleBackgroundImage->GetParent())
		{
			PreviousParent->RemoveChild(TitleBackgroundImage);
		}
		if (UCanvasPanelSlot* NewCanvasSlot = RootCanvas->AddChildToCanvas(TitleBackgroundImage))
		{
			NewCanvasSlot->SetZOrder(-100);   // 모든 메뉴 UI보다 뒤에 깔리도록 음수 Z-Order.
		}
	}

	// "cover" 핏 계산: 영상과 뷰포트의 가로세로비를 비교해, 짧은 쪽을 화면에 꽉 채우고 긴 쪽은 넘치게 둔다.
	const float VideoAspect = VideoSize.X / VideoSize.Y;
	const float ViewportAspect = ViewportSize.X / ViewportSize.Y;
	FVector2D TargetSize;
	if (ViewportAspect < VideoAspect)
	{
		// 화면이 영상보다 세로로 길쭉함 → 세로를 꽉 채우고, 가로는 비율대로 넘치게(좌우가 잘림).
		TargetSize.Y = ViewportSize.Y;
		TargetSize.X = ViewportSize.Y * VideoAspect;
	}
	else
	{
		// 화면이 영상보다 가로로 넓음 → 가로를 꽉 채우고, 세로는 비율대로 넘치게(상하가 잘림 = 상하크롭).
		TargetSize.X = ViewportSize.X;
		TargetSize.Y = ViewportSize.X / VideoAspect;
	}

	TitleBackgroundImage->SetDesiredSizeOverride(TargetSize);
	TitleBackgroundImage->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	TitleBackgroundImage->SetRenderTransform(FWidgetTransform());

	// 정상 경로: 캔버스 슬롯이면 중앙 앵커/정렬로 고정하고 계산된 cover 크기를 그대로 슬롯 크기로 준다(넘치는 부분이 화면 밖으로 잘림).
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(TitleBackgroundImage->Slot))
	{
		CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));     // 화면 중앙 기준 앵커.
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));  // 피벗도 중앙 → 좌우/상하 크롭이 대칭으로 일어남.
		CanvasSlot->SetPosition(FVector2D::ZeroVector);
		CanvasSlot->SetAutoSize(false);
		CanvasSlot->SetSize(TargetSize);
		CanvasSlot->SetZOrder(-100);
		return;
	}

	// 폴백 경로: 슬롯이 캔버스가 아니라 크기를 직접 못 줄 때는, 현재 표시 크기 대비 필요한 배율을 구해 RenderTransform 스케일로 덮는다.
	const FVector2D CurrentImageSize = TitleBackgroundImage->GetCachedGeometry().GetLocalSize();
	if (CurrentImageSize.X > 0.0f && CurrentImageSize.Y > 0.0f)
	{
		// 가로/세로 중 더 큰 배율을 택해야 양쪽 모두 화면을 덮는다(cover 보장).
		const float RenderScale = FMath::Max(TargetSize.X / CurrentImageSize.X, TargetSize.Y / CurrentImageSize.Y);
		if (RenderScale > 0.0f)
		{
			TitleBackgroundImage->SetRenderTransform(FWidgetTransform(FVector2D::ZeroVector, FVector2D(RenderScale, RenderScale), FVector2D::ZeroVector, 0.0f));
		}
	}
}

/** @brief 위젯 Destruct 시 MediaPlayer 델리게이트와 파일 핸들을 정리한다. */
void UTitleMenuWidget::StopTitleBackgroundVideo()
{
	if (mBackgroundRuntime.mMediaPlayer != nullptr)
	{
		mBackgroundRuntime.mMediaPlayer->OnMediaOpened.RemoveAll(this);
		mBackgroundRuntime.mMediaPlayer->OnMediaOpenFailed.RemoveAll(this);
		if (mBackgroundRuntime.mUsesSharedMedia == false)
		{
			mBackgroundRuntime.mMediaPlayer->Close();
		}
	}
}

/** @brief 게임 설정의 Content 상대 경로를 플랫폼별 실제 파일 경로로 바꾼다. */
FString UTitleMenuWidget::ResolveTitleBackgroundVideoPath() const
{
	const FString RequestedPath = GetTitleBackgroundVideoPath();
	return RDUITexture::ResolveContentFilePath(RequestedPath);
}

/** @details MP4(ISO BMFF) 컨테이너의 'tkhd'(Track Header) 박스 끝 8바이트가 표시 width/height(16.16 고정소수)다. */
FVector2D UTitleMenuWidget::ReadTitleBackgroundVideoFileDimensions(const FString& VideoPath) const
{
	TArray<uint8> Bytes;
	if (FFileHelper::LoadFileToArray(Bytes, *VideoPath) == false)
	{
		return FVector2D::ZeroVector;
	}

	const int32 NumBytes = Bytes.Num();
	// MP4/ISO BMFF의 모든 박스 크기/필드는 빅엔디안이므로, 4바이트를 빅엔디안 uint32로 읽는 헬퍼.
	auto ReadBigEndian32 = [&Bytes](int32 Offset) -> uint32
	{
		return (static_cast<uint32>(Bytes[Offset]) << 24)
			| (static_cast<uint32>(Bytes[Offset + 1]) << 16)
			| (static_cast<uint32>(Bytes[Offset + 2]) << 8)
			| static_cast<uint32>(Bytes[Offset + 3]);
	};

	// 파일 전체를 훑으며 'tkhd'(Track Header) 박스 타입 4바이트 시그니처를 찾는다(전체 파싱 대신 시그니처 스캔).
	for (int32 Index = 4; Index + 4 <= NumBytes; ++Index)
	{
		if (Bytes[Index] != 't' || Bytes[Index + 1] != 'k' || Bytes[Index + 2] != 'h' || Bytes[Index + 3] != 'd')
		{
			continue;
		}

		// 박스 레이아웃: [size(4)][type 'tkhd'(4)][...]. 타입 4바이트 앞의 4바이트가 박스 size이므로 BoxStart = Index-4.
		const int32 BoxStart = Index - 4;
		const uint32 BoxSize = ReadBigEndian32(BoxStart);
		const int64 BoxEnd = static_cast<int64>(BoxStart) + static_cast<int64>(BoxSize);
		if (BoxSize < 32 || BoxEnd > NumBytes)
		{
			continue;   // 비정상 크기이거나 파일 범위를 벗어나면 잘못된 매치로 보고 다음 후보로.
		}

		// tkhd 박스의 마지막 8바이트가 표시 width/height(둘 다 16.16 고정소수) → 65536으로 나눠 정수 픽셀로 환산.
		const float Width = static_cast<float>(ReadBigEndian32(static_cast<int32>(BoxEnd) - 8)) / 65536.0f;
		const float Height = static_cast<float>(ReadBigEndian32(static_cast<int32>(BoxEnd) - 4)) / 65536.0f;
		if (Width > 0.0f && Height > 0.0f)
		{
			return FVector2D(Width, Height);
		}
	}

	return FVector2D::ZeroVector;   // tkhd를 못 찾았거나 크기가 0 → 호출자가 기본 해상도로 폴백한다.
}

/** @brief MediaPlayer가 파일을 연 뒤 실제 재생을 시작하는 비동기 완료 지점이다. */
void UTitleMenuWidget::HandleTitleBackgroundMediaOpened(FString OpenedUrl)
{
	if (mBackgroundRuntime.mMediaPlayer != nullptr)
	{
		mBackgroundRuntime.mMediaPlayer->SetLooping(true);
		mBackgroundRuntime.mMediaPlayer->Play();

		const int32 VideoTrack = mBackgroundRuntime.mMediaPlayer->GetSelectedTrack(EMediaPlayerTrack::Video);
		const int32 VideoFormat = mBackgroundRuntime.mMediaPlayer->GetTrackFormat(EMediaPlayerTrack::Video, VideoTrack);
		const FIntPoint VideoDimensions = mBackgroundRuntime.mMediaPlayer->GetVideoTrackDimensions(VideoTrack, VideoFormat);
		if (VideoDimensions.X > 0 && VideoDimensions.Y > 0)
		{
			mBackgroundRuntime.mVideoNativeSize = FVector2D(VideoDimensions.X, VideoDimensions.Y);
		}
		ApplyTitleBackgroundVideoBrush();
		FitTitleBackgroundVideoToViewport();
	}

	UE_LOG(LogRD, Display, TEXT("TitleMenuWidget: title background media opened: %s"), *OpenedUrl);
}

/** @brief 미디어 파일 누락/코덱 실패를 화면 전환 실패로 승격하지 않고 로그에 남긴다. */
void UTitleMenuWidget::HandleTitleBackgroundMediaOpenFailed(FString FailedUrl)
{
	UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: title background media open failed: %s"), *FailedUrl);
}
