/**
 * @file CinematicWidget_Media.cpp
 * @brief 인트로 시네마틱 MP4 영상의 미디어 재생/뷰포트 핏 로직 (UCinematicWidget의 미디어 파트 분할 구현)
 * @author 박용수
 * @date 2026-06-26
 */
// 이 파일: 인트로 MP4 영상 재생 담당. MediaPlayer/MediaTexture/FileMediaSource를 만들어
//          영상을 Slate 이미지에 그리고, 영상 열림→재생 / 실패·종료→FinishCinematic 으로 처리한다.
//          파일은 SVN로 동기화되는 mp4를 디스크에서 직접 연다(없으면 경고만 + 폴백 타이머로 진행).
#include "UI/CinematicWidget.h"

#include "FileMediaSource.h"
#include "MediaPlayer.h"
#include "MediaTexture.h"
#include "Misc/FileHelper.h"
#include "Setting/GamePlaySettings.h"
#include "UI/UITextureLoader.h"

namespace
{
	const TCHAR* const FallbackIntroCinematicVideoPath = TEXT("SVN/OutSideAsset/AICreation/hero_loading_intro4_1280_3s.mp4");

	FString GetIntroCinematicVideoPath()
	{
		const UGamePlaySettings* GamePlaySettings = GetDefault<UGamePlaySettings>();
		if (GamePlaySettings != nullptr && GamePlaySettings->mIntroCinematicVideoPath.IsEmpty() == false)
		{
			return GamePlaySettings->mIntroCinematicVideoPath;
		}

		return FString(FallbackIntroCinematicVideoPath);
	}
}

/**
 * @brief 시네마틱 재생에 필요한 미디어 오브젝트(플레이어/텍스처/소스)와 브러시를 지연 생성한다.
 * @details 각 오브젝트는 nullptr일 때만 1회 생성되므로 반복 호출해도 안전(idempotent)하다.
 *          마지막에 미디어 텍스처를 시네마틱 브러시의 리소스로 연결해 Slate 이미지가 영상을 그리게 한다.
 */
void UCinematicWidget::EnsureCinematicMediaObjects()
{
	if (mCinematicMediaPlayer == nullptr)
	{
		mCinematicMediaPlayer = NewObject<UMediaPlayer>(this, TEXT("IntroCinematicMediaPlayer"));
#if PLATFORM_WINDOWS || PLATFORM_MAC
		mCinematicMediaPlayer->SetDesiredPlayerName(FName(TEXT("ElectraProtron")));
#elif PLATFORM_ANDROID
		// Electra가 자동 선택되면 본 프로젝트의 로컬 H.264 mp4를 "No playable streams"로 거부한다(5.7 실측).
		// OS MediaPlayer 경로(AndroidMedia)가 로컬 파일 재생에 검증돼 있어 명시 지정한다.
		mCinematicMediaPlayer->SetDesiredPlayerName(FName(TEXT("AndroidMedia")));
#endif
		mCinematicMediaPlayer->SetLooping(false); // 인트로는 1회 재생 후 종료(루프 X)
		// 열림/실패/종료 이벤트를 바인딩해 재생 시작·폴백·다음 단계 전환을 제어한다(AddUnique로 중복 바인딩 방지).
		mCinematicMediaPlayer->OnMediaOpened.AddUniqueDynamic(this, &UCinematicWidget::HandleCinematicMediaOpened);
		mCinematicMediaPlayer->OnMediaOpenFailed.AddUniqueDynamic(this, &UCinematicWidget::HandleCinematicMediaOpenFailed);
		mCinematicMediaPlayer->OnEndReached.AddUniqueDynamic(this, &UCinematicWidget::HandleCinematicMediaEndReached);
	}

	if (mCinematicMediaTexture == nullptr)
	{
		mCinematicMediaTexture = NewObject<UMediaTexture>(this, TEXT("IntroCinematicMediaTexture"));
		mCinematicMediaTexture->AutoClear = true;                  // 재생 종료/전환 시 잔상 없이 자동 클리어
		mCinematicMediaTexture->ClearColor = FLinearColor::Black;  // 첫 프레임 도착 전·클리어 시 검은 화면으로 채움
		mCinematicMediaTexture->SetMediaPlayer(mCinematicMediaPlayer); // 텍스처가 이 플레이어의 출력 프레임을 받게 연결
		mCinematicMediaTexture->UpdateResource();                 // 렌더 리소스 즉시 갱신(연결 직후 유효 텍스처 보장)
	}

	if (mCinematicMediaSource == nullptr)
	{
		mCinematicMediaSource = NewObject<UFileMediaSource>(this, TEXT("IntroCinematicMediaSource"));
		mCinematicMediaSource->PrecacheFile = true; // 디스크 파일을 미리 캐시해 재생 시작 끊김(stutter)을 줄임
	}

	mCinematicVideoBrush.SetResourceObject(mCinematicMediaTexture); // 브러시의 그리기 리소스를 미디어 텍스처로 지정
}

/**
 * @brief 인트로 시네마틱 MP4의 비동기 재생을 시작한다.
 * @details 미디어 오브젝트 보장 → 경로 해석 → 파일 존재 확인 → 영상 실제 해상도 선확정 → 소스 open 순으로 진행한다.
 *          실제 재생/실패 처리는 OpenSource 이후 OnMediaOpened/OnMediaOpenFailed 콜백에서 비동기로 이뤄진다.
 * @return open 요청을 정상 발행했으면 true. 미디어 오브젝트 미생성·파일 없음 등으로 시작 불가 시 false.
 */
bool UCinematicWidget::PlayCinematicVideo()
{
	EnsureCinematicMediaObjects();

	if (mCinematicMediaPlayer == nullptr || mCinematicMediaSource == nullptr)
	{
		return false; // 미디어 오브젝트 생성 실패 시 재생 불가
	}

	const FString VideoPath = ResolveCinematicVideoPath(); // 상대 경로면 Content 기준 절대 경로로 변환
	if (FPaths::FileExists(VideoPath) == false)
	{
		UE_LOG(LogRD, Warning, TEXT("Intro cinematic file missing: %s"), *VideoPath);
		return false; // mp4가 디스크에 없으면 호출측이 폴백 타이머로 진행하도록 false 반환
	}

	// cover 비율 기준이 되는 영상 실제 해상도를 파일에서 미리 확정한다(미디어 텍스처/플레이어 해상도는 첫 프레임 전까지 0이라 폴백 시 정사각이 16:9로 눌림).
	mCinematicVideoNativeSize = ReadCinematicVideoFileDimensions(VideoPath);
	if (mCinematicVideoNativeSize.X > 0.0f && mCinematicVideoNativeSize.Y > 0.0f)
	{
		mCinematicVideoBrush.ImageSize = mCinematicVideoNativeSize; // 브러시 이미지 크기를 원본 해상도로 맞춰 비율 보존
	}

	mCinematicMediaPlayer->Rewind();                    // 이전 재생 위치를 처음으로 되돌림
	mCinematicMediaPlayer->Close();                     // 기존 열린 소스를 닫아 깨끗한 상태에서 다시 open
	mCinematicMediaSource->SetFilePath(VideoPath);      // 열 대상 파일 경로 지정
	return mCinematicMediaPlayer->OpenSource(mCinematicMediaSource); // 비동기 open 시작(성공 발행 여부 반환)
}

/**
 * @brief MP4 파일을 직접 파싱해 비디오 트랙의 표시 해상도(width/height)를 읽어온다.
 * @param VideoPath 읽을 mp4 파일의 절대 경로
 * @return 비디오 트랙의 표시 해상도(픽셀). 파일 로드 실패 또는 tkhd 미발견 시 ZeroVector.
 * @details MP4(ISO BMFF) 컨테이너의 'tkhd'(Track Header) 박스 끝 8바이트가 표시 width/height(16.16 고정소수)다.
 *  오디오 트랙 tkhd는 0이므로 첫 번째 양수 해상도(=비디오 트랙)를 반환한다. 파일을 직접 읽어 재생 타이밍과 무관하게 비율을 확정한다.
 */
FVector2D UCinematicWidget::ReadCinematicVideoFileDimensions(const FString& VideoPath) const
{
	TArray<uint8> Bytes;
	if (FFileHelper::LoadFileToArray(Bytes, *VideoPath) == false)
	{
		return FVector2D::ZeroVector; // 파일 로드 실패 → 해상도 미상(폴백 경로에서 처리)
	}

	const int32 NumBytes = Bytes.Num();
	// MP4 박스 헤더의 길이/해상도 필드는 모두 빅엔디안 32비트라 4바이트를 묶어 uint32로 읽는 헬퍼.
	auto ReadBigEndian32 = [&Bytes](int32 Offset) -> uint32
	{
		return (static_cast<uint32>(Bytes[Offset]) << 24)
			| (static_cast<uint32>(Bytes[Offset + 1]) << 16)
			| (static_cast<uint32>(Bytes[Offset + 2]) << 8)
			| static_cast<uint32>(Bytes[Offset + 3]);
	};

	// 바이트 스트림 전체를 훑어 'tkhd' 4바이트 시그니처를 탐색(인덱스 4부터: 앞 4바이트는 박스 크기 자리).
	for (int32 Index = 4; Index + 4 <= NumBytes; ++Index)
	{
		if (Bytes[Index] != 't' || Bytes[Index + 1] != 'k' || Bytes[Index + 2] != 'h' || Bytes[Index + 3] != 'd')
		{
			continue; // 'tkhd' 가 아니면 다음 바이트로
		}

		// 박스 크기(4바이트)는 타입('tkhd') 바로 앞에 온다. 박스 끝 = (타입앞) + 크기.
		const int32 BoxStart = Index - 4;
		const uint32 BoxSize = ReadBigEndian32(BoxStart);
		const int64 BoxEnd = static_cast<int64>(BoxStart) + static_cast<int64>(BoxSize);
		if (BoxSize < 32 || BoxEnd > NumBytes)
		{
			continue; // 박스 길이가 비정상이거나 버퍼를 넘어가면 잘못된 매치로 보고 스킵
		}

		// 표시 해상도는 박스 끝 직전 8바이트(width 4 + height 4)에 16.16 고정소수로 저장 → 65536으로 나눠 실수화.
		const float Width = static_cast<float>(ReadBigEndian32(static_cast<int32>(BoxEnd) - 8)) / 65536.0f;
		const float Height = static_cast<float>(ReadBigEndian32(static_cast<int32>(BoxEnd) - 4)) / 65536.0f;
		if (Width > 0.0f && Height > 0.0f)
		{
			return FVector2D(Width, Height); // 첫 양수 해상도(=비디오 트랙) 발견 시 즉시 반환
		}
	}

	return FVector2D::ZeroVector; // tkhd 미발견(또는 모두 0) → 해상도 미상
}

/**
 * @brief 시네마틱 재생을 중단하고 미디어 이벤트 바인딩을 해제한다.
 * @details 위젯 파괴/조기 스킵 시 호출. 콜백을 모두 제거해 닫는 도중 이벤트가 재진입하지 않게 한 뒤 플레이어를 닫는다.
 */
void UCinematicWidget::StopCinematicMedia()
{
	if (mCinematicMediaPlayer != nullptr)
	{
		// Close()가 유발할 수 있는 종료 콜백이 this로 재진입하지 않도록 먼저 모든 바인딩을 해제한다.
		mCinematicMediaPlayer->OnMediaOpened.RemoveAll(this);
		mCinematicMediaPlayer->OnMediaOpenFailed.RemoveAll(this);
		mCinematicMediaPlayer->OnEndReached.RemoveAll(this);
		mCinematicMediaPlayer->Close(); // 재생 중지 및 미디어 핸들 해제
	}
}

/**
 * @brief 설정된 시네마틱 영상 경로를 재생에 쓸 절대 경로로 해석한다.
 * @return 절대 경로면 그대로, 상대 경로면 프로젝트 Content 디렉터리 기준의 절대 경로.
 */
FString UCinematicWidget::ResolveCinematicVideoPath() const
{
	const FString CinematicVideoPath = GetIntroCinematicVideoPath();
	return RDUITexture::ResolveContentFilePath(CinematicVideoPath);
}

/**
 * @brief 미디어 소스 open 성공 콜백. 실제 재생을 시작하고 종료 폴백 타이머를 건다.
 * @param OpenedUrl 열린 미디어의 URL(엔진 델리게이트 시그니처상 전달되나 본 핸들러에서는 사용하지 않음)
 * @details open 직후 트랙 정보를 통해 영상 실해상도를 한 번 더 확정(브러시 비율 보정)하고,
 *          영상 길이가 유효하면 (길이+1초)를, 아니면 기본 시간을 종료 타이머로 설정한다.
 */
void UCinematicWidget::HandleCinematicMediaOpened(FString OpenedUrl)
{
	if (mCinematicMediaPlayer != nullptr)
	{
		mCinematicMediaPlayer->Play(); // open 완료 시점에 비로소 재생 시작

		// 영상 실제 해상도를 한 번 확보해 cover 비율 기준으로 쓴다.
		// (미디어 텍스처 GetWidth/Height는 첫 프레임 디코드 전까지 0/부정확할 수 있어, 폴백 시 정사각(640x640) 영상이 16:9로 눌릴 수 있다.)
		const int32 VideoTrack = mCinematicMediaPlayer->GetSelectedTrack(EMediaPlayerTrack::Video);           // 선택된 비디오 트랙 인덱스
		const int32 VideoFormat = mCinematicMediaPlayer->GetTrackFormat(EMediaPlayerTrack::Video, VideoTrack); // 해당 트랙의 포맷 인덱스
		const FIntPoint VideoDimensions = mCinematicMediaPlayer->GetVideoTrackDimensions(VideoTrack, VideoFormat); // 트랙 포맷의 실제 픽셀 해상도
		if (VideoDimensions.X > 0 && VideoDimensions.Y > 0)
		{
			mCinematicVideoNativeSize = FVector2D(VideoDimensions.X, VideoDimensions.Y); // 파일 파싱 값을 트랙 실측값으로 갱신
			mCinematicVideoBrush.ImageSize = mCinematicVideoNativeSize;                  // 브러시 이미지 크기를 실해상도에 맞춰 비율 보존
		}

		const float MediaDurationSeconds = StaticCast<float>(mCinematicMediaPlayer->GetDuration().GetTotalSeconds());
		if (MediaDurationSeconds > 0.0f)
		{
			StartDefaultCinematicTimer(MediaDurationSeconds + 1.0f); // 길이를 알면 (영상길이+1초) 후 강제 종료 보장(OnEndReached 누락 대비)
		}
		else
		{
			StartDefaultCinematicTimer(mDefaultCinematicDuration); // 길이 미상이면 기본 재생 시간으로 폴백 타이머 설정
		}
	}
}

/**
 * @brief 미디어 소스 open 실패 콜백. 경고 로그 후 시네마틱을 즉시 종료(다음 단계로 진행)한다.
 * @param FailedUrl open에 실패한 미디어 URL
 */
void UCinematicWidget::HandleCinematicMediaOpenFailed(FString FailedUrl)
{
	UE_LOG(LogRD, Warning, TEXT("Intro cinematic media open failed: %s"), *FailedUrl);
	FinishCinematic(); // 재생 불가 시 멈추지 않고 곧장 인트로 종료 처리
}

/**
 * @brief 영상이 끝까지 재생되었을 때의 콜백. 시네마틱을 종료한다.
 */
void UCinematicWidget::HandleCinematicMediaEndReached()
{
	FinishCinematic(); // 정상 종료 → 다음 단계로 전환
}
