using System;
using UnrealBuildTool;

public class CkCore : CkModuleRules
{
    public CkCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivatePCHHeaderFile = "../CkCommon_PCH.h";

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
                "GameplayTags",
                "MessageLog",
                "NetCore",
                "StructUtils",
                "ToolWidgets",
                "Projects",

                "IrisCore",

                "CkBuildConfig",
                "CkLog",
                "CkSettings",
                "CkThirdParty",
                // ... add other public dependencies that you statically link with here ...
            }
            );

        if (Target.bBuildEditor)
        {
            PublicDependencyModuleNames.AddRange(
                new string[]
                {
                    "UnrealEd",
                    "BlueprintGraph",
                    "GameplayTagsEditor",
                    "PropertyEditor",
                }
                );
        }

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "InputCore",
                "DeveloperSettings",
                "GameplayAbilities",

                "CkLog",
                "CkThirdParty",
                // ... add private dependencies that you statically link with here ...
            }
            );


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
                // ... add any modules that your module loads dynamically here ...
            }
            );
    }
}