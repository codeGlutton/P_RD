#pragma once

#include "RDMinimal.h"
#include "Styling/SlateBrush.h"

#include "TitleMenuRuntimeAssets.generated.h"

class UFileMediaSource;
class UMediaPlayer;
class UMediaTexture;

/** @brief 타이틀 배경 영상 재생용 런타임 객체. */
// 정적 비주얼(로고/버튼/레이아웃)은 WBP_TitleMenu가 책임지고,
// 영상만 C++이 재생한다. 표시 Image는 WBP의 TitleBackgroundImage(BindWidget)를 쓴다.
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
};
