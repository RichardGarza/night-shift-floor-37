#include "OfficeArena.h"
#include "GameConfig.h"
#include "Components/BoxComponent.h"
#include "NightShiftFloor37.h"

namespace OfficeArenaPrivate
{
	/** Inset from half-extent so spawns sit on walkable edge (~2300 cm for 2500 half). */
	constexpr float SpawnEdgeInsetCm = 200.f;

	/** Line vs local AABB (slab method). LocalStart/End in box space; Extent is half-size. */
	bool LineIntersectsLocalAABB(const FVector& LocalStart, const FVector& LocalEnd, const FVector& Extent)
	{
		const FVector Dir = LocalEnd - LocalStart;
		float TMin = 0.f;
		float TMax = 1.f;

		auto ClipAxis = [&](float Start, float D, float MinB, float MaxB) -> bool
		{
			if (FMath::Abs(D) < KINDA_SMALL_NUMBER)
			{
				return Start >= MinB && Start <= MaxB;
			}
			const float InvD = 1.f / D;
			float T1 = (MinB - Start) * InvD;
			float T2 = (MaxB - Start) * InvD;
			if (T1 > T2)
			{
				Swap(T1, T2);
			}
			TMin = FMath::Max(TMin, T1);
			TMax = FMath::Min(TMax, T2);
			return TMin <= TMax;
		};

		if (!ClipAxis(LocalStart.X, Dir.X, -Extent.X, Extent.X))
		{
			return false;
		}
		if (!ClipAxis(LocalStart.Y, Dir.Y, -Extent.Y, Extent.Y))
		{
			return false;
		}
		if (!ClipAxis(LocalStart.Z, Dir.Z, -Extent.Z, Extent.Z))
		{
			return false;
		}
		return true;
	}
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

	// Cubicle maze proxies — low cover ring ~800 cm from atrium center (~1 m tall).
	DefaultCover_CubicleN = CreateDefaultSubobject<UBoxComponent>(TEXT("DefaultCover_CubicleN"));
	DefaultCover_CubicleE = CreateDefaultSubobject<UBoxComponent>(TEXT("DefaultCover_CubicleE"));
	DefaultCover_CubicleS = CreateDefaultSubobject<UBoxComponent>(TEXT("DefaultCover_CubicleS"));
	DefaultCover_CubicleW = CreateDefaultSubobject<UBoxComponent>(TEXT("DefaultCover_CubicleW"));

	// Four resin / egg barrel clusters (DESIGN).
	DefaultCover_ResinNE = CreateDefaultSubobject<UBoxComponent>(TEXT("DefaultCover_ResinNE"));
	DefaultCover_ResinNW = CreateDefaultSubobject<UBoxComponent>(TEXT("DefaultCover_ResinNW"));
	DefaultCover_ResinSE = CreateDefaultSubobject<UBoxComponent>(TEXT("DefaultCover_ResinSE"));
	DefaultCover_ResinSW = CreateDefaultSubobject<UBoxComponent>(TEXT("DefaultCover_ResinSW"));

	// Server-rack / IT-cage proxies (DESIGN: 6 racks — two stacked, one angled).
	DefaultCover_RackStack_A = CreateDefaultSubobject<UBoxComponent>(TEXT("DefaultCover_RackStack_A"));
	DefaultCover_RackStack_B = CreateDefaultSubobject<UBoxComponent>(TEXT("DefaultCover_RackStack_B"));
	DefaultCover_RackAngled = CreateDefaultSubobject<UBoxComponent>(TEXT("DefaultCover_RackAngled"));

	const FVector CubicleExtent(220.f, 80.f, 55.f);
	SetupDefaultCoverVolume(DefaultCover_CubicleN, FVector(0.f, 800.f, 55.f), CubicleExtent);
	SetupDefaultCoverVolume(DefaultCover_CubicleS, FVector(0.f, -800.f, 55.f), CubicleExtent);
	SetupDefaultCoverVolume(DefaultCover_CubicleE, FVector(800.f, 0.f, 55.f), FVector(80.f, 220.f, 55.f));
	SetupDefaultCoverVolume(DefaultCover_CubicleW, FVector(-800.f, 0.f, 55.f), FVector(80.f, 220.f, 55.f));

	const FVector ResinExtent(120.f, 120.f, 70.f);
	SetupDefaultCoverVolume(DefaultCover_ResinNE, FVector(1100.f, 1100.f, 70.f), ResinExtent);
	SetupDefaultCoverVolume(DefaultCover_ResinNW, FVector(-1100.f, 1100.f, 70.f), ResinExtent);
	SetupDefaultCoverVolume(DefaultCover_ResinSE, FVector(1100.f, -1100.f, 70.f), ResinExtent);
	SetupDefaultCoverVolume(DefaultCover_ResinSW, FVector(-1100.f, -1100.f, 70.f), ResinExtent);

	// Stacked racks: taller query boxes (two units stacked ≈ ~240 cm tall proxy).
	const FVector RackStackExtent(90.f, 60.f, 120.f);
	SetupDefaultCoverVolume(DefaultCover_RackStack_A, FVector(450.f, -550.f, 120.f), RackStackExtent);
	SetupDefaultCoverVolume(DefaultCover_RackStack_B, FVector(-500.f, 450.f, 120.f), RackStackExtent);
	// Angled unit — yaw 35° so AI cover queries see a rotated OBB.
	SetupDefaultCoverVolumeRotated(
		DefaultCover_RackAngled,
		FVector(200.f, 600.f, 90.f),
		FVector(90.f, 60.f, 90.f),
		FRotator(0.f, 35.f, 0.f));

	CoverVolumes.Reset();
	CoverVolumes.Add(DefaultCover_CubicleN);
	CoverVolumes.Add(DefaultCover_CubicleE);
	CoverVolumes.Add(DefaultCover_CubicleS);
	CoverVolumes.Add(DefaultCover_CubicleW);
	CoverVolumes.Add(DefaultCover_ResinNE);
	CoverVolumes.Add(DefaultCover_ResinNW);
	CoverVolumes.Add(DefaultCover_ResinSE);
	CoverVolumes.Add(DefaultCover_ResinSW);
	CoverVolumes.Add(DefaultCover_RackStack_A);
	CoverVolumes.Add(DefaultCover_RackStack_B);
	CoverVolumes.Add(DefaultCover_RackAngled);
}

void AOfficeArena::SetupDefaultCoverVolume(UBoxComponent* Box, const FVector& RelativeLocation, const FVector& Extent)
{
	SetupDefaultCoverVolumeRotated(Box, RelativeLocation, Extent, FRotator::ZeroRotator);
}

void AOfficeArena::SetupDefaultCoverVolumeRotated(
	UBoxComponent* Box,
	const FVector& RelativeLocation,
	const FVector& Extent,
	const FRotator& RelativeRotation)
{
	if (!Box)
	{
		return;
	}
	Box->SetupAttachment(BoundsVolume);
	Box->SetRelativeLocation(RelativeLocation);
	Box->SetRelativeRotation(RelativeRotation);
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
		const float CeilingZ = AtriumTowerHeightCm + 200.f;
		CeilingClamp->SetRelativeLocation(FVector(0.f, 0.f, CeilingZ));
	}
}

void AOfficeArena::SyncSpawnTransformsFromData()
{
	AlienSpawnPoints.Reset();
	AlienSpawnPoints.Reserve(AlienSpawnPointData.Num());
	for (const FOfficeArenaSpawnPoint& Pt : AlienSpawnPointData)
	{
		AlienSpawnPoints.Add(Pt.Transform);
	}
}

void AOfficeArena::GatherSpawnPointsFromActors()
{
	if (SpawnPointActors.Num() == 0)
	{
		return;
	}

	AlienSpawnPointData.Reset();
	AlienSpawnPoints.Reset();

	int32 FallbackIndex = 0;
	for (AActor* Marker : SpawnPointActors)
	{
		if (!Marker)
		{
			continue;
		}

		FOfficeArenaSpawnPoint Pt;
		Pt.Transform = Marker->GetActorTransform();

		// Prefer actor tags, then actor label/name, else Spawn_N.
		if (Marker->Tags.Num() > 0)
		{
			Pt.Id = Marker->Tags[0];
		}
		else
		{
			const FString Label = Marker->GetActorNameOrLabel();
			if (!Label.IsEmpty() && !Label.StartsWith(TEXT("TargetPoint")) && !Label.StartsWith(TEXT("Actor")))
			{
				Pt.Id = FName(*Label);
			}
			else
			{
				Pt.Id = FName(*FString::Printf(TEXT("Spawn_%d"), FallbackIndex));
			}
		}

		AlienSpawnPointData.Add(Pt);
		++FallbackIndex;
	}

	SyncSpawnTransformsFromData();
}

void AOfficeArena::EnsureDefaultSpawns()
{
	const int32 Expected = GameConfig ? GameConfig->AlienSpawnPointCount : 8;
	if (AlienSpawnPointData.Num() >= Expected || AlienSpawnPoints.Num() >= Expected)
	{
		// Keep transform mirror coherent if only one side was hand-edited.
		if (AlienSpawnPointData.Num() >= Expected && AlienSpawnPoints.Num() < Expected)
		{
			SyncSpawnTransformsFromData();
		}
		return;
	}

	const FVector Origin = GetActorLocation();
	const float Edge = FMath::Max(100.f, ArenaHalfExtentCm - OfficeArenaPrivate::SpawnEdgeInsetCm);

	// DESIGN: 8 fixed edge points — stairwells, loading dock, elevator bank, service corridor.
	struct FSpawnDef
	{
		float X;
		float Y;
		const TCHAR* Id;
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

	AlienSpawnPointData.Reset();
	const int32 Count = FMath::Clamp(Expected, 1, static_cast<int32>(UE_ARRAY_COUNT(Defs)));
	for (int32 i = 0; i < Count; ++i)
	{
		FOfficeArenaSpawnPoint Pt;
		Pt.Id = FName(Defs[i].Id);
		const FVector Loc = Origin + FVector(Defs[i].X, Defs[i].Y, 0.f);
		Pt.Transform = FTransform(FRotator::ZeroRotator, Loc);
		AlienSpawnPointData.Add(Pt);
		UE_LOG(LogNightShift, Verbose, TEXT("AOfficeArena default spawn[%d] %s @ %s"),
			i, Defs[i].Id, *Loc.ToCompactString());
	}

	SyncSpawnTransformsFromData();

	UE_LOG(LogNightShift, Log,
		TEXT("AOfficeArena: EnsureDefaultSpawns created %d labeled edge spawns (half=%.0f edge=%.0f). Editor markers still recommended."),
		AlienSpawnPointData.Num(), ArenaHalfExtentCm, Edge);
}

void AOfficeArena::RefreshSpawnGather()
{
	GatherSpawnPointsFromActors();
	EnsureDefaultSpawns();

	const int32 Expected = GameConfig ? GameConfig->AlienSpawnPointCount : 8;
	const int32 Have = GetSpawnPointCount();
	if (Have < Expected)
	{
		UE_LOG(LogNightShift, Warning, TEXT("AOfficeArena: expected %d spawn points, have %d."),
			Expected, Have);
	}
}

int32 AOfficeArena::GetSpawnPointCount() const
{
	return AlienSpawnPointData.Num() > 0 ? AlienSpawnPointData.Num() : AlienSpawnPoints.Num();
}

bool AOfficeArena::GetSpawnPointById(FName Id, FTransform& OutTransform) const
{
	if (Id.IsNone())
	{
		return false;
	}
	for (const FOfficeArenaSpawnPoint& Pt : AlienSpawnPointData)
	{
		if (Pt.Id == Id)
		{
			OutTransform = Pt.Transform;
			return true;
		}
	}
	return false;
}

FTransform AOfficeArena::GetFarthestSpawnFrom(const FVector& WorldLocation) const
{
	if (AlienSpawnPointData.Num() > 0)
	{
		FTransform Best = AlienSpawnPointData[0].Transform;
		float BestDistSq = -1.f;
		for (const FOfficeArenaSpawnPoint& Pt : AlienSpawnPointData)
		{
			const float DistSq = FVector::DistSquared(Pt.Transform.GetLocation(), WorldLocation);
			if (DistSq > BestDistSq)
			{
				BestDistSq = DistSq;
				Best = Pt.Transform;
			}
		}
		return Best;
	}

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

	const FTransform CompTM = BoundsVolume->GetComponentTransform();
	FVector Local = CompTM.InverseTransformPosition(WorldLocation);
	const FVector Ext = BoundsVolume->GetUnscaledBoxExtent();
	Local.X = FMath::Clamp(Local.X, -Ext.X, Ext.X);
	Local.Y = FMath::Clamp(Local.Y, -Ext.Y, Ext.Y);
	Local.Z = FMath::Clamp(Local.Z, -Ext.Z, Ext.Z);
	FVector Clamped = CompTM.TransformPosition(Local);

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

void AOfficeArena::UnregisterCoverVolume(UBoxComponent* Volume)
{
	if (!Volume)
	{
		return;
	}
	CoverVolumes.Remove(Volume);
}

int32 AOfficeArena::GetCoverVolumeCount() const
{
	int32 Count = 0;
	for (const TObjectPtr<UBoxComponent>& Vol : CoverVolumes)
	{
		if (Vol)
		{
			++Count;
		}
	}
	return Count;
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

bool AOfficeArena::FindCoverAwayFromThreat(
	const FVector& From,
	const FVector& ThreatLocation,
	FVector& OutCoverPoint) const
{
	const bool bLimitRadius = CoverSearchRadiusCm > 0.f;
	const float RadiusSq = CoverSearchRadiusCm * CoverSearchRadiusCm;

	float BestThreatDistSq = -1.f;
	bool bFound = false;

	for (const TObjectPtr<UBoxComponent>& Vol : CoverVolumes)
	{
		if (!Vol)
		{
			continue;
		}
		const FVector Center = Vol->GetComponentLocation();
		if (bLimitRadius)
		{
			const float FromDistSq = FVector::DistSquared(From, Center);
			if (FromDistSq > RadiusSq)
			{
				continue;
			}
		}

		const float ThreatDistSq = FVector::DistSquared(Center, ThreatLocation);
		if (ThreatDistSq > BestThreatDistSq)
		{
			BestThreatDistSq = ThreatDistSq;
			OutCoverPoint = Center;
			bFound = true;
		}
	}
	return bFound;
}

bool AOfficeArena::DoesLineHitCover(const FVector& Start, const FVector& End) const
{
	for (const TObjectPtr<UBoxComponent>& Vol : CoverVolumes)
	{
		if (!Vol)
		{
			continue;
		}
		const FTransform CompTM = Vol->GetComponentTransform();
		const FVector LocalStart = CompTM.InverseTransformPosition(Start);
		const FVector LocalEnd = CompTM.InverseTransformPosition(End);
		const FVector Extent = Vol->GetUnscaledBoxExtent();
		if (OfficeArenaPrivate::LineIntersectsLocalAABB(LocalStart, LocalEnd, Extent))
		{
			return true;
		}
	}
	return false;
}
