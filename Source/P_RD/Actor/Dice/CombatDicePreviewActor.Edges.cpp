#include "Actor/Dice/CombatDicePreviewActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

void ACombatDicePreviewActor::InitializeEdgeMeshes(UStaticMesh* CubeMesh)
{
	constexpr float EdgePosition = 54.0f;
	const FVector WideEdgeScale(0.014f, 1.08f, 0.014f);
	const FVector TallEdgeScale(0.014f, 0.014f, 1.08f);

	/*
	 * 모든 모서리를 다 그리면 뒤쪽 모서리까지 보이면서 검은 배경이 "어두운 면"처럼 읽힌다.
	 * 정면 윤곽만 얇게 보강해 큐브 형태는 살리고, 빈 와이어프레임 면은 만들지 않는다.
	 */
	AddEdgeMesh(TEXT("Edge_Y_NegX_PosZ"), CubeMesh, FVector(-EdgePosition, 0.0f, EdgePosition), WideEdgeScale);
	AddEdgeMesh(TEXT("Edge_Y_NegX_NegZ"), CubeMesh, FVector(-EdgePosition, 0.0f, -EdgePosition), WideEdgeScale);
	AddEdgeMesh(TEXT("Edge_Z_NegX_PosY"), CubeMesh, FVector(-EdgePosition, EdgePosition, 0.0f), TallEdgeScale);
	AddEdgeMesh(TEXT("Edge_Z_NegX_NegY"), CubeMesh, FVector(-EdgePosition, -EdgePosition, 0.0f), TallEdgeScale);
}

void ACombatDicePreviewActor::AddEdgeMesh(const TCHAR* Name, UStaticMesh* CubeMesh, const FVector& RelativeLocation, const FVector& RelativeScale)
{
	UStaticMeshComponent* EdgeMesh = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	EdgeMesh->SetupAttachment(mDiceRoot);
	EdgeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EdgeMesh->SetCastShadow(false);
	EdgeMesh->SetRelativeLocation(RelativeLocation);
	EdgeMesh->SetRelativeScale3D(RelativeScale);
	if (CubeMesh != nullptr)
	{
		EdgeMesh->SetStaticMesh(CubeMesh);
	}
	if (mEdgeMaterial != nullptr)
	{
		EdgeMesh->SetMaterial(0, mEdgeMaterial);
	}

	mEdgeMeshes.Add(EdgeMesh);
}
