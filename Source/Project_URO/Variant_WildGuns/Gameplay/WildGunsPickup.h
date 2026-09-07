// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pooling/PoolableActor.h"
#include "WildGunsPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UActorPool;

/**
 * Powerup de Escopeta para Wild Guns.
 * Se recolecta DISPARÁNDOLE con el arma del jugador.
 * Implementa IPoolableActor para gestión de memoria optimizada.
 */
UCLASS()
class PROJECT_URO_API AWildGunsPickup : public AActor, public IPoolableActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;

public:
	AWildGunsPickup();

	virtual void Tick(float DeltaTime) override;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	/** Munición que otorga este powerup de escopeta */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	int32 GrantedAmmo = 50;

	/** Tiempo de vida en la escena antes de desaparecer y volver al pool */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	float LifeTime = 15.0f;

	/** Pool al que pertenece este pickup */
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Pooling")
	UActorPool* OwningPool;

	// IPoolableActor
	virtual void OnActivatedFromPool_Implementation() override;
	virtual void OnReturnedToPool_Implementation() override;

	UFUNCTION(BlueprintCallable, Category = "Pickup")
	void ReturnToPool();

	/** Evento en BP al ser recolectado por un disparo */
	UFUNCTION(BlueprintImplementableEvent, Category = "Pickup")
	void BP_OnCollected(const FVector& Location);

	/** Efecto de sonido al ser recolectado */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* SoundChime;

protected:
	FTimerHandle LifeTimerHandle;
	bool bCollected = false;
	FVector InitialSpawnLocation;
	float RunningTime = 0.0f;
};
