using UnrealBuildTool;

public class CkEditorTools : CkModuleRules
{
    public CkEditorTools(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "DeveloperSettings",
            "Slate",
            "SlateCore",

            // For UCk_Plugin_UserSettings_UE base class.
            "CkSettings",
        });
    }
}
