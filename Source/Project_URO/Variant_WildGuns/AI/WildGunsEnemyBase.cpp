// Copyright Epic Games, Inc. All Rights Reserved.

#include "WildGunsEnemyBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Pooling/ActorPool.h"
#include "Pooling/ActorUtilities.h"
#include "Variant_WildGuns/Gameplay/WildGunsProjectile.h"
#include "Variant_WildGuns/WildGunsGameMode.h"

AWildGunsEnemyBase::AWildGunsEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);
	GetCapsuleComponent()->SetCanEverAffectNavigation(false);

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->GravityScale = 1.0f;
		GetCharacterMovement()->MaxWalkSpeed = 300.0f;
		GetCharacterMovement()->bRunPhysicsWithNoController = true;
	}

	// 1. Malla 3D del Maniquí con jerarquía esquelética completa para locomoción
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannyFinder(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (MannyFinder.Succeeded() && GetMesh())
	{
		GetMesh()->SetSkeletalMesh(MannyFinder.Object);
		GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
		GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	}

	if (GetMesh())
	{
		GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);

		static ConstructorHelpers::FClassFinder<UAnimInstance> AnimFinder(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C"));
		if (AnimFinder.Succeeded())
		{
			GetMesh()->SetAnimInstanceClass(AnimFinder.Class);
		}
	}

	// 2. Blueprint de destrucción visual (BP_TwinStickNPCDestruction)
	static ConstructorHelpers::FClassFinder<AActor> ProxyFinder(TEXT("/Game/Variant_TwinStick/Blueprints/AI/BP_TwinStickNPCDestruction.BP_TwinStickNPCDestruction_C"));
	if (ProxyFinder.Succeeded())
	{
		DestructionProxyClass = ProxyFinder.Class;
	}

	// 3. Efectos de sonido
	static ConstructorHelpers::FObjectFinder<USoundBase> SndEnemy(TEXT("/Game/Variant_WildGuns/Audio/SFX_Enemy_Shot.SFX_Enemy_Shot"));
	if (SndEnemy.Succeeded()) SoundEnemyShot = SndEnemy.Object;

	static ConstructorHelpers::FObjectFinder<USoundBase> SndDeathFinder(TEXT("/Game/Variant_WildGuns/Audio/SFX_Explosion_AoE.SFX_Explosion_AoE"));
	if (SndDeathFinder.Succeeded()) SoundDeath = SndDeathFinder.Object;
}

void AWildGunsEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	// Asegurar en tiempo de ejecución que el enemigo tiene la malla y el Blueprint de animación de locomoción
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		USkeletalMesh* CurrentMesh = MeshComp->GetSkeletalMeshAsset();
		if (!CurrentMesh || CurrentMesh->GetName().Contains(TEXT("Meshy")))
		{
			USkeletalMesh* MannyMesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
			if (MannyMesh)
			{
				MeshComp->SetSkeletalMeshAsset(MannyMesh);
				MeshComp->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
				MeshComp->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
			}
		}

		if (!MeshComp->GetAnimClass())
		{
			UClass* AnimClass = LoadClass<UAnimInstance>(nullptr, TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C"));
			if (AnimClass)
			{
				MeshComp->SetAnimInstanceClass(AnimClass);
			}
		}
	}
}

void AWildGunsEnemyBase::OnActivatedFromPool_Implementation()
{
	bIsDead = false;
	CurrentHealth = MaxHealth;

	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);

	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}

	if (GetMesh())
	{
		GetMesh()->SetVisibility(true);
		GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);
	}

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->Activate(true);
	}

	SetActorTickEnabled(true);
}

void AWildGunsEnemyBase::OnReturnedToPool_Implementation()
{
	GetWorldTimerManager().ClearTimer(DeathTimerHandle);
	SetActorTickEnabled(false);

	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->Deactivate();
	}
}

void AWildGunsEnemyBase::ReturnToPool()
{
	GetWorldTimerManager().ClearTimer(DeathTimerHandle);

	if (OwningPool)
	{
		OwningPool->ReturnActorToPool(this);
	}
	else
	{
		UActorUtilities::ToggleActorHidden(this, true);
	}
}

float AWildGunsEnemyBase::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead)
	{
		return 0.0f;
	}

	CurrentHealth -= DamageAmount;
	BP_OnDamaged(CurrentHealth);

	if (CurrentHealth <= 0.0f)
	{
		HandleDeath();
	}

	return DamageAmount;
}

void AWildGunsEnemyBase::HandleDeath()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;

	// Detener movimiento y colisión de inmediato
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->Deactivate();
	}
	SetActorEnableCollision(false);

	// 1. Instanciar Proxy de Destrucción (VFX de fractura/explosión de BP_TwinStickNPCDestruction)
	if (DestructionProxyClass && GetWorld())
	{
		GetWorld()->SpawnActor<AActor>(DestructionProxyClass, GetActorTransform());
	}

	// 2. Sonido de impacto mortal / explosión
	if (SoundDeath)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SoundDeath, GetActorLocation());
	}

	// 3. Ocultar inmediatamente el actor original para que la muerte sea visualmente instantánea
	SetActorHiddenInGame(true);

	// 4. Probabilidad de soltar el Powerup de Escopeta desde el Pool central (25%)
	if (FMath::RandRange(1, 100) <= ShotgunDropChance)
	{
		AGameModeBase* GM = UGameplayStatics::GetGameMode(this);
		if (AWildGunsGameMode* WGGM = Cast<AWildGunsGameMode>(GM))
		{
			if (UActorPool* PickupPool = WGGM->GetPickupPool())
			{
				PickupPool->GetActorFromPool(GetActorLocation(), FRotator::ZeroRotator);
			}
		}
	}

	BP_OnDied();

	// 5. Devolver al pool con un breve retraso para garantizar la limpieza adecuada
	GetWorldTimerManager().SetTimer(DeathTimerHandle, this, &AWildGunsEnemyBase::ReturnToPool, 0.2f, false);
}

void AWildGunsEnemyBase::ShootTowardsPlayer(UActorPool* ProjectilePool, const FVector& MuzzleLocation, float BulletSpeed)
{
	if (!ProjectilePool)
	{
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn)
	{
		return;
	}

	FVector TargetLocation = PlayerPawn->GetActorLocation() + FVector(0.0f, 0.0f, 40.0f);
	FVector AimDirection = (TargetLocation - MuzzleLocation).GetSafeNormal();

	AActor* PooledActor = ProjectilePool->GetActorFromPool(MuzzleLocation, AimDirection.Rotation());
	if (AWildGunsProjectile* Projectile = Cast<AWildGunsProjectile>(PooledActor))
	{
		Projectile->OwningPool = ProjectilePool;
		Projectile->bIsEnemyProjectile = true;
		Projectile->bIsExplosiveShotgun = false;
		Projectile->SetVelocity(AimDirection, BulletSpeed);

		if (SoundEnemyShot)
		{
			UGameplayStatics::PlaySoundAtLocation(this, SoundEnemyShot, MuzzleLocation);
		}
	}
}
