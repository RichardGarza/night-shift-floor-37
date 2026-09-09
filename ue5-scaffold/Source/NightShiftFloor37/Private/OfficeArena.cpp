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
#include "NightShiftAudio.h"
#include "Kismet/GameplayStatics.h"
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
	PrimaryActorTick.bCanEverTick = true; // Sprint AE — neon flicker only; cheap

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

UStaticMeshComponent* AOfficeArena::AddGreyboxBox(const FString& Name, const FVector& Center, const FVector& Size, const FRotator& Rot, const FLinearColor& Color, EArenaSurface Surface)
{
	// Sprint AC — the cover-volume blocks are added before BuildGreybox, so resolve the engine cube here
	// (constructor context) instead of relying on member order. Without this they had no mesh at all.
	if (!GreyboxCubeMesh || !GreyboxMaterial)
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
		static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMatFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		if (!GreyboxCubeMesh && CubeMeshFinder.Succeeded()) { GreyboxCubeMesh = CubeMeshFinder.Object; }
		if (!GreyboxMaterial && ShapeMatFinder.Succeeded()) { GreyboxMaterial = ShapeMatFinder.Object; }
	}
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
	GreyboxSurfaces.Add(Surface);
	return M;
}

UStaticMeshComponent* AOfficeArena::AddGreyboxRamp(const FString& Name, const FVector& SurfaceStart, const FVector& SurfaceEnd, float Width, const FLinearColor& Color, EArenaSurface Surface)
{
	const FVector D = SurfaceEnd - SurfaceStart;
	const float Horiz = D.Size2D();
	const float Len = D.Size();
	const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(D.Y, D.X));
	const float Pitch = FMath::RadiansToDegrees(FMath::Atan2(D.Z, Horiz)); // +pitch raises the +X end
	const FVector Center = (SurfaceStart + SurfaceEnd) * 0.5f + FVector(0.f, 0.f, -10.f);
	return AddGreyboxBox(Name, Center, FVector(Len, Width, 20.f), FRotator(Pitch, Yaw, 0.f), Color, Surface);
}

void AOfficeArena::BuildGreybox()
{
	// Colours: wet office floor, dirty glass perimeter, concrete tower. Cover is coloured in SetupDefaultCoverVolumeRotated.
	// Sprint X — lifted albedos (Richard: "don't want it so dark"); still wet/dirty, not white.
	const FLinearColor Floor(0.20f, 0.23f, 0.22f);
	const FLinearColor Glass(0.16f, 0.22f, 0.25f);
	const FLinearColor Concrete(0.42f, 0.42f, 0.40f);
	const FLinearColor Steel(0.32f, 0.34f, 0.36f);
	const float Half = 2500.f; // ctor default; SyncLayoutFromConfig rescales the bounds, not the greybox

	AddGreyboxBox(TEXT("GB_Floor"), FVector(0.f, 0.f, -10.f), FVector(Half * 2.f, Half * 2.f, 20.f), FRotator::ZeroRotator, Floor, EArenaSurface::Floor);
	// Perimeter: a painted wall (Sprint AC — Richard: "the walls need texture") with a dirty-glass band
	// above it up to the 3.2 m bounds wall, and an emissive trim strip at the seam.
	const float Spandrel = 200.f; // UGameConfig::PerimeterWallHeightCm mirrors this default
	AddGreyboxBox(TEXT("GB_WallN"), FVector(0.f, Half + 10.f, Spandrel + (320.f - Spandrel) * 0.5f), FVector(Half * 2.f + 40.f, 20.f, 320.f - Spandrel), FRotator::ZeroRotator, Glass, EArenaSurface::Glass);
	AddGreyboxBox(TEXT("GB_WallS"), FVector(0.f, -Half - 10.f, Spandrel + (320.f - Spandrel) * 0.5f), FVector(Half * 2.f + 40.f, 20.f, 320.f - Spandrel), FRotator::ZeroRotator, Glass, EArenaSurface::Glass);
	AddGreyboxBox(TEXT("GB_WallE"), FVector(Half + 10.f, 0.f, Spandrel + (320.f - Spandrel) * 0.5f), FVector(20.f, Half * 2.f, 320.f - Spandrel), FRotator::ZeroRotator, Glass, EArenaSurface::Glass);
	AddGreyboxBox(TEXT("GB_WallW"), FVector(-Half - 10.f, 0.f, Spandrel + (320.f - Spandrel) * 0.5f), FVector(20.f, Half * 2.f, 320.f - Spandrel), FRotator::ZeroRotator, Glass, EArenaSurface::Glass);
	const FLinearColor Paint(0.36f, 0.38f, 0.34f);
	AddGreyboxBox(TEXT("GB_SpandrelN"), FVector(0.f, Half + 10.f, Spandrel * 0.5f), FVector(Half * 2.f + 40.f, 24.f, Spandrel), FRotator::ZeroRotator, Paint, EArenaSurface::Wall);
	AddGreyboxBox(TEXT("GB_SpandrelS"), FVector(0.f, -Half - 10.f, Spandrel * 0.5f), FVector(Half * 2.f + 40.f, 24.f, Spandrel), FRotator::ZeroRotator, Paint, EArenaSurface::Wall);
	AddGreyboxBox(TEXT("GB_SpandrelE"), FVector(Half + 10.f, 0.f, Spandrel * 0.5f), FVector(24.f, Half * 2.f, Spandrel), FRotator::ZeroRotator, Paint, EArenaSurface::Wall);
	AddGreyboxBox(TEXT("GB_SpandrelW"), FVector(-Half - 10.f, 0.f, Spandrel * 0.5f), FVector(24.f, Half * 2.f, Spandrel), FRotator::ZeroRotator, Paint, EArenaSurface::Wall);
	// Neon trim: sick green on N/S, amber on E/W — the block colour becomes the emissive colour.
	const FLinearColor NeonG(0.35f, 1.0f, 0.55f);
	const FLinearColor NeonA(1.0f, 0.62f, 0.22f);
	const float TrimZ = Spandrel + 4.f;
	AddGreyboxBox(TEXT("GB_TrimN"), FVector(0.f, Half - 6.f, TrimZ), FVector(Half * 2.f, 8.f, 8.f), FRotator::ZeroRotator, NeonG, EArenaSurface::Neon);
	AddGreyboxBox(TEXT("GB_TrimS"), FVector(0.f, -Half + 6.f, TrimZ), FVector(Half * 2.f, 8.f, 8.f), FRotator::ZeroRotator, NeonG, EArenaSurface::Neon);
	AddGreyboxBox(TEXT("GB_TrimE"), FVector(Half - 6.f, 0.f, TrimZ), FVector(8.f, Half * 2.f, 8.f), FRotator::ZeroRotator, NeonA, EArenaSurface::Neon);
	AddGreyboxBox(TEXT("GB_TrimW"), FVector(-Half + 6.f, 0.f, TrimZ), FVector(8.f, Half * 2.f, 8.f), FRotator::ZeroRotator, NeonA, EArenaSurface::Neon);

	// Atrium tower (DESIGN: 3 open levels, ramps, no rails, ~14 m). Levels 467 / 933 / 1400.
	const float L1 = 467.f, L2 = 933.f, L3 = 1400.f;
	const float Lane = 570.f;   // ramp / bridge centre-line, just outside the 900 cm plates
	const float LaneW = 240.f;
	AddGreyboxBox(TEXT("GB_Column"), FVector(0.f, 0.f, 700.f), FVector(120.f, 120.f, 1400.f), FRotator::ZeroRotator, Concrete, EArenaSurface::Concrete);
	AddGreyboxBox(TEXT("GB_PlateL1"), FVector(0.f, 0.f, L1 - 15.f), FVector(900.f, 900.f, 30.f), FRotator::ZeroRotator, Concrete, EArenaSurface::Concrete);
	AddGreyboxBox(TEXT("GB_PlateL2"), FVector(0.f, 0.f, L2 - 15.f), FVector(900.f, 900.f, 30.f), FRotator::ZeroRotator, Concrete, EArenaSurface::Concrete);
	AddGreyboxBox(TEXT("GB_PlateL3"), FVector(0.f, 0.f, L3 - 15.f), FVector(900.f, 900.f, 30.f), FRotator::ZeroRotator, Concrete, EArenaSurface::Concrete);
	// Spiral: south ramp up to L1, west ramp to L2, east ramp to L3, flat bridges between, each
	// bridge touching its plate edge so you can step across.
	AddGreyboxRamp(TEXT("GB_Ramp1"), FVector(1290.f, -Lane, 0.f), FVector(150.f, -Lane, L1), LaneW, Steel, EArenaSurface::Steel);
	AddGreyboxBox(TEXT("GB_Bridge1"), FVector(-210.f, -Lane, L1 - 15.f), FVector(720.f, LaneW, 30.f), FRotator::ZeroRotator, Steel, EArenaSurface::Steel);
	AddGreyboxRamp(TEXT("GB_Ramp2"), FVector(-Lane, -Lane, L1), FVector(-Lane, Lane, L2), LaneW, Steel, EArenaSurface::Steel);
	AddGreyboxBox(TEXT("GB_Bridge2"), FVector(0.f, Lane, L2 - 15.f), FVector(1140.f + LaneW, LaneW, 30.f), FRotator::ZeroRotator, Steel, EArenaSurface::Steel);
	AddGreyboxRamp(TEXT("GB_Ramp3"), FVector(Lane, Lane, L2), FVector(Lane, -Lane, L3), LaneW, Steel, EArenaSurface::Steel);
	AddGreyboxBox(TEXT("GB_Bridge3"), FVector(0.f, -Lane, L3 - 15.f), FVector(1140.f + LaneW, LaneW, 30.f), FRotator::ZeroRotator, Steel, EArenaSurface::Steel);

	// Raised conference pad with a broken glass wall, and a collapsed drywall berm (DESIGN).
	AddGreyboxBox(TEXT("GB_ConfPad"), FVector(1500.f, 900.f, 35.f), FVector(800.f, 600.f, 70.f), FRotator::ZeroRotator, Concrete, EArenaSurface::Concrete);
	AddGreyboxBox(TEXT("GB_ConfGlass"), FVector(1500.f, 1210.f, 160.f), FVector(600.f, 16.f, 180.f), FRotator::ZeroRotator, Glass, EArenaSurface::Glass);
	AddGreyboxBox(TEXT("GB_Berm"), FVector(-1500.f, -950.f, 35.f), FVector(700.f, 420.f, 70.f), FRotator(0.f, 20.f, 0.f), FLinearColor(0.26f, 0.24f, 0.21f), EArenaSurface::Berm);
}

void AOfficeArena::BuildGreyboxLighting()
{
	// Sprint X — third lift (V was not enough): brighter key + fill, thin fog, hot practicals,
	// plus UGameConfig::ExposureBiasEV on the player camera. Mood kept via colour, not darkness.
	SkyAtmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("GB_SkyAtmosphere"));
	SkyAtmosphere->SetupAttachment(BoundsVolume);

	SunLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("GB_Sun"));
	SunLight->SetupAttachment(BoundsVolume);
	// Slightly higher late-day angle — fewer crushed floor pools, still long shadows.
	SunLight->SetRelativeRotation(FRotator(-24.f, 38.f, 0.f));
	SunLight->Intensity = 5.0f; // Sprint X — was 3.25
	SunLight->LightColor = FColor(255, 190, 145);
	SunLight->bAtmosphereSunLight = true;
	SunLight->SetCastShadows(true);

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("GB_SkyLight"));
	SkyLight->SetupAttachment(BoundsVolume);
	SkyLight->bRealTimeCapture = true;
	SkyLight->Intensity = 1.6f; // Sprint X — was 0.9; fill lifts the shadow sides of cover

	Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("GB_Fog"));
	Fog->SetupAttachment(BoundsVolume);
	Fog->FogDensity = 0.009f; // Sprint X — was 0.016
	Fog->FogHeightFalloff = 0.45f;
	Fog->FogInscatteringLuminance = FLinearColor(0.30f, 0.40f, 0.34f);
	Fog->bEnableVolumetricFog = true;
	Fog->VolumetricFogExtinctionScale = 0.45f; // Sprint X — was 0.7
	Fog->VolumetricFogAlbedo = FColor(195, 210, 200);

	// Six practicals: brighter sick green / amber accents, no shadows (perf).
	for (int32 i = 0; i < 6; ++i)
	{
		const float A = FMath::DegreesToRadians(i * 60.f + 30.f);
		UPointLightComponent* L = CreateDefaultSubobject<UPointLightComponent>(*FString::Printf(TEXT("GB_Practical_%d"), i));
		L->SetupAttachment(BoundsVolume);
		L->SetRelativeLocation(FVector(FMath::Cos(A) * 1600.f, FMath::Sin(A) * 1600.f, 310.f));
		L->Intensity = 600.f; // Sprint X — was 380
		L->AttenuationRadius = 1500.f;
		L->SetCastShadows(false);
		L->LightColor = (i % 2 == 0) ? FColor(110, 235, 155) : FColor(255, 175, 90);
		PracticalLights.Add(L);
	}

	// Sprint AD — a cool fluorescent under each atrium plate: lights the column, the plate undersides
	// and whoever is on the level below, so the tower stops reading as a backlit silhouette.
	const float PlateZ[3] = { 467.f, 933.f, 1400.f };
	for (int32 i = 0; i < 3; ++i)
	{
		UPointLightComponent* L = CreateDefaultSubobject<UPointLightComponent>(*FString::Printf(TEXT("GB_PlateLight_%d"), i));
		L->SetupAttachment(BoundsVolume);
		L->SetRelativeLocation(FVector(0.f, 0.f, PlateZ[i] - 60.f));
		L->Intensity = 900.f; // UGameConfig::AtriumPlateLightIntensity overrides at BeginPlay
		L->AttenuationRadius = 1100.f;
		L->SetCastShadows(false);
		L->LightColor = FColor(190, 225, 235);
		AtriumPlateLights.Add(L);
	}
}

void AOfficeArena::ApplyConfiguredSurfaceMaterials()
{
	if (bSurfaceMaterialsApplied || !bBuildGreybox || !GameConfig || !GameConfig->bArenaSurfaceMaterials)
	{
		return;
	}
	GameConfig->ResolvePhase8LoadedMeshes();
	int32 Applied = 0;
	for (int32 i = 0; i < GreyboxMeshes.Num(); ++i)
	{
		UStaticMeshComponent* M = GreyboxMeshes[i];
		const EArenaSurface Surface = GreyboxSurfaces.IsValidIndex(i) ? GreyboxSurfaces[i] : EArenaSurface::Colour;
		if (!M || Surface == EArenaSurface::Colour)
		{
			continue;
		}
		UMaterialInterface* Parent = nullptr;
		FLinearColor Tint = FLinearColor::White;
		switch (Surface)
		{
		case EArenaSurface::Floor:    Parent = GameConfig->CachedSurfaceFloor;    Tint = GameConfig->FloorTint; break;
		case EArenaSurface::Concrete: Parent = GameConfig->CachedSurfaceConcrete; Tint = GameConfig->ConcreteTint; break;
		case EArenaSurface::Wall:     Parent = GameConfig->CachedSurfaceWall;     Tint = GameConfig->WallTint; break;
		case EArenaSurface::Steel:    Parent = GameConfig->CachedSurfaceMetal; break;
		case EArenaSurface::Berm:     Parent = GameConfig->CachedSurfaceBerm; break;
		case EArenaSurface::Glass:    Parent = GameConfig->CachedSurfaceGlass; break;
		case EArenaSurface::Neon:     Parent = GameConfig->CachedSurfaceNeon; break;
		case EArenaSurface::Resin:    Parent = GameConfig->CachedSurfaceResin; break;
		default: break;
		}
		if (!Parent)
		{
			continue; // soft-miss → colour MID from ApplyGreyboxColors stays
		}
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Parent, this);
		if (!MID)
		{
			continue;
		}
		// World-projected UVs: project along the block's thin axis (Z → XY, Y → XZ, X → YZ).
		const FVector S = M->GetRelativeScale3D();
		float PlaneSel = 0.f;
		if (S.Y <= S.X && S.Y <= S.Z)      { PlaneSel = 1.f; }
		else if (S.X <= S.Y && S.X <= S.Z) { PlaneSel = 2.f; }
		MID->SetScalarParameterValue(TEXT("PlaneSel"), PlaneSel);
		MID->SetVectorParameterValue(TEXT("Tint"), Tint);
		if (Surface == EArenaSurface::Neon)
		{
			MID->SetVectorParameterValue(TEXT("EmissiveColor"), GreyboxColors.IsValidIndex(i) ? GreyboxColors[i] : FLinearColor::Green);
			MID->SetScalarParameterValue(TEXT("EmissiveStrength"), GameConfig->NeonEmissiveStrength);
			M->SetCastShadow(false);
			NeonMIDs.Add(MID);
			NeonComps.Add(M);
			NeonDipping.Add(false);
			NeonBaseStrength = GameConfig->NeonEmissiveStrength;
		}
		M->SetMaterial(0, MID);
		++Applied;
	}
	for (UPointLightComponent* L : AtriumPlateLights)
	{
		if (L)
		{
			L->SetIntensity(GameConfig->AtriumPlateLightIntensity);
		}
	}
	ApplyResinGrowths();
	bSurfaceMaterialsApplied = Applied > 0;
	UE_LOG(LogNightShift, Log, TEXT("AOfficeArena::ApplyConfiguredSurfaceMaterials — %d blocks textured (floor=%s concrete=%s wall=%s metal=%s glass=%s)."),
		Applied,
		GameConfig->CachedSurfaceFloor ? TEXT("ok") : TEXT("miss"),
		GameConfig->CachedSurfaceConcrete ? TEXT("ok") : TEXT("miss"),
		GameConfig->CachedSurfaceWall ? TEXT("ok") : TEXT("miss"),
		GameConfig->CachedSurfaceMetal ? TEXT("ok") : TEXT("miss"),
		GameConfig->CachedSurfaceGlass ? TEXT("ok") : TEXT("miss"));
}

void AOfficeArena::ApplyResinGrowths()
{
	// Sprint AE — clusters of resin blobs climbing each egg-cover block and spilling onto the floor.
	// Deterministic pseudo-random (seeded per cover) so the layout is stable between runs.
	for (UStaticMeshComponent* Old : ResinGrowthVisuals)
	{
		if (Old) { Old->DestroyComponent(); }
	}
	ResinGrowthVisuals.Reset();
	if (!GameConfig || GameConfig->ResinGrowthsPerCover <= 0 || !GameConfig->CachedSurfaceResin)
	{
		return;
	}
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (!Sphere)
	{
		return;
	}
	int32 CoverIndex = 0;
	for (UBoxComponent* Vol : CoverVolumes)
	{
		if (!Vol || !Vol->GetName().Contains(TEXT("Resin")))
		{
			continue;
		}
		FRandomStream Rand(1337 + 17 * CoverIndex++);
		const FVector C = Vol->GetRelativeLocation();
		const FVector E = Vol->GetUnscaledBoxExtent();
		// Which wall is nearest → blobs lean that way (the infestation came in from the edge).
		const FVector Lean = FVector(FMath::Sign(C.X), FMath::Sign(C.Y), 0.f).GetSafeNormal();
		for (int32 i = 0; i < GameConfig->ResinGrowthsPerCover; ++i)
		{
			const float Scale = Rand.FRandRange(0.45f, 1.35f); // engine sphere is 100 cm
			FVector Off;
			if (i < 3)
			{
				// On top / against the block.
				Off = FVector(Rand.FRandRange(-E.X, E.X) * 0.7f, Rand.FRandRange(-E.Y, E.Y) * 0.7f, E.Z + Scale * 25.f);
			}
			else
			{
				// Spilled onto the floor around it, biased toward the wall, half-sunk.
				const float Ang = Rand.FRandRange(0.f, 2.f * PI);
				const float R = Rand.FRandRange(E.X + 40.f, E.X + 260.f);
				Off = FVector(FMath::Cos(Ang) * R, FMath::Sin(Ang) * R, 0.f) + Lean * Rand.FRandRange(0.f, 120.f);
				Off.Z = -E.Z + Scale * 28.f;
			}
			UStaticMeshComponent* Blob = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
			if (!Blob)
			{
				continue;
			}
			Blob->SetStaticMesh(Sphere);
			Blob->SetupAttachment(BoundsVolume);
			Blob->SetRelativeLocation(C + Off);
			Blob->SetRelativeRotation(FRotator(Rand.FRandRange(-15.f, 15.f), Rand.FRandRange(0.f, 360.f), Rand.FRandRange(-15.f, 15.f)));
			Blob->SetRelativeScale3D(FVector(Scale * Rand.FRandRange(0.9f, 1.3f), Scale, Scale * Rand.FRandRange(0.55f, 0.8f))); // squashed
			Blob->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Blob->SetCastShadow(true);
			if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(GameConfig->CachedSurfaceResin, this))
			{
				MID->SetScalarParameterValue(TEXT("EmissiveStrength"), Rand.FRandRange(0.08f, 0.35f));
				Blob->SetMaterial(0, MID);
			}
			Blob->RegisterComponent();
			ResinGrowthVisuals.Add(Blob);
		}
	}
	UE_LOG(LogNightShift, Log, TEXT("AOfficeArena::ApplyResinGrowths — %d resin blobs on %d covers."), ResinGrowthVisuals.Num(), CoverIndex);
}

void AOfficeArena::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (NeonMIDs.Num() == 0 || !GameConfig)
	{
		return;
	}
	// Sprint AE — dying fluorescents: each strip has its own slow hum + occasional deep dips.
	NeonTime += DeltaSeconds;
	const float Depth = FMath::Clamp(GameConfig->NeonFlickerDepth, 0.f, 1.f);
	const float Rate = FMath::Max(GameConfig->NeonFlickerRate, 0.01f);
	for (int32 i = 0; i < NeonMIDs.Num(); ++i)
	{
		UMaterialInstanceDynamic* MID = NeonMIDs[i];
		if (!MID)
		{
			continue;
		}
		const float T = NeonTime * Rate + i * 7.31f;
		const float Hum = 0.92f + 0.08f * FMath::Sin(T * 23.f);                 // mains buzz
		const float Noise = FMath::PerlinNoise1D(T * 1.7f);                        // -1..1 slow wander
		const float Dip = (Noise > 0.55f) ? 1.f - Depth * FMath::Clamp((Noise - 0.55f) / 0.2f, 0.f, 1.f) : 1.f;
		MID->SetScalarParameterValue(TEXT("EmissiveStrength"), NeonBaseStrength * Hum * Dip);
		// Sprint AF — an electric snap when a strip drops out, once per dip, from the strip's nearest point.
		const bool bDipping = Dip < 0.6f;
		if (NeonDipping.IsValidIndex(i))
		{
			if (bDipping && !NeonDipping[i] && NeonComps.IsValidIndex(i) && NeonComps[i])
			{
				FVector At = NeonComps[i]->GetComponentLocation();
				if (APawn* P = UGameplayStatics::GetPlayerPawn(this, 0))
				{
					// Strips are 50 m long: snap from the point on the strip closest to the player.
					const FVector Axis = NeonComps[i]->GetRelativeScale3D().X > NeonComps[i]->GetRelativeScale3D().Y ? FVector(1, 0, 0) : FVector(0, 1, 0);
					const float Along = FMath::Clamp(FVector::DotProduct(P->GetActorLocation() - At, Axis), -2400.f, 2400.f);
					At += Axis * Along;
				}
				NightShiftAudio::PlayAt(this, GameConfig->CachedSoundNeonSnap, At, GameConfig->SfxVolume * 0.5f, 0.2f);
			}
			NeonDipping[i] = bDipping;
		}
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
		if (bSurfaceMaterialsApplied && GreyboxSurfaces.IsValidIndex(i) && GreyboxSurfaces[i] != EArenaSurface::Colour)
		{
			continue; // Sprint AC — textured already
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
	// Sprint AC — cubicle blocks take the painted-wall texture; racks/resin keep their tints (props sit on racks).
	AddGreyboxBox(BoxName + TEXT("_Mesh"), RelativeLocation, Extent * 2.f, RelativeRotation, Color,
		bResin ? EArenaSurface::Resin : bRack ? EArenaSurface::Colour : EArenaSurface::Wall);
}


void AOfficeArena::ApplyConfiguredCoverMeshes()
{
	// Phase 8 / Sprint L: CoverPropMesh (SM_Cubicle) on cubicle volumes only.
	// Soft ref null → no stamp. Resin/rack volumes keep greybox query boxes only.
	if (!GameConfig)
	{
		return;
	}
	GameConfig->ResolvePhase8LoadedMeshes();
	UStaticMesh* CoverMesh = GameConfig->CachedCoverPropMesh.Get();
	if (!CoverMesh)
	{
		return;
	}

	// Clear prior stamp (soft restart / re-apply).
	for (UStaticMeshComponent* Old : CoverPropVisuals)
	{
		if (Old)
		{
			Old->DestroyComponent();
		}
	}
	CoverPropVisuals.Reset();

	// Sprint L — SM_Cubicle only on cubicle volumes (not resin / racks).
	for (UBoxComponent* Vol : CoverVolumes)
	{
		if (!Vol)
		{
			continue;
		}
		const FString VolName = Vol->GetName();
		if (!VolName.Contains(TEXT("Cubicle"), ESearchCase::IgnoreCase))
		{
			continue;
		}
		UStaticMeshComponent* Vis = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
		if (!Vis)
		{
			continue;
		}
		Vis->SetStaticMesh(CoverMesh);
		Vis->SetupAttachment(Vol);
		Vis->SetRelativeLocation(FVector::ZeroVector);
		Vis->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Vis->SetCastShadow(true);
		Vis->RegisterComponent();
		CoverPropVisuals.Add(Vis);
	}
	UE_LOG(LogNightShift, Log, TEXT("AOfficeArena::ApplyConfiguredCoverMeshes — stamped %d cubicle props (resin/racks skipped)."), CoverPropVisuals.Num());
}



void AOfficeArena::ApplyConfiguredOfficeDressMeshes()
{
	// Sprint N — Omie SM_Desk + SM_Chair near cubicle stamps only (low count).
	// Soft miss → no stamp. Never stamp resin/rack volumes.
	if (!GameConfig)
	{
		return;
	}

	GameConfig->ResolvePhase8LoadedMeshes();
	UStaticMesh* DeskMesh = GameConfig->CachedDeskPropMesh.Get();
	UStaticMesh* ChairMesh = GameConfig->CachedChairPropMesh.Get();
	if (!DeskMesh && !ChairMesh)
	{
		return; // greybox fallback
	}

	for (UStaticMeshComponent* Old : OfficeDressVisuals)
	{
		if (Old)
		{
			Old->DestroyComponent();
		}
	}
	OfficeDressVisuals.Reset();

	auto Stamp = [this](UStaticMesh* Mesh, UBoxComponent* Vol, const FVector& RelLoc, const FRotator& RelRot) -> UStaticMeshComponent*
	{
		if (!Mesh || !Vol)
		{
			return nullptr;
		}
		UStaticMeshComponent* Vis = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
		if (!Vis)
		{
			return nullptr;
		}
		Vis->SetStaticMesh(Mesh);
		Vis->SetupAttachment(Vol);
		Vis->SetRelativeLocation(RelLoc);
		Vis->SetRelativeRotation(RelRot);
		Vis->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Vis->SetCastShadow(true);
		Vis->RegisterComponent();
		OfficeDressVisuals.Add(Vis);
		return Vis;
	};

	// Sprint AD — perimeter work pods: two desks + two chairs on a ring just inside the walls, at the
	// half-angles between the eight edge spawns so nothing spawns into a desk. Faces the atrium.
	const int32 Pods = FMath::Clamp(GameConfig->OfficePodCount, 0, 16);
	const float Ring = GameConfig->OfficePodRingRadiusCm;
	for (int32 i = 0; i < Pods; ++i)
	{
		const float AngleDeg = (360.f / FMath::Max(Pods, 1)) * i + (180.f / FMath::Max(Pods, 1));
		const float A = FMath::DegreesToRadians(AngleDeg);
		const FVector Center(FMath::Cos(A) * Ring, FMath::Sin(A) * Ring, 0.f);
		const FRotator FaceIn(0.f, AngleDeg + 180.f, 0.f);           // props look at the atrium
		const FVector Right = FRotationMatrix(FaceIn).GetUnitAxis(EAxis::Y);
		const FVector Fwd = FRotationMatrix(FaceIn).GetUnitAxis(EAxis::X);
		for (float Side : { -1.f, 1.f })
		{
			const FVector DeskLoc = Center + Right * Side * 95.f;
			const FVector ChairLoc = DeskLoc - Fwd * 85.f;
			if (UStaticMeshComponent* D = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient))
			{
				if (DeskMesh) { D->SetStaticMesh(DeskMesh); }
				D->SetupAttachment(BoundsVolume);
				D->SetRelativeLocation(DeskLoc);
				D->SetRelativeRotation(FaceIn);
				D->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				D->SetCastShadow(true);
				D->RegisterComponent();
				OfficeDressVisuals.Add(D);
			}
			if (ChairMesh)
			{
				if (UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient))
				{
					C->SetStaticMesh(ChairMesh);
					C->SetupAttachment(BoundsVolume);
					C->SetRelativeLocation(ChairLoc);
					C->SetRelativeRotation(FaceIn + FRotator(0.f, (Side < 0.f) ? -12.f : 15.f, 0.f)); // slightly askew, abandoned
					C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
					C->SetCastShadow(true);
					C->RegisterComponent();
					OfficeDressVisuals.Add(C);
				}
			}
		}
	}

	// One desk + one chair per cubicle side (≤8 meshes). Offset toward atrium.
	for (UBoxComponent* Vol : CoverVolumes)
	{
		if (!Vol)
		{
			continue;
		}
		const FString VolName = Vol->GetName();
		if (!VolName.Contains(TEXT("Cubicle"), ESearchCase::IgnoreCase))
		{
			continue;
		}

		FVector DeskOff = FVector::ZeroVector;
		FVector ChairOff = FVector::ZeroVector;
		FRotator FaceIn = FRotator::ZeroRotator; // yaw so props face atrium

		if (VolName.Contains(TEXT("CubicleN"), ESearchCase::IgnoreCase))
		{
			DeskOff = FVector(0.f, -140.f, 0.f);
			ChairOff = FVector(0.f, -230.f, 0.f);
			FaceIn = FRotator(0.f, 180.f, 0.f); // face -Y
		}
		else if (VolName.Contains(TEXT("CubicleS"), ESearchCase::IgnoreCase))
		{
			DeskOff = FVector(0.f, 140.f, 0.f);
			ChairOff = FVector(0.f, 230.f, 0.f);
			FaceIn = FRotator(0.f, 0.f, 0.f); // face +Y
		}
		else if (VolName.Contains(TEXT("CubicleE"), ESearchCase::IgnoreCase))
		{
			DeskOff = FVector(-140.f, 0.f, 0.f);
			ChairOff = FVector(-230.f, 0.f, 0.f);
			FaceIn = FRotator(0.f, -90.f, 0.f); // face -X
		}
		else if (VolName.Contains(TEXT("CubicleW"), ESearchCase::IgnoreCase))
		{
			DeskOff = FVector(140.f, 0.f, 0.f);
			ChairOff = FVector(230.f, 0.f, 0.f);
			FaceIn = FRotator(0.f, 90.f, 0.f); // face +X
		}
		else
		{
			// Generic cubicle name — desk slightly toward origin in local XY of volume.
			DeskOff = FVector(0.f, -140.f, 0.f);
			ChairOff = FVector(0.f, -230.f, 0.f);
		}

		Stamp(DeskMesh, Vol, DeskOff, FaceIn);
		Stamp(ChairMesh, Vol, ChairOff, FaceIn);
	}

	UE_LOG(LogNightShift, Log,
		TEXT("AOfficeArena::ApplyConfiguredOfficeDressMeshes — stamped %d desk/chair props near cubicles."),
		OfficeDressVisuals.Num());
}


void AOfficeArena::ApplyConfiguredServerRackMeshes()
{
	// Sprint Q — ServerRackPropMesh on rack-named volumes only (parallel to cubicle-only).
	// Soft miss → greybox rack blocks. Never stamp cubicles/resin.
	if (!GameConfig)
	{
		return;
	}

	GameConfig->ResolvePhase8LoadedMeshes();
	UStaticMesh* RackMesh = GameConfig->CachedServerRackPropMesh.Get();
	if (!RackMesh)
	{
		return;
	}

	for (UStaticMeshComponent* Old : ServerRackPropVisuals)
	{
		if (Old)
		{
			Old->DestroyComponent();
		}
	}
	ServerRackPropVisuals.Reset();

	for (UBoxComponent* Vol : CoverVolumes)
	{
		if (!Vol)
		{
			continue;
		}
		const FString VolName = Vol->GetName();
		if (!VolName.Contains(TEXT("Rack"), ESearchCase::IgnoreCase))
		{
			continue;
		}

		UStaticMeshComponent* Vis = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
		if (!Vis)
		{
			continue;
		}
		Vis->SetStaticMesh(RackMesh);
		Vis->SetupAttachment(Vol);
		Vis->SetRelativeLocation(FVector::ZeroVector);
		// Preserve angled rack yaw from the cover volume.
		Vis->SetRelativeRotation(FRotator::ZeroRotator);
		Vis->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Vis->SetCastShadow(true);
		Vis->RegisterComponent();
		ServerRackPropVisuals.Add(Vis);
	}

	UE_LOG(LogNightShift, Log,
		TEXT("AOfficeArena::ApplyConfiguredServerRackMeshes — stamped %d rack props (cubicles/resin skipped)."),
		ServerRackPropVisuals.Num());
}

void AOfficeArena::ApplyConfiguredFluorescentMeshes()
{
	// Sprint H/R — Poly Haven SM_MountedFluorescent; ≤4 ceiling mounts at true ceiling Z (not on practical PointLights).
	if (!GameConfig)
	{
		return;
	}
	GameConfig->ResolvePhase8LoadedMeshes();
	UStaticMesh* Fluoro = GameConfig->CachedFluorescentLightMesh.Get();
	if (!Fluoro || !BoundsVolume)
	{
		return;
	}

	for (UStaticMeshComponent* Old : FluorescentPropVisuals)
	{
		if (Old)
		{
			Old->DestroyComponent();
		}
	}
	FluorescentPropVisuals.Reset();

	// Ceiling underside in BoundsVolume-relative space (matches SyncLayoutFromConfig / ClampToBounds).
	float CeilingBottomRelZ = AtriumTowerHeightCm + 150.f; // fallback: tower + headroom − slab half
	if (CeilingClamp)
	{
		CeilingBottomRelZ = CeilingClamp->GetRelativeLocation().Z - CeilingClamp->GetUnscaledBoxExtent().Z;
	}
	const float HangDropCm = 35.f; // fixture hangs just under the ceiling plane
	const float MountZ = CeilingBottomRelZ - HangDropCm;

	// Four fixtures on the six-practical ring XY (indices 0,2,3,5) — keep light mood, fix height.
	TArray<int32> Indices;
	const int32 PracticalCount = PracticalLights.Num();
	if (PracticalCount >= 6)
	{
		Indices = { 0, 2, 3, 5 };
	}
	else
	{
		for (int32 i = 0; i < PracticalCount && Indices.Num() < 4; ++i)
		{
			Indices.Add(i);
		}
	}

	for (int32 Idx : Indices)
	{
		UStaticMeshComponent* Vis = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
		if (!Vis)
		{
			continue;
		}
		Vis->SetStaticMesh(Fluoro);
		Vis->SetupAttachment(BoundsVolume);

		float X = 0.f;
		float Y = 0.f;
		float Yaw = Idx * 90.f + 45.f + 90.f;
		if (UPointLightComponent* Practical = PracticalLights.IsValidIndex(Idx) ? PracticalLights[Idx].Get() : nullptr)
		{
			const FVector P = Practical->GetRelativeLocation();
			X = P.X;
			Y = P.Y;
			Yaw = Idx * 60.f + 30.f + 90.f;
		}
		else
		{
			const float A = FMath::DegreesToRadians(Idx * 90.f + 45.f);
			X = FMath::Cos(A) * 1600.f;
			Y = FMath::Sin(A) * 1600.f;
		}

		Vis->SetRelativeLocation(FVector(X, Y, MountZ));
		Vis->SetRelativeRotation(FRotator(0.f, Yaw, 0.f));
		Vis->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Vis->SetCastShadow(false);
		Vis->SetMobility(EComponentMobility::Movable);
		Vis->RegisterComponent();
		FluorescentPropVisuals.Add(Vis);
	}

	UE_LOG(LogNightShift, Log, TEXT("AOfficeArena::ApplyConfiguredFluorescentMeshes — placed %d ceiling fluorescents at Z=%.0f (ceiling underside)."),
		FluorescentPropVisuals.Num(), MountZ);
}

void AOfficeArena::BeginPlay()
{
	Super::BeginPlay();
	ApplyGreyboxColors();
	SyncLayoutFromConfig();
	RefreshSpawnGather();
	ApplyConfiguredCoverMeshes(); // Phase 8 soft ref — no-op when CoverPropMesh unset
	ApplyConfiguredOfficeDressMeshes(); // Sprint N — desk/chair near cubicles
	ApplyConfiguredServerRackMeshes(); // Sprint Q — rack mesh on rack volumes
	ApplyConfiguredFluorescentMeshes(); // Sprint H — few ceiling fluorescents
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

int32 AOfficeArena::FindNearestSpawnIndex(const FVector& WorldLocation) const
{
	const bool bUseData = AlienSpawnPointData.Num() > 0;
	const int32 Count = bUseData ? AlienSpawnPointData.Num() : AlienSpawnPoints.Num();
	if (Count == 0)
	{
		return -1;
	}
	int32 Best = -1;
	float BestDistSq = TNumericLimits<float>::Max();
	for (int32 Idx = 0; Idx < Count; ++Idx)
	{
		const FVector Loc = bUseData ? AlienSpawnPointData[Idx].Transform.GetLocation() : AlienSpawnPoints[Idx].GetLocation();
		const float DistSq = FVector::DistSquared(Loc, WorldLocation);
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Idx;
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
