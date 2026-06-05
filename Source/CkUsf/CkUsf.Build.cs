using UnrealBuildTool;

public class CkUsf : CkModuleRules
{
    public CkUsf(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicIncludePaths.Add(ModuleDirectory);

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "RenderCore",
            "Projects",
            "GameplayTags",

            "CkCore",
            "CkEcs",
            "CkLog",
            "CkGraphics",
        });
    }
}
