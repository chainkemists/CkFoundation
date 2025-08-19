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
                "DeveloperSettings",
                "UMG",
                "CommonUI",

                "CkThirdParty",
                "CkCore",
                "CkEcs",
                "CkEcsExt",
                "CkLog",
                "CkSettings",
                "CkGameSession"
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