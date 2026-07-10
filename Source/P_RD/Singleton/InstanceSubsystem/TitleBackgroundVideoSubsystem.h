/**
 * @file   TitleBackgroundVideoSubsystem.h
 * @brief  타이틀 배경 영상(mp4)을 위젯보다 먼저 열어두는 GameInstance 단위 캐시 서브시스템.
 * @author 박용수
 * @date   2026-06-26
 **/

#pragma once

#include "RDMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "TitleBackgroundVideoSubsystem.generated.h"

class UFileMediaSource;
class UMediaPlayer;
class UMediaTexture;

/**
 * @brief 타이틀 배경 영상을 위젯보다 먼저 열어 첫 프레임 지연을 줄이는 GameInstance 단위 캐시.
 * @details
 *  언리얼 미디어 재생은 세 객체가 한 세트로 동작한다:
 *   - UFileMediaSource : 재생할 파일(mp4 경로)을 가리키는 "소스".
 *   - UMediaPlayer     : 소스를 열어 디코딩/재생/루프하는 "플레이어".
 *   - UMediaTexture    : 플레이어의 현재 프레임을 받아 텍스처로 출력 → 위젯/머티리얼이 이 텍스처를 그린다.
 *  흐름: MediaSource(파일) → MediaPlayer.OpenSource()로 열기 → 재생 → MediaTexture가 매 프레임 갱신 → UI가 텍스처 표시.
 *
 *  이 서브시스템은 GameInstance 단위(레벨 전환에도 살아 있음)라, 타이틀 진입 전에 PreloadTitleBackgroundVideo()로
 *  미리 열어두면 위젯이 뜰 때 첫 프레임이 준비돼 있어 검은 화면/끊김을 줄인다.
 *  미디어 열기는 비동기이므로 OnMediaOpened/OnMediaOpenFailed 콜백으로 완료를 통지받는다.
 */
UCLASS()
class P_RD_API UTitleBackgroundVideoSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * @brief  타이틀 배경 영상을 열고 루프 재생을 시작한다. 위젯 표시 전에 호출해 첫 프레임 지연을 줄인다.
	 * @param  RelativeContentPath  Content 기준 상대 경로의 mp4. 빈 문자열이면 게임 설정의 기본 타이틀 배경 영상.
	 * @details 같은 경로로 이미 열도록 요청했으면 무시(중복 열기 방지). 실제 첫 프레임 준비는 HandleMediaOpened에서 확정.
	 */
	void PreloadTitleBackgroundVideo(const FString& RelativeContentPath);

	/** @brief 타이틀 화면을 벗어날 때 공유 미디어 재생/디코더를 닫는다. */
	void StopTitleBackgroundVideo();

	/** @brief 위젯이 브러시/머티리얼 입력으로 쓸 미디어 플레이어 접근자(아직 안 열렸으면 nullptr 가능). */
	UMediaPlayer* GetMediaPlayer() const { return mMediaPlayer; }

	/** @brief 위젯이 직접 그릴 출력 텍스처 접근자(플레이어의 현재 프레임이 들어 있다). */
	UMediaTexture* GetMediaTexture() const { return mMediaTexture; }

	/** @brief 열린 영상의 원본 픽셀 크기(가로/세로). 위젯이 뷰포트 좌우맞춤+상하크롭 비율을 계산할 때 사용. */
	FVector2D GetVideoNativeSize() const { return mVideoNativeSize; }

	/** @brief 미디어가 실제로 열려(첫 프레임 준비) 재생 중인지. 위젯이 표시 타이밍 판단에 사용. */
	bool IsMediaOpened() const { return mMediaOpened; }

protected:
	/** @brief GameInstance 종료 시 정리: 콜백 해제 + 미디어 Close(디코더/스레드 반환). */
	void Deinitialize() override;

private:
	/** @brief MediaPlayer/MediaTexture/MediaSource 3종을 1회 생성·연결한다(콜백/루프 설정 포함, 중복 호출 안전). */
	void EnsureMediaObjects();

	/** @brief 미디어 열기 성공 콜백(비동기). 루프 재생 시작 + 비디오 트랙에서 원본 해상도를 읽어 mVideoNativeSize 갱신. */
	UFUNCTION()
	void HandleMediaOpened(FString OpenedUrl);

	/** @brief 미디어 열기 실패 콜백(비동기). 요청 플래그 초기화 + 경고 로그. */
	UFUNCTION()
	void HandleMediaOpenFailed(FString FailedUrl);

private:
	/** @brief 영상 디코딩/재생 엔진. */
	UPROPERTY(Transient)
	TObjectPtr<UMediaPlayer> mMediaPlayer;

	/** @brief 플레이어의 현재 프레임을 받는 출력 텍스처(위젯이 이걸 그린다). */
	UPROPERTY(Transient)
	TObjectPtr<UMediaTexture> mMediaTexture;

	/** @brief 재생할 mp4 파일을 가리키는 소스. */
	UPROPERTY(Transient)
	TObjectPtr<UFileMediaSource> mMediaSource;

	/** @brief 현재 열려 있는(또는 요청한) 영상의 Content 상대 경로. 같은 경로 재요청 시 중복 열기 방지용. */
	FString mCurrentRelativePath;

	/** @brief 열린 영상 원본 크기. 열기 전 기본값 1280x1280, 콜백에서 실제 값으로 갱신. */
	FVector2D mVideoNativeSize = FVector2D(1280.0f, 1280.0f);

	/** @brief OpenSource를 한 번이라도 요청했는지(중복 열기 방지 플래그). */
	bool mOpenRequested = false;

	/** @brief 미디어가 실제로 열려 첫 프레임이 준비됐는지(OnMediaOpened에서 true). */
	bool mMediaOpened = false;
};
