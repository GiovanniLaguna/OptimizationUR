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
	FollowCamera->SetFieldOfView(BaseCameraFOV);
	FollowCamera->PostProcessSettings.bOverride_VignetteIntensity = true;
	FollowCamera->PostProcessSettings.VignetteIntensity = 0.4f;
	FollowCamera->PostProcessSettings.bOverride_SceneFringeIntensity = true;
	FollowCamera->PostProcessSettings.SceneFringeIntensity = 0.0f;

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

	BaseSocketOffset = CameraBoom ? CameraBoom->SocketOffset : FVector(0.0f, 0.0f, 60.0f);
	BaseBoomRotation = CameraBoom ? CameraBoom->GetRelativeRotation() : FRotator(-6.0f, 0.0f, 0.0f);
	CurrentCameraFOV = BaseCameraFOV;
	DamageVignetteAmount = 0.4f;
	ChromaticAberrationAmount = 0.0f;

	OnWeaponChanged.Broadcast(CurrentWeapon, ShotgunAmmo);
	OnLivesChanged.Broadcast(Lives);
}

void AWildGunsCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Mantener siempre la vista orientada hacia el fondo de la galería
	SetActorRotation(FRotator(0.0f, 0.0f, 0.0f));

	// Actualizar efectos cinemáticos de cámara (trauma, paralaje de apuntado, FOV y postprocesado)
	UpdateCameraEffects(DeltaTime);

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

		// Retroceso y sacudida de cámara por disparo de escopeta
		AddCameraTrauma(0.38f);
		AddFOVKick(-3.5f);

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
		AddCameraTrauma(0.06f);

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

	// Sacudida cinemática al golpear cuerpo a cuerpo
	AddCameraTrauma(bHitAnyForeground ? 0.55f : 0.25f);
	AddFOVKick(-2.5f);

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

	// Impacto contundente de cámara, retroceso elástico de FOV y viñeta roja / distorsión cromática
	AddCameraTrauma(0.85f);
	AddFOVKick(5.0f);
	TriggerDamageCameraEffects();

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

void AWildGunsCharacter::AddCameraTrauma(float Amount)
{
	CameraTrauma = FMath::Clamp(CameraTrauma + Amount, 0.0f, 1.0f);
}

void AWildGunsCharacter::AddFOVKick(float FOVDelta)
{
	CurrentCameraFOV = FMath::Clamp(CurrentCameraFOV + FOVDelta, 70.0f, 120.0f);
}

void AWildGunsCharacter::TriggerDamageCameraEffects()
{
	DamageVignetteAmount = 1.0f;
	ChromaticAberrationAmount = 3.5f;
}

void AWildGunsCharacter::UpdateCameraEffects(float DeltaTime)
{
	if (!CameraBoom || !FollowCamera)
	{
		return;
	}

	// 1. Decaimiento continuo del trauma de sacudida
	if (CameraTrauma > 0.0f)
	{
		CameraTrauma = FMath::Max(0.0f, CameraTrauma - (TraumaDecayRate * DeltaTime));
	}

	// 2. Cálculo de sacudida procedural (trauma al cuadrado para respuesta no lineal orgánica)
	const float ShakeIntensity = CameraTrauma * CameraTrauma;
	ShakeTimeCounter += DeltaTime;

	float ShakePitch = 0.0f;
	float ShakeYaw = 0.0f;
	float ShakeRoll = 0.0f;
	float ShakeOffsetX = 0.0f;
	float ShakeOffsetY = 0.0f;
	float ShakeOffsetZ = 0.0f;

	if (ShakeIntensity > 0.0001f)
	{
		ShakePitch = FMath::Sin(ShakeTimeCounter * 34.0f) * 1.8f * ShakeIntensity;
		ShakeYaw = FMath::Cos(ShakeTimeCounter * 27.0f) * 1.4f * ShakeIntensity;
		ShakeRoll = FMath::Sin(ShakeTimeCounter * 41.0f) * 2.2f * ShakeIntensity;

		ShakeOffsetX = FMath::Cos(ShakeTimeCounter * 29.0f) * 15.0f * ShakeIntensity;
		ShakeOffsetY = FMath::Sin(ShakeTimeCounter * 31.0f) * 22.0f * ShakeIntensity;
		ShakeOffsetZ = FMath::Sin(ShakeTimeCounter * 37.0f) * 18.0f * ShakeIntensity;
	}

	// 3. Paralaje dinámico / Aim Lead según la posición de la retícula en pantalla
	float ParallaxPitch = 0.0f;
	float ParallaxYaw = 0.0f;
	float ParallaxOffsetY = 0.0f;

	if (AWildGunsPlayerController* WGPC = Cast<AWildGunsPlayerController>(GetController()))
	{
		int32 SizeX = 1920;
		int32 SizeY = 1080;
		WGPC->GetViewportSize(SizeX, SizeY);

		if (SizeX > 0 && SizeY > 0)
		{
			const FVector2D ReticlePos = WGPC->GetReticleScreenPosition();
			const float NormX = FMath::Clamp(((ReticlePos.X / (float)SizeX) - 0.5f) * 2.0f, -1.0f, 1.0f);
			const float NormY = FMath::Clamp(((ReticlePos.Y / (float)SizeY) - 0.5f) * 2.0f, -1.0f, 1.0f);

			ParallaxYaw = NormX * 4.5f * AimParallaxIntensity;
			ParallaxPitch = -NormY * 2.5f * AimParallaxIntensity;
			ParallaxOffsetY = NormX * 70.0f * AimParallaxIntensity;
		}
	}

	// 4. Aplicar rotación y desplazamiento del SpringArm con resorte amortiguado (VInterpTo / RInterpTo)
	const FRotator TargetBoomRot = FRotator(
		BaseBoomRotation.Pitch + ParallaxPitch + ShakePitch,
		BaseBoomRotation.Yaw + ParallaxYaw + ShakeYaw,
		BaseBoomRotation.Roll + ShakeRoll
	);

	const FVector TargetSocketOffset = FVector(
		BaseSocketOffset.X + ShakeOffsetX,
		BaseSocketOffset.Y + ParallaxOffsetY + ShakeOffsetY,
		BaseSocketOffset.Z + ShakeOffsetZ
	);

	CameraBoom->SetRelativeRotation(FMath::RInterpTo(CameraBoom->GetRelativeRotation(), TargetBoomRot, DeltaTime, 14.0f));
	CameraBoom->SocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, TargetSocketOffset, DeltaTime, 14.0f);

	// 5. Dynamic FOV Punch (resorte elástico suave hacia BaseCameraFOV)
	CurrentCameraFOV = FMath::FInterpTo(CurrentCameraFOV, BaseCameraFOV, DeltaTime, 8.0f);
	FollowCamera->SetFieldOfView(CurrentCameraFOV);

	// 6. Post-Process (viñeta de daño y aberración cromática disipándose suavemente)
	DamageVignetteAmount = FMath::FInterpTo(DamageVignetteAmount, 0.4f, DeltaTime, 3.5f);
	ChromaticAberrationAmount = FMath::FInterpTo(ChromaticAberrationAmount, 0.0f, DeltaTime, 4.5f);

	FollowCamera->PostProcessSettings.VignetteIntensity = DamageVignetteAmount;
	FollowCamera->PostProcessSettings.SceneFringeIntensity = ChromaticAberrationAmount;
}
