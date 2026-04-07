using System.IO;
using UnrealBuildTool;

public class CkPmg : CkModuleRules
{
    public CkPmg(ReadOnlyTargetRules Target) : base(Target, UseUnityBuild: false)
    {
        PrivateIncludePaths.AddRange(new string[] {
            // ... add other private include paths required here ...
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "ProceduralMeshComponent",
            "MeshDescription",
            "StaticMeshDescription",

            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkLabel",
            "CkLog",

            "CkProvider",
            "CkRecord",
            "CkSettings",
        });

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                "ModelingComponentsEditorOnly",
                "UnrealEd",
            });
        }
    }
}
