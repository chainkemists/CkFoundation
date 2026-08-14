using System;
using UnrealBuildTool;

public class CkWorldSpaceWidget : CkModuleRules
{
    public CkWorldSpaceWidget(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicIncludePaths.AddRange(
            new string[] {
            }
            );

        PrivateIncludePaths.AddRange(
            new string[] {
            }
            );

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CkCore",
                "CkEcs",
                "CkEcsExt",
                "CkUICore",
            }
            );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "UMG",

                "CkThirdParty",
                "CkCore",
                "CkEcs",
                "CkEcsExt",
                "CkLog",
                "CkSettings",
                "CkUICore"
            }
            );
    }
}
