// Copyright Epic Games, Inc. All Rights Reserved.

#include "WildGunsWalkerNPC.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Variant_WildGuns/WildGunsGameMode.h"

AWildGunsWalkerNPC::AWildGunsWalkerNPC()
{
	PrimaryActorTick.bCanEverTick = true;

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	}
}

void AWildGunsWalkerNPC::OnActivatedFromPool_Implementation()
{
	Super::OnActivatedFromPool_Implementation();

	CurrentState = EWalkerState::WalkingIn;
	StateTimer = 0.0f;
	bHasFiredInState = false;

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
	}
}

void AWildGunsWalkerNPC::SetupWalkDirection(float DirectionY)
{
	WalkDirection = (DirectionY >= 0.0f) ? 1.0f : -1.0f;
}

void AWildGunsWalkerNPC::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsDead)
	{
		return;
	}

	StateTimer += DeltaTime;

	switch (CurrentState)
	{
	case EWalkerState::WalkingIn:
		AddMovementInput(FVector(0.0f, WalkDirection, 0.0f), 1.0f);
		if (StateTimer >= TimeBeforeShooting)
		{
			CurrentState = EWalkerState::AimingAndShooting;
			StateTimer = 0.0f;
			bHasFiredInState = false;
			BP_OnStartShooting();
		}
		break;

	case EWalkerState::AimingAndShooting:
		// Se detiene a disparar
		if (!bHasFiredInState && StateTimer >= (AimAndShootDuration * 0.4f))
		{
			bHasFiredInState = true;
			ExecuteShot();
		}

		if (StateTimer >= AimAndShootDuration)
		{
			CurrentState = EWalkerState::WalkingOut;
			StateTimer = 0.0f;
		}
		break;

	case EWalkerState::WalkingOut:
		// Continúa su marcha para salir de la pantalla
		AddMovementInput(FVector(0.0f, WalkDirection, 0.0f), 1.0f);
		if (StateTimer >= WalkOutDuration)
		{
			ReturnToPool();
		}
		break;
	}
}

void AWildGunsWalkerNPC::ExecuteShot()
{
	AGameModeBase* GM = UGameplayStatics::GetGameMode(this);
	if (AWildGunsGameMode* WGGM = Cast<AWildGunsGameMode>(GM))
	{
		FVector MuzzleLoc = GetActorLocation() + FVector(0.0f, 0.0f, 40.0f) + (GetActorForwardVector() * 30.0f);
		ShootTowardsPlayer(WGGM->GetEnemyProjectilePool(), MuzzleLoc, 2400.0f);
	}
}
