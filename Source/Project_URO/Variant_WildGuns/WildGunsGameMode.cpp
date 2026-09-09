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
#include "ImageUtils.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EngineUtils.h"
#include "Misc/Paths.h"

AWildGunsGameMode::AWildGunsGameMode()
{
	PrimaryActorTick.bCanEverTick = true;

	DefaultPawnClass = AWildGunsCharacter::StaticClass();
	PlayerControllerClass = AWildGunsPlayerController::StaticClass();
	HUDClass = AWildGunsHUD::StaticClass();

	// Componentes de Pooling centrales
	EnemyProjectilePool = CreateDefaultSubobject<UActorPool>(TEXT("EnemyProjectilePool"));
	EnemyProjectilePool->defaultSize = 25;

	PlayerProjectilePool = CreateDefaultSubobject<UActorPool>(TEXT("PlayerProjectilePool"));
	PlayerProjectilePool->defaultSize = 40;

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
	PlayerProjectileClass = AWildGunsProjectile::StaticClass();
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
	if (!PlayerProjectilePool->actorTemplate && PlayerProjectileClass)
	{
		PlayerProjectilePool->actorTemplate = PlayerProjectileClass;
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
	ApplyWesternAesthetics();

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

void AWildGunsGameMode::ApplyWesternAesthetics()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 1. Configuración de Post-Processing estilo Spaghetti Western (Sergio Leone / Technicolor Cálido)
	for (TActorIterator<APostProcessVolume> It(World); It; ++It)
	{
		APostProcessVolume* PPV = *It;
		if (PPV)
		{
			PPV->bUnbound = true;
			FPostProcessSettings& S = PPV->Settings;

			// Temperatura cálida y balance de blancos (sol abrasador del desierto)
			S.bOverride_TemperatureType = true;
			S.TemperatureType = ETemperatureMethod::TEMP_WhiteBalance;
			S.bOverride_WhiteTemp = true;
			S.WhiteTemp = 7100.0f;
			S.bOverride_WhiteTint = true;
			S.WhiteTint = 0.12f;

			// Gradación de color auténtica de celuloide clásico
			S.bOverride_ColorSaturation = true;
			S.ColorSaturation = FVector4(0.85f, 0.82f, 0.78f, 1.0f);

			S.bOverride_ColorContrast = true;
			S.ColorContrast = FVector4(1.20f, 1.16f, 1.12f, 1.0f);

			S.bOverride_ColorGamma = true;
			S.ColorGamma = FVector4(0.98f, 0.97f, 0.94f, 1.0f);

			S.bOverride_ColorGain = true;
			S.ColorGain = FVector4(1.08f, 1.04f, 0.95f, 1.0f);

			S.bOverride_ColorOffset = true;
			S.ColorOffset = FVector4(0.015f, 0.010f, 0.003f, 0.0f);

			// Grano de película de 35mm
			S.bOverride_FilmGrainIntensity = true;
			S.FilmGrainIntensity = 0.42f;

			// Viñeteado de lente clásico
			S.bOverride_VignetteIntensity = true;
			S.VignetteIntensity = 0.52f;

			// Bloom del sol desértico
			S.bOverride_BloomIntensity = true;
			S.BloomIntensity = 0.85f;
		}
	}

	// 2. Calina y polvo desértico en la niebla atmosférica
	for (TActorIterator<AExponentialHeightFog> It(World); It; ++It)
	{
		if (UExponentialHeightFogComponent* FogComp = It->GetComponent())
		{
			FogComp->SetFogDensity(0.012f);
			FogComp->SetFogInscatteringColor(FLinearColor(0.86f, 0.72f, 0.52f, 1.0f));
			FogComp->SetDirectionalInscatteringColor(FLinearColor(1.0f, 0.88f, 0.70f, 1.0f));
		}
	}

	// 3. Sol cálido del desierto
	for (TActorIterator<ADirectionalLight> It(World); It; ++It)
	{
		if (UDirectionalLightComponent* DLC = Cast<UDirectionalLightComponent>(It->GetLightComponent()))
		{
			DLC->SetLightColor(FLinearColor(1.0f, 0.94f, 0.82f));
		}
	}

	// 4. Cargar Material Maestro
	UMaterialInterface* BaseMat = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, TEXT("/Game/Variant_WildGuns/Materials/M_Western_Master")));
	if (!BaseMat)
	{
		BaseMat = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, TEXT("/Game/LevelPrototyping/Materials/M_FlatCol")));
	}

	if (!BaseMat)
	{
		return;
	}

	const FString TexturesDir = FPaths::ProjectContentDir() / TEXT("WesternTextures");

	auto CreateWesternMID = [&](const FString& FileName, float Tiling, float Roughness) -> UMaterialInstanceDynamic*
	{
		const FString FullPath = TexturesDir / FileName;
		UTexture2D* LoadedTex = FImageUtils::ImportFileAsTexture2D(FullPath);
		if (!LoadedTex)
		{
			return nullptr;
		}

		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, this);
		if (MID)
		{
			MID->SetTextureParameterValue(TEXT("BaseTexture"), LoadedTex);
			MID->SetScalarParameterValue(TEXT("Tiling"), Tiling);
			MID->SetScalarParameterValue(TEXT("Roughness"), Roughness);
		}
		return MID;
	};

	UMaterialInstanceDynamic* MID_DustyRoad = CreateWesternMID(TEXT("T_Western_DustyRoad.jpg"), 5.0f, 0.9f);
	UMaterialInstanceDynamic* MID_AgedPlanks = CreateWesternMID(TEXT("T_Western_AgedPlanks.jpg"), 3.0f, 0.8f);
	UMaterialInstanceDynamic* MID_SaloonWall = CreateWesternMID(TEXT("T_Western_SaloonWall.jpg"), 3.0f, 0.85f);
	UMaterialInstanceDynamic* MID_SaloonSign = CreateWesternMID(TEXT("T_Western_SaloonSign.jpg"), 1.0f, 0.7f);
	UMaterialInstanceDynamic* MID_SaloonDoors = CreateWesternMID(TEXT("T_Western_SaloonDoors.jpg"), 1.0f, 0.8f);
	UMaterialInstanceDynamic* MID_RoofShingles = CreateWesternMID(TEXT("T_Western_RoofShingles.jpg"), 4.0f, 0.85f);
	UMaterialInstanceDynamic* MID_WhiskeyBarrel = CreateWesternMID(TEXT("T_Western_WhiskeyBarrel.jpg"), 1.0f, 0.75f);
	UMaterialInstanceDynamic* MID_WoodCrate = CreateWesternMID(TEXT("T_Western_WoodCrate.jpg"), 1.0f, 0.8f);

	// 5. Asignar texturas temáticas a la geometría del escenario en LVL_WildGuns
	for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
	{
		AStaticMeshActor* SMA = *It;
		if (!SMA) continue;

		UStaticMeshComponent* SMC = SMA->GetStaticMeshComponent();
		if (!SMC) continue;

		const FString Label = SMA->GetActorLabel();

		if (Label.Contains(TEXT("Street_Floor")))
		{
			if (MID_DustyRoad) SMC->SetMaterial(0, MID_DustyRoad);
		}
		else if (Label.Contains(TEXT("Boardwalk")) || Label.Contains(TEXT("Post")) || Label.Contains(TEXT("Rail")) || Label.Contains(TEXT("Balcony")))
		{
			if (MID_AgedPlanks) SMC->SetMaterial(0, MID_AgedPlanks);
		}
		else if (Label.Contains(TEXT("Saloon_Sign")))
		{
			if (MID_SaloonSign) SMC->SetMaterial(0, MID_SaloonSign);
		}
		else if (Label.Contains(TEXT("Saloon_Doors")))
		{
			if (MID_SaloonDoors) SMC->SetMaterial(0, MID_SaloonDoors);
		}
		else if (Label.Contains(TEXT("Saloon_Facade")) || Label.Contains(TEXT("Building_Left_Wall")) || Label.Contains(TEXT("Building_Right_Wall")))
		{
			if (MID_SaloonWall) SMC->SetMaterial(0, MID_SaloonWall);
		}
		else if (Label.Contains(TEXT("Roof")) || Label.Contains(TEXT("Cornice")))
		{
			if (MID_RoofShingles) SMC->SetMaterial(0, MID_RoofShingles);
		}
		else if (Label.Contains(TEXT("Barrel")))
		{
			if (MID_WhiskeyBarrel) SMC->SetMaterial(0, MID_WhiskeyBarrel);
		}
		else if (Label.Contains(TEXT("Crate")))
		{
			if (MID_WoodCrate) SMC->SetMaterial(0, MID_WoodCrate);
		}
	}

	// 6. Asignar material a las coberturas destructibles del jugador
	for (AWildGunsDestructibleCover* Cover : CachedCovers)
	{
		if (Cover && MID_WoodCrate)
		{
			TArray<UStaticMeshComponent*> MeshComps;
			Cover->GetComponents<UStaticMeshComponent>(MeshComps);
			for (UStaticMeshComponent* Comp : MeshComps)
			{
				Comp->SetMaterial(0, MID_WoodCrate);
			}
		}
	}
}
