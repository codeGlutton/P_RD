#include "Actor/Dice/CombatDiceRollCaptureActor.h"

#include "Actor/Dice/CombatDicePreviewActor.h"
#include "Actor/Dice/DicePolyhedron.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Math/RotationMatrix.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float TableHalfDepth = 198.0f;
	constexpr float TableHalfWidth = 250.0f;
	constexpr float TableFloorZ = 0.0f;
	// 벽을 충분히 높여 주사위가 튀어서 판 밖으로 넘어가지 못하게 한다.
	constexpr float TableWallHeight = 320.0f;
	constexpr float TableWallThickness = 42.0f;
	constexpr float DiceTableVisibleMarginX = 44.0f;
	constexpr float DiceTableVisibleMarginY = 56.0f;
	constexpr float DiceAlignFrameMargin = 56.0f;
	constexpr float DiceAlignLocalX = -18.0f;

	void ClampToVisibleDiceBoard(float DiceRadius, FVector& LocalLocation, FVector& LocalVelocity, bool& bClamped)
	{
		const float SafeX = FMath::Max(DiceRadius, TableHalfDepth - DiceRadius - DiceTableVisibleMarginX);
		const float SafeY = FMath::Max(DiceRadius, TableHalfWidth - DiceRadius - DiceTableVisibleMarginY);

		if (LocalLocation.X < -SafeX)
		{
			LocalLocation.X = -SafeX;
			LocalVelocity.X = FMath::Abs(LocalVelocity.X) * 0.24f;
			bClamped = true;
		}
		else if (LocalLocation.X > SafeX)
		{
			LocalLocation.X = SafeX;
			LocalVelocity.X = -FMath::Abs(LocalVelocity.X) * 0.24f;
			bClamped = true;
		}

		if (LocalLocation.Y < -SafeY)
		{
			LocalLocation.Y = -SafeY;
			LocalVelocity.Y = FMath::Abs(LocalVelocity.Y) * 0.24f;
			bClamped = true;
		}
		else if (LocalLocation.Y > SafeY)
		{
			LocalLocation.Y = SafeY;
			LocalVelocity.Y = -FMath::Abs(LocalVelocity.Y) * 0.24f;
			bClamped = true;
		}

		const float CornerStartX = SafeX * 0.56f;
		const float CornerStartY = SafeY * 0.56f;
		const float AbsX = FMath::Abs(LocalLocation.X);
		const float AbsY = FMath::Abs(LocalLocation.Y);
		if (AbsX > CornerStartX && AbsY > CornerStartY)
		{
			const float ExcessX = (AbsX - CornerStartX) / FMath::Max(KINDA_SMALL_NUMBER, SafeX - CornerStartX);
			const float ExcessY = (AbsY - CornerStartY) / FMath::Max(KINDA_SMALL_NUMBER, SafeY - CornerStartY);
			const float ExcessSum = ExcessX + ExcessY;
			if (ExcessSum > 1.0f)
			{
				const float Scale = 1.0f / ExcessSum;
				const float NewAbsX = CornerStartX + (AbsX - CornerStartX) * Scale;
				const float NewAbsY = CornerStartY + (AbsY - CornerStartY) * Scale;
				LocalLocation.X = FMath::Sign(LocalLocation.X) * NewAbsX;
				LocalLocation.Y = FMath::Sign(LocalLocation.Y) * NewAbsY;
				LocalVelocity.X *= 0.18f;
				LocalVelocity.Y *= 0.18f;
				bClamped = true;
			}
		}
	}

	void BuildPolyhedronMesh(
		int32 FaceCount,
		float Radius,
		TArray<FVector>& OutVertices,
		TArray<int32>& OutTriangles,
		TArray<FVector>& OutNormals,
		TArray<FVector2D>& OutUVs,
		TArray<FLinearColor>& OutColors,
		TArray<FProcMeshTangent>& OutTangents)
	{
		const RDDicePolyhedron::FDicePolyhedron& Poly = RDDicePolyhedron::Get(FaceCount);
		OutVertices.Reset();
		OutTriangles.Reset();
		OutNormals.Reset();
		OutUVs.Reset();
		OutColors.Reset();
		OutTangents.Reset();

		OutVertices.Reserve(Poly.mVertices.Num());
		OutNormals.Reserve(Poly.mVertices.Num());
		OutUVs.Reserve(Poly.mVertices.Num());
		OutColors.Reserve(Poly.mVertices.Num());
		OutTangents.Reserve(Poly.mVertices.Num());
		for (const FVector& Vertex : Poly.mVertices)
		{
			OutVertices.Add(Vertex * Radius);
			OutNormals.Add(Vertex.GetSafeNormal());
			OutUVs.Add(FVector2D::ZeroVector);
			OutColors.Add(FLinearColor::White);
			OutTangents.Add(FProcMeshTangent(FVector::YAxisVector, false));
		}

		for (const RDDicePolyhedron::FDiceFace& Face : Poly.mFaces)
		{
			for (int32 TriangleIndex = 1; TriangleIndex < Face.mVertexIndices.Num() - 1; ++TriangleIndex)
			{
				OutTriangles.Add(Face.mVertexIndices[0]);
				OutTriangles.Add(Face.mVertexIndices[TriangleIndex]);
				OutTriangles.Add(Face.mVertexIndices[TriangleIndex + 1]);
			}
		}
	}
}

ACombatDiceRollCaptureActor::ACombatDiceRollCaptureActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	InitializeSceneComponents();
	InitializeCaptureMaterial();
	BuildTableCollision();
}

void ACombatDiceRollCaptureActor::InitializeSceneComponents()
{
	mSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(mSceneRoot);
	mSceneRoot->SetMobility(EComponentMobility::Movable);

	mSceneCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("DiceRollSceneCapture"));
	mSceneCaptureComponent->SetupAttachment(mSceneRoot);
	mSceneCaptureComponent->SetMobility(EComponentMobility::Movable);
	mSceneCaptureComponent->SetRelativeLocation(FVector(-40.0f, 0.0f, 620.0f));
	mSceneCaptureComponent->SetRelativeRotation(FRotator(-88.0f, 0.0f, 0.0f));
	mSceneCaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;
	mSceneCaptureComponent->OrthoWidth = 560.0f;
	mSceneCaptureComponent->FOVAngle = 36.0f;
	mSceneCaptureComponent->CaptureSource = SCS_SceneColorHDR;
	mSceneCaptureComponent->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	mSceneCaptureComponent->bCaptureEveryFrame = false;
	mSceneCaptureComponent->bCaptureOnMovement = false;
	mSceneCaptureComponent->ClearShowOnlyComponents();
	mSceneCaptureComponent->ShowOnlyActors.Reset();
	mSceneCaptureComponent->ShowFlags.SetAtmosphere(false);
	mSceneCaptureComponent->ShowFlags.SetFog(false);
	mSceneCaptureComponent->ShowFlags.SetBSP(false);
	mSceneCaptureComponent->ShowFlags.SetLandscape(false);
	mSceneCaptureComponent->ShowFlags.SetBloom(false);
	mSceneCaptureComponent->ShowFlags.SetEyeAdaptation(false);
	mSceneCaptureComponent->ShowFlags.SetMotionBlur(false);

	mKeyLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("DiceRollKeyLight"));
	mKeyLight->SetupAttachment(mSceneRoot);
	mKeyLight->SetMobility(EComponentMobility::Movable);
	mKeyLight->SetRelativeLocation(FVector(-180.0f, -180.0f, 360.0f));
	mKeyLight->SetIntensity(920.0f);
	mKeyLight->SetAttenuationRadius(900.0f);
	mKeyLight->SetCastShadows(false);
	mKeyLight->SetSpecularScale(0.35f);
	mKeyLight->SetLightColor(FLinearColor(0.92f, 0.96f, 1.0f));

	mFillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("DiceRollFillLight"));
	mFillLight->SetupAttachment(mSceneRoot);
	mFillLight->SetMobility(EComponentMobility::Movable);
	mFillLight->SetRelativeLocation(FVector(180.0f, 210.0f, 230.0f));
	mFillLight->SetIntensity(360.0f);
	mFillLight->SetAttenuationRadius(820.0f);
	mFillLight->SetCastShadows(false);
	mFillLight->SetSpecularScale(0.25f);
	mFillLight->SetLightColor(FLinearColor(1.0f, 0.92f, 0.82f));

	mFloorCollision = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DiceRollFloorCollision"));
	mFloorCollision->SetupAttachment(mSceneRoot);
	mFloorCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	mFloorCollision->SetCollisionObjectType(ECC_WorldStatic);
	mFloorCollision->SetCollisionResponseToAllChannels(ECR_Block);
	mFloorCollision->SetHiddenInGame(true);
	mFloorCollision->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		mFloorCollision->SetStaticMesh(CubeMeshFinder.Object);
	}
}

void ACombatDiceRollCaptureActor::InitializeCaptureMaterial()
{
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CaptureMaterialFinder(TEXT("/Game/SVN/OutSideAsset/AICreation/Dice/M_DiceCaptureUI.M_DiceCaptureUI"));
	if (CaptureMaterialFinder.Succeeded())
	{
		mCaptureMaterialTemplate = CaptureMaterialFinder.Object;
	}
}

void ACombatDiceRollCaptureActor::BuildTableCollision()
{
	if (mFloorCollision != nullptr)
	{
		mFloorCollision->SetRelativeLocation(FVector(0.0f, 0.0f, TableFloorZ - 12.0f));
		mFloorCollision->SetRelativeScale3D(FVector(
			(TableHalfDepth * 2.0f + TableWallThickness * 2.0f) / 100.0f,
			(TableHalfWidth * 2.0f + TableWallThickness * 2.0f) / 100.0f,
			0.18f));
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded() == false)
	{
		return;
	}

	struct FWallSpec
	{
		FVector Location;
		FRotator Rotation = FRotator::ZeroRotator;
		FVector Scale;
	};

	// 벽 높이(Z 스케일)를 TableWallHeight에 맞춰 바닥 아래부터 충분히 높게 덮는다(큐브 기본 100 단위).
	const float WallScaleZ = (TableWallHeight + 20.0f) / 100.0f;
	const float WallThicknessScale = TableWallThickness / 100.0f;
	const float SafeX = FMath::Max(PhysicsDiceRadius, TableHalfDepth - PhysicsDiceRadius - DiceTableVisibleMarginX);
	const float SafeY = FMath::Max(PhysicsDiceRadius, TableHalfWidth - PhysicsDiceRadius - DiceTableVisibleMarginY);
	const float CornerStartX = SafeX * 0.56f;
	const float CornerStartY = SafeY * 0.56f;
	const float TopBottomWallScale = (CornerStartX * 2.0f + TableWallThickness) / 100.0f;
	const float SideWallScale = (CornerStartY * 2.0f + TableWallThickness) / 100.0f;
	const float DiagonalDx = SafeX - CornerStartX;
	const float DiagonalDy = SafeY - CornerStartY;
	const float DiagonalWallScale = (FMath::Sqrt(DiagonalDx * DiagonalDx + DiagonalDy * DiagonalDy) + TableWallThickness) / 100.0f;
	const float WallZ = TableWallHeight * 0.5f;

	TArray<FWallSpec> WallSpecs;
	WallSpecs.Reserve(8);
	WallSpecs.Add({ FVector(0.0f, -SafeY - TableWallThickness * 0.5f, WallZ), FRotator::ZeroRotator, FVector(TopBottomWallScale, WallThicknessScale, WallScaleZ) });
	WallSpecs.Add({ FVector(0.0f,  SafeY + TableWallThickness * 0.5f, WallZ), FRotator::ZeroRotator, FVector(TopBottomWallScale, WallThicknessScale, WallScaleZ) });
	WallSpecs.Add({ FVector(-SafeX - TableWallThickness * 0.5f, 0.0f, WallZ), FRotator::ZeroRotator, FVector(WallThicknessScale, SideWallScale, WallScaleZ) });
	WallSpecs.Add({ FVector( SafeX + TableWallThickness * 0.5f, 0.0f, WallZ), FRotator::ZeroRotator, FVector(WallThicknessScale, SideWallScale, WallScaleZ) });

	for (const float SignX : { -1.0f, 1.0f })
	{
		for (const float SignY : { -1.0f, 1.0f })
		{
			const FVector2D P0(SignX * CornerStartX, SignY * SafeY);
			const FVector2D P1(SignX * SafeX, SignY * CornerStartY);
			const FVector2D Direction = P1 - P0;
			const FVector2D OutwardNormal = FVector2D(SignX * DiagonalDy, SignY * DiagonalDx).GetSafeNormal();
			const FVector2D Center2D = (P0 + P1) * 0.5f + OutwardNormal * (TableWallThickness * 0.5f);
			const float YawDegrees = FMath::RadiansToDegrees(FMath::Atan2(Direction.Y, Direction.X));

			WallSpecs.Add({
				FVector(Center2D.X, Center2D.Y, WallZ),
				FRotator(0.0f, YawDegrees, 0.0f),
				FVector(DiagonalWallScale, WallThicknessScale, WallScaleZ)
			});
		}
	}

	for (int32 WallIndex = 0; WallIndex < WallSpecs.Num(); ++WallIndex)
	{
		UStaticMeshComponent* Wall = CreateDefaultSubobject<UStaticMeshComponent>(
			FName(*FString::Printf(TEXT("DiceRollWallCollision_%d"), WallIndex)));
		if (Wall == nullptr)
		{
			continue;
		}

		Wall->SetupAttachment(mSceneRoot);
		Wall->SetStaticMesh(CubeMeshFinder.Object);
		Wall->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Wall->SetCollisionObjectType(ECC_WorldStatic);
		Wall->SetCollisionResponseToAllChannels(ECR_Block);
		Wall->SetRelativeLocation(WallSpecs[WallIndex].Location);
		Wall->SetRelativeRotation(WallSpecs[WallIndex].Rotation);
		Wall->SetRelativeScale3D(WallSpecs[WallIndex].Scale);
		Wall->SetHiddenInGame(true);
		Wall->SetVisibility(false);
		mWallCollisions.Add(Wall);
	}
}

void ACombatDiceRollCaptureActor::InitializeCapture(UObject* RenderTargetOuter, int32 RenderTargetSize)
{
	InitializeCapture(RenderTargetOuter, RenderTargetSize, RenderTargetSize);
}

void ACombatDiceRollCaptureActor::InitializeCapture(UObject* RenderTargetOuter, int32 RenderTargetWidth, int32 RenderTargetHeight)
{
	if (mSceneCaptureComponent == nullptr)
	{
		return;
	}

	const int32 ClampedRenderTargetWidth = FMath::Clamp(RenderTargetWidth, 256, 2048);
	const int32 ClampedRenderTargetHeight = FMath::Clamp(RenderTargetHeight, 256, 2048);
	UObject* TargetOuter = RenderTargetOuter != nullptr ? RenderTargetOuter : this;
	mRenderTarget = NewObject<UTextureRenderTarget2D>(TargetOuter);
	if (mRenderTarget != nullptr)
	{
		mRenderTarget->RenderTargetFormat = RTF_RGBA8;
		mRenderTarget->ClearColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
		mRenderTarget->bAutoGenerateMips = false;
		mRenderTarget->InitAutoFormat(ClampedRenderTargetWidth, ClampedRenderTargetHeight);
		mRenderTarget->UpdateResourceImmediate(true);
	}

	mSceneCaptureComponent->TextureTarget = mRenderTarget;
	if (mCaptureMaterialTemplate != nullptr && mRenderTarget != nullptr)
	{
		mCaptureMaterial = UMaterialInstanceDynamic::Create(mCaptureMaterialTemplate, this);
		if (mCaptureMaterial != nullptr)
		{
			mCaptureMaterial->SetTextureParameterValue(TEXT("DiceCaptureTexture"), mRenderTarget);
		}
	}

	CaptureDice();
}

void ACombatDiceRollCaptureActor::ConfigureDice(const TArray<FCombatDiceRollPhysicsSpec>& DiceSpecs)
{
	ClearDice();

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	mSceneCaptureComponent->ClearShowOnlyComponents();
	mSceneCaptureComponent->ShowOnlyActors.Reset();

	const int32 DiceCount = DiceSpecs.Num();
	FRandomStream ResetStream(13791 + DiceCount * 53);
	mVisualDiceActors.Reserve(DiceCount);
	mPhysicsDiceBodies.Reserve(DiceCount);
	mPhysicsFaceCounts.Reserve(DiceCount);

	for (int32 DiceIndex = 0; DiceIndex < DiceCount; ++DiceIndex)
	{
		const FCombatDiceRollPhysicsSpec& DiceSpec = DiceSpecs[DiceIndex];
		UProceduralMeshComponent* PhysicsBody = CreatePhysicsBody(DiceIndex, DiceSpec.mFaceCount);
		if (PhysicsBody == nullptr)
		{
			continue;
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ACombatDicePreviewActor* VisualActor = World->SpawnActor<ACombatDicePreviewActor>(
			ACombatDicePreviewActor::StaticClass(),
			GetActorLocation(),
			GetActorRotation(),
			SpawnParameters);
		if (VisualActor == nullptr)
		{
			PhysicsBody->DestroyComponent();
			continue;
		}

		VisualActor->SetDiceType(DiceSpec.mFaceCount);
		VisualActor->SetFaceData(DiceSpec.mFaceValues, DiceSpec.mFaceTextures);
		VisualActor->SetDiceColor(DiceSpec.mDiceColor);
		VisualActor->SetBackdropVisible(false);
		VisualActor->SetPreviewLightingEnabled(false);
		VisualActor->SetDiceVisibleInSceneCaptureOnly(true);
		VisualActor->SetActorScale3D(FVector::OneVector);

		mSceneCaptureComponent->ShowOnlyActors.Add(VisualActor);
		mSceneCaptureComponent->ShowOnlyActorComponents(VisualActor, true);

		mPhysicsDiceBodies.Add(PhysicsBody);
		mVisualDiceActors.Add(VisualActor);
		mPhysicsFaceCounts.Add(DiceSpec.mFaceCount);
		mDiceFaceValues.Add(DiceSpec.mFaceValues);
		ResetDiceBodyPose(PhysicsBody, DiceIndex, DiceCount, ResetStream);
	}

	SyncVisualDiceToPhysics();
	mRollActive = false;
	mRollComplete = false;
	mRollElapsed = 0.0f;
	mAligning = false;
	mAlignComplete = false;
	mAlignElapsed = 0.0f;
	mAlignStartTransforms.Reset();
	mAlignTargetTransforms.Reset();
	CaptureDice();
}

void ACombatDiceRollCaptureActor::ClearDice()
{
	for (ACombatDicePreviewActor* VisualActor : mVisualDiceActors)
	{
		if (IsValid(VisualActor))
		{
			VisualActor->Destroy();
		}
	}
	mVisualDiceActors.Reset();

	for (UProceduralMeshComponent* PhysicsBody : mPhysicsDiceBodies)
	{
		if (PhysicsBody != nullptr)
		{
			PhysicsBody->DestroyComponent();
		}
	}
	mPhysicsDiceBodies.Reset();
	mPhysicsFaceCounts.Reset();
	mDiceFaceValues.Reset();
}

UProceduralMeshComponent* ACombatDiceRollCaptureActor::CreatePhysicsBody(int32 DiceIndex, int32 FaceCount)
{
	UProceduralMeshComponent* PhysicsBody = NewObject<UProceduralMeshComponent>(
		this,
		FName(*FString::Printf(TEXT("DiceRollPhysicsBody_%d"), DiceIndex)));
	if (PhysicsBody == nullptr)
	{
		return nullptr;
	}

	PhysicsBody->SetupAttachment(mSceneRoot);
	PhysicsBody->SetMobility(EComponentMobility::Movable);
	// convex hull을 simple 충돌로 채택해야 동적(시뮬레이션) 강체가 된다.
	// 기본값(true)이면 per-poly trimesh가 simple로 잡혀 SetSimulatePhysics가 키네마틱으로 남고
	// AddImpulse/AddAngularImpulse가 전부 무시되어 주사위가 구르지 않는다.
	PhysicsBody->bUseComplexAsSimpleCollision = false;
	PhysicsBody->bUseAsyncCooking = false;
	PhysicsBody->RegisterComponent();
	PhysicsBody->SetHiddenInGame(true);
	PhysicsBody->SetVisibility(false);
	PhysicsBody->SetCanEverAffectNavigation(false);

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> Colors;
	TArray<FProcMeshTangent> Tangents;
	BuildPolyhedronMesh(FaceCount, PhysicsDiceRadius, Vertices, Triangles, Normals, UVs, Colors, Tangents);
	PhysicsBody->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, true);
	PhysicsBody->ClearCollisionConvexMeshes();
	PhysicsBody->AddCollisionConvexMesh(Vertices);

	PhysicsBody->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PhysicsBody->SetCollisionObjectType(ECC_PhysicsBody);
	PhysicsBody->SetCollisionResponseToAllChannels(ECR_Block);
	// 댐핑을 약간 높여 짧은 굴림 시간 안에 깔끔하게 멈추게 한다(빠른 손맛).
	PhysicsBody->SetLinearDamping(1.05f);
	PhysicsBody->SetAngularDamping(1.65f);
	PhysicsBody->SetMassOverrideInKg(NAME_None, 0.18f, true);
	PhysicsBody->BodyInstance.bUseCCD = true;
	PhysicsBody->SetSimulatePhysics(false);
	return PhysicsBody;
}

void ACombatDiceRollCaptureActor::ResetDiceBodyPose(UProceduralMeshComponent* PhysicsBody, int32 DiceIndex, int32 DiceCount, FRandomStream& Stream) const
{
	if (PhysicsBody == nullptr)
	{
		return;
	}

	PhysicsBody->SetSimulatePhysics(false);
	PhysicsBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
	PhysicsBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);

	const int32 ColumnCount = FMath::Min(4, FMath::Max(1, DiceCount));
	const int32 RowIndex = DiceIndex / ColumnCount;
	const int32 ColumnIndex = DiceIndex % ColumnCount;
	const float RowWidth = StaticCast<float>(ColumnCount - 1) * 66.0f;
	const float LocalY = StaticCast<float>(ColumnIndex) * 66.0f - RowWidth * 0.5f;
	const float LocalX = -74.0f + StaticCast<float>(RowIndex) * 52.0f + Stream.FRandRange(-8.0f, 8.0f);
	const float LocalZ = TableFloorZ + PhysicsDiceRadius + 24.0f + StaticCast<float>(DiceIndex % 3) * 7.0f;
	const FVector LocalLocation(LocalX, LocalY, LocalZ);
	const FRotator LocalRotation(
		Stream.FRandRange(-42.0f, 42.0f),
		Stream.FRandRange(0.0f, 360.0f),
		Stream.FRandRange(-42.0f, 42.0f));

	PhysicsBody->SetWorldLocationAndRotation(
		GetActorTransform().TransformPosition(LocalLocation),
		(GetActorQuat() * LocalRotation.Quaternion()).Rotator(),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
}

void ACombatDiceRollCaptureActor::StartRoll(int32 RollSeed)
{
	FRandomStream Stream(RollSeed);
	const int32 DiceCount = mPhysicsDiceBodies.Num();

	for (int32 DiceIndex = 0; DiceIndex < DiceCount; ++DiceIndex)
	{
		UProceduralMeshComponent* PhysicsBody = mPhysicsDiceBodies[DiceIndex];
		if (PhysicsBody == nullptr)
		{
			continue;
		}

		ResetDiceBodyPose(PhysicsBody, DiceIndex, DiceCount, Stream);
		PhysicsBody->SetSimulatePhysics(true);
		PhysicsBody->WakeAllRigidBodies();

		// 좌우(Y) 임펄스가 너무 세면 주사위가 벽을 타고 넘어 판을 벗어난다 → 적당히 줄이고 수직(Z)도 낮춘다.
		// 회전(AngularImpulse)은 손맛을 위해 충분히 유지(이동이 아니라 굴림이라 판을 벗어나게 하지 않는다).
		const float Side = (DiceIndex % 2 == 0) ? 1.0f : -1.0f;
		const FVector LocalImpulse(
			Stream.FRandRange(24.0f, 44.0f),
			-Side * Stream.FRandRange(48.0f, 82.0f),
			Stream.FRandRange(5.0f, 12.0f));
		const FVector LocalAngularImpulse(
			Stream.FRandRange(-820.0f, 820.0f),
			Stream.FRandRange(-1240.0f, 1240.0f),
			Side * Stream.FRandRange(1280.0f, 1880.0f));

		PhysicsBody->AddImpulse(GetActorTransform().TransformVectorNoScale(LocalImpulse), NAME_None, true);
		PhysicsBody->AddAngularImpulseInDegrees(GetActorTransform().TransformVectorNoScale(LocalAngularImpulse), NAME_None, true);
	}

	mRollElapsed = 0.0f;
	mRollActive = true;
	mRollComplete = false;
	mAligning = false;
	mAlignComplete = false;
	mAlignElapsed = 0.0f;
	SyncVisualDiceToPhysics();
	CaptureDice();
}

void ACombatDiceRollCaptureActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (mAligning)
	{
		UpdateAlign(DeltaSeconds);
		return;
	}

	if (mRollActive == false)
	{
		return;
	}

	mRollElapsed += DeltaSeconds;
	for (UProceduralMeshComponent* PhysicsBody : mPhysicsDiceBodies)
	{
		ConstrainDiceToTable(PhysicsBody);
	}
	SyncVisualDiceToPhysics();
	CaptureDice();

	if ((mRollElapsed >= RollCompleteMinSeconds && AreAllDiceSleeping()) || mRollElapsed >= RollForceCompleteSeconds)
	{
		mRollActive = false;
		mRollComplete = true;
		for (UProceduralMeshComponent* PhysicsBody : mPhysicsDiceBodies)
		{
			if (PhysicsBody != nullptr)
			{
				PhysicsBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
				PhysicsBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
				PhysicsBody->SetSimulatePhysics(false);
			}
		}
		SyncVisualDiceToPhysics();
		CaptureDice();
	}
}

void ACombatDiceRollCaptureActor::ConstrainDiceToTable(UProceduralMeshComponent* PhysicsBody) const
{
	if (PhysicsBody == nullptr)
	{
		return;
	}

	const FTransform ActorTransform = GetActorTransform();
	FVector LocalLocation = ActorTransform.InverseTransformPosition(PhysicsBody->GetComponentLocation());
	FVector LocalVelocity = ActorTransform.InverseTransformVectorNoScale(PhysicsBody->GetPhysicsLinearVelocity());
	bool bClamped = false;

	const float SafeZ = TableFloorZ + PhysicsDiceRadius + 2.0f;
	ClampToVisibleDiceBoard(PhysicsDiceRadius, LocalLocation, LocalVelocity, bClamped);

	if (LocalLocation.Z < SafeZ)
	{
		LocalLocation.Z = SafeZ;
		LocalVelocity.Z = FMath::Max(0.0f, LocalVelocity.Z) * 0.25f;
		bClamped = true;
	}

	if (bClamped)
	{
		PhysicsBody->SetWorldLocation(
			ActorTransform.TransformPosition(LocalLocation),
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
		PhysicsBody->SetPhysicsLinearVelocity(ActorTransform.TransformVectorNoScale(LocalVelocity));
	}
}

void ACombatDiceRollCaptureActor::SyncVisualDiceToPhysics()
{
	for (int32 DiceIndex = 0; DiceIndex < mPhysicsDiceBodies.Num(); ++DiceIndex)
	{
		if (mVisualDiceActors.IsValidIndex(DiceIndex) == false)
		{
			continue;
		}

		UProceduralMeshComponent* PhysicsBody = mPhysicsDiceBodies[DiceIndex];
		ACombatDicePreviewActor* VisualActor = mVisualDiceActors[DiceIndex];
		if (PhysicsBody == nullptr || IsValid(VisualActor) == false)
		{
			continue;
		}

		const FTransform VisualDiceTransform(
			PhysicsBody->GetComponentQuat(),
			PhysicsBody->GetComponentLocation(),
			FVector(PhysicsDiceRadius / 96.0f));
		VisualActor->SetDiceWorldTransform(VisualDiceTransform);
	}
}

bool ACombatDiceRollCaptureActor::AreAllDiceSleeping() const
{
	for (const UProceduralMeshComponent* PhysicsBody : mPhysicsDiceBodies)
	{
		if (PhysicsBody != nullptr && PhysicsBody->RigidBodyIsAwake())
		{
			return false;
		}
	}
	return mPhysicsDiceBodies.Num() > 0;
}

int32 ACombatDiceRollCaptureActor::GetSettledFaceOrdinal(int32 DiceIndex) const
{
	if (mPhysicsDiceBodies.IsValidIndex(DiceIndex) == false || mPhysicsFaceCounts.IsValidIndex(DiceIndex) == false)
	{
		return 1;
	}

	const UProceduralMeshComponent* PhysicsBody = mPhysicsDiceBodies[DiceIndex];
	if (PhysicsBody == nullptr)
	{
		return 1;
	}

	const RDDicePolyhedron::FDicePolyhedron& Poly = RDDicePolyhedron::Get(mPhysicsFaceCounts[DiceIndex]);
	int32 BestOrdinal = 1;
	float BestDot = -FLT_MAX;
	const FQuat BodyRotation = PhysicsBody->GetComponentQuat();
	for (int32 FaceIndex = 0; FaceIndex < Poly.GetValueFaceCount(); ++FaceIndex)
	{
		const RDDicePolyhedron::FDiceFace& Face = Poly.GetValueFace(FaceIndex);
		const float Dot = FVector::DotProduct(BodyRotation.RotateVector(Face.mNormal), FVector::UpVector);
		if (Dot > BestDot)
		{
			BestDot = Dot;
			BestOrdinal = FaceIndex + 1;
		}
	}
	return BestOrdinal;
}

void ACombatDiceRollCaptureActor::GetSettledFaceOrdinals(TArray<int32>& OutFaceOrdinals) const
{
	OutFaceOrdinals.Reset(mPhysicsDiceBodies.Num());
	for (int32 DiceIndex = 0; DiceIndex < mPhysicsDiceBodies.Num(); ++DiceIndex)
	{
		OutFaceOrdinals.Add(GetSettledFaceOrdinal(DiceIndex));
	}
}

int32 ACombatDiceRollCaptureActor::GetSettledFaceValue(int32 DiceIndex) const
{
	const int32 Ordinal = GetSettledFaceOrdinal(DiceIndex);   // 1-based 결과면.
	if (mDiceFaceValues.IsValidIndex(DiceIndex) && mDiceFaceValues[DiceIndex].IsValidIndex(Ordinal - 1))
	{
		return mDiceFaceValues[DiceIndex][Ordinal - 1];
	}
	return Ordinal;
}

void ACombatDiceRollCaptureActor::StartAlign()
{
	const int32 DiceCount = mPhysicsDiceBodies.Num();

	// 결과 숫자 크기순(오름차순) 슬롯 배치: SortedDice[slot] = 그 슬롯에 놓일 주사위 index.
	TArray<int32> SortedDice;
	SortedDice.Reserve(DiceCount);
	for (int32 DiceIndex = 0; DiceIndex < DiceCount; ++DiceIndex)
	{
		SortedDice.Add(DiceIndex);
	}
	SortedDice.Sort([this](const int32& A, const int32& B)
	{
		return GetSettledFaceValue(A) < GetSettledFaceValue(B);
	});

	// UpdateAlign이 주사위 index로 접근하므로 index 기준으로 자리만큼 채워 둔다.
	mAlignStartTransforms.Init(FTransform::Identity, DiceCount);
	mAlignTargetTransforms.Init(FTransform::Identity, DiceCount);

	const int32 SpacingCount = FMath::Max(0, DiceCount - 1);
	const float AlignSafeHalfWidth = FMath::Max(PhysicsDiceRadius, TableHalfWidth - DiceAlignFrameMargin);
	const float AvailableRowWidth = FMath::Max(0.0f, AlignSafeHalfWidth * 2.0f - PhysicsDiceRadius * 2.0f);
	const float DynamicRowSpacing = SpacingCount > 0
		? FMath::Min(AlignRowSpacing, AvailableRowWidth / StaticCast<float>(SpacingCount))
		: AlignRowSpacing;
	const float RowWidth = StaticCast<float>(SpacingCount) * DynamicRowSpacing;
	const FVector VisualScale(PhysicsDiceRadius / 96.0f);

	for (int32 Slot = 0; Slot < DiceCount; ++Slot)
	{
		const int32 DiceIndex = SortedDice[Slot];
		UProceduralMeshComponent* PhysicsBody = mPhysicsDiceBodies[DiceIndex];
		if (PhysicsBody == nullptr)
		{
			continue;
		}

		// 시작: 현재(굴림이 멈춘) 물리 바디의 월드 포즈.
		const FTransform StartTransform(PhysicsBody->GetComponentQuat(), PhysicsBody->GetComponentLocation(), VisualScale);

		// 목표: 값 크기순 슬롯에 한 줄로, 결과면이 위로(숫자 똑바로) 오도록.
		const float LocalY = StaticCast<float>(Slot) * DynamicRowSpacing - RowWidth * 0.5f;
		const FVector LocalLocation(DiceAlignLocalX, LocalY, TableFloorZ + PhysicsDiceRadius);
		const FVector TargetLocation = GetActorTransform().TransformPosition(LocalLocation);
		const FTransform TargetTransform(ComputeAlignedFaceUpQuat(DiceIndex), TargetLocation, VisualScale);

		mAlignStartTransforms[DiceIndex] = StartTransform;
		mAlignTargetTransforms[DiceIndex] = TargetTransform;
	}

	mAlignElapsed = 0.0f;
	mAligning = true;
	mAlignComplete = false;
}

void ACombatDiceRollCaptureActor::UpdateAlign(float DeltaSeconds)
{
	mAlignElapsed += DeltaSeconds;
	const float RawAlpha = FMath::Clamp(mAlignElapsed / AlignDurationSeconds, 0.0f, 1.0f);
	const float Alpha = 1.0f - (1.0f - RawAlpha) * (1.0f - RawAlpha);   // EaseOutQuad: 쫙 미끄러져 정렬.

	for (int32 DiceIndex = 0; DiceIndex < mVisualDiceActors.Num(); ++DiceIndex)
	{
		ACombatDicePreviewActor* VisualActor = mVisualDiceActors[DiceIndex];
		if (IsValid(VisualActor) == false
			|| mAlignStartTransforms.IsValidIndex(DiceIndex) == false
			|| mAlignTargetTransforms.IsValidIndex(DiceIndex) == false)
		{
			continue;
		}

		FTransform Blended;
		Blended.Blend(mAlignStartTransforms[DiceIndex], mAlignTargetTransforms[DiceIndex], Alpha);
		VisualActor->SetDiceWorldTransform(Blended);
	}

	CaptureDice();

	if (RawAlpha >= 1.0f)
	{
		mAligning = false;
		mAlignComplete = true;
		CaptureDice();
	}
}

FQuat ACombatDiceRollCaptureActor::ComputeAlignedFaceUpQuat(int32 DiceIndex) const
{
	if (mPhysicsFaceCounts.IsValidIndex(DiceIndex) == false)
	{
		return FQuat::Identity;
	}

	const RDDicePolyhedron::FDicePolyhedron& Poly = RDDicePolyhedron::Get(mPhysicsFaceCounts[DiceIndex]);
	const int32 Ordinal = GetSettledFaceOrdinal(DiceIndex);   // 1-based 결과면.
	const int32 FaceIndex = FMath::Clamp(Ordinal - 1, 0, FMath::Max(0, Poly.GetValueFaceCount() - 1));
	const RDDicePolyhedron::FDiceFace& Face = Poly.GetValueFace(FaceIndex);

	// 결과면을 캡처 카메라(위에서 -Z로 내려다봄) 정면으로 세운다:
	//   - 면 법선 → 월드 +Z (카메라 쪽)
	//   - 면 up    → 월드 +X (탑다운 캡처 화면의 위쪽) → 숫자가 똑바로 선다.
	const FQuat SourceQuat = FRotationMatrix::MakeFromXZ(Face.mNormal, Face.mUp).ToQuat();
	const FQuat TargetQuat = FRotationMatrix::MakeFromXZ(FVector::UpVector, FVector::XAxisVector).ToQuat();
	return TargetQuat * SourceQuat.Inverse();
}

void ACombatDiceRollCaptureActor::CaptureDice() const
{
	if (mSceneCaptureComponent != nullptr && mRenderTarget != nullptr)
	{
		mSceneCaptureComponent->CaptureScene();
	}
}

UTextureRenderTarget2D* ACombatDiceRollCaptureActor::GetRenderTarget() const
{
	return mRenderTarget;
}

UMaterialInstanceDynamic* ACombatDiceRollCaptureActor::GetCaptureMaterial() const
{
	return mCaptureMaterial;
}

void ACombatDiceRollCaptureActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearDice();
	Super::EndPlay(EndPlayReason);
}
