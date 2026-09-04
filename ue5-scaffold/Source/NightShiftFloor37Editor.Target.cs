using UnrealBuildTool;
using System.Collections.Generic;

public class NightShiftFloor37EditorTarget : TargetRules
{
	public NightShiftFloor37EditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
		ExtraModuleNames.Add("NightShiftFloor37");
	}
}
