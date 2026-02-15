using System;
using UnrealBuildTool;

public class CkWatermark : CkModuleRules
{
    public CkWatermark(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicIncludePaths.AddRange(
            new string[] {
                // ... add public include paths required here ...
            }
            );

        PrivateIncludePaths.AddRange(
            new string[] {
                // ... add other private include paths required here ...
            }
            );

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CommonUI",
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "UMG",
                "CkCore",
                "CkUI",
                "CkMemory",
                "CkEcs",
                "GameplayTags",
                // ... add other public dependencies that you statically link with here ...
            }
            );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "DeveloperSettings",
                "CkSettings",
                // ... add private dependencies that you statically link with here ...
            }
            );
    }
}
