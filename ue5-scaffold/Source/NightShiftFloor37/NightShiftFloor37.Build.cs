// Night Shift — Floor 37 | Module build rules
// Target 60fps mid/integrated — keep hot-path deps lean.

using UnrealBuildTool;

public class NightShiftFloor37 : ModuleRules
{
	public NightShiftFloor37(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"UMG",
			"Slate",
			"SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AIModule",
			"NavigationSystem",
			"EngineCameras"
		});
	}
}
