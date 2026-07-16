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
            }
            );

        PrivateIncludePaths.AddRange(
            new string[] {
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
            }
            );

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "FunctionalTesting"
                }
                );
        }

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