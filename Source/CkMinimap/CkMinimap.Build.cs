using System.IO;
using UnrealBuildTool;

public class CkMinimap : CkModuleRules
{
    public CkMinimap(ReadOnlyTargetRules Target) : base(Target)
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
            "CkEntityTag",
            "CkLabel",
            "CkLog",
            "CkPoi",
            // Permanent, designed dependency (CkPoi v2 Gate 3): the projector reads per-consumer presentation
            // (priority/offscreen) from CkPoiDisplayDefinition, keyed by the Minimap consumer tag.
            "CkPoiDisplayDefinition",
            "CkRecord",
            "CkVisibleRange",
            "CkUI",
        });
    }
}
