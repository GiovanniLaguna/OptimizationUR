// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Pooling/PoolableActor.h"
#include "WildGunsEnemyBase.generated.h"

class UActorPool;
class AWildGunsProjectile;

/**
 * Clase base para todos los enemigos de Wild Guns con soporte de Object Pooling.
 */
UCLASS(Abstract)
class PROJECT_URO_API AWildGunsEnemyBase : public ACharacter, public IPoolableActor
{
	GENERATED_BODY()

public:
	AWildGunsEnemyBase();

	virtual void BeginPlay() override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	/** Salud máxima del enemigo */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Stats")
	float MaxHealth = 1.0f;

	/** Salud actual */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy Stats")
	float CurrentHealth = 1.0f;

	/** Probabilidad de soltar el Powerup de escopeta al morir (0 a 100%) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Drop", meta = (ClampMin = 0, ClampMax = 100))
	int32 ShotgunDropChance = 25;

	/** Pool al que pertenece este enemigo */
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Pooling")
	UActorPool* OwningPool;

	// IPoolableActor
	virtual void OnActivatedFromPool_Implementation() override;
	virtual void OnReturnedToPool_Implementation() override;

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	virtual void ReturnToPool();

	/** Evento en BP al recibir daño */
	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy")
	void BP_OnDamaged(float HealthRemaining);

	/** Evento en BP al morir (animación, efectos de sonido) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy")
	void BP_OnDied();

	/** Sonido del disparo de enemigo */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* SoundEnemyShot;

	/** Clase del proxy de destrucción que se genera al morir (ej. BP_TwinStickNPCDestruction) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy FX")
	TSubclassOf<AActor> DestructionProxyClass;

	/** Sonido de muerte/destrucción */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* SoundDeath;

protected:
	bool bIsDead = false;
	FTimerHandle DeathTimerHandle;

	/** Gestiona la muerte y la probabilidad de spawn del Powerup desde el Pool */
	virtual void HandleDeath();

	/** Dispara un proyectil del pool hacia la posición actual del jugador */
	UFUNCTION(BlueprintCallable, Category = "Enemy Combat")
	void ShootTowardsPlayer(UActorPool* ProjectilePool, const FVector& MuzzleLocation, float BulletSpeed = 2000.0f);
};
