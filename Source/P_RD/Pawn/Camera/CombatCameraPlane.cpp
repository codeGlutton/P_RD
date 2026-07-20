

#include "Pawn/Camera/CombatCameraPlane.h"

// Sets default values
ACombatCameraPlane::ACombatCameraPlane()
{
    PrimaryActorTick.bCanEverTick = false;

    mPlaneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaneMesh"));
    mPlaneMesh->SetRelativeScale3D(FVector(10000, 10000, 0));
    RootComponent = mPlaneMesh;

    // 기본 메시 에셋 로드 (엔진 기본 제공 Plane)
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMeshAsset(
        TEXT("/Engine/BasicShapes/Plane.Plane")
    );
    if (PlaneMeshAsset.Succeeded())
    {
        mPlaneMesh->SetStaticMesh(PlaneMeshAsset.Object);
    }

    // 평소엔 화면에 안 보이게, 콜리전도 꺼둠 (필요시 CameraMovementComponent에서 켬)
    mPlaneMesh->SetVisibility(false);

    // 1. 콜리전 활성화 방식
    mPlaneMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // 물리 시뮬레이션 없이 쿼리(트레이스)만

    // 2. Object Type 지정 (커스텀 채널)
    mPlaneMesh->SetCollisionObjectType(ECC_GameTraceChannel3); // CameraMove로 지정한 채널

    // 3. 모든 채널에 대한 기본 반응을 Ignore로 초기화 (깔끔하게 시작)
    mPlaneMesh->SetCollisionResponseToAllChannels(ECR_Ignore);

    // 4. CameraMove 채널에 대해서만 Block으로 오버라이드
    mPlaneMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Block);

    mPlaneMesh->SetVisibility(false);
}

// Called when the game starts or when spawned
void ACombatCameraPlane::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ACombatCameraPlane::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

