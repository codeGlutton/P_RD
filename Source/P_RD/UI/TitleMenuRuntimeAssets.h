#pragma once

/**
 * @file TitleMenuRuntimeAssets.h
 * @brief 타이틀 화면 배경 영상 재생에 필요한 런타임 핸들/리소스 묶음.
 * @details UTitleBackgroundVideoSubsystem이 만든 루프 배경 mp4 자산(MediaPlayer/Texture/Source)과
 *          WBP Image에 깔 브러시, 그리고 cover-crop(좌우맞춤 + 상하크롭) 계산에 필요한 원본 해상도를
 *          한 구조체로 모아 타이틀 위젯의 생명주기에 함께 들고 다니기 위한 헤더다.
 * @author 박용수
 * @date 2026-06-26
 */

#include "RDMinimal.h"
#include "Styling/SlateBrush.h" // FSlateBrush(영상 출력용 브러시) 타입을 멤버로 직접 보관하므로 전체 정의 필요

#include "TitleMenuRuntimeAssets.generated.h"

// 미디어 재생 객체들은 포인터로만 보관하므로 전방 선언으로 헤더 의존을 줄인다.
class UFileMediaSource; // 패키징된 Content 경로의 mp4를 가리키는 파일 소스
class UMediaPlayer;     // 실제 영상 디코딩/재생을 담당
class UMediaTexture;    // 재생 프레임을 받아 브러시/머티리얼로 출력

/**
 * @brief 타이틀 배경 영상 재생용 런타임 객체.
 * @details 정적 비주얼(로고/버튼/레이아웃)은 WBP_TitleMenu가 책임지고, 영상만 C++이 재생한다.
 *          표시 Image는 WBP의 TitleBackgroundImage(BindWidget)를 쓴다.
 *          모든 미디어 객체는 디스크에 저장하지 않는 Transient 리소스이며, 위젯 Destruct 시
 *          정리된다(단, mUsesSharedMedia일 때는 Subsystem이 소유하므로 Close하지 않음).
 */
USTRUCT()
struct P_RD_API FTitleMenuBackgroundRuntimeAssets
{
	GENERATED_BODY()

	/** @brief 타이틀 위젯 생명주기에 묶어 재생/Close를 제어하는 런타임 MediaPlayer. */
	UPROPERTY(Transient)
	TObjectPtr<UMediaPlayer> mMediaPlayer;

	/** @brief WBP Image Brush에 주입되는 영상 출력 텍스처; 자산으로 저장하지 않는 Transient 리소스다. */
	UPROPERTY(Transient)
	TObjectPtr<UMediaTexture> mMediaTexture;

	/** @brief 패키징된 Content 파일 경로를 MediaPlayer에 넘기기 위한 파일 소스 객체. */
	UPROPERTY(Transient)
	TObjectPtr<UFileMediaSource> mMediaSource;

	/** @brief TitleBackgroundImage에 적용하는 브러시 캐시; MediaTexture 참조 유지가 목적이다. */
	FSlateBrush mVideoBrush;

	/** @brief cover-crop 배치 계산에 쓰는 MP4 원본 해상도. */
	FVector2D mVideoNativeSize = FVector2D::ZeroVector;

	/** @brief GameInstanceSubsystem이 소유한 공유 MediaPlayer/Texture를 쓰는 중이면 위젯 Destruct에서 Close하지 않는다. */
	bool mUsesSharedMedia = false;
};
