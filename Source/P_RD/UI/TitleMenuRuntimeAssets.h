#pragma once

#include "RDMinimal.h"
#include "Styling/SlateBrush.h"

#include "TitleMenuRuntimeAssets.generated.h"

class UFileMediaSource;
class UMediaPlayer;
class UMediaTexture;

/**
 * @brief 타이틀 배경 영상 재생용 런타임 객체.
 * @details 정적 비주얼(로고/버튼/레이아웃)은 WBP_TitleMenu가 책임지고,
 *          영상만 C++이 재생한다. 표시 Image는 WBP의 TitleBackgroundImage(BindWidget)를 쓴다.
 */
USTRUCT()
struct P_RD_API FTitleMenuBackgroundRuntimeAssets
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<UMediaPlayer> mMediaPlayer;

	UPROPERTY(Transient)
	TObjectPtr<UMediaTexture> mMediaTexture;

	UPROPERTY(Transient)
	TObjectPtr<UFileMediaSource> mMediaSource;

	FSlateBrush mVideoBrush;
};
