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
	// 주사위가 돌아다니는 판 안쪽 영역(클램프 바운즈). 세로(Depth)는 캡처 세로가 좁아서 작게 잡아야 영역을 안 벗어난다.
	constexpr float TableHalfDepth = 120.0f;
	constexpr float TableHalfWidth = 210.0f;
	constexpr float TableFloorZ = 0.0f;
	// 벽을 충분히 높여 주사위가 튀어서 판 밖으로 넘어가지 못하게 한다.
	constexpr float TableWallHeight = 320.0f;
	constexpr float TableWallThickness = 42.0f;
	constexpr float DiceTableVisibleMarginX = 28.0f;
	constexpr float DiceTableVisibleMarginY = 38.0f;
	// 클램프용 시각 반지름: 물리 반경(30) × 최대 크기보정(~1.45배) ≈ 44. 커진 비주얼도 판 밖으로 안 삐지게 한다.
	constexpr float VisualClampRadius = 44.0f;
	constexpr float DiceAlignFrameMargin = 20.0f;   // 정렬 행 가장자리 여백(작을수록 행이 넓어져 주사위가 안 줄고 들어감)
	constexpr float DiceAlignLocalX = -18.0f;
	// 정지 감지 기반 굴림 종료(고정 시간 강제 대신). 실제로 멈추면 바로 끝낸다 — UEDice/dice-box 방식.
	constexpr float RollMinSeconds = 0.30f;            // 이 시간 전엔 완료 판정 안 함(즉시 멈춤 방지)
	constexpr float RollStillHoldSeconds = 0.045f;     // 임계 속도 이하가 이만큼 연속 유지되면 정지로 본다
	constexpr float RollMaxSeconds = 2.20f;            // 안전 캡: 이 시간엔 무조건 강제 완료
	constexpr float RollStillLinearThreshold = 22.0f;  // 정지 판정 선속도 임계(cm/s) — 높이면 더 일찍 멈춤
	constexpr float RollStillAngularThreshold = 74.0f; // 정지 판정 각속도 임계(deg/s)

	void ClampToVisibleDiceBoard(float DiceRadius, FVector& LocalLocation, FVector& LocalVelocity, bool& bClamped)
	{
		const float SafeX = FMath::Max(DiceRadius, TableHalfDepth - DiceRadius - DiceTableVisibleMarginX);
		const float SafeY = FMath::Max(DiceRadius, TableHalfWidth - DiceRadius - DiceTableVisibleMarginY);

		// 벽 반사계수↑: 판 안에서 리코셰를 크게 만들어 "따다다다당" 손맛을 낸다.
		if (LocalLocation.X < -SafeX)
		{
			LocalLocation.X = -SafeX;
			LocalVelocity.X = FMath::Abs(LocalVelocity.X) * 0.76f;
			bClamped = true;
		}
		else if (LocalLocation.X > SafeX)
		{
			LocalLocation.X = SafeX;
			LocalVelocity.X = -FMath::Abs(LocalVelocity.X) * 0.76f;
			bClamped = true;
		}

		if (LocalLocation.Y < -SafeY)
		{
			LocalLocation.Y = -SafeY;
			LocalVelocity.Y = FMath::Abs(LocalVelocity.Y) * 0.76f;
			bClamped = true;
		}
		else if (LocalLocation.Y > SafeY)
		{
			LocalLocation.Y = SafeY;
			LocalVelocity.Y = -FMath::Abs(LocalVelocity.Y) * 0.76f;
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
				LocalVelocity.X *= 0.72f;
				LocalVelocity.Y *= 0.72f;
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
	mSceneCaptureComponent->OrthoWidth = 430.0f;   // 줌인: 좁아진 판 영역을 가득 잡아 주사위를 크게(눈으로 튜닝)
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
		VisualActor->SetDiceMaterialVariant(DiceIndex);
		VisualActor->SetDiceColor(DiceSpec.mDiceColor);
		VisualActor->ClearHighlightedFace();
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
	// 감쇠를 높여, 한 번 세게 던져도 몇 번 텀블한 뒤 빠르고 깔끔하게 멈추게 한다(UEDice 1.0 / dice-box 0.4 참고).
	// 낮은 감쇠+반복 킥이 "정신 사나운" 원인이었음 → 킥 제거 + 감쇠 상향으로 "진짜 같지만 정돈된" 굴림.
	PhysicsBody->SetLinearDamping(1.35f);
	PhysicsBody->SetAngularDamping(1.85f);
	PhysicsBody->SetMassOverrideInKg(NAME_None, 0.075f, true);
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

	const int32 ColumnCount = FMath::Min(3, FMath::Max(1, DiceCount));
	const int32 RowIndex = DiceIndex / ColumnCount;
	const int32 ColumnIndex = DiceIndex % ColumnCount;
	const int32 RowCount = FMath::DivideAndRoundUp(DiceCount, ColumnCount);
	const int32 RowDiceCount = FMath::Min(ColumnCount, DiceCount - RowIndex * ColumnCount);
	const float RowWidth = StaticCast<float>(FMath::Max(0, RowDiceCount - 1)) * 88.0f;
	const float RowDepth = StaticCast<float>(FMath::Max(0, RowCount - 1)) * 76.0f;
	const float LocalY = StaticCast<float>(ColumnIndex) * 88.0f - RowWidth * 0.5f + Stream.FRandRange(-8.0f, 8.0f);
	const float LocalX = DiceAlignLocalX + StaticCast<float>(RowIndex) * 76.0f - RowDepth * 0.5f + Stream.FRandRange(-9.0f, 9.0f);
	const float LocalZ = TableFloorZ + PhysicsDiceRadius + 42.0f + StaticCast<float>((DiceIndex * 2) % 5) * 9.0f;
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

		// 한 번 세게 던지고 끝(반복 킥 없음). 서로 다른 레인+스핀으로 텀블시키되, 높은 감쇠가 몇 번 구른 뒤 정리한다.
		// 위치는 매 틱 하드클램프로 판 안에 강제되므로 이탈하지 않는다.
		const float Side = (DiceIndex % 2 == 0) ? 1.0f : -1.0f;
		const float Forward = (DiceIndex % 3 == 0) ? -1.0f : 1.0f;
		const FVector LocalImpulse(
			Forward * Stream.FRandRange(55.0f, 92.0f),
			-Side * Stream.FRandRange(90.0f, 150.0f),
			Stream.FRandRange(40.0f, 66.0f));
		const FVector LocalAngularImpulse(
			Stream.FRandRange(-2800.0f, 2800.0f),
			Stream.FRandRange(-3400.0f, 3400.0f),
			Side * Stream.FRandRange(3800.0f, 6200.0f));

		PhysicsBody->AddImpulse(GetActorTransform().TransformVectorNoScale(LocalImpulse), NAME_None, true);
		PhysicsBody->AddAngularImpulseInDegrees(GetActorTransform().TransformVectorNoScale(LocalAngularImpulse), NAME_None, true);
	}

	mRollSeed = RollSeed;
	mRollElapsed = 0.0f;
	mRollStillElapsed = 0.0f;
	mRollActive = true;
	mRollComplete = false;
	mAligning = false;
	mAlignComplete = false;
	mAlignElapsed = 0.0f;
	for (ACombatDicePreviewActor* VisualActor : mVisualDiceActors)
	{
		if (IsValid(VisualActor))
		{
			VisualActor->ClearHighlightedFace();
		}
	}
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

	// 정지 감지: 모든 주사위가 임계 속도 이하로 RollStillHold만큼 연속 유지되면 완료(실제로 멈춘 순간 종료).
	// 최소 시간 전엔 완료하지 않고, 최대 시간(안전 캡)엔 강제 완료한다.
	if (mRollElapsed >= RollMinSeconds && AreAllDiceStill())
	{
		mRollStillElapsed += DeltaSeconds;
	}
	else
	{
		mRollStillElapsed = 0.0f;
	}

	if (mRollStillElapsed >= RollStillHoldSeconds || mRollElapsed >= RollMaxSeconds)
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
		for (int32 DiceIndex = 0; DiceIndex < mVisualDiceActors.Num(); ++DiceIndex)
		{
			if (ACombatDicePreviewActor* VisualActor = mVisualDiceActors[DiceIndex])
			{
				VisualActor->SetHighlightedFace(GetSettledFaceOrdinal(DiceIndex));
			}
		}
		CaptureDice();
	}
}

bool ACombatDiceRollCaptureActor::AreAllDiceStill()
{
	for (UProceduralMeshComponent* PhysicsBody : mPhysicsDiceBodies)
	{
		if (PhysicsBody == nullptr || PhysicsBody->IsSimulatingPhysics() == false)
		{
			continue;
		}
		if (PhysicsBody->GetPhysicsLinearVelocity().Size() > RollStillLinearThreshold
			|| PhysicsBody->GetPhysicsAngularVelocityInDegrees().Size() > RollStillAngularThreshold)
		{
			return false;
		}
	}
	return true;
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
	ClampToVisibleDiceBoard(VisualClampRadius, LocalLocation, LocalVelocity, bClamped);

	if (LocalLocation.Z < SafeZ)
	{
		LocalLocation.Z = SafeZ;
		// 바닥 충돌: 작은 침투도 짧게 튕겨 올려 "따다다다당" 리듬을 유지한다.
		LocalVelocity.Z = (LocalVelocity.Z < -40.0f)
			? -LocalVelocity.Z * 0.78f + 12.0f
			: FMath::Max(28.0f, FMath::Max(0.0f, LocalVelocity.Z) * 0.35f);
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
	// 정렬 주사위는 굴림과 동일한 크기로 둔다(축소 없음). 행 폭(DiceAlignFrameMargin)으로 겹침을 조절한다.
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
		if (mVisualDiceActors.IsValidIndex(DiceIndex) && IsValid(mVisualDiceActors[DiceIndex]))
		{
			mVisualDiceActors[DiceIndex]->SetHighlightedFace(GetSettledFaceOrdinal(DiceIndex));
		}
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
