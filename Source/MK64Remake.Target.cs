using UnrealBuildTool;
using System.Collections.Generic;

public class MK64RemakeTarget : TargetRules
{
    public MK64RemakeTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("MK64Remake");
    }
}
