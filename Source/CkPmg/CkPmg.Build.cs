using System.IO;
using UnrealBuildTool;

public class CkPmg : CkModuleRules
{
    public CkPmg(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "Projects",
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
            "CkResourceLoader",
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

        // Stage the bundled fallback fonts (emoji/symbols) so they load in packaged builds too.
        RuntimeDependencies.Add(Path.Combine(PluginDirectory, "Source", "CkPmg", "Resources", "NotoEmoji-Medium.ttf"), StagedFileType.NonUFS);
        RuntimeDependencies.Add(Path.Combine(PluginDirectory, "Source", "CkPmg", "Resources", "NotoSansSymbols2-Regular.ttf"), StagedFileType.NonUFS);
    }
}
