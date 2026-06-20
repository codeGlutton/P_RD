#include "UI/TitleMenuWidget.h"

#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "FileMediaSource.h"
#include "MediaPlayer.h"
#include "MediaTexture.h"
#include "Misc/FileHelper.h"
#include "UI/UITextureLoader.h"

namespace
{
	const TCHAR* const TitleBackgroundVideoPath = TEXT("SVN/OutSideAsset/AICreation/campfire_titleloop_idle_x3preview.mp4");
	const TCHAR* const LegacyTitleBackgroundVideoPath = TEXT("SVN/OutSideAsset/AICreation/MS_TitleLoop_01.mp4");

	FVector2D ResolveTitleMenuViewportSize(const UTitleMenuWidget* Owner)
	{
		if (Owner == nullptr)
		{
			return FVector2D::ZeroVector;
		}

		const FVector2D CachedWidgetSize = Owner->GetCachedGeometry().GetLocalSize();
		if (CachedWidgetSize.X > 0.0f && CachedWidgetSize.Y > 0.0f)
		{
			return CachedWidgetSize;
		}

		const UWorld* World = Owner->GetWorld();
		const UGameViewportClient* GameViewport = World != nullptr ? World->GetGameViewport() : nullptr;
		FVector2D ViewportSize = FVector2D::ZeroVector;
		if (GameViewport != nullptr)
		{
			GameViewport->GetViewportSize(OUT ViewportSize);
		}
		return ViewportSize;
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

	EnsureTitleBackgroundMediaObjects();
	ApplyTitleBackgroundVideoBrush();
	FitTitleBackgroundVideoToViewport();
	PlayTitleBackgroundVideo();
}

/** @brief 타이틀 위젯이 소유하는 Transient 미디어 객체를 지연 생성하고 델리게이트를 연결한다. */
void UTitleMenuWidget::EnsureTitleBackgroundMediaObjects()
{
	/*
	 * Media 객체들은 WBP에 저장하지 않고 런타임 Transient로만 둔다.
	 * 배경 영상 파일 경로/재생 상태는 실행 환경에 묶이고, WBP는 화면 배치만 검토 가능한 상태로 남아야 한다.
	 */
	if (mBackgroundRuntime.mMediaPlayer == nullptr)
	{
		mBackgroundRuntime.mMediaPlayer = NewObject<UMediaPlayer>(this, TEXT("TitleBackgroundMediaPlayer"));
	}

	if (mBackgroundRuntime.mMediaPlayer != nullptr)
	{
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
		: FVector2D(1280.0f, 720.0f);
	mBackgroundRuntime.mVideoBrush.SetResourceObject(mBackgroundRuntime.mMediaTexture);
	TitleBackgroundImage->SetBrush(mBackgroundRuntime.mVideoBrush);
}

/** @brief 파일 존재와 MediaSource 열기까지 책임지고, 실패해도 타이틀 UI 자체는 유지한다. */
void UTitleMenuWidget::PlayTitleBackgroundVideo()
{
	EnsureTitleBackgroundMediaObjects();

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

	mBackgroundRuntime.mVideoNativeSize = ReadTitleBackgroundVideoFileDimensions(VideoPath);
	ApplyTitleBackgroundVideoBrush();
	FitTitleBackgroundVideoToViewport();

	mBackgroundRuntime.mMediaPlayer->SetLooping(true);
	mBackgroundRuntime.mMediaPlayer->Close();
	mBackgroundRuntime.mMediaSource->SetFilePath(VideoPath);

	if (mBackgroundRuntime.mMediaPlayer->OpenSource(mBackgroundRuntime.mMediaSource) == false)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: failed to open title background video: %s"), *VideoPath);
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
		VideoSize = FVector2D(1280.0f, 720.0f);
	}

	const FVector2D ViewportSize = ResolveTitleMenuViewportSize(this);
	if (ViewportSize.X <= 0.0f || ViewportSize.Y <= 0.0f)
	{
		return;
	}

	const float VideoAspect = VideoSize.X / VideoSize.Y;
	const float ViewportAspect = ViewportSize.X / ViewportSize.Y;
	FVector2D TargetSize;
	if (ViewportAspect < VideoAspect)
	{
		TargetSize.Y = ViewportSize.Y;
		TargetSize.X = ViewportSize.Y * VideoAspect;
	}
	else
	{
		TargetSize.X = ViewportSize.X;
		TargetSize.Y = ViewportSize.X / VideoAspect;
	}

	TitleBackgroundImage->SetDesiredSizeOverride(TargetSize);
	TitleBackgroundImage->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	TitleBackgroundImage->SetRenderTransform(FWidgetTransform());

	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(TitleBackgroundImage->Slot))
	{
		CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetPosition(FVector2D::ZeroVector);
		CanvasSlot->SetAutoSize(false);
		CanvasSlot->SetSize(TargetSize);
		return;
	}

	const FVector2D CurrentImageSize = TitleBackgroundImage->GetCachedGeometry().GetLocalSize();
	if (CurrentImageSize.X > 0.0f && CurrentImageSize.Y > 0.0f)
	{
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
		mBackgroundRuntime.mMediaPlayer->Close();
	}
}

/** @brief Content 상대 경로를 플랫폼별 실제 파일 경로로 바꾼다. */
FString UTitleMenuWidget::ResolveTitleBackgroundVideoPath() const
{
	const FString RequestedPath = mTitleBackgroundVideoPath.IsEmpty() || mTitleBackgroundVideoPath == LegacyTitleBackgroundVideoPath
		? FString(TitleBackgroundVideoPath)
		: mTitleBackgroundVideoPath;
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
	auto ReadBigEndian32 = [&Bytes](int32 Offset) -> uint32
	{
		return (static_cast<uint32>(Bytes[Offset]) << 24)
			| (static_cast<uint32>(Bytes[Offset + 1]) << 16)
			| (static_cast<uint32>(Bytes[Offset + 2]) << 8)
			| static_cast<uint32>(Bytes[Offset + 3]);
	};

	for (int32 Index = 4; Index + 4 <= NumBytes; ++Index)
	{
		if (Bytes[Index] != 't' || Bytes[Index + 1] != 'k' || Bytes[Index + 2] != 'h' || Bytes[Index + 3] != 'd')
		{
			continue;
		}

		const int32 BoxStart = Index - 4;
		const uint32 BoxSize = ReadBigEndian32(BoxStart);
		const int64 BoxEnd = static_cast<int64>(BoxStart) + static_cast<int64>(BoxSize);
		if (BoxSize < 32 || BoxEnd > NumBytes)
		{
			continue;
		}

		const float Width = static_cast<float>(ReadBigEndian32(static_cast<int32>(BoxEnd) - 8)) / 65536.0f;
		const float Height = static_cast<float>(ReadBigEndian32(static_cast<int32>(BoxEnd) - 4)) / 65536.0f;
		if (Width > 0.0f && Height > 0.0f)
		{
			return FVector2D(Width, Height);
		}
	}

	return FVector2D::ZeroVector;
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
			ApplyTitleBackgroundVideoBrush();
			FitTitleBackgroundVideoToViewport();
		}
	}

	UE_LOG(LogRD, Display, TEXT("TitleMenuWidget: title background media opened: %s"), *OpenedUrl);
}

/** @brief 미디어 파일 누락/코덱 실패를 화면 전환 실패로 승격하지 않고 로그에 남긴다. */
void UTitleMenuWidget::HandleTitleBackgroundMediaOpenFailed(FString FailedUrl)
{
	UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: title background media open failed: %s"), *FailedUrl);
}
