// Copyright Epic Games, Inc. All Rights Reserved.

#include "WildGunsDestructibleCover.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

AWildGunsDestructibleCover::AWildGunsDestructibleCover()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	RootComponent = Mesh;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded() && Mesh)
	{
		Mesh->SetStaticMesh(CubeFinder.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WoodMat(TEXT("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_Gray_02.MI_PrototypeGrid_Gray_02"));
	if (WoodMat.Succeeded() && Mesh)
	{
		Mesh->SetMaterial(0, WoodMat.Object);
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> SndCoverBreak(TEXT("/Game/Variant_WildGuns/Audio/SFX_Cover_Break.SFX_Cover_Break"));
	if (SndCoverBreak.Succeeded()) SoundBreak = SndCoverBreak.Object;
}

void AWildGunsDestructibleCover::BeginPlay()
{
	Super::BeginPlay();
	ResetCover();
}

void AWildGunsDestructibleCover::ResetCover()
{
	bIsDestroyed = false;
	CurrentHealth = MaxHealth;

	if (Mesh)
	{
		Mesh->SetVisibility(true);
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
}

float AWildGunsDestructibleCover::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDestroyed)
	{
		return 0.0f;
	}

	CurrentHealth -= DamageAmount;
	BP_OnDamaged(CurrentHealth, MaxHealth);

	if (CurrentHealth <= 0.0f)
	{
		bIsDestroyed = true;
		CurrentHealth = 0.0f;

		if (Mesh)
		{
			Mesh->SetVisibility(false);
			Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}

		if (SoundBreak)
		{
			UGameplayStatics::PlaySoundAtLocation(this, SoundBreak, GetActorLocation());
		}

		BP_OnDestroyed();
		OnCoverDestroyed.Broadcast(this);
	}

	return DamageAmount;
}
