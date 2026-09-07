// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WildGunsDestructibleCover.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCoverDestroyed, AWildGunsDestructibleCover*, DestroyedCover);

class UStaticMeshComponent;

/**
 * Cobertura destructible para Wild Guns.
 * Los enemigos Cover se ocultan detrás de este actor.
 * Al recibir suficientes disparos del jugador se rompe, dejando expuesto al enemigo.
 */
UCLASS()
class PROJECT_URO_API AWildGunsDestructibleCover : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;

public:
	AWildGunsDestructibleCover();

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	UPROPERTY(BlueprintAssignable, Category = "Cover")
	FOnCoverDestroyed OnCoverDestroyed;

	/** Salud máxima de la cobertura */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover")
	float MaxHealth = 5.0f;

	/** Salud actual */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	float CurrentHealth = 5.0f;

	/** Si es true, la cobertura ya fue destruida */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	bool bIsDestroyed = false;

	/** Restablece la cobertura para una nueva partida */
	UFUNCTION(BlueprintCallable, Category = "Cover")
	void ResetCover();

	/** Evento en BP para efectos visuales o partículas al recibir impacto */
	UFUNCTION(BlueprintImplementableEvent, Category = "Cover")
	void BP_OnDamaged(float HealthRemaining, float TotalHealth);

	/** Evento en BP al destruirse la cobertura (humo, escombros, sonido) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Cover")
	void BP_OnDestroyed();

	/** Efecto de sonido al destruirse la cobertura */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* SoundBreak;

protected:
	virtual void BeginPlay() override;
};
