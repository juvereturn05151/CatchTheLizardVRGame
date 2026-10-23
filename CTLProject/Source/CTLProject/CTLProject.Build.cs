using UnrealBuildTool;

public class CTLProject : ModuleRules
{
    public CTLProject(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "EnhancedInput", "UMG", "HeadMountedDisplay", "XRBase", "InputCore" });
        PrivateDependencyModuleNames.AddRange(new[] { "Slate", "SlateCore" });
        if (Target.bBuildEditor) PrivateDependencyModuleNames.Add("UnrealEd");
    }
}
