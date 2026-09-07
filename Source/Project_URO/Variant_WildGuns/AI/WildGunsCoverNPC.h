// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AI/WildGunsEnemyBase.h"
#include "WildGunsCoverNPC.generated.h"

class AWildGunsDestructibleCover;

UENUM(BlueprintType)
enum class ECoverNPCState : uint8
{
	Hiding,
	Peeking,
	Shooting
};

/**
 * Enemigo Background tipo Cover.
 * Se resguarda tras una cobertura, se asoma a disparar y vuelve a ocultarse.
 */
UCLASS()
class PROJECT_URO_API AWildGunsCoverNPC : public AWildGunsEnemyBase
{
	GENERATED_BODY()

public:
	AWildGunsCoverNPC();

	virtual void Tick(float DeltaTime) override;

	virtual void OnActivatedFromPool_Implementation() override;

	/** Asocia una cobertura a este enemigo */
	UFUNCTION(BlueprintCallable, Category = "Cover")
	void AssignCover(AWildGunsDestructibleCover* InCover);

	UFUNCTION()
	void OnCoverDestroyedHandler(AWildGunsDestructibleCover* DestroyedCover);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover")
	float HideDuration = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover")
	float PeekAndShootDuration = 1.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	bool bIsPeeking = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	bool bCoverDestroyed = false;

	/** Evento en BP para animar agacharse / cubrirse */
	UFUNCTION(BlueprintImplementableEvent, Category = "Cover")
	void BP_OnTakeCover();

	/** Evento en BP para animar asomarse y apuntar */
	UFUNCTION(BlueprintImplementableEvent, Category = "Cover")
	void BP_OnPeekOut();

protected:
	ECoverNPCState CurrentState = ECoverNPCState::Hiding;
	float StateTimer = 0.0f;
	bool bHasFiredInCycle = false;
	int32 CyclesCount = 0;
	int32 MaxCycles = 4;

	UPROPERTY()
	AWildGunsDestructibleCover* AssignedCover = nullptr;

	void ExecuteShot();
};
