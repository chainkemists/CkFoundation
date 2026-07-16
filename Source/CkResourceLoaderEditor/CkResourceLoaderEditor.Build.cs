using System.IO;
using UnrealBuildTool;

public class CkResourceLoaderEditor : CkModuleRules
{
    public CkResourceLoaderEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "DeveloperSettings",
            "Slate",
            "SlateCore",

            "CkCore",
            "CkEcs",
            "CkLog",
            "CkResourceLoader",
            "CkSettings",
        });
    }
}
