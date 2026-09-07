// Copyright Epic Games, Inc. All Rights Reserved.

#include "WildGunsPickup.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Pooling/ActorPool.h"
#include "Pooling/ActorUtilities.h"
#include "Variant_WildGuns/WildGunsCharacter.h"

AWildGunsPickup::AWildGunsPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(45.0f);
	CollisionSphere->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CollisionSphere->SetCanEverAffectNavigation(false);
	RootComponent = CollisionSphere;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> BoxFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (BoxFinder.Succeeded() && Mesh)
	{
		Mesh->SetStaticMesh(BoxFinder.Object);
		Mesh->SetRelativeScale3D(FVector(0.45f, 0.45f, 0.45f));
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GlowMat(TEXT("/Game/Variant_TwinStick/M_Glow.M_Glow"));
	if (GlowMat.Succeeded() && Mesh)
	{
		Mesh->SetMaterial(0, GlowMat.Object);
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> SndPickup(TEXT("/Game/Variant_WildGuns/Audio/SFX_Powerup_Chime.SFX_Powerup_Chime"));
	if (SndPickup.Succeeded()) SoundChime = SndPickup.Object;
}

void AWildGunsPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	RunningTime += DeltaTime;
	// Efecto de rotación y suave flotación arcade
	AddActorLocalRotation(FRotator(0.0f, 90.0f * DeltaTime, 0.0f));

	FVector CurrentLoc = GetActorLocation();
	CurrentLoc.Z = InitialSpawnLocation.Z + FMath::Sin(RunningTime * 3.0f) * 10.0f;
	SetActorLocation(CurrentLoc);
}

void AWildGunsPickup::OnActivatedFromPool_Implementation()
{
	bCollected = false;
	RunningTime = 0.0f;
	InitialSpawnLocation = GetActorLocation();

	if (CollisionSphere)
	{
		CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}

	SetActorTickEnabled(true);

	GetWorldTimerManager().ClearTimer(LifeTimerHandle);
	GetWorldTimerManager().SetTimer(LifeTimerHandle, this, &AWildGunsPickup::ReturnToPool, LifeTime, false);
}

void AWildGunsPickup::OnReturnedToPool_Implementation()
{
	GetWorldTimerManager().ClearTimer(LifeTimerHandle);
	SetActorTickEnabled(false);

	if (CollisionSphere)
	{
		CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AWildGunsPickup::ReturnToPool()
{
	GetWorldTimerManager().ClearTimer(LifeTimerHandle);

	if (OwningPool)
	{
		OwningPool->ReturnActorToPool(this);
	}
	else
	{
		UActorUtilities::ToggleActorHidden(this, true);
	}
}

float AWildGunsPickup::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	if (bCollected)
	{
		return 0.0f;
	}

	bCollected = true;

	// Entregar el arma al jugador
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (AWildGunsCharacter* WGChar = Cast<AWildGunsCharacter>(PlayerPawn))
	{
		WGChar->EquipShotgun(GrantedAmmo);
	}

	if (SoundChime)
	{
		UGameplayStatics::PlaySound2D(this, SoundChime);
	}

	BP_OnCollected(GetActorLocation());
	ReturnToPool();

	return DamageAmount;
}
