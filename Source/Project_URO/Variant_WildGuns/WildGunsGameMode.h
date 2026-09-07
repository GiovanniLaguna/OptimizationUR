// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "WildGunsGameMode.generated.h"

class UActorPool;
class AWildGunsDestructibleCover;
class AWildGunsCharacter;

UENUM(BlueprintType)
enum class EWildGunsMatchState : uint8
{
	Playing,
	GameOver,
	Victory
};

/**
 * Game Mode para la réplica del Nivel 1 de Wild Guns.
 * Administra el cronómetro de 60 segundos, el flujo de fin de partida (Victoria/Game Over)
 * y contiene los Object Pools centrales para proyectiles, enemigos y powerups.
 */
UCLASS()
class PROJECT_URO_API AWildGunsGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWildGunsGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// --- Acceso a Pools Reutilizables ---
	UFUNCTION(BlueprintPure, Category = "Pooling")
	UActorPool* GetEnemyProjectilePool() const { return EnemyProjectilePool; }

	UFUNCTION(BlueprintPure, Category = "Pooling")
	UActorPool* GetShotgunProjectilePool() const { return ShotgunProjectilePool; }

	UFUNCTION(BlueprintPure, Category = "Pooling")
	UActorPool* GetWalkerPool() const { return WalkerPool; }

	UFUNCTION(BlueprintPure, Category = "Pooling")
	UActorPool* GetCoverNPCPool() const { return CoverNPCPool; }

	UFUNCTION(BlueprintPure, Category = "Pooling")
	UActorPool* GetForegroundNPCPool() const { return ForegroundNPCPool; }

	UFUNCTION(BlueprintPure, Category = "Pooling")
	UActorPool* GetPickupPool() const { return PickupPool; }

	UFUNCTION(BlueprintPure, Category = "Pooling")
	UActorPool* GetDecalPool() const { return DecalPool; }

	// --- Control de Flujo de Partida ---
	UFUNCTION(BlueprintCallable, Category = "WildGuns Flow")
	void HandleGameOver();

	UFUNCTION(BlueprintCallable, Category = "WildGuns Flow")
	void HandleVictory();

	UFUNCTION(BlueprintPure, Category = "WildGuns Flow")
	float GetMatchTimeRemaining() const { return MatchTimeRemaining; }

	UFUNCTION(BlueprintPure, Category = "WildGuns Flow")
	EWildGunsMatchState GetMatchState() const { return MatchState; }

protected:
	// Duración de la partida (60 segundos)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WildGuns Match")
	float MatchDuration = 60.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WildGuns Match")
	float MatchTimeRemaining = 60.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WildGuns Match")
	EWildGunsMatchState MatchState = EWildGunsMatchState::Playing;

	// Clases base para inicializar los pools
	UPROPERTY(EditAnywhere, Category = "Pooling|Classes")
	TSubclassOf<AActor> EnemyProjectileClass;

	UPROPERTY(EditAnywhere, Category = "Pooling|Classes")
	TSubclassOf<AActor> ShotgunProjectileClass;

	UPROPERTY(EditAnywhere, Category = "Pooling|Classes")
	TSubclassOf<AActor> WalkerNPCClass;

	UPROPERTY(EditAnywhere, Category = "Pooling|Classes")
	TSubclassOf<AActor> CoverNPCClass;

	UPROPERTY(EditAnywhere, Category = "Pooling|Classes")
	TSubclassOf<AActor> ForegroundNPCClass;

	UPROPERTY(EditAnywhere, Category = "Pooling|Classes")
	TSubclassOf<AActor> PickupClass;

	UPROPERTY(EditAnywhere, Category = "Pooling|Classes")
	TSubclassOf<AActor> DecalClass;

	// Componentes de Pool
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pooling|Pools")
	UActorPool* EnemyProjectilePool;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pooling|Pools")
	UActorPool* ShotgunProjectilePool;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pooling|Pools")
	UActorPool* WalkerPool;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pooling|Pools")
	UActorPool* CoverNPCPool;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pooling|Pools")
	UActorPool* ForegroundNPCPool;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pooling|Pools")
	UActorPool* PickupPool;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pooling|Pools")
	UActorPool* DecalPool;

	// Audio
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* BGMTheme;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* VictorySound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* GameOverSound;

	UPROPERTY()
	UAudioComponent* BGMAudioComponent;

protected:
	// Sistema de Generación (Spawners)
	FTimerHandle WalkerSpawnTimer;
	FTimerHandle CoverSpawnTimer;
	FTimerHandle ForegroundSpawnTimer;

	void SpawnWalkerFromPool();
	void SpawnCoverNPCFromPool();
	void SpawnForegroundNPCFromPool();

	TArray<AWildGunsDestructibleCover*> CachedCovers;
	void CacheCoversInLevel();
	void SetupGalleryEnvironment();

	void UpdateHUD();
};
