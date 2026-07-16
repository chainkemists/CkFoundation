using System.IO;
using UnrealBuildTool;

public class CkInventory : CkModuleRules
{
    public CkInventory(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "AssetRegistry",

            "IrisCore",
            "NetCore",

            "CkAttribute",
            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkGrid",
            "CkLabel",
            "CkLog",

            "CkRecord",
            "CkSettings",
            "CkTagSet",

            "UMG",
            "Slate",
            "SlateCore",
            "InputCore",
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
        });
    }
}
