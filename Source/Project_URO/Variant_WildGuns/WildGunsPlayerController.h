// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "WildGunsPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class AWildGunsCharacter;

/**
 * Player Controller para Wild Guns.
 * Administra el movimiento de la retícula de apuntado, trazado al mundo tridimensional,
 * control de Pausa y transiciones de fundido de pantalla.
 */
UCLASS()
class PROJECT_URO_API AWildGunsPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AWildGunsPlayerController();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupInputComponent() override;

	/** Proyecta la posición de la retícula al espacio 3D para apuntar */
	UFUNCTION(BlueprintPure, Category = "WildGuns Aiming")
	FVector GetReticleWorldLocation() const;

	/** Posición actual en pantalla de la retícula */
	UFUNCTION(BlueprintPure, Category = "WildGuns Aiming")
	FVector2D GetReticleScreenPosition() const { return ReticleScreenPos; }

	/** Alterna el estado de pausa del juego */
	UFUNCTION(BlueprintCallable, Category = "WildGuns Flow")
	void TogglePause();

	UFUNCTION(BlueprintPure, Category = "WildGuns Flow")
	bool IsGamePaused() const { return bIsGamePaused; }

	/** Ejecuta una transición suave de fundido de pantalla (Camera Fade) */
	UFUNCTION(BlueprintCallable, Category = "WildGuns Flow")
	void PlayCameraFade(bool bFadeOut, float Duration = 1.0f);

	/** Reinicia o carga el nivel principal */
	UFUNCTION(BlueprintCallable, Category = "WildGuns Flow")
	void RestartLevelGame();

	/** Regresa al Menú Principal */
	UFUNCTION(BlueprintCallable, Category = "WildGuns Flow")
	void ReturnToMainMenu();

protected:
	UPROPERTY(EditAnywhere, Category = "Input|Mapping")
	TArray<UInputMappingContext*> InputMappings;

	UPROPERTY(EditAnywhere, Category = "Input|Actions")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, Category = "Input|Actions")
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, Category = "Input|Actions")
	UInputAction* ShootAction;

	UPROPERTY(EditAnywhere, Category = "Input|Actions")
	UInputAction* MeleeAction;

	UPROPERTY(EditAnywhere, Category = "Input|Actions")
	UInputAction* AimAction;

	UPROPERTY(EditAnywhere, Category = "Input|Actions")
	UInputAction* PauseAction;

	// Callbacks de Input
	void OnMoveTriggered(const FInputActionValue& Value);
	void OnJumpTriggered();
	void OnShootStarted();
	void OnShootCompleted();
	void OnMeleeTriggered();
	void OnAimTriggered(const FInputActionValue& Value);
	void OnPauseTriggered();

protected:
	FVector2D ReticleScreenPos;
	bool bIsGamePaused = false;
	bool bUsingGamepadAim = false;

	AWildGunsCharacter* GetControlledWildGunsCharacter() const;
};
