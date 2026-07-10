#include "Actor/Dice/CombatDiceRollCaptureActor.h"

#include "Actor/Dice/CombatDicePreviewActor.h"
#include "Actor/Dice/DicePolyhedron.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Math/RotationMatrix.h"
#include "ProceduralMeshComponent.h"
#include "Sound/SoundBase.h"
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
	constexpr float RollMinSeconds = 0.72f;            // 초반 굴림은 보장하되 너무 빠른 구간은 짧게 둔다
	constexpr float RollStillHoldSeconds = 0.130f;     // 임계 속도 이하가 이만큼 연속 유지되면 정지로 본다
	constexpr float RollMaxSeconds = 3.35f;            // 안전 캡: 멈춤 감지가 우선이고, 아직 움직이면 조금 더 둔다
	constexpr float RollStillLinearThreshold = 12.0f;  // 정지 판정 선속도 임계(cm/s)
	constexpr float RollStillAngularThreshold = 42.0f; // 정지 판정 각속도 임계(deg/s)
	constexpr float RollMaxAngularVelocity = 16000.0f; // 눈금이 완전히 사라질 정도의 과속을 피한다
	constexpr float RollEaseDampingStartSeconds = 0.78f;
	constexpr float RollEaseDampingFullSeconds = 2.15f;
	constexpr float RollEaseLinearDampingMin = 0.45f;
	constexpr float RollEaseLinearDampingMax = 2.95f;
	constexpr float RollEaseAngularDampingMin = 0.78f;
	constexpr float RollEaseAngularDampingMax = 4.35f;
	constexpr float RollEaseLinearSpeedStart = 320.0f;
	constexpr float RollEaseLinearSpeedEnd = 48.0f;
	constexpr float RollEaseAngularSpeedStart = 7600.0f;
	constexpr float RollEaseAngularSpeedEnd = 190.0f;
	constexpr float CoinRollAssistMinLinearSpeed = 26.0f;
	constexpr float CoinRollAssistBlendSpeed = 5.6f;
	constexpr float CoinRollVisualSpinMultiplier = 5.8f;
	constexpr float CoinSettleAssistStartSeconds = 1.15f;
	constexpr float CoinSettleLinearSpeedMax = 46.0f;
	constexpr float CoinSettleAngularSpeedMax = 700.0f;
	constexpr float CoinSettleFlatDotMin = 0.64f;     // d2가 거의 멈췄을 때만 edge 자세를 눕힌다.
	constexpr float CoinSettleTipAngularSpeed = 960.0f;
	constexpr float CoinSettleYawDampingSpeed = 3.5f;
	constexpr int32 DiceCollisionBoostMaxCount = 2;        // 주사위끼리 맞부딪혀 더 세지는 연출의 과열 방지
	constexpr float DiceCollisionBoostCooldownSeconds = 0.09f;
	constexpr float DiceCollisionBoostMinClosingSpeed = 34.0f;
	constexpr float DiceCollisionBoostLinearImpulse = 36.0f;
	constexpr float DiceCollisionBoostLiftImpulse = 4.0f;
	constexpr float DiceCollisionBoostAngularImpulse = 5200.0f;
	constexpr int32 ImpactEffectPoolSize = 8;
	constexpr float ImpactEffectDurationSeconds = 0.18f;
	constexpr float ImpactEffectCooldownSeconds = 0.055f;
	constexpr float ImpactEffectMinSpeed = 70.0f;
	constexpr float ImpactEffectMinNormalImpulse = 1.0f;
	constexpr float ImpactEffectMinSize = 10.0f;
	constexpr float ImpactEffectMaxSize = 22.0f;
	constexpr float ImpactEffectZOffset = 7.0f;
	constexpr float ImpactSoundCooldownSeconds = 0.045f;

	void ClampToVisibleDiceBoard(float DiceRadius, FVector& LocalLocation, FVector& LocalVelocity, bool& bClamped)
	{
		const float SafeX = FMath::Max(DiceRadius, TableHalfDepth - DiceRadius - DiceTableVisibleMarginX);
		const float SafeY = FMath::Max(DiceRadius, TableHalfWidth - DiceRadius - DiceTableVisibleMarginY);

		// 벽 반사계수↑: 판 안에서 리코셰를 크게 만들어 "따다다다당" 손맛을 낸다.
		if (LocalLocation.X < -SafeX)
		{
			LocalLocation.X = -SafeX;
			LocalVelocity.X = FMath::Abs(LocalVelocity.X) * 0.66f;
			bClamped = true;
		}
		else if (LocalLocation.X > SafeX)
		{
			LocalLocation.X = SafeX;
			LocalVelocity.X = -FMath::Abs(LocalVelocity.X) * 0.66f;
			bClamped = true;
		}

		if (LocalLocation.Y < -SafeY)
		{
			LocalLocation.Y = -SafeY;
			LocalVelocity.Y = FMath::Abs(LocalVelocity.Y) * 0.66f;
			bClamped = true;
		}
		else if (LocalLocation.Y > SafeY)
		{
			LocalLocation.Y = SafeY;
			LocalVelocity.Y = -FMath::Abs(LocalVelocity.Y) * 0.66f;
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
				LocalVelocity.X *= 0.58f;
				LocalVelocity.Y *= 0.58f;
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

	void AddImpactVertex(
		const FVector& Position,
		TArray<FVector>& Vertices,
		TArray<FVector>& Normals,
		TArray<FVector2D>& UVs,
		TArray<FLinearColor>& Colors,
		TArray<FProcMeshTangent>& Tangents)
	{
		Vertices.Add(Position);
		Normals.Add(FVector::UpVector);
		UVs.Add(FVector2D::ZeroVector);
		Colors.Add(FLinearColor(0.72f, 1.0f, 0.94f, 1.0f));
		Tangents.Add(FProcMeshTangent(FVector::XAxisVector, false));
	}

	void BuildImpactEffectMesh(UProceduralMeshComponent* ImpactComponent)
	{
		if (ImpactComponent == nullptr)
		{
			return;
		}

		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		TArray<FVector> Normals;
		TArray<FVector2D> UVs;
		TArray<FLinearColor> Colors;
		TArray<FProcMeshTangent> Tangents;

		constexpr int32 SegmentCount = 12;
		constexpr float InnerRadius = 0.58f;
		constexpr float OuterRadius = 1.00f;
		constexpr float SegmentFill = 0.62f;
		for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; SegmentIndex += 2)
		{
			const float AngleA = (UE_TWO_PI / SegmentCount) * StaticCast<float>(SegmentIndex);
			const float AngleB = AngleA + (UE_TWO_PI / SegmentCount) * SegmentFill;
			const FVector InnerA(FMath::Cos(AngleA) * InnerRadius, FMath::Sin(AngleA) * InnerRadius, 0.0f);
			const FVector OuterA(FMath::Cos(AngleA) * OuterRadius, FMath::Sin(AngleA) * OuterRadius, 0.0f);
			const FVector OuterB(FMath::Cos(AngleB) * OuterRadius, FMath::Sin(AngleB) * OuterRadius, 0.0f);
			const FVector InnerB(FMath::Cos(AngleB) * InnerRadius, FMath::Sin(AngleB) * InnerRadius, 0.0f);
			const int32 BaseIndex = Vertices.Num();
			AddImpactVertex(InnerA, Vertices, Normals, UVs, Colors, Tangents);
			AddImpactVertex(OuterA, Vertices, Normals, UVs, Colors, Tangents);
			AddImpactVertex(OuterB, Vertices, Normals, UVs, Colors, Tangents);
			AddImpactVertex(InnerB, Vertices, Normals, UVs, Colors, Tangents);
			Triangles.Add(BaseIndex + 0);
			Triangles.Add(BaseIndex + 1);
			Triangles.Add(BaseIndex + 2);
			Triangles.Add(BaseIndex + 0);
			Triangles.Add(BaseIndex + 2);
			Triangles.Add(BaseIndex + 3);
		}

		constexpr int32 SparkCount = 4;
		for (int32 SparkIndex = 0; SparkIndex < SparkCount; ++SparkIndex)
		{
			const float Angle = (UE_TWO_PI / SparkCount) * StaticCast<float>(SparkIndex) + UE_PI * 0.25f;
			const FVector Direction(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
			const FVector Perpendicular(-Direction.Y, Direction.X, 0.0f);
			const FVector Tip = Direction * 1.24f;
			const FVector BaseCenter = Direction * 0.22f;
			const FVector Left = BaseCenter + Perpendicular * 0.055f;
			const FVector Right = BaseCenter - Perpendicular * 0.055f;
			const int32 BaseIndex = Vertices.Num();
			AddImpactVertex(Left, Vertices, Normals, UVs, Colors, Tangents);
			AddImpactVertex(Right, Vertices, Normals, UVs, Colors, Tangents);
			AddImpactVertex(Tip, Vertices, Normals, UVs, Colors, Tangents);
			Triangles.Add(BaseIndex + 0);
			Triangles.Add(BaseIndex + 1);
			Triangles.Add(BaseIndex + 2);
		}

		ImpactComponent->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);
	}

}

ACombatDiceRollCaptureActor::ACombatDiceRollCaptureActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	InitializeSceneComponents();
	InitializeCaptureMaterial();
	InitializeAudioAssets();
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

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ImpactMaterialFinder(TEXT("/Engine/EngineDebugMaterials/VertexColorMaterial.VertexColorMaterial"));
	if (ImpactMaterialFinder.Succeeded())
	{
		mImpactMaterialTemplate = ImpactMaterialFinder.Object;
	}
}

void ACombatDiceRollCaptureActor::InitializeAudioAssets()
{
	static ConstructorHelpers::FObjectFinder<USoundBase> RollSoundFinder(TEXT("/Game/SVN/OutSideAsset/SFX/OpenGameArt_CC0/Dice/Roll/SFX_DiceRoll_RPG_CC0_09.SFX_DiceRoll_RPG_CC0_09"));
	if (RollSoundFinder.Succeeded())
	{
		mRollSound = RollSoundFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> ImpactSoundFinder(TEXT("/Game/SVN/OutSideAsset/SFX/OpenGameArt_CC0/Dice/Impact/SFX_HardSmallImpact_Kenney_CC0_01.SFX_HardSmallImpact_Kenney_CC0_01"));
	if (ImpactSoundFinder.Succeeded())
	{
		mImpactSound = ImpactSoundFinder.Object;
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

void ACombatDiceRollCaptureActor::EnsureImpactEffectComponents()
{
	if (mSceneCaptureComponent == nullptr)
	{
		return;
	}

	for (int32 EffectIndex = mImpactEffectComponents.Num(); EffectIndex < ImpactEffectPoolSize; ++EffectIndex)
	{
		UProceduralMeshComponent* ImpactComponent = NewObject<UProceduralMeshComponent>(
			this,
			FName(*FString::Printf(TEXT("DiceRollImpactEffect_%d"), EffectIndex)));
		if (ImpactComponent == nullptr)
		{
			continue;
		}

		ImpactComponent->SetupAttachment(mSceneRoot);
		ImpactComponent->SetMobility(EComponentMobility::Movable);
		ImpactComponent->RegisterComponent();
		ImpactComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ImpactComponent->SetCanEverAffectNavigation(false);
		ImpactComponent->SetCastShadow(false);
		ImpactComponent->SetVisibleInSceneCaptureOnly(true);
		ImpactComponent->SetHiddenInGame(false);
		ImpactComponent->SetVisibility(false);
		ImpactComponent->SetTranslucentSortPriority(16);
		BuildImpactEffectMesh(ImpactComponent);
		if (mImpactMaterialTemplate != nullptr)
		{
			ImpactComponent->SetMaterial(0, mImpactMaterialTemplate);
		}

		mImpactEffectComponents.Add(ImpactComponent);
	}

	mImpactEffectAges.SetNumZeroed(mImpactEffectComponents.Num());
	mImpactEffectDurations.SetNumZeroed(mImpactEffectComponents.Num());
	mImpactEffectBaseSizes.SetNumZeroed(mImpactEffectComponents.Num());
	mLastImpactSoundTime = -1000.0f;
	for (UProceduralMeshComponent* ImpactComponent : mImpactEffectComponents)
	{
		if (ImpactComponent != nullptr)
		{
			mSceneCaptureComponent->ShowOnlyComponent(ImpactComponent);
		}
	}
	ResetImpactEffects();
}

void ACombatDiceRollCaptureActor::ResetImpactEffects()
{
	mLastImpactEffectTime = -1000.0f;
	mImpactEffectCursor = 0;
	mImpactEffectAges.SetNumZeroed(mImpactEffectComponents.Num());
	mImpactEffectDurations.SetNumZeroed(mImpactEffectComponents.Num());
	mImpactEffectBaseSizes.SetNumZeroed(mImpactEffectComponents.Num());
	for (UProceduralMeshComponent* ImpactComponent : mImpactEffectComponents)
	{
		if (ImpactComponent != nullptr)
		{
			ImpactComponent->SetVisibility(false);
		}
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
	EnsureImpactEffectComponents();

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
		VisualActor->SetHideNonHighlightedFaceTexts(false);
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
		ResetDiceBodyPose(PhysicsBody, DiceIndex, DiceCount, DiceSpec.mFaceCount, ResetStream);
	}

	SyncVisualDiceToPhysics();
	mRollActive = false;
	mRollComplete = false;
	mRollElapsed = 0.0f;
	mLastCollisionBoostTime = -1000.0f;
	mCollisionBoostsUsed = 0;
	ResetImpactEffects();
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
	mCollisionBoostsUsed = 0;
	mLastCollisionBoostTime = -1000.0f;
	ResetImpactEffects();
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
	PhysicsBody->SetNotifyRigidBodyCollision(true);
	PhysicsBody->OnComponentHit.AddUniqueDynamic(this, &ACombatDiceRollCaptureActor::HandlePhysicsBodyHit);
	// 초반엔 눈금을 못 읽을 만큼 빠르게 돌고, 이후 테이블/벽 충돌과 감쇠로 자연스럽게 정리되게 둔다.
	PhysicsBody->SetLinearDamping(1.20f);
	PhysicsBody->SetAngularDamping(1.28f);
	PhysicsBody->SetMassOverrideInKg(NAME_None, 0.075f, true);
	PhysicsBody->SetPhysicsMaxAngularVelocityInDegrees(RollMaxAngularVelocity, false);
	PhysicsBody->BodyInstance.bUseCCD = true;
	PhysicsBody->SetSimulatePhysics(false);
	return PhysicsBody;
}

void ACombatDiceRollCaptureActor::ResetDiceBodyPose(UProceduralMeshComponent* PhysicsBody, int32 DiceIndex, int32 DiceCount, int32 FaceCount, FRandomStream& Stream) const
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
	const FRotator LocalRotation = FaceCount == 2
		? FRotator(
			((DiceIndex % 2) == 0 ? 1.0f : -1.0f) * Stream.FRandRange(58.0f, 74.0f),
			Stream.FRandRange(0.0f, 360.0f),
			Stream.FRandRange(-10.0f, 10.0f))
		: FRotator(
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
	SyncVisualDiceToPhysics();

	if (mRollSound != nullptr)
	{
		const int32 SoundDiceCount = FMath::Max(1, DiceCount);
		const float PerDiceVolume = FMath::Clamp(0.78f / FMath::Sqrt(StaticCast<float>(SoundDiceCount)), 0.34f, 0.62f);
		for (int32 DiceIndex = 0; DiceIndex < SoundDiceCount; ++DiceIndex)
		{
			const float Pitch = 0.96f + 0.025f * StaticCast<float>((DiceIndex + RollSeed) % 5);
			UGameplayStatics::PlaySound2D(this, mRollSound, PerDiceVolume, Pitch);
		}
	}

	for (int32 DiceIndex = 0; DiceIndex < DiceCount; ++DiceIndex)
	{
		UProceduralMeshComponent* PhysicsBody = mPhysicsDiceBodies[DiceIndex];
		if (PhysicsBody == nullptr)
		{
			continue;
		}

		const int32 FaceCount = mPhysicsFaceCounts.IsValidIndex(DiceIndex) ? mPhysicsFaceCounts[DiceIndex] : 6;
		const bool bCoinDice = FaceCount == 2;
		ResetDiceBodyPose(PhysicsBody, DiceIndex, DiceCount, FaceCount, Stream);
		PhysicsBody->SetSimulatePhysics(true);
		PhysicsBody->SetPhysicsMaxAngularVelocityInDegrees(RollMaxAngularVelocity, false);
		PhysicsBody->WakeAllRigidBodies();

		// 한 번 강하게 던지고 끝(반복 킥 없음). 초반 회전은 숫자가 읽히지 않을 정도로 크게 준다.
		// 위치는 매 틱 하드클램프로 판 안에 강제되므로 이탈하지 않는다.
		const float Side = (DiceIndex % 2 == 0) ? 1.0f : -1.0f;
		const float Forward = (DiceIndex % 3 == 0) ? -1.0f : 1.0f;
		const FVector LocalImpulse = bCoinDice
			? FVector(
				Forward * Stream.FRandRange(44.0f, 74.0f),
				-Side * Stream.FRandRange(90.0f, 142.0f),
				Stream.FRandRange(10.0f, 20.0f))
			: FVector(
				Forward * Stream.FRandRange(46.0f, 82.0f),
				-Side * Stream.FRandRange(82.0f, 138.0f),
				Stream.FRandRange(28.0f, 48.0f));
		const FVector LocalPlanarImpulse(LocalImpulse.X, LocalImpulse.Y, 0.0f);
		const FVector LocalTravelDirection = LocalPlanarImpulse.GetSafeNormal();
		const FVector LocalRollAxis = FVector::CrossProduct(LocalTravelDirection, FVector::UpVector).GetSafeNormal();
		const FVector LocalAngularImpulse = bCoinDice
			? LocalRollAxis * Stream.FRandRange(7400.0f, 11200.0f)
				+ LocalTravelDirection * Stream.FRandRange(-820.0f, 820.0f)
				+ FVector::UpVector * Side * Stream.FRandRange(650.0f, 1450.0f)
			: FVector(
				Stream.FRandRange(-4200.0f, 4200.0f),
				Stream.FRandRange(-5400.0f, 5400.0f),
				Side * Stream.FRandRange(6200.0f, 10400.0f));

		PhysicsBody->AddImpulse(GetActorTransform().TransformVectorNoScale(LocalImpulse), NAME_None, true);
		PhysicsBody->AddAngularImpulseInDegrees(GetActorTransform().TransformVectorNoScale(LocalAngularImpulse), NAME_None, true);
	}

	mRollSeed = RollSeed;
	mRollElapsed = 0.0f;
	mRollStillElapsed = 0.0f;
	mLastCollisionBoostTime = -1000.0f;
	mCollisionBoostsUsed = 0;
	ResetImpactEffects();
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
			VisualActor->SetHideNonHighlightedFaceTexts(false);
		}
	}
	SyncVisualDiceToPhysics();
	CaptureDice();
}

void ACombatDiceRollCaptureActor::TriggerImpactEffect(const FVector& WorldLocation, float Strength)
{
	if (mRollElapsed - mLastImpactEffectTime < ImpactEffectCooldownSeconds)
	{
		return;
	}
	if (mImpactEffectComponents.IsEmpty())
	{
		EnsureImpactEffectComponents();
	}
	if (mImpactEffectComponents.IsEmpty())
	{
		return;
	}

	const int32 EffectIndex = mImpactEffectCursor % mImpactEffectComponents.Num();
	UProceduralMeshComponent* ImpactComponent = mImpactEffectComponents[EffectIndex];
	if (ImpactComponent == nullptr)
	{
		return;
	}

	const FVector WorldUp = GetActorTransform().TransformVectorNoScale(FVector::UpVector).GetSafeNormal();
	const float ClampedStrength = FMath::Clamp(Strength, 0.0f, 1.0f);
	const float BaseSize = FMath::Lerp(ImpactEffectMinSize, ImpactEffectMaxSize, ClampedStrength);
	const float InitialScale = BaseSize * 0.46f;
	const float Yaw = FMath::Fmod(StaticCast<float>((mImpactEffectCursor * 47 + mRollSeed) % 360), 360.0f);

	ImpactComponent->SetWorldLocation(WorldLocation + WorldUp * ImpactEffectZOffset);
	ImpactComponent->SetWorldRotation(FRotator(0.0f, Yaw, 0.0f));
	ImpactComponent->SetWorldScale3D(FVector(InitialScale, InitialScale, InitialScale));
	ImpactComponent->SetVisibility(true);

	mImpactEffectAges[EffectIndex] = 0.0f;
	mImpactEffectDurations[EffectIndex] = ImpactEffectDurationSeconds;
	mImpactEffectBaseSizes[EffectIndex] = BaseSize;
	mLastImpactEffectTime = mRollElapsed;
	++mImpactEffectCursor;

	if (mImpactSound != nullptr)
	{
		const UWorld* World = GetWorld();
		const float Now = World != nullptr ? World->GetTimeSeconds() : mRollElapsed;
		if (Now - mLastImpactSoundTime >= ImpactSoundCooldownSeconds)
		{
			UGameplayStatics::PlaySound2D(this, mImpactSound, FMath::Lerp(0.68f, 1.24f, ClampedStrength));
			mLastImpactSoundTime = Now;
		}
	}
}

void ACombatDiceRollCaptureActor::UpdateImpactEffects(float DeltaSeconds)
{
	for (int32 EffectIndex = 0; EffectIndex < mImpactEffectComponents.Num(); ++EffectIndex)
	{
		UProceduralMeshComponent* ImpactComponent = mImpactEffectComponents[EffectIndex];
		if (ImpactComponent == nullptr || ImpactComponent->IsVisible() == false)
		{
			continue;
		}

		if (mImpactEffectDurations.IsValidIndex(EffectIndex) == false
			|| mImpactEffectDurations[EffectIndex] <= KINDA_SMALL_NUMBER)
		{
			ImpactComponent->SetVisibility(false);
			continue;
		}

		mImpactEffectAges[EffectIndex] += DeltaSeconds;
		const float RawAlpha = FMath::Clamp(mImpactEffectAges[EffectIndex] / mImpactEffectDurations[EffectIndex], 0.0f, 1.0f);
		if (RawAlpha >= 1.0f)
		{
			ImpactComponent->SetVisibility(false);
			continue;
		}

		const float BaseSize = mImpactEffectBaseSizes.IsValidIndex(EffectIndex)
			? mImpactEffectBaseSizes[EffectIndex]
			: ImpactEffectMinSize;
		const float Scale = BaseSize * (0.46f + RawAlpha * 1.18f);
		ImpactComponent->SetWorldScale3D(FVector(Scale, Scale, Scale));
		ImpactComponent->AddLocalRotation(FRotator(0.0f, 520.0f * DeltaSeconds, 0.0f));
	}
}

void ACombatDiceRollCaptureActor::HandlePhysicsBodyHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (mRollActive == false || OtherActor != this || HitComponent == nullptr || OtherComp == nullptr)
	{
		return;
	}

	UProceduralMeshComponent* HitBody = Cast<UProceduralMeshComponent>(HitComponent);
	UProceduralMeshComponent* OtherBody = Cast<UProceduralMeshComponent>(OtherComp);
	if (HitBody == nullptr || HitBody == OtherBody)
	{
		return;
	}

	const int32 HitDiceIndex = mPhysicsDiceBodies.IndexOfByPredicate(
		[HitBody](const TObjectPtr<UProceduralMeshComponent>& CandidateBody)
		{
			return CandidateBody.Get() == HitBody;
		});
	if (HitDiceIndex == INDEX_NONE)
	{
		return;
	}

	const int32 OtherDiceIndex = OtherBody != nullptr
		? mPhysicsDiceBodies.IndexOfByPredicate(
			[OtherBody](const TObjectPtr<UProceduralMeshComponent>& CandidateBody)
			{
				return CandidateBody.Get() == OtherBody;
			})
		: INDEX_NONE;

	FVector SeparationDirection = OtherBody != nullptr
		? HitBody->GetComponentLocation() - OtherBody->GetComponentLocation()
		: Hit.Normal.GetSafeNormal();
	if (SeparationDirection.Normalize() == false)
	{
		SeparationDirection = Hit.Normal.GetSafeNormal();
	}
	if (SeparationDirection.IsNearlyZero())
	{
		return;
	}

	const FVector HitVelocity = HitBody->GetPhysicsLinearVelocity();
	const FVector RelativeVelocity = OtherBody != nullptr
		? HitVelocity - OtherBody->GetPhysicsLinearVelocity()
		: HitVelocity;
	const float ClosingSpeed = FMath::Abs(FVector::DotProduct(RelativeVelocity, SeparationDirection));
	const float ImpactSpeed = FMath::Max(ClosingSpeed, RelativeVelocity.Size());
	if (ImpactSpeed >= ImpactEffectMinSpeed || NormalImpulse.Size() >= ImpactEffectMinNormalImpulse)
	{
		FVector EffectLocation = HitBody->GetComponentLocation();
		if (Hit.ImpactPoint.IsNearlyZero() == false)
		{
			EffectLocation = FVector(Hit.ImpactPoint);
		}
		const float EffectStrength = FMath::Clamp(ImpactSpeed / 360.0f, 0.25f, 1.0f);
		TriggerImpactEffect(EffectLocation, EffectStrength);
	}

	if (OtherDiceIndex == INDEX_NONE)
	{
		return;
	}
	if (mCollisionBoostsUsed >= DiceCollisionBoostMaxCount)
	{
		return;
	}
	if (mRollElapsed - mLastCollisionBoostTime < DiceCollisionBoostCooldownSeconds)
	{
		return;
	}
	if (ClosingSpeed < DiceCollisionBoostMinClosingSpeed && NormalImpulse.Size() <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FVector WorldUp = GetActorTransform().TransformVectorNoScale(FVector::UpVector).GetSafeNormal();
	FVector TangentDirection = FVector::CrossProduct(WorldUp, SeparationDirection).GetSafeNormal();
	if (TangentDirection.IsNearlyZero())
	{
		TangentDirection = FVector::RightVector;
	}

	const float BoostScale = (mCollisionBoostsUsed == 0) ? 1.0f : 0.72f;
	const FVector HitImpulse = SeparationDirection * (DiceCollisionBoostLinearImpulse * BoostScale)
		+ WorldUp * (DiceCollisionBoostLiftImpulse * BoostScale);
	const FVector OtherImpulse = -SeparationDirection * (DiceCollisionBoostLinearImpulse * BoostScale)
		+ WorldUp * (DiceCollisionBoostLiftImpulse * BoostScale);
	const float SpinSign = ((HitDiceIndex + OtherDiceIndex + mCollisionBoostsUsed) % 2 == 0) ? 1.0f : -1.0f;
	const FVector HitAngularImpulse = (TangentDirection + SeparationDirection * 0.45f).GetSafeNormal()
		* (DiceCollisionBoostAngularImpulse * BoostScale * SpinSign);
	const FVector OtherAngularImpulse = (-TangentDirection - SeparationDirection * 0.45f).GetSafeNormal()
		* (DiceCollisionBoostAngularImpulse * BoostScale * SpinSign);

	HitBody->AddImpulse(HitImpulse, NAME_None, true);
	OtherBody->AddImpulse(OtherImpulse, NAME_None, true);
	HitBody->AddAngularImpulseInDegrees(HitAngularImpulse, NAME_None, true);
	OtherBody->AddAngularImpulseInDegrees(OtherAngularImpulse, NAME_None, true);
	HitBody->WakeAllRigidBodies();
	OtherBody->WakeAllRigidBodies();

	mLastCollisionBoostTime = mRollElapsed;
	++mCollisionBoostsUsed;
	mRollStillElapsed = 0.0f;
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
	for (int32 DiceIndex = 0; DiceIndex < mPhysicsDiceBodies.Num(); ++DiceIndex)
	{
		UProceduralMeshComponent* PhysicsBody = mPhysicsDiceBodies[DiceIndex];
		ConstrainDiceToTable(PhysicsBody);
		ApplyRollTimeDamping(PhysicsBody, DeltaSeconds);
		ApplyCoinRollStabilization(PhysicsBody, DiceIndex, DeltaSeconds);
	}
	SyncVisualDiceToPhysics();
	UpdateImpactEffects(DeltaSeconds);
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
		for (int32 DiceIndex = 0; DiceIndex < mPhysicsDiceBodies.Num(); ++DiceIndex)
		{
			UProceduralMeshComponent* PhysicsBody = mPhysicsDiceBodies[DiceIndex];
			if (PhysicsBody != nullptr)
			{
				if (mPhysicsFaceCounts.IsValidIndex(DiceIndex) && mPhysicsFaceCounts[DiceIndex] == 2)
				{
					PhysicsBody->SetWorldLocationAndRotation(
						PhysicsBody->GetComponentLocation(),
						ComputeAlignedFaceUpQuat(DiceIndex).Rotator(),
						false,
						nullptr,
						ETeleportType::TeleportPhysics);
				}
				PhysicsBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
				PhysicsBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
				PhysicsBody->SetSimulatePhysics(false);
			}
		}
		SyncVisualDiceToPhysics();
		UpdateImpactEffects(DeltaSeconds);
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
	const float MaxSafeZ = TableFloorZ + PhysicsDiceRadius + 96.0f;
	ClampToVisibleDiceBoard(VisualClampRadius, LocalLocation, LocalVelocity, bClamped);

	if (LocalLocation.Z < SafeZ)
	{
		LocalLocation.Z = SafeZ;
		// 바닥 충돌은 리듬만 살리고, 위로 에너지가 누적되어 판 밖으로 튀는 상황은 막는다.
		LocalVelocity.Z = (LocalVelocity.Z < -40.0f)
			? -LocalVelocity.Z * 0.38f + 4.0f
			: FMath::Max(0.0f, LocalVelocity.Z) * 0.18f;
		LocalVelocity.X *= 0.88f;
		LocalVelocity.Y *= 0.88f;
		bClamped = true;
	}
	else if (LocalLocation.Z > MaxSafeZ)
	{
		LocalLocation.Z = MaxSafeZ;
		LocalVelocity.Z = FMath::Min(0.0f, LocalVelocity.Z) - 18.0f;
		LocalVelocity.X *= 0.74f;
		LocalVelocity.Y *= 0.74f;
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

void ACombatDiceRollCaptureActor::ApplyRollTimeDamping(UProceduralMeshComponent* PhysicsBody, float DeltaSeconds) const
{
	if (PhysicsBody == nullptr || PhysicsBody->IsSimulatingPhysics() == false || DeltaSeconds <= 0.0f)
	{
		return;
	}

	const float RawAlpha = FMath::Clamp(
		(mRollElapsed - RollEaseDampingStartSeconds) / FMath::Max(KINDA_SMALL_NUMBER, RollEaseDampingFullSeconds - RollEaseDampingStartSeconds),
		0.0f,
		1.0f);
	if (RawAlpha <= 0.0f)
	{
		return;
	}

	const float Alpha = RawAlpha * RawAlpha * (3.0f - 2.0f * RawAlpha);
	const float LinearDecay = FMath::Exp(-FMath::Lerp(RollEaseLinearDampingMin, RollEaseLinearDampingMax, Alpha) * DeltaSeconds);
	const float AngularDecay = FMath::Exp(-FMath::Lerp(RollEaseAngularDampingMin, RollEaseAngularDampingMax, Alpha) * DeltaSeconds);
	const float MaxLinearSpeed = FMath::Lerp(RollEaseLinearSpeedStart, RollEaseLinearSpeedEnd, Alpha);
	const float MaxAngularSpeed = FMath::Lerp(RollEaseAngularSpeedStart, RollEaseAngularSpeedEnd, Alpha);

	FVector LinearVelocity = PhysicsBody->GetPhysicsLinearVelocity() * LinearDecay;
	if (LinearVelocity.SizeSquared() > FMath::Square(MaxLinearSpeed))
	{
		LinearVelocity = LinearVelocity.GetClampedToMaxSize(MaxLinearSpeed);
	}

	FVector AngularVelocity = PhysicsBody->GetPhysicsAngularVelocityInDegrees() * AngularDecay;
	if (AngularVelocity.SizeSquared() > FMath::Square(MaxAngularSpeed))
	{
		AngularVelocity = AngularVelocity.GetClampedToMaxSize(MaxAngularSpeed);
	}

	PhysicsBody->SetPhysicsLinearVelocity(LinearVelocity);
	PhysicsBody->SetPhysicsAngularVelocityInDegrees(AngularVelocity);
}

void ACombatDiceRollCaptureActor::ApplyCoinRollStabilization(UProceduralMeshComponent* PhysicsBody, int32 DiceIndex, float DeltaSeconds) const
{
	if (PhysicsBody == nullptr
		|| PhysicsBody->IsSimulatingPhysics() == false
		|| DeltaSeconds <= 0.0f
		|| mPhysicsFaceCounts.IsValidIndex(DiceIndex) == false
		|| mPhysicsFaceCounts[DiceIndex] != 2)
	{
		return;
	}

	const RDDicePolyhedron::FDicePolyhedron& Poly = RDDicePolyhedron::Get(2);
	if (Poly.GetValueFaceCount() <= 0)
	{
		return;
	}

	const FTransform ActorTransform = GetActorTransform();
	const FVector WorldUp = ActorTransform.TransformVectorNoScale(FVector::UpVector).GetSafeNormal();
	const FQuat CurrentRotation = PhysicsBody->GetComponentQuat();
	const FVector CoinAxis = CurrentRotation.RotateVector(Poly.GetValueFace(0).mNormal).GetSafeNormal();
	const float AxisDotUp = FVector::DotProduct(CoinAxis, WorldUp);
	const float Flatness = FMath::Abs(AxisDotUp);
	const FVector LinearVelocity = PhysicsBody->GetPhysicsLinearVelocity();
	const FVector PlanarVelocity = LinearVelocity - WorldUp * FVector::DotProduct(LinearVelocity, WorldUp);
	const float PlanarSpeed = PlanarVelocity.Size();
	FVector AngularVelocity = PhysicsBody->GetPhysicsAngularVelocityInDegrees();

	if (PlanarSpeed > CoinRollAssistMinLinearSpeed)
	{
		const FVector TravelDirection = PlanarVelocity / PlanarSpeed;
		const FVector RollAxis = FVector::CrossProduct(TravelDirection, WorldUp).GetSafeNormal();
		if (RollAxis.IsNearlyZero() == false)
		{
			const float TargetRollSpeed = FMath::RadiansToDegrees(PlanarSpeed / FMath::Max(1.0f, PhysicsDiceRadius))
				* CoinRollVisualSpinMultiplier;
			const float CurrentRollSpeed = FVector::DotProduct(AngularVelocity, RollAxis);
			const float AssistAlpha = FMath::Clamp(DeltaSeconds * CoinRollAssistBlendSpeed, 0.0f, 1.0f);
			AngularVelocity += RollAxis * ((TargetRollSpeed - CurrentRollSpeed) * AssistAlpha);
		}
	}

	if (mRollElapsed >= CoinSettleAssistStartSeconds
		&& PlanarSpeed <= CoinSettleLinearSpeedMax
		&& AngularVelocity.Size() <= CoinSettleAngularSpeedMax
		&& Flatness < CoinSettleFlatDotMin)
	{
		const FVector TargetAxis = AxisDotUp < 0.0f ? -WorldUp : WorldUp;
		const FVector TipAxis = FVector::CrossProduct(CoinAxis, TargetAxis).GetSafeNormal();
		if (TipAxis.IsNearlyZero() == false)
		{
			const float EdgeAlpha = FMath::Clamp((CoinSettleFlatDotMin - Flatness) / CoinSettleFlatDotMin, 0.0f, 1.0f);
			const float YawSpin = FVector::DotProduct(AngularVelocity, WorldUp);
			const float YawDampingAlpha = FMath::Clamp(DeltaSeconds * CoinSettleYawDampingSpeed * EdgeAlpha, 0.0f, 1.0f);
			AngularVelocity += TipAxis * CoinSettleTipAngularSpeed * EdgeAlpha * DeltaSeconds;
			AngularVelocity -= WorldUp * YawSpin * YawDampingAlpha;
		}
	}

	if (AngularVelocity.SizeSquared() > FMath::Square(RollMaxAngularVelocity))
	{
		AngularVelocity = AngularVelocity.GetClampedToMaxSize(RollMaxAngularVelocity);
	}
	PhysicsBody->SetPhysicsAngularVelocityInDegrees(AngularVelocity);
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

	UpdateImpactEffects(DeltaSeconds);
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
