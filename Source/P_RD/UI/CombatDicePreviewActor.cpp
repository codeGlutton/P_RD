#include "UI/CombatDicePreviewActor.h"

#include "Engine/StaticMesh.h"
#include "UI/CombatDicePreviewActorPrivate.h"
#include "UObject/ConstructorHelpers.h"

ACombatDicePreviewActor::ACombatDicePreviewActor()
{
	PrimaryActorTick.bCanEverTick = false;

	InitializeSceneComponents();
	InitializeLighting();

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* CubeMesh = CubeMeshFinder.Succeeded() ? CubeMeshFinder.Object : nullptr;
	ApplyCubeMesh(CubeMesh);

	LoadDiceNumberFont();
	InitializeMaterials();
	InitializeEdgeMeshes(CubeMesh);
	InitializeFaceTexts();
	SetFaceValues({ 1, 2, 3, 4, 5, 6 });
}

FRotator ACombatDicePreviewActor::GetSettledFaceRotation(int32 FaceValue)
{
	return RDCombatDicePreview::MakeFaceToCameraRotation(FaceValue);
}
