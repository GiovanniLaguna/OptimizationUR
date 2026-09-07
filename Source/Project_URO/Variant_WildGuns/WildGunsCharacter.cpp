// Copyright Epic Games, Inc. All Rights Reserved.

#include "WildGunsCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Pooling/ActorPool.h"
#include "Pooling/PooledDecalActor.h"
#include "Variant_WildGuns/AI/WildGunsForegroundNPC.h"
#include "Variant_WildGuns/Gameplay/WildGunsProjectile.h"
#include "Variant_WildGuns/WildGunsGameMode.h"
#include "Variant_WildGuns/WildGunsPlayerController.h"

AWildGunsCharacter::AWildGunsCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Salto y Doble Salto
	JumpMaxCount = 2;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bOrientRotationToMovement = false;
		GetCharacterMovement()->MaxWalkSpeed = 600.0f;
		GetCharacterMovement()->JumpZVelocity = 750.0f;
		GetCharacterMovement()->AirControl = 0.8f;
		GetCharacterMovement()->GravityScale = 1.6f;
	}

	// Configuración de Cámara estilo Arcade Wild Guns
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 550.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 0.0f, 60.0f);
	CameraBoom->SetRelativeRotation(FRotator(-6.0f, 0.0f, 0.0f));
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Cargar automáticamente el modelo 3D de Wild Guns para el jugador
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> WGMeshFinder(TEXT("/Game/Characters/Player/SKM_Meshy_AI_Wild_Guns_Idle_Pose_C_0804014729_texture.SKM_Meshy_AI_Wild_Guns_Idle_Pose_C_0804014729_texture"));
	if (WGMeshFinder.Succeeded() && GetMesh())
	{
		GetMesh()->SetSkeletalMesh(WGMeshFinder.Object);
		GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
		GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	}

	// Cargar automáticamente efectos de sonido
	static ConstructorHelpers::FObjectFinder<USoundBase> SndMG(TEXT("/Game/Variant_WildGuns/Audio/SFX_Gunshot_MG.SFX_Gunshot_MG"));
	if (SndMG.Succeeded()) SoundGunshotMG = SndMG.Object;

	static ConstructorHelpers::FObjectFinder<USoundBase> SndSG(TEXT("/Game/Variant_WildGuns/Audio/SFX_Gunshot_Shotgun.SFX_Gunshot_Shotgun"));
	if (SndSG.Succeeded()) SoundGunshotShotgun = SndSG.Object;

	static ConstructorHelpers::FObjectFinder<USoundBase> SndJmp(TEXT("/Game/Variant_WildGuns/Audio/SFX_Jump.SFX_Jump"));
	if (SndJmp.Succeeded()) SoundJump = SndJmp.Object;

	static ConstructorHelpers::FObjectFinder<USoundBase> SndMel(TEXT("/Game/Variant_WildGuns/Audio/SFX_Melee_Punch.SFX_Melee_Punch"));
	if (SndMel.Succeeded()) SoundMelee = SndMel.Object;

	static ConstructorHelpers::FObjectFinder<USoundBase> SndHrt(TEXT("/Game/Variant_WildGuns/Audio/SFX_Player_Hurt.SFX_Player_Hurt"));
	if (SndHrt.Succeeded()) SoundHurt = SndHrt.Object;
}

void AWildGunsCharacter::BeginPlay()
{
	Super::BeginPlay();

	Lives = 3;
	CurrentHP = 1.0f;
	CurrentWeapon = EWildGunsWeapon::MachineGun;
	ShotgunAmmo = 0;
	bIsShooting = false;
	bIsInvulnerable = false;

	OnWeaponChanged.Broadcast(CurrentWeapon, ShotgunAmmo);
	OnLivesChanged.Broadcast(Lives);
}

void AWildGunsCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Mantener siempre la vista orientada hacia el fondo de la galería
	SetActorRotation(FRotator(0.0f, 0.0f, 0.0f));

	// Si el jugador está en el aire, no puede disparar
	if (bIsShooting && !GetCharacterMovement()->IsMovingOnGround())
	{
		StopFiring();
	}
}

void AWildGunsCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AWildGunsCharacter::MoveRight(float Value)
{
	// Si está disparando, no se puede mover ("mientras se dispara, no se puede mover")
	if (bIsShooting || FMath::IsNearlyZero(Value))
	{
		return;
	}

	// Strafe exclusivamente lateral
	AddMovementInput(FVector(0.0f, 1.0f, 0.0f), Value);
}

void AWildGunsCharacter::Jump()
{
	// Si está disparando, primero se cancela el disparo para poder saltar
	if (bIsShooting)
	{
		StopFiring();
	}

	if (SoundJump)
	{
		UGameplayStatics::PlaySound2D(this, SoundJump);
	}

	Super::Jump();
}

void AWildGunsCharacter::StartFiring()
{
	// "solamente puede dispararse si el jugador está en el piso (no saltando)"
	if (!GetCharacterMovement() || !GetCharacterMovement()->IsMovingOnGround())
	{
		return;
	}

	if (bIsShooting)
	{
		return;
	}

	bIsShooting = true;
	GetCharacterMovement()->StopMovementImmediately();

	// Ejecutar primer disparo de inmediato
	ExecuteShot();

	// Configurar ciclo continuo de disparo según cadencia del arma actual
	const float FireRate = (CurrentWeapon == EWildGunsWeapon::Shotgun) ? ShotgunFireRate : MachineGunFireRate;
	GetWorldTimerManager().SetTimer(ShootLoopTimer, this, &AWildGunsCharacter::ExecuteShot, FireRate, true);
}

void AWildGunsCharacter::StopFiring()
{
	bIsShooting = false;
	GetWorldTimerManager().ClearTimer(ShootLoopTimer);
}

void AWildGunsCharacter::EquipShotgun(int32 AmmoCount)
{
	CurrentWeapon = EWildGunsWeapon::Shotgun;
	ShotgunAmmo = AmmoCount;

	OnWeaponChanged.Broadcast(CurrentWeapon, ShotgunAmmo);

	// Si estaba disparando, reiniciar temporizador con la cadencia de la escopeta
	if (bIsShooting)
	{
		GetWorldTimerManager().ClearTimer(ShootLoopTimer);
		GetWorldTimerManager().SetTimer(ShootLoopTimer, this, &AWildGunsCharacter::ExecuteShot, ShotgunFireRate, true);
	}
}

FVector AWildGunsCharacter::GetAimTargetLocation() const
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (AWildGunsPlayerController* WGPC = Cast<AWildGunsPlayerController>(PC))
	{
		return WGPC->GetReticleWorldLocation();
	}

	// Fallback hacia el frente
	return GetActorLocation() + FVector(2000.0f, 0.0f, 40.0f);
}

void AWildGunsCharacter::ExecuteShot()
{
	if (!GetCharacterMovement() || !GetCharacterMovement()->IsMovingOnGround())
	{
		StopFiring();
		return;
	}

	const FVector MuzzleLoc = GetActorLocation() + FVector(40.0f, 15.0f, 30.0f);
	const FVector TargetLoc = GetAimTargetLocation();
	const FVector ShotDir = (TargetLoc - MuzzleLoc).GetSafeNormal();

	AGameModeBase* GM = UGameplayStatics::GetGameMode(this);
	AWildGunsGameMode* WGGM = Cast<AWildGunsGameMode>(GM);

	if (CurrentWeapon == EWildGunsWeapon::Shotgun)
	{
		// Consumir 1 bala de escopeta
		ShotgunAmmo = FMath::Max(0, ShotgunAmmo - 1);

		// Disparo a base de Proyectil con AoE desde el Pool
		if (WGGM && WGGM->GetShotgunProjectilePool())
		{
			AActor* PooledActor = WGGM->GetShotgunProjectilePool()->GetActorFromPool(MuzzleLoc, ShotDir.Rotation());
			if (AWildGunsProjectile* Projectile = Cast<AWildGunsProjectile>(PooledActor))
			{
				Projectile->OwningPool = WGGM->GetShotgunProjectilePool();
				Projectile->bIsEnemyProjectile = false;
				Projectile->bIsExplosiveShotgun = true;
				Projectile->ExplosionRadius = 260.0f;
				Projectile->Damage = 2.0f;
				Projectile->SetVelocity(ShotDir, 3500.0f);
			}
		}

		if (SoundGunshotShotgun)
		{
			UGameplayStatics::PlaySound2D(this, SoundGunshotShotgun);
		}

		BP_OnShotFired(EWildGunsWeapon::Shotgun, MuzzleLoc, TargetLoc);

		// Si se acaba la munición, revertir a la metralleta default
		if (ShotgunAmmo <= 0)
		{
			CurrentWeapon = EWildGunsWeapon::MachineGun;
			OnWeaponChanged.Broadcast(CurrentWeapon, 0);

			if (bIsShooting)
			{
				GetWorldTimerManager().ClearTimer(ShootLoopTimer);
				GetWorldTimerManager().SetTimer(ShootLoopTimer, this, &AWildGunsCharacter::ExecuteShot, MachineGunFireRate, true);
			}
		}
		else
		{
			OnWeaponChanged.Broadcast(CurrentWeapon, ShotgunAmmo);
		}
	}
	else
	{
		// Metralleta Default: Balas infinitas, disparo de alta cadencia con registro de impacto arcade
		FHitResult HitResult;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);

		const FVector TraceEnd = MuzzleLoc + (ShotDir * 6000.0f);
		FCollisionShape BulletSphere = FCollisionShape::MakeSphere(25.0f);

		// 1. Barrido con radio de 25cm por Visibility
		bool bHit = GetWorld()->SweepSingleByChannel(HitResult, MuzzleLoc, TraceEnd, FQuat::Identity, ECC_Visibility, BulletSphere, QueryParams);
		if (!bHit)
		{
			// 2. Línea directa por Visibility
			bHit = GetWorld()->LineTraceSingleByChannel(HitResult, MuzzleLoc, TraceEnd, ECC_Visibility, QueryParams);
		}
		if (!bHit)
		{
			// 3. Respaldo por Pawn para máxima fiabilidad
			bHit = GetWorld()->SweepSingleByChannel(HitResult, MuzzleLoc, TraceEnd, FQuat::Identity, ECC_Pawn, BulletSphere, QueryParams);
		}

		if (bHit && HitResult.GetActor())
		{
			// Infligir daño a actores, enemigos, coberturas y powerups
			UGameplayStatics::ApplyPointDamage(
				HitResult.GetActor(),
				1.0f,
				ShotDir,
				HitResult,
				GetController(),
				this,
				UDamageType::StaticClass()
			);

			// Decal de impacto mediante el pool de decals
			if (WGGM && WGGM->GetDecalPool())
			{
				FRotator DecalRot = HitResult.ImpactNormal.Rotation();
				DecalRot.Pitch += 180.0f;
				WGGM->GetDecalPool()->GetActorFromPool(HitResult.ImpactPoint, DecalRot);
			}
		}

		if (SoundGunshotMG)
		{
			UGameplayStatics::PlaySound2D(this, SoundGunshotMG);
		}

		BP_OnShotFired(EWildGunsWeapon::MachineGun, MuzzleLoc, bHit ? HitResult.ImpactPoint : TraceEnd);
	}
}

void AWildGunsCharacter::PerformMeleeAttack()
{
	const FVector StartLoc = GetActorLocation();
	const FVector Forward = GetActorForwardVector();

	// Barrido esférico de 260cm en el plano del jugador para impactar a los ForegroundNPCs que llegan por los laterales
	TArray<FHitResult> HitResults;
	FCollisionShape SweepSphere = FCollisionShape::MakeSphere(260.0f);
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	bool bHitAnyForeground = false;
	FVector MeleeHitLocation = StartLoc + (Forward * 80.0f);

	bool bHit = GetWorld()->SweepMultiByChannel(
		HitResults,
		StartLoc,
		StartLoc + (Forward * 40.0f),
		FQuat::Identity,
		ECC_Pawn,
		SweepSphere,
		Params
	);

	if (bHit)
	{
		for (const FHitResult& Hit : HitResults)
		{
			if (AWildGunsForegroundNPC* FGEnemy = Cast<AWildGunsForegroundNPC>(Hit.GetActor()))
			{
				bHitAnyForeground = true;
				MeleeHitLocation = Hit.ImpactPoint;

				// Infligir daño Melee para derrotarlo
				UGameplayStatics::ApplyDamage(
					FGEnemy,
					10.0f,
					GetController(),
					this,
					UWildGunsMeleeDamageType::StaticClass()
				);
			}
		}
	}

	if (SoundMelee)
	{
		UGameplayStatics::PlaySound2D(this, SoundMelee);
	}

	BP_OnMeleeAttack(MeleeHitLocation, bHitAnyForeground);
}

float AWildGunsCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsInvulnerable || Lives <= 0)
	{
		return 0.0f;
	}

	Lives--;
	OnLivesChanged.Broadcast(Lives);
	BP_OnLifeLost(Lives);

	if (SoundHurt)
	{
		UGameplayStatics::PlaySound2D(this, SoundHurt);
	}

	if (Lives <= 0)
	{
		bIsShooting = false;
		GetWorldTimerManager().ClearTimer(ShootLoopTimer);
		BP_OnGameOver();

		AGameModeBase* GM = UGameplayStatics::GetGameMode(this);
		if (AWildGunsGameMode* WGGM = Cast<AWildGunsGameMode>(GM))
		{
			WGGM->HandleGameOver();
		}
	}
	else
	{
		// Invulnerabilidad temporal tras recibir daño (parpadeo)
		bIsInvulnerable = true;
		GetWorldTimerManager().SetTimer(InvulnerabilityTimerHandle, this, &AWildGunsCharacter::EndInvulnerability, InvulnerabilityDuration, false);
	}

	return DamageAmount;
}

void AWildGunsCharacter::EndInvulnerability()
{
	bIsInvulnerable = false;
}
