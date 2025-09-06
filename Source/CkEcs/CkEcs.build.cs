using System;
using System.IO;
using UnrealBuildTool;

public class CkEcs : CkModuleRules
{
    public CkEcs(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivatePCHHeaderFile = "Public/CkEcs_PCH.h";
        SharedPCHHeaderFile = "Public/CkEcs_PCH.h";

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
                "CoreUObject",
                "Engine",

                "Iris",
                "IrisCore",
                "NetCore",

                "CkThirdParty",
                "CkCore",
                "CkProfile",
                "CkLog",
                "CkMemory",
                "CkProfile",
                "CkSettings",
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
                "FunctionalTesting"
                // ... add private dependencies that you statically link with here ...
            }
            );

        if (Target.Type == TargetRules.TargetType.Editor)
        {
            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "UnrealEd",
                    "AssetTools",
                    "Kismet",
                    "KismetCompiler",
                    "GraphEditor",
                    "BlueprintGraph",
                });
        }
    }
}