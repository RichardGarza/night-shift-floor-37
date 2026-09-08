#include "GameConfig.h"
#include "NightShiftFloor37.h"
#include "UObject/SoftObjectPath.h"

UGameConfig::UGameConfig()
{
	// DESIGN numeric defaults live on UPROPERTY initializers.
	EnsurePhase8DefaultSoftPaths();
}

void UGameConfig::EnsurePhase8DefaultSoftPaths()
{
	// Sprint G — expected Editor-imported StaticMesh asset paths (FBX staged under Content/Imported).
	// SoftLoad fails quietly → greybox fallback until SoftwareStarter / Editor import lands .uasset.
	if (AlienBodyMesh.IsNull())
	{
		AlienBodyMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Imported/Aliens/SM_Alien.SM_Alien")));
	}
	if (CoverPropMesh.IsNull())
	{
		CoverPropMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Imported/Props/Office/SM_Cubicle.SM_Cubicle")));
	}
	if (DeskPropMesh.IsNull())
	{
		DeskPropMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Imported/Props/Office/SM_Desk.SM_Desk")));
	}
	if (ChairPropMesh.IsNull())
	{
		ChairPropMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Imported/Props/Office/SM_Chair.SM_Chair")));
	}
	if (FluorescentLightMesh.IsNull())
	{
		FluorescentLightMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Imported/Props/Lights/SM_MountedFluorescent.SM_MountedFluorescent")));
	}
}


void UGameConfig::ResolvePhase8LoadedMeshes()
{
	// Sprint O — one LoadSynchronous batch; ApplyConfigured* reuses these pointers.
	if (bPhase8MeshesResolved)
	{
		return;
	}
	EnsurePhase8DefaultSoftPaths();
	CachedAlienBodyMesh = AlienBodyMesh.LoadSynchronous();
	CachedAlienHeadMesh = AlienHeadMesh.LoadSynchronous();
	CachedAlienSkeletalMesh = AlienSkeletalMesh.LoadSynchronous();
	CachedCoverPropMesh = CoverPropMesh.LoadSynchronous();
	CachedDeskPropMesh = DeskPropMesh.LoadSynchronous();
	CachedChairPropMesh = ChairPropMesh.LoadSynchronous();
	CachedFluorescentLightMesh = FluorescentLightMesh.LoadSynchronous();
	bPhase8MeshesResolved = true;
	UE_LOG(LogNightShift, Log, TEXT("UGameConfig::ResolvePhase8LoadedMeshes — cached Phase 8 meshes (body=%s cover=%s fluoro=%s)."),
		CachedAlienBodyMesh ? *CachedAlienBodyMesh->GetName() : TEXT("null"),
		CachedCoverPropMesh ? *CachedCoverPropMesh->GetName() : TEXT("null"),
		CachedFluorescentLightMesh ? *CachedFluorescentLightMesh->GetName() : TEXT("null"));
}

UGameConfig* UGameConfig::ResolveOrCreate(UObject* Outer, UGameConfig* Existing)
{
	if (Existing)
	{
		Existing->EnsurePhase8DefaultSoftPaths();
		Existing->ResolvePhase8LoadedMeshes();
		return Existing;
	}

	static const TCHAR* Paths[] = {
		TEXT("/Game/Data/DA_GameConfig.DA_GameConfig"),
		TEXT("/Game/Data/DA_GameConfig"),
	};

	for (const TCHAR* AssetPath : Paths)
	{
		if (UObject* Loaded = StaticLoadObject(UGameConfig::StaticClass(), nullptr, AssetPath))
		{
			if (UGameConfig* AsConfig = Cast<UGameConfig>(Loaded))
			{
				AsConfig->EnsurePhase8DefaultSoftPaths();
				AsConfig->ResolvePhase8LoadedMeshes();
				UE_LOG(LogNightShift, Log, TEXT("UGameConfig::ResolveOrCreate — loaded %s"), AssetPath);
				return AsConfig;
			}
		}
	}

	UObject* OuterObj = Outer ? Outer : GetTransientPackage();
	UGameConfig* Created = NewObject<UGameConfig>(OuterObj, TEXT("RuntimeGameConfig"));
	Created->ResolvePhase8LoadedMeshes();
	UE_LOG(LogNightShift, Warning,
		TEXT("UGameConfig::ResolveOrCreate — no /Game/Data/DA_GameConfig; using NewObject DESIGN defaults (PIE-safe). Create the Data Asset in Editor when ready."));
	return Created;
}
