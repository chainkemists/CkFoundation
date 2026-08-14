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
            "CkEntityTag",
            "CkLog",
            "CkPoi",
            // Link-level dep (CkPoi v2 Gate 3 LNK2019): including CkPoiDisplayDefinition_Utils.h pulls
            // CkRecord_Fragment.h, whose FFragment_RecordOfEntityExtensions instantiates the CKRECORD_API
            // FCk_Handle_EntityExtension copy ctor in this module's TUs.
            "CkRecord",
            // Permanent, designed dependency (CkPoi v2 Gate 3): the projector reads per-consumer presentation
            // (priority/offscreen/display asset) from CkPoiDisplayDefinition, keyed by the Compass consumer tag.
            "CkPoiDisplayDefinition",
            "CkVisibleRange",
            "CkUICore",
        });
    }
}
