using UnrealBuildTool;

public class CkWebUmg : CkModuleRules
{
    public CkWebUmg(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicIncludePaths.Add(ModuleDirectory);

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "Slate",
            "SlateCore",
            "Json",
            "Projects",
            "UMG",
            "RenderCore",
            "ImageWrapper",

            "CkCore",
            "CkLog",
            "CkThirdParty",
        });
    }
}
