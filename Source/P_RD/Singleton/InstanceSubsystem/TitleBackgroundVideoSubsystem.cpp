/**
 * @file   TitleBackgroundVideoSubsystem.cpp
 * @brief  타이틀 배경 영상(mp4) 프리로드/재생 GameInstance 서브시스템 구현.
 * @author 박용수
 * @date   2026-06-26
 **/

#include "Singleton/InstanceSubsystem/TitleBackgroundVideoSubsystem.h"

#include "FileMediaSource.h"
#include "MediaPlayer.h"
#include "MediaTexture.h"
#include "Setting/GamePlaySettings.h"
#include "UI/UITextureLoader.h"

namespace
{
	const TCHAR* const FallbackTitleBackgroundVideoPath = TEXT("SVN/OutSideAsset/AICreation/campfire_titleloop_idle_x3preview.mp4");

	FString GetDefaultTitleBackgroundVideoPath()
	{
		const UGamePlaySettings* GamePlaySettings = GetDefault<UGamePlaySettings>();
		if (GamePlaySettings != nullptr && GamePlaySettings->mTitleBackgroundVideoPath.IsEmpty() == false)
		{
			return GamePlaySettings->mTitleBackgroundVideoPath;
		}

		return FString(FallbackTitleBackgroundVideoPath);
	}
}

/**
 * @brief  타이틀 배경 영상을 열어 루프 재생을 시작한다(위젯 표시 전 호출용).
 * @param  RelativeContentPath  Content 기준 상대 경로의 mp4. 빈 문자열이면 게임 설정의 기본 타이틀 배경 영상.
 * @details 같은 경로를 이미 열도록 요청했으면 즉시 반환(중복 열기 방지). 파일이 없으면 경고만 남기고 반환.
 *          미디어 열기는 비동기이므로 실제 첫 프레임 준비는 HandleMediaOpened 콜백에서 확정된다.
 */
void UTitleBackgroundVideoSubsystem::PreloadTitleBackgroundVideo(const FString& RelativeContentPath)
{
	// 빈 경로면 게임 설정의 기본 영상을 사용.
	const FString RequestedPath = RelativeContentPath.IsEmpty()
		? GetDefaultTitleBackgroundVideoPath()
		: RelativeContentPath;

	// 같은 영상을 이미 열도록 요청했으면 다시 열지 않는다(GameInstance라 화면 재진입 시 중복 호출될 수 있음).
	if (mOpenRequested && RequestedPath == mCurrentRelativePath)
	{
		return;
	}

	// MediaPlayer/MediaTexture/MediaSource 3종이 준비돼 있는지 보장(최초 1회 생성·연결).
	EnsureMediaObjects();
	if (mMediaPlayer == nullptr || mMediaSource == nullptr)
	{
		return;
	}

	// Content 상대 경로 → 실제 디스크 절대 경로로 해석(에디터/패키지 환경 모두 대응).
	const FString VideoPath = RDUITexture::ResolveContentFilePath(RequestedPath);
	if (FPaths::FileExists(VideoPath) == false)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleBackgroundVideoSubsystem: title background video missing: %s"), *VideoPath);
		return;
	}

	mCurrentRelativePath = RequestedPath;
	mOpenRequested = true;
	mMediaOpened = false;

	// PlayOnOpen: 열리는 즉시 자동 재생 / SetLooping: 끝나면 처음으로 반복.
	mMediaPlayer->PlayOnOpen = true;
	mMediaPlayer->SetLooping(true);
	// 이전 재생을 닫아 상태를 초기화하고, 소스에 새 파일 경로를 지정한 뒤 연다.
	mMediaPlayer->Close();
	mMediaSource->SetFilePath(VideoPath);

	// OpenSource는 "열기 시작" 비동기 요청 — 성공/실패는 OnMediaOpened/OnMediaOpenFailed 콜백으로 통지된다.
	if (mMediaPlayer->OpenSource(mMediaSource) == false)
	{
		// 즉시 거부된 경우(소스 무효 등) — 요청 플래그를 되돌린다.
		mOpenRequested = false;
		UE_LOG(LogRD, Warning, TEXT("TitleBackgroundVideoSubsystem: failed to open title background video: %s"), *VideoPath);
	}
	else
	{
		// 열기 시작 성공 — PlayOnOpen이 있어도 명시적으로 Play를 한 번 더 호출(안전).
		mMediaPlayer->Play();
	}
}

/**
 * @brief  GameInstance 종료 시 정리: 콜백 해제 + 미디어 Close로 디코더/스레드를 반환한다.
 */
void UTitleBackgroundVideoSubsystem::Deinitialize()
{
	StopTitleBackgroundVideo();
	Super::Deinitialize();
}

void UTitleBackgroundVideoSubsystem::StopTitleBackgroundVideo()
{
	if (mMediaPlayer != nullptr)
	{
		// 종료 후 콜백이 죽은 this를 건드리지 않도록 먼저 해제한다.
		mMediaPlayer->OnMediaOpened.RemoveAll(this);
		mMediaPlayer->OnMediaOpenFailed.RemoveAll(this);
		mMediaPlayer->Close();
	}
	if (mMediaTexture != nullptr)
	{
		mMediaTexture->SetMediaPlayer(nullptr);
		mMediaTexture->UpdateResource();
	}

	mCurrentRelativePath.Reset();
	mOpenRequested = false;
	mMediaOpened = false;
}

/**
 * @brief  MediaPlayer/MediaTexture/MediaSource 3종을 1회 생성하고 서로 연결한다.
 * @details 텍스처는 SetMediaPlayer로 플레이어를 바라보게 연결되고, 플레이어는 열기 성공/실패 콜백에 등록된다.
 *          이미 만들어진 객체가 있으면 재생성하지 않는다(중복 호출 안전).
 */
void UTitleBackgroundVideoSubsystem::EnsureMediaObjects()
{
	// 1) 재생 엔진(MediaPlayer).
	if (mMediaPlayer == nullptr)
	{
		mMediaPlayer = NewObject<UMediaPlayer>(this, TEXT("SharedTitleBackgroundMediaPlayer"));
#if PLATFORM_WINDOWS || PLATFORM_MAC
		mMediaPlayer->SetDesiredPlayerName(FName(TEXT("ElectraProtron")));
#elif PLATFORM_ANDROID
		// Electra가 자동 선택되면 본 프로젝트의 로컬 H.264 mp4를 "No playable streams"로 거부한다(5.7 실측).
		// OS MediaPlayer 경로(AndroidMedia)가 로컬 파일 재생에 검증돼 있어 명시 지정한다.
		mMediaPlayer->SetDesiredPlayerName(FName(TEXT("AndroidMedia")));
#endif
	}
	if (mMediaPlayer != nullptr)
	{
		mMediaPlayer->PlayOnOpen = true;
		mMediaPlayer->SetLooping(true);
		// AddUnique: 중복 등록 방지(EnsureMediaObjects가 여러 번 불려도 콜백은 1개로 유지).
		mMediaPlayer->OnMediaOpened.AddUniqueDynamic(this, &UTitleBackgroundVideoSubsystem::HandleMediaOpened);
		mMediaPlayer->OnMediaOpenFailed.AddUniqueDynamic(this, &UTitleBackgroundVideoSubsystem::HandleMediaOpenFailed);
	}

	// 2) 출력 텍스처(MediaTexture) — 위젯이 브러시로 그릴 대상.
	if (mMediaTexture == nullptr)
	{
		mMediaTexture = NewObject<UMediaTexture>(this, TEXT("SharedTitleBackgroundMediaTexture"));
		mMediaTexture->AutoClear = true;                 // 프레임이 없을 때 ClearColor로 비운다.
		mMediaTexture->ClearColor = FLinearColor::Black; // 첫 프레임 전/끊김 시 검은색.
	}
	if (mMediaTexture != nullptr)
	{
		// 텍스처가 이 플레이어의 프레임을 받도록 연결한 뒤 GPU 리소스를 갱신한다.
		mMediaTexture->SetMediaPlayer(mMediaPlayer);
		mMediaTexture->UpdateResource();
	}

	// 3) 파일 소스(FileMediaSource) — 실제 mp4 경로는 PreloadTitleBackgroundVideo에서 SetFilePath로 지정.
	if (mMediaSource == nullptr)
	{
		mMediaSource = NewObject<UFileMediaSource>(this, TEXT("SharedTitleBackgroundMediaSource"));
		mMediaSource->PrecacheFile = true; // 파일을 미리 캐시해 첫 재생 지연을 줄인다.
	}
}

/**
 * @brief  미디어가 성공적으로 열렸을 때(비동기) 호출. 루프 재생 시작 + 원본 해상도 추출.
 * @param  OpenedUrl  열린 미디어의 URL(로그용).
 * @details 선택된 비디오 트랙/포맷에서 실제 픽셀 크기를 읽어 mVideoNativeSize에 저장한다.
 *          위젯은 이 값으로 뷰포트 좌우맞춤+상하크롭 비율을 계산한다.
 */
void UTitleBackgroundVideoSubsystem::HandleMediaOpened(FString OpenedUrl)
{
	mMediaOpened = true;
	if (mMediaPlayer != nullptr)
	{
		mMediaPlayer->SetLooping(true);
		mMediaPlayer->Play();

		// 선택된 비디오 트랙 → 그 트랙의 포맷 → 포맷의 해상도 순으로 원본 크기를 조회한다.
		const int32 VideoTrack = mMediaPlayer->GetSelectedTrack(EMediaPlayerTrack::Video);
		const int32 VideoFormat = mMediaPlayer->GetTrackFormat(EMediaPlayerTrack::Video, VideoTrack);
		const FIntPoint VideoDimensions = mMediaPlayer->GetVideoTrackDimensions(VideoTrack, VideoFormat);
		if (VideoDimensions.X > 0 && VideoDimensions.Y > 0)
		{
			mVideoNativeSize = FVector2D(VideoDimensions.X, VideoDimensions.Y);
		}
	}

	UE_LOG(LogRD, Display, TEXT("TitleBackgroundVideoSubsystem: media opened: %s"), *OpenedUrl);
}

/**
 * @brief  미디어 열기에 실패했을 때(비동기) 호출. 요청 플래그를 되돌리고 경고를 남긴다.
 * @param  FailedUrl  실패한 미디어 URL(로그용).
 */
void UTitleBackgroundVideoSubsystem::HandleMediaOpenFailed(FString FailedUrl)
{
	mOpenRequested = false;
	mMediaOpened = false;
	UE_LOG(LogRD, Warning, TEXT("TitleBackgroundVideoSubsystem: media open failed: %s"), *FailedUrl);
}
