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
                "CkUICore",
                "CkMemory",
                "CkEcs",
                "CkJolt",
                "GameplayTags",
                // ... add other public dependencies that you statically link with here ...
            }
            );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "DeveloperSettings",
                "CkSettings",
                "CkLog",
                // ... add private dependencies that you statically link with here ...
            }
            );

        // Build identity (HeadHash + branch merge-bases) is generated centrally by CkCore.Build.cs into
        // CkCore/Public/CkCore/Generated/CkCore_BuildId.h so both the networking layer (GameState /
        // PlayerState version replication) and this watermark UI share a single source of truth.
    }
}
