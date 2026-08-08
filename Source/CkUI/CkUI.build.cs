using System;
using UnrealBuildTool;

public class CkUI : CkModuleRules
{
    public CkUI(ReadOnlyTargetRules Target) : base(Target)
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
                "CommonUI",
                "CommonInput",
                // Public: CkInputAction_Widget.h exposes EPlayerMappableKeySlot in its reflected surface.
                "EnhancedInput",
                "CkCore",
                "CkEcs",
                "CkGraphics",
                "CkInput",
            }
            );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "InputCore",
                "Slate",
                "SlateCore",
                "GameplayTags",
                "DeveloperSettings",
                "UMG",
                "CommonUI",
                "CommonInput",
                "Paper2D",
                "RenderCore",

                "CkThirdParty",
                "CkCore",
                "CkEcs",
                "CkLog",
                "CkSettings",
                "CkGameSession"
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