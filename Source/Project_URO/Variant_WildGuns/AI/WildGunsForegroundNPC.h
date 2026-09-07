// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AI/WildGunsEnemyBase.h"
#include "GameFramework/DamageType.h"
#include "WildGunsForegroundNPC.generated.h"

/** Tipo de daño exclusivo para ataques cuerpo a cuerpo (Melee) */
UCLASS()
class PROJECT_URO_API UWildGunsMeleeDamageType : public UDamageType
{
	GENERATED_BODY()
};

/**
 * Enemigo Foreground.
 * Aparece en el primer plano a la misma profundidad del jugador y corre hacia él.
 * ES INMUNE A LOS DISPAROS DE BALA.
 * Solo puede ser derrotado con el ataque cuerpo a cuerpo (Melee).
 */
UCLASS()
class PROJECT_URO_API AWildGunsForegroundNPC : public AWildGunsEnemyBase
{
	GENERATED_BODY()

public:
	AWildGunsForegroundNPC();

	virtual void Tick(float DeltaTime) override;

	virtual void OnActivatedFromPool_Implementation() override;

	/**
	 * Filtra el daño: Solo acepta daño si proviene de un ataque Melee (UWildGunsMeleeDamageType o flag Melee).
	 * Las balas convencionales no causan ningún daño.
	 */
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	/** Daño infligido al jugador al golpearlo */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foreground Combat")
	float AttackDamage = 1.0f;

	/** Distancia de golpe cuerpo a cuerpo */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foreground Combat")
	float AttackRange = 130.0f;

	/** Enfriamiento entre ataques */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foreground Combat")
	float AttackCooldown = 1.5f;

	/** Velocidad al perseguir al jugador */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foreground Combat")
	float RunSpeed = 340.0f;

	/** Evento en BP cuando un disparo rebota por ser inmune */
	UFUNCTION(BlueprintImplementableEvent, Category = "Foreground Combat")
	void BP_OnBulletImmune(const FVector& HitLocation);

	/** Efecto de sonido de rebote metálico al impactar una bala (inmune) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* SoundRicochet;

	/** Evento en BP al lanzar golpe al jugador */
	UFUNCTION(BlueprintImplementableEvent, Category = "Foreground Combat")
	void BP_OnPunchAttack();

protected:
	float AttackTimer = 0.0f;
	void PerformMeleeHit();
};
