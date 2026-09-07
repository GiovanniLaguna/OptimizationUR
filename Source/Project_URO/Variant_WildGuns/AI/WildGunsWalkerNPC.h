// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AI/WildGunsEnemyBase.h"
#include "WildGunsWalkerNPC.generated.h"

UENUM(BlueprintType)
enum class EWalkerState : uint8
{
	WalkingIn,
	AimingAndShooting,
	WalkingOut
};

/**
 * Enemigo Background tipo Walker.
 * Camina lateralmente, se detiene a disparar al jugador y continúa su marcha saliendo de pantalla.
 */
UCLASS()
class PROJECT_URO_API AWildGunsWalkerNPC : public AWildGunsEnemyBase
{
	GENERATED_BODY()

public:
	AWildGunsWalkerNPC();

	virtual void Tick(float DeltaTime) override;

	virtual void OnActivatedFromPool_Implementation() override;

	/** Configura la dirección de patrulla (1.0 = derecha, -1.0 = izquierda) */
	UFUNCTION(BlueprintCallable, Category = "Walker")
	void SetupWalkDirection(float DirectionY);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Walker")
	float MoveSpeed = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Walker")
	float TimeBeforeShooting = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Walker")
	float AimAndShootDuration = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Walker")
	float WalkOutDuration = 4.0f;

	UFUNCTION(BlueprintImplementableEvent, Category = "Walker")
	void BP_OnStartShooting();

protected:
	EWalkerState CurrentState = EWalkerState::WalkingIn;
	float StateTimer = 0.0f;
	float WalkDirection = 1.0f;
	bool bHasFiredInState = false;

	void ExecuteShot();
};
