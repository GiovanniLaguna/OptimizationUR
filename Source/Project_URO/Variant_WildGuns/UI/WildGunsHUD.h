// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "WildGunsHUD.generated.h"

class UWildGunsUI;

/**
 * HUD principal para Wild Guns.
 * Administra la creación y visibilidad de los widgets de UI del juego.
 */
UCLASS()
class PROJECT_URO_API AWildGunsHUD : public AHUD
{
	GENERATED_BODY()

public:
	AWildGunsHUD();

	virtual void BeginPlay() override;
	virtual void DrawHUD() override;
	virtual void NotifyHitBoxClick(FName BoxName) override;

	/** Procesa clics directos de ratón sobre los botones de Pausa, Victoria o Game Over */
	UFUNCTION(BlueprintCallable, Category = "WildGuns HUD")
	bool HandleScreenClick(FVector2D ClickPos);

	UFUNCTION(BlueprintPure, Category = "WildGuns HUD")
	UWildGunsUI* GetUIWidget() const { return UIWidget; }

	/** Si es true, muestra la barra inferior con la guía de controles y botones */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WildGuns HUD")
	bool bShowControlsGuide = true;

	/** Alterna la visibilidad de la guía de controles */
	UFUNCTION(BlueprintCallable, Category = "WildGuns HUD")
	void ToggleControlsGuide() { bShowControlsGuide = !bShowControlsGuide; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WildGuns HUD")
	TSubclassOf<UWildGunsUI> UIWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WildGuns HUD", Transient)
	TObjectPtr<UWildGunsUI> UIWidget;
};
