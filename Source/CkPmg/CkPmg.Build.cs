using System.IO;
using UnrealBuildTool;

public class CkPmg : CkModuleRules
{
    public CkPmg(ReadOnlyTargetRules Target) : base(Target)
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
            "GeometryCore",
            "GeometryAlgorithms",

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

        // FreeType is an engine ThirdParty lib (used by Slate/Text3D). It is NOT
        // compiled for dedicated-server targets, so gate the glyph code behind a define.
        bool bWithFreeType = Target.Type != TargetType.Server && Target.bCompileFreeType;
        if (bWithFreeType)
        {
            AddEngineThirdPartyPrivateStaticDependencies(Target, "FreeType2");
        }
        PublicDefinitions.Add("CK_PMG_WITH_FREETYPE=" + (bWithFreeType ? "1" : "0"));
    }
}
