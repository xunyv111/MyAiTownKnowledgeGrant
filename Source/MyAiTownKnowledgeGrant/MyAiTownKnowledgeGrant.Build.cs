using UnrealBuildTool;

public class MyAiTownKnowledgeGrant : ModuleRules
{
    public MyAiTownKnowledgeGrant(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "GameplayTags", "DeveloperSettings", "UMG"
        });
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "HTTP", "Json", "Slate", "SlateCore"
        });
    }
}
