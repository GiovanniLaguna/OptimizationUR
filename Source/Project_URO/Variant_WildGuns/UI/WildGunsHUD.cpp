// Copyright Epic Games, Inc. All Rights Reserved.

#include "WildGunsHUD.h"
#include "WildGunsUI.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Variant_WildGuns/WildGunsGameMode.h"
#include "Variant_WildGuns/WildGunsPlayerController.h"
#include "Variant_WildGuns/WildGunsCharacter.h"

AWildGunsHUD::AWildGunsHUD()
{
}

void AWildGunsHUD::BeginPlay()
{
	Super::BeginPlay();

	FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this);
	if (CurrentLevelName.Contains(TEXT("Menu"), ESearchCase::IgnoreCase))
	{
		return;
	}

	if (UIWidgetClass)
	{
		APlayerController* PC = GetOwningPlayerController();
		if (PC)
		{
			UIWidget = CreateWidget<UWildGunsUI>(PC, UIWidgetClass);
			if (UIWidget)
			{
				UIWidget->AddToViewport(0);
			}
		}
	}
}

void AWildGunsHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this);
	if (CurrentLevelName.Contains(TEXT("Menu"), ESearchCase::IgnoreCase))
	{
		return;
	}

	const float ScreenW = Canvas->ClipX;
	const float ScreenH = Canvas->ClipY;

	AWildGunsPlayerController* PC = Cast<AWildGunsPlayerController>(GetOwningPlayerController());
	AWildGunsCharacter* WGChar = PC ? Cast<AWildGunsCharacter>(PC->GetPawn()) : nullptr;
	AWildGunsGameMode* GM = GetWorld()->GetAuthGameMode<AWildGunsGameMode>();

	UFont* LargeFont = GEngine->GetLargeFont();
	UFont* MedFont = GEngine->GetMediumFont();

	// 1. Dibujar Barra Arcade Superior (Timer, Vidas, Arma)
	const float BarW = FMath::Min(860.0f, ScreenW - 40.0f);
	const float BarH = 65.0f;
	const float BarX = (ScreenW - BarW) * 0.5f;
	const float BarY = 20.0f;

	// Fondo y marco
	DrawRect(FLinearColor(0.04f, 0.04f, 0.08f, 0.88f), BarX, BarY, BarW, BarH);
	DrawRect(FLinearColor(0.88f, 0.7f, 0.2f, 1.0f), BarX, BarY, BarW, 2.0f);
	DrawRect(FLinearColor(0.88f, 0.7f, 0.2f, 1.0f), BarX, BarY + BarH - 2.0f, BarW, 2.0f);
	DrawRect(FLinearColor(0.88f, 0.7f, 0.2f, 1.0f), BarX, BarY, 2.0f, BarH);
	DrawRect(FLinearColor(0.88f, 0.7f, 0.2f, 1.0f), BarX + BarW - 2.0f, BarY, 2.0f, BarH);

	// Vidas
	int32 Lives = WGChar ? WGChar->GetLives() : 3;
	FString LivesStr = TEXT("LIVES: ");
	for (int32 i = 0; i < 3; ++i)
	{
		LivesStr += (i < Lives) ? TEXT(" [O]") : TEXT(" [X]");
	}
	DrawText(LivesStr, FLinearColor(0.2f, 1.0f, 0.4f, 1.0f), BarX + 25.0f, BarY + 22.0f, MedFont, 1.0f, false);

	// Cronómetro Central (Parpadeo cuando quedan <= 15s)
	float TimeRemaining = GM ? GM->GetMatchTimeRemaining() : 60.0f;
	int32 TotalSec = FMath::CeilToInt(TimeRemaining);
	FString TimeStr = FString::Printf(TEXT("TIME: %02d:%02d"), TotalSec / 60, TotalSec % 60);

	FLinearColor TimerColor = (TotalSec <= 15) ?
		((FMath::Sin(GetWorld()->GetTimeSeconds() * 8.0f) > 0.0f) ? FLinearColor(1.0f, 0.1f, 0.1f, 1.0f) : FLinearColor(1.0f, 0.9f, 0.1f, 1.0f)) :
		FLinearColor(1.0f, 0.95f, 0.3f, 1.0f);

	DrawText(TimeStr, TimerColor, BarX + (BarW * 0.5f) - 65.0f, BarY + 18.0f, LargeFont, 1.0f, false);

	// Arma actual
	EWildGunsWeapon CurrentWp = WGChar ? WGChar->GetCurrentWeapon() : EWildGunsWeapon::MachineGun;
	int32 Ammo = WGChar ? WGChar->GetCurrentAmmo() : 0;
	FString WeaponStr = (CurrentWp == EWildGunsWeapon::MachineGun) ?
		TEXT("GUN: MACHINE GUN [INF]") :
		FString::Printf(TEXT("GUN: SHOTGUN [%d/50]"), Ammo);
	FLinearColor WeaponColor = (CurrentWp == EWildGunsWeapon::Shotgun) ?
		FLinearColor(1.0f, 0.8f, 0.1f, 1.0f) :
		FLinearColor(0.6f, 0.85f, 1.0f, 1.0f);

	DrawText(WeaponStr, WeaponColor, BarX + BarW - 240.0f, BarY + 22.0f, MedFont, 1.0f, false);

	// 2. Retícula Arcade Dinámica
	if (PC && (!GM || GM->GetMatchState() == EWildGunsMatchState::Playing) && !PC->IsGamePaused())
	{
		const FVector2D ReticlePos = PC->GetReticleScreenPosition();
		const float ReticleSize = 18.0f;
		const FLinearColor ReticleColor = FLinearColor(1.0f, 0.85f, 0.15f, 0.95f);
		const float Thickness = 2.0f;

		DrawLine(ReticlePos.X - ReticleSize, ReticlePos.Y, ReticlePos.X - 5.0f, ReticlePos.Y, ReticleColor, Thickness);
		DrawLine(ReticlePos.X + 5.0f, ReticlePos.Y, ReticlePos.X + ReticleSize, ReticlePos.Y, ReticleColor, Thickness);
		DrawLine(ReticlePos.X, ReticlePos.Y - ReticleSize, ReticlePos.X, ReticlePos.Y - 5.0f, ReticleColor, Thickness);
		DrawLine(ReticlePos.X, ReticlePos.Y + 5.0f, ReticlePos.X, ReticlePos.Y + ReticleSize, ReticleColor, Thickness);

		// Centro
		DrawRect(FLinearColor(1.0f, 0.2f, 0.2f, 1.0f), ReticlePos.X - 2.0f, ReticlePos.Y - 2.0f, 4.0f, 4.0f);
	}

	// 3. Guía de Botones y Controles en la Interfaz (Arcade HUD)
	if (bShowControlsGuide && (!PC || !PC->IsGamePaused()) && (!GM || GM->GetMatchState() == EWildGunsMatchState::Playing))
	{
		const float GuideH = 34.0f;
		const float GuideW = FMath::Min(1060.0f, ScreenW - 40.0f);
		const float GuideX = (ScreenW - GuideW) * 0.5f;
		const float GuideY = ScreenH - GuideH - 12.0f;

		// Fondo de barra semitransparente estilo arcade western con bordes dorados
		DrawRect(FLinearColor(0.03f, 0.04f, 0.06f, 0.88f), GuideX, GuideY, GuideW, GuideH);
		DrawRect(FLinearColor(0.88f, 0.70f, 0.20f, 0.85f), GuideX, GuideY, GuideW, 1.5f);
		DrawRect(FLinearColor(0.88f, 0.70f, 0.20f, 0.85f), GuideX, GuideY + GuideH - 1.5f, GuideW, 1.5f);
		DrawRect(FLinearColor(0.88f, 0.70f, 0.20f, 0.85f), GuideX, GuideY, 1.5f, GuideH);
		DrawRect(FLinearColor(0.88f, 0.70f, 0.20f, 0.85f), GuideX + GuideW - 1.5f, GuideY, 1.5f, GuideH);

		struct FButtonPrompt
		{
			FString Key;
			FString Action;
		};

		const TArray<FButtonPrompt> Prompts = {
			{ TEXT("[A / D]"), TEXT("MOVERSE") },
			{ TEXT("[ESPACIO]"), TEXT("SALTO x2") },
			{ TEXT("[RATON]"), TEXT("APUNTAR") },
			{ TEXT("[CLIC IZQ]"), TEXT("DISPARAR") },
			{ TEXT("[CLIC DER / F]"), TEXT("MELEE") },
			{ TEXT("[ESC / P]"), TEXT("PAUSA") }
		};

		const float Spacing = (GuideW - 20.0f) / Prompts.Num();

		for (int32 i = 0; i < Prompts.Num(); ++i)
		{
			const FButtonPrompt& P = Prompts[i];
			const float SlotX = GuideX + 10.0f + (i * Spacing);

			// Tecla en dorado / ámbar
			DrawText(P.Key, FLinearColor(1.0f, 0.86f, 0.22f, 1.0f), SlotX + 8.0f, GuideY + 9.0f, MedFont, 0.82f, false);

			// Acción en blanco nítido
			const float KeyLen = P.Key.Len() * 8.0f;
			DrawText(P.Action, FLinearColor(0.95f, 0.95f, 0.95f, 0.95f), SlotX + 8.0f + KeyLen + 6.0f, GuideY + 9.0f, MedFont, 0.82f, false);

			// Divisor vertical
			if (i < Prompts.Num() - 1)
			{
				DrawLine(SlotX + Spacing - 4.0f, GuideY + 6.0f, SlotX + Spacing - 4.0f, GuideY + GuideH - 6.0f, FLinearColor(0.5f, 0.45f, 0.25f, 0.5f), 1.0f);
			}
		}
	}

	// Obtener posición del ratón para efectos de hover
	float MouseX = 0.0f, MouseY = 0.0f;
	if (PC)
	{
		PC->GetMousePosition(MouseX, MouseY);
	}
	const FVector2D MousePos(MouseX, MouseY);

	// 4. Pantalla de Pausa
	if (PC && PC->IsGamePaused())
	{
		DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.68f), 0.0f, 0.0f, ScreenW, ScreenH);

		const float BoxW = 460.0f;
		const float BoxH = 320.0f;
		const float BoxX = (ScreenW - BoxW) * 0.5f;
		const float BoxY = (ScreenH - BoxH) * 0.5f;

		DrawRect(FLinearColor(0.05f, 0.06f, 0.09f, 0.96f), BoxX, BoxY, BoxW, BoxH);
		DrawRect(FLinearColor(0.88f, 0.7f, 0.2f, 1.0f), BoxX, BoxY, BoxW, 2.0f);
		DrawRect(FLinearColor(0.88f, 0.7f, 0.2f, 1.0f), BoxX, BoxY + BoxH - 2.0f, BoxW, 2.0f);
		DrawRect(FLinearColor(0.88f, 0.7f, 0.2f, 1.0f), BoxX, BoxY, 2.0f, BoxH);
		DrawRect(FLinearColor(0.88f, 0.7f, 0.2f, 1.0f), BoxX + BoxW - 2.0f, BoxY, 2.0f, BoxH);

		DrawText(TEXT("--- GAME PAUSED ---"), FLinearColor(1.0f, 0.88f, 0.2f, 1.0f), BoxX + 110.0f, BoxY + 25.0f, LargeFont, 1.0f, false);

		const float BtnW = 280.0f;
		const float BtnH = 42.0f;
		const float BtnX = BoxX + (BoxW - BtnW) * 0.5f;

		const bool bHoverResume = (MousePos.X >= BtnX && MousePos.X <= BtnX + BtnW && MousePos.Y >= BoxY + 80.0f && MousePos.Y <= BoxY + 80.0f + BtnH);
		const bool bHoverRestart = (MousePos.X >= BtnX && MousePos.X <= BtnX + BtnW && MousePos.Y >= BoxY + 138.0f && MousePos.Y <= BoxY + 138.0f + BtnH);
		const bool bHoverMainMenu = (MousePos.X >= BtnX && MousePos.X <= BtnX + BtnW && MousePos.Y >= BoxY + 196.0f && MousePos.Y <= BoxY + 196.0f + BtnH);

		// Botón Reanudar
		DrawRect(bHoverResume ? FLinearColor(0.2f, 0.62f, 0.28f, 1.0f) : FLinearColor(0.12f, 0.38f, 0.16f, 0.95f), BtnX, BoxY + 80.0f, BtnW, BtnH);
		DrawText(TEXT("[ RESUME ]  (ESC)"), bHoverResume ? FLinearColor(1.0f, 1.0f, 0.3f, 1.0f) : FLinearColor::White, BtnX + 50.0f, BoxY + 92.0f, MedFont, 1.0f, false);
		AddHitBox(FVector2D(BtnX, BoxY + 80.0f), FVector2D(BtnW, BtnH), TEXT("Btn_Resume"), true);

		// Botón Reiniciar
		DrawRect(bHoverRestart ? FLinearColor(0.58f, 0.38f, 0.15f, 1.0f) : FLinearColor(0.38f, 0.24f, 0.1f, 0.95f), BtnX, BoxY + 138.0f, BtnW, BtnH);
		DrawText(TEXT("[ RESTART ]  (R)"), bHoverRestart ? FLinearColor(1.0f, 1.0f, 0.3f, 1.0f) : FLinearColor::White, BtnX + 52.0f, BoxY + 150.0f, MedFont, 1.0f, false);
		AddHitBox(FVector2D(BtnX, BoxY + 138.0f), FVector2D(BtnW, BtnH), TEXT("Btn_Restart"), true);

		// Botón Menú Principal
		DrawRect(bHoverMainMenu ? FLinearColor(0.58f, 0.22f, 0.22f, 1.0f) : FLinearColor(0.38f, 0.14f, 0.14f, 0.95f), BtnX, BoxY + 196.0f, BtnW, BtnH);
		DrawText(TEXT("[ MAIN MENU ]  (M)"), bHoverMainMenu ? FLinearColor(1.0f, 1.0f, 0.3f, 1.0f) : FLinearColor::White, BtnX + 42.0f, BoxY + 208.0f, MedFont, 1.0f, false);
		AddHitBox(FVector2D(BtnX, BoxY + 196.0f), FVector2D(BtnW, BtnH), TEXT("Btn_MainMenu"), true);

		// Recordatorio de controles en la base del panel de pausa
		DrawText(TEXT("GUIA: [A/D] Mover | [Espacio] Doble Salto | [Clic Izq] Disparo | [F] Melee | [H] HUD"), FLinearColor(0.72f, 0.82f, 0.92f, 0.85f), BoxX + 16.0f, BoxY + BoxH - 26.0f, MedFont, 0.74f, false);
	}

	// 4. Pantalla de Victoria (Sobrevivir el minuto)
	if (GM && GM->GetMatchState() == EWildGunsMatchState::Victory)
	{
		DrawRect(FLinearColor(0.01f, 0.14f, 0.05f, 0.78f), 0.0f, 0.0f, ScreenW, ScreenH);

		const float BoxW = 560.0f;
		const float BoxH = 320.0f;
		const float BoxX = (ScreenW - BoxW) * 0.5f;
		const float BoxY = (ScreenH - BoxH) * 0.5f;

		DrawRect(FLinearColor(0.03f, 0.08f, 0.04f, 0.96f), BoxX, BoxY, BoxW, BoxH);
		DrawRect(FLinearColor(0.2f, 0.95f, 0.4f, 1.0f), BoxX, BoxY, BoxW, 3.0f);
		DrawRect(FLinearColor(0.2f, 0.95f, 0.4f, 1.0f), BoxX, BoxY + BoxH - 3.0f, BoxW, 3.0f);
		DrawRect(FLinearColor(0.2f, 0.95f, 0.4f, 1.0f), BoxX, BoxY, 3.0f, BoxH);
		DrawRect(FLinearColor(0.2f, 0.95f, 0.4f, 1.0f), BoxX + BoxW - 3.0f, BoxY, 3.0f, BoxH);

		DrawText(TEXT("STAGE 1 CLEAR - YOU SURVIVED!"), FLinearColor(0.2f, 1.0f, 0.4f, 1.0f), BoxX + 50.0f, BoxY + 30.0f, LargeFont, 1.0f, false);
		DrawText(TEXT("Time: 60s  |  Level 1 Completed Successfully!"), FLinearColor::White, BoxX + 75.0f, BoxY + 80.0f, MedFont, 1.0f, false);

		const float BtnW = 280.0f;
		const float BtnH = 45.0f;
		const float BtnX = BoxX + (BoxW - BtnW) * 0.5f;

		const bool bHoverRestart = (MousePos.X >= BtnX && MousePos.X <= BtnX + BtnW && MousePos.Y >= BoxY + 140.0f && MousePos.Y <= BoxY + 140.0f + BtnH);
		const bool bHoverMainMenu = (MousePos.X >= BtnX && MousePos.X <= BtnX + BtnW && MousePos.Y >= BoxY + 205.0f && MousePos.Y <= BoxY + 205.0f + BtnH);

		DrawRect(bHoverRestart ? FLinearColor(0.2f, 0.7f, 0.28f, 1.0f) : FLinearColor(0.12f, 0.48f, 0.18f, 0.95f), BtnX, BoxY + 140.0f, BtnW, BtnH);
		DrawText(TEXT("[ PLAY AGAIN ]  (Press R)"), bHoverRestart ? FLinearColor(1.0f, 1.0f, 0.3f, 1.0f) : FLinearColor::White, BtnX + 32.0f, BoxY + 153.0f, MedFont, 1.0f, false);
		AddHitBox(FVector2D(BtnX, BoxY + 140.0f), FVector2D(BtnW, BtnH), TEXT("Btn_Restart"), true);

		DrawRect(bHoverMainMenu ? FLinearColor(0.55f, 0.32f, 0.22f, 1.0f) : FLinearColor(0.35f, 0.2f, 0.15f, 0.95f), BtnX, BoxY + 205.0f, BtnW, BtnH);
		DrawText(TEXT("[ MAIN MENU ]  (Press M)"), bHoverMainMenu ? FLinearColor(1.0f, 1.0f, 0.3f, 1.0f) : FLinearColor::White, BtnX + 40.0f, BoxY + 218.0f, MedFont, 1.0f, false);
		AddHitBox(FVector2D(BtnX, BoxY + 205.0f), FVector2D(BtnW, BtnH), TEXT("Btn_MainMenu"), true);
	}

	// 5. Pantalla de Game Over (3 Vidas agotadas)
	if (GM && GM->GetMatchState() == EWildGunsMatchState::GameOver)
	{
		DrawRect(FLinearColor(0.18f, 0.02f, 0.02f, 0.82f), 0.0f, 0.0f, ScreenW, ScreenH);

		const float BoxW = 560.0f;
		const float BoxH = 320.0f;
		const float BoxX = (ScreenW - BoxW) * 0.5f;
		const float BoxY = (ScreenH - BoxH) * 0.5f;

		DrawRect(FLinearColor(0.1f, 0.03f, 0.03f, 0.96f), BoxX, BoxY, BoxW, BoxH);
		DrawRect(FLinearColor(0.95f, 0.2f, 0.2f, 1.0f), BoxX, BoxY, BoxW, 3.0f);
		DrawRect(FLinearColor(0.95f, 0.2f, 0.2f, 1.0f), BoxX, BoxY + BoxH - 3.0f, BoxW, 3.0f);
		DrawRect(FLinearColor(0.95f, 0.2f, 0.2f, 1.0f), BoxX, BoxY, 3.0f, BoxH);
		DrawRect(FLinearColor(0.95f, 0.2f, 0.2f, 1.0f), BoxX + BoxW - 3.0f, BoxY, 3.0f, BoxH);

		DrawText(TEXT("GAME OVER - OUT OF LIVES"), FLinearColor(1.0f, 0.25f, 0.25f, 1.0f), BoxX + 80.0f, BoxY + 30.0f, LargeFont, 1.0f, false);
		DrawText(TEXT("The West has claimed another bounty hunter."), FLinearColor(0.85f, 0.85f, 0.85f, 1.0f), BoxX + 90.0f, BoxY + 80.0f, MedFont, 1.0f, false);

		const float BtnW = 280.0f;
		const float BtnH = 45.0f;
		const float BtnX = BoxX + (BoxW - BtnW) * 0.5f;

		const bool bHoverRestart = (MousePos.X >= BtnX && MousePos.X <= BtnX + BtnW && MousePos.Y >= BoxY + 140.0f && MousePos.Y <= BoxY + 140.0f + BtnH);
		const bool bHoverMainMenu = (MousePos.X >= BtnX && MousePos.X <= BtnX + BtnW && MousePos.Y >= BoxY + 205.0f && MousePos.Y <= BoxY + 205.0f + BtnH);

		DrawRect(bHoverRestart ? FLinearColor(0.72f, 0.18f, 0.18f, 1.0f) : FLinearColor(0.48f, 0.12f, 0.12f, 0.95f), BtnX, BoxY + 140.0f, BtnW, BtnH);
		DrawText(TEXT("[ RETRY STAGE ]  (Press R)"), bHoverRestart ? FLinearColor(1.0f, 1.0f, 0.3f, 1.0f) : FLinearColor::White, BtnX + 30.0f, BoxY + 153.0f, MedFont, 1.0f, false);
		AddHitBox(FVector2D(BtnX, BoxY + 140.0f), FVector2D(BtnW, BtnH), TEXT("Btn_Restart"), true);

		DrawRect(bHoverMainMenu ? FLinearColor(0.5f, 0.32f, 0.32f, 1.0f) : FLinearColor(0.3f, 0.2f, 0.2f, 0.95f), BtnX, BoxY + 205.0f, BtnW, BtnH);
		DrawText(TEXT("[ MAIN MENU ]  (Press M)"), bHoverMainMenu ? FLinearColor(1.0f, 1.0f, 0.3f, 1.0f) : FLinearColor::White, BtnX + 40.0f, BoxY + 218.0f, MedFont, 1.0f, false);
		AddHitBox(FVector2D(BtnX, BoxY + 205.0f), FVector2D(BtnW, BtnH), TEXT("Btn_MainMenu"), true);
	}
}

bool AWildGunsHUD::HandleScreenClick(FVector2D ClickPos)
{
	AWildGunsPlayerController* PC = Cast<AWildGunsPlayerController>(GetOwningPlayerController());
	if (!PC) return false;

	AWildGunsGameMode* GM = GetWorld()->GetAuthGameMode<AWildGunsGameMode>();
	const float ScreenW = Canvas ? Canvas->ClipX : 1920.0f;
	const float ScreenH = Canvas ? Canvas->ClipY : 1080.0f;

	// 1. Caso Pausa
	if (PC->IsGamePaused())
	{
		const float BoxW = 460.0f;
		const float BoxH = 290.0f;
		const float BoxX = (ScreenW - BoxW) * 0.5f;
		const float BoxY = (ScreenH - BoxH) * 0.5f;
		const float BtnW = 280.0f;
		const float BtnH = 42.0f;
		const float BtnX = BoxX + (BoxW - BtnW) * 0.5f;

		// Botón Reanudar
		if (ClickPos.X >= BtnX && ClickPos.X <= BtnX + BtnW &&
			ClickPos.Y >= BoxY + 80.0f && ClickPos.Y <= BoxY + 80.0f + BtnH)
		{
			PC->TogglePause();
			return true;
		}

		// Botón Reiniciar
		if (ClickPos.X >= BtnX && ClickPos.X <= BtnX + BtnW &&
			ClickPos.Y >= BoxY + 138.0f && ClickPos.Y <= BoxY + 138.0f + BtnH)
		{
			PC->RestartLevelGame();
			return true;
		}

		// Botón Menú Principal
		if (ClickPos.X >= BtnX && ClickPos.X <= BtnX + BtnW &&
			ClickPos.Y >= BoxY + 196.0f && ClickPos.Y <= BoxY + 196.0f + BtnH)
		{
			PC->ReturnToMainMenu();
			return true;
		}
	}

	// 2. Caso Victoria o Game Over
	if (GM && (GM->GetMatchState() == EWildGunsMatchState::Victory || GM->GetMatchState() == EWildGunsMatchState::GameOver))
	{
		const float BoxW = 560.0f;
		const float BoxH = 320.0f;
		const float BoxX = (ScreenW - BoxW) * 0.5f;
		const float BoxY = (ScreenH - BoxH) * 0.5f;
		const float BtnW = 280.0f;
		const float BtnH = 45.0f;
		const float BtnX = BoxX + (BoxW - BtnW) * 0.5f;

		// Botón Retry / Reiniciar
		if (ClickPos.X >= BtnX && ClickPos.X <= BtnX + BtnW &&
			ClickPos.Y >= BoxY + 140.0f && ClickPos.Y <= BoxY + 140.0f + BtnH)
		{
			PC->RestartLevelGame();
			return true;
		}

		// Botón Menú Principal
		if (ClickPos.X >= BtnX && ClickPos.X <= BtnX + BtnW &&
			ClickPos.Y >= BoxY + 205.0f && ClickPos.Y <= BoxY + 205.0f + BtnH)
		{
			PC->ReturnToMainMenu();
			return true;
		}
	}

	return false;
}

void AWildGunsHUD::NotifyHitBoxClick(FName BoxName)
{
	Super::NotifyHitBoxClick(BoxName);

	AWildGunsPlayerController* PC = Cast<AWildGunsPlayerController>(GetOwningPlayerController());
	if (!PC) return;

	if (BoxName == TEXT("Btn_Resume"))
	{
		PC->TogglePause();
	}
	else if (BoxName == TEXT("Btn_Restart"))
	{
		PC->RestartLevelGame();
	}
	else if (BoxName == TEXT("Btn_MainMenu"))
	{
		PC->ReturnToMainMenu();
	}
}
