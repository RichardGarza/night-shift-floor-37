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
	if (ServerRackPropMesh.IsNull())
	{
		ServerRackPropMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Imported/Props/Office/SM_ServerRack.SM_ServerRack")));
	}
	if (FluorescentLightMesh.IsNull())
	{
		FluorescentLightMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Imported/Props/Lights/SM_MountedFluorescent.SM_MountedFluorescent")));
	}

	// Sprint X — Quaternius Alien.fbx imported as a skeletal mesh + takes (Scripts/import_alien_skeletal.py).
	auto SoftAnim = [](const TCHAR* Path) { return TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(Path)); };
	if (AlienSkeletalMesh.IsNull())
	{
		AlienSkeletalMesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Imported/Aliens/Skel/SK_Alien.SK_Alien")));
	}
	if (AlienAnims.Idle.IsNull())     { AlienAnims.Idle     = SoftAnim(TEXT("/Game/Imported/Aliens/Skel/SK_AlienCharacterArmature_Idle.SK_AlienCharacterArmature_Idle")); }
	if (AlienAnims.Walk.IsNull())     { AlienAnims.Walk     = SoftAnim(TEXT("/Game/Imported/Aliens/Skel/SK_AlienCharacterArmature_Walk.SK_AlienCharacterArmature_Walk")); }
	if (AlienAnims.Run.IsNull())      { AlienAnims.Run      = SoftAnim(TEXT("/Game/Imported/Aliens/Skel/SK_AlienCharacterArmature_Run.SK_AlienCharacterArmature_Run")); }
	if (AlienAnims.Attack.IsNull())   { AlienAnims.Attack   = SoftAnim(TEXT("/Game/Imported/Aliens/Skel/SK_AlienCharacterArmature_Punch.SK_AlienCharacterArmature_Punch")); }
	if (AlienAnims.HitReact.IsNull()) { AlienAnims.HitReact = SoftAnim(TEXT("/Game/Imported/Aliens/Skel/SK_AlienCharacterArmature_HitReact.SK_AlienCharacterArmature_HitReact")); }
	if (AlienAnims.Death.IsNull())    { AlienAnims.Death    = SoftAnim(TEXT("/Game/Imported/Aliens/Skel/SK_AlienCharacterArmature_Death.SK_AlienCharacterArmature_Death")); }

	// Sprint W — UE template mannequin + rifle (copied from Engine/Templates/TemplateResources).
	if (PlayerSkeletalMesh.IsNull())
	{
		PlayerSkeletalMesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple")));
	}
	if (RifleMesh.IsNull())
	{
		RifleMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Weapons/Rifle/Meshes/SM_Rifle.SM_Rifle")));
	}
	static const TCHAR* R = TEXT("/Game/Characters/Mannequins/Anims/Rifle/");
	auto RifleAnim = [&SoftAnim](const TCHAR* Sub, const TCHAR* Name)
	{
		return SoftAnim(*FString::Printf(TEXT("/Game/Characters/Mannequins/Anims/%s%s.%s"), Sub, Name, Name));
	};
	(void)R;
	if (PlayerAnims.Idle.IsNull())      { PlayerAnims.Idle      = RifleAnim(TEXT("Rifle/"),      TEXT("MF_Rifle_Idle_ADS")); }
	if (PlayerAnims.WalkFwd.IsNull())   { PlayerAnims.WalkFwd   = RifleAnim(TEXT("Rifle/Walk/"), TEXT("MF_Rifle_Walk_Fwd")); }
	if (PlayerAnims.WalkBwd.IsNull())   { PlayerAnims.WalkBwd   = RifleAnim(TEXT("Rifle/Walk/"), TEXT("MF_Rifle_Walk_Bwd")); }
	if (PlayerAnims.WalkLeft.IsNull())  { PlayerAnims.WalkLeft  = RifleAnim(TEXT("Rifle/Walk/"), TEXT("MF_Rifle_Walk_Left")); }
	if (PlayerAnims.WalkRight.IsNull()) { PlayerAnims.WalkRight = RifleAnim(TEXT("Rifle/Walk/"), TEXT("MF_Rifle_Walk_Right")); }
	if (PlayerAnims.JogFwd.IsNull())    { PlayerAnims.JogFwd    = RifleAnim(TEXT("Rifle/Jog/"),  TEXT("MF_Rifle_Jog_Fwd")); }
	if (PlayerAnims.JogBwd.IsNull())    { PlayerAnims.JogBwd    = RifleAnim(TEXT("Rifle/Jog/"),  TEXT("MF_Rifle_Jog_Bwd")); }
	if (PlayerAnims.JogLeft.IsNull())   { PlayerAnims.JogLeft   = RifleAnim(TEXT("Rifle/Jog/"),  TEXT("MF_Rifle_Jog_Left")); }
	if (PlayerAnims.JogRight.IsNull())  { PlayerAnims.JogRight  = RifleAnim(TEXT("Rifle/Jog/"),  TEXT("MF_Rifle_Jog_Right")); }
	if (PlayerAnims.JumpStart.IsNull()) { PlayerAnims.JumpStart = RifleAnim(TEXT("Rifle/Jump/"), TEXT("MM_Rifle_Jump_Start")); }
	if (PlayerAnims.FallLoop.IsNull())  { PlayerAnims.FallLoop  = RifleAnim(TEXT("Rifle/Jump/"), TEXT("MM_Rifle_Jump_Fall_Loop")); }
	if (PlayerAnims.Land.IsNull())      { PlayerAnims.Land      = RifleAnim(TEXT("Rifle/Jump/"), TEXT("MM_Rifle_Jump_Fall_Land")); }
	if (PlayerAnims.Reload.IsNull())    { PlayerAnims.Reload    = RifleAnim(TEXT("Rifle/"),      TEXT("MM_Rifle_Reload")); }
	if (PlayerAnims.Death.IsNull())     { PlayerAnims.Death     = RifleAnim(TEXT("Death/"),      TEXT("MM_Death_Front_01")); }
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
	CachedServerRackPropMesh = ServerRackPropMesh.LoadSynchronous();
	CachedFluorescentLightMesh = FluorescentLightMesh.LoadSynchronous();
	CachedPlayerSkeletalMesh = PlayerSkeletalMesh.LoadSynchronous();
	CachedRifleMesh = RifleMesh.LoadSynchronous();
	CachedPlayerAnims.Idle      = PlayerAnims.Idle.LoadSynchronous();
	CachedPlayerAnims.WalkFwd   = PlayerAnims.WalkFwd.LoadSynchronous();
	CachedPlayerAnims.WalkBwd   = PlayerAnims.WalkBwd.LoadSynchronous();
	CachedPlayerAnims.WalkLeft  = PlayerAnims.WalkLeft.LoadSynchronous();
	CachedPlayerAnims.WalkRight = PlayerAnims.WalkRight.LoadSynchronous();
	CachedPlayerAnims.JogFwd    = PlayerAnims.JogFwd.LoadSynchronous();
	CachedPlayerAnims.JogBwd    = PlayerAnims.JogBwd.LoadSynchronous();
	CachedPlayerAnims.JogLeft   = PlayerAnims.JogLeft.LoadSynchronous();
	CachedPlayerAnims.JogRight  = PlayerAnims.JogRight.LoadSynchronous();
	CachedPlayerAnims.JumpStart = PlayerAnims.JumpStart.LoadSynchronous();
	CachedPlayerAnims.FallLoop  = PlayerAnims.FallLoop.LoadSynchronous();
	CachedPlayerAnims.Land      = PlayerAnims.Land.LoadSynchronous();
	CachedPlayerAnims.Reload    = PlayerAnims.Reload.LoadSynchronous();
	CachedPlayerAnims.Death     = PlayerAnims.Death.LoadSynchronous();
	CachedAlienAnims.Idle     = AlienAnims.Idle.LoadSynchronous();
	CachedAlienAnims.Walk     = AlienAnims.Walk.LoadSynchronous();
	CachedAlienAnims.Run      = AlienAnims.Run.LoadSynchronous();
	CachedAlienAnims.Attack   = AlienAnims.Attack.LoadSynchronous();
	CachedAlienAnims.HitReact = AlienAnims.HitReact.LoadSynchronous();
	CachedAlienAnims.Death    = AlienAnims.Death.LoadSynchronous();
	bPhase8MeshesResolved = true;
	UE_LOG(LogNightShift, Log, TEXT("UGameConfig::ResolvePhase8LoadedMeshes — player=%s rifle=%s alienSkel=%s (idle anim %s / alien run %s)."),
		CachedPlayerSkeletalMesh ? *CachedPlayerSkeletalMesh->GetName() : TEXT("null"),
		CachedRifleMesh ? *CachedRifleMesh->GetName() : TEXT("null"),
		CachedAlienSkeletalMesh ? *CachedAlienSkeletalMesh->GetName() : TEXT("null"),
		CachedPlayerAnims.Idle ? TEXT("ok") : TEXT("null"),
		CachedAlienAnims.Run ? TEXT("ok") : TEXT("null"));
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
