// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "RDMinimal.h"
#include "Shakes/SimpleCameraShakePattern.h"
#include "OrthographicCameraShakePattern.generated.h"


/** A perlin noise shaker for a single number. */
USTRUCT(BlueprintType)
struct FOrthographicNoiseShaker
{
	GENERATED_BODY()

	/** Amplitude of the perlin noise. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = OrthographicNoise)
	float Amplitude;

	/** Frequency of the sinusoidal oscillation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = OrthographicNoise)
	float Frequency;

	/** Creates a new perlin noise shaker. */
	FOrthographicNoiseShaker()
		: Amplitude(1.f)
		, Frequency(1.f)
	{
	}

	/** Advances the shake time and returns the current value */
	float Update(float DeltaTime, float AmplitudeMultiplier, float FrequencyMultiplier, float& InOutCurrentOffset) const;
};

/**
 * A camera shake that uses Perlin noise to shake the camera.
 */
UCLASS(meta = (AutoExpandCategories = "Location,Rotation,OrthoWidth,Timing"))
class P_RD_API UOrthographicCameraShakePattern : public USimpleCameraShakePattern
{
public:

	GENERATED_BODY()

	UOrthographicCameraShakePattern(const FObjectInitializer& ObjInit);

public:

	/** Amplitude multiplier for all location shake */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Location)
	float LocationAmplitudeMultiplier = 1.f;

	/** Frequency multiplier for all location shake */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Location)
	float LocationFrequencyMultiplier = 1.f;

	/** Shake in the Y axis. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Location)
	FOrthographicNoiseShaker Y;

	/** Shake in the Z axis. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Location)
	FOrthographicNoiseShaker Z;

	/** Amplitude multiplier for all rotation shake */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rotation)
	float RotationAmplitudeMultiplier = 1.f;

	/** Frequency multiplier for all rotation shake */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rotation)
	float RotationFrequencyMultiplier = 1.f;

	/** Roll shake. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rotation)
	FOrthographicNoiseShaker Roll;

	/** FOV shake. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = OrthoWidth)
	FOrthographicNoiseShaker OrthoWidth;

private:

	// UCameraShakePattern interface
	virtual void StartShakePatternImpl(const FCameraShakePatternStartParams& Params) override;
	virtual void UpdateShakePatternImpl(const FCameraShakePatternUpdateParams& Params, FCameraShakePatternUpdateResult& OutResult) override;
	virtual void ScrubShakePatternImpl(const FCameraShakePatternScrubParams& Params, FCameraShakePatternUpdateResult& OutResult) override;

	void UpdateOrthographicNoise(float DeltaTime, FCameraShakePatternUpdateResult& OutResult);

private:

	/** Initial perlin noise offset for location oscillation. */
	FVector3f InitialLocationOffset;
	/** Current perlin noise offset for location oscillation. */
	FVector3f CurrentLocationOffset;

	/** Initial perlin noise offset for rotation oscillation. */
	FVector3f InitialRotationOffset;
	/** Current perlin noise offset for rotation oscillation. */
	FVector3f CurrentRotationOffset;

	/** Initial perlin noise offset for OrthoWidth oscillation */
	float InitialOrthoWidthOffset;
	/** Current perlin noise offset for OrthoWidth oscillation */
	float CurrentOrthoWidthOffset;
};