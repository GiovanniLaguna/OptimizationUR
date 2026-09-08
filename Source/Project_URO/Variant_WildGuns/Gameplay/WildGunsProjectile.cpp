// Copyright Epic Games, Inc. All Rights Reserved.

#include "WildGunsProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Pooling/ActorPool.h"
#include "Pooling/ActorUtilities.h"
#include "Pooling/PooledDecalActor.h"
#include "Variant_WildGuns/WildGunsCharacter.h"

AWildGunsProjectile::AWildGunsProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(18.0f);
	CollisionSphere->SetCollisionProfileName(TEXT("Projectile"));
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	CollisionSphere->SetNotifyRigidBodyCollision(true);
	RootComponent = CollisionSphere;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereFinder.Succeeded() && Mesh)
	{
		Mesh->SetStaticMesh(SphereFinder.Object);
		Mesh->SetRelativeScale3D(FVector(0.18f, 0.18f, 0.35f));
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GlowMat(TEXT("/Game/Variant_TwinStick/M_Glow.M_Glow"));
	if (GlowMat.Succeeded() && Mesh)
	{
		Mesh->SetMaterial(0, GlowMat.Object);
	}

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionSphere;
	ProjectileMovement->InitialSpeed = 3000.0f;
	ProjectileMovement->MaxSpeed = 3000.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.0f;

	static ConstructorHelpers::FObjectFinder<USoundBase> SndExplode(TEXT("/Game/Variant_WildGuns/Audio/SFX_Explosion_AoE.SFX_Explosion_AoE"));
	if (SndExplode.Succeeded()) SoundExplosion = SndExplode.Object;
}

void AWildGunsProjectile::OnActivatedFromPool_Implementation()
{
	bImpactProcessed = false;

	if (CollisionSphere)
	{
		CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}

	if (ProjectileMovement)
	{
		ProjectileMovement->Activate(true);
		ProjectileMovement->Velocity = GetActorForwardVector() * ProjectileMovement->InitialSpeed;
	}

	GetWorldTimerManager().ClearTimer(LifeSpanTimerHandle);
	GetWorldTimerManager().SetTimer(LifeSpanTimerHandle, this, &AWildGunsProjectile::ReturnToPool, MaxLifeSpan, false);
}

void AWildGunsProjectile::OnReturnedToPool_Implementation()
{
	GetWorldTimerManager().ClearTimer(LifeSpanTimerHandle);

	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
	}

	if (CollisionSphere)
	{
		CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AWildGunsProjectile::SetVelocity(const FVector& Direction, float Speed)
{
	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = Speed;
		ProjectileMovement->MaxSpeed = Speed;
		ProjectileMovement->Velocity = Direction.GetSafeNormal() * Speed;
	}
	SetActorRotation(Direction.Rotation());
}

void AWildGunsProjectile::ReturnToPool()
{
	GetWorldTimerManager().ClearTimer(LifeSpanTimerHandle);

	if (OwningPool)
	{
		OwningPool->ReturnActorToPool(this);
	}
	else
	{
		UActorUtilities::ToggleActorHidden(this, true);
	}
}

void AWildGunsProjectile::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

	if (bImpactProcessed || !Other || Other == this)
	{
		return;
	}

	bImpactProcessed = true;

	if (bIsExplosiveShotgun)
	{
		// Daño en Área (AoE)
		UGameplayStatics::ApplyRadialDamage(
			this,
			Damage,
			HitLocation,
			ExplosionRadius,
			UDamageType::StaticClass(),
			TArray<AActor*>(),
			this,
			nullptr,
			false,
			ECC_Visibility
		);

		if (SoundExplosion)
		{
			UGameplayStatics::PlaySoundAtLocation(this, SoundExplosion, HitLocation);
		}

		// Sacudida cinemática de cámara por onda expansiva
		if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			if (AWildGunsCharacter* WGChar = Cast<AWildGunsCharacter>(PlayerPawn))
			{
				const float Dist = FVector::Dist(PlayerPawn->GetActorLocation(), HitLocation);
				const float ShockwaveTrauma = FMath::Clamp(1.0f - (Dist / 3000.0f), 0.15f, 0.45f);
				WGChar->AddCameraTrauma(ShockwaveTrauma);
			}
		}

		BP_OnExploded(HitLocation, true);
	}
	else
	{
		// Daño puntual
		UGameplayStatics::ApplyPointDamage(
			Other,
			Damage,
			GetVelocity().GetSafeNormal(),
			Hit,
			nullptr,
			this,
			UDamageType::StaticClass()
		);

		BP_OnExploded(HitLocation, false);
	}

	ReturnToPool();
}
