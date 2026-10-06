using UnrealBuildTool;
using System.Collections.Generic;

public class MK64RemakeEditorTarget : TargetRules
{
    public MK64RemakeEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("MK64Remake");
    }
}
