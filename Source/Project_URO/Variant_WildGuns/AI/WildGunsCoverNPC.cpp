// Copyright Epic Games, Inc. All Rights Reserved.

#include "WildGunsCoverNPC.h"
#include "Kismet/GameplayStatics.h"
#include "Variant_WildGuns/Gameplay/WildGunsDestructibleCover.h"
#include "Variant_WildGuns/WildGunsGameMode.h"

AWildGunsCoverNPC::AWildGunsCoverNPC()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> QuinnFinder(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
	if (QuinnFinder.Succeeded() && GetMesh())
	{
		GetMesh()->SetSkeletalMesh(QuinnFinder.Object);
		GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
		GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	}
}

void AWildGunsCoverNPC::OnActivatedFromPool_Implementation()
{
	Super::OnActivatedFromPool_Implementation();

	CurrentState = ECoverNPCState::Hiding;
	StateTimer = 0.0f;
	bIsPeeking = false;
	bCoverDestroyed = false;
	bHasFiredInCycle = false;
	CyclesCount = 0;

	BP_OnTakeCover();
}

void AWildGunsCoverNPC::AssignCover(AWildGunsDestructibleCover* InCover)
{
	AssignedCover = InCover;
	if (AssignedCover)
	{
		AssignedCover->OnCoverDestroyed.AddDynamic(this, &AWildGunsCoverNPC::OnCoverDestroyedHandler);
		bCoverDestroyed = AssignedCover->bIsDestroyed;
	}
}

void AWildGunsCoverNPC::OnCoverDestroyedHandler(AWildGunsDestructibleCover* DestroyedCover)
{
	bCoverDestroyed = true;
	bIsPeeking = true;
	BP_OnPeekOut();
}

void AWildGunsCoverNPC::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsDead)
	{
		return;
	}

	StateTimer += DeltaTime;

	if (bCoverDestroyed)
	{
		// Sin cobertura: permanece expuesto disparando periódicamente
		bIsPeeking = true;
		if (StateTimer >= PeekAndShootDuration)
		{
			StateTimer = 0.0f;
			ExecuteShot();
		}
		return;
	}

	switch (CurrentState)
	{
	case ECoverNPCState::Hiding:
		if (StateTimer >= HideDuration)
		{
			CurrentState = ECoverNPCState::Peeking;
			StateTimer = 0.0f;
			bIsPeeking = true;
			bHasFiredInCycle = false;
			BP_OnPeekOut();
		}
		break;

	case ECoverNPCState::Peeking:
		if (!bHasFiredInCycle && StateTimer >= (PeekAndShootDuration * 0.4f))
		{
			bHasFiredInCycle = true;
			ExecuteShot();
		}

		if (StateTimer >= PeekAndShootDuration)
		{
			CyclesCount++;
			if (CyclesCount >= MaxCycles)
			{
				ReturnToPool();
				return;
			}

			CurrentState = ECoverNPCState::Hiding;
			StateTimer = 0.0f;
			bIsPeeking = false;
			BP_OnTakeCover();
		}
		break;

	default:
		break;
	}
}

void AWildGunsCoverNPC::ExecuteShot()
{
	AGameModeBase* GM = UGameplayStatics::GetGameMode(this);
	if (AWildGunsGameMode* WGGM = Cast<AWildGunsGameMode>(GM))
	{
		FVector MuzzleLoc = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f) + (GetActorForwardVector() * 30.0f);
		ShootTowardsPlayer(WGGM->GetEnemyProjectilePool(), MuzzleLoc, 2400.0f);
	}
}
