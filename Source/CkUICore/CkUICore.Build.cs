using System;
using UnrealBuildTool;

public class CkUICore : CkModuleRules
{
    public CkUICore(ReadOnlyTargetRules Target) : base(Target)
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
                // Public headers expose these directly: CkUI_Types.h -> InputCoreTypes/GameplayTagContainer,
                // CkUI_Utils.h -> Blueprint/UserWidget.h + CommonActivatableWidget + CommonInputTypeEnum.
                "InputCore",
                "GameplayTags",
                "UMG",
                "CommonUI",
                "CommonInput",
                "Slate",
                "SlateCore",
                "CkCore",
                "CkEcs",
            }
            );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
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
