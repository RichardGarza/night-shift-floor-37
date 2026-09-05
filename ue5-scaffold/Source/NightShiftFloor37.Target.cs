using UnrealBuildTool;

public class NightShiftFloor37Target : TargetRules
{
	public NightShiftFloor37Target(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("NightShiftFloor37");
	}
}
