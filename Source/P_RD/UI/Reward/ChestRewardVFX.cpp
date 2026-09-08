#include "UI/Reward/ChestRewardVFX.h"
#include "EffekseerSystemComponent.h"
#include "EffekseerEmitterComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#if WITH_EDITOR
#include "AssetCompilingManager.h"
#endif

void AChestRewardVFX::PreloadAssets()
{
	LoadObject<UEffekseerEffect>(nullptr, TEXT("/Game/UI/RewardConcept03New/Effekseer/chest_radiance.chest_radiance"));
	LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/UI/RewardConcept03New/Effekseer/M_ChestLiveParticles.M_ChestLiveParticles"));
	LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/UI/RewardConcept03New/Effekseer/MI_ChestParticle.MI_ChestParticle"));
	LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
	for (const TCHAR* Name : {TEXT("M_Additive_Inst"), TEXT("M_Additive_DD_Inst"), TEXT("M_Translucent_Inst"), TEXT("M_Translucent_DD_Inst"), TEXT("M_Opaque_Inst"), TEXT("M_Opaque_DD_Inst"), TEXT("M_Lighting_Inst"), TEXT("M_DistortionAdditive_Inst"), TEXT("M_DistortionAdditive_DD_Inst"), TEXT("M_DistortionTranslucent_Inst"), TEXT("M_DistortionTranslucent_DD_Inst")})
	{
		LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("/Effekseer/Materials/%s.%s"), Name, Name));
	}
#if WITH_EDITOR
	FAssetCompilingManager::Get().FinishAllCompilation();
#endif
}

AChestRewardVFX::AChestRewardVFX()
{
	Particles = CreateDefaultSubobject<UEffekseerSystemComponent>(TEXT("ChestParticles"));
	SetRootComponent(Particles);
	Particles->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Particles->SetVisibleInSceneCaptureOnly(true);
	Emitter = CreateDefaultSubobject<UEffekseerEmitterComponent>(TEXT("ChestEmitter"));
	Emitter->SetupAttachment(Particles);
	Emitter->SetAutoActivate(false);
	Emitter->SetVisibleInSceneCaptureOnly(true);
	Emitter->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("ChestCapture"));
	Capture->SetupAttachment(Particles);
	Capture->SetRelativeLocation(FVector(0, 40, 8));
	Capture->SetRelativeRotation(FRotator(0, -90, 0));
	Capture->ProjectionType = ECameraProjectionMode::Perspective;
	Capture->FOVAngle = 48;
	Capture->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;
	Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	Capture->ShowFlags.SetAtmosphere(false);
	Capture->ShowFlags.SetFog(false);
	Capture->ShowFlags.SetBloom(false);
	Capture->ShowFlags.SetDynamicShadows(false);
	Capture->ShowFlags.SetAntiAliasing(false);
	Capture->bCaptureEveryFrame = true;
	Capture->bCaptureOnMovement = false;
	Capture->AddTickPrerequisiteComponent(Particles);
}

UMaterialInstanceDynamic* AChestRewardVFX::StartEffect()
{
	auto* Effect = LoadObject<UEffekseerEffect>(nullptr,
		TEXT("/Game/UI/RewardConcept03New/Effekseer/chest_radiance.chest_radiance"));
	auto* Base = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Game/UI/RewardConcept03New/Effekseer/M_ChestLiveParticles.M_ChestLiveParticles"));
	if (!Effect || !Base) return nullptr;
	Particles->AdditiveMaterial = LoadObject<UMaterialInstanceConstant>(nullptr, TEXT("/Effekseer/Materials/M_Additive_Inst.M_Additive_Inst"));
	Particles->TranslucentMaterial = LoadObject<UMaterialInstanceConstant>(nullptr, TEXT("/Effekseer/Materials/M_Translucent_Inst.M_Translucent_Inst"));
	Particles->Additive_DD_Material = LoadObject<UMaterialInstanceConstant>(nullptr, TEXT("/Effekseer/Materials/M_Additive_DD_Inst.M_Additive_DD_Inst"));
	Particles->Translucent_DD_Material = LoadObject<UMaterialInstanceConstant>(nullptr, TEXT("/Effekseer/Materials/M_Translucent_DD_Inst.M_Translucent_DD_Inst"));
	Particles->OpaqueMaterial = LoadObject<UMaterialInstanceConstant>(nullptr, TEXT("/Effekseer/Materials/M_Opaque_Inst.M_Opaque_Inst"));
	Particles->Opaque_DD_Material = LoadObject<UMaterialInstanceConstant>(nullptr, TEXT("/Effekseer/Materials/M_Opaque_DD_Inst.M_Opaque_DD_Inst"));
	Particles->LightingMaterial = LoadObject<UMaterialInstanceConstant>(nullptr, TEXT("/Effekseer/Materials/M_Lighting_Inst.M_Lighting_Inst"));
	Particles->DistortionAdditiveMaterial = LoadObject<UMaterialInstanceConstant>(nullptr, TEXT("/Effekseer/Materials/M_DistortionAdditive_Inst.M_DistortionAdditive_Inst"));
	Particles->DistortionAdditive_DD_Material = LoadObject<UMaterialInstanceConstant>(nullptr, TEXT("/Effekseer/Materials/M_DistortionAdditive_DD_Inst.M_DistortionAdditive_DD_Inst"));
	Particles->DistortionTranslucentMaterial = LoadObject<UMaterialInstanceConstant>(nullptr, TEXT("/Effekseer/Materials/M_DistortionTranslucent_Inst.M_DistortionTranslucent_Inst"));
	Particles->DistortionTranslucent_DD_Material = LoadObject<UMaterialInstanceConstant>(nullptr, TEXT("/Effekseer/Materials/M_DistortionTranslucent_DD_Inst.M_DistortionTranslucent_DD_Inst"));
	// This effect uses basic colored sprites only. Render before depth of field
	// so the isolated scene capture includes every sprite, including soft glows.
	if (auto* Simple = LoadObject<UMaterialInstanceConstant>(nullptr, TEXT("/Game/UI/RewardConcept03New/Effekseer/MI_ChestParticle.MI_ChestParticle")))
	{
		Particles->AdditiveMaterial = Simple;
		Particles->Additive_DD_Material = Simple;
		Particles->TranslucentMaterial = Simple;
		Particles->Translucent_DD_Material = Simple;
	}
	// Missing color/alpha maps are neutral white in Effekseer. Preserve this
	// explicitly instead of handing Unreal a null texture parameter.
	Particles->AssignMaterials(Effect, nullptr);
	auto* White = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
	for (const auto& Pair : Particles->GeneratedFixedMaterials)
	{
		if (!Pair.Key->Texture && White) Pair.Value->SetTextureParameterValue(TEXT("ColorTexture"), White);
		if (!Pair.Key->AlphaTexture && White) Pair.Value->SetTextureParameterValue(TEXT("AlphaTexture"), White);
	}
	Target = NewObject<UTextureRenderTarget2D>(this);
	Target->ClearColor = FLinearColor::Black;
	Target->InitCustomFormat(768, 512, PF_FloatRGBA, false);
	Target->UpdateResourceImmediate(true);
	Capture->TextureTarget = Target;
	Capture->ShowOnlyComponent(Particles);
	Capture->ShowOnlyComponent(Emitter);
	Material = UMaterialInstanceDynamic::Create(Base, this);
	Material->SetTextureParameterValue(TEXT("Particles"), Target);
	Emitter->System = this;
	Emitter->Effect = Effect;
	Emitter->SetRelativeScale3D(FVector(.12f));
	Emitter->AllColor = FColor::White;
	Emitter->Speed = 1.f;
	Emitter->Activate(true);
	return Material;
}
