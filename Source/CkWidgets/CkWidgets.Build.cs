using System;
using UnrealBuildTool;

public class CkWidgets : CkModuleRules
{
    public CkWidgets(ReadOnlyTargetRules Target) : base(Target)
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
                "CoreUObject",
                "Engine",
                "InputCore",
                "UMG",
                "Slate",
                "SlateCore",
                "CommonUI",
                "CommonInput",
                // Public: CkInputAction_Widget.h exposes EPlayerMappableKeySlot in its reflected surface.
                "EnhancedInput",
                "CkCore",
                // Public: CkScreen_Utils.h takes FCk_Handle in a reflected signature.
                "CkEcs",
                "CkUICore",
                "CkGraphics",
                "CkInput",
            }
            );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "GameplayTags",
                "Paper2D",
                "RenderCore",

                "CkThirdParty",
                "CkLog",
            }
            );

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(
                new []
                {
                    "UnrealEd"
                }
            );
        }
    }
}
