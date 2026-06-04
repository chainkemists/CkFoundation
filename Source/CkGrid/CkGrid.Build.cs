using System.IO;
using UnrealBuildTool;

public class CkGrid : CkModuleRules
{
    public CkGrid(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
            // ... add other private include paths required here ...
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "Paper2D",
            "GameplayTags",
            "DeveloperSettings",

            "IrisCore",
            "NetCore",

            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkLabel",
            "CkLog",
            "CkRecord",
            "CkSettings",
        });

        // Editor-only: the DebugDrawAll processor draws a grid preview when its owning spawner
        // actor is selected in the editor. Resolving the selected spawner -> preview entity needs
        // UnrealEd (GEditor / USelection) and CkEntitySpawner (ACk_EntitySpawner_UE accessor).
        // None of this is compiled into packaged/runtime builds.
        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                "UnrealEd",
                "CkEntitySpawner",
            });
        }
    }
}