using System.IO;
using UnrealBuildTool;

public class CkPoiDisplayDefinition : CkModuleRules
{
    public CkPoiDisplayDefinition(ReadOnlyTargetRules Target) : base(Target)
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

            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkLabel",
            "CkLog",
            // A display definition is composed on, and resolved off, a POI — the public API takes
            // FCk_Handle_Poi. CkPoi does NOT depend back on this module, so there is no cycle.
            "CkPoi",
            "CkRecord",
            "CkSettings",
            "CkVisibleRange",
        });
    }
}
