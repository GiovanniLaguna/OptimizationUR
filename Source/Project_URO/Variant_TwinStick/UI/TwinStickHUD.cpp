// Copyright Epic Games, Inc. All Rights Reserved.


#include "TwinStickHUD.h"
#include "TwinStickUI.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

ATwinStickHUD::ATwinStickHUD()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ATwinStickHUD::BeginPlay()
{
	Super::BeginPlay();

	FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this);
	if (CurrentLevelName.Contains(TEXT("Menu"), ESearchCase::IgnoreCase))
	{
		return;
	}

	// Ensure we have a player controller and a valid widget class assigned
	APlayerController* PC = GetOwningPlayerController();
	if (PC && UIWidgetClass)
	{
		UIWidget = CreateWidget<UTwinStickUI>(PC, UIWidgetClass);
		if (UIWidget)
		{
			UIWidget->AddToViewport(0);
		}
	}
}

void ATwinStickHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas || !bShowControlsGuide)
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
	UFont* MedFont = GEngine->GetMediumFont();

	const float GuideH = 34.0f;
	const float GuideW = FMath::Min(980.0f, ScreenW - 40.0f);
	const float GuideX = (ScreenW - GuideW) * 0.5f;
	const float GuideY = ScreenH - GuideH - 12.0f;

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
		{ TEXT("[W / A / S / D]"), TEXT("MOVERSE") },
		{ TEXT("[RATON]"), TEXT("APUNTAR") },
		{ TEXT("[CLIC IZQ]"), TEXT("DISPARAR") },
		{ TEXT("[ESPACIO]"), TEXT("DASH") },
		{ TEXT("[E / F]"), TEXT("DINAMITA / ITEM") }
	};

	const float Spacing = (GuideW - 20.0f) / Prompts.Num();

	for (int32 i = 0; i < Prompts.Num(); ++i)
	{
		const FButtonPrompt& P = Prompts[i];
		const float SlotX = GuideX + 10.0f + (i * Spacing);

		DrawText(P.Key, FLinearColor(1.0f, 0.86f, 0.22f, 1.0f), SlotX + 8.0f, GuideY + 9.0f, MedFont, 0.82f, false);
		const float KeyLen = P.Key.Len() * 8.0f;
		DrawText(P.Action, FLinearColor(0.95f, 0.95f, 0.95f, 0.95f), SlotX + 8.0f + KeyLen + 6.0f, GuideY + 9.0f, MedFont, 0.82f, false);

		if (i < Prompts.Num() - 1)
		{
			DrawLine(SlotX + Spacing - 4.0f, GuideY + 6.0f, SlotX + Spacing - 4.0f, GuideY + GuideH - 6.0f, FLinearColor(0.5f, 0.45f, 0.25f, 0.5f), 1.0f);
		}
	}
}
