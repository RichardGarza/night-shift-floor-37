#include "OfficeArena.h"
#include "GameConfig.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "NightShiftFloor37.h"

namespace OfficeArenaPrivate
{
	/** Point-in-box test in the box's local space (UBoxComponent has no OverlapPoint). */
	bool BoxContainsPoint(const UBoxComponent* Box, const FVector& WorldPoint)
	{
		if (!Box)
		{
			return false;
		}
		const FVector Local = Box->GetComponentTransform().InverseTransformPosition(WorldPoint);
		const FVector Extent = Box->GetUnscaledBoxExtent();
		return FMath::Abs(Local.X) <= Extent.X
			&& FMath::Abs(Local.Y) <= Extent.Y
			&& FMath::Abs(Local.Z) <= Extent.Z;
	}

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
	// Bounds/ceiling are enforced by ClampToBounds math, not physics — keep them out of every trace.
	BoundsVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoundsVolume->SetCollisionResponseToAllChannels(ECR_Ignore);

	CeilingClamp = CreateDefaultSubobject<UBoxComponent>(TEXT("CeilingClamp"));
	CeilingClamp->SetupAttachment(BoundsVolume);
	CeilingClamp->SetBoxExtent(FVector(2500.f, 2500.f, 50.f));
	CeilingClamp->SetRelativeLocation(FVector(0.f, 0.f, 1600.f)); // ~ atrium + headroom
	CeilingClamp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CeilingClamp->SetCollisionResponseToAllChannels(ECR_Ignore);

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
	SetupDefaultCoverVolume(DefaultCover_RackStack_A, FVector(1500.f, -550.f, 120.f), RackStackExtent);
	SetupDefaultCoverVolume(DefaultCover_RackStack_B, FVector(-1500.f, 450.f, 120.f), RackStackExtent);
	// Angled unit — yaw 35° so AI cover queries see a rotated OBB.
	SetupDefaultCoverVolumeRotated(
		DefaultCover_RackAngled,
		FVector(400.f, 1500.f, 90.f),
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

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMat(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	GreyboxCubeMesh = CubeMesh.Succeeded() ? CubeMesh.Object : nullptr;
	GreyboxMaterial = ShapeMat.Succeeded() ? ShapeMat.Object : nullptr;
	BuildGreybox();
	BuildGreyboxLighting();
}

UStaticMeshComponent* AOfficeArena::AddGreyboxBox(const FString& Name, const FVector& Center, const FVector& Size, const FRotator& Rot, const FLinearColor& Color)
{
	UStaticMeshComponent* M = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
	M->SetupAttachment(BoundsVolume);
	M->SetRelativeLocation(Center);
	M->SetRelativeRotation(Rot);
	M->SetRelativeScale3D(Size / 100.f); // engine cube is 100 cm
	if (GreyboxCubeMesh)
	{
		M->SetStaticMesh(GreyboxCubeMesh);
	}
	if (GreyboxMaterial)
	{
		M->SetMaterial(0, GreyboxMaterial);
	}
	M->SetCollisionProfileName(TEXT("BlockAll"));
	M->SetCastShadow(true);
	GreyboxMeshes.Add(M);
	GreyboxColors.Add(Color);
	return M;
}

UStaticMeshComponent* AOfficeArena::AddGreyboxRamp(const FString& Name, const FVector& SurfaceStart, const FVector& SurfaceEnd, float Width, const FLinearColor& Color)
{
	const FVector D = SurfaceEnd - SurfaceStart;
	const float Horiz = D.Size2D();
	const float Len = D.Size();
	const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(D.Y, D.X));
	const float Pitch = FMath::RadiansToDegrees(FMath::Atan2(D.Z, Horiz)); // +pitch raises the +X end
	const FVector Center = (SurfaceStart + SurfaceEnd) * 0.5f + FVector(0.f, 0.f, -10.f);
	return AddGreyboxBox(Name, Center, FVector(Len, Width, 20.f), FRotator(Pitch, Yaw, 0.f), Color);
}

void AOfficeArena::BuildGreybox()
{
	// Colours: wet office floor, dirty glass perimeter, concrete tower. Cover is coloured in SetupDefaultCoverVolumeRotated.
	const FLinearColor Floor(0.07f, 0.09f, 0.08f);
	const FLinearColor Glass(0.10f, 0.15f, 0.17f);
	const FLinearColor Concrete(0.30f, 0.30f, 0.28f);
	const FLinearColor Steel(0.22f, 0.24f, 0.26f);
	const float Half = 2500.f; // ctor default; SyncLayoutFromConfig rescales the bounds, not the greybox

	AddGreyboxBox(TEXT("GB_Floor"), FVector(0.f, 0.f, -10.f), FVector(Half * 2.f, Half * 2.f, 20.f), FRotator::ZeroRotator, Floor);
	AddGreyboxBox(TEXT("GB_WallN"), FVector(0.f, Half + 10.f, 160.f), FVector(Half * 2.f + 40.f, 20.f, 320.f), FRotator::ZeroRotator, Glass);
	AddGreyboxBox(TEXT("GB_WallS"), FVector(0.f, -Half - 10.f, 160.f), FVector(Half * 2.f + 40.f, 20.f, 320.f), FRotator::ZeroRotator, Glass);
	AddGreyboxBox(TEXT("GB_WallE"), FVector(Half + 10.f, 0.f, 160.f), FVector(20.f, Half * 2.f, 320.f), FRotator::ZeroRotator, Glass);
	AddGreyboxBox(TEXT("GB_WallW"), FVector(-Half - 10.f, 0.f, 160.f), FVector(20.f, Half * 2.f, 320.f), FRotator::ZeroRotator, Glass);

	// Atrium tower (DESIGN: 3 open levels, ramps, no rails, ~14 m). Levels 467 / 933 / 1400.
	const float L1 = 467.f, L2 = 933.f, L3 = 1400.f;
	const float Lane = 570.f;   // ramp / bridge centre-line, just outside the 900 cm plates
	const float LaneW = 240.f;
	AddGreyboxBox(TEXT("GB_Column"), FVector(0.f, 0.f, 700.f), FVector(120.f, 120.f, 1400.f), FRotator::ZeroRotator, Concrete);
	AddGreyboxBox(TEXT("GB_PlateL1"), FVector(0.f, 0.f, L1 - 15.f), FVector(900.f, 900.f, 30.f), FRotator::ZeroRotator, Concrete);
	AddGreyboxBox(TEXT("GB_PlateL2"), FVector(0.f, 0.f, L2 - 15.f), FVector(900.f, 900.f, 30.f), FRotator::ZeroRotator, Concrete);
	AddGreyboxBox(TEXT("GB_PlateL3"), FVector(0.f, 0.f, L3 - 15.f), FVector(900.f, 900.f, 30.f), FRotator::ZeroRotator, Concrete);
	// Spiral: south ramp up to L1, west ramp to L2, east ramp to L3, flat bridges between, each
	// bridge touching its plate edge so you can step across.
	AddGreyboxRamp(TEXT("GB_Ramp1"), FVector(1290.f, -Lane, 0.f), FVector(150.f, -Lane, L1), LaneW, Steel);
	AddGreyboxBox(TEXT("GB_Bridge1"), FVector(-210.f, -Lane, L1 - 15.f), FVector(720.f, LaneW, 30.f), FRotator::ZeroRotator, Steel);
	AddGreyboxRamp(TEXT("GB_Ramp2"), FVector(-Lane, -Lane, L1), FVector(-Lane, Lane, L2), LaneW, Steel);
	AddGreyboxBox(TEXT("GB_Bridge2"), FVector(0.f, Lane, L2 - 15.f), FVector(1140.f + LaneW, LaneW, 30.f), FRotator::ZeroRotator, Steel);
	AddGreyboxRamp(TEXT("GB_Ramp3"), FVector(Lane, Lane, L2), FVector(Lane, -Lane, L3), LaneW, Steel);
	AddGreyboxBox(TEXT("GB_Bridge3"), FVector(0.f, -Lane, L3 - 15.f), FVector(1140.f + LaneW, LaneW, 30.f), FRotator::ZeroRotator, Steel);

	// Raised conference pad with a broken glass wall, and a collapsed drywall berm (DESIGN).
	AddGreyboxBox(TEXT("GB_ConfPad"), FVector(1500.f, 900.f, 35.f), FVector(800.f, 600.f, 70.f), FRotator::ZeroRotator, Concrete);
	AddGreyboxBox(TEXT("GB_ConfGlass"), FVector(1500.f, 1210.f, 160.f), FVector(600.f, 16.f, 180.f), FRotator::ZeroRotator, Glass);
	AddGreyboxBox(TEXT("GB_Berm"), FVector(-1500.f, -950.f, 35.f), FVector(700.f, 420.f, 70.f), FRotator(0.f, 20.f, 0.f), FLinearColor(0.26f, 0.24f, 0.21f));
}

void AOfficeArena::BuildGreyboxLighting()
{
	// Dying sun through dirty glass: low warm directional light + sky, green-tinted fog, practicals.
	SkyAtmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("GB_SkyAtmosphere"));
	SkyAtmosphere->SetupAttachment(BoundsVolume);
	SunLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("GB_Sun"));
	SunLight->SetupAttachment(BoundsVolume);
	SunLight->SetRelativeRotation(FRotator(-9.f, 35.f, 0.f));
	SunLight->Intensity = 3.5f;
	SunLight->LightColor = FColor(255, 190, 130);
	SunLight->bAtmosphereSunLight = true;
	SunLight->SetCastShadows(true);
	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("GB_SkyLight"));
	SkyLight->SetupAttachment(BoundsVolume);
	SkyLight->bRealTimeCapture = true;
	SkyLight->Intensity = 1.f;
	Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("GB_Fog"));
	Fog->SetupAttachment(BoundsVolume);
	Fog->FogDensity = 0.02f;
	Fog->FogHeightFalloff = 0.3f;
	Fog->FogInscatteringLuminance = FLinearColor(0.22f, 0.32f, 0.26f);
	Fog->bEnableVolumetricFog = true;
	Fog->VolumetricFogExtinctionScale = 1.5f;
	// Six practicals around the floor: sick green / amber, no shadows (perf rule).
	for (int32 i = 0; i < 6; ++i)
	{
		const float A = FMath::DegreesToRadians(i * 60.f + 30.f);
		UPointLightComponent* L = CreateDefaultSubobject<UPointLightComponent>(*FString::Printf(TEXT("GB_Practical_%d"), i));
		L->SetupAttachment(BoundsVolume);
		L->SetRelativeLocation(FVector(FMath::Cos(A) * 1600.f, FMath::Sin(A) * 1600.f, 330.f));
		L->Intensity = 120.f; // cd — auto exposure is off, keep practicals as accents
		L->AttenuationRadius = 1200.f;
		L->SetCastShadows(false);
		L->LightColor = (i % 2 == 0) ? FColor(120, 255, 150) : FColor(255, 180, 90);
		PracticalLights.Add(L);
	}
}

void AOfficeArena::ApplyGreyboxColors()
{
	for (int32 i = 0; i < GreyboxMeshes.Num(); ++i)
	{
		UStaticMeshComponent* M = GreyboxMeshes[i];
		if (!M)
		{
			continue;
		}
		if (!bBuildGreybox)
		{
			M->SetVisibility(false);
			M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			continue;
		}
		if (UMaterialInstanceDynamic* MID = M->CreateAndSetMaterialInstanceDynamic(0))
		{
			MID->SetVectorParameterValue(TEXT("Color"), GreyboxColors.IsValidIndex(i) ? GreyboxColors[i] : FLinearColor::Gray);
		}
	}
	if (!bBuildGreybox)
	{
		if (SunLight) { SunLight->SetVisibility(false); }
		if (SkyLight) { SkyLight->SetVisibility(false); }
		if (SkyAtmosphere) { SkyAtmosphere->SetVisibility(false); }
		if (Fog) { Fog->SetVisibility(false); }
		for (UPointLightComponent* L : PracticalLights) { if (L) { L->SetVisibility(false); } }
	}
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
	// Cover volumes are consulted only via this actor's own point/line math (IsPointInCover,
	// DoesLineHitCover). They must never block rifle traces, alien LOS probes, or movement.
	Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Box->SetCollisionResponseToAllChannels(ECR_Ignore);
	Box->SetHiddenInGame(true);

	// Matching greybox block so the cover exists physically (blocks shots, LOS, movement).
	const FString BoxName = Box->GetName();
	const bool bResin = BoxName.Contains(TEXT("Resin"));
	const bool bRack = BoxName.Contains(TEXT("Rack"));
	const FLinearColor Color = bResin ? FLinearColor(0.42f, 0.36f, 0.12f) : bRack ? FLinearColor(0.16f, 0.18f, 0.22f) : FLinearColor(0.34f, 0.32f, 0.27f);
	AddGreyboxBox(BoxName + TEXT("_Mesh"), RelativeLocation, Extent * 2.f, RelativeRotation, Color);
}

void AOfficeArena::BeginPlay()
{
	Super::BeginPlay();
	ApplyGreyboxColors();
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
		const FVector Loc = Origin + FVector(Defs[i].X, Defs[i].Y, 100.f); // capsule half-height + clearance
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

FTransform AOfficeArena::GetFarthestUnusedSpawnFrom(const FVector& WorldLocation, const TArray<int32>& ExcludeIndices, int32& OutIndex) const
{
	const bool bUseData = AlienSpawnPointData.Num() > 0;
	const int32 Count = bUseData ? AlienSpawnPointData.Num() : AlienSpawnPoints.Num();
	OutIndex = -1;
	if (Count == 0)
	{
		return FTransform::Identity;
	}

	auto TransformAt = [&](int32 i) -> const FTransform&
	{
		return bUseData ? AlienSpawnPointData[i].Transform : AlienSpawnPoints[i];
	};

	// Pass 1: farthest point not already used this batch. Pass 2 (all excluded): farthest overall.
	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		float BestDistSq = -1.f;
		for (int32 i = 0; i < Count; ++i)
		{
			if (Pass == 0 && ExcludeIndices.Contains(i))
			{
				continue;
			}
			const float DistSq = FVector::DistSquared(TransformAt(i).GetLocation(), WorldLocation);
			if (DistSq > BestDistSq)
			{
				BestDistSq = DistSq;
				OutIndex = i;
			}
		}
		if (OutIndex >= 0)
		{
			break;
		}
	}
	return TransformAt(OutIndex);
}

bool AOfficeArena::IsInsideBounds(const FVector& WorldLocation) const
{
	if (!BoundsVolume)
	{
		return true;
	}
	return OfficeArenaPrivate::BoxContainsPoint(BoundsVolume, WorldLocation);
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
		if (OfficeArenaPrivate::BoxContainsPoint(Vol, WorldLocation))
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
