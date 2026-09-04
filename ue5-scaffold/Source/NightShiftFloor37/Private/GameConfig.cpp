#include "GameConfig.h"
#include "NightShiftFloor37.h"
#include "UObject/SoftObjectPath.h"

UGameConfig::UGameConfig()
{
	// Defaults are set on UPROPERTY initializers to match DESIGN.md.
	// Keep this constructor empty so the Data Asset CDO mirrors those numbers.
}

UGameConfig* UGameConfig::ResolveOrCreate(UObject* Outer, UGameConfig* Existing)
{
	if (Existing)
	{
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
				UE_LOG(LogNightShift, Log, TEXT("UGameConfig::ResolveOrCreate — loaded %s"), AssetPath);
				return AsConfig;
			}
		}
	}

	UObject* OuterObj = Outer ? Outer : GetTransientPackage();
	UGameConfig* Created = NewObject<UGameConfig>(OuterObj, TEXT("RuntimeGameConfig"));
	UE_LOG(LogNightShift, Warning,
		TEXT("UGameConfig::ResolveOrCreate — no /Game/Data/DA_GameConfig; using NewObject DESIGN defaults (PIE-safe). Create the Data Asset in Editor when ready."));
	return Created;
}
