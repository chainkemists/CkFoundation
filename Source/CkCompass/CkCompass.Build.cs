using System.IO;
using UnrealBuildTool;

public class CkCompass : CkModuleRules
{
    public CkCompass(ReadOnlyTargetRules Target) : base(Target)
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

            "UMG",
            "Slate",
            "SlateCore",
            "CommonUI",

            "CkCamera",
            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkLog",
            "CkPoi",
            // Direct consumer of UCk_Poi_DisplayDefinition_PDA symbols (moved there in CkPoi v2 Gate 2);
            // Gate 4 formalizes the full Poi -> EntityTag+PoiDisplayDefinition dependency swap.
            "CkPoiDisplayDefinition",
            "CkUI",
        });
    }
}
