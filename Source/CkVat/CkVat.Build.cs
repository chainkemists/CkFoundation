using System.IO;
using UnrealBuildTool;

public class CkVat : CkModuleRules
{
    public CkVat(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",

            // Shader-directory mapping for /CkVat (module startup)
            "Projects",
            "RenderCore",

            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkGraphics",
            "CkIsmRenderer",
            "CkLog",
            "CkResourceLoader",
            "CkUsf",
        });
    }
}
