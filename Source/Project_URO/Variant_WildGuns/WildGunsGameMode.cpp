// Copyright Epic Games, Inc. All Rights Reserved.

#include "WildGunsGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Pooling/ActorPool.h"
#include "Pooling/PooledDecalActor.h"
#include "Variant_WildGuns/AI/WildGunsCoverNPC.h"
#include "Variant_WildGuns/AI/WildGunsForegroundNPC.h"
#include "Variant_WildGuns/AI/WildGunsWalkerNPC.h"
#include "Variant_WildGuns/Gameplay/WildGunsDestructibleCover.h"
#include "Variant_WildGuns/Gameplay/WildGunsPickup.h"
#include "Variant_WildGuns/Gameplay/WildGunsProjectile.h"
#include "Variant_WildGuns/UI/WildGunsHUD.h"
#include "Variant_WildGuns/UI/WildGunsUI.h"
#include "Variant_WildGuns/WildGunsCharacter.h"
#include "Variant_WildGuns/WildGunsPlayerController.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWave.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"

AWildGunsGameMode::AWildGunsGameMode()
{
	PrimaryActorTick.bCanEverTick = true;

	DefaultPawnClass = AWildGunsCharacter::StaticClass();
	PlayerControllerClass = AWildGunsPlayerController::StaticClass();
	HUDClass = AWildGunsHUD::StaticClass();

	// Componentes de Pooling centrales
	EnemyProjectilePool = CreateDefaultSubobject<UActorPool>(TEXT("EnemyProjectilePool"));
	EnemyProjectilePool->defaultSize = 25;

	ShotgunProjectilePool = CreateDefaultSubobject<UActorPool>(TEXT("ShotgunProjectilePool"));
	ShotgunProjectilePool->defaultSize = 15;

	WalkerPool = CreateDefaultSubobject<UActorPool>(TEXT("WalkerPool"));
	WalkerPool->defaultSize = 8;

	CoverNPCPool = CreateDefaultSubobject<UActorPool>(TEXT("CoverNPCPool"));
	CoverNPCPool->defaultSize = 6;

	ForegroundNPCPool = CreateDefaultSubobject<UActorPool>(TEXT("ForegroundNPCPool"));
	ForegroundNPCPool->defaultSize = 6;

	PickupPool = CreateDefaultSubobject<UActorPool>(TEXT("PickupPool"));
	PickupPool->defaultSize = 5;

	DecalPool = CreateDefaultSubobject<UActorPool>(TEXT("DecalPool"));
	DecalPool->defaultSize = 40;

	// Clases base predeterminadas en C++
	EnemyProjectileClass = AWildGunsProjectile::StaticClass();
	ShotgunProjectileClass = AWildGunsProjectile::StaticClass();
	WalkerNPCClass = AWildGunsWalkerNPC::StaticClass();
	CoverNPCClass = AWildGunsCoverNPC::StaticClass();
	ForegroundNPCClass = AWildGunsForegroundNPC::StaticClass();
	PickupClass = AWildGunsPickup::StaticClass();
	DecalClass = APooledDecalActor::StaticClass();

	// Intentar cargar Blueprints especializados si están disponibles
	static ConstructorHelpers::FClassFinder<AWildGunsEnemyBase> WalkerBPFinder(TEXT("/Game/Variant_WildGuns/Blueprints/BP_WildGunsWalker.BP_WildGunsWalker_C"));
	if (WalkerBPFinder.Succeeded()) WalkerNPCClass = WalkerBPFinder.Class;

	static ConstructorHelpers::FClassFinder<AWildGunsEnemyBase> CoverBPFinder(TEXT("/Game/Variant_WildGuns/Blueprints/BP_WildGunsCover.BP_WildGunsCover_C"));
	if (CoverBPFinder.Succeeded()) CoverNPCClass = CoverBPFinder.Class;

	static ConstructorHelpers::FClassFinder<AWildGunsEnemyBase> ForegroundBPFinder(TEXT("/Game/Variant_WildGuns/Blueprints/BP_WildGunsForeground.BP_WildGunsForeground_C"));
	if (ForegroundBPFinder.Succeeded()) ForegroundNPCClass = ForegroundBPFinder.Class;

	// Cargar automáticamente pistas de audio y efectos
	static ConstructorHelpers::FObjectFinder<USoundBase> SndBGM(TEXT("/Game/Variant_WildGuns/Audio/BGM_WildGuns_Theme.BGM_WildGuns_Theme"));
	if (SndBGM.Succeeded())
	{
		BGMTheme = SndBGM.Object;
		if (USoundWave* SoundWave = Cast<USoundWave>(BGMTheme))
		{
			SoundWave->bLooping = true;
		}
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> SndVic(TEXT("/Game/Variant_WildGuns/Audio/SFX_Victory_Fanfare.SFX_Victory_Fanfare"));
	if (SndVic.Succeeded()) VictorySound = SndVic.Object;

	static ConstructorHelpers::FObjectFinder<USoundBase> SndGO(TEXT("/Game/Variant_WildGuns/Audio/SFX_Game_Over.SFX_Game_Over"));
	if (SndGO.Succeeded()) GameOverSound = SndGO.Object;
}

void AWildGunsGameMode::BeginPlay()
{
	// Asignar plantillas a los pools si no están fijadas en el editor
	if (!EnemyProjectilePool->actorTemplate && EnemyProjectileClass)
	{
		EnemyProjectilePool->actorTemplate = EnemyProjectileClass;
	}
	if (!ShotgunProjectilePool->actorTemplate && ShotgunProjectileClass)
	{
		ShotgunProjectilePool->actorTemplate = ShotgunProjectileClass;
	}
	if (!WalkerPool->actorTemplate && WalkerNPCClass)
	{
		WalkerPool->actorTemplate = WalkerNPCClass;
	}
	if (!CoverNPCPool->actorTemplate && CoverNPCClass)
	{
		CoverNPCPool->actorTemplate = CoverNPCClass;
	}
	if (!ForegroundNPCPool->actorTemplate && ForegroundNPCClass)
	{
		ForegroundNPCPool->actorTemplate = ForegroundNPCClass;
	}
	if (!PickupPool->actorTemplate && PickupClass)
	{
		PickupPool->actorTemplate = PickupClass;
	}
	if (!DecalPool->actorTemplate && DecalClass)
	{
		DecalPool->actorTemplate = DecalClass;
	}

	Super::BeginPlay();

	FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this);
	if (CurrentLevelName.Contains(TEXT("Menu"), ESearchCase::IgnoreCase))
	{
		return;
	}

	MatchTimeRemaining = MatchDuration;
	MatchState = EWildGunsMatchState::Playing;

	SetupGalleryEnvironment();
	CacheCoversInLevel();

	// Iniciar oleadas escalonadas mediante temporizadores
	GetWorldTimerManager().SetTimer(WalkerSpawnTimer, this, &AWildGunsGameMode::SpawnWalkerFromPool, 4.0f, true, 2.0f);
	GetWorldTimerManager().SetTimer(CoverSpawnTimer, this, &AWildGunsGameMode::SpawnCoverNPCFromPool, 6.0f, true, 4.0f);
	GetWorldTimerManager().SetTimer(ForegroundSpawnTimer, this, &AWildGunsGameMode::SpawnForegroundNPCFromPool, 7.5f, true, 5.0f);

	// Iniciar reproducción de la música de fondo en bucle continuo
	if (BGMTheme)
	{
		if (USoundWave* SoundWave = Cast<USoundWave>(BGMTheme))
		{
			SoundWave->bLooping = true;
		}

		BGMAudioComponent = UGameplayStatics::SpawnSound2D(this, BGMTheme, 0.65f, 1.0f, 0.0f, nullptr, false, false);
		if (BGMAudioComponent)
		{
			BGMAudioComponent->bAutoDestroy = false;
			BGMAudioComponent->OnAudioFinished.RemoveDynamic(this, &AWildGunsGameMode::OnBGMAudioFinished);
			BGMAudioComponent->OnAudioFinished.AddDynamic(this, &AWildGunsGameMode::OnBGMAudioFinished);
		}
	}
}

void AWildGunsGameMode::CacheCoversInLevel()
{
	CachedCovers.Empty();
	TArray<AActor*> FoundCovers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AWildGunsDestructibleCover::StaticClass(), FoundCovers);
	for (AActor* Actor : FoundCovers)
	{
		if (AWildGunsDestructibleCover* Cover = Cast<AWildGunsDestructibleCover>(Actor))
		{
			CachedCovers.Add(Cover);
		}
	}
}

void AWildGunsGameMode::SetupGalleryEnvironment()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// Si ya existen las coberturas, el nivel ya tiene el escenario montado
	TArray<AActor*> ExistingCovers;
	UGameplayStatics::GetAllActorsOfClass(World, AWildGunsDestructibleCover::StaticClass(), ExistingCovers);
	if (ExistingCovers.Num() >= 3)
	{
		return;
	}

	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UMaterialInterface* FloorMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_TopDark.MI_PrototypeGrid_TopDark"));
	UMaterialInterface* WoodMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_Gray_02.MI_PrototypeGrid_Gray_02"));
	UMaterialInterface* WallMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_Gray.MI_PrototypeGrid_Gray"));
	UMaterialInterface* AccentMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/LevelPrototyping/Materials/M_PrototypeGrid.M_PrototypeGrid"));

	auto SpawnProp = [&](UStaticMesh* Mesh, UMaterialInterface* Mat, const FVector& Loc, const FVector& Scale, const FRotator& Rot = FRotator::ZeroRotator) -> AStaticMeshActor*
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AStaticMeshActor* SMA = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Loc, Rot, Params);
		if (SMA)
		{
			SMA->SetActorScale3D(Scale);
			if (UStaticMeshComponent* SMC = SMA->GetStaticMeshComponent())
			{
				SMC->SetMobility(EComponentMobility::Movable);
				if (Mesh) SMC->SetStaticMesh(Mesh);
				if (Mat) SMC->SetMaterial(0, Mat);
				SMC->SetCollisionProfileName(TEXT("BlockAll"));
			}
		}
		return SMA;
	};

	// 1. Piso de la calle y plataforma frontal
	SpawnProp(CubeMesh, FloorMat, FVector(750.0f, 0.0f, -25.0f), FVector(26.0f, 36.0f, 0.5f));
	SpawnProp(CubeMesh, WoodMat, FVector(0.0f, 0.0f, 5.0f), FVector(5.0f, 26.0f, 0.2f));

	// Postes y barandilla frontal
	SpawnProp(CubeMesh, WoodMat, FVector(50.0f, -1100.0f, 50.0f), FVector(0.3f, 0.3f, 1.0f));
	SpawnProp(CubeMesh, WoodMat, FVector(50.0f, 1100.0f, 50.0f), FVector(0.3f, 0.3f, 1.0f));
	SpawnProp(CubeMesh, WoodMat, FVector(50.0f, -900.0f, 75.0f), FVector(0.15f, 3.5f, 0.15f));
	SpawnProp(CubeMesh, WoodMat, FVector(50.0f, 900.0f, 75.0f), FVector(0.15f, 3.5f, 0.15f));

	// 2. Saloon central de fondo
	SpawnProp(CubeMesh, WallMat, FVector(1500.0f, 0.0f, 225.0f), FVector(1.0f, 22.0f, 4.5f));
	SpawnProp(CubeMesh, WoodMat, FVector(1410.0f, 0.0f, 220.0f), FVector(2.6f, 22.0f, 0.25f));
	SpawnProp(CubeMesh, WoodMat, FVector(1300.0f, 0.0f, 265.0f), FVector(0.15f, 22.0f, 0.7f));
	SpawnProp(CubeMesh, WoodMat, FVector(1480.0f, 0.0f, 465.0f), FVector(1.6f, 14.0f, 0.6f));
	SpawnProp(CubeMesh, AccentMat, FVector(1460.0f, 0.0f, 390.0f), FVector(0.3f, 6.0f, 0.8f));
	SpawnProp(CubeMesh, WoodMat, FVector(1480.0f, 0.0f, 75.0f), FVector(0.2f, 3.2f, 1.5f));

	// Ventanas Saloon
	SpawnProp(CubeMesh, WoodMat, FVector(1480.0f, -500.0f, 95.0f), FVector(0.2f, 2.5f, 1.5f));
	SpawnProp(CubeMesh, WoodMat, FVector(1480.0f, 500.0f, 95.0f), FVector(0.2f, 2.5f, 1.5f));
	SpawnProp(CubeMesh, WoodMat, FVector(1480.0f, -650.0f, 310.0f), FVector(0.2f, 2.0f, 1.4f));
	SpawnProp(CubeMesh, WoodMat, FVector(1480.0f, 0.0f, 310.0f), FVector(0.2f, 2.0f, 1.4f));
	SpawnProp(CubeMesh, WoodMat, FVector(1480.0f, 650.0f, 310.0f), FVector(0.2f, 2.0f, 1.4f));

	// 3. Edificios laterales
	SpawnProp(CubeMesh, WoodMat, FVector(1050.0f, -1150.0f, 200.0f), FVector(9.0f, 1.0f, 4.0f));
	SpawnProp(CubeMesh, FloorMat, FVector(1050.0f, -1080.0f, 380.0f), FVector(8.5f, 2.0f, 0.15f), FRotator(10.0f, 0.0f, 0.0f));
	SpawnProp(CubeMesh, WoodMat, FVector(1050.0f, 1150.0f, 200.0f), FVector(9.0f, 1.0f, 4.0f));
	SpawnProp(CubeMesh, FloorMat, FVector(1050.0f, 1080.0f, 380.0f), FVector(8.5f, 2.0f, 0.15f), FRotator(-10.0f, 0.0f, 0.0f));

	// 4. Barriles y cajas
	SpawnProp(CylinderMesh, WoodMat, FVector(920.0f, -650.0f, 50.0f), FVector(0.8f, 0.8f, 1.0f));
	SpawnProp(CylinderMesh, WoodMat, FVector(920.0f, 650.0f, 50.0f), FVector(0.8f, 0.8f, 1.0f));
	SpawnProp(CubeMesh, WoodMat, FVector(980.0f, -280.0f, 40.0f), FVector(0.8f, 0.8f, 0.8f));
	SpawnProp(CubeMesh, WoodMat, FVector(980.0f, 280.0f, 40.0f), FVector(0.8f, 0.8f, 0.8f));

	// 5. Coberturas destructibles
	const TArray<FVector> CoverPositions = {
		FVector(950.0f, -480.0f, 65.0f),
		FVector(1000.0f, 0.0f, 65.0f),
		FVector(950.0f, 480.0f, 65.0f)
	};
	for (int32 i = 0; i < CoverPositions.Num(); ++i)
	{
		FActorSpawnParameters CoverParams;
		CoverParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AWildGunsDestructibleCover* NewCover = World->SpawnActor<AWildGunsDestructibleCover>(AWildGunsDestructibleCover::StaticClass(), CoverPositions[i], FRotator::ZeroRotator, CoverParams);
		if (NewCover)
		{
			NewCover->SetActorScale3D(FVector(1.0f, 2.4f, 1.3f));
			CachedCovers.Add(NewCover);
		}
	}

	// 6. Iluminación
	TArray<AActor*> FoundLights;
	UGameplayStatics::GetAllActorsOfClass(World, ADirectionalLight::StaticClass(), FoundLights);
	if (FoundLights.Num() == 0)
	{
		FActorSpawnParameters LightParams;
		ADirectionalLight* SunLight = World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), FVector(0.0f, 0.0f, 1500.0f), FRotator(-45.0f, 35.0f, 0.0f), LightParams);
		if (SunLight)
		{
			if (UDirectionalLightComponent* DLC = Cast<UDirectionalLightComponent>(SunLight->GetComponentByClass(UDirectionalLightComponent::StaticClass())))
			{
				DLC->SetMobility(EComponentMobility::Movable);
				DLC->SetIntensity(12.0f);
				DLC->SetLightColor(FLinearColor(1.0f, 0.96f, 0.90f, 1.0f));
				DLC->bAtmosphereSunLight = true;
				DLC->AtmosphereSunLightIndex = 0;
				DLC->SetCastShadows(true);
			}
		}

		ASkyLight* SkyLight = World->SpawnActor<ASkyLight>(ASkyLight::StaticClass(), FVector(0.0f, 0.0f, 800.0f), FRotator::ZeroRotator, LightParams);
		if (SkyLight)
		{
			if (USkyLightComponent* SLC = Cast<USkyLightComponent>(SkyLight->GetComponentByClass(USkyLightComponent::StaticClass())))
			{
				SLC->SetMobility(EComponentMobility::Movable);
				SLC->SetIntensity(3.5f);
				SLC->SetLightColor(FLinearColor(0.85f, 0.92f, 1.0f, 1.0f));
				SLC->bRealTimeCapture = true;
			}
		}
	}
}

void AWildGunsGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MatchState == EWildGunsMatchState::Playing)
	{
		MatchTimeRemaining -= DeltaTime;
		UpdateHUD();

		// CONDICIÓN DE VICTORIA: Sobrevivir el minuto
		if (MatchTimeRemaining <= 0.0f)
		{
			MatchTimeRemaining = 0.0f;
			HandleVictory();
		}
	}
}

void AWildGunsGameMode::UpdateHUD()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	AWildGunsHUD* HUD = Cast<AWildGunsHUD>(PC->GetHUD());
	if (!HUD) return;

	UWildGunsUI* UI = HUD->GetUIWidget();
	if (!UI) return;

	int32 TotalSeconds = FMath::CeilToInt(MatchTimeRemaining);
	int32 Minutes = TotalSeconds / 60;
	int32 Seconds = TotalSeconds % 60;
	FString FormattedTime = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);

	UI->UpdateTimer(MatchTimeRemaining, FormattedTime);

	if (AWildGunsCharacter* WGChar = Cast<AWildGunsCharacter>(PC->GetPawn()))
	{
		UI->UpdateLives(WGChar->GetLives());
		UI->UpdateWeaponStatus(WGChar->GetCurrentWeapon(), WGChar->GetCurrentAmmo());
	}
}

void AWildGunsGameMode::SpawnWalkerFromPool()
{
	if (MatchState != EWildGunsMatchState::Playing) return;

	// Aparición en el fondo (X = 1400 a 1600)
	const bool bFromLeft = FMath::RandBool();
	const float SpawnY = bFromLeft ? -1300.0f : 1300.0f;
	const float DirectionY = bFromLeft ? 1.0f : -1.0f;
	const FVector SpawnLoc = FVector(FMath::RandRange(1400.0f, 1600.0f), SpawnY, 100.0f);
	const FRotator SpawnRot = FRotator(0.0f, (DirectionY > 0.0f) ? 90.0f : -90.0f, 0.0f);

	if (WalkerPool)
	{
		AActor* PooledActor = WalkerPool->GetActorFromPool(SpawnLoc, SpawnRot);
		if (AWildGunsWalkerNPC* Walker = Cast<AWildGunsWalkerNPC>(PooledActor))
		{
			Walker->OwningPool = WalkerPool;
			Walker->SetupWalkDirection(DirectionY);
		}
	}
}

void AWildGunsGameMode::SpawnCoverNPCFromPool()
{
	if (MatchState != EWildGunsMatchState::Playing) return;

	if (CachedCovers.Num() == 0)
	{
		CacheCoversInLevel();
	}

	if (CachedCovers.Num() > 0)
	{
		// Seleccionar una cobertura disponible
		int32 Index = FMath::RandRange(0, CachedCovers.Num() - 1);
		AWildGunsDestructibleCover* SelectedCover = CachedCovers[Index];
		if (SelectedCover && !SelectedCover->bIsDestroyed)
		{
			FVector CoverLoc = SelectedCover->GetActorLocation();
			// Posicionar al enemigo justo detrás de la cobertura
			FVector SpawnLoc = CoverLoc + FVector(90.0f, 0.0f, 0.0f);

			if (CoverNPCPool)
			{
				AActor* PooledActor = CoverNPCPool->GetActorFromPool(SpawnLoc, FRotator(0.0f, 180.0f, 0.0f));
				if (AWildGunsCoverNPC* CoverNPC = Cast<AWildGunsCoverNPC>(PooledActor))
				{
					CoverNPC->OwningPool = CoverNPCPool;
					CoverNPC->AssignCover(SelectedCover);
				}
			}
		}
	}
}

void AWildGunsGameMode::SpawnForegroundNPCFromPool()
{
	if (MatchState != EWildGunsMatchState::Playing) return;

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn) return;

	const FVector PlayerLoc = PlayerPawn->GetActorLocation();
	const bool bFromLeft = FMath::RandBool();
	const float SpawnY = bFromLeft ? (PlayerLoc.Y - 1400.0f) : (PlayerLoc.Y + 1400.0f);
	// Misma profundidad X que el jugador
	const FVector SpawnLoc = FVector(PlayerLoc.X, SpawnY, PlayerLoc.Z);

	if (ForegroundNPCPool)
	{
		AActor* PooledActor = ForegroundNPCPool->GetActorFromPool(SpawnLoc, FRotator::ZeroRotator);
		if (AWildGunsForegroundNPC* FGEnemy = Cast<AWildGunsForegroundNPC>(PooledActor))
		{
			FGEnemy->OwningPool = ForegroundNPCPool;
		}
	}
}

void AWildGunsGameMode::HandleGameOver()
{
	if (MatchState != EWildGunsMatchState::Playing) return;

	MatchState = EWildGunsMatchState::GameOver;

	GetWorldTimerManager().ClearTimer(WalkerSpawnTimer);
	GetWorldTimerManager().ClearTimer(CoverSpawnTimer);
	GetWorldTimerManager().ClearTimer(ForegroundSpawnTimer);

	if (BGMAudioComponent)
	{
		BGMAudioComponent->Stop();
	}
	if (GameOverSound)
	{
		UGameplayStatics::PlaySound2D(this, GameOverSound);
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		if (AWildGunsHUD* HUD = Cast<AWildGunsHUD>(PC->GetHUD()))
		{
			if (UWildGunsUI* UI = HUD->GetUIWidget())
			{
				UI->ShowGameOverScreen();
			}
		}

		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
		PC->bEnableClickEvents = true;
		PC->bEnableMouseOverEvents = true;
	}
}

void AWildGunsGameMode::HandleVictory()
{
	if (MatchState != EWildGunsMatchState::Playing) return;

	MatchState = EWildGunsMatchState::Victory;

	GetWorldTimerManager().ClearTimer(WalkerSpawnTimer);
	GetWorldTimerManager().ClearTimer(CoverSpawnTimer);
	GetWorldTimerManager().ClearTimer(ForegroundSpawnTimer);

	if (BGMAudioComponent)
	{
		BGMAudioComponent->Stop();
	}
	if (VictorySound)
	{
		UGameplayStatics::PlaySound2D(this, VictorySound);
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		if (AWildGunsHUD* HUD = Cast<AWildGunsHUD>(PC->GetHUD()))
		{
			if (UWildGunsUI* UI = HUD->GetUIWidget())
			{
				UI->ShowVictoryScreen();
			}
		}

		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
		PC->bEnableClickEvents = true;
		PC->bEnableMouseOverEvents = true;
	}
}

void AWildGunsGameMode::OnBGMAudioFinished()
{
	if (MatchState == EWildGunsMatchState::Playing && BGMAudioComponent)
	{
		BGMAudioComponent->Play(0.0f);
	}
}
