// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Variant_WildGuns/WildGunsCharacter.h"
#include "WildGunsUI.generated.h"

/**
 * Widget de interfaz principal para Wild Guns.
 * Muestra el cronómetro regresivo de 60s, vidas restantes, estado del arma,
 * posición de la retícula y pantallas de Pausa, Game Over y Victoria.
 */
UCLASS(Abstract)
class PROJECT_URO_API UWildGunsUI : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Actualiza el temporizador regresivo de 60 segundos */
	UFUNCTION(BlueprintImplementableEvent, Category = "WildGuns UI")
	void UpdateTimer(float RemainingSeconds, const FString& FormattedTime);

	/** Actualiza la cantidad de vidas restantes del jugador */
	UFUNCTION(BlueprintImplementableEvent, Category = "WildGuns UI")
	void UpdateLives(int32 RemainingLives);

	/** Actualiza el arma equipada (Metralleta / Escopeta) y el conteo de munición */
	UFUNCTION(BlueprintImplementableEvent, Category = "WildGuns UI")
	void UpdateWeaponStatus(EWildGunsWeapon WeaponType, int32 AmmoCount);

	/** Actualiza las coordenadas en pantalla de la retícula de apuntado */
	UFUNCTION(BlueprintImplementableEvent, Category = "WildGuns UI")
	void UpdateReticlePosition(FVector2D ScreenPosition);

	/** Muestra u oculta la pantalla de Pausa */
	UFUNCTION(BlueprintImplementableEvent, Category = "WildGuns UI")
	void SetPauseMenuVisible(bool bVisible);

	/** Muestra la pantalla de Game Over con opciones de retorno */
	UFUNCTION(BlueprintImplementableEvent, Category = "WildGuns UI")
	void ShowGameOverScreen();

	/** Muestra la pantalla de Victoria al sobrevivir el minuto */
	UFUNCTION(BlueprintImplementableEvent, Category = "WildGuns UI")
	void ShowVictoryScreen();

	/** Ejecuta la animación de transición de pantalla (Fade In / Fade Out) */
	UFUNCTION(BlueprintImplementableEvent, Category = "WildGuns UI")
	void PlayScreenFade(bool bFadeOut, float Duration = 1.0f);
};
