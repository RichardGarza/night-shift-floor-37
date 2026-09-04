#include "FXPoolInterface.h"
#include "GameConfig.h"
#include "NightShiftFloor37.h"
#include "Components/SceneComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/World.h"

// ---------------------------------------------------------------------------
// APooledTracerActor — placeholder visual; replace with Niagara later
// ---------------------------------------------------------------------------

APooledTracerActor::APooledTracerActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

void APooledTracerActor::Activate(const FVector& Start, const FVector& End, float DurationSeconds)
{
	TracerStart = Start;
	TracerEnd = End;
	TimeRemaining = FMath::Max(DurationSeconds, 0.001f);
	bActive = true;
	SetActorLocation(Start);
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);
}

void APooledTracerActor::Deactivate()
{
	bActive = false;
	TimeRemaining = 0.f;
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);
}

void APooledTracerActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bActive)
	{
		return;
	}
	TimeRemaining -= DeltaSeconds;
	if (TimeRemaining <= 0.f)
	{
		Deactivate();
	}
}

// ---------------------------------------------------------------------------
// AFXPoolManager
// ---------------------------------------------------------------------------

AFXPoolManager::AFXPoolManager()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("PoolRoot"));
	SetRootComponent(Root);
}

void AFXPoolManager::BeginPlay()
{
	Super::BeginPlay();
	ApplyPoolSizesFromConfig();
	WarmPools();
	UE_LOG(LogNightShift, Log, TEXT("AFXPoolManager warmed — tracers %d, muzzle lights %d"),
		TracerSlots.Num(), MuzzleSlots.Num());
}

void AFXPoolManager::ApplyPoolSizesFromConfig()
{
	if (!GameConfig)
	{
		return;
	}
	TracerPoolSize = GameConfig->TracerPoolSize;
	MuzzleLightPoolSize = GameConfig->MuzzleLightPoolSize;
}

void AFXPoolManager::WarmPools()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TracerSlots.Reset();
	TracerSlots.Reserve(TracerPoolSize);
	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (int32 i = 0; i < TracerPoolSize; ++i)
	{
		APooledTracerActor* Tracer = World->SpawnActor<APooledTracerActor>(
			APooledTracerActor::StaticClass(), GetActorLocation(), FRotator::ZeroRotator, Params);
		if (Tracer)
		{
			Tracer->Deactivate();
			TracerSlots.Add(Tracer);
		}
	}

	MuzzleSlots.Reset();
	MuzzleSlots.Reserve(MuzzleLightPoolSize);
	for (int32 i = 0; i < MuzzleLightPoolSize; ++i)
	{
		FMuzzleSlot Slot;
		const FName LightName(*FString::Printf(TEXT("MuzzleLight_%d"), i));
		Slot.Light = NewObject<UPointLightComponent>(this, LightName);
		if (Slot.Light)
		{
			Slot.Light->RegisterComponent();
			Slot.Light->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
			Slot.Light->SetVisibility(false);
			Slot.Light->SetIntensity(0.f);
			Slot.Light->SetAttenuationRadius(250.f);
			Slot.Light->SetCastShadows(false);
			Slot.bActive = false;
			Slot.TimeRemaining = 0.f;
			MuzzleSlots.Add(Slot);
		}
	}

	NextTracerIndex = 0;
	NextMuzzleIndex = 0;
}

void AFXPoolManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	for (FMuzzleSlot& Slot : MuzzleSlots)
	{
		if (!Slot.bActive || !Slot.Light)
		{
			continue;
		}
		Slot.TimeRemaining -= DeltaSeconds;
		if (Slot.TimeRemaining <= 0.f)
		{
			Slot.bActive = false;
			Slot.Light->SetVisibility(false);
			Slot.Light->SetIntensity(0.f);
		}
	}
}

void AFXPoolManager::ActivateTracer(const FVector& Start, const FVector& End, float DurationMs)
{
	if (TracerSlots.Num() == 0)
	{
		return;
	}

	const float DurationSec = DurationMs * 0.001f;
	const int32 Count = TracerSlots.Num();

	for (int32 i = 0; i < Count; ++i)
	{
		const int32 Idx = (NextTracerIndex + i) % Count;
		APooledTracerActor* Slot = TracerSlots[Idx];
		if (Slot && Slot->IsAvailable())
		{
			Slot->Activate(Start, End, DurationSec);
			NextTracerIndex = (Idx + 1) % Count;
			return;
		}
	}

	if (APooledTracerActor* Steal = TracerSlots[NextTracerIndex])
	{
		Steal->Activate(Start, End, DurationSec);
		NextTracerIndex = (NextTracerIndex + 1) % Count;
	}
}

void AFXPoolManager::ActivateMuzzleLight(const FVector& WorldLocation, float DurationMs)
{
	if (MuzzleSlots.Num() == 0)
	{
		return;
	}

	const float DurationSec = DurationMs * 0.001f;
	const int32 Count = MuzzleSlots.Num();

	for (int32 i = 0; i < Count; ++i)
	{
		const int32 Idx = (NextMuzzleIndex + i) % Count;
		FMuzzleSlot& Slot = MuzzleSlots[Idx];
		if (!Slot.bActive && Slot.Light)
		{
			Slot.bActive = true;
			Slot.TimeRemaining = DurationSec;
			Slot.Light->SetWorldLocation(WorldLocation);
			Slot.Light->SetVisibility(true);
			Slot.Light->SetIntensity(3000.f);
			NextMuzzleIndex = (Idx + 1) % Count;
			return;
		}
	}

	FMuzzleSlot& Steal = MuzzleSlots[NextMuzzleIndex];
	if (Steal.Light)
	{
		Steal.bActive = true;
		Steal.TimeRemaining = DurationSec;
		Steal.Light->SetWorldLocation(WorldLocation);
		Steal.Light->SetVisibility(true);
		Steal.Light->SetIntensity(3000.f);
		NextMuzzleIndex = (NextMuzzleIndex + 1) % Count;
	}
}
