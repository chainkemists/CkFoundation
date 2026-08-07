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
            "RHI",
            "Projects",
            "GameplayTags",
            "DeveloperSettings",     // UCk_Usf_Stylize_ProjectSettings_UE derives from UDeveloperSettingsBackedByCVars

            "CkCore",
            "CkEcs",
            "CkLog",
            "CkSettings",
            "CkGraphics",
        });
    }
}
