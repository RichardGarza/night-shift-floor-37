using UnrealBuildTool;

public class NightShiftFloor37EditorTarget : TargetRules
{
	public NightShiftFloor37EditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("NightShiftFloor37");
	}
}
