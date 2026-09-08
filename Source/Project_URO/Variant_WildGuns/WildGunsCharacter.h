// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WildGunsCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UActorPool;
class AWildGunsProjectile;

UENUM(BlueprintType)
enum class EWildGunsWeapon : uint8
{
	MachineGun,
	Shotgun
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponChangedSignature, EWildGunsWeapon, NewWeapon, int32, CurrentAmmo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLivesChangedSignature, int32, RemainingLives);

/**
 * Personaje jugable para Wild Guns.
 * Movimiento horizontal estricto (Strafe), doble salto, inmovilización al disparar en suelo,
 * metralleta default infinita, escopeta con munición limitada y AoE, ataque Melee, 1 HP y 3 vidas.
 */
UCLASS()
class PROJECT_URO_API AWildGunsCharacter : public ACharacter
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

public:
	AWildGunsCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	// --- Movimiento ---
	/** Movimiento horizontal estricto (Strafe lateral) */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void MoveRight(float Value);

	/** Salto y Doble Salto */
	virtual void Jump() override;

	// --- Combate y Disparo ---
	/** Comienza a disparar */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StartFiring();

	/** Detiene el disparo */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StopFiring();

	/** Ataque cuerpo a cuerpo (Melee) contra enemigos Foreground */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void PerformMeleeAttack();

	/** Equipa la escopeta con la munición otorgada por el powerup */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void EquipShotgun(int32 AmmoCount = 50);

	/** Obtiene el arma actualmente equipada */
	UFUNCTION(BlueprintPure, Category = "Combat")
	EWildGunsWeapon GetCurrentWeapon() const { return CurrentWeapon; }

	/** Obtiene la munición restante del arma equipada */
	UFUNCTION(BlueprintPure, Category = "Combat")
	int32 GetCurrentAmmo() const { return ShotgunAmmo; }

	/** Obtiene las vidas restantes */
	UFUNCTION(BlueprintPure, Category = "Stats")
	int32 GetLives() const { return Lives; }

	// --- Eventos / Delegados ---
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnWeaponChangedSignature OnWeaponChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnLivesChangedSignature OnLivesChanged;

	// --- Eventos en Blueprint para Animaciones / SFX / VFX ---
	UFUNCTION(BlueprintImplementableEvent, Category = "WildGuns|Events")
	void BP_OnShotFired(EWildGunsWeapon WeaponType, const FVector& MuzzleLocation, const FVector& TargetLocation);

	UFUNCTION(BlueprintImplementableEvent, Category = "WildGuns|Events")
	void BP_OnMeleeAttack(const FVector& HitLocation, bool bHitForegroundEnemy);

	UFUNCTION(BlueprintImplementableEvent, Category = "WildGuns|Events")
	void BP_OnLifeLost(int32 LivesLeft);

	UFUNCTION(BlueprintImplementableEvent, Category = "WildGuns|Events")
	void BP_OnGameOver();

public:
	/** Vidas del jugador (inicia con 3) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 Lives = 3;

	/** Salud del jugador (siempre 1 HP) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxHP = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float CurrentHP = 1.0f;

	/** Cadencia de disparo de la metralleta */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float MachineGunFireRate = 0.12f;

	/** Cadencia de disparo de la escopeta (más lenta que la metralleta) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float ShotgunFireRate = 0.38f;

	/** Rango del ataque cuerpo a cuerpo */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float MeleeRange = 160.0f;

	/** Duración de la invulnerabilidad tras recibir daño */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float InvulnerabilityDuration = 2.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	bool bIsShooting = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	bool bIsInvulnerable = false;

	// Audio
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* SoundGunshotMG;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* SoundGunshotShotgun;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* SoundJump;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* SoundMelee;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* SoundHurt;

	// --- Efectos Cinemáticos de Cámara ---
	/** Añade trauma de sacudida a la cámara (se acumula hasta 1.0) */
	UFUNCTION(BlueprintCallable, Category = "WildGuns|Camera")
	void AddCameraTrauma(float Amount);

	/** Aplica un golpe elástico instantáneo de campo de visión (FOV Punch) */
	UFUNCTION(BlueprintCallable, Category = "WildGuns|Camera")
	void AddFOVKick(float FOVDelta);

	/** Dispara efectos visuales de daño (viñeta y aberración cromática) */
	UFUNCTION(BlueprintCallable, Category = "WildGuns|Camera")
	void TriggerDamageCameraEffects();

	/** Tasa a la que decae el trauma por segundo */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WildGuns|Camera")
	float TraumaDecayRate = 1.6f;

	/** FOV base de la cámara de juego */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WildGuns|Camera")
	float BaseCameraFOV = 90.0f;

	/** Intensidad del movimiento de paralaje al apuntar con la retícula */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WildGuns|Camera")
	float AimParallaxIntensity = 1.0f;

protected:
	EWildGunsWeapon CurrentWeapon = EWildGunsWeapon::MachineGun;
	int32 ShotgunAmmo = 0;
	float FireTimer = 0.0f;
	FTimerHandle InvulnerabilityTimerHandle;
	FTimerHandle ShootLoopTimer;

	// Variables internas del sistema de cámara dinámica
	float CameraTrauma = 0.0f;
	float CurrentCameraFOV = 90.0f;
	float DamageVignetteAmount = 0.0f;
	float ChromaticAberrationAmount = 0.0f;
	float ShakeTimeCounter = 0.0f;

	FVector BaseSocketOffset = FVector(0.0f, 0.0f, 60.0f);
	FRotator BaseBoomRotation = FRotator(-6.0f, 0.0f, 0.0f);

	void UpdateCameraEffects(float DeltaTime);
	void ExecuteShot();
	void EndInvulnerability();
	FVector GetAimTargetLocation() const;
};
