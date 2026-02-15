using System;
using UnrealBuildTool;

public class CkUI : CkModuleRules
{
    public CkUI(ReadOnlyTargetRules Target) : base(Target)
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
                "UMG",      // Public: stat widget headers include UWidget/Components/Widget.h
                "CkCore",
                "CkEcs",
                "CkEcsExt",
                // ... add other public dependencies that you statically link with here ...
            }
            );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "GameplayTags",
                "CkThirdParty",
                "CkLog",
                "CkMemory",
                "CkGameSession",
                // ... add private dependencies that you statically link with here ...
            }
            );

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(
                new []
                {
                    "UnrealEd"
                }
            );
        }
    }
}