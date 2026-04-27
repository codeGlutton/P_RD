#include "Pawn/SRPGUnit.h"

ASRPGUnit::ASRPGUnit()
{
	PrimaryActorTick.bCanEverTick = true;

}

void ASRPGUnit::BeginPlay()
{
	Super::BeginPlay();
	
}

void ASRPGUnit::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ASRPGUnit::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void ASRPGUnit::SetGenericTeamId(const FGenericTeamId& TeamID)
{
	mTeamId = TeamID;
}

FGenericTeamId ASRPGUnit::GetGenericTeamId() const
{
	return mTeamId;
}

UAbilitySystemComponent* ASRPGUnit::GetAbilitySystemComponent() const
{
	return mAbilitySystemComp;
}