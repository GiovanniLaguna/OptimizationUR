// Copyright Epic Games, Inc. All Rights Reserved.

#include "WildGunsForegroundNPC.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Variant_WildGuns/WildGunsCharacter.h"

AWildGunsForegroundNPC::AWildGunsForegroundNPC()
{
	PrimaryActorTick.bCanEverTick = true;

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	}

	SetActorScale3D(FVector(1.15f, 1.15f, 1.15f));

	static ConstructorHelpers::FObjectFinder<USoundBase> SndRico(TEXT("/Game/Variant_WildGuns/Audio/SFX_Bullet_Ricochet.SFX_Bullet_Ricochet"));
	if (SndRico.Succeeded()) SoundRicochet = SndRico.Object;
}

void AWildGunsForegroundNPC::OnActivatedFromPool_Implementation()
{
	Super::OnActivatedFromPool_Implementation();

	AttackTimer = 0.0f;

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
	}
}

float AWildGunsForegroundNPC::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead)
	{
		return 0.0f;
	}

	// INMUNIDAD A DISPAROS: Solo acepta daño si es tipo Melee (UWildGunsMeleeDamageType)
	const bool bIsMeleeDamage = DamageEvent.DamageTypeClass && DamageEvent.DamageTypeClass->IsChildOf(UWildGunsMeleeDamageType::StaticClass());

	if (!bIsMeleeDamage)
	{
		// Los disparos de bala no le hacen daño alguno
		if (SoundRicochet)
		{
			UGameplayStatics::PlaySoundAtLocation(this, SoundRicochet, GetActorLocation());
		}

		BP_OnBulletImmune(GetActorLocation());
		return 0.0f;
	}

	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void AWildGunsForegroundNPC::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsDead)
	{
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn)
	{
		return;
	}

	FVector PlayerLoc = PlayerPawn->GetActorLocation();
	FVector CurrentLoc = GetActorLocation();

	// Mantener el mismo plano de profundidad X
	if (!FMath::IsNearlyEqual(CurrentLoc.X, PlayerLoc.X, 10.0f))
	{
		CurrentLoc.X = PlayerLoc.X;
		SetActorLocation(CurrentLoc);
	}

	float DistanceY = FMath::Abs(PlayerLoc.Y - CurrentLoc.Y);
	float DirectionY = (PlayerLoc.Y >= CurrentLoc.Y) ? 1.0f : -1.0f;

	AttackTimer += DeltaTime;

	if (DistanceY > AttackRange)
	{
		// Correr lateralmente hacia el jugador
		AddMovementInput(FVector(0.0f, DirectionY, 0.0f), 1.0f);
	}
	else
	{
		// En rango de golpe: ejecutar ataque
		if (AttackTimer >= AttackCooldown)
		{
			AttackTimer = 0.0f;
			PerformMeleeHit();
		}
	}
}

void AWildGunsForegroundNPC::PerformMeleeHit()
{
	BP_OnPunchAttack();

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (AWildGunsCharacter* WGChar = Cast<AWildGunsCharacter>(PlayerPawn))
	{
		float Distance = FVector::Dist2D(GetActorLocation(), WGChar->GetActorLocation());
		if (Distance <= (AttackRange + 30.0f))
		{
			UGameplayStatics::ApplyDamage(
				WGChar,
				AttackDamage,
				GetController(),
				this,
				UWildGunsMeleeDamageType::StaticClass()
			);
		}
	}
}
