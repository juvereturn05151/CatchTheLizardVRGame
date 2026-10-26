using UnrealBuildTool;

public class CTLProject : ModuleRules
{
    public CTLProject(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "EnhancedInput", "UMG", "HeadMountedDisplay", "XRBase", "InputCore" });
        PrivateDependencyModuleNames.AddRange(new[] { "Slate", "SlateCore", "AudioMixer", "Niagara" });
        if (Target.bBuildEditor) PrivateDependencyModuleNames.AddRange(new[] { "UnrealEd", "NavigationSystem", "AnimGraph", "BlueprintGraph", "ModelContextProtocolEngine" });
    }
}
