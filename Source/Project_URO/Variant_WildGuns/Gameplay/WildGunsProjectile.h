// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pooling/PoolableActor.h"
#include "WildGunsProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UActorPool;

/**
 * Proyectil reutilizable para Wild Guns gestionado mediante Object Pooling.
 * Utilizado para disparos de enemigos y disparos de escopeta del jugador.
 */
UCLASS()
class PROJECT_URO_API AWildGunsProjectile : public AActor, public IPoolableActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UProjectileMovementComponent* ProjectileMovement;

public:
	AWildGunsProjectile();

	virtual void NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

	/** Pool al que pertenece este proyectil */
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Pooling")
	UActorPool* OwningPool;

	// IPoolableActor
	virtual void OnActivatedFromPool_Implementation() override;
	virtual void OnReturnedToPool_Implementation() override;

	/** Devuelve este proyectil al pool */
	UFUNCTION(BlueprintCallable, Category = "Pooling")
	void ReturnToPool();

public:
	/** Si es true, el proyectil daña al jugador. Si es false, daña a enemigos y props */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	bool bIsEnemyProjectile = false;

	/** Si es true, activa una explosión en área (AoE) al impactar (Escopeta) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	bool bIsExplosiveShotgun = false;

	/** Radio de la explosión AoE de la escopeta */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile", meta = (ClampMin = 10.0f))
	float ExplosionRadius = 250.0f;

	/** Cantidad de daño */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float Damage = 1.0f;

	/** Tiempo de vida antes de regresar automáticamente al pool */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float MaxLifeSpan = 4.0f;

	/** Velocidad del proyectil */
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SetVelocity(const FVector& Direction, float Speed);

	/** Efecto visual/evento en BP al impactar o explotar */
	UFUNCTION(BlueprintImplementableEvent, Category = "Projectile")
	void BP_OnExploded(const FVector& Location, bool bWasExplosive);

	/** Efecto de sonido de explosión AoE */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* SoundExplosion;

protected:
	FTimerHandle LifeSpanTimerHandle;

	/** Material de decal a estampar en la superficie de impacto */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decals")
	TObjectPtr<UMaterialInterface> DecalMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decals")
	FVector DecalSize = FVector(32.0f, 16.0f, 16.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decals")
	float DecalLifeSpan = 5.0f;

	bool bImpactProcessed = false;
};
