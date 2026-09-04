#include "OfficeArena.h"
#include "GameConfig.h"
#include "Components/BoxComponent.h"
#include "NightShiftFloor37.h"

namespace OfficeArenaPrivate
{
	/** Inset from half-extent so spawns sit on walkable edge (~2300 cm for 2500 half). */
	constexpr float SpawnEdgeInsetCm = 200.f;
}

AOfficeArena::AOfficeArena()
{
	PrimaryActorTick.bCanEverTick = false;

	BoundsVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("BoundsVolume"));
	SetRootComponent(BoundsVolume);
	// DESIGN: ~50x50 m → 2500 cm half-extent each XY axis if centered.
	BoundsVolume->SetBoxExtent(FVector(2500.f, 2500.f, 1000.f));
	BoundsVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	CeilingClamp = CreateDefaultSubobject<UBoxComponent>(TEXT("CeilingClamp"));
	CeilingClamp->SetupAttachment(BoundsVolume);
	CeilingClamp->SetBoxExtent(FVector(2500.f, 2500.f, 50.f));
	CeilingClamp->SetRelativeLocation(FVector(0.f, 0.f, 1600.f)); // ~ atrium + headroom
	CeilingClamp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	// Default cubicle-ish low cover around atrium (~800 cm from center, ~1 m tall).
	DefaultCover_CubicleN = CreateDefaultSubobject<UBoxComponent>(TEXT("DefaultCover_CubicleN"));
	DefaultCover_CubicleE = CreateDefaultSubobject<UBoxComponent>(TEXT("DefaultCover_CubicleE"));
	DefaultCover_CubicleS = CreateDefaultSubobject<UBoxComponent>(TEXT("DefaultCover_CubicleS"));
	DefaultCover_CubicleW = CreateDefaultSubobject<UBoxComponent>(TEXT("DefaultCover_CubicleW"));
	DefaultCover_ResinNE = CreateDefaultSubobject<UBoxComponent>(TEXT("DefaultCover_ResinNE"));
	DefaultCover_ResinSW = CreateDefaultSubobject<UBoxComponent>(TEXT("DefaultCover_ResinSW"));

	const FVector CubicleExtent(220.f, 80.f, 55.f); // low desk/cubicle height
	SetupDefaultCoverVolume(DefaultCover_CubicleN, FVector(0.f, 800.f, 55.f), CubicleExtent);
	SetupDefaultCoverVolume(DefaultCover_CubicleS, FVector(0.f, -800.f, 55.f), CubicleExtent);
	SetupDefaultCoverVolume(DefaultCover_CubicleE, FVector(800.f, 0.f, 55.f), FVector(80.f, 220.f, 55.f));
	SetupDefaultCoverVolume(DefaultCover_CubicleW, FVector(-800.f, 0.f, 55.f), FVector(80.f, 220.f, 55.f));

	const FVector ResinExtent(120.f, 120.f, 70.f); // barrel cluster footprint
	SetupDefaultCoverVolume(DefaultCover_ResinNE, FVector(1100.f, 1100.f, 70.f), ResinExtent);
	SetupDefaultCoverVolume(DefaultCover_ResinSW, FVector(-1100.f, -1100.f, 70.f), ResinExtent);

	CoverVolumes.Add(DefaultCover_CubicleN);
	CoverVolumes.Add(DefaultCover_CubicleE);
	CoverVolumes.Add(DefaultCover_CubicleS);
	CoverVolumes.Add(DefaultCover_CubicleW);
	CoverVolumes.Add(DefaultCover_ResinNE);
	CoverVolumes.Add(DefaultCover_ResinSW);
}

void AOfficeArena::SetupDefaultCoverVolume(UBoxComponent* Box, const FVector& RelativeLocation, const FVector& Extent)
{
	if (!Box)
	{
		return;
	}
	Box->SetupAttachment(BoundsVolume);
	Box->SetRelativeLocation(RelativeLocation);
	Box->SetBoxExtent(Extent);
	Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Box->SetHiddenInGame(true);
}

void AOfficeArena::BeginPlay()
{
	Super::BeginPlay();
	SyncLayoutFromConfig();
	RefreshSpawnGather();
}

void AOfficeArena::SyncLayoutFromConfig()
{
	if (GameConfig)
	{
		ArenaHalfExtentCm = GameConfig->ArenaSizeMeters * 50.f; // meters → half-extent cm
		AtriumTowerHeightCm = GameConfig->AtriumTowerHeightMeters * 100.f;
	}

	if (BoundsVolume)
	{
		const FVector Ext = BoundsVolume->GetUnscaledBoxExtent();
		BoundsVolume->SetBoxExtent(FVector(ArenaHalfExtentCm, ArenaHalfExtentCm, Ext.Z));
	}
	if (CeilingClamp)
	{
		const FVector Ext = CeilingClamp->GetUnscaledBoxExtent();
		CeilingClamp->SetBoxExtent(FVector(ArenaHalfExtentCm, ArenaHalfExtentCm, Ext.Z));
		// Sit ceiling just above atrium tower + headroom (~2 m).
		const float CeilingZ = AtriumTowerHeightCm + 200.f;
		CeilingClamp->SetRelativeLocation(FVector(0.f, 0.f, CeilingZ));
	}
}

void AOfficeArena::GatherSpawnPointsFromActors()
{
	if (SpawnPointActors.Num() == 0)
	{
		return;
	}
	AlienSpawnPoints.Reset();
	for (AActor* Marker : SpawnPointActors)
	{
		if (Marker)
		{
			AlienSpawnPoints.Add(Marker->GetActorTransform());
		}
	}
}

void AOfficeArena::EnsureDefaultSpawns()
{
	const int32 Expected = GameConfig ? GameConfig->AlienSpawnPointCount : 8;
	if (AlienSpawnPoints.Num() >= Expected)
	{
		return;
	}

	const FVector Origin = GetActorLocation();
	const float Edge = FMath::Max(100.f, ArenaHalfExtentCm - OfficeArenaPrivate::SpawnEdgeInsetCm);

	// 8 edge spawns: 4 mid-edge + 4 corners on walkable square (±Edge cm), Z = floor (arena origin Z).
	// Conceptual DESIGN labels (Editor markers still preferred for final art):
	//   Mid N/S  — stairwells
	//   Mid E    — loading dock
	//   Mid W    — elevator bank
	//   Corners  — service corridor approaches
	struct FSpawnDef
	{
		float X;
		float Y;
		const TCHAR* Label;
	};

	const FSpawnDef Defs[] = {
		{ 0.f, Edge, TEXT("Stairwell_N") },
		{ 0.f, -Edge, TEXT("Stairwell_S") },
		{ Edge, 0.f, TEXT("LoadingDock_E") },
		{ -Edge, 0.f, TEXT("ElevatorBank_W") },
		{ Edge, Edge, TEXT("ServiceCorridor_NE") },
		{ -Edge, Edge, TEXT("ServiceCorridor_NW") },
		{ Edge, -Edge, TEXT("ServiceCorridor_SE") },
		{ -Edge, -Edge, TEXT("ServiceCorridor_SW") },
	};

	AlienSpawnPoints.Reset();
	const int32 Count = FMath::Clamp(Expected, 1, static_cast<int32>(UE_ARRAY_COUNT(Defs)));
	for (int32 i = 0; i < Count; ++i)
	{
		const FVector Loc = Origin + FVector(Defs[i].X, Defs[i].Y, 0.f);
		AlienSpawnPoints.Add(FTransform(FRotator::ZeroRotator, Loc));
		UE_LOG(LogNightShift, Verbose, TEXT("AOfficeArena default spawn[%d] %s @ %s"),
			i, Defs[i].Label, *Loc.ToCompactString());
	}

	UE_LOG(LogNightShift, Log,
		TEXT("AOfficeArena: EnsureDefaultSpawns created %d procedural edge spawns (half=%.0f edge=%.0f). Editor markers still recommended."),
		AlienSpawnPoints.Num(), ArenaHalfExtentCm, Edge);
}

void AOfficeArena::RefreshSpawnGather()
{
	GatherSpawnPointsFromActors();
	EnsureDefaultSpawns();

	const int32 Expected = GameConfig ? GameConfig->AlienSpawnPointCount : 8;
	if (AlienSpawnPoints.Num() < Expected)
	{
		UE_LOG(LogNightShift, Warning, TEXT("AOfficeArena: expected %d spawn points, have %d."),
			Expected, AlienSpawnPoints.Num());
	}
}

FTransform AOfficeArena::GetFarthestSpawnFrom(const FVector& WorldLocation) const
{
	FTransform Best = AlienSpawnPoints.Num() > 0 ? AlienSpawnPoints[0] : FTransform::Identity;
	float BestDistSq = -1.f;
	for (const FTransform& T : AlienSpawnPoints)
	{
		const float DistSq = FVector::DistSquared(T.GetLocation(), WorldLocation);
		if (DistSq > BestDistSq)
		{
			BestDistSq = DistSq;
			Best = T;
		}
	}
	return Best;
}

bool AOfficeArena::IsInsideBounds(const FVector& WorldLocation) const
{
	if (!BoundsVolume)
	{
		return true;
	}
	return BoundsVolume->OverlapPoint(WorldLocation);
}

FVector AOfficeArena::ClampToBounds(const FVector& WorldLocation) const
{
	if (!BoundsVolume)
	{
		return WorldLocation;
	}

	// OBB clamp into BoundsVolume local box (handles arena rotation).
	const FTransform CompTM = BoundsVolume->GetComponentTransform();
	FVector Local = CompTM.InverseTransformPosition(WorldLocation);
	const FVector Ext = BoundsVolume->GetUnscaledBoxExtent();
	Local.X = FMath::Clamp(Local.X, -Ext.X, Ext.X);
	Local.Y = FMath::Clamp(Local.Y, -Ext.Y, Ext.Y);
	Local.Z = FMath::Clamp(Local.Z, -Ext.Z, Ext.Z);
	FVector Clamped = CompTM.TransformPosition(Local);

	// Also clamp Z below CeilingClamp underside so nobody leaves through the roof.
	if (CeilingClamp)
	{
		const FVector CeilCenter = CeilingClamp->GetComponentLocation();
		const FVector CeilExtent = CeilingClamp->GetScaledBoxExtent();
		const float CeilingBottomZ = CeilCenter.Z - CeilExtent.Z;
		Clamped.Z = FMath::Min(Clamped.Z, CeilingBottomZ);
	}

	return Clamped;
}

void AOfficeArena::EnforceBoundsOnActor(AActor* Actor)
{
	if (!Actor)
	{
		return;
	}
	const FVector Loc = Actor->GetActorLocation();
	if (IsInsideBounds(Loc))
	{
		// Still enforce ceiling even when XY is inside.
		if (CeilingClamp)
		{
			const FVector CeilCenter = CeilingClamp->GetComponentLocation();
			const FVector CeilExtent = CeilingClamp->GetScaledBoxExtent();
			const float CeilingBottomZ = CeilCenter.Z - CeilExtent.Z;
			if (Loc.Z > CeilingBottomZ)
			{
				Actor->SetActorLocation(FVector(Loc.X, Loc.Y, CeilingBottomZ));
			}
		}
		return;
	}
	Actor->SetActorLocation(ClampToBounds(Loc));
}

void AOfficeArena::RegisterCoverVolume(UBoxComponent* Volume)
{
	if (!Volume)
	{
		return;
	}
	CoverVolumes.AddUnique(Volume);
}

bool AOfficeArena::IsPointInCover(const FVector& WorldLocation) const
{
	for (const TObjectPtr<UBoxComponent>& Vol : CoverVolumes)
	{
		if (Vol && Vol->OverlapPoint(WorldLocation))
		{
			return true;
		}
	}
	return false;
}

bool AOfficeArena::GetNearestCoverPoint(const FVector& WorldLocation, FVector& OutCoverPoint) const
{
	float BestDistSq = TNumericLimits<float>::Max();
	bool bFound = false;
	for (const TObjectPtr<UBoxComponent>& Vol : CoverVolumes)
	{
		if (!Vol)
		{
			continue;
		}
		const FVector Center = Vol->GetComponentLocation();
		const float DistSq = FVector::DistSquared(WorldLocation, Center);
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			OutCoverPoint = Center;
			bFound = true;
		}
	}
	return bFound;
}
