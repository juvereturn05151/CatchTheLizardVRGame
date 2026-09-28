using UnrealBuildTool;

public class CTLProject : ModuleRules
{
    public CTLProject(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "EnhancedInput", "UMG" });
    }
}
