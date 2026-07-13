// Fill out your copyright notice in the Description page of Project Settings.

#include "Actor/BoardActor/Obstacle/Obstacle.h"

#include "RDCollision.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"

#include "Actor/BoardActor/Obstacle/ObstacleModel.cpp"

// Sets default values
AObstacle::AObstacle()
{
	mCapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
	mMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	mArrowComp = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowComp"));

	if (mCapsuleComp != nullptr)
	{
		mCapsuleComp->InitCapsuleSize(34.0f, 88.0f);
		mCapsuleComp->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);

		mCapsuleComp->CanCharacterStepUpOn = ECB_No;
		mCapsuleComp->SetShouldUpdatePhysicsVolume(true);
		mCapsuleComp->SetCanEverAffectNavigation(false);
		mCapsuleComp->bDynamicObstacle = true;
		RootComponent = mCapsuleComp;
	}

	if (mMeshComp != nullptr)
	{
		mMeshComp->AlwaysLoadOnClient = true;
		mMeshComp->AlwaysLoadOnServer = true;
		mMeshComp->bOwnerNoSee = false;
		mMeshComp->bCastDynamicShadow = true;
		mMeshComp->bAffectDynamicIndirectLighting = true;
		mMeshComp->PrimaryComponentTick.TickGroup = TG_PrePhysics;
		mMeshComp->SetupAttachment(mCapsuleComp);
		mMeshComp->SetGenerateOverlapEvents(false);
		mMeshComp->SetCanEverAffectNavigation(false);
		mMeshComp->SetCollisionProfileName(RDCollisionProfiles::BoardActor);
		mMeshComp->SetRelativeRotation(FRotator(0., -90., 0.));
	}

	if (mArrowComp != nullptr)
	{
		mArrowComp->ArrowColor = FColor(150, 200, 255);
		mArrowComp->bTreatAsASprite = true;
		mArrowComp->SetupAttachment(mCapsuleComp);
		mArrowComp->bIsScreenSizeScaled = true;
		mArrowComp->SetSimulatePhysics(false);
	}
}

void AObstacle::BindModel(UObjectModel* Model)
{
	IActorView::BindModel(Model);
}

void AObstacle::UnbindModel(UObjectModel* Model)
{
	IActorView::UnbindModel(Model);
}

UObjectModel* AObstacle::GetModel_Internal() const
{
	return mObstacleModel.Get();
}

void AObstacle::OnPlaceTileTransform(const FTileTransform& TileTransform, const FTransform& Transform)
{
	FVector UnitLocation = Transform.GetLocation() + FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	FTransform UnitTransform = Transform;
	UnitTransform.SetLocation(UnitLocation);

	SetActorTransform(UnitTransform);
}