using System.IO;
using UnrealBuildTool;

public class CkGameSettings : CkModuleRules
{
    public CkGameSettings(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "DeveloperSettings",

            "CkCore",
            "CkCVar",
            "CkLog",
            "CkSettings",

            // Widget layer only (settings screen + keybinding page) — the core registry must never include from these.
            "UMG",
            "Slate",
            "SlateCore",
            "CommonUI",
            "CommonInput",
            "EnhancedInput",
            "InputCore",
            "CkEcs",
            "CkUI",
            "CkUICore",
            "CkInput",
        });
    }
}
