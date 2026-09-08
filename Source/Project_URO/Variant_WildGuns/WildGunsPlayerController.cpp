// Copyright Epic Games, Inc. All Rights Reserved.

#include "WildGunsPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "Variant_WildGuns/WildGunsCharacter.h"
#include "Variant_WildGuns/WildGunsGameMode.h"
#include "Variant_WildGuns/UI/WildGunsHUD.h"
#include "Variant_WildGuns/UI/WildGunsUI.h"

AWildGunsPlayerController::AWildGunsPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void AWildGunsPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Activar Enhanced Input Subsystem
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		for (UInputMappingContext* Mapping : InputMappings)
		{
			if (Mapping)
			{
				Subsystem->AddMappingContext(Mapping, 0);
			}
		}
	}

	// Posición inicial de la retícula al centro de la pantalla
	int32 SizeX = 1920;
	int32 SizeY = 1080;
	GetViewportSize(SizeX, SizeY);
	ReticleScreenPos = FVector2D(SizeX * 0.5f, SizeY * 0.45f);
}

void AWildGunsPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this);
	if (CurrentLevelName.Contains(TEXT("Menu"), ESearchCase::IgnoreCase))
	{
		bShowMouseCursor = true;
		return;
	}

	// Actualizar posición de la retícula desde el ratón si se está usando
	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (GetMousePosition(MouseX, MouseY) && !bUsingGamepadAim)
	{
		ReticleScreenPos = FVector2D(MouseX, MouseY);
	}

	// Notificar al widget de UI de la nueva posición de retícula
	if (AWildGunsHUD* WGHUD = Cast<AWildGunsHUD>(GetHUD()))
	{
		if (UWildGunsUI* UI = WGHUD->GetUIWidget())
		{
			UI->UpdateReticlePosition(ReticleScreenPos);
		}
	}

	// Controles globales universales de teclado / ratón
	if (WasInputKeyJustPressed(EKeys::Escape) || WasInputKeyJustPressed(EKeys::P))
	{
		TogglePause();
	}

	AWildGunsGameMode* GM = GetWorld()->GetAuthGameMode<AWildGunsGameMode>();
	const bool bIsGameOverOrVictory = GM && (GM->GetMatchState() != EWildGunsMatchState::Playing);

	if (bIsGamePaused || bIsGameOverOrVictory)
	{
		// 1. Clic directo con ratón sobre los botones dibujados en pantalla
		if (WasInputKeyJustPressed(EKeys::LeftMouseButton))
		{
			if (AWildGunsHUD* WGHUD = Cast<AWildGunsHUD>(GetHUD()))
			{
				WGHUD->HandleScreenClick(FVector2D(MouseX, MouseY));
			}
		}

		// 2. Atajos de teclado (R / Enter / Espacio para reintentar; M para Menú Principal)
		if (WasInputKeyJustPressed(EKeys::R) || WasInputKeyJustPressed(EKeys::Enter) || WasInputKeyJustPressed(EKeys::SpaceBar))
		{
			RestartLevelGame();
		}
		if (WasInputKeyJustPressed(EKeys::M))
		{
			ReturnToMainMenu();
		}
		return;
	}

	// Movimiento lateral (Strafe)
	float MoveAxis = 0.0f;
	if (IsInputKeyDown(EKeys::A) || IsInputKeyDown(EKeys::Left))
	{
		MoveAxis -= 1.0f;
	}
	if (IsInputKeyDown(EKeys::D) || IsInputKeyDown(EKeys::Right))
	{
		MoveAxis += 1.0f;
	}
	if (MoveAxis != 0.0f)
	{
		if (AWildGunsCharacter* WGChar = GetControlledWildGunsCharacter())
		{
			WGChar->MoveRight(MoveAxis);
		}
	}

	// Salto
	if (WasInputKeyJustPressed(EKeys::SpaceBar) || WasInputKeyJustPressed(EKeys::W) || WasInputKeyJustPressed(EKeys::Up))
	{
		OnJumpTriggered();
	}

	// Disparo con clic izquierdo
	if (WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		OnShootStarted();
	}
	if (WasInputKeyJustReleased(EKeys::LeftMouseButton))
	{
		OnShootCompleted();
	}

	// Ataque Melee con F o clic derecho
	if (WasInputKeyJustPressed(EKeys::F) || WasInputKeyJustPressed(EKeys::RightMouseButton))
	{
		OnMeleeTriggered();
	}
}

void AWildGunsPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AWildGunsPlayerController::OnMoveTriggered);
		}

		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AWildGunsPlayerController::OnJumpTriggered);
		}

		if (ShootAction)
		{
			EnhancedInputComponent->BindAction(ShootAction, ETriggerEvent::Started, this, &AWildGunsPlayerController::OnShootStarted);
			EnhancedInputComponent->BindAction(ShootAction, ETriggerEvent::Completed, this, &AWildGunsPlayerController::OnShootCompleted);
		}

		if (MeleeAction)
		{
			EnhancedInputComponent->BindAction(MeleeAction, ETriggerEvent::Started, this, &AWildGunsPlayerController::OnMeleeTriggered);
		}

		if (AimAction)
		{
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Triggered, this, &AWildGunsPlayerController::OnAimTriggered);
		}

		if (PauseAction)
		{
			EnhancedInputComponent->BindAction(PauseAction, ETriggerEvent::Started, this, &AWildGunsPlayerController::OnPauseTriggered);
		}
	}
}

AWildGunsCharacter* AWildGunsPlayerController::GetControlledWildGunsCharacter() const
{
	return Cast<AWildGunsCharacter>(GetPawn());
}

void AWildGunsPlayerController::OnMoveTriggered(const FInputActionValue& Value)
{
	if (AWildGunsCharacter* WGChar = GetControlledWildGunsCharacter())
	{
		WGChar->MoveRight(Value.Get<float>());
	}
}

void AWildGunsPlayerController::OnJumpTriggered()
{
	if (AWildGunsCharacter* WGChar = GetControlledWildGunsCharacter())
	{
		WGChar->Jump();
	}
}

void AWildGunsPlayerController::OnShootStarted()
{
	if (AWildGunsCharacter* WGChar = GetControlledWildGunsCharacter())
	{
		WGChar->StartFiring();
	}
}

void AWildGunsPlayerController::OnShootCompleted()
{
	if (AWildGunsCharacter* WGChar = GetControlledWildGunsCharacter())
	{
		WGChar->StopFiring();
	}
}

void AWildGunsPlayerController::OnMeleeTriggered()
{
	if (AWildGunsCharacter* WGChar = GetControlledWildGunsCharacter())
	{
		WGChar->PerformMeleeAttack();
	}
}

void AWildGunsPlayerController::OnAimTriggered(const FInputActionValue& Value)
{
	FVector2D AimInput = Value.Get<FVector2D>();
	if (!AimInput.IsNearlyZero())
	{
		bUsingGamepadAim = true;
		int32 SizeX = 1920;
		int32 SizeY = 1080;
		GetViewportSize(SizeX, SizeY);

		ReticleScreenPos.X = FMath::Clamp(ReticleScreenPos.X + AimInput.X * 800.0f * GetWorld()->GetDeltaSeconds(), 50.0f, (float)SizeX - 50.0f);
		ReticleScreenPos.Y = FMath::Clamp(ReticleScreenPos.Y - AimInput.Y * 800.0f * GetWorld()->GetDeltaSeconds(), 50.0f, (float)SizeY - 50.0f);
	}
}

void AWildGunsPlayerController::OnPauseTriggered()
{
	TogglePause();
}

void AWildGunsPlayerController::TogglePause()
{
	bIsGamePaused = !bIsGamePaused;
	SetPause(bIsGamePaused);

	if (AWildGunsHUD* WGHUD = Cast<AWildGunsHUD>(GetHUD()))
	{
		if (UWildGunsUI* UI = WGHUD->GetUIWidget())
		{
			UI->SetPauseMenuVisible(bIsGamePaused);
		}
	}

	if (bIsGamePaused)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
		bShowMouseCursor = true;
		bEnableClickEvents = true;
		bEnableMouseOverEvents = true;
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = true;
	}
}

void AWildGunsPlayerController::PlayCameraFade(bool bFadeOut, float Duration)
{
	if (PlayerCameraManager)
	{
		if (bFadeOut)
		{
			PlayerCameraManager->StartCameraFade(0.0f, 1.0f, Duration, FLinearColor::Black, false, true);
		}
		else
		{
			PlayerCameraManager->StartCameraFade(1.0f, 0.0f, Duration, FLinearColor::Black, false, false);
		}
	}

	if (AWildGunsHUD* WGHUD = Cast<AWildGunsHUD>(GetHUD()))
	{
		if (UWildGunsUI* UI = WGHUD->GetUIWidget())
		{
			UI->PlayScreenFade(bFadeOut, Duration);
		}
	}
}

FVector AWildGunsPlayerController::GetReticleWorldLocation() const
{
	FVector WorldOrigin;
	FVector WorldDirection;

	if (DeprojectScreenPositionToWorld(ReticleScreenPos.X, ReticleScreenPos.Y, WorldOrigin, WorldDirection))
	{
		FHitResult HitResult;
		FCollisionQueryParams Params;
		if (APawn* ControlledPawn = GetPawn())
		{
			Params.AddIgnoredActor(ControlledPawn);
		}

		const FVector TraceEnd = WorldOrigin + (WorldDirection * 9000.0f);
		if (GetWorld()->LineTraceSingleByChannel(HitResult, WorldOrigin, TraceEnd, ECC_Visibility, Params))
		{
			return HitResult.ImpactPoint;
		}

		return WorldOrigin + (WorldDirection * 3500.0f);
	}

	if (APawn* ControlledPawn = GetPawn())
	{
		return ControlledPawn->GetActorLocation() + FVector(2500.0f, 0.0f, 50.0f);
	}

	return FVector::ZeroVector;
}

void AWildGunsPlayerController::RestartLevelGame()
{
	// Despausar inmediatamente si el juego estaba pausado
	if (bIsGamePaused)
	{
		SetPause(false);
		bIsGamePaused = false;
	}

	SetInputMode(FInputModeGameOnly());

	FString CurrentMapName = UGameplayStatics::GetCurrentLevelName(this);
	if (CurrentMapName.IsEmpty() || CurrentMapName.Contains(TEXT("Menu"), ESearchCase::IgnoreCase))
	{
		CurrentMapName = TEXT("LVL_WildGuns");
	}

	UGameplayStatics::OpenLevel(this, FName(*CurrentMapName));
}

void AWildGunsPlayerController::ReturnToMainMenu()
{
	// Despausar inmediatamente si el juego estaba pausado
	if (bIsGamePaused)
	{
		SetPause(false);
		bIsGamePaused = false;
	}

	FInputModeGameAndUI MenuMode;
	MenuMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	MenuMode.SetHideCursorDuringCapture(false);
	SetInputMode(MenuMode);
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	UGameplayStatics::OpenLevel(this, FName(TEXT("LVL_WildGuns_MainMenu")));
}
