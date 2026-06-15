#pragma once

#include "RDMinimal.h"
#include "Styling/SlateBrush.h"

#include "TitleMenuRuntimeAssets.generated.h"

class UFileMediaSource;
class UImage;
class UMediaPlayer;
class UMediaTexture;
class UTexture2D;

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

	UPROPERTY(Transient)
	TObjectPtr<UImage> mVideoImage;

	FSlateBrush mVideoBrush;
};

USTRUCT()
struct P_RD_API FTitleMenuLogoRuntimeAssets
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> mTexture;

	UPROPERTY(Transient)
	TObjectPtr<UImage> mImage;

	FSlateBrush mBrush;
};

USTRUCT()
struct P_RD_API FTitleMenuButtonRuntimeAssets
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> mMainTexture;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> mSettingsTexture;

	UPROPERTY(Transient)
	TObjectPtr<UImage> mContinueBackgroundImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> mStartBackgroundImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> mSettingsBackgroundImage;

	FSlateBrush mMainBrush;

	FSlateBrush mSettingsBrush;
};
