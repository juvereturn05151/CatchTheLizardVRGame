using UnrealBuildTool;

public class CTLProjectEditorTarget : TargetRules
{
    public CTLProjectEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("CTLProject");
    }
}

